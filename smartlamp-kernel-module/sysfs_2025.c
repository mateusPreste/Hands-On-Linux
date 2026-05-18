#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102)");
MODULE_LICENSE("GPL");

#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositivo USB

static char recv_line[MAX_RECV_LINE];              // Buffer para armazenar linha completa recebida
static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saída da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10c4 
#define PRODUCT_ID  0xea60

static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); 
static void usb_disconnect(struct usb_interface *ifce);                            
static int  usb_write_serial(char *cmd, int param);                                
static int  usb_read_serial(char *cmd);                                            

static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

static struct kobj_attribute  led_attribute       = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute       = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  threshold_attribute = __ATTR(threshold, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct attribute      *attrs[]             = { &led_attribute.attr, &ldr_attribute.attr, &threshold_attribute.attr, NULL };
static struct attribute_group attr_group          = { .attrs = attrs };
static struct kobject        *sys_obj;

static int smartlamp_config_serial(struct usb_device *dev)
{
    int ret;
    u32 baudrate = 115200;

    printk(KERN_INFO "SmartLamp: Configurando a porta serial...\n");

    ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                          0x00, 0x41, 0x0001, 0, NULL, 0, 1000);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao habilitar a UART (código %d)\n", ret);
        return ret;
    }

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

static struct usb_driver smartlamp_driver = {
    .name        = "smartlamp",
    .probe       = usb_probe,
    .disconnect  = usb_disconnect,
    .id_table    = id_table,
};

module_usb_driver(smartlamp_driver);

static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;
    int ret;

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");

    smartlamp_device = interface_to_usbdev(interface);
    usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    ret = smartlamp_config_serial(smartlamp_device);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Falha na configuração da serial\n");
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return ret;
    }

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

static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");

    if (sys_obj) {
        sysfs_remove_group(sys_obj, &attr_group);
        kobject_put(sys_obj);
    }

    kfree(usb_in_buffer);
    kfree(usb_out_buffer);
}

static int usb_write_serial(char *cmd, int param) {
    int ret, actual_size;

    if (param >= 0)
        sprintf(usb_out_buffer, "%s %d\n", cmd, param);
    else
        sprintf(usb_out_buffer, "%s\n", cmd);

    printk(KERN_INFO "SmartLamp: Enviando comando: %s", usb_out_buffer);

    ret = usb_bulk_msg(
        smartlamp_device, 
        usb_sndbulkpipe(smartlamp_device, usb_out), 
        usb_out_buffer, 
        strlen(usb_out_buffer), 
        &actual_size, 
        1000
    );

    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro de codigo %d ao enviar comando!\n", ret);
        return -1;
    }

    return 0;
}

static int usb_read_serial(char *cmd)
{
    int ret, actual_size;
    int recv_size = 0;
    int i;
    int value;

    printk(KERN_INFO "SmartLamp: Aguardando resposta do dispositivo...\n");

    ret = usb_bulk_msg(
        smartlamp_device,
        usb_rcvbulkpipe(smartlamp_device, usb_in),
        usb_in_buffer,
        usb_max_size,
        &actual_size,
        2500
    );

    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro na leitura USB (%d)\n", ret);
        return ret;
    }

    for (i = 0; i < actual_size; i++) {
        if (recv_size >= MAX_RECV_LINE - 1) {
            printk(KERN_ERR "SmartLamp: Buffer overflow\n");
            return -1;
        }

        recv_line[recv_size++] = usb_in_buffer[i];

        if (usb_in_buffer[i] == '\n') {
            recv_line[recv_size] = '\0';
            printk(KERN_INFO "SmartLamp: Linha recebida: %s\n", recv_line);

            if (strcmp(cmd, "GET_LDR") == 0) {
                if (sscanf(recv_line, "GET_LDR %d", &value) == 1)
                    return value;
            } else if (strcmp(cmd, "GET_LED") == 0) {
                if (sscanf(recv_line, "GET_LED %d", &value) == 1)
                    return value;
            } else if (strcmp(cmd, "GET_THRESHOLD") == 0) {
                if (sscanf(recv_line, "GET_THRESHOLD %d", &value) == 1)
                    return value;
            } else if (strcmp(cmd, "SET_LED") == 0) {
                if (sscanf(recv_line, "SET_LED %d", &value) == 1)
                    return value;
            } else if (strcmp(cmd, "SET_THRESHOLD") == 0) {
                if (sscanf(recv_line, "SET_THRESHOLD %d", &value) == 1)
                    return value;
            } else {
                printk(KERN_ERR "SmartLamp: comando desconhecido: %s\n", cmd);
                return -1;
            }

            printk(KERN_ERR "SmartLamp: Erro ao converter valor\n");
            return -1;
        }
    }

    return -1;
}

static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff)
{
    int value = -1;
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
    } else {
        printk(KERN_ERR "SmartLamp: atributo inválido: %s\n", attr_name);
        return -EINVAL;
    }

    if (value < 0) {
        printk(KERN_ERR "SmartLamp: erro ao ler %s\n", attr_name);
        return -EIO;
    }

    return sprintf(buff, "%d\n", value);
}

static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count)
{
    long ret, value;
    const char *attr_name = attr->attr.name;

    ret = kstrtol(buff, 10, &value);
    if (ret) {
        printk(KERN_ALERT "SmartLamp: valor de %s invalido.\n", attr_name);
        return -EINVAL;
    }

    printk(KERN_INFO "SmartLamp: Setando %s para %ld ...\n", attr_name, value);

    if (strcmp(attr_name, "ldr") == 0) {
        printk(KERN_ERR "SmartLamp: ldr é somente leitura\n");
        return -EACCES;
    }

    if (strcmp(attr_name, "led") == 0) {
        usb_write_serial("SET_LED", value);
        usb_read_serial("SET_LED");
    } else if (strcmp(attr_name, "threshold") == 0) {
        usb_write_serial("SET_THRESHOLD", value);
        usb_read_serial("SET_THRESHOLD");
    } else {
        printk(KERN_ERR "SmartLamp: atributo inválido: %s\n", attr_name);
        return -EINVAL;
    }

    return count;
}