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

#ifndef __PSCNV_VRAM_H__
#define __PSCNV_VRAM_H__
#include "pscnv_drm.h"

#define PSCNV_MEM_PAGE_SIZE 0x1000

/* A VRAM object of any kind. */
struct pscnv_bo {
	struct drm_device *dev;
	/* size. Always a multiple of page size. */
	uint64_t size;
	/* misc flags, see below. */
	int flags;
	int tile_flags;
	/* cookie: free-form 32-bit number displayed in debug info. */
	uint32_t cookie;
	/* hypervisor object handle */
	uint32_t hyper_handle;
	/* the following used for GEM objects only */
	uint32_t user[8];
	struct drm_gem_object *gem;
	/* list of offsets into virtual vram */
	uint32_t *pages;
	/* CHAN only, pointer to a channel (FreeBSD doesn't allow overriding mmap) */
	struct pscnv_chan *chan;
};

extern struct pscnv_bo *pscnv_mem_alloc(struct drm_device *,
		uint64_t size, int flags, int tile_flags, uint32_t cookie);
extern int pscnv_mem_free(struct pscnv_bo *);

#endif
