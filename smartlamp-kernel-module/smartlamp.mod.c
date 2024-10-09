#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
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

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x92997ed8, "_printk" },
	{ 0x76b74ec3, "kobject_put" },
	{ 0x37a0cba, "kfree" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x754d539c, "strlen" },
	{ 0x499cbaef, "usb_bulk_msg" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0xa19b956, "__stack_chk_fail" },
	{ 0x3854774b, "kstrtoll" },
	{ 0x41daf981, "kernel_kobj" },
	{ 0x5b7fa4b9, "kobject_create_and_add" },
	{ 0x1af1895d, "sysfs_create_group" },
	{ 0x93c7edeb, "usb_find_common_endpoints" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xdf6dd88f, "usb_deregister" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xe7444c51, "usb_register_driver" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xb3debb2c, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v10C4pEA60d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "D28D7B8E9E398633526D256");
