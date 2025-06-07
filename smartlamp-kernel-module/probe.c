#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");

#define MAX_RECV_LINE 100

static struct usb_device *smartlamp_device;
static uint usb_in, usb_out;
static char *usb_in_buffer, *usb_out_buffer;
static int usb_max_size;

#define VENDOR_ID    0x0000  // Valor temporário
#define PRODUCT_ID   0x0000  // Valor temporário
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int usb_probe(struct usb_interface *ifce, const struct usb_device_id *id);
static void usb_disconnect(struct usb_interface *ifce);
static int usb_read_serial(void);
static int usb_write_serial(const char *cmd, int param);

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver smartlamp_driver = {
    .name        = "smartlamp",
    .probe       = usb_probe,
    .disconnect  = usb_disconnect,
    .id_table    = id_table,
};

static int smartlamp_open(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t smartlamp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    int val = usb_read_serial();
    char output[32];
    int len = snprintf(output, sizeof(output), "%d\n", val);
    return simple_read_from_buffer(buf, count, ppos, output, len);
}

static ssize_t smartlamp_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    char input[32];
    if (count >= sizeof(input)) return -EINVAL;
    if (copy_from_user(input, buf, count)) return -EFAULT;
    input[count] = '\0';
    return usb_write_serial("CMD", simple_strtol(input, NULL, 10)) ? -EIO : count;
}

static const struct file_operations smartlamp_fops = {
    .owner = THIS_MODULE,
    .open = smartlamp_open,
    .read = smartlamp_read,
    .write = smartlamp_write,
};

static struct miscdevice smartlamp_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "smartlamp",
    .fops = &smartlamp_fops,
    .mode = 0666,
};

module_usb_driver(smartlamp_driver);

static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in = NULL, *usb_endpoint_out = NULL;
    int ret;

    smartlamp_device = interface_to_usbdev(interface);

    ret = usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);
    if (ret) return ret;

    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;

    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    if (!usb_in_buffer || !usb_out_buffer) {
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return -ENOMEM;
    }

    ret = misc_register(&smartlamp_misc_device);
    if (ret) {
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return ret;
    }

    return 0;
}

static void usb_disconnect(struct usb_interface *interface) {
    misc_deregister(&smartlamp_misc_device);
    kfree(usb_in_buffer);
    kfree(usb_out_buffer);
}

static int usb_read_serial(void) {
    int ret, actual_size;
    int retries = 10;

    while (retries-- > 0) {
        memset(usb_in_buffer, 0, usb_max_size);

        ret = usb_bulk_msg(smartlamp_device,
                           usb_rcvbulkpipe(smartlamp_device, usb_in),
                           usb_in_buffer,
                           min(usb_max_size, MAX_RECV_LINE),
                           &actual_size,
                           1000);
        if (!ret && actual_size > 0) {
            return simple_strtol(usb_in_buffer, NULL, 10);
        }

        msleep(100);
    }

    return -EIO;
}

static int usb_write_serial(const char *cmd, int param) {
    int ret, actual_size;

    snprintf(usb_out_buffer, usb_max_size, "%s %d\n", cmd, param);

    ret = usb_bulk_msg(smartlamp_device,
                       usb_sndbulkpipe(smartlamp_device, usb_out),
                       usb_out_buffer,
                       strlen(usb_out_buffer),
                       &actual_size,
                       1000);

    return ret;
}
