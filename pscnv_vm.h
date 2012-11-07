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

#ifndef __PSCNV_VM_H__
#define __PSCNV_VM_H__

struct pscnv_bo;
struct pscnv_chan;

struct pscnv_vspace {
	int vid;
	struct drm_device *dev;
	struct mutex lock;
	struct drm_file *filp;
	struct kref ref;
};


extern struct pscnv_vspace *pscnv_vspace_new(struct drm_device *);
extern int pscnv_vspace_map(struct pscnv_vspace *, struct pscnv_bo *,
		uint64_t start, uint64_t end, int back,
		uint32_t flags, uint64_t *result);
extern int pscnv_vspace_unmap(struct pscnv_vspace *, uint64_t start);

extern void pscnv_vspace_ref_free(struct kref *ref);

static inline void pscnv_vspace_ref(struct pscnv_vspace *vs) {
	kref_get(&vs->ref);
}

static inline void pscnv_vspace_unref(struct pscnv_vspace *vs) {
	kref_put(&vs->ref, pscnv_vspace_ref_free);
}

extern int pscnv_mmap(struct file *filp, struct vm_area_struct *vma);

#endif
