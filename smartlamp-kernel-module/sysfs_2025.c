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

#define VENDOR_ID   SUBSTITUA_PELO_VENDORID /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  SUBSTITUA_PELO_PRODUCTID /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                            // Executado quando o dispositivo USB é desconectado da USB
static int  usb_write_serial(char *cmd, int param);                                // Executado para enviar um comando para o dispositivo
static int  usb_read_serial(char *cmd);                                            // Executado para ler a resposta esperada da porta serial

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido
// Exemplo: cat /sys/kernel/smartlamp/led
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito
// Exemplo: echo "100" | sudo tee /sys/kernel/smartlamp/led
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

// Variáveis para criar os arquivos em /sys/kernel/smartlamp/{led, ldr}
static struct kobj_attribute  led_attribute = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct attribute      *attrs[]       = { &led_attribute.attr, &ldr_attribute.attr, NULL };
static struct attribute_group attr_group    = { .attrs = attrs };
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

    // Cria os arquivos /sys/kernel/smartlamp/led e /sys/kernel/smartlamp/ldr
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

// Envia um comando para o dispositivo USB
// Exemplo de uso: usb_write_serial("SET_LED", 80);
// Exemplo de uso: usb_write_serial("GET_LDR", -1);
static int usb_write_serial(char *cmd, int param) {
    int ret, actual_size;

    printk(KERN_INFO "SmartLamp: Enviando comando: %s\n", cmd);

    if (param >= 0)
        sprintf(usb_out_buffer, "%s %d\n", cmd, param);
    else
        sprintf(usb_out_buffer, "%s\n", cmd);

    ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out),
                       usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao enviar comando (código %d)\n", ret);
        return -1;
    }

    printk(KERN_INFO "SmartLamp: Comando enviado com sucesso\n");
    return 0;
}

// Lê a resposta do dispositivo para o comando informado.
// Exemplo de resposta esperada para GET_LDR: "RES GET_LDR 450\n"
static int usb_read_serial(char *cmd) {
    int ret, actual_size;
    int recv_size = 0;
    int retries = 10;
    int i;
    char resp_expected[MAX_RECV_LINE];
    char *resp_pos;
    long resp_number = -1;

    sprintf(resp_expected, "RES %s", cmd);
    printk(KERN_INFO "SmartLamp: Aguardando resposta: %s\n", resp_expected);

    while (retries > 0) {
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in),
                           usb_in_buffer, min(usb_max_size, MAX_RECV_LINE),
                           &actual_size, 1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Código: %d\n", retries, ret);
            retries--;
            continue;
        }

        for (i = 0; i < actual_size; i++) {
            if (usb_in_buffer[i] == '\n') {
                recv_line[recv_size] = '\0';
                printk(KERN_INFO "SmartLamp: Linha recebida: '%s'\n", recv_line);

                if (!strncmp(recv_line, resp_expected, strlen(resp_expected))) {
                    resp_pos = &recv_line[strlen(resp_expected) + 1];
                    ret = kstrtol(resp_pos, 10, &resp_number);
                    if (ret) {
                        printk(KERN_ERR "SmartLamp: Resposta inválida para %s\n", cmd);
                        return -1;
                    }

                    return resp_number;
                }

                recv_size = 0;
                retries--;
            } else if (recv_size < MAX_RECV_LINE - 1) {
                recv_line[recv_size++] = usb_in_buffer[i];
            } else {
                printk(KERN_ERR "SmartLamp: Linha recebida excedeu o tamanho máximo\n");
                recv_size = 0;
                retries--;
            }
        }
    }

    return -1;
}

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff) {
    int value;
    const char *attr_name = attr->attr.name;

    printk(KERN_INFO "SmartLamp: Lendo %s ...\n", attr_name);

    if (!strcmp(attr_name, "led")) {
        usb_write_serial("GET_LED", -1);
        value = usb_read_serial("GET_LED");
    } else {
        usb_write_serial("GET_LDR", -1);
        value = usb_read_serial("GET_LDR");
    }

    sprintf(buff, "%d\n", value);
    return strlen(buff);
}

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count) {
    long ret, value;
    const char *attr_name = attr->attr.name;

    ret = kstrtol(buff, 10, &value);
    if (ret) {
        printk(KERN_ALERT "SmartLamp: valor de %s invalido.\n", attr_name);
        return -EACCES;
    }

    printk(KERN_INFO "SmartLamp: Setando %s para %ld ...\n", attr_name, value);

    if (!strcmp(attr_name, "led")) {
        ret = usb_write_serial("SET_LED", value);
        if (!ret)
            ret = usb_read_serial("SET_LED");
    }
    else {
        printk(KERN_ALERT "SmartLamp: o valor do ldr (sensor de luz) eh apenas para leitura.\n");
        return -EACCES;
    }

    if (ret < 0) {
        printk(KERN_ALERT "SmartLamp: erro ao setar o valor do %s.\n", attr_name);
        return -EACCES;
    }

    return strlen(buff);
}
