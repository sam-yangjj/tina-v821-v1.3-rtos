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

#ifndef _CCU_GATE_H_
#define _CCU_GATE_H_

#include "aw_ccu.h"

#define MAX_GATE_CONTROL_BIT_OFFSET MAX_CONTROL_BIT_OFFSET
#define GATE_ABSENT_BIT_OFFSET CTRL_BIT_ABSENT_BIT_OFFSET

/* currently not using */
enum ccu_gate_var_type
{
	CCU_GATE_VAR_TYPE1 = 1,
	CCU_GATE_VAR_TYPE2,
};

typedef struct ccu_gate
{
	struct aw_ccu_clk_hw ccu_clk_hw;
	uint8_t var_type; /* currently not using */
	uint8_t bit_offset;
} ccu_gate_t;

#define AW_CCU_GATE(_struct_name, _clk_name, _flags, _parent, _reg_offset, _gate_bit_offset) \
struct ccu_gate _struct_name = \
{ \
	.bit_offset = _gate_bit_offset, \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = CLK_HW_INFO(_clk_name, _parent, &g_ccu_gate_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_GATE2(_struct_name, _clk_name, _flags, _parent, _reg_offset, _gate_bit_offset) \
struct ccu_gate _struct_name = \
{ \
	.bit_offset = _gate_bit_offset, \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = \
			{				\
				.flags		= _flags,			\
				.name	  = _clk_name,			  \
				.parents  =  (const clk_number_t []){ _parent }, \
				.num_parents	= 1,				\
				.ops		  = &g_ccu_gate_hw_ops,			  \
			}\
		} \
	} \
}

int ccu_gate_init(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t gate_bit_offset);

static inline int ccu_gate_set_enable_state_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t gate_bit_offset, int is_enabled)
{
	return ccu_clk_hw_set_control_bit(ccu_clk_hw, gate_bit_offset, is_enabled, 1);
}

static inline int ccu_gate_set_enable_state_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t gate_bit_offset, int is_enabled)
{
	return ccu_clk_hw_set_control_bit(ccu_clk_hw, gate_bit_offset, is_enabled, 0);
}

static inline int ccu_gate_get_enable_state_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t gate_bit_offset, int *is_enabled)
{
	return ccu_clk_hw_get_control_bit(ccu_clk_hw, gate_bit_offset, is_enabled, 1);
}

static inline int ccu_gate_get_enable_state_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t gate_bit_offset, int *is_enabled)
{
	return ccu_clk_hw_get_control_bit(ccu_clk_hw, gate_bit_offset, is_enabled, 0);
}

extern const struct clk_hw_ops g_ccu_gate_hw_ops;

#endif /* _CCU_GATE_H_ */
