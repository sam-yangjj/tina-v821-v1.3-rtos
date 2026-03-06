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
#include "ccu_mux.h"
#include "ccu_gate.h"

static int mux_link_get_gate_bit_offset(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t *gate_bit_offset);
static int mux_link_get_ccu_div(aw_ccu_clk_hw_t *ccu_clk_hw, uint32_t div_id, ccu_div_base_t **div);

static inline struct ccu_mux *hw_to_ccu_mux(struct clk_hw *hw)
{
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);

	return container_of(ccu_clk_hw, struct ccu_mux, ccu_clk_hw);
}

static inline uint8_t ccu_mux_get_link_type(const ccu_mux_t *cm)
{
#ifdef SAVE_CCU_MUX_STRUCT_MEM
	return cm->mux.link_type;
#else
	return cm->link_type;
#endif
}

int ccu_mux_set_parent(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mux_base_t *mux, uint8_t parent_index)
{
	int ret;
	clk_controller_t *controller;
	ccu_mux_base_with_update_bit_t *mux_var1;
	uint32_t reg_addr, reg_data, field_value, flags;

	ret = clk_hw_get_controller(&ccu_clk_hw->hw, &controller);
	if (ret)
		return ret;

	field_value = parent_index;

	reg_addr = controller->reg_base + ccu_clk_hw->reg_offset;

	flags = hal_spin_lock_irqsave(&controller->lock);

	reg_data = readl(reg_addr);
	reg_data &= ~(((1 << mux->field_width) - 1) << mux->field_offset);
	reg_data |= (field_value << mux->field_offset);
	writel(reg_data, reg_addr);

	if (mux->var_type == CCU_MUX_VAR_TYPE1)
	{
		mux_var1 = (ccu_mux_base_with_update_bit_t *)mux;
		ret = ccu_clk_hw_set_control_bit_without_lock(ccu_clk_hw,
			mux_var1->update_bit_offset, CCU_MUX_CLK_HW_VALIDATE_CFG_VALUE);
	}

	hal_spin_unlock_irqrestore(&controller->lock, flags);

	return ret;
}

int ccu_mux_get_parent(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mux_base_t *mux, uint8_t *index)
{
	int ret;
	unsigned long reg_base = 0;

	u32 reg_data;
	u8 field_value;

	ret = clk_hw_get_controller_reg_base(&ccu_clk_hw->hw, &reg_base);
	if (ret)
		return ret;

	reg_data = readl(reg_base + ccu_clk_hw->reg_offset);
	field_value = (reg_data >> mux->field_offset) & ((1 << mux->field_width) - 1);

	*index = field_value;

	return 0;
}

int ccu_mux_init(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mux_base_t *mux)
{
	int ret;
	uint8_t link_type;
	ccu_mux_t *cm = hw_to_ccu_mux(&ccu_clk_hw->hw);
	ccu_div_base_t *div1 = NULL, *div2 = NULL;
	uint8_t gate_bit_offset = GATE_ABSENT_BIT_OFFSET;

	link_type = ccu_mux_get_link_type(cm);

	clk_hw_dbg(&ccu_clk_hw->hw, "mux link type: %u", link_type);

	clk_hw_dbg(&ccu_clk_hw->hw, "try to get gate from mux link type");
	ret = mux_link_get_gate_bit_offset(ccu_clk_hw, &gate_bit_offset);
	if (ret)
	{
		if (ret != CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE)
			return ret;
	}
	else
	{
		ret = ccu_gate_init(ccu_clk_hw, gate_bit_offset);
		if (ret)
			return ret;
	}

	clk_hw_dbg(&ccu_clk_hw->hw, "try to get div1 from mux link type");
	ret = mux_link_get_ccu_div(ccu_clk_hw, 0, &div1);
	if (ret)
	{
		if (ret != CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE)
			return ret;
	}
	else
	{
		ret = ccu_div_init(ccu_clk_hw, div1);
		if (ret)
			return ret;
	}

	if (link_type == CCU_MUX_LINK_TYPE4)
	{
		clk_hw_dbg(&ccu_clk_hw->hw, "try to get div2 from mux link type");
		ret = mux_link_get_ccu_div(ccu_clk_hw, 1, &div2);
		if (ret)
		{
			return ret;
		}
		else
		{
			ret = ccu_div_init(ccu_clk_hw, div2);
			if (ret)
				return ret;
		}
	}

	return clk_core_check_ctrl_field_info(mux->field_offset, mux->field_width, MAX_MUX_CTRL_FIELD_VALUE);
}

static int mux_init(struct clk_hw *hw)
{
	ccu_mux_t *cm = hw_to_ccu_mux(hw);
	return ccu_mux_init(&cm->ccu_clk_hw, &cm->mux);
}

static int mux_set_parent(struct clk_hw *hw, u8 parent_index)
{
	ccu_mux_t *cm = hw_to_ccu_mux(hw);
	return ccu_mux_set_parent(&cm->ccu_clk_hw, &cm->mux, parent_index);
}

static int mux_get_parent(struct clk_hw *hw, uint8_t *parent_index)
{
	ccu_mux_t *cm = hw_to_ccu_mux(hw);

	return ccu_mux_get_parent(&cm->ccu_clk_hw, &cm->mux, parent_index);
}

static int mux_link_get_gate_bit_offset(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t *gate_bit_offset)
{
	uint8_t link_type;
	ccu_mux_link_type1_t *type1;
	ccu_mux_link_type3_t *type3;
	ccu_mux_link_type4_t *type4;

	ccu_mux_t *cm = hw_to_ccu_mux(&ccu_clk_hw->hw);

	link_type = ccu_mux_get_link_type(cm);

	if ((!link_type) || (link_type == CCU_MUX_LINK_TYPE2))
	{
		clk_dbg("%u.%u, no gate clk hw in link type", ccu_clk_hw->hw.clk.cc_id, ccu_clk_hw->hw.clk.clk_id);
		return CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE;
	}

	if (link_type == CCU_MUX_LINK_TYPE1)
	{
		type1 = (ccu_mux_link_type1_t *)cm;
		*gate_bit_offset = type1->gate_bit_offset;
	}
	else if (link_type == CCU_MUX_LINK_TYPE3)
	{
		type3 = (ccu_mux_link_type3_t *)cm;
		*gate_bit_offset = type3->gate_bit_offset;
	}
	else if (link_type == CCU_MUX_LINK_TYPE4)
	{
		type4 = (ccu_mux_link_type4_t *)cm;
		*gate_bit_offset = type4->gate_bit_offset;
	}
	else
	{
		return CLK_RET_LINK_TYPE_NOT_SUPPORTED;
	}

	return 0;
}

static int mux_link_get_ccu_div(aw_ccu_clk_hw_t *ccu_clk_hw, uint32_t div_id, ccu_div_base_t **div)
{
	uint8_t link_type;
	ccu_mux_link_type2_t *mux_link_type2;
	ccu_mux_link_type4_t *mux_link_type4;

	ccu_mux_t *cm = hw_to_ccu_mux(&ccu_clk_hw->hw);

	link_type = ccu_mux_get_link_type(cm);

	if ((!link_type) || (link_type == CCU_MUX_LINK_TYPE1))
	{
		clk_dbg("%u.%u, no divider clk hw in link type", ccu_clk_hw->hw.clk.cc_id, ccu_clk_hw->hw.clk.clk_id);
		return CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE;
	}

	if ((link_type == CCU_MUX_LINK_TYPE2) || (link_type == CCU_MUX_LINK_TYPE3))
	{
		if (div_id != 0)
			return CLI_RET_INVALID_DIV_ID_IN_LINK_TYPE;

		mux_link_type2 = (ccu_mux_link_type2_t *)cm;
		*div = (ccu_div_base_t *)&mux_link_type2->div;
		return 0;
	}

	if (link_type == CCU_MUX_LINK_TYPE4)
	{
		mux_link_type4 = (ccu_mux_link_type4_t *)cm;
		if (div_id == 0)
		{
			*div = (ccu_div_base_t *)&mux_link_type4->div1;
		}
		else if (div_id == 1)
		{
			*div = (ccu_div_base_t *)&mux_link_type4->div2;
		}
		else
		{
			return CLI_RET_INVALID_DIV_ID_IN_LINK_TYPE;
		}

		return 0;
	}

	return CLK_RET_LINK_TYPE_NOT_SUPPORTED;
}

static int mux_link_set_div_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t freq)
{
	int ret;
	unsigned long flags;
	uint8_t link_type;
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);
	ccu_mux_t *cm = hw_to_ccu_mux(hw);
	ccu_mux_base_t *mux = &cm->mux;
	ccu_mux_base_with_update_bit_t *mux_var1;
	ccu_div_base_t *div1 = NULL, *div2 = NULL;
	clk_controller_t *controller;

	link_type = ccu_mux_get_link_type(cm);

	ret = mux_link_get_ccu_div(ccu_clk_hw, 0, &div1);
	if (ret)
	{
		return ret;
	}

	if (link_type == CCU_MUX_LINK_TYPE4)
	{
		ret = mux_link_get_ccu_div(ccu_clk_hw, 1, &div2);
		if (ret)
			return ret;
	}

	ret = clk_hw_get_controller(hw, &controller);
	if (ret)
		return ret;
	
	flags = hal_spin_lock_irqsave(&controller->lock);

	if (link_type == CCU_MUX_LINK_TYPE4)
		ret = ccu_div_link_div_set_freq_without_lock(ccu_clk_hw, div1, div2, parent_freq, freq);
	else
		ret = ccu_div_set_freq_without_lock(ccu_clk_hw, div1, parent_freq, freq);

	if (mux->var_type == CCU_MUX_VAR_TYPE1)
	{
		mux_var1 = (ccu_mux_base_with_update_bit_t *)mux;
		ret = ccu_clk_hw_set_control_bit_without_lock(ccu_clk_hw,
			mux_var1->update_bit_offset, CCU_MUX_CLK_HW_VALIDATE_CFG_VALUE);
	}
	hal_spin_unlock_irqrestore(&controller->lock, flags);
	return ret;

}

static int mux_link_get_div_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq)
{
	int ret;
	uint8_t link_type;
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);
	ccu_mux_t *cm = hw_to_ccu_mux(hw);
	ccu_div_base_t *div1 = NULL, *div2 = NULL;

	link_type = ccu_mux_get_link_type(cm);

	ret = mux_link_get_ccu_div(ccu_clk_hw, 0, &div1);
	if (ret)
	{
		if (ret == CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE)
		{
			*freq = parent_freq;
			return 0;
		}

		return ret;
	}

	if (link_type == CCU_MUX_LINK_TYPE4)
	{
		ret = mux_link_get_ccu_div(ccu_clk_hw, 1, &div2);
		if (ret)
			return ret;
	}

	if (link_type == CCU_MUX_LINK_TYPE4)
		ret = ccu_div_link_div_get_freq_without_lock(ccu_clk_hw, div1, div2, parent_freq, freq);
	else
		ret = ccu_div_get_freq_without_lock(ccu_clk_hw, div1, parent_freq, freq);

	return ret;
}

static int mux_link_round_div_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq)
{
	int ret;
	uint8_t link_type;
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);
	ccu_mux_t *cm = hw_to_ccu_mux(hw);
	ccu_div_base_t *div1 = NULL, *div2 = NULL;

	link_type = ccu_mux_get_link_type(cm);

	ret = mux_link_get_ccu_div(ccu_clk_hw, 0, &div1);
	if (ret)
	{
		if (ret == CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE)
		{
			*freq = parent_freq;
			return 0;
		}

		return ret;
	}

	if (link_type == CCU_MUX_LINK_TYPE4)
	{
		ret = mux_link_get_ccu_div(ccu_clk_hw, 1, &div2);
		if (ret)
			return ret;
	}

	if (link_type == CCU_MUX_LINK_TYPE4)
		ret = ccu_div_link_div_round_freq_without_lock(ccu_clk_hw, div1, div2, parent_freq, freq);
	else
		ret = ccu_div_round_freq_without_lock(ccu_clk_hw, div1, parent_freq, freq);

	return ret;
}

static int mux_link_set_gate_enable_state(struct clk_hw *hw, int is_enabled)
{
	int ret;
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);
	uint8_t gate_bit_offset = GATE_ABSENT_BIT_OFFSET;

	ret = mux_link_get_gate_bit_offset(ccu_clk_hw, &gate_bit_offset);
	if (ret)
	{
		if (ret == CLK_RET_CLK_HW_NOT_EXIST_IN_LINK_TYPE)
			return 0;

		return ret;
	}

	return ccu_gate_set_enable_state_with_lock(ccu_clk_hw, gate_bit_offset, is_enabled);
}

static int mux_link_get_gate_enable_state(struct clk_hw *hw, int *is_enabled)
{
	int ret;
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);
	uint8_t gate_bit_offset = GATE_ABSENT_BIT_OFFSET;

	ret = mux_link_get_gate_bit_offset(ccu_clk_hw, &gate_bit_offset);
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

const struct clk_hw_ops g_ccu_mux_hw_ops =
{
	.init = mux_init,

	.set_enable_state = mux_link_set_gate_enable_state,
	.get_enable_state = mux_link_get_gate_enable_state,

	.set_freq = mux_link_set_div_freq,
	.get_freq = mux_link_get_div_freq,
	.round_freq = mux_link_round_div_freq,

	.get_parent = mux_get_parent,
	.set_parent = mux_set_parent,
};

