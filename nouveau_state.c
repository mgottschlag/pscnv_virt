/*
 * Copyright 2005 Stephane Marchesin
 * Copyright 2008 Stuart Bennett
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "pscnv_virt_drv.h"
#include "pscnv_virt_call.h"

#include <drm/drm.h>

/*#include "drm_sarea.h"
#include "drm_crtc_helper.h"

#include <linux/swab.h>
#include <linux/slab.h>

#include "pscnv_drm.h"
#include "nouveau_reg.h"
#include "nouveau_fbcon.h"
#include "nouveau_pm.h"
#include "nv50_display.h"
#include "pscnv_vm.h"
#include "pscnv_chan.h"
#include "pscnv_fifo.h"*/
#include "pscnv_ioctl.h"

/* here a client dies, release the stuff that was allocated for its
 * file_priv */
void nouveau_preclose(struct drm_device *dev, struct drm_file *file_priv)
{
	pscnv_chan_cleanup(dev, file_priv);
	pscnv_vspace_cleanup(dev, file_priv);
}

/* first module load, setup the mmio/fb mapping */
int nouveau_firstopen(struct drm_device *dev)
{
	// TODO
	return 0;
}

static struct apertures_struct *nouveau_get_apertures(struct drm_device *dev)
{
	struct pci_dev *pdev = dev->pdev;
	struct apertures_struct *aper = alloc_apertures(3);
	if (!aper)
		return NULL;

	aper->ranges[0].base = pci_resource_start(pdev, 1);
	aper->ranges[0].size = pci_resource_len(pdev, 1);
	aper->count = 1;

	if (pci_resource_len(pdev, 2)) {
		aper->ranges[aper->count].base = pci_resource_start(pdev, 2);
		aper->ranges[aper->count].size = pci_resource_len(pdev, 2);
		aper->count++;
	}

	if (pci_resource_len(pdev, 3)) {
		aper->ranges[aper->count].base = pci_resource_start(pdev, 3);
		aper->ranges[aper->count].size = pci_resource_len(pdev, 3);
		aper->count++;
	}

	return aper;
}

int nouveau_load(struct drm_device *dev, unsigned long flags)
{
	struct drm_pscnv_virt_private *dev_priv;
	resource_size_t mmio_start_offs, call_start_offset;
	int ret;

	/* allocate the private device data */
	dev_priv = kzalloc(sizeof(*dev_priv), GFP_KERNEL);
	if (!dev_priv)
		return -ENOMEM;
	dev->dev_private = dev_priv;
	dev_priv->dev = dev;

	dev_priv->flags = flags;

	/* resource 0 is mmio regs */
	/* resource 1 is hypercall buffer */
	/* resource 2 is mapped vram */

	/* map the mmio regs */
	mmio_start_offs = drm_get_resource_start(dev, 0);
	ret = drm_addmap(dev, mmio_start_offs, PSCNV_VIRT_MMIO_SIZE,
		_DRM_REGISTERS, _DRM_KERNEL | _DRM_DRIVER, &dev_priv->mmio);

	if (ret) {
		NV_ERROR(dev, "Unable to initialize the mmio mapping.\n");
		return ret;
	}

	/* map the ring buffer */
	call_start_offset = drm_get_resource_start(dev, 1);
	ret = drm_addmap(dev, call_start_offset, PSCNV_CALL_AREA_SIZE,
		_DRM_REGISTERS, _DRM_KERNEL | _DRM_DRIVER, &dev_priv->call_data);

	if (ret) {
		NV_ERROR(dev, "Unable to initialize the call data mapping.\n");
		return ret;
	}

	dev_priv->vram_base = drm_get_resource_start(dev, 2);
	dev_priv->vram_size = drm_get_resource_len(dev, 2);

	/* initialize the hypercall interface */
	pscnv_virt_call_init(dev_priv);
	ret = drm_irq_install(dev);
	if (ret) {
		NV_ERROR(dev, "Unable to register the call interrupt.\n");
		return ret;
	}

	memset(dev_priv->vspaces, 0, sizeof(dev_priv->vspaces));
	memset(dev_priv->chans, 0, sizeof(dev_priv->chans));

#if 0
	struct drm_nouveau_private *dev_priv;
	uint32_t reg0, strap;
	resource_size_t mmio_start_offs;
	int ret;

	dev_priv = kzalloc(sizeof(*dev_priv), GFP_KERNEL);
	if (!dev_priv)
		return -ENOMEM;
	dev->dev_private = dev_priv;
	dev_priv->dev = dev;

	dev_priv->flags = flags/* & NOUVEAU_FLAGS*/;
	dev_priv->init_state = NOUVEAU_CARD_INIT_DOWN;

	NV_DEBUG(dev, "vendor: 0x%X device: 0x%X\n",
		 dev->pci_vendor, dev->pci_device);

	dev_priv->wq = create_workqueue("nouveau");
	if (!dev_priv->wq)
		return -EINVAL;

	/* resource 0 is mmio regs */
	/* resource 1 is linear FB */
	/* resource 2 is RAMIN (mmio regs + 0x1000000) */
	/* resource 6 is bios */

	/* map the mmio regs */
	mmio_start_offs = drm_get_resource_start(dev, 0);
	ret = drm_addmap(dev, mmio_start_offs, 0x00800000, _DRM_REGISTERS,
	    _DRM_KERNEL | _DRM_DRIVER, &dev_priv->mmio);

	if (ret) {
		NV_ERROR(dev, "Unable to initialize the mmio mapping. "
			 "Please report your setup to " DRIVER_EMAIL "\n");
		return ret;
	}
	NV_DEBUG(dev, "regs mapped ok at 0x%llx\n",
					(unsigned long long)mmio_start_offs);

#ifdef __BIG_ENDIAN
	/* Put the card in BE mode if it's not */
	if (nv_rd32(dev, NV03_PMC_BOOT_1))
		nv_wr32(dev, NV03_PMC_BOOT_1, 0x00000001);

	DRM_MEMORYBARRIER();
#endif

	/* Time to determine the card architecture */
	reg0 = nv_rd32(dev, NV03_PMC_BOOT_0);

	/* We're dealing with >=NV10 */
	if ((reg0 & 0x0f000000) > 0) {
		/* Bit 27-20 contain the architecture in hex */
		dev_priv->chipset = (reg0 & 0xff00000) >> 20;
	/* NV04 or NV05 */
	} else if ((reg0 & 0xff00fff0) == 0x20004000) {
		if (reg0 & 0x00f00000)
			dev_priv->chipset = 0x05;
		else
			dev_priv->chipset = 0x04;
	} else {
		dev_priv->chipset = (reg0 & 0xf0000) >> 16;
		if (dev_priv->chipset < 1 || dev_priv->chipset > 3)
			dev_priv->chipset = 0xff;
	}

	switch (dev_priv->chipset & 0xf0) {
	case 0x00:
		if (dev_priv->chipset >= 4)
			dev_priv->card_type = NV_04;
		else
			dev_priv->card_type = dev_priv->chipset;
		break;
	case 0x10:
	case 0x20:
	case 0x30:
		dev_priv->card_type = dev_priv->chipset & 0xf0;
		break;
	case 0x40:
	case 0x60:
		dev_priv->card_type = NV_40;
		break;
	case 0x50:
	case 0x80:
	case 0x90:
	case 0xa0:
		dev_priv->card_type = NV_50;
		break;
	case 0xc0:
		dev_priv->card_type = NV_C0;
		break;
	case 0xd0:
		dev_priv->card_type = NV_D0;
		break;
	default:
		NV_INFO(dev, "Unsupported chipset 0x%08x\n", reg0);
		return -EINVAL;
	}

	NV_INFO(dev, "Detected an NV%02x generation card (0x%08x)\n",
		dev_priv->card_type, reg0);

	/* determine frequency of timing crystal */
	strap = nv_rd32(dev, 0x101000);
	if ( dev_priv->chipset < 0x17 ||
	    (dev_priv->chipset >= 0x20 && dev_priv->chipset <= 0x25))
		strap &= 0x00000040;
	else
		strap &= 0x00400040;

	switch (strap) {
	case 0x00000000: dev_priv->crystal = 13500; break;
	case 0x00000040: dev_priv->crystal = 14318; break;
	case 0x00400000: dev_priv->crystal = 27000; break;
	case 0x00400040: dev_priv->crystal = 25000; break;
	}

	NV_DEBUG(dev, "crystal freq: %dKHz\n", dev_priv->crystal);

	dev_priv->fb_size = drm_get_resource_len(dev, 1);
	dev_priv->fb_phys = drm_get_resource_start(dev, 1);
	dev_priv->mmio_phys = drm_get_resource_start(dev, 0);

	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		int ret = nouveau_remove_conflicting_drivers(dev);
		if (ret)
			return ret;
	}

	/* map larger RAMIN aperture on NV40 cards */
	if (dev_priv->card_type >= NV_40) {
		int ramin_bar = 2;
		if (drm_get_resource_len(dev, ramin_bar) < PAGE_SIZE)
			ramin_bar = 3;

		dev_priv->ramin_size = drm_get_resource_len(dev, ramin_bar);
		ret = drm_addmap(dev, drm_get_resource_start(dev, ramin_bar),
				 dev_priv->ramin_size, _DRM_REGISTERS,
				 _DRM_KERNEL | _DRM_DRIVER, &dev_priv->ramin);
		if (ret) {
			NV_ERROR(dev, "Failed to init RAMIN mapping\n");
			return ret;
		}
	}

	nouveau_OF_copy_vbios_to_ramin(dev);

	/* Special flags */
	if (dev->pci_device == 0x01a0)
		dev_priv->flags |= NV_NFORCE;
	else if (dev->pci_device == 0x01f0)
		dev_priv->flags |= NV_NFORCE2;

	/* For kernel modesetting, init card now and bring up fbcon */
	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		int ret = nouveau_card_init(dev);
		if (ret)
			return ret;
	}
	device_printf(dev->device,
				"taking over the fictitious range 0x%lx-0x%lx...",
				dev_priv->fb_phys, dev_priv->fb_phys + dev_priv->fb_size);
	if ((ret = -vm_phys_fictitious_reg_range(dev_priv->fb_phys, dev_priv->fb_phys + dev_priv->fb_size, 0 /* PLHK FIXME */)))
		return (ret);
	device_printf(dev->device, "OK\n");
#endif

	return 0;
}

static void nouveau_close(struct drm_device *dev)
{
#if 0
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	/* In the case of an error dev_priv may not be allocated yet */
	if (dev_priv)
		nouveau_card_takedown(dev);
#endif
}

void nouveau_lastclose(struct drm_device *dev)
{
	//nouveau_close(dev);
}

int nouveau_unload(struct drm_device *dev)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;

	/*destroy_workqueue(dev_priv->wq);*/

	drm_irq_uninstall(dev);

	drm_rmmap(dev, dev_priv->mmio);
	drm_rmmap(dev, dev_priv->call_data);

	kfree(dev_priv);
	dev->dev_private = NULL;
	return 0;
}

#if 0
/* Wait until (value(reg) & mask) == val, up until timeout has hit */
bool nouveau_wait_until(struct drm_device *dev, uint64_t timeout,
			uint32_t reg, uint32_t mask, uint32_t val)
{
	uint64_t start = nv04_timer_read(dev);

	do {
		if ((nv_rd32(dev, reg) & mask) == val)
			return true;
	} while (nv04_timer_read(dev) - start < timeout);

	return false;
}

/* Wait until (value(reg) & mask) != val, up until timeout has hit. */
bool nouveau_wait_until_neq(struct drm_device *dev, uint64_t timeout,
			    uint32_t reg, uint32_t mask, uint32_t val)
{
	uint64_t start = nv04_timer_read(dev);

	do {
		if ((nv_rd32(dev, reg) & mask) != val)
			return true;
	} while (nv04_timer_read(dev) - start < timeout);

	return false;
}

bool
nouveau_wait_cb(struct drm_device *dev, uint64_t timeout,
		bool (*cond)(void *), void *data)
{
	uint64_t start = nv04_timer_read(dev);

	do {
		if (cond(data) == true)
			return true;
	} while (nv04_timer_read(dev) - start < timeout);

	return false;
}
#endif
