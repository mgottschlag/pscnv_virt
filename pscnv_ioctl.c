#include <drm/drm.h>
#include "pscnv_virt_drv.h"
/*#include "nouveau_reg.h"*/
#include "pscnv_ioctl.h"
#include "pscnv_vm.h"
#include "pscnv_chan.h"
/*#include "pscnv_fifo.h"*/
#include "pscnv_gem.h"
#include "pscnv_drm.h"
/*#include "nv50_chan.h"
#include "nvc0_graph.h"
#include "pscnv_kapi.h"*/
#include "pscnv_mem.h"

//#include "nvc0_pgraph.xml.h"

#include "pscnv_virt_call.h"

#include <linux/spinlock.h>

#define PTIMER_TIME_REG 0x4
#define GPU_INFO_REG_BASE 0x8

#define PSCNV_INFO_PCI_VENDOR   0
#define PSCNV_INFO_PCI_DEVICE   1
#define PSCNV_INFO_BUS_TYPE     2
#define PSCNV_INFO_CHIPSET_ID   3
#define PSCNV_INFO_GRAPH_UNITS  4
#define PSCNV_INFO_GPC_COUNT    5
#define PSCNV_INFO_TP_COUNT_IDX 6
#define PSCNV_INFO_MP_COUNT     7
#define PSCNV_INFO_COUNT        8

static uint32_t pscnv_read_gpu_info(struct drm_pscnv_virt_private *dev,
				    uint32_t param) {
	return DRM_READ32(dev->mmio, GPU_INFO_REG_BASE + param * 4);
}

int pscnv_ioctl_getparam(struct drm_device *dev, void *data,
			 struct drm_file *file_priv)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	struct drm_pscnv_getparam *getparam = data;
	/*unsigned call;
	struct drm_local_map *call_data = dev_priv->call_data;*/

	switch ((u32)getparam->param) {
	case PSCNV_GETPARAM_MP_COUNT:
		getparam->value = pscnv_read_gpu_info(dev_priv,
				PSCNV_INFO_MP_COUNT);
		break;
	case PSCNV_GETPARAM_CHIPSET_ID:
		getparam->value = pscnv_read_gpu_info(dev_priv,
				PSCNV_INFO_CHIPSET_ID);
		/*call = pscnv_virt_call_alloc(dev_priv);
		NV_ERROR(dev, "prev. command: %08x\n", DRM_READ32(call_data, call));
		DRM_WRITE32(call_data, call, PSCNV_CMD_GET_PARAM);
		DRM_WRITE32(call_data, call + 4, PSCNV_GETPARAM_CHIPSET_ID);
		pscnv_virt_call(dev_priv, call);
		getparam->value = DRM_READ32(call_data, call + 4);
		NV_ERROR(dev, "get_param: status %08x\n", DRM_READ32(call_data, call));
		NV_ERROR(dev, "get_param: read %llx\n", getparam->value);
		pscnv_virt_call_finish(dev_priv, call);*/
		break;
	case PSCNV_GETPARAM_PCI_VENDOR:
		/* TODO: do we want the virtual device here? */
		getparam->value = pscnv_read_gpu_info(dev_priv,
				PSCNV_INFO_PCI_VENDOR);
		break;
	case PSCNV_GETPARAM_PCI_DEVICE:
		/* TODO: do we want the virtual device here? */
		getparam->value = pscnv_read_gpu_info(dev_priv,
				PSCNV_INFO_PCI_DEVICE);
		break;
	case PSCNV_GETPARAM_BUS_TYPE:
		/* TODO: do we want the virtual device here? */
		getparam->value = pscnv_read_gpu_info(dev_priv,
				PSCNV_INFO_BUS_TYPE);
		break;
	case PSCNV_GETPARAM_PTIMER_TIME:
		/* TODO */
		getparam->value = 0xdeadc0de;
		break;
	case PSCNV_GETPARAM_FB_SIZE:
		/* TODO: return vram bar size */
		getparam->value = dev_priv->vram_size;
		break;
	case PSCNV_GETPARAM_GPC_COUNT:
		getparam->value = pscnv_read_gpu_info(dev_priv,
				PSCNV_INFO_GPC_COUNT);
		break;
	case PSCNV_GETPARAM_TP_COUNT_IDX:
		getparam->value = pscnv_read_gpu_info(dev_priv,
				PSCNV_INFO_TP_COUNT_IDX);
		break;
	case PSCNV_GETPARAM_BAR0_ADDR:
		/* TODO: we do not have any nv50-style bar0 */
		goto fail;
		break;
	case PSCNV_GETPARAM_GRAPH_UNITS:
		getparam->value = pscnv_read_gpu_info(dev_priv,
				PSCNV_INFO_GRAPH_UNITS);
		break;
	default:
		goto fail;
	}

	return 0;
fail:
	NV_ERROR(dev, "unknown parameter %lld\n", getparam->param);
	return -EINVAL;
}

int pscnv_ioctl_gem_new(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	struct drm_pscnv_gem_info *info = data;
	struct drm_gem_object *obj;
	struct pscnv_bo *bo;
	int ret;

	obj = pscnv_gem_new(dev, info->size, info->flags, info->tile_flags, info->cookie, info->user);
	if (!obj) {
		return -ENOMEM;
	}
	bo = obj->driver_private;

	/* could change due to page size align */
	info->size = bo->size;
	info->handle = 0; /* FreeBSD expects this to be 0 else allocation fails */
	ret = drm_gem_handle_create(file_priv, obj, &info->handle);

#ifdef __linux__
	info->map_handle = (uint64_t)info->handle << 32;
#else
	info->map_handle = DRM_GEM_MAPPING_OFF(obj->map_list.key) |
			   DRM_GEM_MAPPING_KEY;
#endif
	drm_gem_object_handle_unreference_unlocked (obj);

	return ret;
}

int pscnv_ioctl_gem_info(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	struct drm_pscnv_gem_info *info = data;
	struct drm_gem_object *obj;
	struct pscnv_bo *bo;
	int i;

	obj = drm_gem_object_lookup(dev, file_priv, info->handle);
	if (!obj)
		return -EBADF;

	bo = obj->driver_private;

	info->cookie = bo->cookie;
	info->flags = bo->flags;
	info->tile_flags = bo->tile_flags;
	info->size = obj->size;
#ifdef __linux__
	info->map_handle = (uint64_t)info->handle | 32;
#else
	info->map_handle = DRM_GEM_MAPPING_OFF(obj->map_list.key) |
			   DRM_GEM_MAPPING_KEY;
#endif
	for (i = 0; i < DRM_ARRAY_SIZE(bo->user); i++)
		info->user[i] = bo->user[i];

	drm_gem_object_unreference_unlocked(obj);

	return 0;
}

static struct pscnv_vspace *
pscnv_get_vspace(struct drm_device *dev, struct drm_file *file_priv, int vid)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	unsigned long flags;
	spin_lock_irqsave(&dev_priv->vs_lock, flags);

	if (vid < 128 && vid >= 0 && dev_priv->vspaces[vid] && dev_priv->vspaces[vid]->filp == file_priv) {
		struct pscnv_vspace *res = dev_priv->vspaces[vid];
		pscnv_vspace_ref(res);
		spin_unlock_irqrestore(&dev_priv->vs_lock, flags);
		return res;
	}
	spin_unlock_irqrestore(&dev_priv->vs_lock, flags);
	return 0;
}

int pscnv_ioctl_vspace_new(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	struct drm_pscnv_vspace_req *req = data;
	struct pscnv_vspace *vspace;

	/* create a vspace */
	vspace = pscnv_vspace_new(dev);
	if (vspace == 0) {
		return -ENOMEM;
	}

	vspace->filp = file_priv;

	req->vid = vspace->vid;
	return 0;
}

int pscnv_ioctl_vspace_free(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	struct drm_pscnv_vspace_req *req = data;
	struct pscnv_vspace *vs;
	unsigned long flags;

	vs = pscnv_get_vspace(dev, file_priv, req->vid);
	if (!vs)
		return -ENOENT;

	spin_lock_irqsave(&dev_priv->vs_lock, flags);
	BUG_ON(dev_priv->vspaces[vs->vid] != vs);
	dev_priv->vspaces[vs->vid] = 0;
	spin_unlock_irqrestore(&dev_priv->vs_lock, flags);

	vs->filp = 0;
	pscnv_vspace_unref(vs);
	pscnv_vspace_unref(vs);

	return 0;
}

int pscnv_ioctl_vspace_map(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
#if 0
	struct drm_pscnv_vspace_map *req = data;
	struct pscnv_vspace *vs;
	struct drm_gem_object *obj;
	struct pscnv_bo *bo;
	struct pscnv_mm_node *map;
	int ret;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	vs = pscnv_get_vspace(dev, file_priv, req->vid);
	if (!vs)
		return -ENOENT;

	obj = drm_gem_object_lookup(dev, file_priv, req->handle);
	if (!obj) {
		pscnv_vspace_unref(vs);
		return -EBADF;
	}

	bo = obj->driver_private;

	ret = pscnv_vspace_map(vs, bo, req->start, req->end, req->back, &map);
	if (!ret)
		req->offset = map->start;

	pscnv_vspace_unref(vs);

	return ret;
#endif
	return -EINVAL;
}

int pscnv_ioctl_vspace_unmap(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
#if 0
	struct drm_pscnv_vspace_unmap *req = data;
	struct pscnv_vspace *vs;
	int ret;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;


	vs = pscnv_get_vspace(dev, file_priv, req->vid);
	if (!vs)
		return -ENOENT;

	ret = pscnv_vspace_unmap(vs, req->offset);

	pscnv_vspace_unref(vs);

	return ret;
#endif
	return -EINVAL;
}

void pscnv_vspace_cleanup(struct drm_device *dev, struct drm_file *file_priv) {
	int vid;
	struct pscnv_vspace *vs;

	for (vid = 0; vid < 128; vid++) {
		vs = pscnv_get_vspace(dev, file_priv, vid);
		if (!vs)
			continue;
		vs->filp = 0;
		pscnv_vspace_unref(vs);
		pscnv_vspace_unref(vs);
	}
}


struct pscnv_chan *
pscnv_get_chan(struct drm_device *dev, struct drm_file *file_priv, int cid)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	unsigned long flags;
	spin_lock_irqsave(&dev_priv->ch_lock, flags);

	if (cid < 128 && cid >= 0 && dev_priv->chans[cid] && dev_priv->chans[cid]->filp == file_priv) {
		struct pscnv_chan *res = dev_priv->chans[cid];
		pscnv_chan_ref(res);
		spin_unlock_irqrestore(&dev_priv->ch_lock, flags);
		return res;
	}
	spin_unlock_irqrestore(&dev_priv->ch_lock, flags);
	return 0;
}

int pscnv_ioctl_chan_new(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	struct drm_pscnv_chan_new *req = data;
	struct pscnv_vspace *vs;
	struct pscnv_chan *ch;

	vs = pscnv_get_vspace(dev, file_priv, req->vid);
	if (!vs)
		return -ENOENT;

	ch = pscnv_chan_new(dev, vs);
	if (!ch) {
		pscnv_vspace_unref(vs);
		return -ENOMEM;
	}
	pscnv_vspace_unref(vs);
	req->map_handle = 0xc0000000 | ch->cid << 16;

	req->cid = ch->cid;

	ch->filp = file_priv;

	return 0;
#if 0
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	struct drm_pscnv_chan_new *req = data;
	volatile struct pscnv_chan_new_cmd *cmd;
	uint32_t call;
	uint32_t vid = req->vid;
	uint32_t cid, map_handle;

	if (req->vid >= PSCNV_VIRT_VSPACE_COUNT
			|| dev_priv->vspaces[vid].filp != file_priv) {
		NV_ERROR(dev, "pscnv_ioctl_chan_new: Invalid vspace.\n");
		return -ENOENT;
	}
	/* create a chan */
	call = pscnv_virt_call_alloc(dev_priv);
	cmd = dev_priv->call_data->handle + call;
	cmd->command = PSCNV_CMD_CHAN_NEW;
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(dev, "Failed to create the chan.\n");
		return -ENOMEM;
	}
	cid = cmd->cid;
	map_handle = cmd->map_handle;
	pscnv_virt_call_finish(dev_priv, call);

	/* insert the file into the list to mark the vspace as allocated */
	if (cid >= PSCNV_VIRT_CHAN_COUNT) {
		NV_ERROR(dev, "Invalid chan id.\n");
		return -ENOMEM;
	}
	if (dev_priv->chans[cid].filp != NULL) {
		NV_ERROR(dev, "Bug: chan id already in use.\n");
		return -ENOMEM;
	}
	dev_priv->chans[cid].filp = file_priv;
	dev_priv->chans[cid].map_handle = 

	req->vid = vid;
	req->map_handle = ((uint64_t)1 << 32) + cid * 0x2000;

	return 0;
#endif
}

int pscnv_ioctl_chan_free(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	struct drm_pscnv_chan_free *req = data;
	struct pscnv_chan *ch;
	unsigned long flags;

	ch = pscnv_get_chan(dev, file_priv, req->cid);
	if (!ch)
		return -ENOENT;

	spin_lock_irqsave(&dev_priv->ch_lock, flags);
	BUG_ON(dev_priv->chans[ch->cid] != ch);
	dev_priv->chans[ch->cid] = 0;
	spin_unlock_irqrestore(&dev_priv->ch_lock, flags);

	ch->filp = 0;
	pscnv_chan_unref(ch);
	pscnv_chan_unref(ch);

	return 0;
#if 0
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	struct drm_pscnv_chan_free *req = data;
	volatile struct pscnv_chan_free_cmd *cmd;
	uint32_t call;

	if (req->cid >= PSCNV_VIRT_CHAN_COUNT
			|| dev_priv->chans[req->cid].filp != file_priv) {
		NV_ERROR(dev, "pscnv_ioctl_chan_free: invalid chan.\n");
		return -ENOENT;
	}

	dev_priv->chans[req->cid].filp = NULL;

	call = pscnv_virt_call_alloc(dev_priv);
	cmd = dev_priv->call_data->handle + call;
	cmd->command = PSCNV_CMD_CHAN_FREE;
	cmd->cid = req->cid;
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(dev, "Failed to free the chan.\n");
		return -ENOMEM;
	}
	pscnv_virt_call_finish(dev_priv, call);

	return 0;
#endif
}

int pscnv_ioctl_obj_vdma_new(struct drm_device *dev, void *data,
						struct drm_file *file_priv) {
#if 0
	struct drm_pscnv_obj_vdma_new *req = data;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct pscnv_chan *ch;
	int ret;
	uint32_t oclass, inst;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	if (dev_priv->card_type != NV_50)
		return -ENOSYS;

	oclass = req->oclass;

	if (oclass != 2 && oclass != 3 && oclass != 0x3d)
		return -EINVAL;

	ch = pscnv_get_chan(dev, file_priv, req->cid);
	if (!ch)
		return -ENOENT;

	inst = nv50_chan_dmaobj_new(ch, 0x7fc00000 | oclass, req->start, req->size);
	if (!inst) {
		pscnv_chan_unref(ch);
		return -ENOMEM;
	}

	ret = pscnv_ramht_insert (&ch->ramht, req->handle, inst >> 4);

	pscnv_chan_unref(ch);

	return ret;
#endif
	return -EINVAL;
}

void pscnv_chan_cleanup(struct drm_device *dev, struct drm_file *file_priv) {
	int cid;
	struct pscnv_chan *ch;

	for (cid = 0; cid < 128; cid++) {
		ch = pscnv_get_chan(dev, file_priv, cid);
		if (!ch)
			continue;
		ch->filp = 0;
		pscnv_chan_unref(ch);
		pscnv_chan_unref(ch);
	}
}

int pscnv_ioctl_obj_eng_new(struct drm_device *dev, void *data,
						struct drm_file *file_priv) {
#if 0
	struct drm_pscnv_obj_eng_new *req = data;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct pscnv_chan *ch;
	int ret;
	int i;
	uint32_t oclass = req->oclass;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	for (i = 0; i < PSCNV_ENGINES_NUM; i++)
		if (dev_priv->engines[i]) {
			uint32_t *pclass = dev_priv->engines[i]->oclasses;
			if (!pclass)
				continue;
			while (*pclass) {
				if (*pclass == oclass)
					goto found;
				pclass++;
			}
		}
	return -ENODEV;

found:
	ch = pscnv_get_chan(dev, file_priv, req->cid);
	if (!ch)
		return -ENOENT;

	if (!ch->engdata[i]) {
		ret = dev_priv->engines[i]->chan_alloc(dev_priv->engines[i], ch);
		if (ret) {
			pscnv_chan_unref(ch);
			return ret;
		}
	}

	ret = dev_priv->engines[i]->chan_obj_new(dev_priv->engines[i], ch, req->handle, oclass, req->flags);

	pscnv_chan_unref(ch);
	return ret;
#endif
	return -EINVAL;
}

int pscnv_ioctl_fifo_init(struct drm_device *dev, void *data,
						struct drm_file *file_priv) {
#if 0
	struct drm_pscnv_fifo_init *req = data;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct pscnv_chan *ch;
	int ret;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	if (!dev_priv->fifo || !dev_priv->fifo->chan_init_dma)
		return -ENODEV;

	ch = pscnv_get_chan(dev, file_priv, req->cid);
	if (!ch)
		return -ENOENT;

	ret = dev_priv->fifo->chan_init_dma(ch, req->pb_handle, req->flags, req->slimask, req->pb_start);

	pscnv_chan_unref(ch);

	return ret;
#endif
	return -EINVAL;
}

int pscnv_ioctl_fifo_init_ib(struct drm_device *dev, void *data,
						struct drm_file *file_priv) {
#if 0
	struct drm_pscnv_fifo_init_ib *req = data;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct pscnv_chan *ch;
	int ret;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	if (!dev_priv->fifo || !dev_priv->fifo->chan_init_ib)
		return -ENODEV;

	ch = pscnv_get_chan(dev, file_priv, req->cid);
	if (!ch)
		return -ENOENT;

	ret = dev_priv->fifo->chan_init_ib(ch, req->pb_handle, req->flags, req->slimask, req->ib_start, req->ib_order);

	pscnv_chan_unref(ch);

	return ret;
#endif
	return -EINVAL;
}
