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
#include "platform_sun300iw1.h"
#include "platform_irq.h"
#include "../irq_core.h"
#include <hal/sunxi_hal_common.h>
#include <hal_interrupt.h>
#include <hal_cache.h>
#ifdef CONFIG_PM_STANDBY_MEMORY
#include "pm_mem.h"
#endif

#define REMOTEPROC_E907_READY_TO_STOP	(0x16aa0001)
#define REMOTEPROC_E907_STOP_REC_REG	(0x4a00020c)

#define SUN300IW1_E907_TS_CLK_REG (0x4200100C)

//Not currently supported, irq_major_string & irq_group_string  will be updated later
#ifdef CONFIG_PM_STANDBY_MEMORY
__standby_unsaved_data const char *irq_major_string[PLAT_CLIC_IRQ_CNT] =
#else
const char *irq_major_string[PLAT_CLIC_IRQ_CNT] =
#endif
{
NULL
};

#ifdef PLAT_HAS_NON_ROOT_IC
#ifdef CONFIG_PM_STANDBY_MEMORY
__standby_unsaved_data const char *irq_group_string[PLAT_NON_ROOT_IRQ_CONTROLLER_CNT * PLAT_NON_ROOT_IC_IRQ_CNT] =
#else
const char *irq_group_string[PLAT_NON_ROOT_IRQ_CONTROLLER_CNT * PLAT_NON_ROOT_IC_IRQ_CNT] =
#endif
{
NULL
};
#endif

int plat_arch_timer_clock_init(void)
{
	/* config ts clock's gating and source (freq=40Mhz)*/
	hal_writel(0x80000000, SUN300IW1_E907_TS_CLK_REG);

	return 0;
}

void platform_remoteproc_stop_eve(void)
{
	hal_interrupt_disable();
	irq_core_disable_root_ic_irq();
	hal_writel(REMOTEPROC_E907_READY_TO_STOP, REMOTEPROC_E907_STOP_REC_REG);
	while (1) {
		__asm__ volatile("wfi":::"memory", "cc");
	}
}

#define SPIF_IRQ_DELEG_RTC_IDX	(0)
#define SPIF_IRQ_DELEG_RTC_BIT	(3)
#define SPIF_IRQ_READY_RTC_BIT	(4)
#define SPIF_IRQ_RECORD_RTC_BIT	(4)

#define SPIF_IRQ				MAKE_IRQn(35, 0)
#define RTC_DATA_REG(N)			(0x4A000000L + 0x200 + 4 * (N))

#define SUNXI_SPI_FLASH_BASE	(0x44F00000L)

#define SPIF_INT_EN_REG			(SUNXI_SPI_FLASH_BASE + 0x14)  /* interrupt enable register */
#define SPIF_RF_RDY_INT_EN		(0x1 << 0)  /* read fifo is ready */
#define SPIF_RF_ENPTY_INT_EN	(0x1 << 1)
#define SPIF_RF_FULL_INT_EN		(0x1 << 2)
#define SPIF_WF_RDY_INT_EN		(0x1 << 4)  /* write fifo is ready */
#define SPIF_WF_ENPTY_INT_EN	(0x1 << 5)
#define SPIF_WF_FULL_INT_EN		(0x1 << 6)
#define SPIF_RF_OVF_INT_EN		(0x1 << 8)
#define SPIF_RF_UDF_INT_EN		(0x1 << 9)
#define SPIF_WF_OVF_INT_EN		(0x1 << 10)
#define SPIF_WF_UDF_INT_EN		(0x1 << 11)
#define SPIF_DMA_TRANS_DONE_EN	(0x1 << 24)
#define SPIF_PREFETCH_READ_EN	(0x1 << 25)
#define SPIF_INT_STA_READY_EN	(SPIF_RF_RDY_INT_EN | SPIF_WF_RDY_INT_EN)
#define SPIF_INT_STA_TC_EN		(SPIF_DMA_TRANS_DONE_EN | SPIF_PREFETCH_READ_EN)
#define SPIF_INT_STA_ERR_EN		(SPIF_RF_OVF_INT_EN | SPIF_RF_UDF_INT_EN | SPIF_WF_OVF_INT_EN)

#define SPIF_INT_STA_REG		(SUNXI_SPI_FLASH_BASE + 0x18)  /* interrupt status register */
#define SPIF_RF_RDY				(1 << 0)
#define SPIF_RF_EMPTY			(1 << 1)
#define SPIF_RF_FULL			(1 << 2)
#define SPIF_WF_RDY				(1 << 4)
#define SPIF_WF_EMPTY			(1 << 5)
#define SPIF_WF_FULL			(1 << 6)
#define SPIF_RF_OVF				(1 << 8)
#define SPIF_RF_UDF				(1 << 9)
#define SPIF_WF_OVF				(1 << 10)
#define SPIF_WF_UDF				(1 << 11)
#define SPIF_DMA_TRANS_DONE		(1 << 24)
#define SPIF_PREFETCH_READ_BACK	(1 << 25)
#define SPIF_INT_STA_READY		(SPIF_RF_READY | SPIF_WF_READY)
#define SPIF_INT_STA_TC			(SPIF_DMA_TRANS_DONE | SPIF_PREFETCH_READ_BACK)
#define SPIF_INT_STA_ERR		(SPIF_RF_OVF | SPIF_RF_UDF | SPIF_WF_OVF)

#define SPIF_DMA_CTL_REG		(SUNXI_SPI_FLASH_BASE + 0x40)  /* DMA control register */
#define DMA_DATA_LEN			(0x1ffff)
#define SPIF_MAX_TRANS_NUM		(65536)
#define SPIF_DATA_NUM_POS		(0)

#define SPIF_DSC_REG			(SUNXI_SPI_FLASH_BASE + 0x44)  /* DMA descriptor start address register */
#define CFG_DMA_START			(1 << 0)

struct spif_descriptor_op {
	uint32_t hburst_rw_flag;
	uint32_t block_data_len;
	uint32_t data_addr;
	uint32_t next_des_addr;
	uint32_t trans_phase;//0x20
	uint32_t flash_addr;//0x24
	uint32_t cmd_mode_buswidth;//0x28
	uint32_t addr_dummy_data_count;//0x2c
} __packed;

static hal_irqreturn_t hal_spif_handler(void *data)
{
	uint32_t irq_sta, enable, val;
	struct spif_descriptor_op *spif_op;

	(void)data;

	enable = hal_readl(SPIF_INT_EN_REG);
	irq_sta = hal_readl(SPIF_INT_STA_REG);

	if ((enable & SPIF_DMA_TRANS_DONE_EN) && (irq_sta & SPIF_INT_STA_TC)) {
		spif_op = (typeof(spif_op))(readl(SPIF_DSC_REG) << 2);
		hal_dcache_invalidate((unsigned long)spif_op, sizeof(*spif_op));

		val = hal_readl(RTC_DATA_REG(SPIF_IRQ_RECORD_RTC_BIT));
		val += ((spif_op->block_data_len & DMA_DATA_LEN) >> SPIF_DATA_NUM_POS);

		if (spif_op->next_des_addr == 0) {
			val |= (1 << 30);
			/* no need to deleg spif irq */
			if (!(hal_readl(RTC_DATA_REG(SPIF_IRQ_DELEG_RTC_IDX)) & (1 << SPIF_IRQ_DELEG_RTC_BIT))) {
#ifdef CONFIG_WAIT_SPIF_CONTROLLER
				hal_writel(hal_readl(RTC_DATA_REG(CONFIG_SPIF_WAIT_INDEX)) | (1 << CONFIG_SPIF_WAIT_BIT),
								RTC_DATA_REG(CONFIG_SPIF_WAIT_INDEX));
#endif
				hal_disable_irq(SPIF_IRQ);
				hal_free_irq(SPIF_IRQ);
			}
		} else {
			/* set next descriptor address */
			writel(spif_op->next_des_addr, SPIF_DSC_REG);
			/* start spif dma */
			hal_writel(hal_readl(SPIF_DMA_CTL_REG) | CFG_DMA_START, SPIF_DMA_CTL_REG);
		}
		hal_writel(val, RTC_DATA_REG(SPIF_IRQ_RECORD_RTC_BIT));
	} else if ((enable & SPIF_INT_STA_ERR_EN) &&  (irq_sta & SPIF_INT_STA_ERR)) {
		hal_writel(irq_sta | (1 << 31), RTC_DATA_REG(SPIF_IRQ_RECORD_RTC_BIT));
	}

	/* clear pending */
	writel(irq_sta, SPIF_INT_STA_REG);
	hal_interrupt_clear_pending(SPIF_IRQ);

	return HAL_IRQ_OK;
}

void platform_fastboot_init_early(void)
{
	u32 val;
	if (hal_readl(RTC_DATA_REG(SPIF_IRQ_DELEG_RTC_IDX)) & (1 << SPIF_IRQ_READY_RTC_BIT)) {
		val = hal_readl(RTC_DATA_REG(SPIF_IRQ_DELEG_RTC_IDX));
		val |= (1 << SPIF_IRQ_DELEG_RTC_BIT);
		hal_writel(val, RTC_DATA_REG(SPIF_IRQ_DELEG_RTC_IDX));
	}

	if (hal_readl(RTC_DATA_REG(SPIF_IRQ_DELEG_RTC_IDX)) & (1 << SPIF_IRQ_DELEG_RTC_BIT)) {
		/* deleg the spif irq */
		hal_request_irq(SPIF_IRQ, hal_spif_handler, "spif", NULL);
		hal_enable_irq(SPIF_IRQ);
		val = hal_readl(RTC_DATA_REG(SPIF_IRQ_DELEG_RTC_IDX));
		val |= (1 << SPIF_IRQ_READY_RTC_BIT);
		hal_writel(val, RTC_DATA_REG(SPIF_IRQ_DELEG_RTC_IDX));
	}
}

void arch_plat_system_init_early(void)
{
	platform_fastboot_init_early();
}
