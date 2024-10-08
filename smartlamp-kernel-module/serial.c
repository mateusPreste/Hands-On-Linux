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
    .name       = "smartlamp_read",
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


static int usb_read_serial(void) {
    int ret, actual_size;
    int retries = 10;  // Número de tentativas de leitura da USB
    char *response_prefix = "RES GET_LDR ";
    int response_prefix_len = strlen(response_prefix);
    int ldr_value = -1;
    char buffer[MAX_RECV_LINE + 1];  // Buffer acumulado
    int buffer_len = 0;
    bool prefix_found = false;  // Indica se o prefixo foi encontrado

    // Inicializa o buffer
    memset(buffer, 0, sizeof(buffer));

    // Tenta ler dados da USB até 10 vezes ou até encontrar o valor desejado
    while (retries > 0) {
        // Lê os dados da porta USB e armazena em usb_in_buffer
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in),
                           usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 5000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", retries--, ret);
            continue;
        }

        // Adiciona os novos dados ao buffer acumulado
        if (buffer_len + actual_size < MAX_RECV_LINE) {
            strncpy(buffer + buffer_len, usb_in_buffer, actual_size);
            buffer_len += actual_size;
        } else {
            printk(KERN_ERR "SmartLamp: Buffer estourado. Dados recebidos são muito grandes.\n");
            break;
        }

        // Garante que o buffer acumulado está terminado em null
        buffer[buffer_len] = '\0';

        printk(KERN_INFO "SmartLamp: Dados acumulados: %s\n", buffer);

        // Verifica se o prefixo "RES GET_LDR " foi encontrado
        if (!prefix_found) {
            char *start = strstr(buffer, response_prefix);
            if (start != NULL) {
                prefix_found = true;
                // Remove qualquer dado "lixo" antes do prefixo
                buffer_len = strlen(start);
                memmove(buffer, start, buffer_len);
                buffer[buffer_len] = '\0';  // Assegura o final da string
                printk(KERN_INFO "SmartLamp: Prefixo encontrado, processamento iniciado.\n");
            } else {
                // Se não encontrou o prefixo, continua acumulando dados
                printk(KERN_INFO "SmartLamp: Prefixo não encontrado, aguardando mais dados...\n");
                continue;
            }
        }

        // Se o prefixo foi encontrado, verifica se já recebemos uma linha completa (terminada por '\n')
        if (prefix_found && strchr(buffer, '\n')) {
            // Extrai o valor após o prefixo e antes do '\n'
            char *number_start = buffer + response_prefix_len;
            char *endptr;
            long value = simple_strtol(number_start, &endptr, 10);

            // Se a conversão foi bem-sucedida e encontramos um número
            if (endptr != number_start) {
                ldr_value = (int)value;
                printk(KERN_INFO "SmartLamp: LDR Value recebido: %d\n", ldr_value);
                return ldr_value;
            } else {
                printk(KERN_INFO "SmartLamp: Prefixo encontrado, mas número ainda não disponível.\n");
            }

            // Limpa o buffer depois de processar a linha completa
            memset(buffer, 0, sizeof(buffer));
            buffer_len = 0;
            prefix_found = false;  // Reseta para procurar por outro prefixo se necessário
        }

        retries--;
    }

    printk(KERN_ERR "SmartLamp: Não foi possível ler o valor do LDR após várias tentativas.\n");
    return -1;  // Retorna -1 se não conseguiu ler um valor válido
}


