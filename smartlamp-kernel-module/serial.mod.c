#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
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
	{ 0x367fcc51, "module_layout" },
	{ 0x4e89151d, "usb_deregister" },
	{ 0xf0fca0ae, "usb_register_driver" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x93c7edeb, "usb_find_common_endpoints" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0xb742fd7, "simple_strtol" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x5a921311, "strncmp" },
	{ 0xb341d9ba, "usb_bulk_msg" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x37a0cba, "kfree" },
	{ 0x92997ed8, "_printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v10C4pEA60d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "BCF016B417A6CC1C9245E69");
