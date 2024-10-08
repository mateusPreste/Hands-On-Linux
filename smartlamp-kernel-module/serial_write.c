#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");


#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositvo USB


static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10c4  /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xea60  /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int  usb_read_serial(void);                                                   // Executado para ler a saida da porta serial
static int usb_write_serial(char *cmd, int param);

MODULE_DEVICE_TABLE(usb, id_table);
bool ignore = true;
int LDR_value = 0;

static struct usb_driver smartlamp_driver = {
    .name        = "smartlamp_write",     // Nome do driver
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


    // usb_write_serial("SET_LED", 0);
    // usb_write_serial("SET_LED", 100);

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
}

// Função para enviar comandos via serial
static int usb_write_serial(char *cmd, int param) {
    int ret, actual_size;
    char resp_expected[MAX_RECV_LINE];    // Resposta esperada do comando enviado
    int response_value;

    // Formatar o comando de forma que o firmware o reconheça
    // Por exemplo: "SET_LED 100" ou "GET_LDR"
    snprintf(usb_out_buffer, MAX_RECV_LINE, "%s %d\n", cmd, param);

    // Exibir o comando que será enviado
    printk(KERN_INFO "SmartLamp: Enviando comando: %s\n", usb_out_buffer);

    // Enviar o comando para o dispositivo USB
    int max_attempts = 10;
    int attempt = 0;

    do {
        printk(KERN_INFO "SmartLamp: Tentando enviar comando... {%s} %d\n", usb_out_buffer, usb_sndbulkpipe(smartlamp_device, usb_out));

        ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out),
        usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000*HZ);
        attempt++;
        if (ret) {
        printk(KERN_ERR "SmartLamp: -Error sending bulk message: %d\n", ret);
        // Add more specific error handling based on the return value
        if (ret == -ETIMEDOUT) {
            printk(KERN_ERR "SmartLamp: Timeout occurred\n");
            // Retry or take other actions based on the timeout
        } else if (ret == -EPIPE) {
            printk(KERN_ERR "SmartLamp: Stalled endpoint\n");
            // Handle stalled endpoint condition
        }
}

        if (ret == -EAGAIN && attempt < max_attempts) {
            printk(KERN_INFO "SmartLamp: Tentando novamente (tentativa %d)...\n", attempt);
            msleep(500); // Aguarda antes de tentar novamente
        } else if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao enviar comando! Código de erro: %d\n", ret);
            return -1;
        }

    } while (ret == -EAGAIN && attempt < max_attempts);

    // Agora, preparar para receber a resposta relacionada ao comando enviado
    // Exemplo: se o comando foi "GET_LDR", esperamos uma resposta como "RES GET_LDR X"
    snprintf(resp_expected, MAX_RECV_LINE, "RES %s", cmd);

    // Chamar a função de leitura para processar a resposta do dispositivo
    response_value = usb_read_serial();

    // Validar o valor retornado (se necessário, baseado no comando enviado)
    if (strncmp(resp_expected, "RES GET_LDR", strlen("RES GET_LDR")) == 0) {
        printk(KERN_INFO "SmartLamp: Valor do LDR recebido: %d\n", response_value);
    }
    else if (strncmp(resp_expected, "RES GET_LED", strlen("RES GET_LED")) == 0) {
        printk(KERN_INFO "SmartLamp: Valor do LED recebido: %d\n", response_value);
    }
    else if (strncmp(resp_expected, "RES SET_LED", strlen("RES SET_LED")) == 0) {
        if (response_value == 1) {
            printk(KERN_INFO "SmartLamp: LED ajustado com sucesso.\n");
        } else {
            printk(KERN_ERR "SmartLamp: Falha ao ajustar o LED.\n");
        }
    }
    else {
        printk(KERN_ERR "SmartLamp: Comando não reconhecido ou erro na resposta.\n");
        return -1;
    }

    return response_value;  // Retorna o valor lido ou status da operação
}


static int usb_read_serial(void) {
    int ret, actual_size, attempts = 0;
    char response[MAX_RECV_LINE];  // Armazena a resposta do ESP32
    char *start_ptr;
    
    memset(response, 0, sizeof(response));
    // Continuar tentando ler a mensagem do ESP32 até o máximo de tentativas
    while (attempts < 20) {
        // Limpa o buffer de resposta

        // Lê a resposta do ESP32
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in),
                           usb_in_buffer, usb_max_size, &actual_size, 1000*HZ);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler da porta serial! Código: %d\n", ret);
            return -1;
        }

        // Acumula o conteúdo lido no buffer
        strncat(response, usb_in_buffer, actual_size);
        printk(KERN_INFO "SmartLamp: RESPOSTA: %d\n", response);


        // Verifica se o buffer contém um fim de linha '\n', indicando que a resposta está completa
        start_ptr = strchr(response, '\n');
        if (start_ptr) {
            // Garante que a string está bem formatada até o '\n'
            *start_ptr = '\0';

            // Ignora valores "lixo" no começo da resposta (dados inválidos ou ruídos)
            start_ptr = strstr(response, "RES ");
            if (start_ptr == NULL) {
                printk(KERN_INFO "SmartLamp: Dados não reconhecidos, tentando novamente...\n");
                attempts++;
                continue;
            }

            // Identifica e trata as três respostas diferentes do ESP32
            if (strncmp(start_ptr, "RES GET_LDR", strlen("RES GET_LDR")) == 0) {
                // Extrai o valor do LDR
                int ldr_value = simple_strtol(start_ptr + strlen("RES GET_LDR "), NULL, 10);
                printk(KERN_INFO "SmartLamp: LDR Value recebido: %d\n", ldr_value);
                LDR_value = ldr_value; // Atualiza a variável global com o valor do LDR
                return ldr_value;
            } 
            else if (strncmp(start_ptr, "RES GET_LED", strlen("RES GET_LED")) == 0) {
                // Extrai o valor do LED
                int led_value = simple_strtol(start_ptr + strlen("RES GET_LED "), NULL, 10);
                printk(KERN_INFO "SmartLamp: LED Value recebido: %d\n", led_value);
                return led_value;
            } 
            else if (strncmp(start_ptr, "RES SET_LED", strlen("RES SET_LED")) == 0) {
                // Extrai a confirmação de ajuste do LED
                int set_led_status = simple_strtol(start_ptr + strlen("RES SET_LED "), NULL, 10);
                if (set_led_status == 1) {
                    printk(KERN_INFO "SmartLamp: LED atualizado com sucesso.\n");
                } else {
                    printk(KERN_ERR "SmartLamp: Erro ao atualizar LED.\n");
                }
                return set_led_status;
            } 
            else {
                printk(KERN_ERR "SmartLamp: Resposta não reconhecida: %s\n", start_ptr);
                attempts++;
            }
        } else {
            printk(KERN_INFO "SmartLamp: Aguardando por resposta completa...\n");
            attempts++;
        }
    }

    printk(KERN_ERR "SmartLamp: Falha ao ler resposta após várias tentativas.\n");
    return -1;
}
