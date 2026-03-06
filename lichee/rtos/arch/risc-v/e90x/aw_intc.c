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

#include "irq_core.h"
#include "aw_intc.h"

static inline uint8_t aw_intc_readb(uint32_t reg_addr)
{
	uint32_t aligned_addr, reg_data, bit_offset;

	aligned_addr = reg_addr & (~0x3);
	reg_data = ic_readl(aligned_addr);

	bit_offset = (reg_addr & 0x3) << 3;

	return (reg_data >> bit_offset) & 0xFF;
}

static inline void aw_intc_writeb(uint8_t data, uint32_t reg_addr)
{
	uint32_t aligned_addr, reg_data, bit_offset, bit_mask;

	aligned_addr = reg_addr & (~0x3);
	reg_data = ic_readl(aligned_addr);

	bit_offset = (reg_addr & 0x3) << 3;
	bit_mask = 0xFF << bit_offset;

	reg_data &= ~bit_mask;
	reg_data |= (uint32_t)data << bit_offset;

	ic_writel(reg_data, aligned_addr);
}

static inline int aw_intc_irq_ctrl(uint32_t reg_addr, uint32_t irq_bit, int enable)
{
	uint8_t reg_data = ic_readb(reg_addr);

	if (enable)
		reg_data |= (1 << irq_bit);
	else
		reg_data &= ~(1 << irq_bit);

	aw_intc_writeb(reg_data, reg_addr);
	return 0;
}

int aw_intc_init(const struct irq_controller *ic)
{
	//disable all interrupt
	aw_intc_writeb(0, ic->reg_base_addr);
	return 0;
}

int aw_intc_irq_enable(const irq_controller_t *ic, uint32_t irq_id)
{
	return aw_intc_irq_ctrl(ic->reg_base_addr, irq_id, 1);
}

int aw_intc_irq_disable(const struct irq_controller *ic, uint32_t irq_id)
{
	return aw_intc_irq_ctrl(ic->reg_base_addr, irq_id, 0);
}

int aw_intc_irq_is_enabled(const struct irq_controller *ic, uint32_t irq_id)
{
	if (aw_intc_readb(ic->reg_base_addr) & (1 << irq_id))
		return 1;

	return 0;
}

irq_controller_ops_t g_aw_intc_ops =
{
	.init = aw_intc_init,
	.irq_enable = aw_intc_irq_enable,
	.irq_disable = aw_intc_irq_disable,
	.irq_is_enabled = aw_intc_irq_is_enabled,
	.irq_is_pending = NULL,
	.irq_set_pending = NULL,
	.irq_set_trigger_type = NULL,
};

#if defined(CONFIG_STANDBY) || defined(CONFIG_COMPONENTS_PM)
#include "pm_syscore.h"

static int aw_intc_suspend(void *data, suspend_mode_t mode)
{
	irq_controller_t *ic;
	uint32_t reg_data;
	uint32_t *irq_status;

	ic = (irq_controller_t *)data;
	irq_status = ic->priv;

	reg_data = aw_intc_readb(ic->reg_base_addr);
	*irq_status = reg_data;

	return 0;
}

static void aw_intc_resume(void *data, suspend_mode_t mode)
{
	irq_controller_t *ic;
	uint32_t *irq_status;

	ic = (irq_controller_t *)data;
	irq_status = ic->priv;

	aw_intc_writeb(*irq_status, ic->reg_base_addr);
}

int aw_intc_pm_init(void *obj)
{
	int ret;
	irq_controller_t *ic;

	if (!obj)
		return -1;

	ic = (irq_controller_t *)obj;
	ic->pm_ops.name = "aw_intc_pm";
	ic->pm_ops.suspend = aw_intc_suspend;
	ic->pm_ops.resume = aw_intc_resume;
	ic->pm_ops.data = obj;

	ret = pm_syscore_register(&ic->pm_ops);
	if (ret)
		printf("Warning: pm_devops_register for aw intc failed, ret: %d", ret);

	return ret;
}
#endif