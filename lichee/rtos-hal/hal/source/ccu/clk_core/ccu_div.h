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

#ifndef _CCU_DIV_H_
#define _CCU_DIV_H_

#include "aw_ccu.h"

/*
 * currently placing the link type field in ccu_div_base_t type can save 4 byte
 * for every ccu_div_t or other link type object in 32bit platform
 */
#define SAVE_CCU_DIV_STRUCT_MEM

enum ccu_div_var_type
{
	CCU_DIV_VAR_TYPE0 = 0, /* default variant, divider value = field value + 1, linear divider */
	CCU_DIV_VAR_TYPE1, /* variant type1, divider value = 2 ^ field value, power divider */
	CCU_DIV_VAR_TYPE2, /* variant type2, has division map table, table divider */
	CCU_DIV_VAR_TYPE_MAX,
};

/*
Currently ccu divider variant 1 has the map, but not only variant 1 can have map in the future.
So there is no one-to-one correspondence between the ccu_div_base_xxx type and variant x.
For example, ccu_div_base_t can be ccu divider variant type 0 and ccu divider variant type 1.
*/
typedef struct ccu_div_base
{
#ifdef SAVE_CCU_DIV_STRUCT_MEM
	uint8_t link_type;
	uint8_t var_type;
#else
	uint8_t var_type;
#endif

	uint8_t field_offset;
	uint8_t field_width;
} ccu_div_base_t;

typedef struct ccu_div_base_with_map
{
#ifdef SAVE_CCU_DIV_STRUCT_MEM
	uint8_t link_type;
	uint8_t var_type;
#else
	uint8_t var_type;
#endif

	uint8_t field_offset;
	uint8_t field_width;

	const division_map_t *map_table;
} ccu_div_base_with_map_t;

/* parent class */
typedef ccu_div_base_t ccu_div_base_min_t;
/* child class */
typedef ccu_div_base_with_map_t ccu_div_base_max_t;

#ifdef SAVE_CCU_DIV_STRUCT_MEM
#define CCU_DIV_BASE(_var_type, _link_type, _field_offset, _field_width) \
{ \
	.link_type = _link_type, \
	.var_type = _var_type, \
	.field_offset = _field_offset, \
	.field_width = _field_width, \
}

#define CCU_DIV_BASE_WITH_MAP(_var_type, _link_type, _field_offset, _field_width, _div_map_table) \
{ \
	.link_type = _link_type, \
	.var_type = _var_type, \
	.field_offset = _field_offset, \
	.field_width = _field_width, \
	.map_table = _div_map_table, \
}
#else
#define CCU_DIV_BASE(_var_type, _link_type, _field_offset, _field_width) \
{ \
	.var_type = _var_type, \
	.field_offset = _field_offset, \
	.field_width = _field_width, \
} \
.link_type = _link_type

#define CCU_DIV_BASE_WITH_MAP(_var_type, _link_type, _field_offset, _field_width, _div_map_table) \
{ \
	.var_type = _var_type, \
	.field_offset = _field_offset, \
	.field_width = _field_width, \
	.map_table = _div_map_table, \
} \
.link_type = _link_type
#endif

enum ccu_div_link_type
{
	CCU_DIV_LINK_TYPE0 = 0, /* div link no other hw */
	CCU_DIV_LINK_TYPE1, /* div link gate */
};

typedef struct ccu_div
{
	struct aw_ccu_clk_hw ccu_clk_hw;
#ifndef SAVE_CCU_DIV_STRUCT_MEM
	uint8_t link_type;
#endif
	struct ccu_div_base div;
} ccu_div_t;

typedef struct ccu_div_with_map
{
	struct aw_ccu_clk_hw ccu_clk_hw;
#ifndef SAVE_CCU_DIV_STRUCT_MEM
	uint8_t link_type;
#endif
	ccu_div_base_with_map_t div;
} ccu_div_with_map_t;

typedef ccu_div_t ccu_div_link_type0_t;
typedef ccu_div_with_map_t ccu_div_with_map_link_type0_t;

typedef struct ccu_div_link_type1
{
	struct aw_ccu_clk_hw ccu_clk_hw;
	/* link_type or div must be second member in any ccu_div_link_type struct */
#ifndef SAVE_CCU_DIV_STRUCT_MEM
	uint8_t link_type;
#endif
	ccu_div_base_max_t div;
	uint8_t gate_bit_offset;
} ccu_div_link_type1_t;

#define AW_CCU_DIV_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _div_var_type, _div_field_offset, _div_field_width) \
ccu_div_link_type0_t _struct_name = \
{ \
	.div = CCU_DIV_BASE(_div_var_type, CCU_DIV_LINK_TYPE0, \
		_div_field_offset, _div_field_width), \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = CLK_HW_INFO(_clk_name, _parent, &g_ccu_div_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_DIV_WITH_TAB_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _div_var_type, _div_field_offset, _div_field_width, _div_map_table) \
ccu_div_with_map_link_type0_t _struct_name = \
{ \
	.div = CCU_DIV_BASE_WITH_MAP(_div_var_type, CCU_DIV_LINK_TYPE0, \
		_div_field_offset, _div_field_width, _div_map_table), \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = CLK_HW_INFO(_clk_name, _parent, &g_ccu_div_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_DIV_BASE_MAX_LINK_TYPE1_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _div_var_type, _div_field_offset, _div_field_width, _div_map_table, \
			_gate_bit_offset) \
ccu_div_link_type1_t _struct_name = \
{ \
	.div = CCU_DIV_BASE_WITH_MAP(_div_var_type, CCU_DIV_LINK_TYPE1, \
		_div_field_offset, _div_field_width, _div_map_table), \
	.gate_bit_offset = _gate_bit_offset, \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = CLK_HW_INFO(_clk_name, _parent, &g_ccu_div_hw_ops, _flags) \
		} \
	} \
}


#define AW_CCU_DIV_LINEAR(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _field_offset, _field_width) \
		AW_CCU_DIV_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_DIV_VAR_TYPE0, _field_offset, _field_width)

#define AW_CCU_DIV_POWER(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _field_offset, _field_width) \
		AW_CCU_DIV_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_DIV_VAR_TYPE1, _field_offset, _field_width)

#define AW_CCU_DIV_WITH_TABLE(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _field_offset, _field_width, _div_map_table) \
		AW_CCU_DIV_WITH_TAB_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_DIV_VAR_TYPE2, _field_offset, _field_width, _div_map_table)


#define AW_CCU_DIV_LINEAR_LINK_GATE(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _div_field_offset, _div_field_width, _gate_bit_offset) \
		AW_CCU_DIV_BASE_MAX_LINK_TYPE1_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_DIV_VAR_TYPE0, _div_field_offset, _div_field_width, NULL, \
			_gate_bit_offset)

#define AW_CCU_DIV_POWER_LINK_GATE(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _div_field_offset, _div_field_width, _gate_bit_offset) \
		AW_CCU_DIV_BASE_MAX_LINK_TYPE1_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_DIV_VAR_TYPE1, _div_field_offset, _div_field_width, NULL, \
			_gate_bit_offset)

#define AW_CCU_DIV_WITH_TABLE_LINK_GATE(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, _div_field_offset, _div_field_width, _div_map_table, \
			_gate_bit_offset) \
		AW_CCU_DIV_BASE_MAX_LINK_TYPE1_FULL(_struct_name, _clk_name, _flags, _parent, \
			_reg_offset, CCU_DIV_VAR_TYPE2, _div_field_offset, _div_field_width, _div_map_table, \
			_gate_bit_offset)

#define AW_CCU_DIV AW_CCU_DIV_LINEAR
#define AW_CCU_LDIV AW_CCU_DIV_LINEAR
#define AW_CCU_PDIV AW_CCU_DIV_POWER
#define AW_CCU_TDIV AW_CCU_DIV_WITH_TABLE

#define AW_CCU_DIV_LINK_GATE AW_CCU_DIV_LINEAR_LINK_GATE
#define AW_CCU_LDIV_LINK_GATE AW_CCU_DIV_LINEAR_LINK_GATE
#define AW_CCU_PDIV_LINK_GATE AW_CCU_DIV_POWER_LINK_GATE
#define AW_CCU_TDIV_LINK_GATE AW_CCU_DIV_WITH_TABLE_LINK_GATE


extern const clk_hw_ops_t g_ccu_div_hw_ops;

int ccu_div_init(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_div_base_t *div);

int ccu_div_set_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_div_base_t *div,
					 uint32_t parent_freq, uint32_t freq, int is_need_lock);

static inline int ccu_div_set_freq_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_div_base_t *div,
					uint32_t parent_freq, uint32_t freq)
{
	return ccu_div_set_freq(ccu_clk_hw, div, parent_freq, freq, 1);
}

static inline int ccu_div_set_freq_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_div_base_t *div,
					uint32_t parent_freq, uint32_t freq)
{
	return ccu_div_set_freq(ccu_clk_hw, div, parent_freq, freq, 0);
}

int ccu_div_get_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_div_base_t *div,
					 uint32_t parent_freq, uint32_t *freq, int is_need_lock);

static inline int ccu_div_get_freq_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_div_base_t *div,
					uint32_t parent_freq, uint32_t *freq)
{
	return ccu_div_get_freq(ccu_clk_hw, div, parent_freq, freq, 1);
}

static inline int ccu_div_get_freq_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_div_base_t *div,
					uint32_t parent_freq, uint32_t *freq)
{
	return ccu_div_get_freq(ccu_clk_hw, div, parent_freq, freq, 0);
}

int ccu_div_round_freq(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_div_base_t *div,
					   uint32_t parent_freq, uint32_t *freq, int is_need_lock);

static inline int ccu_div_round_freq_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_div_base_t *div,
					uint32_t parent_freq, uint32_t *freq)
{
	return ccu_div_round_freq(ccu_clk_hw, div, parent_freq, freq, 1);
}

static inline int ccu_div_round_freq_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
					ccu_div_base_t *div,
					uint32_t parent_freq, uint32_t *freq)
{
	return ccu_div_round_freq(ccu_clk_hw, div, parent_freq, freq, 0);
}

int ccu_div_link_div_set_freq(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					 uint32_t parent_freq, uint32_t freq, int is_need_lock);

static inline int ccu_div_link_div_set_freq_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					 uint32_t parent_freq, uint32_t freq)
{
	return ccu_div_link_div_set_freq(ccu_clk_hw, div1, div2, parent_freq, freq, 1);
}

static inline int ccu_div_link_div_set_freq_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					 uint32_t parent_freq, uint32_t freq)
{
	return ccu_div_link_div_set_freq(ccu_clk_hw, div1, div2, parent_freq, freq, 0);
}

int ccu_div_link_div_get_freq(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
	uint32_t parent_freq, uint32_t *freq, int is_need_lock);

static inline int ccu_div_link_div_get_freq_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					 uint32_t parent_freq, uint32_t *freq)
{
	return ccu_div_link_div_get_freq(ccu_clk_hw, div1, div2, parent_freq, freq, 1);
}

static inline int ccu_div_link_div_get_freq_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					 uint32_t parent_freq, uint32_t *freq)
{
	return ccu_div_link_div_get_freq(ccu_clk_hw, div1, div2, parent_freq, freq, 0);
}

int ccu_div_link_div_round_freq(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					   uint32_t parent_freq, uint32_t *freq, int is_need_lock);

static inline int ccu_div_link_div_round_freq_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					 uint32_t parent_freq, uint32_t *freq)
{
	return ccu_div_link_div_round_freq(ccu_clk_hw, div1, div2, parent_freq, freq, 1);
}

static inline int ccu_div_link_div_round_freq_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	const ccu_div_base_t *div1, const ccu_div_base_t *div2,
					 uint32_t parent_freq, uint32_t *freq)
{
	return ccu_div_link_div_round_freq(ccu_clk_hw, div1, div2, parent_freq, freq, 0);
}

#endif /* _CCU_DIV_H_ */
