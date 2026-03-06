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
#include "ccu_pll.h"

#define MAX_CHECK_PLL_STABLE_TIMES 200

static inline struct ccu_pll *hw_to_ccu_pll(struct clk_hw *hw)
{
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);

	return container_of(ccu_clk_hw, struct ccu_pll, ccu_clk_hw);
}

int ccu_pll_init(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll)
{
	int ret;

	ret = ccu_mult_init(ccu_clk_hw, &pll->mult);
	if (ret)
		return ret;

	return 0;
}

static int pll_wait_stable(uint32_t lock_ctrl_reg_addr, uint32_t lock_ctrl_bit_offset,
	uint32_t lock_state_reg_addr, uint32_t lock_state_bit_offset,
	uint32_t pll_stable_time)
{
	int wait_pll_stable_cnt;
	uint32_t reg_data;

	/* lock PLL */
	if (lock_ctrl_bit_offset != PLL_CTRL_BIT_ABSENT_BIT_OFFSET)
	{
		reg_data = readl(lock_ctrl_reg_addr);
		reg_data |= (1 << lock_ctrl_bit_offset);
		writel(reg_data, lock_ctrl_reg_addr);
	}

	if (pll_stable_time)
		clk_udelay(pll_stable_time);

	/* check whether PLL is stable */
	wait_pll_stable_cnt = 0;
	while (1)
	{
		wait_pll_stable_cnt++;

		reg_data = readl(lock_state_reg_addr);
		if (reg_data & (1 << lock_state_bit_offset))
			break;

		if (wait_pll_stable_cnt == MAX_CHECK_PLL_STABLE_TIMES)
		{
			return CLK_RET_PLL_WAIT_STABLE_TIMEOUT;
		}
	}

	return 0;
}

static int pll_ctrl_module_enable(uint32_t pll_ctrl_reg_addr,
	uint32_t enable_bit_offset, int is_enabled)
{
	uint32_t reg_data;
	reg_data = readl(pll_ctrl_reg_addr);
	if (is_enabled)
	{
		reg_data |= (1 << enable_bit_offset);
	}
	else
	{
		reg_data &= ~(1 << enable_bit_offset);
	}
	writel(reg_data, pll_ctrl_reg_addr);

	return 0;
}

static int pll_ctrl_output_gate(uint32_t pll_ctrl_reg_addr,
	uint32_t output_gate_bit_offset, int enabled)
{
	uint32_t reg_data;
	reg_data = readl(pll_ctrl_reg_addr);
	if (enabled)
	{
		reg_data |= (1 << output_gate_bit_offset);
	}
	else
	{
		reg_data &= ~(1 << output_gate_bit_offset);
	}
	writel(reg_data, pll_ctrl_reg_addr);

	return 0;
}

int ccu_pll_set_enable_state(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll, int is_enabled)
{
	int ret;
	clk_controller_t *controller;
	unsigned long flags;
	//uint32_t reg_addr, reg_data;
	uint32_t reg_base, pll_ctrl_reg_addr, pll_ctrl_reg_data, last_enable_state;

	uint32_t output_gate_bit_offset, enable_bit_offset;
	uint32_t lock_ctrl_reg_addr, lock_ctrl_bit_offset;
	uint32_t lock_state_reg_addr, lock_state_bit_offset;
	uint32_t pll_stable_time;

	uint8_t var_type;
	ccu_pll_base_type1_t *base_type1_pll;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	reg_base = controller->reg_base;
	pll_ctrl_reg_addr = reg_base + ccu_clk_hw->reg_offset;

	var_type = pll->var_type;

	if (var_type == CCU_PLL_VAR_TYPE0)
	{
		output_gate_bit_offset = pll->output_gate_bit_offset;

		pll_ctrl_reg_data = readl(pll_ctrl_reg_addr);
		last_enable_state = pll_ctrl_reg_data & (1 << output_gate_bit_offset);
	}
	else if (var_type == CCU_PLL_VAR_TYPE1)
	{
		base_type1_pll = (ccu_pll_base_type1_t *)pll;

		pll_stable_time = base_type1_pll->stable_time;

		enable_bit_offset = base_type1_pll->enable_bit_offset;
		lock_ctrl_reg_addr = pll_ctrl_reg_addr;
		lock_state_reg_addr = reg_base + base_type1_pll->lock_state_reg_offset;
		lock_ctrl_bit_offset = base_type1_pll->lock_ctrl_bit_offset;
		lock_state_bit_offset = base_type1_pll->lock_state_bit_offset;

		pll_ctrl_reg_data = readl(pll_ctrl_reg_addr);
		last_enable_state = pll_ctrl_reg_data & (1 << enable_bit_offset);
	}
	else
	{
		return CLK_RET_PLL_VAR_TYPE_NOT_SUPPORTED;
	}

	/* check whether we need to do next step */
	if (last_enable_state)
	{
		if (is_enabled)
			return 0;
	}
	else
	{
		if (!is_enabled)
			return 0;
	}

	flags = hal_spin_lock_irqsave(&controller->lock);

	if (var_type == CCU_PLL_VAR_TYPE0)
	{
		ret = pll_ctrl_output_gate(pll_ctrl_reg_addr, output_gate_bit_offset, is_enabled);
	}
	else if (var_type == CCU_PLL_VAR_TYPE1)
	{
		ret = pll_ctrl_module_enable(pll_ctrl_reg_addr, enable_bit_offset, is_enabled);
		if (ret)
		{
			goto exit_with_unlock;
		}

		if (last_enable_state)
			goto exit_with_unlock;

		/* disabled to enabled, need to wait pll stable */
		ret = pll_wait_stable(lock_ctrl_reg_addr, lock_ctrl_bit_offset,
			lock_state_reg_addr, lock_state_bit_offset,
			pll_stable_time);
		if (ret)
		{
			clk_warn("pll(%u.%u) '%s' is still not stable!",
					 ccu_clk_hw->hw.clk.cc_id, ccu_clk_hw->hw.clk.clk_id,
					 clk_hw_get_name(&ccu_clk_hw->hw));
			ret = 0;
		}

	}

exit_with_unlock:
	hal_spin_unlock_irqrestore(&controller->lock, flags);

	return ret;
}

int ccu_pll_get_enable_state(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll, int *is_enabled)
{
	int ret, pll_is_enabled;
	clk_controller_t *controller;

	uint32_t reg_addr, reg_data;
	uint32_t ldo_ctrl_bit_offset, enable_bit_offset, output_gate_bit_offset;

	uint8_t var_type;
	ccu_pll_base_type1_t *base_type1_pll;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	reg_addr = controller->reg_base + ccu_clk_hw->reg_offset;
	reg_data = readl(reg_addr);

	var_type = pll->var_type;

	if (var_type == CCU_PLL_VAR_TYPE0)
	{
		ldo_ctrl_bit_offset = pll->ldo_ctrl_bit_offset;
		enable_bit_offset = pll->enable_bit_offset;
		output_gate_bit_offset = pll->output_gate_bit_offset;
	}
	else if (var_type == CCU_PLL_VAR_TYPE1)
	{
		base_type1_pll = (ccu_pll_base_type1_t *)pll;

		ldo_ctrl_bit_offset = PLL_CTRL_BIT_ABSENT_BIT_OFFSET;
		enable_bit_offset = base_type1_pll->enable_bit_offset;
		output_gate_bit_offset = PLL_CTRL_BIT_ABSENT_BIT_OFFSET;
	}
	else
	{
		return CLK_RET_PLL_VAR_TYPE_NOT_SUPPORTED;
	}


	pll_is_enabled = 1;

	if (ldo_ctrl_bit_offset != PLL_CTRL_BIT_ABSENT_BIT_OFFSET)
	{
		if (!(reg_data & (1 << ldo_ctrl_bit_offset)))
		{
			pll_is_enabled = 0;
			goto exit_with_state;
		}
	}

	if (enable_bit_offset != PLL_CTRL_BIT_ABSENT_BIT_OFFSET)
	{
		if (!(reg_data & (1 << enable_bit_offset)))
		{
			pll_is_enabled = 0;
			goto exit_with_state;
		}

	}

	if (output_gate_bit_offset != PLL_CTRL_BIT_ABSENT_BIT_OFFSET)
	{
		if (!(reg_data & (1 << output_gate_bit_offset)))
		{
			pll_is_enabled = 0;
			goto exit_with_state;
		}

	}

exit_with_state:
	*is_enabled = pll_is_enabled;
	return 0;
}

int ccu_pll_set_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll, uint32_t parent_freq, uint32_t freq)
{
	int ret;
	unsigned long flags;
	uint32_t reg_base, pll_ctrl_reg_addr, pll_ctrl_reg_data;
	uint32_t ldo_ctrl_bit_offset, enable_bit_offset;
	uint32_t lock_ctrl_reg_addr, lock_ctrl_bit_offset;
	uint32_t lock_state_reg_addr, lock_state_bit_offset;
	uint32_t pll_stable_time;

	clk_controller_t *controller;
	uint8_t var_type;
	ccu_pll_base_type1_t *base_type1_pll;
	ccu_mult_base_t *mult;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	reg_base = controller->reg_base;
	pll_ctrl_reg_addr = reg_base + ccu_clk_hw->reg_offset;

	var_type = pll->var_type;
	if (var_type == CCU_PLL_VAR_TYPE0)
	{
		pll_stable_time = pll->stable_time;
		ldo_ctrl_bit_offset = pll->ldo_ctrl_bit_offset;
		enable_bit_offset = pll->enable_bit_offset;
		lock_ctrl_reg_addr = pll_ctrl_reg_addr;
		lock_state_reg_addr = pll_ctrl_reg_addr;
		lock_ctrl_bit_offset = pll->lock_ctrl_bit_offset;
		lock_state_bit_offset = pll->lock_state_bit_offset;
		mult = &pll->mult;
	}
	else if (var_type == CCU_PLL_VAR_TYPE1)
	{
		base_type1_pll = (ccu_pll_base_type1_t *)pll;

		pll_stable_time = base_type1_pll->stable_time;
		ldo_ctrl_bit_offset = PLL_CTRL_BIT_ABSENT_BIT_OFFSET;
		//enable_bit_offset = base_type1_pll->enable_bit_offset;
		//lock_ctrl_reg_addr = pll_ctrl_reg_addr;
		//lock_state_reg_addr = reg_base + base_type1_pll->lock_state_reg_offset;
		//lock_ctrl_bit_offset = base_type1_pll->lock_ctrl_bit_offset;
		//lock_state_bit_offset = base_type1_pll->lock_state_bit_offset;
		mult = &base_type1_pll->mult;
	}
	else
	{
		return CLK_RET_PLL_VAR_TYPE_NOT_SUPPORTED;
	}

	flags = hal_spin_lock_irqsave(&controller->lock);

	if (ldo_ctrl_bit_offset != PLL_CTRL_BIT_ABSENT_BIT_OFFSET)
	{
		pll_ctrl_reg_data = readl(pll_ctrl_reg_addr);
		pll_ctrl_reg_data |= (1 << ldo_ctrl_bit_offset);
		writel(pll_ctrl_reg_data, pll_ctrl_reg_addr);
	}

	ret= ccu_mult_set_freq_without_lock(ccu_clk_hw, mult, parent_freq, freq);
	if (ret)
	{
		goto exit_with_unlock;
	}

	if (var_type == CCU_PLL_VAR_TYPE0)
	{
		/* enable PLL */
		ret = pll_ctrl_module_enable(pll_ctrl_reg_addr, enable_bit_offset, 1);
		if (ret)
		{
			goto exit_with_unlock;
		}

		ret = pll_wait_stable(lock_ctrl_reg_addr, lock_ctrl_bit_offset,
			lock_state_reg_addr, lock_state_bit_offset,
			pll_stable_time);
		if (ret)
		{
			clk_warn("pll(%u.%u) '%s' is still not stable!",
					 ccu_clk_hw->hw.clk.cc_id, ccu_clk_hw->hw.clk.clk_id,
					 clk_hw_get_name(&ccu_clk_hw->hw));
			ret = 0;
		}
	}

exit_with_unlock:
	hal_spin_unlock_irqrestore(&controller->lock, flags);

	return ret;
}

int ccu_pll_get_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_pll_base_t *pll, uint32_t parent_freq, uint32_t *freq)
{
	//int ret, is_enabled = 0;
	uint8_t var_type;
	ccu_pll_base_type1_t *base_type1_pll;
	ccu_mult_base_t *mult;

	var_type = pll->var_type;
	if (var_type == CCU_PLL_VAR_TYPE0)
	{
		mult = &pll->mult;
	}
	else if (var_type == CCU_PLL_VAR_TYPE1)
	{
		base_type1_pll = (ccu_pll_base_type1_t *)pll;
		mult = &base_type1_pll->mult;
	}
	else
	{
		return CLK_RET_PLL_VAR_TYPE_NOT_SUPPORTED;
	}

	return ccu_mult_get_freq_without_lock(ccu_clk_hw, mult, parent_freq, freq);
}

static int pll_init(struct clk_hw *hw)
{
	struct ccu_pll *cp = hw_to_ccu_pll(hw);
	return ccu_pll_init(&cp->ccu_clk_hw, &cp->pll);
}

static int pll_set_enable_state(struct clk_hw *hw, int is_enabled)
{
	struct ccu_pll *cp = hw_to_ccu_pll(hw);
	return ccu_pll_set_enable_state(&cp->ccu_clk_hw, &cp->pll, is_enabled);
}

static int pll_get_enable_state(struct clk_hw *hw, int *is_enabled)
{
	struct ccu_pll *cp = hw_to_ccu_pll(hw);
	return ccu_pll_get_enable_state(&cp->ccu_clk_hw, &cp->pll, is_enabled);
}

static int pll_set_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t freq)
{
	struct ccu_pll *cp = hw_to_ccu_pll(hw);
	return ccu_pll_set_freq(&cp->ccu_clk_hw, &cp->pll, parent_freq, freq);
}

static int pll_get_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq)
{
	struct ccu_pll *cp = hw_to_ccu_pll(hw);
	return ccu_pll_get_freq(&cp->ccu_clk_hw, &cp->pll, parent_freq, freq);
}

static int pll_round_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq)
{
	struct ccu_pll *cp = hw_to_ccu_pll(hw);
	return ccu_mult_round_freq_without_lock(&cp->ccu_clk_hw, &cp->pll.mult, parent_freq, freq);
}

const struct clk_hw_ops g_ccu_pll_hw_ops =
{
	.init = pll_init,
	.set_enable_state = pll_set_enable_state,
	.get_enable_state = pll_get_enable_state,

	.set_freq = pll_set_freq,
	.get_freq = pll_get_freq,
	.round_freq = pll_round_freq,
};

