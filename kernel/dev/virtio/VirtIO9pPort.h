#ifndef KRX_VIRTIO_VIRTIO9PTRANSPORT_H
#define KRX_VIRTIO_VIRTIO9PTRANSPORT_H

#include "ddk/DKDevice.h"
#include "ddk/DKVirtIOTransport.h"
#include "dev/safe_endian.h"

@interface VirtIO9pPort : DKDevice <DKVirtIODelegate> {
@public
	TAILQ_TYPE_ENTRY(VirtIO9pPort) m_tagListEntry;

@protected
	char m_tagName[64];

	struct virtio_queue m_reqQueue;

	/*! I/O packets waiting for submission. */
	TAILQ_HEAD(, iop) pending_packets;

	/*! Base of requests array. */
	struct vio9p_req *m_requests;
	/*! Request freelist. */
	TAILQ_HEAD(, vio9p_req) free_reqs;
	/*! Virtio requests currently running. */
	TAILQ_HEAD(, vio9p_req) in_flight_reqs;
}

+ (VirtIO9pPort *)forTag:(const char *)tag;

@end

#endif /* KRX_VIRTIO_VIRTIO9PTRANSPORT_H */
