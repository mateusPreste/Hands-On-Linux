#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");


#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositvo USB


// static char recv_line[MAX_RECV_LINE];              // Armazena dados vindos da USB até receber um caractere de nova linha '\n'
static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10c4  /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xea60  /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int  usb_read_serial(void);
static int usb_write_serial(const char *cmd, int param);   
int usb_send_cmd(const char* command, int param);

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido (e.g., cat /sys/kernel/smartlamp/led)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/smartlamp/led)
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);   
// Variáveis para criar os arquivos no /sys/kernel/smartlamp/{led, ldr}
static struct kobj_attribute  led_attribute = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct attribute      *attrs[]       = { &led_attribute.attr, &ldr_attribute.attr, NULL };
static struct attribute_group attr_group    = { .attrs = attrs };
static struct kobject        *sys_obj;                                             // Executado para ler a saida da porta serial

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

    // Cria arquivos do /sys/kernel/smartlamp/*
    sys_obj = kobject_create_and_add("smartlamp", kernel_kobj);
    ignore = sysfs_create_group(sys_obj, &attr_group); // AQUI

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    ignore =  usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);  // AQUI
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    // LDR_value = usb_read_serial();
    // LDR_value = usb_send_cmd("GET_LDR", -1);
    // printk("LDR Value: %d\n", LDR_value);

    // // SET LED
    int led_value = usb_send_cmd("SET_LED", 0);
    printk("INITIAL LED VALUE: %d\n", led_value);    

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    if (sys_obj) kobject_put(sys_obj);      // Remove os arquivos em /sys/kernel/smartlamp
    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
}

// Envia um comando via USB, espera e retorna a resposta do dispositivo (convertido para int)
// Exemplo de Comando:  SET_LED 80
// Exemplo de Resposta: RES SET_LED 1
// Exemplo de chamada da função usb_send_cmd para SET_LED: usb_send_cmd("SET_LED", 80);
int usb_send_cmd(const char* command, int param) {
    // char send_buffer[256];
    int response_value;
    

    // Lê a resposta pela USB usando a função usb_read_serial que já faz o tratamento
    response_value = usb_write_serial(command, param);

    // Verifica se a resposta foi válida
    if (response_value == -1) {
        printk(KERN_ERR "SmartLamp: Erro ao obter a resposta da USB.\n");
    }

    // Retorna o valor inteiro da resposta (ou -1 em caso de erro)
    return response_value;
}




// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido (e.g., cat /sys/kernel/smartlamp/led)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff) {
    int value;
    const char *attr_name = attr->attr.name;

    printk(KERN_INFO "SmartLamp: Lendo %s ...\n", attr_name);

    // Lê o valor atual do led ou ldr
    if (strcmp(attr_name, "led") == 0) {
        value = usb_send_cmd("GET_LED", -1);  // Comando para obter o valor do LED
    } else if (strcmp(attr_name, "ldr") == 0) {
        value = usb_send_cmd("GET_LDR", -1);  // Comando para obter o valor do LDR
    } else {
        return -EINVAL;  // Arquivo desconhecido
    }

    if (value < 0) {
        printk(KERN_ERR "SmartLamp: Erro ao ler valor de %s\n", attr_name);
        return value;  // Retorna erro
    }

    sprintf(buff, "%d\n", value);  // Cria a mensagem com o valor
    return strlen(buff);
}



// Essa função não deve ser alterada durante a task sysfs
// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/smartlamp/led)
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count) {
    long ret, value;
    const char *attr_name = attr->attr.name;

    // Converte o valor recebido para long
    ret = kstrtol(buff, 10, &value);
    if (ret) {
        printk(KERN_ALERT "SmartLamp: valor de %s invalido.\n", attr_name);
        return -EACCES;
    }
 
    printk(KERN_INFO "SmartLamp: Setando %s para %ld ...\n", attr_name, value);

    // Se o arquivo sendo escrito for "led", envia o comando para ajustar o valor do LED
    if (strcmp(attr_name, "led") == 0) {
        ret = usb_send_cmd("SET_LED", value);  // Comando para setar o valor do LED
    } 
    // Se o arquivo sendo escrito for "ldr", você pode ignorar ou implementar algo similar, mas normalmente o LDR é de leitura
    else if (strcmp(attr_name, "ldr") == 0) {
        printk(KERN_ALERT "SmartLamp: Não é possível setar o valor do LDR.\n");
        return -1;
    } else {
        printk(KERN_ALERT "SmartLamp: Comando desconhecido para %s.\n", attr_name);
        return -EINVAL;  // Comando desconhecido
    }

    // Verifica se o comando foi executado corretamente
    if (ret < 0) {
        printk(KERN_ALERT "SmartLamp: Erro ao setar o valor de %s.\n", attr_name);
        return -EACCES;
    }

    return strlen(buff);
}

// Função para enviar comandos via serial
static int usb_write_serial(const char *cmd, int param) {
    int ret, actual_size;
    char resp_expected[MAX_RECV_LINE];    // Resposta esperada do comando enviado
    int response_value;
    int attempt = 0;

    // Formatar o comando de forma que o firmware o reconheça
    // Por exemplo: "SET_LED 100" ou "GET_LDR", para o GET_LDR, -1, deve somente enviar GET_LDR

    if(param == -1){
        snprintf(usb_out_buffer, MAX_RECV_LINE, "%s", cmd);
    }else{
        snprintf(usb_out_buffer, MAX_RECV_LINE, "%s %d", cmd, param);
    }

    // Exibir o comando que será enviado
    printk(KERN_INFO "SmartLamp: Enviando comando: %s\n", usb_out_buffer);

    // Enviar o comando para o dispositivo USB

    do {
        printk(KERN_INFO "SmartLamp: Tentando enviar comando... {%s} %d\n", usb_out_buffer, usb_sndbulkpipe(smartlamp_device, usb_out));

        ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out),
        usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000);
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

        if (ret == -EAGAIN && attempt < 10) {
            printk(KERN_INFO "SmartLamp: Tentando novamente (tentativa %d)...\n", attempt);
            msleep(500); // Aguarda antes de tentar novamente
        } else if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao enviar comando! Código de erro: %d\n", ret);
            return -1;
        }

    } while (ret == -EAGAIN && attempt < 10);

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
                           usb_in_buffer, usb_max_size, &actual_size, 1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler da porta serial! Código: %d\n", ret);
            return -1;
        }

        // Acumula o conteúdo lido no buffer
        strncat(response, usb_in_buffer, actual_size);
        printk(KERN_INFO "SmartLamp: RESPOSTA: %s\n", response);


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
