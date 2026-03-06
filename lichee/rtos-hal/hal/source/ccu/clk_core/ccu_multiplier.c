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
#include "ccu_multiplier.h"

static int ccu_mult_base_get_max_mult_value(const ccu_mult_base_t *mult, uint32_t *max_mult_value);

static inline struct ccu_mult *hw_to_ccu_mult(struct clk_hw *hw)
{
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);

	return container_of(ccu_clk_hw, struct ccu_mult, ccu_clk_hw);
}

static int integer_mult_get_target_mult_value(uint32_t parent_freq, uint32_t freq, uint32_t *mult_value)
{
	uint32_t tmp_mult_val, real_output_freq;

	if (freq < parent_freq)
	{
		return CLK_RET_TARGET_FREQ_LESS_THAN_PARENT_FREQ;
	}

	tmp_mult_val = freq / parent_freq;

	real_output_freq = parent_freq * tmp_mult_val;
	if (real_output_freq != freq)
		return CLK_RET_TARGET_FREQ_NOT_AVAILABLE;

	*mult_value = tmp_mult_val;
	return 0;
}

static int ccu_mult_base_field_to_mult_value(const ccu_mult_base_t *mult,
	uint32_t field_value, uint32_t *mult_value)
{
	uint8_t var_type;
	uint32_t tmp_mult_val = 0;

	var_type = mult->var_type;

	if (var_type == CCU_MULT_VAR_TYPE0)
	{
		tmp_mult_val = field_value + 1;
	}
	else if (var_type == CCU_MULT_VAR_TYPE1)
	{
		if (field_value == 0)
			tmp_mult_val = 1;
		else
			tmp_mult_val = field_value;
	}
	else
	{
		return CLK_RET_MULT_VAR_TYPE_NOT_SUPPORTED;
	}

	*mult_value = tmp_mult_val;
	return 0;
}

static int ccu_mult_base_mult_to_field_value(const ccu_mult_base_t *mult,
	uint32_t mult_value, uint32_t *field_value)
{
	int ret;
	uint8_t var_type;
	uint32_t max_mult_value = 0, tmp_field_value = 0, max_field_value;

	if (!mult_value)
		return CLK_RET_MULT_VAL_IS_ZERO;

	ret = ccu_mult_base_get_max_mult_value(mult, &max_mult_value);
	if (ret)
		return ret;

	if (mult_value > max_mult_value)
		return CLK_RET_MULT_VAL_EXCEED_UPPER_LIMIT;

	var_type = mult->var_type;
	if (var_type == CCU_MULT_VAR_TYPE0)
	{
		tmp_field_value = mult_value - 1;
	}
	else if (var_type == CCU_MULT_VAR_TYPE1)
	{
		tmp_field_value = mult_value;
	}
	else
	{
		return CLK_RET_DIV_VAR_TYPE_NOT_SUPPORTED;
	}

	max_field_value = (1 << mult->field_width) - 1;
	if (tmp_field_value > max_field_value)
	{
		return CLK_RET_FIELD_VALUE_EXCEED_UPPER_LIMIT;
	}

	*field_value = tmp_field_value;
	return 0;
}

static int ccu_mult_base_get_target_field_value(const ccu_mult_base_t *mult,
	uint32_t parent_freq, uint32_t freq, uint32_t *field_value)
{
	int ret;
	uint32_t mult_val, tmp_field_value;

	mult_val = 0;
	ret = integer_mult_get_target_mult_value(parent_freq, freq, &mult_val);
	if (ret)
		return ret;

	tmp_field_value = 0;
	ret = ccu_mult_base_mult_to_field_value(mult, mult_val, &tmp_field_value);
	if (ret)
		return ret;

	*field_value = tmp_field_value;
	return 0;
}

static int ccu_mult_base_get_closest_mult_value(const ccu_mult_base_t *mult,
	uint32_t *mult_value)
{
	int ret;
	uint32_t target_mult_val, tmp_mult_val = 0, field_value, max_field_value;
	uint32_t diff, min_diff = 0;
	int is_less, target_mult_is_less = 0;

	target_mult_val = *mult_value;
	max_field_value = (1 << mult->field_width) - 1;
	for (field_value = 0; field_value < max_field_value; field_value++)
	{
		ret = ccu_mult_base_field_to_mult_value(mult, field_value, &tmp_mult_val);
		if (ret)
		{
			if (ret == CLK_RET_FIELD_VALUE_NOT_FOUND_IN_MAP)
				continue;

			return ret;
		}

		if (target_mult_val >= tmp_mult_val)
		{
			diff = target_mult_val - tmp_mult_val;
			is_less = 0;
		}
		else
		{
			diff = tmp_mult_val - target_mult_val;
			is_less = 1;
		}
		if (min_diff > diff)
		{
			min_diff = diff;
			target_mult_is_less = is_less;
		}
	}

	if (target_mult_is_less)
	{
		tmp_mult_val = target_mult_val + min_diff;
	}
	else
	{
		tmp_mult_val = target_mult_val - min_diff;
	}

	*mult_value = tmp_mult_val;
	return 0;
}

static int ccu_mult_base_get_max_mult_value(const ccu_mult_base_t *mult,
	uint32_t *max_mult_value)
{
	int ret;
	uint32_t mult_val, max_field_value;

	mult_val = 0;
	max_field_value = (1 << mult->field_width) - 1;
	ret = ccu_mult_base_field_to_mult_value(mult, max_field_value, &mult_val);
	if (ret)
		return ret;

	*max_mult_value = mult_val;
	return 0;
}

int ccu_mult_init(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mult_base_t *mult)
{
	return clk_core_check_ctrl_field_info(mult->field_offset, mult->field_width, UINT32_MAX);
}

int ccu_mult_set_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mult_base_t *mult,
					  uint32_t parent_freq, uint32_t freq, int is_need_lock)
{
	int ret;
	clk_controller_t *controller;
	unsigned long flags;

	uint32_t reg_addr, reg_data, field_value;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	ret = ccu_mult_base_get_target_field_value(mult, parent_freq, freq, &field_value);
	if (ret)
		return ret;

	reg_addr = controller->reg_base + ccu_clk_hw->reg_offset;

	if (is_need_lock)
		flags = hal_spin_lock_irqsave(&controller->lock);

	reg_data = readl(reg_addr);
	reg_data &= ~(((1 << mult->field_width) - 1) << mult->field_offset);

	writel(reg_data | (field_value << mult->field_offset), reg_addr);

	if (is_need_lock)
		hal_spin_unlock_irqrestore(&controller->lock, flags);

	return 0;
}

int ccu_mult_get_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mult_base_t *mult,
					  uint32_t parent_freq, uint32_t *freq, int is_need_lock)
{
	int ret;

	uint32_t reg_addr, reg_data, field_value, mult_val;
	clk_controller_t *controller;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	reg_addr = controller->reg_base + ccu_clk_hw->reg_offset;

	reg_data = readl(reg_addr);
	field_value = reg_data >> mult->field_offset;
	field_value &= (1 << mult->field_width) - 1;

	mult_val = 1;
	ret = ccu_mult_base_field_to_mult_value(mult, field_value, &mult_val);
	if (ret)
		return ret;

	clk_hw_dbg(&ccu_clk_hw->hw, "parent_freq: %u, field_value: %u, mult_value: %u",
		parent_freq, field_value, mult_val);
	*freq = parent_freq * mult_val;
	return 0;
}

int ccu_mult_round_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mult_base_t *mult,
						uint32_t parent_freq, uint32_t *freq, int is_need_lock)
{
	int ret;
	uint32_t closest_mult_val, field_value;

	closest_mult_val = DIV_ROUND_CLOSEST_ULL(*freq, parent_freq);
	if (!closest_mult_val)
		closest_mult_val = 1;

	ret = ccu_mult_base_mult_to_field_value(mult, closest_mult_val, &field_value);
	if (ret)
	{
		/* closest multiple value is not available, try to get a new closest multiple value */
		ret = ccu_mult_base_get_closest_mult_value(mult, &closest_mult_val);
		if (ret)
			return ret;
	}

	*freq = parent_freq * closest_mult_val;
	return 0;
}

static int mult_init(struct clk_hw *hw)
{
	struct ccu_mult *cm = hw_to_ccu_mult(hw);
	return ccu_mult_init(&cm->ccu_clk_hw, &cm->mult);
}

static int mult_set_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t freq)
{
	struct ccu_mult *cm = hw_to_ccu_mult(hw);
	return ccu_mult_set_freq_with_lock(&cm->ccu_clk_hw, &cm->mult, parent_freq, freq);
}

static int mult_get_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq)
{
	struct ccu_mult *cm = hw_to_ccu_mult(hw);
	return ccu_mult_get_freq_without_lock(&cm->ccu_clk_hw, &cm->mult, parent_freq, freq);
}

static int mult_round_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq)
{
	struct ccu_mult *cm = hw_to_ccu_mult(hw);
	return ccu_mult_round_freq_without_lock(&cm->ccu_clk_hw, &cm->mult, parent_freq, freq);
}

const struct clk_hw_ops g_ccu_mult_hw_ops =
{
	.init = mult_init,
	.set_freq = mult_set_freq,
	.get_freq = mult_get_freq,
	.round_freq = mult_round_freq,
};

