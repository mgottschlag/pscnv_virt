/*
 * Copyright (C) 2006 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Ben Skeggs <darktama@iinet.net.au>
 */

#include <drm/drm.h>
#include "pscnv_drm.h"
#include "pscnv_virt_drv.h"

void
nouveau_irq_preinstall(struct drm_device *dev)
{
	NV_ERROR(dev, "nouveau_irq_preinstall.\n");
#if 0
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	/* Master disable */
	nv_wr32(dev, NV03_PMC_INTR_EN_0, 0);

	if (dev_priv->card_type >= NV_50) {
		if (dev_priv->card_type < NV_D0)
			INIT_WORK(&dev_priv->irq_work, nv50_display_irq_handler_bh);
		else
			INIT_WORK(&dev_priv->irq_work, nvd0_display_bh);
		INIT_WORK(&dev_priv->hpd_work, nv50_display_irq_hotplug_bh);
//		INIT_LIST_HEAD(&dev_priv->vbl_waiting);
	}
#endif
}

int
nouveau_irq_postinstall(struct drm_device *dev)
{
	NV_ERROR(dev, "nouveau_irq_postinstall.\n");
#if 0
	/* Master enable */
	nv_wr32(dev, NV03_PMC_INTR_EN_0, NV_PMC_INTR_EN_0_MASTER_ENABLE);
	return 0;
#endif
	return 0;
}

void
nouveau_irq_uninstall(struct drm_device *dev)
{
#if 0
	/* Master disable */
	nv_wr32(dev, NV03_PMC_INTR_EN_0, 0);
#endif
}

static void
nouveau_crtc_irq_handler(struct drm_device *dev, int crtc)
{
#if 0
	if (crtc & 1)
		nv_wr32(dev, NV_CRTC0_INTSTAT, NV_CRTC_INTR_VBLANK);

	if (crtc & 2)
		nv_wr32(dev, NV_CRTC1_INTSTAT, NV_CRTC_INTR_VBLANK);
#endif
}

static void
nouveau_pbus_irq_handler(struct drm_device *dev) {
#if 0
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t status = nv_rd32(dev, 0x1100);
	uint32_t mask = (dev_priv->chipset >= 0xc0) ? 0xf : 0x8;

	if (status & mask) {
		uint32_t addr = nv_rd32(dev, 0x9084);
		uint32_t data = nv_rd32(dev, 0x9088);

		if (!(addr & 1)) {
			NV_ERROR(dev, "PBUS: Unknown MMIO problem %06x %08x\n", addr, data);
		} else if (addr & 0x2) {
			NV_ERROR(dev, "PBUS: MMIO write fault, addr %06x data %08x\n", addr & ~3, data);
		} else {
			NV_ERROR(dev, "PBUS: MMIO read fault, addr %06x\n", addr & ~3);
		}
		if (status & 1)
			NV_ERROR(dev, "PBUS: accessed disabled PSUBFIFO\n");
		if (status & 2)
			NV_ERROR(dev, "PBUS: accessed wrong address inside unit\n");
		if (status & 4)
			NV_ERROR(dev, "PBUS: accessed address is outside unit\n");

		if (dev_priv->chipset >= 0xc0) {
			nv_wr32(dev, 0x9084, 0);
			nv_wr32(dev, 0x9088, 0);
		}
		nv_wr32(dev, 0x1100, status & mask);
		status &= ~mask;
	}

	if (status) {
		NV_ERROR(dev, "PBUS: unknown interrupt %08x\n", status);
		nv_wr32(dev, 0x1100, status);
	}
#endif
}

void nouveau_irq_register(struct drm_device *dev, int irq, pscnv_virt_irqhandler_t handler) {
#if 0
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	unsigned long flags;
	spin_lock_irqsave(&dev_priv->context_switch_lock, flags);
	BUG_ON(dev_priv->irq_handler[irq]);
	dev_priv->irq_handler[irq] = handler;
	spin_unlock_irqrestore(&dev_priv->context_switch_lock, flags);
#endif
}

void nouveau_irq_unregister(struct drm_device *dev, int irq) {
#if 0
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	unsigned long flags;
	spin_lock_irqsave(&dev_priv->context_switch_lock, flags);
	BUG_ON(!dev_priv->irq_handler[irq]);
	dev_priv->irq_handler[irq] = 0;
	spin_unlock_irqrestore(&dev_priv->context_switch_lock, flags);
#endif
}

#if 0
irqreturn_t
nouveau_irq_handler(DRM_IRQ_ARGS)
{
#if 0
	struct drm_device *dev = (struct drm_device *)arg;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t status;
	unsigned long flags;
	int i;

	status = nv_rd32(dev, NV03_PMC_INTR_0);
	if (!status)
		return IRQ_NONE;
	spin_lock_irqsave(&dev_priv->context_switch_lock, flags);

	if (status & 0x80000000) {
		NV_ERROR(dev, "Got a SOFTWARE interrupt for no good reason.\n");
		nv_wr32(dev, NV03_PMC_INTR_0, 0);
		status &= ~0x80000000;
	}

	if (status & 0x10000000) {
		nouveau_pbus_irq_handler(dev);
		status &= ~0x10000000;
	}

	for (i = 0; i < 32; i++) {
		if (status & 1 << i) {
			if (dev_priv->irq_handler[i]) {
				dev_priv->irq_handler[i](dev, i);
				status &= ~(1 << i);
			}
		}
	}

#if 0
	if (dev_priv->fbdev_info) {
		fbdev_flags = dev_priv->fbdev_info->flags;
		dev_priv->fbdev_info->flags |= FBINFO_HWACCEL_DISABLED;
	}
#endif

	if (status & NV_PMC_INTR_0_CRTCn_PENDING) {
		nouveau_crtc_irq_handler(dev, (status>>24)&3);
		status &= ~NV_PMC_INTR_0_CRTCn_PENDING;
	}

	if (status & (NV_PMC_INTR_0_NV50_DISPLAY_PENDING |
		      NV_PMC_INTR_0_NV50_I2C_PENDING)) {
		nv50_display_irq_handler(dev);
		status &= ~(NV_PMC_INTR_0_NV50_DISPLAY_PENDING |
			    NV_PMC_INTR_0_NV50_I2C_PENDING);
	}

	if (status)
		NV_ERROR(dev, "Unhandled PMC INTR status bits 0x%08x\n", status);

#if 0
	if (dev_priv->fbdev_info)
		dev_priv->fbdev_info->flags = fbdev_flags;
#endif

	spin_unlock_irqrestore(&dev_priv->context_switch_lock, flags);
#endif

	return IRQ_HANDLED;
}
#endif
