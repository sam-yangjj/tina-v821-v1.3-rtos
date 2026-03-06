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

#ifndef _CCU_MUX_H_
#define _CCU_MUX_H_

#include "aw_ccu.h"
#include "ccu_div.h"

/*
 * currently placing the link type field in ccu_mux_base_t type can save 4 byte
 * for every ccu_mux_t object in 32bit platform
 */
#define SAVE_CCU_MUX_STRUCT_MEM


#define MAX_MUX_CTRL_FIELD_VALUE (255)
#define MAX_MUX_PARENT_NUM (MAX_MUX_CTRL_FIELD_VALUE + 1)

#define CCU_MUX_CLK_HW_VALIDATE_CFG_VALUE CCU_CLK_HW_VALIDATE_CFG_VALUE

#define MUX_CTRL_BIT_ABSENT_BIT_OFFSET CTRL_BIT_ABSENT_BIT_OFFSET

enum ccu_mux_var_type
{
	CCU_MUX_VAR_TYPE0 = 0, /* default variant */
	CCU_MUX_VAR_TYPE1, /* mux with update bit */
};

typedef struct ccu_mux_base
{
#ifdef SAVE_CCU_MUX_STRUCT_MEM
	uint8_t link_type : 4;
	uint8_t var_type : 4;
#else
	uint8_t var_type;
#endif

	uint8_t field_offset;
	uint8_t field_width;
} ccu_mux_base_t;

typedef struct ccu_mux_base_with_update_bit
{
#ifdef SAVE_CCU_MUX_STRUCT_MEM
	uint8_t link_type : 4;
	uint8_t var_type : 4;
#else
	uint8_t var_type;
#endif

	uint8_t field_offset;
	uint8_t field_width;

	uint8_t update_bit_offset;
} ccu_mux_base_with_update_bit_t;

/* parent class */
typedef ccu_mux_base_t ccu_mux_base_min_t;
/* child class */
typedef ccu_mux_base_with_update_bit_t ccu_mux_base_max_t;


#ifdef SAVE_CCU_MUX_STRUCT_MEM
#define CCU_MUX_BASE(_var_type, _link_type, _field_offset, _field_width) \
{ \
	.link_type = _link_type, \
	.var_type = _var_type, \
	.field_offset = _field_offset, \
	.field_width = _field_width, \
}

#define CCU_MUX_BASE_WITH_UPDATE_BIT(_var_type, _link_type, _field_offset, _field_width, \
	_update_bit_offset) \
{ \
	.link_type = _link_type, \
	.var_type = _var_type, \
	.field_offset = _field_offset, \
	.field_width = _field_width, \
	.update_bit_offset = _update_bit_offset, \
}
#else
#define CCU_MUX_BASE(_var_type, _link_type, _field_offset, _field_width) \
{ \
	.var_type = _var_type, \
	.field_offset = _field_offset, \
	.field_width = _field_width, \
}, \
.link_type = _link_type

#define CCU_MUX_BASE_WITH_UPDATE_BIT(_var_type, _link_type, _field_offset, _field_width, \
		_update_bit_offset) \
{ \
	.var_type = _var_type, \
	.field_offset = _field_offset, \
	.field_width = _field_width, \
	.update_bit_offset = _update_bit_offset, \
}, \
.link_type = _link_type
#endif

#define CCU_MUX_BASE_VARIANT0(_link_type, _field_offset, _field_width) \
	CCU_MUX_BASE(CCU_MUX_VAR_TYPE0, _link_type, _field_offset, _field_width)

enum ccu_mux_link_type
{
	CCU_MUX_LINK_TYPE0 = 0, /* mux link no other hw */
	CCU_MUX_LINK_TYPE1, /* mux link gate */
	CCU_MUX_LINK_TYPE2, /* mux link divider */
	CCU_MUX_LINK_TYPE3, /* mux link divider and link gate */
	CCU_MUX_LINK_TYPE4, /* mux link 2 divider and link gate */
};

typedef struct ccu_mux
{
	struct aw_ccu_clk_hw ccu_clk_hw;
#ifndef SAVE_CCU_MUX_STRUCT_MEM
	uint8_t link_type;
#endif
	ccu_mux_base_min_t mux;
} ccu_mux_t;

typedef struct ccu_mux_with_update_bit
{
	struct aw_ccu_clk_hw ccu_clk_hw;
#ifndef SAVE_CCU_MUX_STRUCT_MEM
	uint8_t link_type;
#endif
	ccu_mux_base_with_update_bit_t mux;
} ccu_mux_with_update_bit_t;

typedef ccu_mux_t ccu_mux_link_type0_t;
typedef ccu_mux_with_update_bit_t ccu_mux_with_update_bit_link_type0_t;

typedef struct ccu_mux_link_type1
{
	struct aw_ccu_clk_hw ccu_clk_hw;
	/* link_type or mux must be second member in any ccu_mux_link_type struct */
#ifndef SAVE_CCU_MUX_STRUCT_MEM
	uint8_t link_type;
#endif
	ccu_mux_base_max_t mux;
	uint8_t gate_bit_offset;
} ccu_mux_link_type1_t;

typedef struct ccu_mux_link_type2
{
	struct aw_ccu_clk_hw ccu_clk_hw;
#ifndef SAVE_CCU_MUX_STRUCT_MEM
	uint8_t link_type;
#endif
	ccu_mux_base_max_t mux;
	ccu_div_base_max_t div;
} ccu_mux_link_type2_t;

typedef struct ccu_mux_link_type3
{
	struct aw_ccu_clk_hw ccu_clk_hw;
#ifndef SAVE_CCU_MUX_STRUCT_MEM
	uint8_t link_type;
#endif
	ccu_mux_base_max_t mux;
	ccu_div_base_max_t div;
	uint8_t gate_bit_offset;
} ccu_mux_link_type3_t;

typedef struct ccu_mux_link_type4
{
	struct aw_ccu_clk_hw ccu_clk_hw;
#ifndef SAVE_CCU_MUX_STRUCT_MEM
	uint8_t link_type;
#endif
	ccu_mux_base_max_t mux;
	ccu_div_base_max_t div1;
	ccu_div_base_max_t div2;
	uint8_t gate_bit_offset;
} ccu_mux_link_type4_t;

#define AW_CCU_MUX_LINK_TYPE0_FULL(_struct_name, _clk_name, _flags, _parents, \
			_reg_offset, _mux_var_type, _mux_field_offset, _mux_field_width) \
ccu_mux_link_type0_t _struct_name = \
{ \
	.mux = CCU_MUX_BASE(_mux_var_type, CCU_MUX_LINK_TYPE0, \
		_mux_field_offset, _mux_field_width), \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = MUX_CLK_HW_INFO(_clk_name, _parents, &g_ccu_mux_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_MUX_WITH_UPDATE_BIT_LINK_TYPE0_FULL(_struct_name, _clk_name, _flags, _parents, \
			_reg_offset, _mux_var_type, _mux_field_offset, _mux_field_width, _update_bit_offset) \
ccu_mux_with_update_bit_link_type0_t _struct_name = \
{ \
	.mux = CCU_MUX_BASE_WITH_UPDATE_BIT(_mux_var_type, CCU_MUX_LINK_TYPE0, \
		_mux_field_offset, _mux_field_width, _update_bit_offset), \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = MUX_CLK_HW_INFO(_clk_name, _parents, &g_ccu_mux_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_MUX_BASE_MAX_LINK_TYPE1_FULL(_struct_name, _clk_name, _flags, _parents, \
			_reg_offset, _mux_var_type, _mux_field_offset, _mux_field_width, _update_bit_offset, \
			_gate_bit_offset) \
ccu_mux_link_type1_t _struct_name = \
{ \
	.mux = CCU_MUX_BASE_WITH_UPDATE_BIT(_mux_var_type, CCU_MUX_LINK_TYPE1, \
		_mux_field_offset, _mux_field_width, _update_bit_offset), \
	.gate_bit_offset = _gate_bit_offset, \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = MUX_CLK_HW_INFO(_clk_name, _parents, &g_ccu_mux_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_MUX_BASE_MAX_LINK_TYPE2_FULL(_struct_name, _clk_name, _flags, _parents, \
			_reg_offset, _mux_var_type, _mux_field_offset, _mux_field_width, _update_bit_offset, \
			_div_var_type, _div_field_offset, _div_field_width, _div_map_table) \
ccu_mux_link_type2_t _struct_name = \
{ \
	.mux = CCU_MUX_BASE_WITH_UPDATE_BIT(_mux_var_type, CCU_MUX_LINK_TYPE2, \
		_mux_field_offset, _mux_field_width, _update_bit_offset), \
	.div = CCU_DIV_BASE_WITH_MAP(_div_var_type, CCU_DIV_LINK_TYPE0, \
		_div_field_offset, _div_field_width, _div_map_table), \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = MUX_CLK_HW_INFO(_clk_name, _parents, &g_ccu_mux_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_MUX_BASE_MAX_LINK_TYPE3_FULL(_struct_name, _clk_name, _flags, _parents, \
			_reg_offset, _mux_var_type, _mux_field_offset, _mux_field_width, _update_bit_offset, \
			_div_var_type, _div_field_offset, _div_field_width, _div_map_table, \
			_gate_bit_offset) \
ccu_mux_link_type3_t _struct_name = \
{ \
	.mux = CCU_MUX_BASE_WITH_UPDATE_BIT(_mux_var_type, CCU_MUX_LINK_TYPE3, \
		_mux_field_offset, _mux_field_width, _update_bit_offset), \
	.div = CCU_DIV_BASE_WITH_MAP(_div_var_type, CCU_DIV_LINK_TYPE0, \
		_div_field_offset, _div_field_width, _div_map_table), \
	.gate_bit_offset = _gate_bit_offset, \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = MUX_CLK_HW_INFO(_clk_name, _parents, &g_ccu_mux_hw_ops, _flags) \
		} \
	} \
}

#define AW_CCU_MUX_BASE_MAX_LINK_TYPE4_FULL(_struct_name, _clk_name, _flags, _parents, \
			_reg_offset, _mux_var_type, _mux_field_offset, _mux_field_width, _update_bit_offset, \
			_div1_var_type, _div1_field_offset, _div1_field_width, _div1_map_table, \
			_div2_var_type, _div2_field_offset, _div2_field_width, _div2_map_table, \
			_gate_bit_offset) \
ccu_mux_link_type4_t _struct_name = \
{ \
	.mux = CCU_MUX_BASE_WITH_UPDATE_BIT(_mux_var_type, CCU_MUX_LINK_TYPE4, \
		_mux_field_offset, _mux_field_width, _update_bit_offset), \
	.div1 = CCU_DIV_BASE_WITH_MAP(_div1_var_type, CCU_DIV_LINK_TYPE0, \
		_div1_field_offset, _div1_field_width, _div1_map_table), \
	.div2 = CCU_DIV_BASE_WITH_MAP(_div2_var_type, CCU_DIV_LINK_TYPE0, \
		_div2_field_offset, _div2_field_width, _div2_map_table), \
	.gate_bit_offset = _gate_bit_offset, \
	.ccu_clk_hw = \
	{ \
		.reg_offset = _reg_offset, \
		.features = 0, \
		.hw = \
		{ \
			.info = MUX_CLK_HW_INFO(_clk_name, _parents, &g_ccu_mux_hw_ops, _flags) \
		} \
	} \
}





#define AW_CCU_MUX(_struct_name, _clk_name, _flags, _parents, \
				_reg_offset, _field_offset, _field_width) \
	AW_CCU_MUX_LINK_TYPE0_FULL(_struct_name, _clk_name, _flags, _parents, \
	_reg_offset, CCU_MUX_VAR_TYPE0, _field_offset, _field_width)

#define AW_CCU_MUX_WITH_UPD(_struct_name, _clk_name, _flags, _parents, \
				_reg_offset, _field_offset, _field_width, _update_bit_offset) \
	AW_CCU_MUX_WITH_UPDATE_BIT_LINK_TYPE0_FULL(_struct_name, _clk_name, _flags, _parents, \
	_reg_offset, CCU_MUX_VAR_TYPE1, _field_offset, _field_width, _update_bit_offset)


#define AW_CCU_MUX_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE1_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		_gate_bit_offset)

#define AW_CCU_MUX_WITH_UPD_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE1_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_gate_bit_offset)


#define AW_CCU_MUX_LINK_LDIV(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div_field_offset, _div_field_width) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE2_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE0, _div_field_offset, _div_field_width, NULL)

#define AW_CCU_MUX_LINK_PDIV(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div_field_offset, _div_field_width) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE2_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE1, _div_field_offset, _div_field_width, NULL)

#define AW_CCU_MUX_LINK_TDIV(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div_field_offset, _div_field_width, _div_map_table) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE2_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE2, _div_field_offset, _div_field_width, _div_map_table)

#define AW_CCU_MUX_WITH_UPD_LINK_LDIV(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div_field_offset, _div_field_width) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE2_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE0, _div_field_offset, _div_field_width, NULL)

#define AW_CCU_MUX_WITH_UPD_LINK_PDIV(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div_field_offset, _div_field_width) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE2_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE1, _div_field_offset, _div_field_width, NULL)

#define AW_CCU_MUX_WITH_UPD_LINK_TDIV(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div_field_offset, _div_field_width, _div_map_table) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE2_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE2, _div_field_offset, _div_field_width, _div_map_table)


#define AW_CCU_MUX_LINK_LDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div_field_offset, _div_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE3_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE0, _div_field_offset, _div_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_LINK_PDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div_field_offset, _div_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE3_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE1, _div_field_offset, _div_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_LINK_TDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div_field_offset, _div_field_width, _div_map_table, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE3_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE2, _div_field_offset, _div_field_width, _div_map_table, \
		_gate_bit_offset)

#define AW_CCU_MUX_WITH_UPD_LINK_LDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div_field_offset, _div_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE3_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE0, _div_field_offset, _div_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_WITH_UPD_LINK_PDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div_field_offset, _div_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE3_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE1, _div_field_offset, _div_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_WITH_UPD_LINK_TDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div_field_offset, _div_field_width, _div_map_table, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE3_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE2, _div_field_offset, _div_field_width, _div_map_table, \
		_gate_bit_offset)


#define AW_CCU_MUX_LINK_LDIV_LDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div1_field_offset, _div1_field_width, \
		_div2_field_offset, _div2_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE4_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE0, _div1_field_offset, _div1_field_width, NULL, \
		CCU_DIV_VAR_TYPE0, _div2_field_offset, _div2_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_LINK_LDIV_PDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div1_field_offset, _div1_field_width, \
		_div2_field_offset, _div2_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE4_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE0, _div1_field_offset, _div1_field_width, NULL, \
		CCU_DIV_VAR_TYPE1, _div2_field_offset, _div2_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_LINK_PDIV_PDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div1_field_offset, _div1_field_width, \
		_div2_field_offset, _div2_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE4_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE1, _div1_field_offset, _div1_field_width, NULL, \
		CCU_DIV_VAR_TYPE1, _div2_field_offset, _div2_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_LINK_PDIV_LDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, \
		_div1_field_offset, _div1_field_width, \
		_div2_field_offset, _div2_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE4_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE0, _mux_field_offset, _mux_field_width, MUX_CTRL_BIT_ABSENT_BIT_OFFSET, \
		CCU_DIV_VAR_TYPE1, _div1_field_offset, _div1_field_width, NULL, \
		CCU_DIV_VAR_TYPE0, _div2_field_offset, _div2_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_WITH_UPD_LINK_LDIV_LDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div1_field_offset, _div1_field_width, \
		_div2_field_offset, _div2_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE4_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE0, _div1_field_offset, _div1_field_width, NULL, \
		CCU_DIV_VAR_TYPE0, _div2_field_offset, _div2_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_WITH_UPD_LINK_LDIV_PDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div1_field_offset, _div1_field_width, \
		_div2_field_offset, _div2_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE4_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE0, _div1_field_offset, _div1_field_width, NULL, \
		CCU_DIV_VAR_TYPE1, _div2_field_offset, _div2_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_WITH_UPD_LINK_PDIV_PDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div1_field_offset, _div1_field_width, \
		_div2_field_offset, _div2_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE4_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE1, _div1_field_offset, _div1_field_width, NULL, \
		CCU_DIV_VAR_TYPE1, _div2_field_offset, _div2_field_width, NULL, \
		_gate_bit_offset)

#define AW_CCU_MUX_WITH_UPD_LINK_PDIV_LDIV_LINK_GATE(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		_div1_field_offset, _div1_field_width, \
		_div2_field_offset, _div2_field_width, \
		_gate_bit_offset) \
	AW_CCU_MUX_BASE_MAX_LINK_TYPE4_FULL(_struct_name, _clk_name, _flags, _parents, \
		_reg_offset, CCU_MUX_VAR_TYPE1, _mux_field_offset, _mux_field_width, _update_bit_offset, \
		CCU_DIV_VAR_TYPE1, _div1_field_offset, _div1_field_width, NULL, \
		CCU_DIV_VAR_TYPE0, _div2_field_offset, _div2_field_width, NULL, \
		_gate_bit_offset)

extern const struct clk_hw_ops g_ccu_mux_hw_ops;

int ccu_mux_init(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mux_base_t *mux);
int ccu_mux_set_parent(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mux_base_t *mux, u8 parent_index);
int ccu_mux_get_parent(aw_ccu_clk_hw_t *ccu_clk_hw, ccu_mux_base_t *mux, uint8_t *index);

#endif /* _CCU_MUX_H_ */
