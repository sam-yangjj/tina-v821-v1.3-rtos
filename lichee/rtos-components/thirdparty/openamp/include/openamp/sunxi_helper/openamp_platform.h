#ifndef __OPENAMP_SUNXI_PLARFORM_H_
#define __OPENAMP_SUNXI_PLARFORM_H_

#include <openamp/open_amp.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * No matter in which remoteproc/resource_table, the ID of vdev used for rpmsg
 * is always 0.
 */
#define RPMSG_VDEV_ID			0

/*
 * RPROC_CPU_OPS_ID is the index of rproc_global_impls array.
 * more infomation please refer to sunxi_helper/sunXXiwX/rproc_ops.c.
 *
 * RSC_CPU_ID is the number of resource_table.
 * usually 0 means that the resource_table is used to communicate with the cpu,
 * and 1 means that the resource_table is used to communicate with another core.
 * more infomation please refer to sunxi_helper/sunXXiwX/rsc_tab.c.
 */
#define RESOURCE_CPU_ID			0
#define RESOURCE_ANOTHER_ID		1

#define RPROC_CPU_ID			0			/* unique id for rproc */
#define RPROC_CPU_OPS_ID		0			/* index for rproc_ops */
#define RSC_CPU_ID				RESOURCE_CPU_ID

#define RPROC_DSP_ID			1
#define RPROC_DSP_OPS_ID		1
#define RSC_DSP_ID				RESOURCE_ANOTHER_ID

#define RPROC_RV_ID				1
#define RPROC_RV_OPS_ID			2
#define RSC_RV_ID				RESOURCE_ANOTHER_ID

#if defined(CONFIG_ARCH_SUN55IW3P1) || defined(CONFIG_ARCH_SUN55IW3)
#define NR_RPROC				2
#else
#define NR_RPROC				1
#endif

struct remoteproc *openamp_sunxi_create_rproc(int proc_id, int ops_id, int rsc_id);
void openamp_sunxi_release_rproc(struct remoteproc *rproc);
struct remoteproc *openamp_sunxi_get_rproc(int id);
int openamp_sunxi_get_rproc_id(struct remoteproc *rproc);

struct rpmsg_device *openamp_sunxi_create_rpmsg_vdev(
		struct remoteproc *rproc,
		int vdev_id, int role,
		void (*rst_cb)(struct virtio_device *vdev),
		rpmsg_ns_bind_cb ns_bind_cb);
void openamp_sunxi_release_rpmsg_vdev(struct remoteproc *rproc,
		struct rpmsg_device *rpmsg_dev);
struct rpmsg_device *openamp_sunxi_get_rpmsg_vdev(int id);
int openamp_sunxi_get_rpmsg_vdev_id(struct rpmsg_device *rpmsg_dev);

void resource_table_dump(void *rsc, int size);

#ifdef __cplusplus
}
#endif

#endif /* __OPENAMP_SUNXI_PLARFORM_H_ */
