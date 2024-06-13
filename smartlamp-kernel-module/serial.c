#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ctype.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");


#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositvo USB


static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10C4 /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xEA60 /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int  usb_read_serial(void);                                                   // Executado para ler a saida da porta serial

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

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    ignore =  usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);  // AQUI
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    LDR_value = usb_read_serial();

    printk("LDR Value: %d\n", LDR_value);

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
}

static int usb_read_serial() {
    int ret, actual_size;
    int retries = 10;                       // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    // int valor;
    // char *token = usb_in_buffer;
    // char *lastToken = NULL;
    int i = 0;
    int numero = -2;
    char meu_bufferbuffer[MAX_RECV_LINE];

    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0) {
        // Lê os dados da porta serial e armazena em usb_in_buffer
            // usb_in_buffer - contem a resposta em string do dispositivo
            // actual_size - contem o tamanho da resposta em bytes
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", ret, retries--);
            continue;
        }

    //kstrtol
        for(i = 0; i < actual_size; i++) {
          printk(KERN_INFO "%c", usb_in_buffer[i]);
          meu_bufferbuffer[i] = usb_in_buffer[i];
        }

        // for(i = 0; i < actual_size; i++) {
        //   printk(KERN_INFO "%c", usb_in_buffer[i]);
        // }

        // while (*token != '\0') {
        //   printk(KERN_INFO "Valor: %s", token);
        //   if (strncmp(token, "LDR_VALUE", 9) == 0) {
        //     lastToken = token + 10;
        //     break;
        //   } else if (lastToken != NULL) {
        //     break;
        //   }
        //   token++;
        // }
        // sscanf(lastToken, "%d", &valor);
        //
        // //caso tenha recebido a mensagem 'RES_LDR X' via serial acesse o buffer 'usb_in_buffer' e retorne apenas o valor da resposta X
        // //retorne o valor de X em inteiro
        // return valor;
        // return 11;
    }


    // const char *buffer = "lixo lixo lixo\nRES GET_LDR 1234\n";
    char *result = strstr(meu_bufferbuffer, "RES GET_LDR"); // Procura a substring "RES GET_LDR"
    if (result != NULL) {
        const char *ptr = result + strlen("RES GET_LDR");
        while (*ptr && !isdigit(*ptr)) {
            ptr++;
        }
        if (*ptr) {
            sscanf(ptr, "%d", &numero);
            printk(KERN_INFO "O número inteiro encontrado é: %d\n", numero);
        } else {
            printk(KERN_INFO "Número inteiro não encontrado na substring.\n");
        }
    } else {
        printk(KERN_INFO "A substring 'RES GET_LDR' não foi encontrada.\n");
    }

    // const char *kernel_version = "Linux version 5.10.0-8-generic";
    // const char *substring = "5.10";
    // char buffer[20]; // Tamanho do buffer para armazenar a substring
    // char *result = memmem(kernel_version, strlen(kernel_version), substring, strlen(substring));
    // if (result != NULL) {
    //     strncpy(buffer, result, strlen(substring));
    //     buffer[strlen(substring)] = '\0'; // Adiciona o caractere nulo ao final da substring
    //     printk(KERN_INFO "A substring '%s' foi encontrada.\n", buffer);
    // } else {
    //     printk(KERN_INFO "A substring '%s' não foi encontrada.\n", substring);
    // }
    return numero;

    // return -1; 
}
