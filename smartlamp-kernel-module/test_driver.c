#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Mock sysfs do SmartLamp para testar o daemon de brilho");
MODULE_LICENSE("GPL");

static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

static struct kobj_attribute led_attribute = __ATTR(led, 0664, attr_show, attr_store);
static struct kobj_attribute ldr_attribute = __ATTR(ldr, 0664, attr_show, attr_store);
static struct kobj_attribute threshold_attribute = __ATTR(threshold, 0664, attr_show, attr_store);

static struct attribute *attrs[] = {
    &led_attribute.attr,
    &ldr_attribute.attr,
    &threshold_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *sys_obj;
static int mock_led __maybe_unused;
static int mock_ldr __maybe_unused = 50;
static int mock_threshold __maybe_unused = 50;

// Função que garante que o valor está entre 0 e 100
static int clamp_percent(long value)
{
    if (value < 0)
        return 0;
    if (value > 100)
        return 100;
    return value;
}

// Função de LEITURA (chamada ao usar o comando 'cat')
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff)
{
    const char *attr_name = attr->attr.name;
    int value = 0;

    (void)sys_obj; // Evita warning de variável não utilizada

    // Compara o nome do atributo e define o 'value' com a variável correta
    if (strcmp(attr_name, "led") == 0) {
        value = mock_led;
    } else if (strcmp(attr_name, "ldr") == 0) {
        value = mock_ldr;
    } else if (strcmp(attr_name, "threshold") == 0) {
        value = mock_threshold;
    }

    // Retorna o valor formatado como texto para o utilizador
    return sprintf(buff, "%d\n", value);
}

// Função de ESCRITA (chamada ao usar o comando 'echo')
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count)
{
    const char *attr_name = attr->attr.name;
    long value;
    int ret;

    (void)sys_obj; // Evita warning de variável não utilizada

    // Converte o texto recebido para um número longo (base 10)
    ret = kstrtol(buff, 10, &value);
    if (ret)
        return ret;

    // Limita o valor entre 0 e 100
    value = clamp_percent(value);

    // Compara o nome do atributo e guarda o 'value' na variável correta
    if (strcmp(attr_name, "led") == 0) {
        mock_led = value;
    } else if (strcmp(attr_name, "ldr") == 0) {
        mock_ldr = value;
    } else if (strcmp(attr_name, "threshold") == 0) {
        mock_threshold = value;
    }

    return count;
}

// Função de Inicialização do Módulo
static int __init smartlamp_mock_init(void)
{
    int ret;

    sys_obj = kobject_create_and_add("smartlamp", kernel_kobj);
    if (!sys_obj)
        return -ENOMEM;

    ret = sysfs_create_group(sys_obj, &attr_group);
    if (ret) {
        kobject_put(sys_obj);
        sys_obj = NULL;
        return ret;
    }

    pr_info("SmartLamp mock: arquivos criados em /sys/kernel/smartlamp\n");
    return 0;
}

// Função de Limpeza ao remover o módulo
static void __exit smartlamp_mock_exit(void)
{
    if (sys_obj) {
        sysfs_remove_group(sys_obj, &attr_group);
        kobject_put(sys_obj);
        sys_obj = NULL;
    }

    pr_info("SmartLamp mock: modulo removido\n");
}

module_init(smartlamp_mock_init);
module_exit(smartlamp_mock_exit);