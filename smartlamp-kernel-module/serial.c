#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102)");
MODULE_LICENSE("GPL");

#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositivo USB
#define TEST_COMMAND "GET_LDR"

static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saída da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10c4  /* Encontre o VendorID do smartlamp */
#define PRODUCT_ID  0xea60  /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int usb_probe(struct usb_interface *ifce, const struct usb_device_id *id);
static void usb_disconnect(struct usb_interface *ifce);
static int usb_read_serial(void);
char* concat(const char *s1, const char *s2);

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver smartlamp_driver = {
    .name       = "smartlamp",
    .probe      = usb_probe,
    .disconnect = usb_disconnect,
    .id_table   = id_table,
};

module_usb_driver(smartlamp_driver);

static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in = NULL, *usb_endpoint_out = NULL;
    struct usb_host_interface *interface_desc;
    int i;

    printk(KERN_INFO "SmartLamp:     Dispositivo conectado ...\n");

    smartlamp_device = interface_to_usbdev(interface);

    interface_desc = interface->cur_altsetting;

    // Itera sobre os endpoints da interface
    for (i = 0; i < interface_desc->desc.bNumEndpoints; ++i) {
        struct usb_endpoint_descriptor *endpoint = &interface_desc->endpoint[i].desc;

        if (usb_endpoint_dir_in(endpoint)) {
            usb_endpoint_in = endpoint;
        } else if (usb_endpoint_dir_out(endpoint)) {
            usb_endpoint_out = endpoint;
        }
    }

    if (!usb_endpoint_in || !usb_endpoint_out) {
        printk(KERN_ERR "SmartLamp: Não conseguiu encontrar endpoints.\n");
        return -ENODEV;
    }

    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;

    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    if (!usb_in_buffer || !usb_out_buffer) {
        printk(KERN_ERR "SmartLamp: Falha na alocação de buffers.\n");
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return -ENOMEM;
    }
    
    // test_serial_communication();

    int LDR_value = usb_read_serial();
    printk(KERN_INFO "SmartLamp: LDR_VALUE %d\n", LDR_value);

    return 0;
}


static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    kfree(usb_in_buffer);
    kfree(usb_out_buffer);
}

char* concat(const char *s1, const char *s2) {
    const size_t len1 = s1 ? strlen(s1) : 0; // Handle NULL for s1
    const size_t len2 = strlen(s2);          // s2 should not be NULL

    char *result = kmalloc(len1 + len2 + 1, GFP_KERNEL); // +1 for the null-terminator

    if (!result) {
        printk(KERN_ERR "Memory allocation failed\n");
        return NULL;
    }

    // If s1 is not NULL, copy it to result
    if (s1) {
        memcpy(result, s1, len1);
    }
    // Copy s2 to result, including the null-terminator
    memcpy(result + len1, s2, len2 + 1); // +1 to copy the null-terminator

    return result;
}

static int usb_read_serial() {
    int ret, actual_size;
    int retries = 2;  // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    char *response_prefix = "RES GET_LDR ";
    char *resp = NULL;
    int response_prefix_len = strlen(response_prefix);
    int ldr_value = -1;
    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0) {
        // Lê os dados da porta serial e armazena em usb_in_buffer
        // usb_in_buffer - contem a resposta em string do dispositivo
        // actual_size - contem o tamanho da resposta em bytes
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 1000);
         if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", retries--, ret);
            continue;
        }
        usb_in_buffer[actual_size] = '\0'; // Assegura que o buffer está terminado em null
        
        // Concatena os valores obtidos do buffer
        resp = concat(resp, usb_in_buffer); // Concatenate the usb_in_buffer to resp

        // printk(KERN_INFO "SmartLamp: Dados recebidos: (%s)\n", resp);
        
        // Verifica se a resposta começa com "RES GET_LDR "
        if (strncmp(resp, response_prefix, response_prefix_len) == 0) {
            // Verifica se a resposta tem comprimento suficiente para conter o valor
            // printk(KERN_INFO "SmartLamp: actual_size : %d, prefix_lenght : %d", (int)strlen(resp), response_prefix_len);
            if ((int)strlen(resp) > response_prefix_len) {
                // Extrai o valor após o prefixo
                ldr_value = simple_strtol(resp + response_prefix_len, NULL, 13);
                printk(KERN_INFO "SmartLamp: LDR Value recebido: %d\n", ldr_value);
                return ldr_value;
            }
        }
    }
    return -1;
}

