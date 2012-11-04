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

#include "drm/drm.h"
#include "pscnv_virt_drv.h"
#include "pscnv_mem.h"
#include "pscnv_vm.h"
#include "pscnv_chan.h"
#include "pscnv_virt_call.h"

struct pscnv_vspace *
pscnv_vspace_new (struct drm_device *dev) {
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	struct pscnv_vspace *res = kzalloc(sizeof *res, GFP_KERNEL);
	volatile struct pscnv_vspace_cmd *cmd;
	uint32_t call;

	if (!res) {
		NV_ERROR(dev, "VM: Couldn't alloc vspace\n");
		return 0;
	}
	res->dev = dev;
	kref_init(&res->ref);
	mutex_init(&res->lock);
	/* create a vspace */
	call = pscnv_virt_call_alloc(dev_priv);
	cmd = dev_priv->call_data->handle + call;
	cmd->command = PSCNV_CMD_VSPACE_ALLOC;
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(dev, "Failed to allocate the vspace.\n");
		pscnv_virt_call_finish(dev_priv, call);
		kfree(res);
		return 0;
	}
	res->vid = cmd->vid;
	pscnv_virt_call_finish(dev_priv, call);

	/* insert the vspace into the global vspace list */
	BUG_ON(dev_priv->vspaces[res->vid] != 0);
	dev_priv->vspaces[res->vid] = res;

	return res;
}

void pscnv_vspace_ref_free(struct kref *ref) {
	struct pscnv_vspace *vs = container_of(ref, struct pscnv_vspace, ref);
	struct drm_pscnv_virt_private *dev_priv = vs->dev->dev_private;
	volatile struct pscnv_vspace_cmd *cmd;
	uint32_t call;

	NV_INFO(vs->dev, "VM: Freeing vspace %d\n", vs->vid);
	
	call = pscnv_virt_call_alloc(dev_priv);
	cmd = dev_priv->call_data->handle + call;
	cmd->command = PSCNV_CMD_VSPACE_FREE;
	cmd->vid = vs->vid;
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(vs->dev, "Failed to free the vspace.\n");
	}
	pscnv_virt_call_finish(dev_priv, call);

	kfree(vs);
}

/*int
pscnv_vspace_map(struct pscnv_vspace *vs, struct pscnv_bo *bo,
		uint64_t start, uint64_t end, int back,
		struct pscnv_mm_node **res)
{
	struct pscnv_mm_node *node;
	int ret;
	struct drm_nouveau_private *dev_priv = vs->dev->dev_private;
	mutex_lock(&vs->lock);
	ret = dev_priv->vm->place_map(vs, bo, start, end, back, &node);
	if (ret) {
		mutex_unlock(&vs->lock);
		return ret;
	}
	node->tag = bo;
	node->tag2 = vs;
	if (pscnv_vm_debug >= 1)
		NV_INFO(vs->dev, "VM: vspace %d: Mapping BO %x/%d at %llx-%llx.\n", vs->vid, bo->cookie, bo->serial, node->start,
				node->start + node->size);
	ret = dev_priv->vm->do_map(vs, bo, node->start);
	if (ret) {
		pscnv_vspace_unmap_node_unlocked(node);
	}
	*res = node;
	mutex_unlock(&vs->lock);
	return ret;
}

int
pscnv_vspace_unmap(struct pscnv_vspace *vs, uint64_t start) {
	int ret;
	mutex_lock(&vs->lock);
	ret = pscnv_vspace_unmap_node_unlocked(pscnv_mm_find_node(vs->mm, start));
	mutex_unlock(&vs->lock);
	return ret;
}*/