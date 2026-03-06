/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <openamp/open_amp.h>
#include <openamp/sunxi_helper/openamp_log.h>
#include <openamp/sunxi_helper/shmem_ops.h>
#include <openamp/sunxi_helper/msgbox_ipi.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/sunxi_rproc.h>
#include <openamp/sunxi_helper/remoteproc_common.h>
#ifdef CONFIG_ARCH_RISCV_PMP
#include <pmp.h>
#endif

#include <hal_msgbox.h>
#include <backtrace.h>

void vring_ipi_common_callback(void *priv, uint32_t data)
{
	int ret;
	struct vring_ipi *ipi = priv;

	openamp_dbg("name: %s, rx data: %u\r\n", ipi->info.name, data);
	/* tell rproc recv a message */
	ret = remoteproc_get_notification(ipi->rproc, data);
	if (ret < 0)
		openamp_err("remoteproc_get_notification for %s (id: %u) failed\n",
				ipi->info.name, data);
}

struct remoteproc* rproc_common_init(struct remoteproc *rproc,
				   struct remoteproc_ops *ops, void *arg)
{
	struct rproc_common_private *rproc_priv = arg;
	struct vring_ipi *ipi = &rproc_priv->vring_ipi;

	if (!rproc || !ops || !arg) {
		openamp_err("Invalid argumemts\r\n");
		goto out;
	}

	ipi->rproc = rproc;
	ipi->ipi = msgbox_ipi_create(&ipi->info, vring_ipi_common_callback, ipi);
	if(!ipi->ipi) {
		openamp_err("Failed to create ving_ipi\r\n");
		goto out;
	}

	metal_list_init(&rproc_priv->mem_list);
	metal_list_init(&rproc_priv->io_list);

	rproc->priv = rproc_priv;
	rproc->ops = ops;

	openamp_dbg("init rproc...\r\n");

	return rproc;

out:
	return NULL;
}

void rproc_common_remove(struct remoteproc *rproc)
{
	struct rproc_common_private *rproc_priv = rproc->priv;
	struct rproc_mem_list_entry *mem_entry;
	struct rproc_io_list_entry *io_entry;
	struct metal_list *node_current;
	struct metal_list *node_next;

	openamp_info("remove rproc...\r\n");

	msgbox_ipi_release(rproc_priv->vring_ipi.ipi);

	/*
	 * To safely delete all the nodes in list and free their memory,
	 * we operate the list directly rather than use the helpers
	 * metal_list_for_each() or metal_list_del().
	 */
	node_current = rproc_priv->mem_list.next;
	while (node_current != &rproc_priv->mem_list) {
		/*
		 * Delete the current node in list first, and save its next node.
		 * Because the memory of current node will be free later.
		 */
		node_current->next->prev = node_current->prev;
		node_current->prev->next = node_current->next;
		node_next = node_current->next;

		mem_entry = metal_container_of(node_current,
				struct rproc_mem_list_entry, node);
		metal_free_memory(mem_entry);

		node_current = node_next;
	}

	/*
	 * Similar to the previous operations of mem_list.
	 */
	node_current = rproc_priv->io_list.next;
	while (node_current != &rproc_priv->io_list) {
		node_current->next->prev = node_current->prev;
		node_current->prev->next = node_current->next;
		node_next = node_current->next;

		io_entry = metal_container_of(node_current,
				struct rproc_io_list_entry, node);
		metal_free_memory(io_entry);

		node_current = node_next;
	}
}

void *rproc_common_mmap(struct remoteproc *rproc,
		      metal_phys_addr_t *pa, metal_phys_addr_t *da,
		      size_t size, unsigned int attribute,
		      struct metal_io_region **io)
{
	struct rproc_common_private *rproc_priv = rproc->priv;
	metal_phys_addr_t lpa = METAL_BAD_PHYS;
	metal_phys_addr_t lda = METAL_BAD_PHYS;
	metal_phys_addr_t lva;
	struct rproc_mem_list_entry *mem_entry;
	struct rproc_io_list_entry *io_entry;
	int ret;

	lpa = *pa;
	lda = *da;

	openamp_info("remoteproc mmap pa: 0x%x, da: 0x%x, size = 0x%x\n",
			(uint32_t)(lpa), (uint32_t)(lda), (uint32_t)size);

	/*
	 * In this remoteproc implementation:
	 *   for DSP:
	 *       VA means physical address seen on the DSP side(same as DA).
	 *       PA means physical address seen on the CPU side.
	 *       DA means physical address seen on the DSP side(same as VA).
	 *       VA = DA != PA
	 *   for RV:
	 *       VA means device address seen on the RV side(same as DA).
	 *       PA means physical address seen on the CPU side.
	 *       DA means device address seen on the RV side(sam as PA, if no iommu).
	 *       VA = PA = DA
	 */
	if (lpa == METAL_BAD_PHYS && lda == METAL_BAD_PHYS) {
		openamp_err("Invalid pa and da\r\n");
		goto out;
	} else if (lpa == METAL_BAD_PHYS) {
		ret = mem_va_to_pa(lda, &lpa, NULL);
		if(ret != 0) {
			openamp_err("Failed to translate da 0x%lx to va\r\n", (unsigned long)lda);
			goto out;
		}
		openamp_dbg("Translate da 0x%lx to pa 0x%lx\n", (unsigned long)lda, (unsigned long)lpa);
	}else if (lda == METAL_BAD_PHYS) {
		ret = mem_pa_to_va(lpa, &lda, NULL);
		if (ret != 0) {
			openamp_err("Failed to translate pa 0x%lx to da\r\n", (unsigned long)lpa);
			goto out;
		}
		openamp_dbg("Translate pa 0x%lx to da 0x%lx\n", (unsigned long)lpa, (unsigned long)lda);
	}

	mem_entry = metal_allocate_memory(sizeof(*mem_entry));
	if (!mem_entry) {
		openamp_err("Failed to allocate memory for remoteproc mem\r\n");
		goto out;
	}

	io_entry = metal_allocate_memory(sizeof(*io_entry));
	if (!io_entry) {
		openamp_err("Failed to allocate memory for remoteproc io\r\n");
		goto free_mem;
	}

	openamp_dbg("add rproc_mem lpa: 0x%lx, lda: 0x%lx\n", lpa, lda);

	remoteproc_init_mem(&mem_entry->mem, NULL, lpa, lda, size, &io_entry->io);

	ret = mem_pa_to_va(lpa, &lva, NULL);
	if (ret < 0) {
		openamp_err("Failed to translate pa 0x%lx to va\n", lpa);
		goto free_io;
	}

	openamp_dbg("register metal io: va:0x%lx -> pa:0x%lx\n", lva, (unsigned long)mem_entry->mem.pa);

	metal_io_init(&io_entry->io, (void *)lva, &mem_entry->mem.pa, size, (uint32_t)(-1),
					attribute, &shmem_sunxi_io_ops);

	remoteproc_add_mem(rproc, &mem_entry->mem);

	*pa = lpa;
	*da = lda;
	if (io)
		*io = &(io_entry->io);

	metal_list_add_tail(&rproc_priv->mem_list, &mem_entry->node);
	metal_list_add_tail(&rproc_priv->io_list, &io_entry->node);

	openamp_info("map pa(0x%08lx) to va(0x%08lx)\r\n", (unsigned long)lpa, (unsigned long)lva);
#if defined(CONFIG_ARCH_RISCV_PMP) && !defined(CONFIG_PMP_EARLY_ENABLE)
	/*
	 * FIXME: to make the pmp region contiguous, we align the addr to 4K.
	 */
	pmp_add_region((unsigned long)lva, (unsigned long)lva +
					ALIGN_UP(size, 4096), PMP_R | PMP_W);
#endif

	openamp_dbg("\r\n");

	return metal_io_phys_to_virt(&(io_entry->io), mem_entry->mem.pa);
free_io:
	metal_free_memory(io_entry);
free_mem:
	metal_free_memory(mem_entry);
out:
	return NULL;
}

int rproc_common_notify(struct remoteproc *rproc, uint32_t id)
{
	struct rproc_common_private *rproc_priv = rproc->priv;
	struct vring_ipi *ipi = &rproc_priv->vring_ipi;
	int ret;

	openamp_dbg("%s id: %u\n", ipi->info.name, id);

	ret = msgbox_ipi_notify(ipi->ipi, id);
	if (ret < 0) {
		openamp_err("%s (id: %d) notify failed\n",
				ipi->info.name, id);
		goto out;
	}

	return 0;
out:
	return ret;
}

struct remoteproc_ops rproc_common_ops = {
	.init       = rproc_common_init,
	.remove     = rproc_common_remove,
	.mmap       = rproc_common_mmap,
	.handle_rsc = NULL,
	.config     = NULL,
	.start      = NULL,
	.stop       = NULL,
	.shutdown   = NULL,
	.notify     = rproc_common_notify,
	.get_mem    = NULL,
};
