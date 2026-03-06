#ifndef OPENAMP_SUN20IW3_RSC_TAB_H_
#define OPENAMP_SUN20IW3_RSC_TAB_H_

#include <stddef.h>
#include <openamp/open_amp.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/sunxi_rproc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MULTI_OPENAMP_SUPPORT
#if defined(CONFIG_OPENAMP_TORV_RPROC_MASTER) || defined(CONFIG_OPENAMP_TODSP_RPROC_MASTER)
#define MULTI_OPENAMP_MASTER_MODE
#define OPENAMP_APPEND_RESOURCE_ENTRIES			4
#else
#define MULTI_OPENAMP_SLAVE_MODE
#define OPENAMP_APPEND_RESOURCE_ENTRIES			1
#endif
#else  /* CONFIG_MULTI_OPENAMP_SUPPORT */
#define OPENAMP_APPEND_RESOURCE_ENTRIES			0
#endif /* CONFIG_MULTI_OPENAMP_SUPPORT */

#ifdef CONFIG_AMP_TRACE_SUPPORT
#define AMP_TRACE_RESOURCE_ENTRIES			(1)
#else
#define AMP_TRACE_RESOURCE_ENTRIES			(0)
#endif

#ifdef CONFIG_AMP_SHARE_IRQ
#define AMP_SHARE_IRQ_ENTRIES				(1)
#else
#define AMP_SHARE_IRQ_ENTRIES				(0)
#endif

#define NUM_RESOURCE_ENTRIES	(2 + OPENAMP_APPEND_RESOURCE_ENTRIES + AMP_TRACE_RESOURCE_ENTRIES + AMP_SHARE_IRQ_ENTRIES)
#define NUM_VRINGS				2
#define VRING_NUM_BUFFS			128

#define VRING0_ID				1
#define VRING1_ID				2

#define RVDEV_SHM_LEN			(RPMSG_BUFFER_SIZE * VRING_NUM_BUFFS * NUM_VRINGS)
/*
 * The size of each should be not less than
 *     PAGE_ALIGN(vring_size(num, align))
 *   = PAGE_ALIGN(PAGE_ALIGN(16 * num + 6 + 2 * num) + 6 + 8 * num)
 *   if size == 8K,  the size should be not large than 227
 *   if size == 12K, the size should be not large than 454
 *   if size == 16K, the size should be not large than 454
 *   if size == 20K, the size should be not large than 682
 */
#define RVDEV_VRING_LEN			(VRING_SIZE(VRING_NUM_BUFFS, 64))

#define FW_RSC_ADDR_ANY 			(-1)

/*
 * This is not a real resource table put in the ".resource_table" section.
 * It is only used for the openamps between dsp and riscv to create their
 * remoteprocs and rpmsgs.
 */
struct dsp_and_rv_resource_table {
	uint32_t version;
	uint32_t num;
	uint32_t reserved[2];
	uint32_t offset[2];

	struct fw_rsc_carveout rvdev_shm;

	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0;
	struct fw_rsc_vdev_vring vring1;
} __attribute__((packed));

struct sunxi_resource_table {
	uint32_t version;
	uint32_t num;
	uint32_t reserved[2];
	uint32_t offset[NUM_RESOURCE_ENTRIES];

	/* shared memory entry for rpmsg virtIO device */
	struct fw_rsc_carveout rvdev_shm;

#ifdef CONFIG_AMP_TRACE_SUPPORT
	struct fw_rsc_aw_trace aw_trace;
#endif

	/* rpmsg vdev entry */
	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0;
	struct fw_rsc_vdev_vring vring1;
#ifdef MULTI_OPENAMP_MASTER_MODE
	/* carveouts used between dsp and riscv */
	struct fw_rsc_carveout dsp_and_rv_rsc_table;
	struct fw_rsc_carveout dsp_and_rv_rvdev_shm;
	struct fw_rsc_carveout dsp_and_rv_vring0;
	struct fw_rsc_carveout dsp_and_rv_vring1;
#endif
#ifdef MULTI_OPENAMP_SLAVE_MODE
	/* carveouts used between dsp and riscv */
	struct fw_rsc_carveout dsp_and_rv_rsc_table;
#endif
#ifdef CONFIG_AMP_SHARE_IRQ
	struct fw_rsc_carveout share_irq;
#endif
} __attribute__((packed));

#ifdef __cplusplus
}
#endif

#endif /* OPENAMP_SUN20IW3_RSC_TAB_H_ */
