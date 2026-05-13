#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xe8213e80, "_printk" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xedc3f731, "kobject_put" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0x1f55c5b2, "kstrtoll" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x888b8f57, "strcmp" },
	{ 0xdd6830c7, "sprintf" },
	{ 0x7320379d, "sysfs_remove_group" },
	{ 0xd272d446, "__fentry__" },
	{ 0x550b89f1, "kernel_kobj" },
	{ 0x83f1675b, "kobject_create_and_add" },
	{ 0xeb5ade73, "sysfs_create_group" },
	{ 0xbebe66ff, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xe8213e80,
	0xd272d446,
	0xedc3f731,
	0xbd03ed67,
	0x1f55c5b2,
	0xd272d446,
	0x888b8f57,
	0xdd6830c7,
	0x7320379d,
	0xd272d446,
	0x550b89f1,
	0x83f1675b,
	0xeb5ade73,
	0xbebe66ff,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"_printk\0"
	"__x86_return_thunk\0"
	"kobject_put\0"
	"__ref_stack_chk_guard\0"
	"kstrtoll\0"
	"__stack_chk_fail\0"
	"strcmp\0"
	"sprintf\0"
	"sysfs_remove_group\0"
	"__fentry__\0"
	"kernel_kobj\0"
	"kobject_create_and_add\0"
	"sysfs_create_group\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "F93E85398B56B81E4655F4D");
