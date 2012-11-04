
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

struct pscnv_map_cmd {
    uint32_t command;
    uint32_t handle;
    uint64_t result_table;
    uint32_t page_count;
};

struct pscnv_vspace_cmd {
    uint32_t command;
    uint32_t vid;
};

struct pscnv_chan_new_cmd {
	uint32_t command;
	uint32_t vid;
	uint32_t cid;
	uint32_t map_handle;
};

struct pscnv_chan_free_cmd {
    uint32_t command;
    uint32_t cid;
};


#define PSCNV_WRITE(map, addr, data, type) \
		*((volatile type*)((char*)map->handle + addr)) = data;

void pscnv_virt_call_init(struct drm_pscnv_virt_private *dev);

unsigned pscnv_virt_call_alloc(struct drm_pscnv_virt_private *dev);
void pscnv_virt_call_finish(struct drm_pscnv_virt_private *dev,
		unsigned buffer);
void pscnv_virt_call(struct drm_pscnv_virt_private *dev, unsigned buffer);

#endif
