#include <openamp/open_amp.h>
#include <openamp/sunxi_helper/openamp_log.h>
#include <openamp/sunxi_helper/shmem_ops.h>
#include <openamp/sunxi_helper/msgbox_ipi.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/sunxi_rproc.h>
#include <openamp/sunxi_helper/remoteproc_common.h>
#include <hal_msgbox.h>
#include <backtrace.h>
#ifdef CONFIG_ARCH_RISCV_PMP
#include <pmp.h>
#endif

#include "rsc_tab.h"

#define MSGBOX_ARM   0
#define MSGBOX_DSP   1
#define MSGBOX_CPUS  2
#define MSGBOX_RISCV 3

static struct rproc_common_private rproc_tocpu_priv = {
	.vring_ipi = {
		.ipi = NULL,
		.info = {
			.name = "cpu-vring-ipi",
			.remote_id = MSGBOX_ARM,
			.read_ch = CONFIG_MBOX_CHANNEL,
			.write_ch = CONFIG_MBOX_CHANNEL,
			.queue_size = CONFIG_MBOX_QUEUE_LENGTH,
		},
		.callback = vring_ipi_common_callback,
	},
};

#ifndef CONFIG_TODSP_MBOX_CHANNEL
#define CONFIG_TODSP_MBOX_CHANNEL			CONFIG_MBOX_CHANNEL
#endif
#ifndef CONFIG_TODSP_MBOX_QUEUE_LENGTH
#define CONFIG_TODSP_MBOX_QUEUE_LENGTH		CONFIG_MBOX_QUEUE_LENGTH
#endif
static struct rproc_common_private rproc_todsp_priv = {
	.vring_ipi = {
		.ipi = NULL,
		.info = {
			.name = "dsp-vring-ipi",
			.remote_id = MSGBOX_DSP,
			.read_ch = CONFIG_TODSP_MBOX_CHANNEL,
			.write_ch = CONFIG_TODSP_MBOX_CHANNEL,
			.queue_size = CONFIG_TODSP_MBOX_QUEUE_LENGTH,
		},
		.callback = vring_ipi_common_callback,
	},
};

#ifndef CONFIG_TORV_MBOX_CHANNEL
#define CONFIG_TORV_MBOX_CHANNEL			CONFIG_MBOX_CHANNEL
#endif
#ifndef CONFIG_TORV_MBOX_QUEUE_LENGTH
#define CONFIG_TORV_MBOX_QUEUE_LENGTH		CONFIG_MBOX_QUEUE_LENGTH
#endif
static struct rproc_common_private rproc_torv_priv = {
	.vring_ipi = {
		.ipi = NULL,
		.info = {
			.name = "rv-vring-ipi",
			.remote_id = MSGBOX_RISCV,
			.read_ch = CONFIG_TORV_MBOX_CHANNEL,
			.write_ch = CONFIG_TORV_MBOX_CHANNEL,
			.queue_size = CONFIG_TORV_MBOX_QUEUE_LENGTH,
		},
		.callback = vring_ipi_common_callback,
	},
};

struct rproc_global_impls sunxi_rproc_impls[] = {
	{ .ops = &rproc_common_ops, .priv = &rproc_tocpu_priv },
	{ .ops = &rproc_common_ops, .priv = &rproc_todsp_priv },
	{ .ops = &rproc_common_ops, .priv = &rproc_torv_priv },
};

const size_t sunxi_rproc_impls_size = GET_RPROC_GLOBAL_IMPLS_ITEMS(sunxi_rproc_impls);
