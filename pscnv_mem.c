
#include "pscnv_mem.h"
#include "pscnv_virt_drv.h"

#include "pscnv_virt_call.h"

struct pscnv_bo *pscnv_mem_alloc(struct drm_device *dev,
		uint64_t size, int flags, int tile_flags, uint32_t cookie) {
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	/*uint32_t page_count = size / PSCNV_MEM_PAGE_SIZE;*/
	uint32_t call;
	struct drm_local_map *call_data = dev_priv->call_data;
	uint32_t status;
	/* struct pscnv_alloc_mem_cmd cmd;*/
	volatile struct pscnv_alloc_mem_cmd *cmd;
	struct pscnv_bo *res;

	/* allocate the buffer struct */
	res = kzalloc(sizeof *res, GFP_KERNEL);
	if (!res)
		return 0;
	size = (size + PSCNV_MEM_PAGE_SIZE - 1) & ~(PSCNV_MEM_PAGE_SIZE - 1);
	size = PAGE_ALIGN(size);
	res->dev = dev;
	res->size = size;
	res->flags = flags;
	res->tile_flags = tile_flags;
	res->cookie = cookie;

	/* call the hypervisor for memory allocation */
	call = pscnv_virt_call_alloc(dev_priv);
	cmd = call_data->handle + call;
	cmd->command = PSCNV_CMD_MEM_ALLOC;
	cmd->flags = flags;
	cmd->size = size;
	cmd->tile_flags = tile_flags;
	cmd->cookie = cookie;
	pscnv_virt_call(dev_priv, call);
	status = DRM_READ32(call_data, call);
	if (status != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(dev, "Memory allocation failed.\n");
		kfree(res);
		return 0;
	}
	res->hyper_handle = cmd->handle;
	pscnv_virt_call_finish(dev_priv, call);

	return res;
}
int pscnv_mem_free(struct pscnv_bo *bo) {
	NV_ERROR(bo->dev, "pscnv_mem_free not implemented.\n");
	/* TODO */
	return -1;
}

extern int pscnv_mmap(struct file *filp, struct vm_area_struct *vma) {
	struct drm_file *priv = filp->private_data;
	struct drm_device *dev = priv->minor->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_gem_object *obj;
	struct pscnv_bo *bo;
	int ret;

	if (vma->vm_pgoff * PAGE_SIZE < (1ull << 31))
		return drm_mmap(filp, vma);

	if (vma->vm_pgoff * PAGE_SIZE < (1ull << 32))
		/*return pscnv_chan_mmap(filp, vma);*/
		return -EINVAL; /* TODO */

	obj = drm_gem_object_lookup(dev, priv, (vma->vm_pgoff * PAGE_SIZE) >> 32);
	if (!obj)
		return -ENOENT;
	bo = obj->driver_private;

	if (vma->vm_end - vma->vm_start > bo->size) {
		drm_gem_object_unreference_unlocked(obj);
		return -EINVAL;
	}

	// TODO
	return -EINVAL;
}
