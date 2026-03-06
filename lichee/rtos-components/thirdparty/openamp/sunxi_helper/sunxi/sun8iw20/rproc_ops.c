#include <openamp/open_amp.h>
#include <openamp/sunxi_helper/openamp_log.h>
#include <openamp/sunxi_helper/shmem_ops.h>
#include <openamp/sunxi_helper/msgbox_ipi.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/sunxi_rproc.h>
#include <openamp/sunxi_helper/remoteproc_common.h>
#include <hal_msgbox.h>
#include "rsc_tab.h"

#define MSGBOX_RISCV 1
#define MSGBOX_ARM   0

static struct rproc_common_private rproc_tocpu_priv = {
	.vring_ipi = {
		.ipi = NULL,
		.info = {
			.name = "cpu-vring-ipi",
			.remote_id = MSGBOX_ARM,
			.read_ch = 0,
			.write_ch = 0,
			.queue_size = CONFIG_MBOX_QUEUE_LENGTH,
		},
		.callback = vring_ipi_common_callback,
	},
};

struct rproc_global_impls sunxi_rproc_impls[] = {
	{ .ops = &rproc_common_ops, .priv = &rproc_tocpu_priv },
};

const size_t sunxi_rproc_impls_size = GET_RPROC_GLOBAL_IMPLS_ITEMS(sunxi_rproc_impls);
