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

#include "clk_core.h"
#include "ccu_reset.h"

int ccu_rst_hw_set_ctrl_bit(reset_hw_t *rst_hw, int is_set)
{
	int ret;
	uint32_t reg_addr, reg_data, bit_mask, flags;
	clk_controller_t *controller;

	if (!rst_hw)
		return CLK_RET_INVALID_RST_HW;

	clk_dbg("set reset hw ctrl bit, rc_id: %u, reg_offset: 0x%x, bit_offset: %u, is_set: %d",
			rst_hw->rc_id, rst_hw->reg_offset, rst_hw->bit_offset, is_set);

	ret = rst_hw_get_controller(rst_hw, &controller);
	if (ret)
		return ret;

	reg_addr = controller->reg_base + rst_hw->reg_offset;
	bit_mask = 1 << (uint32_t)rst_hw->bit_offset;

	flags = hal_spin_lock_irqsave(&controller->lock);

	reg_data = readl(reg_addr);

	if (is_set)
		reg_data |= bit_mask;
	else
		reg_data &= ~bit_mask;

	writel(reg_data, reg_addr);

	hal_spin_unlock_irqrestore(&controller->lock, flags);

	return 0;
}

int ccu_rst_hw_get_ctrl_bit(reset_hw_t *rst_hw, int *is_set)
{
	int ret;
	uint32_t reg_addr, reg_data, bit_mask;
	clk_controller_t *controller;

	if (!rst_hw)
		return CLK_RET_INVALID_RST_HW;

	clk_dbg("get reset hw ctrl bit, rc_id: %u, reg_offset: 0x%x, bit_offset: %u, is_set: %d",
			rst_hw->rc_id, rst_hw->reg_offset, rst_hw->bit_offset, *is_set);

	ret = rst_hw_get_controller(rst_hw, &controller);
	if (ret)
		return ret;

	reg_addr = controller->reg_base + rst_hw->reg_offset;
	bit_mask = 1 << (uint32_t)rst_hw->bit_offset;

	reg_data = readl(reg_addr);

	if (reg_data & bit_mask)
		*is_set = 0;
	else
		*is_set = 1;

	return 0;
}
