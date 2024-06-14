#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/leds.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");


#define MAX_RECV_LINE 996 // Tamanho máximo de uma linha de resposta do dispositvo USB


static char recv_line[MAX_RECV_LINE];              // Armazena dados vindos da USB até receber um caractere de nova linha '\n'
static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB
struct led_classdev *smartlamp_led;

#define VENDOR_ID   0x10C4 /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xea60 /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int  usb_read_serial(void);   
static int usb_send_cmd(char *cmd, int param);

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido (e.g., cat /sys/kernel/smartlamp/led)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/smartlamp/led)
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);   
// Variáveis para criar os arquivos no /sys/kernel/smartlamp/{led, ldr}
static struct kobj_attribute  led_attribute = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  temp_attribute = __ATTR(temp, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  hum_attribute = __ATTR(hum, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct attribute      *attrs[]       = { &led_attribute.attr, &ldr_attribute.attr, &temp_attribute.attr, &hum_attribute.attr, NULL };
static struct attribute_group attr_group    = { .attrs = attrs };
static struct kobject        *sys_obj;                                             // Executado para ler a saida da porta serial

MODULE_DEVICE_TABLE(usb, id_table);

bool ignore = true;
int LDR_value = 0;

static struct usb_driver smartlamp_driver = {
    .name        = "smartlamp",     // Nome do driver
    .probe       = usb_probe,       // Executado quando o dispositivo é conectado na USB
    .disconnect  = usb_disconnect,  // Executado quando o dispositivo é desconectado na USB
    .id_table    = id_table,        // Tabela com o VendorID e ProductID do dispositivo
};

module_usb_driver(smartlamp_driver);

static int led_brightness_set(struct led_classdev *led_cdev, enum led_brightness value)
{
    int ret;

    // pr_info("SmartLamp: Setting Value: %d\n", value);

    if(value == LED_OFF){
        ret = usb_send_cmd("SET_LED", 0);
    }
    else if(value == LED_ON){
        ret = usb_send_cmd("SET_LED", 100);
    }
    else if(value == LED_HALF){
        ret = usb_send_cmd("SET_LED", 50);
    }
    else if(value == LED_FULL){
        ret = usb_send_cmd("SET_LED", 100);
    }

    return 0;
}

// Executado quando o dispositivo é conectado na USB
static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");
    int err;

    // Cria arquivos do /sys/kernel/smartlamp/*
    sys_obj = kobject_create_and_add("smartlamp", kernel_kobj);
    ignore = sysfs_create_group(sys_obj, &attr_group); // AQUI

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    ignore =  usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);  // AQUI
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    smartlamp_led = devm_kzalloc(&interface->dev, sizeof(*smartlamp_led), GFP_KERNEL);

    if (!smartlamp_led)
		return -ENOMEM;
        
    smartlamp_led->name = "smartlamp:led";
	smartlamp_led->brightness_set_blocking = led_brightness_set;
	smartlamp_led->flags = LED_CORE_SUSPENDRESUME;
	smartlamp_led->max_brightness = 255;

    err = devm_led_classdev_register(&interface->dev, smartlamp_led);
	if (err) {
		dev_err(&interface->dev, "failed to register white LED\n");
		return err;
	}

    LDR_value = usb_read_serial();

    printk("LDR Value: %d\n", LDR_value);

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    devm_led_classdev_unregister(&interface->dev, smartlamp_led);
    kfree(smartlamp_led);
    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
    kobject_put(sys_obj);
   
}

static char **extract_cmd_value(char *command) {

    int counter = 0;
    int i;
    int j = 0;
    char **cmd_value = kmalloc(2, GFP_KERNEL);
    char *value = kmalloc(20, GFP_KERNEL);
    char *cmd = kmalloc(20, GFP_KERNEL);

    for(i = 0; i < strlen(command); i++){

        if(command[i] == ' '){
            counter++;
        }
        
        if(counter == 2){
            cmd[i] = '\0';
            i++;
            break;
        }

        cmd[i] = command[i];
    }
        
    for(; i < strlen(command); i++) {
        if(command[i] == '\r')
        {
            break;
        }
        
        value[j] = command[i];
        j++;
    }

    value[j] = '\0';

    cmd_value[0] = cmd;
    cmd_value[1] = value;

    return cmd_value;
}

static int usb_read_serial() {
    int ret, actual_size;
    int retries = 10;  // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    char *response_prefix = "RES GET_LDR ";
    int response_prefix_len = strlen(response_prefix);
    int ldr_value = -1;
    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0) {
        // Lê os dados da porta serial e armazena em usb_in_buffer
        // usb_in_buffer - contem a resposta em string do dispositivo
        // actual_size - contem o tamanho da resposta em bytes
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 5000);
         if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", ret, retries--);
            continue;
        }
        usb_in_buffer[actual_size] = '\0'; // Assegura que o buffer está terminado em null
        printk(KERN_INFO "SmartLamp: Dados recebidos: %s\n", usb_in_buffer);
        // Verifica se a resposta começa com "RES GET_LDR "
        if (strncmp(usb_in_buffer, response_prefix, response_prefix_len) == 0) {
            // Extrai o valor após o prefixo
            ldr_value = simple_strtol(usb_in_buffer + response_prefix_len, NULL, 10);
            printk(KERN_INFO "SmartLamp: LDR Value recebido: %d\n", ldr_value);
            return ldr_value;
        }

        if(strcmp("\n", usb_in_buffer) == 0) break;
    }
    return -1;
}

// Envia um comando via USB, espera e retorna a resposta do dispositivo (convertido para int)
// Exemplo de Comando:  SET_LED 80
// Exemplo de Resposta: RES SET_LED 1
// Exemplo de chamada da função usb_send_cmd para SET_LED: usb_send_cmd("SET_LED", 80);
static int usb_send_cmd(char *cmd, int param) {
    int recv_size = 0;                      // Quantidade de caracteres no recv_line
    int ret, actual_size;//, i;
    int retries = 10;                       // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    char resp_expected[MAX_RECV_LINE];      // Resposta esperada do comando
    // char *resp_pos;                         // Posição na linha lida que contém o número retornado pelo dispositivo
    long resp_number = -1;                  // Número retornado pelo dispositivo (e.g., valor do led, valor do ldr)

    printk(KERN_INFO "SmartLamp: Enviando comando: %s\n", cmd);

    // preencha o buffer                     // Caso contrário, é só o comando mesmo
    if(param == -9999)
        sprintf(usb_out_buffer, "%s", cmd);
    else
        sprintf(usb_out_buffer, "%s %d", cmd, param);
    // Envia o comando (usb_out_buffer) para a USB
    pr_info("SmartLamp: usb_out_buffer: %s", usb_out_buffer);
    // Procure a documentação da função usb_bulk_msg para entender os parâmetros
    ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out), usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro de codigo %d ao enviar comando!\n", ret);
        return -1;
    }

    sprintf(resp_expected, "RES %s", cmd);  // Resposta esperada. Ficará lendo linhas até receber essa resposta.

    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0) {
        // Lê dados da USB
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", ret, retries--);
            continue;
        }

        recv_size++;
        strcat(recv_line, usb_in_buffer);

        if(strcmp("\n", usb_in_buffer) == 0) {
            recv_line[recv_size - 1] = '\0'; // Assegura que o buffer está terminado em null
            printk(KERN_INFO "SmartLamp: Dados recebidos: %s\n", recv_line);
            // Verifica se a resposta começa com "RES GET_LDR "
            char **cmd_value = extract_cmd_value(recv_line);
            if (strncmp(cmd_value[0], resp_expected, strlen(resp_expected)) == 0) {
                // Extrai o valor após o prefixo
                resp_number = simple_strtol(cmd_value[1], NULL, 10);
                printk(KERN_INFO "SmartLamp: Valor recebido: %ld\n", resp_number);
            }
            kfree(cmd_value);
            break;
        }
        
    }

    recv_size = 0;
    memset(recv_line, 0, MAX_RECV_LINE);
    return resp_number; // Não recebi a resposta esperada do dispositivo
}

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido (e.g., cat /sys/kernel/smartlamp/led)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff) {
    // value representa o valor do led ou ldr
    int value;
    // attr_name representa o nome do arquivo que está sendo lido (ldr ou led)
    const char *attr_name = attr->attr.name;

    // printk indicando qual arquivo está sendo lido
    printk(KERN_INFO "SmartLamp: Lendo %s ...\n", attr_name);

    // Implemente a leitura do valor do led ou ldr usando a função usb_send_cmd()
    if(strcmp(attr_name, "led") == 0)
    {
        value = usb_send_cmd("GET_LED", -9999);
    }
    else if(strcmp(attr_name, "ldr") == 0)
    {
        value = usb_send_cmd("GET_LDR", -9999);
    }
    else if(strcmp(attr_name, "temp") == 0)
    {
        value = usb_send_cmd("GET_TEMP", -9999);
    }
    else if(strcmp(attr_name, "hum") == 0)
    {
        value = usb_send_cmd("GET_HUM", -9999);
    }

    sprintf(buff, "%d\n", value);                   // Cria a mensagem com o valor do led, ldr
    return strlen(buff);
}


// Essa função não deve ser alterada durante a task sysfs
// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/smartlamp/led)
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count) {
    long ret, value;
    const char *attr_name = attr->attr.name;

    // Converte o valor recebido para long
    ret = kstrtol(buff, 10, &value);
    if (ret) {
        printk(KERN_ALERT "SmartLamp: valor de %s invalido.\n", attr_name);
        return -EACCES;
    }

    printk(KERN_INFO "SmartLamp: Setando %s para %ld ...\n", attr_name, value);

    // utilize a função usb_send_cmd para enviar o comando SET_LED X
    ret = usb_send_cmd("SET_LED", value);

    if (ret < 0) {
        printk(KERN_ALERT "SmartLamp: erro ao setar o valor do %s.\n", attr_name);
        return -EACCES;
    }

    return strlen(buff);
}