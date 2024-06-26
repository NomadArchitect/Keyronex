#ifndef KRX_DDK_DKVirtIOMMIOTransport_H
#define KRX_DDK_DKVirtIOMMIOTransport_H

#include "ddk/DKDevice.h"
#include "kdk/endian.h"
#include "kdk/vm.h"

struct vring_used_elem;

struct virtio_queue {
	/* queue number/index */
	uint16_t index;
	/* length of uqeue */
	uint16_t length;

	/* manipulation spinlock */
	kspinlock_t spinlock;

	/* index of first free descriptor */
	uint16_t free_desc_index;
	/* number of free descriptors */
	uint16_t nfree_descs;
	/* last seen used index */
	uint16_t last_seen_used;

	/* page out of which desc, avail, used are allocated */
	vm_page_t *page;

	/* virtual address of descriptor array */
	volatile struct vring_desc *desc;
	/* virtual address of driver ring */
	volatile struct vring_avail *avail;
	/* virtual address of device ring */
	volatile struct vring_used *used;

	/* for PCI - notification offset */
	uint16_t notify_off;
};

#define QUEUE_DESC_AT(PQUEUE, IDX) ((PQUEUE)->desc[IDX])

@protocol DKVirtIOTransport;

@protocol DKVirtIODelegate
+ (BOOL)probeWithProvider:(DKDevice<DKVirtIOTransport> *)provider;

- (void)deferredProcessing;
@end

/*!
 * VirtIO MMIO device nub.
 */
@protocol DKVirtIOTransport

@property (retain) DKDevice<DKVirtIODelegate> *delegate;
@property (readonly) volatile void *deviceConfig;

- (void)enqueueDPC;
- (void)resetDevice;
- (BOOL)exchangeFeatures:(uint64_t)required;
- (int)setupQueue:(struct virtio_queue *)queue index:(uint16_t)index;
- (int)enableDevice;
- (int)allocateDescNumOnQueue:(struct virtio_queue *)queue;
- (void)freeDescNum:(uint16_t)num onQueue:(struct virtio_queue *)queue;
- (void)submitDescNum:(uint16_t)descNum toQueue:(struct virtio_queue *)queue;
- (void)notifyQueue:(struct virtio_queue *)queue;

@end

#endif /* KRX_DDK_DKVirtIOMMIOTransport_H */
