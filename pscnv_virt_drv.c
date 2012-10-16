/*
 * Copyright 2005 Stephane Marchesin.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <linux/console.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/pci_ids.h>

#include "pscnv_virt_drv.h"
#include <drm/drm.h>

#include "pscnv_ioctl.h"
#include "pscnv_drm.h"
#include "pscnv_vm.h"
#include "pscnv_gem.h"

#define PCI_VENDOR_ID_REDHAT 0x1af4
#define PCI_DEVICE_ID_PSCNV_VIRT 0x10f0

static struct drm_ioctl_desc nouveau_ioctls[] = {
	DRM_IOCTL_DEF_DRV(PSCNV_GETPARAM, pscnv_ioctl_getparam, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_GEM_NEW, pscnv_ioctl_gem_new, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_GEM_INFO, pscnv_ioctl_gem_info, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_VSPACE_NEW, pscnv_ioctl_vspace_new, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_VSPACE_FREE, pscnv_ioctl_vspace_free, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_VSPACE_MAP, pscnv_ioctl_vspace_map, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_VSPACE_UNMAP, pscnv_ioctl_vspace_unmap, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_CHAN_NEW, pscnv_ioctl_chan_new, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_CHAN_FREE, pscnv_ioctl_chan_free, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_OBJ_VDMA_NEW, pscnv_ioctl_obj_vdma_new, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_FIFO_INIT, pscnv_ioctl_fifo_init, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_OBJ_ENG_NEW, pscnv_ioctl_obj_eng_new, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PSCNV_FIFO_INIT_IB, pscnv_ioctl_fifo_init_ib, DRM_UNLOCKED),
};

static struct pci_device_id pciidlist[] = {
	{
		PCI_DEVICE(PCI_VENDOR_ID_REDHAT, PCI_DEVICE_ID_PSCNV_VIRT),
		.class = PCI_BASE_CLASS_SYSTEM << 16,
		.class_mask  = 0xff << 16,
	},
	{}
};

MODULE_DEVICE_TABLE(pci, pciidlist);

static struct drm_driver driver;

static int __devinit
pscnv_virt_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	return drm_get_pci_dev(pdev, ent, &driver);
}

static void
pscnv_virt_pci_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);

	drm_put_dev(dev);
}

int
pscnv_virt_pci_suspend(struct pci_dev *pdev, pm_message_t pm_state)
{
	return -ENODEV;
}

int
pscnv_virt_pci_resume(struct pci_dev *pdev)
{
	return -ENODEV;
}

static const struct file_operations nouveau_driver_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.mmap = pscnv_mmap,
	.poll = drm_poll,
	.fasync = drm_fasync,
};

static struct drm_driver driver = {
	.driver_features =
		DRIVER_USE_AGP | DRIVER_PCI_DMA | DRIVER_SG |
		DRIVER_HAVE_IRQ | DRIVER_IRQ_SHARED | DRIVER_GEM,
	.load = nouveau_load,
	.firstopen = nouveau_firstopen,
	.lastclose = nouveau_lastclose,
	.unload = nouveau_unload,
	.preclose = nouveau_preclose,
	.irq_preinstall = nouveau_irq_preinstall,
	.irq_postinstall = nouveau_irq_postinstall,
	.irq_uninstall = nouveau_irq_uninstall,
	.irq_handler = nouveau_irq_handler,
	.ioctls = nouveau_ioctls,
	.num_ioctls = DRM_ARRAY_SIZE(nouveau_ioctls),

	.fops = &nouveau_driver_fops,

	.gem_free_object = pscnv_gem_free_object,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
#ifdef GIT_REVISION
	.date = GIT_REVISION,
#else
	.date = DRIVER_DATE,
#endif
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

static struct pci_driver pscnv_virt_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
	.probe = pscnv_virt_pci_probe,
	.remove = pscnv_virt_pci_remove,
	.suspend = pscnv_virt_pci_suspend,
	.resume = pscnv_virt_pci_resume
};

static int __init pscnv_virt_init(void)
{
	return drm_pci_init(&driver, &pscnv_virt_pci_driver);
}

static void __exit pscnv_virt_exit(void)
{
	drm_pci_exit(&driver, &pscnv_virt_pci_driver);
}

module_init(pscnv_virt_init);
module_exit(pscnv_virt_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
