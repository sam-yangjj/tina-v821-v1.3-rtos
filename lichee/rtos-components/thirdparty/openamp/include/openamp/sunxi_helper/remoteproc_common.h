#ifndef __OPENAMP_SUNXI_HELPER_REMOTEPROC_COMMON_H__
#define __OPENAMP_SUNXI_HELPER_REMOTEPROC_COMMON_H__

#include <openamp/open_amp.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/msgbox_ipi.h>

#define AMP_ALIGN(x, align)			((x + align -1 ) & ~(align - 1))
#define VRING_SIZE(num, align)		\
		AMP_ALIGN(AMP_ALIGN( \
				num * sizeof(struct vring_desc) + \
				sizeof(struct vring_avail) + \
				num * sizeof(uint16_t) + sizeof(uint16_t), \
				align) + \
			sizeof(struct vring_used) + \
			num * sizeof(struct vring_used_elem) + \
			sizeof(uint16_t), align)

struct rproc_mem_list_entry {
	struct remoteproc_mem mem;
	struct metal_list node;
};

struct rproc_io_list_entry {
	struct metal_io_region io;
	struct metal_list node;
};

struct vring_ipi {
	/* msgbox_ipi instance */
	struct msgbox_ipi *ipi;
	/* Some entries to create msgbox_ipi */
	struct msgbox_ipi_info info;
	msgbox_ipi_callback callback;

	/* A pointer to the remoteproc that the vring_ipi belongs to */
	struct remoteproc *rproc;
};

struct rproc_common_private {
	struct vring_ipi vring_ipi;
	struct metal_list mem_list;
	struct metal_list io_list;
};

extern struct remoteproc_ops rproc_common_ops;

void vring_ipi_common_callback(void *priv, uint32_t data);

struct remoteproc *rproc_common_init(struct remoteproc *rproc,
		struct remoteproc_ops *ops, void *arg);
void rproc_common_remove(struct remoteproc *rproc);
void *rproc_common_mmap(struct remoteproc *rproc,
		metal_phys_addr_t *pa, metal_phys_addr_t *da, size_t size,
		unsigned int attribute, struct metal_io_region **io);
int rproc_common_notify(struct remoteproc *rproc, uint32_t id);

#endif
