
#include "pscnv_virt_call.h"

#include <linux/spinlock.h>

#define EXECUTE_CALL_REG 0x0
#define EXECUTE_INT_ACK_REG 0x4

void pscnv_virt_call_init(struct drm_pscnv_virt_private *dev)
{
	dev->free_call_slots = -1;
	sema_init(&dev->call_slot_count, PSCNV_CALL_SLOT_COUNT);
	spin_lock_init(&dev->call_slot_lock);
	init_waitqueue_head(&dev->irq_wq);
}

unsigned pscnv_virt_call_alloc(struct drm_pscnv_virt_private *dev)
{
	unsigned long flags;
	unsigned bit;
	uint32_t mask;

	down(&dev->call_slot_count);
	spin_lock_irqsave(&dev->call_slot_lock, flags);
	bit = fls(dev->free_call_slots) - 1;
	mask = 1 << bit;
	dev->free_call_slots &= ~mask;
	spin_unlock_irqrestore(&dev->call_slot_lock, flags);
	return bit * PSCNV_CALL_SLOT_SIZE;
}
void pscnv_virt_call_finish(struct drm_pscnv_virt_private *dev,
		unsigned buffer)
{
	unsigned long flags;
	unsigned bit;
	uint32_t mask;

	spin_lock_irqsave(&dev->call_slot_lock, flags);
	bit = buffer / PSCNV_CALL_SLOT_SIZE;
	mask = 1 << bit;
	dev->free_call_slots |= mask;
	spin_unlock_irqrestore(&dev->call_slot_lock, flags);
	up(&dev->call_slot_count);
}
void pscnv_virt_call(struct drm_pscnv_virt_private *dev, unsigned buffer)
{
	DRM_WRITE32(dev->mmio, EXECUTE_CALL_REG, buffer);
	/* wait for the result - the hypervisor overwrites the first word */
	while (wait_event_interruptible_timeout(dev->irq_wq,
			(DRM_READ32(dev->call_data, buffer) & 0x80000000) != 0,
			2 * HZ) < 0);
}

irqreturn_t nouveau_irq_handler(DRM_IRQ_ARGS)
{
	struct drm_device *dev = (struct drm_device *)arg;
	struct drm_pscnv_virt_private *dev_priv = dev->dev_private;

	DRM_WRITE32(dev_priv->mmio, EXECUTE_INT_ACK_REG, 0);

	NV_ERROR(dev, "Received an IRQ.\n");
	wake_up_interruptible(&dev_priv->irq_wq);

	return IRQ_HANDLED;
}
