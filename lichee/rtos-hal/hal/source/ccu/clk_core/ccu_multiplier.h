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

#ifndef _CCU_MULT_H_
#define _CCU_MULT_H_

#include "aw_ccu.h"

/* basic multiplier, it has no other control function except multiple */
enum ccu_mult_var_type
{
	CCU_MULT_VAR_TYPE0 = 0, /* default variant, multiplier value = field value + 1, linear with minimum value 1 */
	CCU_MULT_VAR_TYPE1, /* variant type1, multiplier value = field value, linear with minimum value 0 */
};

typedef struct ccu_mult_base
{
	uint8_t var_type;
	u8 field_offset;
	u8 field_width;
} ccu_mult_base_t;

#define CCU_MULT_BASE(_var_type, _field_offset, _field_width) \
{ \
	.var_type = _var_type, \
	.field_offset = _field_offset, \
	.field_width = _field_width, \
}

typedef struct ccu_mult
{
	struct aw_ccu_clk_hw ccu_clk_hw;
	ccu_mult_base_t mult;
} ccu_mult_t;

#define AW_CCU_MULT(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _field_offset, _field_width) \
struct ccu_mult _struct_name = \
{ \
	.mult = \
	{ \
		.field_offset = _field_offset, \
		.field_width = _field_width, \
	}, \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = CLK_HW_INFO(_clk_name, _parent, &g_ccu_mult_hw_ops, _flags) \
		} \
	} \
}


extern const struct clk_hw_ops g_ccu_mult_hw_ops;



int ccu_mult_init(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mult_base_t *mult);

int ccu_mult_set_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mult_base_t *mult,
					  uint32_t parent_freq, uint32_t freq, int is_need_lock);

static inline int ccu_mult_set_freq_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_mult_base_t *mult,
					uint32_t parent_freq, uint32_t freq)
{
	return ccu_mult_set_freq(ccu_clk_hw, mult, parent_freq, freq, 1);
}

static inline int ccu_mult_set_freq_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_mult_base_t *mult,
					uint32_t parent_freq, uint32_t freq)
{
	return ccu_mult_set_freq(ccu_clk_hw, mult, parent_freq, freq, 0);
}

int ccu_mult_get_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mult_base_t *mult,
					  uint32_t parent_freq, uint32_t *freq, int is_need_lock);

static inline int ccu_mult_get_freq_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_mult_base_t *mult,
					uint32_t parent_freq, uint32_t *freq)
{
	return ccu_mult_get_freq(ccu_clk_hw, mult, parent_freq, freq, 1);
}

static inline int ccu_mult_get_freq_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_mult_base_t *mult,
					uint32_t parent_freq, uint32_t *freq)
{
	return ccu_mult_get_freq(ccu_clk_hw, mult, parent_freq, freq, 0);
}

int ccu_mult_round_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mult_base_t *mult,
						uint32_t parent_freq, uint32_t *freq, int is_need_lock);

static inline int ccu_mult_round_freq_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_mult_base_t *mult,
					uint32_t parent_freq, uint32_t *freq)
{
	return ccu_mult_round_freq(ccu_clk_hw, mult, parent_freq, freq, 1);
}

static inline int ccu_mult_round_freq_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_mult_base_t *mult,
					uint32_t parent_freq, uint32_t *freq)
{
	return ccu_mult_round_freq(ccu_clk_hw, mult, parent_freq, freq, 0);
}

#endif /* _CCU_MULT_H_ */
