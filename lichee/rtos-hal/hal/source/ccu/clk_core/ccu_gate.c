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
#include "ccu_gate.h"

static inline struct ccu_gate *hw_to_ccu_gate(struct clk_hw *hw)
{
	struct aw_ccu_clk_hw *ccu_clk_hw = hw_to_ccu_clk_hw(hw);

	return container_of(ccu_clk_hw, struct ccu_gate, ccu_clk_hw);
}

int ccu_gate_init(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t gate_bit_offset)
{
	if ((gate_bit_offset > MAX_GATE_CONTROL_BIT_OFFSET)
		&& (gate_bit_offset != GATE_ABSENT_BIT_OFFSET))
	{
		clk_err("invalid gate bit offset: %u", gate_bit_offset);
		return CLK_RET_INVALID_GATE_BIT_OFFSET;
	}

	return 0;
}

static int gate_set_enable_state(struct clk_hw *hw, int is_enabled)
{
	struct ccu_gate *cg = hw_to_ccu_gate(hw);
	return ccu_gate_set_enable_state_with_lock(&cg->ccu_clk_hw, cg->bit_offset, is_enabled);
}

static int gate_gate_get_enable_state(struct clk_hw *hw, int *is_enabled)
{
	struct ccu_gate *cg = hw_to_ccu_gate(hw);
	return ccu_gate_get_enable_state_without_lock(&cg->ccu_clk_hw, cg->bit_offset, is_enabled);
}

int gate_init(struct clk_hw *hw)
{
	aw_ccu_clk_hw_t *ccu_clk_hw = hw_to_ccu_clk_hw(hw);
	struct ccu_gate *cg = hw_to_ccu_gate(hw);

	return ccu_gate_init(ccu_clk_hw, cg->bit_offset);
}

const struct clk_hw_ops g_ccu_gate_hw_ops =
{
	.init = gate_init,
	.set_enable_state = gate_set_enable_state,
	.get_enable_state = gate_gate_get_enable_state,
};
