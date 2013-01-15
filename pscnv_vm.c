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

struct vs_mapping {
	uint64_t start;
	struct pscnv_bo *bo;
	struct rb_node node;
};

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
	res->mappings = RB_ROOT;
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
	struct rb_node *mapping_node;

	NV_INFO(vs->dev, "VM: Freeing vspace %d\n", vs->vid);
	
	/* call the hypervisor to free the vspace */
	call = pscnv_virt_call_alloc(dev_priv);
	cmd = dev_priv->call_data->handle + call;
	cmd->command = PSCNV_CMD_VSPACE_FREE;
	cmd->vid = vs->vid;
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(vs->dev, "Failed to free the vspace.\n");
	}
	pscnv_virt_call_finish(dev_priv, call);

	/* delete the guest memory structures */
	mapping_node = rb_first(&vs->mappings);
	while (mapping_node != NULL) {
		struct vs_mapping *mapping = rb_entry(mapping_node, struct vs_mapping, node);
		rb_erase(mapping_node, &vs->mappings);
		kfree(mapping);
		mapping_node = rb_first(&vs->mappings);
	}
	kfree(vs);
}

static struct rb_node *
vspace_search_mapping_node(struct pscnv_vspace *vs, uint64_t start)
{
	struct rb_node *node = vs->mappings.rb_node;
	while (node != NULL) {
		struct vs_mapping *mapping = rb_entry(node, struct vs_mapping, node);
		if (mapping->start > start) {
			node = node->rb_left;
		} else if (mapping->start < start) {
			node = node->rb_right;
		} else {
			return node;
		}
	}
	return NULL;
}

static void
vspace_insert_mapping(struct pscnv_vspace *vs, struct vs_mapping *mapping)
{
	struct rb_node *parent = NULL;
	struct rb_node **link = &vs->mappings.rb_node;
	struct vs_mapping *entry;
	while (*link != NULL) {
		parent = *link;
		entry = rb_entry(parent, struct vs_mapping, node);
		if (entry->start > mapping->start) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
		}
	}
	rb_link_node(&mapping->node, parent, link);
	rb_insert_color(&mapping->node, &vs->mappings);
}
int
pscnv_vspace_map(struct pscnv_vspace *vs, struct pscnv_bo *bo,
		uint64_t start, uint64_t end, int back, uint32_t flags,
		uint64_t *result)
{
	struct drm_pscnv_virt_private *dev_priv = vs->dev->dev_private;
	volatile struct pscnv_vspace_map_cmd *cmd;
	uint32_t call;
	struct vs_mapping *mapping;

	/* perform the map operation in the hypervisor */
	call = pscnv_virt_call_alloc(dev_priv);
	cmd = dev_priv->call_data->handle + call;
	cmd->command = PSCNV_CMD_VSPACE_MAP;
	cmd->vid = vs->vid;
	cmd->handle = bo->hyper_handle;
	cmd->back = back;
	cmd->start = start;
	cmd->end = end;
	cmd->flags = flags;
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(vs->dev, "pscnv_vspace_map: Command failed.\n");
		pscnv_virt_call_finish(dev_priv, call);
		return -EINVAL;
	}
	*result = cmd->offset;
	pscnv_virt_call_finish(dev_priv, call);

	/* insert an entry into the mapping list (necessary to track existing
	 * objects so that they can be freed later) */
	mapping = kzalloc(sizeof *mapping, GFP_KERNEL);
	if (mapping == NULL) {
		return -ENOMEM;
	}
	mapping->start = *result;
	mapping->bo = bo;
	mutex_lock(&vs->lock);
	vspace_insert_mapping(vs, mapping);
	mutex_unlock(&vs->lock);

	return 0;
}

int
pscnv_vspace_unmap(struct pscnv_vspace *vs, uint64_t start) {
	struct drm_pscnv_virt_private *dev_priv = vs->dev->dev_private;
	volatile struct pscnv_vspace_unmap_cmd *cmd;
	uint32_t call;
	struct rb_node *mapping_node;
	struct vs_mapping *mapping;

	call = pscnv_virt_call_alloc(dev_priv);
	cmd = dev_priv->call_data->handle + call;
	cmd->command = PSCNV_CMD_VSPACE_UNMAP;
	cmd->vid = vs->vid;
	cmd->offset = start;
	//NV_ERROR(vs->dev, "PSCNV_CMD_VSPACE_UNMAP.\n");
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(vs->dev, "pscnv_vspace_unmap: Command failed.\n");
		pscnv_virt_call_finish(dev_priv, call);
		return -EINVAL;
	}
	//NV_ERROR(vs->dev, "PSCNV_CMD_VSPACE_UNMAP finished.\n");
	pscnv_virt_call_finish(dev_priv, call);

	/* search for the mapping in the mapping list and decrease the refcount */
	mutex_lock(&vs->lock);
	mapping_node = vspace_search_mapping_node(vs, start);
	if (mapping_node == NULL) {
		NV_ERROR(vs->dev, "pscnv_vspace_unmap: Unknown mapping!\n");
	} else {
		mapping = rb_entry(mapping_node, struct vs_mapping, node);
		drm_gem_object_unreference(mapping->bo->gem);
	}
	mutex_unlock(&vs->lock);

	return 0;
}

