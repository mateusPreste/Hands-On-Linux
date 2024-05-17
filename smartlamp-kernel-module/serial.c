#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/string.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102)");
MODULE_LICENSE("GPL");

#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositivo USB

static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saída da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10c4 /* Encontre o VendorID do smartlamp */
#define PRODUCT_ID  0xea60 /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int usb_read_serial(void);                                                 // Executado para ler a saída da porta serial

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

// Executado quando o dispositivo é conectado na USB
static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;
    int ret;
    int actual_size;

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    ignore = usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);
    if (ignore) {
        printk(KERN_ERR "SmartLamp: Não foi possível encontrar endpoints USB.\n");
        return -ENODEV;
    }
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    if (!usb_in_buffer || !usb_out_buffer) {
        printk(KERN_ERR "SmartLamp: Falha ao alocar buffers USB.\n");
        return -ENOMEM;
    }

    // Envia um comando para obter o valor do LDR
    strcpy(usb_out_buffer, "GET_LDR\n");
    ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out), usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao enviar comando para o dispositivo USB. Código: %d\n", ret);
    } else {
        printk(KERN_INFO "SmartLamp: Comando GET_LDR enviado com sucesso. Tamanho: %d\n", actual_size);
    }

    // Lê a resposta do dispositivo
    LDR_value = usb_read_serial();

    printk(KERN_INFO "SmartLamp: LDR Value: %d\n", LDR_value);

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
}

// Função para ler a saída da porta serial
static int usb_read_serial() {
    int ret, actual_size;
    int retries = 10;                       // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    char line_buffer[MAX_RECV_LINE] = {0};  // Buffer para armazenar a linha completa
    int line_length = 0;

    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0) {
        // Lê os dados da porta serial e armazena em usb_in_buffer
        // usb_in_buffer - contém a resposta em string do dispositivo
        // actual_size - contém o tamanho da resposta em bytes
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Código: %d\n", retries, ret);
            retries--;
            continue;
        }

        // Adiciona os dados recebidos ao buffer de linha
        if (line_length + actual_size < MAX_RECV_LINE) {
            memcpy(line_buffer + line_length, usb_in_buffer, actual_size);
            line_length += actual_size;
            line_buffer[line_length] = '\0';
        } else {
            printk(KERN_ERR "SmartLamp: Buffer de linha excedido.\n");
            return -1;
        }

        // Verifica se uma linha completa foi recebida
        if (strchr(line_buffer, '\n')) {
            // Depuração: Imprime o conteúdo do buffer de linha recebido
            printk(KERN_INFO "SmartLamp: Conteúdo recebido: %s\n", line_buffer);

            // Procura por 'RES GET_LDR ' no início da resposta
            char *start = strstr(line_buffer, "RES GET_LDR ");
            if (!start) {
                printk(KERN_ERR "SmartLamp: Mensagem incompleta ou inválida.\n");
                continue;
            }

            // Extrai o valor de X após 'RES GET_LDR'
            start += strlen("RES GET_LDR "); // Move o ponteiro para o início do valor de X
            int value = 0;
            sscanf(start, "%d", &value);

            return value;
        }
    }

    return -1;
}
