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

#define VENDOR_ID   0x10c4 /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xea60 /* Encontre o ProductID do smartlamp */
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
    if (ret)
    {
        printk(KERN_ERR "SmartLamp: Erro ao habilitar a UART (código %d)\n", ret);
        return ret;
    }

    // 2. Define o baud rate
    //    Comando específico do vendor Silicon Labs (CP210X_SET_BAUDRATE)
    //    bRequest: 0x1E (CP210X_SET_BAUDRATE)
    ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                          0x1E, 0x41, 0, 0, &baudrate, sizeof(baudrate), 1000);
    if (ret < 0)
    {
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

    // Quando param for negativo, envia apenas "COMANDO\n".
    // Quando param for 0 ou maior, envia "COMANDO PARAMETRO\n".
    if (param >= 0)
        sprintf(usb_out_buffer, "%s %d\n", cmd, param);
    else
        sprintf(usb_out_buffer, "%s\n", cmd);

    // Envia o buffer pela USB usando usb_bulk_msg
    ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out),
                       usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao enviar comando (código %d)\n", ret);
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
    char expected_prefix[MAX_RECV_LINE];
    bool message_complete = false;

    printk(KERN_INFO "SmartLamp: Aguardando resposta para %s...\n", cmd);

    // Monta o prefixo esperado: "RES GET_LDR", "RES SET_LED", etc.
    snprintf(expected_prefix, sizeof(expected_prefix), "RES %s", cmd);

    // Loop externo: continua até encontrar a resposta correta para o comando
    while (1) {
        // Limpa o buffer e reinicia o acúmulo para cada nova linha
        memset(recv_line, 0, MAX_RECV_LINE);
        recv_size = 0;
        message_complete = false;

        // Loop interno: acumula bytes até encontrar '\n' (baseado na Tarefa 2.3)
        while (!message_complete) {
            ret = usb_bulk_msg(smartlamp_device,
                               usb_rcvbulkpipe(smartlamp_device, usb_in),
                               usb_in_buffer,
                               usb_max_size,
                               &actual_size,
                               2000); // Timeout de 2000ms

            if (ret) {
                printk(KERN_ERR "SmartLamp: Erro ao ler porta USB (Codigo: %d)\n", ret);
                return -1;
            }

            // Copia os dados recebidos para recv_line, acumulando byte a byte
            for (i = 0; i < actual_size; i++) {
                if (recv_size < MAX_RECV_LINE - 1) { // Proteção contra buffer overflow
                    recv_line[recv_size] = usb_in_buffer[i];

                    // Se achou o caractere de nova linha, finaliza a string e sai
                    if (recv_line[recv_size] == '\n') {
                        recv_line[recv_size + 1] = '\0';
                        message_complete = true;
                        break;
                    }
                    recv_size++;
                } else {
                    printk(KERN_WARNING "SmartLamp: Buffer cheio! Truncando string.\n");
                    recv_line[MAX_RECV_LINE - 1] = '\0';
                    message_complete = true;
                    break;
                }
            }
        }

        // Imprime a linha inteira acumulada no buffer
        printk(KERN_INFO "SmartLamp: Mensagem bruta recebida: %s", recv_line);

        // Ignora linhas que não começam com "RES <cmd>"
        if (strncmp(recv_line, expected_prefix, strlen(expected_prefix)) != 0) {
            printk(KERN_INFO "SmartLamp: Ignorando linha (esperava '%s')\n", expected_prefix);
            continue;
        }

        // Extrai o valor numérico da resposta e retorna
        if (sscanf(recv_line, "RES %*s %d", &value) == 1) {
            printk(KERN_INFO "SmartLamp: Valor extraído de %s: %d\n", cmd, value);
            return value;
        }

        printk(KERN_WARNING "SmartLamp: Nao foi possivel extrair o valor da resposta.\n");
        return -1;
    }
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

    ret = kstrtol(buff, 10, &value);
    if (ret) {
        printk(KERN_ALERT "SmartLamp: valor de %s invalido.\n", attr_name);
        return -EACCES;
    }

    printk(KERN_INFO "SmartLamp: Setando %s para %ld ...\n", attr_name, value);

    // ldr é somente leitura: recusa escrita
    if (strcmp(attr_name, "ldr") == 0) {
        printk(KERN_ALERT "SmartLamp: ldr e somente leitura.\n");
        return -EACCES;
    }

    if (strcmp(attr_name, "led") == 0) {
        ret = usb_write_serial("SET_LED", (int)value);
        if (ret == 0)
            ret = usb_read_serial("SET_LED");

    } else if (strcmp(attr_name, "threshold") == 0) {
        ret = usb_write_serial("SET_THRESHOLD", (int)value);
        if (ret == 0)
            ret = usb_read_serial("SET_THRESHOLD");
    }

    if (ret < 0) {
        printk(KERN_ALERT "SmartLamp: erro ao setar o valor do %s.\n", attr_name);
        return -EACCES;
    }

    return strlen(buff);
}