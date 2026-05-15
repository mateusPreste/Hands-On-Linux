#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102)");
MODULE_LICENSE("GPL");

#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositivo USB

static char recv_line[MAX_RECV_LINE];              // Buffer para armazenar linha completa recebida
static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10C4 /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xEA60 /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                            // Executado quando o dispositivo USB é desconectado da USB
static int  usb_write_serial(char *cmd, int param);                                // Executado para enviar um comando para o dispositivo
static int  usb_read_serial(char *cmd);                                            // Executado para ler a resposta esperada da porta serial

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr, threshold} é lido
// Exemplo: cat /sys/kernel/smartlamp/led
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr, threshold} é escrito
// Exemplo: echo "100" | sudo tee /sys/kernel/smartlamp/led
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

// Variáveis para criar os arquivos em /sys/kernel/smartlamp/{led, ldr, threshold}
static struct kobj_attribute  led_attribute       = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute       = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  threshold_attribute = __ATTR(threshold, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct attribute      *attrs[]             = { &led_attribute.attr, &ldr_attribute.attr, &threshold_attribute.attr, NULL };
static struct attribute_group attr_group          = { .attrs = attrs };
static struct kobject        *sys_obj;

// Função para configurar os parâmetros seriais do CP2102 via Control-Messages
static int smartlamp_config_serial(struct usb_device *dev)
{
    int ret;
    u32 baudrate = 9600; // Defina o baud rate que seu ESP32 usa!

    printk(KERN_INFO "SmartLamp: Configurando a porta serial...\n");

    // 1. Habilita a interface UART do CP2102
    //    Comando específico do vendor Silicon Labs (CP210X_IFC_ENABLE)
    //    bmRequestType: 0x41 (Vendor, Host-to-Device, Interface)
    //    bRequest: 0x00 (CP210X_IFC_ENABLE)
    //    wValue: 0x0001 (UART Enable)
    ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                          0x00, 0x41, 0x0001, 0, NULL, 0, 1000);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao habilitar a UART (código %d)\n", ret);
        return ret;
    }

    // 2. Define o baud rate
    //    Comando específico do vendor Silicon Labs (CP210X_SET_BAUDRATE)
    //    bRequest: 0x1E (CP210X_SET_BAUDRATE)
    ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                          0x1E, 0x41, 0, 0, &baudrate, sizeof(baudrate), 1000);
    if (ret < 0) {
        printk(KERN_ERR "SmartLamp: Erro ao configurar o baud rate (código %d)\n", ret);
        return ret;
    }

    printk(KERN_INFO "SmartLamp: Baud rate configurado para %d\n", baudrate);
    return 0;
}

MODULE_DEVICE_TABLE(usb, id_table);
bool ignore = true;

static struct usb_driver smartlamp_driver = {
    .name        = "smartlamp",     // Nome do driver
    .probe       = usb_probe,       // Executado quando o dispositivo é conectado na USB
    .disconnect  = usb_disconnect,  // Executado quando o dispositivo é desconectado na USB
    .id_table    = id_table,        // Tabela com o VendorID e ProductID do dispositivo
};

module_usb_driver(smartlamp_driver);

// Executado quando o dispositivo é conectado na USB
static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;
    int ret;

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    ignore =  usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    // Chama a função para configurar a porta serial antes de usar
    ret = smartlamp_config_serial(smartlamp_device);
    if (ret)
    {
        printk(KERN_ERR "SmartLamp: Falha na configuração da serial\n");
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return ret;
    }

    // Cria os arquivos /sys/kernel/smartlamp/led, /sys/kernel/smartlamp/ldr e /sys/kernel/smartlamp/threshold
    sys_obj = kobject_create_and_add("smartlamp", kernel_kobj);
    if (!sys_obj) {
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return -ENOMEM;
    }

    ret = sysfs_create_group(sys_obj, &attr_group);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao criar arquivos no sysfs\n");
        kobject_put(sys_obj);
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return ret;
    }

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");

    if (sys_obj) {
        sysfs_remove_group(sys_obj, &attr_group);
        kobject_put(sys_obj);
    }

    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
}

// Envia um comando para o dispositivo USB.
// Esta função deve reaproveitar a implementação feita na Tarefa 2.2.
// Em sysfs_2025.c, param >= 0 indica comando com parâmetro (ex: SET_LED 80)
// e param < 0 indica comando sem parâmetro (ex: GET_LDR).
// Exemplos de uso:
// - usb_write_serial("SET_LED", 80) deve enviar "SET_LED 80\n"
// - usb_write_serial("GET_LDR", -1) deve enviar "GET_LDR\n"
static int usb_write_serial(char *cmd, int param) {
    int ret, actual_size;

    printk(KERN_INFO "SmartLamp: Enviando comando: %s %d\n", cmd, param);

    // TASK 2.4: adapte aqui a solução da Tarefa 2.2.
    // Dica: quando param for negativo, envie apenas "COMANDO\n".
    // Quando param for 0 ou maior, envie "COMANDO PARAMETRO\n".
    // Depois, envie o buffer pela USB usando usb_bulk_msg.

    if (param < 0) { // Comando sem parâmetro (Ex: GET_LDR)
        sprintf(usb_out_buffer, "%s\n", cmd);
    } else {         // Comando com parâmetro (Ex: SET_LED 100)
        sprintf(usb_out_buffer, "%s %d\n", cmd, param);
    }

    ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out),
                       usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000);
    if (ret) {
        return -1;
    }

    return 0;
}

// Lê respostas da porta serial até encontrar uma resposta para o comando esperado.
// Esta função deve reaproveitar a implementação feita na Tarefa 2.3.
// O parâmetro cmd indica qual resposta deve ser aceita.
// Exemplos:
// - usb_read_serial("GET_LDR") deve aceitar "RES GET_LDR 45\n" e retornar 45.
// - usb_read_serial("SET_LED") deve aceitar "RES SET_LED 1\n" e retornar 1.
// Mensagens de outros comandos devem ser ignoradas.
static int usb_read_serial(char *cmd) {
    int ret, actual_size;
    int recv_size = 0;  // Quantidade de caracteres já recebidos em recv_line
    int i;
    int value = -1;
    char prefix[32];

    sprintf(prefix, "RES %s", cmd); // Monta o prefixo esperado (Ex: "RES GET_LDR")

    printk(KERN_INFO "SmartLamp: Aguardando resposta para %s...\n", cmd);

    // TASK 2.4: adapte aqui a solução da Tarefa 2.3.
    //
    // IMPORTANTE: Os dados podem chegar fragmentados (byte a byte ou em blocos)
    // Você deve acumular os dados em recv_line até encontrar o caractere '\n'
    // e processar apenas linhas que comecem com "RES <cmd>".
    //
    // Dicas:
    // - Use um loop para continuar lendo até encontrar '\n'
    // - Use usb_bulk_msg com usb_rcvbulkpipe para cada leitura
    // - Copie os dados de usb_in_buffer para recv_line
    // - Cuidado com buffer overflow: verifique recv_size < MAX_RECV_LINE
    // - Defina um timeout adequado (ex: 2000ms)
    // - Ignore linhas que não correspondem ao comando esperado
    // - Após receber a linha correta, extraia o valor numérico e retorne

    memset(recv_line, 0, MAX_RECV_LINE);

    while (recv_size < MAX_RECV_LINE - 1) {
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in),
                           usb_in_buffer, min(usb_max_size, MAX_RECV_LINE - recv_size), 
                           &actual_size, 2000);
        if (ret) {
            return -1;
        }

        for (i = 0; i < actual_size; i++) {
            recv_line[recv_size] = usb_in_buffer[i];
            
            if (recv_line[recv_size] == '\n') {
                recv_line[recv_size + 1] = '\0';
                
                // Verifica se a resposta recebida corresponde ao comando esperado
                if (strncmp(recv_line, prefix, strlen(prefix)) == 0) {
                    // Pula o tamanho do prefixo para ler apenas o número retornado
                    sscanf(recv_line + strlen(prefix), " %d", &value);
                    return value;
                }
                
                // Se foi uma linha diferente (ex: ping de sensor lido fora de hora), zera e tenta de novo
                recv_size = 0;
                memset(recv_line, 0, MAX_RECV_LINE);
                break; // Quebra o for para realizar novo usb_bulk_msg no while
            } else {
                recv_size++;
            }
        }
    }

    return -1;
}


// Executado quando algum arquivo em /sys/kernel/smartlamp/{led, ldr, threshold} é lido.
// Parâmetros:
// - sys_obj: objeto do kernel que representa o diretório /sys/kernel/smartlamp.
// - attr: atributo que representa o arquivo acessado (led, ldr ou threshold).
// - buff: buffer onde a função deve escrever o texto que será retornado para o usuário.
// Retorno: quantidade de bytes escritos em buff.
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff) {
    int value = -1;
    // attr_name guarda o nome do arquivo sysfs acessado.
    // Exemplos: "led" quando o usuário roda cat /sys/kernel/smartlamp/led,
    // "ldr" quando lê /sys/kernel/smartlamp/ldr e "threshold" para threshold.
    const char *attr_name = attr->attr.name;

    printk(KERN_INFO "SmartLamp: Lendo %s ...\n", attr_name);

    // TASK 2.4: implemente a leitura via sysfs.
    // Use attr_name para identificar se o usuario leu led, ldr ou threshold.
    // Para cada arquivo, envie o comando GET correspondente ao firmware
    // e use usb_read_serial("GET_...") para obter o valor retornado em buff.

    if (strcmp(attr_name, "led") == 0) {
        usb_write_serial("GET_LED", -1);
        value = usb_read_serial("GET_LED");
    } else if (strcmp(attr_name, "ldr") == 0) {
        usb_write_serial("GET_LDR", -1);
        value = usb_read_serial("GET_LDR");
    } else if (strcmp(attr_name, "threshold") == 0) {
        usb_write_serial("GET_THRESHOLD", -1);
        value = usb_read_serial("GET_THRESHOLD");
    }

    sprintf(buff, "%d\n", value);
    return strlen(buff);
}

// Executado quando algum arquivo em /sys/kernel/smartlamp/{led, ldr, threshold} recebe escrita.
// Exemplo: echo 100 | sudo tee /sys/kernel/smartlamp/led
// Parâmetros:
// - sys_obj: objeto do kernel que representa o diretório /sys/kernel/smartlamp.
// - attr: atributo que representa o arquivo escrito (led, ldr ou threshold).
// - buff: buffer com o texto enviado pelo usuário.
// - count: quantidade de bytes recebidos em buff.
// Retorno: quantidade de bytes processados ou código de erro negativo.
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count) {
    long ret, value;
    // attr_name guarda o nome do arquivo sysfs que recebeu a escrita.
    // Use esse valor para decidir se o comando será SET_LED, SET_THRESHOLD
    // ou se a operação deve ser recusada porque ldr é somente leitura.
    const char *attr_name = attr->attr.name;

    // Impede a escrita no LDR
    if (strcmp(attr_name, "ldr") == 0) {
        printk(KERN_ALERT "SmartLamp: Permissão negada. ldr é somente leitura.\n");
        return -EACCES;
    }

    ret = kstrtol(buff, 10, &value);
    if (ret) {
        printk(KERN_ALERT "SmartLamp: valor de %s invalido.\n", attr_name);
        return -EACCES;
    }

    // TASK 2.4: implemente a escrita via sysfs.
    // Use attr_name para permitir escrita em led e threshold.
    // Para led, envie SET_LED com o valor recebido.
    // Para threshold, envie SET_THRESHOLD com o valor recebido.
    // Depois de enviar, leia a resposta do firmware com usb_read_serial("SET_...").
    // O arquivo ldr representa o sensor de luz e deve ser somente leitura.

    if (ret < 0) {
        printk(KERN_ALERT "SmartLamp: erro ao setar o valor do %s.\n", attr_name);
        return -EACCES;
    }

    printk(KERN_INFO "SmartLamp: Setando %s para %ld ...\n", attr_name, value);

    if (strcmp(attr_name, "led") == 0) {
        usb_write_serial("SET_LED", value);
        usb_read_serial("SET_LED"); // Lê a resposta 'RES SET_LED' para esvaziar o buffer
    } else if (strcmp(attr_name, "threshold") == 0) {
        usb_write_serial("SET_THRESHOLD", value);
        usb_read_serial("SET_THRESHOLD"); 
    }

    return count;
}
