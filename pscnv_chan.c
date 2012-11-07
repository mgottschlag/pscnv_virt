/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2010 PathScale Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <drm/drm.h>
#include "pscnv_virt_drv.h"
#include "pscnv_mem.h"
#include "pscnv_vm.h"
#include "pscnv_chan.h"
#include "pscnv_ioctl.h"
#include "pscnv_virt_call.h"

struct pscnv_chan *
pscnv_chan_new (struct drm_device *dev, struct pscnv_vspace *vs) {
	struct drm_pscnv_virt_private *dev_priv = vs->dev->dev_private;
	struct pscnv_chan *res = kzalloc(sizeof *res, GFP_KERNEL);
	volatile struct pscnv_chan_new_cmd *cmd;
	uint32_t call;

	if (!res) {
		NV_ERROR(vs->dev, "CHAN: Couldn't alloc channel\n");
		return 0;
	}
	res->dev = dev;
	res->vspace = vs;
	pscnv_vspace_ref(vs);

	/* create a channel */
	call = pscnv_virt_call_alloc(dev_priv);
	cmd = dev_priv->call_data->handle + call;
	cmd->command = PSCNV_CMD_CHAN_NEW;
	cmd->vid = vs->vid;
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		pscnv_vspace_unref(vs);
		kfree(res);
		NV_ERROR(dev, "Failed to create channel.\n");
		return 0;
	}
	res->cid = cmd->cid;
	pscnv_virt_call_finish(dev_priv, call);

	kref_init(&res->ref);

	/* insert the channel into the global channel list */
	BUG_ON(dev_priv->chans[res->cid] != 0);
	dev_priv->chans[res->cid] = res;
	return res;
}

void pscnv_chan_ref_free(struct kref *ref) {
	struct pscnv_chan *ch = container_of(ref, struct pscnv_chan, ref);
	struct drm_device *dev = ch->dev;
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	volatile struct pscnv_chan_free_cmd *cmd;
	uint32_t call;

	NV_INFO(dev, "CHAN: Freeing channel %d\n", ch->cid);

	call = pscnv_virt_call_alloc(dev_priv);
	cmd = dev_priv->call_data->handle + call;
	cmd->command = PSCNV_CMD_CHAN_FREE;
	cmd->cid = ch->cid;
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(dev, "Failed to free the channel.\n");
	}
	pscnv_virt_call_finish(dev_priv, call);

	pscnv_vspace_unref(ch->vspace);
	kfree(ch);
}

static void pscnv_chan_vm_open(struct vm_area_struct *vma) {
	struct pscnv_chan *ch = vma->vm_private_data;
	pscnv_chan_ref(ch);
}

static void pscnv_chan_vm_close(struct vm_area_struct *vma) {
	struct pscnv_chan *ch = vma->vm_private_data;
	pscnv_chan_unref(ch);
}

static struct vm_operations_struct pscnv_chan_vm_ops = {
	.open = pscnv_chan_vm_open,
	.close = pscnv_chan_vm_close,
};

int pscnv_chan_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_file *priv = filp->private_data;
	struct drm_device *dev = priv->minor->dev;
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	int cid;
	struct pscnv_chan *ch;

	if ((vma->vm_pgoff * PAGE_SIZE & ~0x7f0000ull) != 0xc0000000) {
		return -ENOSYS;
	}
	cid = (vma->vm_pgoff * PAGE_SIZE >> 16) & 0x7f;
	/* channels are longer on nv50 */
	if (dev_priv->is_nv50 && vma->vm_end - vma->vm_start > 0x2000) {
		NV_ERROR(dev, "pscnv_chan_mmap: too long (nv50)\n");
		return -EINVAL;
	} else if (vma->vm_end - vma->vm_start > 0x1000) {
		NV_ERROR(dev, "pscnv_chan_mmap: too long (nvc0)\n");
		return -EINVAL;
	}

	ch = pscnv_get_chan(dev, filp->private_data, cid);
	if (!ch)
		return -ENOENT;

	vma->vm_flags |= VM_RESERVED | VM_IO | VM_PFNMAP | VM_DONTEXPAND;
	vma->vm_ops = &pscnv_chan_vm_ops;
	vma->vm_private_data = ch;

	vma->vm_file = filp;

	if (dev_priv->is_nv50) {
		return remap_pfn_range(vma, vma->vm_start,
				(dev_priv->chan_base + cid * 0x2000) >> PAGE_SHIFT,
				vma->vm_end - vma->vm_start, PAGE_SHARED);
	} else {
		return remap_pfn_range(vma, vma->vm_start,
				(dev_priv->chan_base + cid * 0x1000) >> PAGE_SHIFT,
				vma->vm_end - vma->vm_start, PAGE_SHARED);
	}
}
