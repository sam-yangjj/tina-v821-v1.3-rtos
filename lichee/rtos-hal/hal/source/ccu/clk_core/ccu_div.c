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
#include "ccu_div.h"
#include "ccu_gate.h"

static inline int ccu_div_base_check(const ccu_div_base_t *div);
static int ccu_div_base_get_max_div_value(const ccu_div_base_t *div, uint32_t *max_div_value);

static int is_power_of_2(uint32_t n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

static inline int __ffs(uint32_t value)
{
	uint32_t offset;

	for (offset = 0; offset < sizeof(value) * 8; offset++)
	{
		if (value & (1 << offset))
		{
			return offset;
		}
	}
	return -1;
}

static inline struct ccu_div *hw_to_ccu_div(struct clk_hw *hw)
{
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);

	return container_of(ccu_clk_hw, struct ccu_div, ccu_clk_hw);
}

static int integer_div_get_target_div_value(uint32_t parent_freq, uint32_t freq, uint32_t *div_value)
{
	uint32_t tmp_div_val, real_output_freq;

	if (freq > parent_freq)
	{
		return CLK_RET_TARGET_FREQ_GREATER_THAN_PARENT_FREQ;
	}

	tmp_div_val = parent_freq / freq;

	real_output_freq = parent_freq / tmp_div_val;
	if (real_output_freq != freq)
		return CLK_RET_TARGET_FREQ_NOT_AVAILABLE;

	*div_value = tmp_div_val;
	return 0;
}

static inline uint8_t ccu_div_get_link_type(const ccu_div_t *cd)
{
#ifdef SAVE_CCU_DIV_STRUCT_MEM
	return cd->div.link_type;
#else
	return cd->link_type;
#endif
}

static inline int ccu_div_base_check(const ccu_div_base_t *div)
{
	const ccu_div_base_with_map_t *map_div;
	const division_map_t *map;
	uint32_t max_field_value;

	if (div->var_type >= CCU_DIV_VAR_TYPE_MAX)
		return CLK_RET_INVALID_DIV_VAR_TYPE;

	if (div->var_type == CCU_DIV_VAR_TYPE2)
	{
		map_div = (const ccu_div_base_with_map_t *)div;
		if (!map_div->map_table)
			return CLK_RET_DIV_MAP_NOT_EXIST_IN_VARIANT;

		max_field_value = (1 << div->field_width) - 1;
		for (map = map_div->map_table; map->div_value; map++)
		{
			if (map->field_value > max_field_value)
				return CLK_RET_DIV_MAP_FIELD_VALUE_EXCEED_UPPER_LIMIT;
		}
	}

	return clk_core_check_ctrl_field_info(div->field_offset, div->field_width, UINT16_MAX);
}

static int ccu_div_base_field_to_div_value(const ccu_div_base_t *div,
	uint32_t field_value, uint32_t *div_value)
{
	uint8_t var_type;
	uint32_t tmp_div_val = 0;
	const ccu_div_base_with_map_t *map_div;
	const division_map_t *map;

	var_type = div->var_type;

	if (var_type == CCU_DIV_VAR_TYPE0)
	{
		tmp_div_val = field_value + 1;
	}
	else if (var_type == CCU_DIV_VAR_TYPE1)
	{
		tmp_div_val = 1 << field_value;
	}
	else if (var_type == CCU_DIV_VAR_TYPE2)
	{
		map_div = (const ccu_div_base_with_map_t *)div;
		for (map = map_div->map_table; map->div_value; map++)
		{
			if (map->field_value == field_value)
			{
				tmp_div_val = map->div_value;
				break;
			}
		}

		if (!map->div_value)
			return CLK_RET_FIELD_VALUE_NOT_FOUND_IN_MAP;
	}

	else
	{
		return CLK_RET_DIV_VAR_TYPE_NOT_SUPPORTED;
	}

	*div_value = tmp_div_val;
	return 0;
}

static int ccu_div_base_div_to_field_value(const ccu_div_base_t *div,
	uint32_t div_value, uint32_t *field_value)
{
	int ret;
	uint8_t var_type;
	uint32_t max_div_value = 0, tmp_field_value = 0, max_field_value;
	const ccu_div_base_with_map_t *map_div;
	const division_map_t *map;

	if (!div_value)
		return CLK_RET_DIV_VAL_IS_ZERO;

	ret = ccu_div_base_get_max_div_value(div, &max_div_value);
	if (ret)
		return ret;

	if (div_value > max_div_value)
		return CLK_RET_DIV_VAL_EXCEED_UPPER_LIMIT;

	var_type = div->var_type;
	if (var_type == CCU_DIV_VAR_TYPE0)
	{
		tmp_field_value = div_value - 1;
	}
	else if (var_type == CCU_DIV_VAR_TYPE1)
	{
		if (!is_power_of_2(div_value))
			return CLK_RET_DIV_VAL_IS_NOT_POWER_OF_TWO;

		tmp_field_value = __ffs(div_value);
	}
	else if (var_type == CCU_DIV_VAR_TYPE2)
	{
		map_div = (const ccu_div_base_with_map_t *)div;
		for (map = map_div->map_table; map->div_value; map++)
		{
			if (map->div_value == div_value)
				break;
		}

		if (!map->div_value)
			return CLK_RET_DIV_VAL_NOT_FOUND_IN_MAP;

		tmp_field_value = map->field_value;
	}
	else
	{
		return CLK_RET_DIV_VAR_TYPE_NOT_SUPPORTED;
	}

	max_field_value = (1 << div->field_width) - 1;
	if (tmp_field_value > max_field_value)
	{
		return CLK_RET_FIELD_VALUE_EXCEED_UPPER_LIMIT;
	}

	*field_value = tmp_field_value;
	return 0;
}

static int ccu_div_base_get_target_field_value(const ccu_div_base_t *div,
	uint32_t parent_freq, uint32_t freq, uint32_t *field_value)
{
	int ret;
	uint32_t div_val, tmp_field_value;

	div_val = 0;
	ret = integer_div_get_target_div_value(parent_freq, freq, &div_val);
	if (ret)
		return ret;

	tmp_field_value = 0;
	ret = ccu_div_base_div_to_field_value(div, div_val, &tmp_field_value);
	if (ret)
		return ret;

	*field_value = tmp_field_value;
	return 0;
}

static int ccu_div_base_get_closest_div_value(const ccu_div_base_t *div,
	uint32_t *div_value)
{
	int ret;
	uint32_t target_div_val, tmp_div_val = 0, field_value, max_field_value;
	uint32_t diff, min_diff = 0;
	int is_less, target_div_is_less = 0;

	target_div_val = *div_value;
	max_field_value = (1 << div->field_width) - 1;
	for (field_value = 0; field_value < max_field_value; field_value++)
	{
		ret = ccu_div_base_field_to_div_value(div, field_value, &tmp_div_val);
		if (ret)
		{
			if (ret == CLK_RET_FIELD_VALUE_NOT_FOUND_IN_MAP)
				continue;

			return ret;
		}

		if (target_div_val >= tmp_div_val)
		{
			diff = target_div_val - tmp_div_val;
			is_less = 0;
		}
		else
		{
			diff = tmp_div_val - target_div_val;
			is_less = 1;
		}
		if (min_diff > diff)
		{
			min_diff = diff;
			target_div_is_less = is_less;
		}
	}

	if (target_div_is_less)
	{
		tmp_div_val = target_div_val + min_diff;
	}
	else
	{
		tmp_div_val = target_div_val - min_diff;
	}

	*div_value = tmp_div_val;
	return 0;
}

static int ccu_div_base_get_max_div_value(const ccu_div_base_t *div,
	uint32_t *max_div_value)
{
	int ret;
	uint8_t var_type;
	uint32_t div_val, max_field_value;
	const ccu_div_base_with_map_t *map_div;
	const division_map_t *map;

	var_type = div->var_type;

	if (var_type == CCU_DIV_VAR_TYPE2)
	{
		map_div = (const ccu_div_base_with_map_t *)div;
		map = map_div->map_table;
		div_val = map->div_value;
		map++;
		for (; map->div_value; map++)
		{
			if (map->div_value > div_val)
				div_val = map->div_value;
		}
	}
	else
	{
		div_val = 0;
		max_field_value = (1 << div->field_width) - 1;
		ret = ccu_div_base_field_to_div_value(div, max_field_value, &div_val);
		if (ret)
			return ret;
	}

	*max_div_value = div_val;
	return 0;
}


static int ccu_double_div_base_field_to_div_value(const ccu_div_base_t *div1, const ccu_div_base_t *div2,
	uint32_t div1_field_value, uint32_t div2_field_value, uint32_t *div_val)
{
	int ret;
	uint32_t div1_val, div2_val;

	div1_val = 0;
	ret = ccu_div_base_field_to_div_value(div1, div1_field_value, &div1_val);
	if (ret)
		return ret;

	div2_val = 0;
	ret = ccu_div_base_field_to_div_value(div2, div2_field_value, &div2_val);
	if (ret)
		return ret;

	*div_val = div1_val * div2_val;
	return 0;
}

static int ccu_double_div_base_div_to_field_value(const ccu_div_base_t *div1,
	const ccu_div_base_t *div2,
	uint32_t div_value,
	uint32_t *div1_field_value, uint32_t *div2_field_value)
{
	int ret;
	uint32_t div1_max_field_value, div2_max_field_value;
	uint32_t tmp_div1_f_val, tmp_div2_f_val;
	uint32_t tmp_div1_val, tmp_div2_val;
	uint32_t tmp_div_val;

	div1_max_field_value = (1 << div1->field_width) - 1;
	div2_max_field_value = (1 << div2->field_width) - 1;
	for (tmp_div1_f_val = 0; tmp_div1_f_val < div1_max_field_value; tmp_div1_f_val++)
	{
		ret = ccu_div_base_field_to_div_value(div1, tmp_div1_f_val, &tmp_div1_val);
		if (ret)
		{
			if (ret == CLK_RET_FIELD_VALUE_NOT_FOUND_IN_MAP)
				continue;

			return ret;
		}

		for (tmp_div2_f_val = 0; tmp_div2_f_val < div2_max_field_value; tmp_div2_f_val++)
		{
			ret = ccu_div_base_field_to_div_value(div2, tmp_div2_f_val, &tmp_div2_val);
			if (ret)
			{
				if (ret == CLK_RET_FIELD_VALUE_NOT_FOUND_IN_MAP)
					continue;
			
				return ret;
			}

			tmp_div_val = tmp_div1_val * tmp_div2_val;
			if (tmp_div_val == div_value)
			{
				*div1_field_value = tmp_div1_f_val;
				*div2_field_value = tmp_div2_f_val;
				return 0;
			}
		}
	}

	return CLK_RET_FIELD_VALUE_NOT_FOUND_FOR_TARGET_FREQ;
}

static int ccu_double_div_base_get_target_field_value(const ccu_div_base_t *div1,
	const ccu_div_base_t *div2,
	uint32_t parent_freq, uint32_t freq, uint32_t *div1_field_value, uint32_t *div2_field_value)
{
	int ret;
	uint32_t div_val, tmp1, tmp2;

	div_val = 0;
	ret = integer_div_get_target_div_value(parent_freq, freq, &div_val);
	if (ret)
		return ret;

	tmp1 = 0;
	tmp2 = 0;
	ret = ccu_double_div_base_div_to_field_value(div1, div2, div_val, &tmp1, &tmp2);
	if (ret)
		return ret;

	*div1_field_value = tmp1;
	*div2_field_value = tmp2;
	return 0;

}

static int ccu_double_div_base_get_closest_div_value(const ccu_div_base_t *div1,
	const ccu_div_base_t *div2,
	uint32_t *div_value)
{
	int ret;
	uint32_t tmp_div1_f_val, tmp_div2_f_val, div1_max_field_value, div2_max_field_value;
	uint32_t tmp_div1_val, tmp_div2_val;
	uint32_t target_div_val, tmp_div_val;
	uint32_t diff, min_diff = 0;
	int is_less, target_div_is_less = 0;

	target_div_val = *div_value;
	div1_max_field_value = (1 << div1->field_width) - 1;
	div2_max_field_value = (1 << div2->field_width) - 1;
	for (tmp_div1_f_val = 0; tmp_div1_f_val < div1_max_field_value; tmp_div1_f_val++)
	{
		ret = ccu_div_base_field_to_div_value(div1, tmp_div1_f_val, &tmp_div1_val);
		if (ret)
		{
			if (ret == CLK_RET_FIELD_VALUE_NOT_FOUND_IN_MAP)
				continue;

			return ret;
		}

		for (tmp_div2_f_val = 0; tmp_div2_f_val < div2_max_field_value; tmp_div2_f_val++)
		{
			ret = ccu_div_base_field_to_div_value(div2, tmp_div2_f_val, &tmp_div2_val);
			if (ret)
			{
				if (ret == CLK_RET_FIELD_VALUE_NOT_FOUND_IN_MAP)
					continue;

				return ret;
			}

			tmp_div_val = tmp_div1_val * tmp_div2_val;
			if (target_div_val >= tmp_div_val)
			{
				diff = target_div_val - tmp_div_val;
				is_less = 0;
			}
			else
			{
				diff = tmp_div_val - target_div_val;
				is_less = 1;
			}

			if (min_diff > diff)
			{
				min_diff = diff;
				target_div_is_less = is_less;
			}
		}
	}

	if (target_div_is_less)
	{
		tmp_div_val = target_div_val + min_diff;
	}
	else
	{
		tmp_div_val = target_div_val - min_diff;
	}

	*div_value = tmp_div_val;
	return 0;
}

int ccu_div_link_div_set_freq(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					 uint32_t parent_freq, uint32_t freq, int is_need_lock)
{
	int ret;
	unsigned long flags;
	uint32_t div1_f_val, div2_f_val;
	uint32_t reg_addr, reg_data;
	clk_controller_t *controller;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	ret = ccu_double_div_base_get_target_field_value(div1, div2,
		parent_freq, freq, &div1_f_val, &div2_f_val);
	if (ret)
		return ret;

	clk_hw_dbg(&ccu_clk_hw->hw, "parent_freq: %u, freq: %u, div1_f_val: %u, div2_f_val: %u",
		parent_freq, freq, div1_f_val, div2_f_val);

	reg_addr = controller->reg_base + ccu_clk_hw->reg_offset;

	div1_f_val &= (1 << div1->field_width) - 1;
	div2_f_val &= (1 << div2->field_width) - 1;

	if (is_need_lock)
		flags = hal_spin_lock_irqsave(&controller->lock);

	reg_data = readl(reg_addr);

	reg_data &= ~(((1 << div1->field_width) - 1) << div1->field_offset);
	reg_data &= ~(((1 << div2->field_width) - 1) << div2->field_offset);

	writel(reg_data |
		(div1_f_val << div1->field_offset) | (div2_f_val << div2->field_offset),
		reg_addr);

	if (is_need_lock)
		hal_spin_unlock_irqrestore(&controller->lock, flags);

	return 0;
}

int ccu_div_link_div_get_freq(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
	uint32_t parent_freq, uint32_t *freq, int is_need_lock)
{
	int ret;
	uint32_t div1_f_val, div2_f_val;
	uint32_t div_val;
	uint32_t reg_addr, reg_data;
	clk_controller_t *controller;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	reg_addr = controller->reg_base + ccu_clk_hw->reg_offset;

	/* input parameter 'is_need_lock' is reserved for possible future using, currently not need */
	reg_data = readl(reg_addr);

	div1_f_val = reg_data >> div1->field_offset;
	div1_f_val &= (1 << div1->field_width) - 1;

	div2_f_val = reg_data >> div2->field_offset;
	div2_f_val &= (1 << div2->field_width) - 1;

	div_val = 0;
	ret = ccu_double_div_base_field_to_div_value(div1, div2, div1_f_val, div2_f_val, &div_val);
	if (ret)
		return ret;

	*freq = parent_freq / div_val;
	return 0;
}

int ccu_div_link_div_round_freq(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					   uint32_t parent_freq, uint32_t *freq, int is_need_lock)
{
	int ret;
	uint32_t div_val, tmp1, tmp2;

	div_val = DIV_ROUND_CLOSEST_ULL(parent_freq, *freq);
	if (!div_val)
		div_val = 1;

	/* input parameter 'is_need_lock' is reserved for possible future using, currently not need */
	ret = ccu_double_div_base_div_to_field_value(div1, div2, div_val, &tmp1, &tmp2);
	if (ret)
	{
		/* closest division value is not available, try to get a new closest division value */
		ret = ccu_double_div_base_get_closest_div_value(div1, div2, &div_val);
		if (ret)
			return ret;
	}

	*freq = parent_freq / div_val;
	return 0;
}

int ccu_div_init(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_div_base_t *div)
{
	return ccu_div_base_check(div);
}

int ccu_div_set_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_div_base_t *div,
					 uint32_t parent_freq, uint32_t freq, int is_need_lock)
{
	int ret;
	unsigned long flags;
	uint32_t reg_addr, reg_data, field_value = 0;
	clk_controller_t *controller;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	ret = ccu_div_base_get_target_field_value(div, parent_freq, freq, &field_value);
	if (ret)
		return ret;

	reg_addr = controller->reg_base + ccu_clk_hw->reg_offset;

	field_value &= (1 << div->field_width) - 1;

	if (is_need_lock)
		flags = hal_spin_lock_irqsave(&controller->lock);

	reg_data = readl(reg_addr);

	reg_data &= ~(((1 << div->field_width) - 1) << div->field_offset);

	writel(reg_data | (field_value << div->field_offset), reg_addr);

	if (is_need_lock)
		hal_spin_unlock_irqrestore(&controller->lock, flags);

	return 0;
}

int ccu_div_get_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_div_base_t *div,
					 uint32_t parent_freq, uint32_t *freq, int is_need_lock)
{
	int ret;
	unsigned long flags;
	uint32_t reg_addr, reg_data, field_value, div_val;
	clk_controller_t *controller;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	reg_addr = controller->reg_base + ccu_clk_hw->reg_offset;

	if (is_need_lock)
		flags = hal_spin_lock_irqsave(&controller->lock);

	reg_data = readl(reg_addr);

	if (is_need_lock)
		hal_spin_unlock_irqrestore(&controller->lock, flags);

	field_value = reg_data >> div->field_offset;
	field_value &= (1 << div->field_width) - 1;

	div_val = 1;
	ret = ccu_div_base_field_to_div_value(div, field_value, &div_val);
	if (ret)
		return ret;

	clk_hw_dbg(&ccu_clk_hw->hw, "field_value: %u, div_val: %u", field_value, div_val);
	*freq = parent_freq / div_val;
	return 0;
}

int ccu_div_round_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_div_base_t *div,
					   uint32_t parent_freq, uint32_t *freq, int is_need_lock)
{
	int ret;
	uint32_t closest_div_val, field_value;

	closest_div_val = DIV_ROUND_CLOSEST_ULL(parent_freq, *freq);
	if (!closest_div_val)
		closest_div_val = 1;

	ret = ccu_div_base_div_to_field_value(div, closest_div_val, &field_value);
	if (ret)
	{
		/* closest division value is not available, try to get a new closest division value */
		ret = ccu_div_base_get_closest_div_value(div, &closest_div_val);
		if (ret)
			return ret;
	}

	*freq = parent_freq / closest_div_val;
	return 0;
}

int div_init(struct clk_hw *hw)
{
	ccu_div_t *cd = hw_to_ccu_div(hw);
	return ccu_div_init(&cd->ccu_clk_hw, &cd->div);
}

static int div_set_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t freq)
{
	struct ccu_div *cd = hw_to_ccu_div(hw);
	return ccu_div_set_freq_with_lock(&cd->ccu_clk_hw, &cd->div, parent_freq, freq);
}

static int div_get_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq)
{
	struct ccu_div *cd = hw_to_ccu_div(hw);
	return ccu_div_get_freq_without_lock(&cd->ccu_clk_hw, &cd->div, parent_freq, freq);
}

static int div_round_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq)
{
	struct ccu_div *cd = hw_to_ccu_div(hw);
	return ccu_div_round_freq_without_lock(&cd->ccu_clk_hw, &cd->div, parent_freq, freq);
}

static int div_link_get_gate_bit_offset(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t *gate_bit_offset)
{
	uint8_t link_type;
	ccu_div_link_type1_t *type1;

	ccu_div_t *cd = hw_to_ccu_div(&ccu_clk_hw->hw);

	link_type = ccu_div_get_link_type(cd);

	if (!link_type)
	{
		clk_dbg("%u.%u, no gate clk hw in link type", ccu_clk_hw->hw.clk.cc_id, ccu_clk_hw->hw.clk.clk_id);
		return CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE;
	}

	if (link_type == CCU_DIV_LINK_TYPE1)
	{
		type1 = (ccu_div_link_type1_t *)cd;
		*gate_bit_offset = type1->gate_bit_offset;
	}
	else
	{
		return CLK_RET_DIV_LINK_TYPE_NOT_SUPPORTED;
	}

	return 0;
}

static int div_link_set_gate_enable_state(struct clk_hw *hw, int is_enabled)
{
	int ret;
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);
	uint8_t gate_bit_offset = GATE_ABSENT_BIT_OFFSET;

	ret = div_link_get_gate_bit_offset(ccu_clk_hw, &gate_bit_offset);
	if (ret)
	{
		if (ret == CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE)
			return 0;

		return ret;
	}

	return ccu_gate_set_enable_state_with_lock(ccu_clk_hw, gate_bit_offset, is_enabled);
}

static int div_link_get_gate_enable_state(struct clk_hw *hw, int *is_enabled)
{
	int ret;
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);
	uint8_t gate_bit_offset = GATE_ABSENT_BIT_OFFSET;

	ret = div_link_get_gate_bit_offset(ccu_clk_hw, &gate_bit_offset);
	if (ret)
	{
		if (ret == CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE)
		{
			*is_enabled = 1;
			return 0;
		}

		return ret;
	}

	return ccu_gate_get_enable_state_without_lock(ccu_clk_hw, gate_bit_offset, is_enabled);
}

const struct clk_hw_ops g_ccu_div_hw_ops =
{
	.init = div_init,

	.set_enable_state = div_link_set_gate_enable_state,
	.get_enable_state = div_link_get_gate_enable_state,

	.set_freq = div_set_freq,
	.get_freq = div_get_freq,
	.round_freq = div_round_freq,
};

