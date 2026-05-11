#include <linux/module.h>
#include <linux/usb.h>
#include <linux/random.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver SmartLamp com sysfs e valores aleatórios");
MODULE_LICENSE("GPL");

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id);
static void usb_disconnect(struct usb_interface *ifce);
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

// Arquivos criados em /sys/kernel/smartlamp/{led, ldr}
static struct kobj_attribute  led_attribute = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct attribute      *attrs[]       = { &led_attribute.attr, &ldr_attribute.attr, NULL };
static struct attribute_group attr_group    = { .attrs = attrs };
static struct kobject        *sys_obj;

// CP2102, chip USB-Serial usado pelo ESP32.
#define VENDOR_ID  0x10C4
#define PRODUCT_ID 0xEA60

static const struct usb_device_id id_table[] = {
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    {}
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver smartlamp_driver = {
    .name       = "smartlamp_random",
    .probe      = usb_probe,
    .disconnect = usb_disconnect,
    .id_table   = id_table,
};

module_usb_driver(smartlamp_driver);

static int smartlamp_random_value(void)
{
    return get_random_u32_below(101);
}

static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    int ret;

    printk(KERN_INFO "SmartLamp Random: Dispositivo conectado.\n");

    sys_obj = kobject_create_and_add("smartlamp", kernel_kobj);
    if (!sys_obj)
        return -ENOMEM;

    ret = sysfs_create_group(sys_obj, &attr_group);
    if (ret) {
        printk(KERN_ERR "SmartLamp Random: Erro ao criar arquivos no sysfs.\n");
        kobject_put(sys_obj);
        sys_obj = NULL;
        return ret;
    }

    return 0;
}

static void usb_disconnect(struct usb_interface *interface)
{
    printk(KERN_INFO "SmartLamp Random: Dispositivo desconectado.\n");

    if (sys_obj) {
        sysfs_remove_group(sys_obj, &attr_group);
        kobject_put(sys_obj);
        sys_obj = NULL;
    }
}

static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff)
{
    int value = smartlamp_random_value();

    printk(KERN_INFO "SmartLamp Random: Lendo %s = %d\n", attr->attr.name, value);

    return sprintf(buff, "%d\n", value);
}

static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count)
{
    long value;
    int ret;

    if (strcmp(attr->attr.name, "led")) {
        printk(KERN_ALERT "SmartLamp Random: o valor do ldr eh apenas para leitura.\n");
        return -EACCES;
    }

    ret = kstrtol(buff, 10, &value);
    if (ret || value < 0 || value > 100) {
        printk(KERN_ALERT "SmartLamp Random: valor de led invalido.\n");
        return -EINVAL;
    }

    printk(KERN_INFO "SmartLamp Random: Escrita em led recebida: %ld\n", value);
    return count;
}
