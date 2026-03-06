#include <openamp/sunxi_helper/shmem_ops.h>
#include <hal_cache.h>
#include "rsc_tab.h"

#define RPMSG_IPU_C0_FEATURES			(1 << VIRTIO_RPMSG_F_NS)

#define VRING_TX_ADDRESS				FW_RSC_U32_ADDR_ANY
#define VRING_RX_ADDRESS 				FW_RSC_U32_ADDR_ANY

#define VRING_ALIGN						4096

#define SHMEM_PA						FW_RSC_U32_ADDR_ANY
#define SHMEM_DA						FW_RSC_U32_ADDR_ANY
#define SHMEM_SIZE						0x40000			/* 256K */

#define __section_t(s)					__attribute((__section__(#s)))
#define __resource						__section_t(.resource_table)

#ifdef CONFIG_AMP_TRACE_SUPPORT
extern char amp_log_buffer[];
#endif

/**
 *	Note: vdev da and vring da need to same with linux dts reserved-memory
 *	otherwise linux will alloc new memory
 * */
static struct sunxi_resource_table __resource resource_table = {
	.version = 1,
	.num = NUM_RESOURCE_ENTRIES,
	.reserved = {0, 0},
	.offset = {
		offsetof(struct sunxi_resource_table, rvdev_shm),
#ifdef CONFIG_AMP_TRACE_SUPPORT
		offsetof(struct sunxi_resource_table, aw_trace),
#endif
		offsetof(struct sunxi_resource_table, vdev),
#ifdef MULTI_OPENAMP_MASTER_MODE
		offsetof(struct sunxi_resource_table, dsp_and_rv_rsc_table),
		offsetof(struct sunxi_resource_table, dsp_and_rv_rvdev_shm),
		offsetof(struct sunxi_resource_table, dsp_and_rv_vring0),
		offsetof(struct sunxi_resource_table, dsp_and_rv_vring1),
#endif
#ifdef MULTI_OPENAMP_SLAVE_MODE
		offsetof(struct sunxi_resource_table, dsp_and_rv_rsc_table),
#endif
#ifdef CONFIG_AMP_SHARE_IRQ
		offsetof(struct sunxi_resource_table, share_irq),
#endif
#ifdef CONFIG_COMPONENTS_AMP_USER_RESOURCE
		AMP_USER_RESOURCE_MEMBERS_OFFSET
#endif
	},
	.rvdev_shm = {
		.type = RSC_CARVEOUT,
		.da = SHMEM_DA,
		.pa = SHMEM_PA,
		.len = SHMEM_SIZE,
		.flags = MEM_CACHEABLE,
		.reserved = 0,
		.name = "vdev0buffer",
	},

#ifdef CONFIG_AMP_TRACE_SUPPORT
	.aw_trace = {
		.type = RSC_AW_TRACE,
		.da = (uint32_t)amp_log_buffer,
		.len = CONFIG_AMP_TRACE_BUF_SIZE,
		.reserved = 0,
		.name = "log",
	},
#endif

#ifdef CONFIG_COMPONENTS_AMP_USER_RESOURCE
	AMP_USER_RESOURCE_MEMBERS_DATA
#endif

	.vdev = {
		.type = RSC_VDEV,
		.id = VIRTIO_ID_RPMSG,
		.notifyid = 0,
		.dfeatures = RPMSG_IPU_C0_FEATURES,
		.gfeatures = RPMSG_IPU_C0_FEATURES,
		.config_len = 0,
		.status = 0,
		.num_of_vrings = NUM_VRINGS,
		.reserved = {0, 0},
	},

	.vring0 = {
		.da = VRING_TX_ADDRESS,
		.align = VRING_ALIGN,
		.num = VRING_NUM_BUFFS,
		.notifyid = VRING0_ID,
		.reserved = 0,
	},

	.vring1 = {
		.da = VRING_RX_ADDRESS,
		.align = VRING_ALIGN,
		.num = VRING_NUM_BUFFS,
		.notifyid = VRING1_ID,
		.reserved = 0,
	},

#ifdef MULTI_OPENAMP_MASTER_MODE
	/* it will reserve a memroy entry ot the same name in dts */
	.dsp_and_rv_rsc_table = {
		.type = RSC_CARVEOUT,
		.da = FW_RSC_U32_ADDR_ANY,
		.pa = FW_RSC_U32_ADDR_ANY,
		.len = sizeof(struct dsp_and_rv_resource_table),
		.flags = MEM_NONCACHEABLE,
		.reserved = 0,	/* must be zero */
		.name = "dsp_and_rv_rsc_table",
	},
	/* it will reserve a memroy entry ot the same name in dts */
	.dsp_and_rv_rvdev_shm = {
		.type = RSC_CARVEOUT,
		.da = FW_RSC_U32_ADDR_ANY,
		.pa = FW_RSC_U32_ADDR_ANY,
		.len = RVDEV_SHM_LEN,
		.flags = MEM_NONCACHEABLE,
		.reserved = 0,	/* must be zero */
		.name = "dsp_and_rv_vdevbuffer",
	},
	/* it will reserve a memroy entry ot the same name in dts */
	.dsp_and_rv_vring0 = {
		.type = RSC_CARVEOUT,
		.da = FW_RSC_U32_ADDR_ANY,
		.pa = FW_RSC_U32_ADDR_ANY,
		.len = RVDEV_VRING_LEN,
		.flags = MEM_NONCACHEABLE,
		.reserved = 0,	/* must be zero */
		.name = "dsp_and_rv_vdevvring0",
	},
	/* it will reserve a memroy entry ot the same name in dts */
	.dsp_and_rv_vring1 = {
		.type = RSC_CARVEOUT,
		.da = FW_RSC_U32_ADDR_ANY,
		.pa = FW_RSC_U32_ADDR_ANY,
		.len = RVDEV_VRING_LEN,
		.flags = MEM_NONCACHEABLE,
		.reserved = 0,	/* must be zero */
		.name = "dsp_and_rv_vdevvring1",
	},
#endif
#ifdef MULTI_OPENAMP_SLAVE_MODE
	/* it will reserve a memroy entry ot the same name in dts */
	.dsp_and_rv_rsc_table = {
		.type = RSC_CARVEOUT,
		.da = FW_RSC_U32_ADDR_ANY,
		.pa = FW_RSC_U32_ADDR_ANY,
		.len = sizeof(struct dsp_and_rv_resource_table),
		.flags = MEM_NONCACHEABLE,
		.reserved = 0,	/* must be zero */
		.name = "dsp_and_rv_rsc_table",
	},
#endif /* MULTI_OPENAMP_SLAVE_MODE */
#ifdef CONFIG_AMP_SHARE_IRQ
	.share_irq = {
		.type = RSC_CARVEOUT,
		.da = FW_RSC_ADDR_ANY,
		.pa = FW_RSC_ADDR_ANY,
		.len = 0x1000,
		.flags = MEM_NONCACHEABLE,
		.reserved = 0,	/* must be zero */
		.name = "share_irq_table",
	},
#endif
};

#ifdef MULTI_OPENAMP_SLAVE_MODE
static struct dsp_and_rv_resource_table dsp_and_rv_rsc_table = {
	.version = 1,
	.num = 2,
	.reserved = {0, 0},
	.offset = {
		offsetof(struct dsp_and_rv_resource_table, rvdev_shm),
		offsetof(struct dsp_and_rv_resource_table, vdev),
	},
	.rvdev_shm = {
		.type = RSC_CARVEOUT,
		.da = SHMEM_DA,
		.pa = SHMEM_PA,
		.len = RVDEV_SHM_LEN,
		.flags = MEM_CACHEABLE,
		.reserved = 0,
		.name = "vdev1buffer", /* should be the "vdev%dbuffer" format */
	},

	.vdev = {
		.type = RSC_VDEV,
		.id = VIRTIO_ID_RPMSG,
		.notifyid = 0,
		.dfeatures = RPMSG_IPU_C0_FEATURES,
		.gfeatures = RPMSG_IPU_C0_FEATURES,
		.config_len = 0,
		.status = 0,
		.num_of_vrings = NUM_VRINGS,
		.reserved = {0, 0},
	},

	.vring0 = {
		.da = VRING_TX_ADDRESS,
		.align = VRING_ALIGN,
		.num = VRING_NUM_BUFFS,
		.notifyid = VRING0_ID,
		.reserved = 0,
	},

	.vring1 = {
		.da = VRING_RX_ADDRESS,
		.align = VRING_ALIGN,
		.num = VRING_NUM_BUFFS,
		.notifyid = VRING1_ID,
		.reserved = 0,
	},
};
#endif /* MULTI_OPENAMP_SLAVE_MODE */


int resource_table_init(int rsc_id, void **table_ptr, int *length)
{
	int ret = 0;
	struct dsp_and_rv_resource_table *table_va;
	metal_phys_addr_t table_lpa, table_lva;

	/* avoid compile warn */
	(void)table_va;
	(void)table_lpa;
	(void)table_lva;

	switch (rsc_id) {
	case RESOURCE_CPU_ID:
		*length = sizeof(resource_table);
		*table_ptr = (void *)&resource_table;
		break;

	case RESOURCE_ANOTHER_ID:
#ifdef MULTI_OPENAMP_MASTER_MODE
		table_lpa = resource_table.dsp_and_rv_rsc_table.pa;

		ret = mem_pa_to_va(table_lpa, &table_lva, NULL);
		if (ret < 0) {
			openamp_err("Failed to translate pa 0x%lx to va\n", table_lpa);
			goto out;
		}
		table_va  = (void *)table_lva;

		openamp_dbg("map share resource_table pa: 0x%lx -> va: 0x%lx\n",
				(unsigned long)table_lpa, (unsigned long)table_lva);
		/*
		 *	step1:
		 *		wait for the slave copy its own resource_table to the shared resource_table.
		 *		NOTE: we need to make sure the data is 0 every time rvdev_shm starts.
		 */
		openamp_dbg("share rvdev_shm da: 0x%x, pa: 0x%x; vring0.da: "
				"0x%x; vring1.da: 0x%x\n",
				table_va->rvdev_shm.da,
				table_va->rvdev_shm.pa,
				table_va->vring0.da,
				table_va->vring1.da);

		openamp_dbg("Waiting SLAVE to initialize resource table\n");
		hal_dcache_invalidate((unsigned long)table_va, sizeof(*table_va));
		while (table_va->rvdev_shm.da != SHMEM_DA || table_va->reserved[0] != 1) {
			metal_cpu_yield();
			hal_dcache_invalidate((unsigned long)table_va, sizeof(*table_va));
		}

		hal_dcache_invalidate((unsigned long)table_va, sizeof(*table_va));
		hal_msleep(1);
		table_va->rvdev_shm.da = resource_table.dsp_and_rv_rvdev_shm.da;
		table_va->rvdev_shm.pa = resource_table.dsp_and_rv_rvdev_shm.pa;
		table_va->vring0.da = resource_table.dsp_and_rv_vring0.pa;
		table_va->vring1.da = resource_table.dsp_and_rv_vring1.pa;
		table_va->reserved[0] = 2;
		hal_dcache_clean((unsigned long)table_va, sizeof(*table_va));

		openamp_dbg("update share rvdev_shm da: 0x%x, pa: 0x%x; vring0.da: "
				"0x%x; vring1.da: 0x%x\n",
				table_va->rvdev_shm.da,
				table_va->rvdev_shm.pa,
				table_va->vring0.da,
				table_va->vring1.da);

		*length = sizeof(*table_va);
		*table_ptr = table_va;
#endif
#ifdef MULTI_OPENAMP_SLAVE_MODE
		table_lpa = resource_table.dsp_and_rv_rsc_table.pa;

		ret = mem_pa_to_va(table_lpa, &table_lva, NULL);
		if (ret < 0) {
			openamp_err("Failed to translate pa 0x%lx to va\n", table_lpa);
			goto out;
		}
		table_va  = (void *)table_lva;

		openamp_dbg("map share resource_table pa: 0x%lx -> va: 0x%lx\n",
				(unsigned long)table_lpa, (unsigned long)table_lva);

		/*
		 *	Setp2:
		 *		Copy the dsp_and_rv_rsc_table information to the carveout space
		 */
		memcpy(table_va, &dsp_and_rv_rsc_table, sizeof(dsp_and_rv_rsc_table));
		table_va->reserved[0] = 1;
		hal_dcache_clean((unsigned long)table_va, sizeof(*table_va));

		/*
		 *	Setp3:
		 *		 Wait until MASTER updated the resource table
		 */
		openamp_dbg("new rvdev_shm da: 0x%x, pa: 0x%x; vring0.da: 0x%x; vring1.da: 0x%x\n",
				table_va->rvdev_shm.da,
				table_va->rvdev_shm.pa,
				table_va->vring0.da,
				table_va->vring1.da);

		openamp_dbg("Waiting MASTER to update resource table\n");
		hal_dcache_invalidate((unsigned long)table_va, sizeof(*table_va));
		while (table_va->rvdev_shm.da == SHMEM_DA || table_va->reserved[0] != 2) {
			metal_cpu_yield();
			hal_dcache_invalidate((unsigned long)table_va, sizeof(*table_va));
		}

		hal_msleep(1);

		openamp_dbg("new rvdev_shm da: 0x%x, pa: 0x%x; vring0.da: 0x%x; vring1.da: 0x%x\n",
				table_va->rvdev_shm.da,
				table_va->rvdev_shm.pa,
				table_va->vring0.da,
				table_va->vring1.da);

		table_va->reserved[0] = 0;
		*length = sizeof(*table_va);
		*table_ptr = table_va;
#endif
		break;
	default:
		openamp_err("Invalid rsc_id: %d\n", rsc_id);
		ret = -EINVAL;
		goto out;
	}

out:
	return ret;
}

struct fw_rsc_carveout *resource_table_get_rvdev_shm_entry(void *rsc_table, int rvdev_id)
{
	struct dsp_and_rv_resource_table *table_va;
	metal_phys_addr_t table_lpa, table_lva;

	/* avoid compile warn */
	(void)table_va;
	(void)table_lpa;
	(void)table_lva;

	switch(rvdev_id) {
	case RESOURCE_CPU_ID:
		return &resource_table.rvdev_shm;
		break;
	case RESOURCE_ANOTHER_ID:
#ifdef CONFIG_MULTI_OPENAMP_SUPPORT
		table_lpa = resource_table.dsp_and_rv_rsc_table.pa;

		if (mem_pa_to_va(table_lpa, &table_lva, NULL) < 0) {
			openamp_err("Failed to translate pa 0x%lx to va\n", table_lpa);
			return NULL;
		}
		table_va  = (void *)table_lva;

		return &table_va->rvdev_shm;
#endif
		break;
	default:
		openamp_err("Invalid rvdev_id: %d\n", rvdev_id);
		return NULL;
	}

	return NULL;
}

unsigned int resource_table_vdev_notifyid(void)
{
	return resource_table.vdev.notifyid;
}

struct fw_rsc_vdev *resource_table_get_vdev(int index)
{
	return &resource_table.vdev;
}

#ifdef CONFIG_AMP_SHARE_IRQ
struct fw_rsc_carveout *resource_table_get_share_irq_entry(void *rsc_table, int rvdev_id)
{
	return &resource_table.share_irq;
}
#endif
