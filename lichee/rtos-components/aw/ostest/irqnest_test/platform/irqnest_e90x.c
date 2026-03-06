/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
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
/*
* Copyright (c) 2024-2030 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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

#include <string.h>
#include <sunxi_hal_common.h>
#include <hal_interrupt.h>
#include <os_test_log.h>

#include <../e90x/platform/platform.h>
#include <../e90x/irq_core.h>
#include <../e90x/clic.h>

#include "../irqnest_test.h"

int irqnest_platform_get_irq_num(int hw_irqid)
{
	return MAKE_IRQn(hw_irqid, 0);
}

int irqnest_platform_trigger_irq(int hw_irqid)
{
	hal_writeb(1, PLAT_CLIC_BASE_ADDR + CLIC_INT_X_IP_REG_OFF(hw_irqid));
	dsb();

	return 0;
}

int irqnest_platform_irq_request_config(int hw_irqid)
{
	int ret;
	int irq_num = MAKE_IRQn(hw_irqid, 0);

	/* make CLICINTIP reg rw, and pending clean automatically */
	ret = irq_core_set_irq_trigger_type(irq_num, IRQ_TRIGGER_TYPE_EDGE_RISING);
	if (ret)
		os_test_err("set irq %d trigger type failed, ret: %d", irq_num, ret);

	return ret;
}

#define IRQNEST_CONTROL_REG_CNT	(5)
#define CLIC_MINTTHRESH_REG 	(NEST_PLAT_ROOT_IC_REG_BASE_ADDR + CLIC_MINTTHRESH_REG_OFF)
static uint32_t irqnest_extra_tmp[IRQNEST_CONTROL_REG_CNT] = {0};
static uint32_t irqnest_extra_ctx_entry[IRQNEST_CONTROL_REG_CNT * TEST_NUM_MAX] = {0};
static uint32_t irqnest_extra_ctx_exit[IRQNEST_CONTROL_REG_CNT * TEST_NUM_MAX] = {0};
void irqnest_platform_ctx_extra_check_entry(int hw_irqid, uint32_t nest, uint32_t seq_num)
{
	__asm__ volatile (
	"la t0, irqnest_extra_tmp\n"
	"csrr t1, mepc\n"
	"sw t1, 0 * 4( t0 )\n"
	"csrr t1, mscratch\n"
	"sw t1, 1 * 4( t0 )\n"
	"csrr t1, mstatus\n"
	"sw t1, 2 * 4( t0 )\n"
	"csrr t1, mcause\n"
	"sw t1, 3 * 4( t0 )\n"
	: : : "t0", "t1", "memory");

	irqnest_extra_tmp[IRQNEST_CONTROL_REG_CNT - 1] = hal_readl(CLIC_MINTTHRESH_REG);
	dsb();

	memcpy(&irqnest_extra_ctx_entry[IRQNEST_CONTROL_REG_CNT * seq_num], irqnest_extra_tmp, IRQNEST_CONTROL_REG_CNT * sizeof(uint32_t));

#ifdef CONFIG_IRQNEST_TEST_CTX_CONTENT_DEBUG
	int i;
	irqnest_test_dbg("irqid( %d ) extra ENTRY ctx content:", hw_irqid);
	for (i = 0; i < IRQNEST_CONTROL_REG_CNT; i++)
		irqnest_test_dbg("0x%x", irqnest_extra_ctx_entry[IRQNEST_CONTROL_REG_CNT * seq_num + i]);
#endif
}

int irqnest_platform_ctx_extra_check_exit(int hw_irqid, uint32_t nest, uint32_t seq_num)
{
	int i;

	__asm__ volatile (
	"la t0, irqnest_extra_tmp\n"
	"csrr t1, mepc\n"
	"sw t1, 0 * 4( t0 )\n"
	"csrr t1, mscratch\n"
	"sw t1, 1 * 4( t0 )\n"
	"csrr t1, mstatus\n"
	"sw t1, 2 * 4( t0 )\n"
	"csrr t1, mcause\n"
	"sw t1, 3 * 4( t0 )\n"
	: : : "t0", "t1", "memory");

	irqnest_extra_tmp[IRQNEST_CONTROL_REG_CNT - 1] = hal_readl(CLIC_MINTTHRESH_REG);
	dsb();

	memcpy(&irqnest_extra_ctx_exit[IRQNEST_CONTROL_REG_CNT * seq_num], irqnest_extra_tmp, IRQNEST_CONTROL_REG_CNT * sizeof(uint32_t));

#ifdef CONFIG_IRQNEST_TEST_CTX_CONTENT_DEBUG
	irqnest_test_dbg("irqid( %d ) extra EXIT ctx content:", hw_irqid);
	for (i = 0; i < IRQNEST_CONTROL_REG_CNT; i++)
		irqnest_test_dbg("0x%x", irqnest_extra_ctx_exit[IRQNEST_CONTROL_REG_CNT * seq_num + i]);
#endif

	/* compare control regs data */
	if (memcmp(&irqnest_extra_ctx_entry[IRQNEST_CONTROL_REG_CNT * seq_num], &irqnest_extra_ctx_exit[IRQNEST_CONTROL_REG_CNT * seq_num], IRQNEST_CONTROL_REG_CNT)) {
		os_test_err("control regs restore data error, current irqid( %d ), nest( %d )", hw_irqid, nest);
		os_test_err("--- irqid( %d ) ENTRY extra data:", hw_irqid);
		for (i = 0; i < IRQNEST_CONTROL_REG_CNT; i++)
			os_test_err("0x%x", irqnest_extra_ctx_entry[IRQNEST_CONTROL_REG_CNT * seq_num + i]);
		os_test_err("--- irqid( %d ) EXIT extra data:", hw_irqid);
		for (i = 0; i < IRQNEST_CONTROL_REG_CNT; i++)
			os_test_err("0x%x", irqnest_extra_ctx_exit[IRQNEST_CONTROL_REG_CNT * seq_num + i]);

		return -1;
	}

	return 0;
}
