/*
 * Copyright 2005 Stephane Marchesin.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __NOUVEAU_DRV_H__
#define __NOUVEAU_DRV_H__

#include <drm/drmP.h>
#define drm_get_resource_start(dev, x) pci_resource_start((dev)->pdev, (x))
#define drm_get_resource_len(dev, x) pci_resource_len((dev)->pdev, (x))
#include <linux/kref.h>

#define DRIVER_AUTHOR       "Mathias Gottschlag"
#define DRIVER_EMAIL        "mathias.gottschlag@student.kit.edu"

#define DRIVER_NAME     "pscnv"
#define DRIVER_DESC     "nVidia NV50"
#define DRIVER_DATE     "20121007"

#define DRIVER_MAJOR        0
#define DRIVER_MINOR        0
#define DRIVER_PATCHLEVEL   16

#define DRM_FILE_PAGE_OFFSET (0x100000000ULL >> PAGE_SHIFT)

typedef void (*pscnv_virt_irqhandler_t) (struct drm_device *dev, int irq);

#define MAX_NUM_DCB_ENTRIES 16

#define NOUVEAU_MAX_CHANNEL_NR 128
#define NOUVEAU_MAX_TILE_NR 15

#define NV50_VM_MAX_VRAM (2*1024*1024*1024ULL)
#define NV50_VM_BLOCK    (512*1024*1024ULL)
#define NV50_VM_VRAM_NR  (NV50_VM_MAX_VRAM / NV50_VM_BLOCK)

#define PSCNV_VIRT_MMIO_SIZE 0x1000
#define PSCNV_VIRT_VRAM_SIZE 0x10000000

#define PSCNV_CALL_SLOT_SIZE 0x100
#define PSCNV_CALL_SLOT_COUNT 32
#define PSCNV_CALL_AREA_SIZE (PSCNV_CALL_SLOT_COUNT * PSCNV_CALL_SLOT_SIZE)

enum nouveau_card_type {
	NV_01      = 0x01,
	NV_02      = 0x02,
	NV_03      = 0x03,
	NV_04      = 0x04,
	NV_10      = 0x10,
	NV_20      = 0x20,
	NV_30      = 0x30,
	NV_40      = 0x40,
	NV_50      = 0x50,
	NV_C0      = 0xc0,
	NV_D0      = 0xd0,
	NV_E0      = 0xe0,
};

#define PSCNV_VIRT_VSPACE_COUNT 128

#define PSCNV_VIRT_CHAN_COUNT 128

struct drm_pscnv_virt_private {
	struct drm_device *dev;

	int flags;

	/* the card type, takes NV_* as values */
	/*enum nouveau_card_type card_type;*/
	/* exact chipset, derived from NV_PMC_BOOT_0 */
	/*int chipset;
	int flags;*/

	/*pscnv_virt_irqhandler_t irq_handler[32];*/

	struct drm_local_map *mmio;
	struct drm_local_map *call_data;
	uint32_t free_call_slots;
	struct semaphore call_slot_count;
	spinlock_t call_slot_lock;
	wait_queue_head_t irq_wq;

	uint64_t vram_size;
	uint64_t vram_base;

	struct pscnv_vspace *vspaces[PSCNV_VIRT_VSPACE_COUNT];
	struct pscnv_chan *chans[PSCNV_VIRT_CHAN_COUNT];
	spinlock_t vs_lock;
	spinlock_t ch_lock;

	/*struct nouveau_fbdev *nfbdev;
	struct apertures_struct *apertures;*/
};

#ifdef __linux__
extern int nouveau_pci_suspend(struct pci_dev *pdev, pm_message_t pm_state);
extern int nouveau_pci_resume(struct pci_dev *pdev);
#endif

/* nouveau_state.c */
extern void nouveau_preclose(struct drm_device *dev, struct drm_file *);
extern int  nouveau_load(struct drm_device *, unsigned long flags);
extern int  nouveau_firstopen(struct drm_device *);
extern void nouveau_lastclose(struct drm_device *);
extern int  nouveau_unload(struct drm_device *);
extern bool nouveau_wait_until(struct drm_device *, uint64_t timeout,
                   uint32_t reg, uint32_t mask, uint32_t val);
extern bool nouveau_wait_until_neq(struct drm_device *, uint64_t timeout,
                   uint32_t reg, uint32_t mask, uint32_t val);
extern bool nouveau_wait_cb(struct drm_device *, uint64_t timeout,
                bool (*cond)(void *), void *);
//extern bool nouveau_wait_for_idle(struct drm_device *);
extern int  nouveau_card_init(struct drm_device *);

/* nouveau_irq.c */
extern irqreturn_t nouveau_irq_handler(DRM_IRQ_ARGS);
extern void        nouveau_irq_preinstall(struct drm_device *);
extern int         nouveau_irq_postinstall(struct drm_device *);
extern void        nouveau_irq_uninstall(struct drm_device *);
extern void        nouveau_irq_register(struct drm_device *, int irq, pscnv_virt_irqhandler_t handler);
extern void        nouveau_irq_unregister(struct drm_device *, int irq);

static inline int
nouveau_debugfs_init(struct drm_minor *minor)
{
	return 0;
}

static inline void nouveau_debugfs_takedown(struct drm_minor *minor)
{
}

/* nouveau_acpi.c */
#define ROM_BIOS_PAGE 4096
#if defined(CONFIG_ACPI)
void nouveau_register_dsm_handler(void);
void nouveau_unregister_dsm_handler(void);
int nouveau_acpi_get_bios_chunk(uint8_t *bios, int offset, int len);
bool nouveau_acpi_rom_supported(struct pci_dev *pdev);
int nouveau_acpi_edid(struct drm_device *, struct drm_connector *);
#elif defined(__linux__)
static inline void nouveau_register_dsm_handler(void) {}
static inline void nouveau_unregister_dsm_handler(void) {}
static inline bool nouveau_acpi_rom_supported(struct pci_dev *pdev) { return false; }
static inline int nouveau_acpi_get_bios_chunk(uint8_t *bios, int offset, int len) { return -EINVAL; }
static inline int nouveau_acpi_edid(struct drm_device *dev, struct drm_connector *connector) { return -EINVAL; }
#else
static inline int nouveau_acpi_edid(struct drm_device *dev, struct drm_connector *connector) { return -EINVAL; }
#endif

/* nouveau_backlight.c */
#ifdef CONFIG_DRM_NOUVEAU_BACKLIGHT
extern int nouveau_backlight_init(struct drm_device *);
extern void nouveau_backlight_exit(struct drm_device *);
#else
static inline int nouveau_backlight_init(struct drm_device *dev)
{
	return 0;
}

static inline void nouveau_backlight_exit(struct drm_device *dev) { }
#endif

/* nouveau_display.c */
int nouveau_display_create(struct drm_device *dev);
void nouveau_display_destroy(struct drm_device *dev);

/* nv04_timer.c */
extern int  nv04_timer_init(struct drm_device *);
extern uint64_t nv04_timer_read(struct drm_device *);

extern long nouveau_compat_ioctl(struct file *file, unsigned int cmd,
                 unsigned long arg);

static inline u32 nv_rd32(struct drm_device *dev, unsigned reg)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	return DRM_READ32(dev_priv->mmio, reg);
}

static inline u32 nv_rd08(struct drm_device *dev, unsigned reg)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	return DRM_READ8(dev_priv->mmio, reg);
}

static inline void nv_wr32(struct drm_device *dev, unsigned reg, u32 val)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	DRM_WRITE32(dev_priv->mmio, reg, val);
}

static inline void nv_wr08(struct drm_device *dev, unsigned reg, u8 val)
{
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;
	DRM_WRITE8(dev_priv->mmio, reg, val);
}

static inline u32 nv_mask(struct drm_device *dev, u32 reg, u32 mask, u32 val)
{
	u32 tmp = nv_rd32(dev, reg);
	nv_wr32(dev, reg, (tmp & ~mask) | val);
	return tmp;
}

#define nv_wait(dev, reg, mask, val) \
	nouveau_wait_until(dev, 2000000000ULL, (reg), (mask), (val))

#define nv_wait_neq(dev, reg, mask, val) \
	nouveau_wait_until_neq(dev, 2000000000ULL, (reg), (mask), (val))
#define nv_wait_cb(dev, func, data) \
	nouveau_wait_cb(dev, 2000000000ULL, (func), (data))

/*
 * Logging
 * Argument d is (struct drm_device *).
 */
#ifdef __linux__
#define NV_PRINTK(level, d, fmt, arg...) \
	printk(level "[" DRM_NAME "] " DRIVER_NAME " %s: " fmt, \
                    pci_name(d->pdev), ##arg)
#ifndef NV_DEBUG_NOTRACE
#define NV_DEBUG(d, fmt, arg...) do {                                          \
	if (drm_debug & DRM_UT_DRIVER) {                                       \
		NV_PRINTK(KERN_DEBUG, d, "%s:%d - " fmt, __func__,             \
				__LINE__, ##arg);                                    \
	}                                                                      \
} while (0)
#define NV_DEBUG_KMS(d, fmt, arg...) do {                                      \
	if (drm_debug & DRM_UT_KMS) {                                          \
		NV_PRINTK(KERN_DEBUG, d, "%s:%d - " fmt, __func__,             \
				__LINE__, ##arg);                                    \
	}                                                                      \
} while (0)
#else
#define NV_DEBUG(d, fmt, arg...) do {                                          \
	if (drm_debug & DRM_UT_DRIVER)                                         \
		NV_PRINTK(KERN_DEBUG, d, fmt, ##arg);                          \
} while (0)
#define NV_DEBUG_KMS(d, fmt, arg...) do {                                      \
	if (drm_debug & DRM_UT_KMS)                                            \
		NV_PRINTK(KERN_DEBUG, d, fmt, ##arg);                          \
} while (0)
#endif
#define NV_ERROR(d, fmt, arg...) NV_PRINTK(KERN_ERR, d, fmt, ##arg)
#define NV_INFO(d, fmt, arg...) NV_PRINTK(KERN_INFO, d, fmt, ##arg)
#define NV_TRACEWARN(d, fmt, arg...) NV_PRINTK(KERN_NOTICE, d, fmt, ##arg)
#define NV_TRACE(d, fmt, arg...) NV_PRINTK(KERN_INFO, d, fmt, ##arg)
#define NV_WARN(d, fmt, arg...) NV_PRINTK(KERN_WARNING, d, fmt, ##arg)
#else
#define NV_ERROR(d, fmt, arg...) do { (void)d; DRM_ERROR(fmt, ##arg); } while (0)
#define NV_INFO(d, fmt, arg...) do { (void)d; DRM_INFO(fmt, ##arg); } while (0)
#define NV_DEBUG(d, fmt, arg...) do { (void)d; DRM_DEBUG_DRIVER(fmt, ##arg); } while (0)
#define NV_DEBUG_KMS(d, fmt, arg...) do { (void)d; DRM_DEBUG_KMS(fmt, ##arg); } while (0)
#define NV_TRACEWARN(d, fmt, arg...) do { (void)d; DRM_INFO(fmt, ##arg); } while (0)
#define NV_TRACE(d, fmt, arg...) do { (void)d; DRM_INFO(fmt, ##arg); } while (0)
#define NV_WARN(d, fmt, arg...) do { (void)d; DRM_INFO(fmt, ##arg); } while (0)
#endif

static inline bool
nv_match_device(struct drm_device *dev, unsigned device,
		unsigned sub_vendor, unsigned sub_device)
{
	return dev->pdev->device == device &&
		dev->pdev->subsystem_vendor == sub_vendor &&
		dev->pdev->subsystem_device == sub_device;
}

#endif /* __NOUVEAU_DRV_H__ */
