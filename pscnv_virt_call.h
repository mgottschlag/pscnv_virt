
#ifndef __PSCNV_VIRT_RING_H__
#define __PSCNV_VIRT_RING_H__

#include "pscnv_virt_drv.h"

#define PSCNV_CALL_PARAM_SIZE (PSCNV_CALL_SLOT_SIZE - 4)

#define PSCNV_CMD_GET_PARAM 1
#define PSCNV_CMD_MEM_ALLOC 2
#define PSCNV_CMD_MAP 3
#define PSCNV_CMD_MEM_FREE 4
#define PSCNV_CMD_VSPACE_ALLOC 5
#define PSCNV_CMD_VSPACE_FREE 6
#define PSCNV_CMD_CHAN_NEW 7
#define PSCNV_CMD_CHAN_FREE 8
#define PSCNV_CMD_VSPACE_MAP 9
#define PSCNV_CMD_VSPACE_UNMAP 10
#define PSCNV_CMD_OBJ_VDMA_NEW 11
#define PSCNV_CMD_OBJ_ENG_NEW 12
#define PSCNV_CMD_FIFO_INIT 13
#define PSCNV_CMD_FIFO_INIT_IB 14

#define PSCNV_RESULT_NO_ERROR 0x80000000
#define PSCNV_RESULT_UNKNOWN_CMD 0x80000001
#define PSCNV_RESULT_ERROR 0x80000002

struct pscnv_alloc_mem_cmd {
    uint32_t command;
    uint32_t flags;
    uint64_t size;
    uint32_t tile_flags;
    uint32_t cookie;
    uint32_t handle;
};

struct pscnv_free_mem_cmd {
    uint32_t command;
    uint32_t handle;
};

struct pscnv_map_cmd {
    uint32_t command;
    uint32_t handle;
    uint32_t start;
};

struct pscnv_vspace_cmd {
    uint32_t command;
    uint32_t vid;
};

struct pscnv_chan_new_cmd {
	uint32_t command;
	uint32_t vid;
	uint32_t cid;
};

struct pscnv_chan_free_cmd {
    uint32_t command;
    uint32_t cid;
};

struct pscnv_vspace_map_cmd {
	uint32_t command;
	uint32_t vid;
	uint32_t handle;
	uint32_t back;
	uint64_t start;
	uint64_t end;
	uint64_t offset;
	uint32_t flags;
};

struct pscnv_vspace_unmap_cmd {
	uint32_t command;
	uint32_t vid;
	uint64_t offset;
};

struct pscnv_obj_vdma_new_cmd {
	uint32_t command;
	uint32_t cid;
	uint32_t handle;
	uint32_t oclass;
	uint64_t start;
	uint64_t size;
	uint32_t flags;
	int32_t ret;
};

struct pscnv_fifo_init_cmd {
	uint32_t command;
	uint32_t cid;
	uint32_t pb_handle;
	uint32_t flags;
	uint64_t pb_start;
	uint32_t slimask;
	int32_t ret;
};

struct pscnv_fifo_init_ib_cmd {
	uint32_t command;
	uint32_t cid;
	uint32_t pb_handle;
	uint32_t flags;
	uint64_t ib_start;
	uint32_t slimask;
	uint32_t ib_order;
	int32_t ret;
};

struct pscnv_obj_eng_new_cmd {
	uint32_t command;
	uint32_t cid;
	uint32_t handle;
	uint32_t oclass;
	uint32_t flags;
	int32_t ret;
};


#define PSCNV_WRITE(map, addr, data, type) \
		*((volatile type*)((char*)map->handle + addr)) = data;

void pscnv_virt_call_init(struct drm_pscnv_virt_private *dev);

unsigned pscnv_virt_call_alloc(struct drm_pscnv_virt_private *dev);
void pscnv_virt_call_finish(struct drm_pscnv_virt_private *dev,
		unsigned buffer);
void pscnv_virt_call(struct drm_pscnv_virt_private *dev, unsigned buffer);

#endif
