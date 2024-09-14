    #include <linux/module.h>
    #include <linux/usb.h>
    #include <linux/slab.h>

    MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
    MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102)");
    MODULE_LICENSE("GPL");

    #define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositivo USB

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

        int LDR_value = usb_read_serial();
        printk(KERN_INFO "SmartLamp: LDR_VALUE %d\n", LDR_value);

        return 0;
    }

    static void usb_disconnect(struct usb_interface *interface) {
        printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
    }

static int usb_read_serial() {
    int ret, actual_size;
    int retries = 36;
    int ldr_value = -1;
    char *search_str = "SmartLamp Initialized.\n";
    char *response_start = "RES GET_LDR ";
    char *found_pos;
    char *resp;
    size_t resp_size = 0;
    size_t search_str_len = strlen(search_str);
    size_t response_start_len = strlen(response_start);

    // Aloca memória para o buffer de resposta
    resp = kmalloc(usb_max_size + 1, GFP_KERNEL);
    if (!resp) {
        printk(KERN_ERR "SmartLamp: Falha na alocação de memória para o buffer de resposta.\n");
        return -ENOMEM;
    }

    // Inicializa o buffer de resposta
    memset(resp, 0, usb_max_size + 1);

    printk(KERN_INFO "SmartLamp: Iniciando leitura da serial.\n");

    while (retries > 0) {
        // Tenta ler dados da USB
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in),
                           usb_in_buffer, usb_max_size, &actual_size, 5000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Código: %d\n", retries, ret);
            retries--;
            continue;
        }

        usb_in_buffer[actual_size] = '\0'; // Termina a string

        // Concatena o buffer de entrada à resposta acumulada
        if (resp_size + actual_size < usb_max_size) {
            memcpy(resp + resp_size, usb_in_buffer, actual_size);
            resp_size += actual_size;
            resp[resp_size] = '\0'; // Termina a string
        } else {
            printk(KERN_ERR "SmartLamp: Buffer de resposta excedido. Tamanho atual: %zu\n", resp_size);
            kfree(resp);
            return -ENOMEM;
        }

        // Verifica se a mensagem "SmartLamp Initialized.\n" foi encontrada
        found_pos = strstr(resp, search_str);
        if (found_pos) {
            printk(KERN_INFO "SmartLamp: Mensagem inicializada encontrada. Buffer acumulado: %s\n", resp);

            // Avança o buffer para começar após a mensagem inicial
            size_t offset = found_pos - resp + search_str_len;
            size_t remaining_size = resp_size - offset;
            if (remaining_size > 0) {
                memmove(resp, resp + offset, remaining_size);
                resp[remaining_size] = '\0'; // Termina a string
                resp_size = remaining_size;
            } else {
                resp[0] = '\0'; // Limpa o buffer se não houver dados restantes
                resp_size = 0;
            }

            // Procura pela resposta relevante após a mensagem inicial
            found_pos = strstr(resp, response_start);
            if (found_pos) {
                // Obtém o valor após "RES GET_LDR "
                size_t value_offset = found_pos - resp + response_start_len;
                char *value_str = resp + value_offset;
                ldr_value = simple_strtol(value_str, NULL, 10);
                printk(KERN_INFO "SmartLamp: Valor do LDR recebido: %d\n", ldr_value);
                kfree(resp);
                return ldr_value; // Retorna o valor do LDR recebido
            } else {
                printk(KERN_INFO "SmartLamp: Mensagem de resposta 'RES GET_LDR ' não encontrada após a mensagem inicial. Buffer: %s\n", resp);
            }
        } else {
            printk(KERN_INFO "SmartLamp: Mensagem inicial não encontrada ainda. Buffer acumulado: %s\n", resp);
        }

        retries--;
        printk(KERN_INFO "SmartLamp: Tentativas restantes: %d\n", retries);
    }

    printk(KERN_ERR "SmartLamp: Falha ao ler o valor do LDR após várias tentativas. Buffer final: %s\n", resp);
    kfree(resp);
    return -1; // Retorna -1 se não conseguir ler o valor do LDR
}


