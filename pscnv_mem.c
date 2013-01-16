
#include "pscnv_mem.h"
#include "pscnv_virt_drv.h"
#include "pscnv_chan.h"

#include "pscnv_virt_call.h"

struct pscnv_bo *pscnv_mem_alloc(struct drm_device *dev,
		uint64_t size, int flags, int tile_flags, uint32_t cookie) {
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	/*uint32_t page_count = size / PSCNV_MEM_PAGE_SIZE;*/
	uint32_t call, status;
	struct drm_local_map *call_data = dev_priv->call_data;
	volatile struct pscnv_alloc_mem_cmd *cmd;
	struct pscnv_bo *res;
	uint64_t orig_size = size;

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
	cmd->size = orig_size;
	cmd->tile_flags = tile_flags;
	cmd->cookie = cookie;
	pscnv_virt_call(dev_priv, call);
	status = DRM_READ32(call_data, call);
	if (status != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(dev, "Memory allocation failed.\n");
		kfree(res);
		pscnv_virt_call_finish(dev_priv, call);
		return 0;
	}
	res->hyper_handle = cmd->handle;
	pscnv_virt_call_finish(dev_priv, call);

	return res;
}
int pscnv_mem_free(struct pscnv_bo *bo) {
	struct drm_pscnv_virt_private *dev_priv = bo->dev->dev_private;
	volatile struct pscnv_free_mem_cmd *cmd;
	uint32_t call, status;
	struct drm_local_map *call_data = dev_priv->call_data;

	call = pscnv_virt_call_alloc(dev_priv);
	cmd = call_data->handle + call;
	cmd->command = PSCNV_CMD_MEM_FREE;
	cmd->handle = bo->hyper_handle;
	pscnv_virt_call(dev_priv, call);
	status = DRM_READ32(call_data, call);
	if (status != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(bo->dev, "PSCNV_CMD_MEM_FREE failed.\n");
	}
	pscnv_virt_call_finish(dev_priv, call);

	kfree(bo);
	return -1;
}

static struct vm_operations_struct pscnv_vm_ops = {
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

extern int pscnv_mmap(struct file *filp, struct vm_area_struct *vma) {
	struct drm_file *priv = filp->private_data;
	struct drm_device *dev = priv->minor->dev;
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	struct drm_gem_object *obj;
	struct pscnv_bo *bo;
	uint32_t call;
	struct drm_local_map *call_data = dev_priv->call_data;
	volatile struct pscnv_map_cmd *cmd;
	unsigned int page_count;

	if (vma->vm_pgoff * PAGE_SIZE < (1ull << 31))
		return drm_mmap(filp, vma);

	if (vma->vm_pgoff * PAGE_SIZE < (1ull << 32))
		return pscnv_chan_mmap(filp, vma);

	obj = drm_gem_object_lookup(dev, priv, (vma->vm_pgoff * PAGE_SIZE) >> 32);
	if (!obj)
		return -ENOENT;
	bo = obj->driver_private;

	if (vma->vm_end - vma->vm_start > bo->size) {
		drm_gem_object_unreference_unlocked(obj);
		return -EINVAL;
	}

	/* tell the hypervisor to map the object */
	/* TODO: Skip this if the object is already mapped */
	page_count = bo->size >> PAGE_SHIFT;
	call = pscnv_virt_call_alloc(dev_priv);
	cmd = call_data->handle + call;
	cmd->command = PSCNV_CMD_MAP;
	cmd->handle = bo->hyper_handle;
	pscnv_virt_call(dev_priv, call);
	if (cmd->command != PSCNV_RESULT_NO_ERROR) {
		NV_ERROR(dev, "Mapping failed.\n");
		pscnv_virt_call_finish(dev_priv, call);
		/* TODO: revert everything */
		drm_gem_object_unreference_unlocked(obj);
		return -ENOMEM;
	}
	bo->start = cmd->start;
	pscnv_virt_call_finish(dev_priv, call);

	/* remap the object into user memory (the actual mapping is done in the
	   pagefault handler) */
	vma->vm_flags |= VM_RESERVED | VM_IO | VM_PFNMAP | VM_DONTEXPAND;
	vma->vm_ops = &pscnv_vm_ops;
	vma->vm_private_data = obj;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	/*vma->vm_page_prot = pgprot_writecombine(vm_get_page_prot(vma->vm_flags));*/

	vma->vm_file = filp;

	return remap_pfn_range(vma, vma->vm_start, 
			(dev_priv->vram_base + bo->start) >> PAGE_SHIFT,
			vma->vm_end - vma->vm_start, PAGE_SHARED);
}
