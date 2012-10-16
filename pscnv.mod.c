#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x94481e1f, "module_layout" },
	{ 0xfdb8d95c, "drm_release" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x79b11ecc, "drm_pci_exit" },
	{ 0x76ac579a, "malloc_sizes" },
	{ 0x5f79b834, "drm_mmap" },
	{ 0x1d09239c, "mutex_unlock" },
	{ 0x21e7844c, "drm_rmmap" },
	{ 0xf432dd3d, "__init_waitqueue_head" },
	{ 0xe8fb72e1, "drm_irq_install" },
	{ 0x8f64aa4, "_raw_spin_unlock_irqrestore" },
	{ 0x71bc089c, "current_task" },
	{ 0x27e1a049, "printk" },
	{ 0xb18993a6, "drm_put_dev" },
	{ 0xe317967d, "drm_gem_handle_create" },
	{ 0xa03a4654, "drm_addmap" },
	{ 0xb28434e9, "mutex_lock" },
	{ 0x8ecfc9f9, "drm_gem_object_release" },
	{ 0x68aca4ad, "down" },
	{ 0x4ea1f5a2, "drm_gem_object_free" },
	{ 0x4228f5ee, "drm_ioctl" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0x4005e24f, "drm_gem_object_alloc" },
	{ 0xcb9124b9, "drm_irq_uninstall" },
	{ 0x63192781, "drm_get_pci_dev" },
	{ 0x4ecc4513, "drm_gem_object_lookup" },
	{ 0xc9d80c4a, "kmem_cache_alloc_trace" },
	{ 0x9327f5ce, "_raw_spin_lock_irqsave" },
	{ 0xcf21d241, "__wake_up" },
	{ 0x37a0cba, "kfree" },
	{ 0x5c8b5ce8, "prepare_to_wait" },
	{ 0x71e3cecb, "up" },
	{ 0x912f5705, "drm_gem_object_handle_free" },
	{ 0xfa66f77c, "finish_wait" },
	{ 0x5911de01, "drm_pci_init" },
	{ 0xac255432, "dev_get_drvdata" },
	{ 0x9440aeb4, "drm_poll" },
	{ 0x13b2b0c1, "drm_fasync" },
	{ 0xe4b34dd0, "drm_open" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=drm";

MODULE_ALIAS("pci:v00001AF4d000010F0sv*sd*bc08sc*i*");
