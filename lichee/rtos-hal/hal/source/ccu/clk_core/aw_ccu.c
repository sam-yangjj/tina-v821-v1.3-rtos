/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 *the the People's Republic of China and other countries.
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

#include "aw_ccu.h"

#ifdef CONFIG_COMPONENTS_PM
#include <pm_syscore.h>
#include <pm_debug.h>
#endif

void dump_aw_ccu_clk_hw(const aw_ccu_clk_hw_t *hw)
{
	clk_info("----------------AW CCU Clock Hardware object info----------------");
	clk_info("Clock Hardware(%p):", hw);

	clk_info("reg_offset: %u", hw->reg_offset);
	clk_info("features: %u", hw->features);
	clk_info("hw: %p", &hw->hw);
}

static int check_ctrl_bit_offset(const clk_hw_t *hw, uint8_t ctrl_bit_offset)
{
	if (ctrl_bit_offset <= MAX_CONTROL_BIT_OFFSET)
	{
		return 1;
	}

	if (ctrl_bit_offset == CTRL_BIT_ABSENT_BIT_OFFSET)
	{
		clk_hw_dbg(hw, "control bit is absent!");
		return 0;
	}

	clk_hw_err(hw, "invalid control bit index %u", ctrl_bit_offset);
	return CLK_RET_INVALID_CTRL_BIT_OFFSET;
}

int ccu_clk_hw_set_control_bit(aw_ccu_clk_hw_t *ccu_clk_hw,
uint8_t ctrl_bit_offset, int is_set, int is_need_lock)
{
	int ret;
	unsigned long flags;
	uint32_t reg_data;
	clk_controller_t *controller;

	ret = check_ctrl_bit_offset(&ccu_clk_hw->hw, ctrl_bit_offset);
	if (ret <= 0)
	{
		return ret;
	}

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	if (is_need_lock)
		flags = hal_spin_lock_irqsave(&controller->lock);

	reg_data = readl(controller->reg_base + ccu_clk_hw->reg_offset);

	if (is_set)
		reg_data |= 1 << (uint32_t)ctrl_bit_offset;
	else
		reg_data &= ~(1 << (uint32_t)ctrl_bit_offset);

	writel(reg_data, controller->reg_base + ccu_clk_hw->reg_offset);

	if (is_need_lock)
		hal_spin_unlock_irqrestore(&controller->lock, flags);

	return 0;
}

int ccu_clk_hw_get_control_bit(aw_ccu_clk_hw_t *ccu_clk_hw,
uint8_t ctrl_bit_offset, int *is_set, int is_need_lock)
{
	int ret;
	uint32_t reg_data;
	clk_controller_t *controller;

	ret = check_ctrl_bit_offset(&ccu_clk_hw->hw, ctrl_bit_offset);
	if (ret <= 0)
	{
		return ret;
	}

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	/* input parameter 'is_need_lock' is reserved for possible future using, currently not need */

	reg_data = readl(controller->reg_base + ccu_clk_hw->reg_offset);

	if (reg_data & (1 << (uint32_t)ctrl_bit_offset))
		*is_set = 1;
	else
		*is_set = 0;

	return 0;
}

