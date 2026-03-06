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

static int fixed_factor_get_freq(clk_hw_t *hw, uint32_t parent_freq, uint32_t *freq)
{
	struct clk_fixed_factor *fixed = hw_to_clk_fixed_factor(hw);
	unsigned long long int tmp_freq;

	tmp_freq = (unsigned long long int)parent_freq * fixed->mult_val;
	tmp_freq /= fixed->div_val;

	if (tmp_freq > UINT32_MAX)
	{
		clk_err("%u.%u clk freq exceed upper limit %u, p_freq: %u, mult: %u, div: %u",
				hw->clk.cc_id, hw->clk.clk_id, (uint32_t)UINT32_MAX,
				parent_freq, fixed->mult_val, fixed->div_val);
		return CLK_RET_CLK_FREQ_EXCEED_UPPER_LIMIT;
	}

	*freq = tmp_freq;
	return 0;
}

static int fixed_factor_round_freq(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq)
{
	return fixed_factor_get_freq(hw, parent_freq, freq);
}

const struct clk_hw_ops g_clk_fixed_factor_ops =
{
	.get_freq = fixed_factor_get_freq,
	.round_freq = fixed_factor_round_freq,
};
