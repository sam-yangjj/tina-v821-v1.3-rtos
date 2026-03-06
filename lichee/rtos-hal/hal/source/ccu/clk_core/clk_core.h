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

#ifndef __CLK_CORE_H__
#define __CLK_CORE_H__

#include <limits.h>
#include <aw_common.h>
#include <hal_log.h>
#include <sunxi_hal_common.h>
#include <hal_atomic.h>

#include "ccu_reset.h"

#include "clk_common.h"

#define CLK_HW_INFO_STRUCT_HAS_NAME_MEMBER

//#define CLK_CORE_DEBUG

/* clk related module log macro */
#define CLK_LOG_COLOR_NONE "\e[0m"
#define CLK_LOG_COLOR_RED "\e[31m"
#define CLK_LOG_COLOR_GREEN "\e[32m"
#define CLK_LOG_COLOR_YELLOW "\e[33m"
#define CLK_LOG_COLOR_BLUE "\e[34m"

#define clk_printf printf

#ifdef CLK_CORE_DEBUG
#define clk_dbg_without_newline(fmt,...) \
			clk_printf(CLK_LOG_COLOR_BLUE "[CLK_D][%s:%d] " fmt \
				CLK_LOG_COLOR_NONE, __func__, __LINE__, ##__VA_ARGS__)

#define clk_dbg(fmt,...) clk_dbg_without_newline(fmt"\n", ##__VA_ARGS__)
#else
#define clk_dbg_without_newline(fmt,...)
#define clk_dbg(fmt, args...)
#endif /* CLK_DEBUG */

#define clk_info_without_newline(fmt,...) \
			clk_printf(CLK_LOG_COLOR_GREEN "[CLK_I][%s:%d] " fmt \
				CLK_LOG_COLOR_NONE, __func__, __LINE__, ##__VA_ARGS__)

#define clk_info(fmt,...) clk_info_without_newline(fmt"\n", ##__VA_ARGS__)

#define clk_warn_without_newline(fmt,...) \
			clk_printf(CLK_LOG_COLOR_YELLOW "[CLK_W][%s:%d] " fmt \
				CLK_LOG_COLOR_NONE, __func__, __LINE__, ##__VA_ARGS__)

#define clk_warn(fmt,...) clk_warn_without_newline(fmt"\n", ##__VA_ARGS__)

#define clk_err_without_newline(fmt,...) \
			clk_printf(CLK_LOG_COLOR_RED "[CLK_E][%s:%d] " fmt \
				CLK_LOG_COLOR_NONE, __func__, __LINE__, ##__VA_ARGS__)

#define clk_err(fmt,...) clk_err_without_newline(fmt"\n", ##__VA_ARGS__)


#ifdef CLK_HW_INFO_STRUCT_HAS_NAME_MEMBER
#define clk_hw_dbg(hw, fmt,...) \
			clk_dbg("['%s' (%u:%u)] " fmt, clk_hw_get_name((hw)), (hw)->clk.cc_id, (hw)->clk.clk_id, ##__VA_ARGS__)
#define clk_hw_info(hw, fmt,...) \
			clk_info("['%s' (%u:%u)] " fmt, clk_hw_get_name((hw)), (hw)->clk.cc_id, (hw)->clk.clk_id, ##__VA_ARGS__)
#define clk_hw_warn(hw, fmt,...) \
			clk_warn("['%s' (%u:%u)] " fmt, clk_hw_get_name((hw)), (hw)->clk.cc_id, (hw)->clk.clk_id, ##__VA_ARGS__)
#define clk_hw_err(hw, fmt,...) \
			clk_err("['%s' (%u:%u)] " fmt, clk_hw_get_name((hw)), (hw)->clk.cc_id, (hw)->clk.clk_id, ##__VA_ARGS__)
#else
#define clk_hw_dbg(hw, fmt,...) \
			clk_dbg("[%u:%u] "fmt, (hw)->clk.cc_id, (hw)->clk.clk_id, ##__VA_ARGS__)
#define clk_hw_info(hw, fmt, ...) \
			clk_info("[%u:%u] " fmt, (hw)->clk.cc_id, (hw)->clk.clk_id, ##__VA_ARGS__)
#define clk_hw_warn(hw, fmt,...) \
			clk_warn("[%u:%u] " fmt, (hw)->clk.cc_id, (hw)->clk.clk_id, ##__VA_ARGS__)
#define clk_hw_err(hw, fmt,...) \
			clk_err("[%u:%u] " fmt, (hw)->clk.cc_id, (hw)->clk.clk_id, ##__VA_ARGS__)
#endif

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

//#define CLK_HW_INFO_PROVIDE_NAME


//human

//#define DIV_ROUND_UP_ULL(n, d) (((n) + (d) - 1) / (d))

//#define DIV_ROUND_DOWN_ULL(ll, d)
//    ({ unsigned long long _tmp = (ll); do_div(_tmp, d); _tmp; })

#define DIV_ROUND_DOWN_ULL(ll, d) \
	((unsigned long long)(ll) / (d))

#define DIV_ROUND_UP_ULL(ll, d) \
	DIV_ROUND_DOWN_ULL((unsigned long long)(ll) + (d) - 1, (d))

#define DIV_ROUND_CLOSEST_ULL(x, divisor) \
	(((unsigned long long)(x) + (divisor) / 2 ) / (divisor))


/*
 * flags used across common struct clk.  these flags should only affect the
 * top-level framework.  custom flags for dealing with hardware specifics
 * belong in struct clk_foo
 *
 * Please update clk_flags[] in drivers/clk/clk.c when making changes here!
 */
#define CLK_SET_RATE_GATE   BIT(0) /* must be gated across rate change */
#define CLK_SET_PARENT_GATE BIT(1) /* must be gated across re-parent */
#define CLK_SET_RATE_PARENT BIT(2) /* propagate rate change up one level */
#define CLK_IGNORE_UNUSED   BIT(3) /* do not gate even if unused */
/* unused */
/* unused */
#define CLK_GET_RATE_NOCACHE    BIT(6) /* do not use the cached clk rate */
#define CLK_SET_RATE_NO_REPARENT BIT(7) /* don't re-parent on rate change */
#define CLK_GET_ACCURACY_NOCACHE BIT(8) /* do not use the cached clk accuracy */
#define CLK_RECALC_NEW_RATES    BIT(9) /* recalc rates after notifications */
#define CLK_SET_RATE_UNGATE BIT(10) /* clock needs to run to set rate */
#define CLK_IS_CRITICAL     BIT(11) /* do not gate, ever */
/* parents need enable during gate/ungate, set rate and re-parent */
#define CLK_OPS_PARENT_ENABLE   BIT(12)
/* duty cycle call may be forwarded to the parent clock */
#define CLK_DUTY_CYCLE_PARENT   BIT(13)
#define CLK_DONT_HOLD_STATE BIT(14) /* Don't hold state */

struct clk_hw;

typedef struct clk_hw_ops
{
	int (*init)(struct clk_hw *hw);

	int (*set_enable_state)(struct clk_hw *hw, int is_enabled);
	int (*get_enable_state)(struct clk_hw *hw, int *is_enabled);

	int (*set_parent)(struct clk_hw *hw, u8 index);
	int (*get_parent)(struct clk_hw *hw, uint8_t *index);

	int (*set_freq)(struct clk_hw *hw, uint32_t parent_freq, uint32_t freq);
	int (*get_freq)(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq);
	int (*round_freq)(struct clk_hw *hw, uint32_t parent_freq, uint32_t *freq);
} clk_hw_ops_t;

typedef struct clk_desc
{
	clk_id_t clk_id;
	clk_controller_id_t cc_id;
} clk_desc_t;

typedef uint32_t clk_number_t;

#define CLK_ID_BITS   16
#define CLK_ID_MASK   ((1U << CLK_ID_BITS) - 1)

#define CC_ID(clk_number)  ((unsigned int) ((clk_number) >> CLK_ID_BITS))
#define CLK_ID(clk_number)  ((unsigned int) ((clk_number) & CLK_ID_MASK))
#define MAKE_CLKn(cc_id, clk_id) (((cc_id) << CLK_ID_BITS) | (clk_id))

#define CLK_GLOBAL_ID_FMT_STR "%u:%u"

#define MAX_CLK_ID CLK_ID_MASK

#define MAX_CLK_ENABLE_CNT UINT8_MAX

typedef struct clk
{
	uint8_t enable_count;
	clk_controller_id_t cc_id;
	clk_id_t clk_id;
	struct clk *parent;
} clk_t;

typedef struct clk_hw_info
{
	uint8_t num_parents;
	uint16_t flags; /* framework level flags */
#ifdef CLK_HW_INFO_STRUCT_HAS_NAME_MEMBER
	const char *name;
#endif
	const struct clk_hw_ops *ops;
	const clk_number_t *parents;
} clk_hw_info_t;

typedef struct clk_hw
{
	clk_hw_info_t info;
	clk_t clk;
} clk_hw_t;

static inline clk_hw_t *clk_to_hw(clk_t *clk)
{
	return container_of(clk, clk_hw_t, clk);
}

struct clk_controller;
typedef struct clk_controller_ops
{
	int (*init)(struct clk_controller *controller);
	//int (*get_clk_hw)(clk_controller_t *controller, clk_hw_t *hw);
} clk_controller_ops_t;

typedef struct clk_controller
{
	clk_controller_id_t id;
	uint32_t clk_num;
	unsigned long reg_base;
	hal_spinlock_t lock;
	struct clk_hw **clk_hws;

	uint32_t reset_num;
	reset_hw_t *reset_hws;

	clk_controller_ops_t *ops;
} clk_controller_t;

typedef struct clk_fixed_factor
{
	struct clk_hw hw;
	uint16_t mult_val;
	uint16_t div_val;
} clk_fixed_factor_t;

extern const struct clk_hw_ops g_clk_fixed_factor_ops;

#define hw_to_clk_fixed_factor(_hw) container_of(_hw, struct clk_fixed_factor, hw)

struct clk_div_table
{
	uint32_t val;
	uint32_t div;
};

/* division value map table description */
typedef struct division_map
{
	uint16_t field_value;
	uint16_t div_value;
} division_map_t;

/**
 * struct clk_divider - adjustable divider clock
 *
 * @hw:     handle between common and hardware-specific interfaces
 * @reg:    register containing the divider
 * @shift:  shift to the divider bit field
 * @width:  width of the divider bit field
 * @table:  array of value/divider pairs, last entry should have div = 0
 * @lock:   register lock
 *
 * Clock with an adjustable divider affecting its output frequency.  Implements
 * .recalc_rate, .set_rate and .round_rate
 *
 * Flags:
 * CLK_DIVIDER_ONE_BASED - by default the divisor is the value read from the
 *  register plus one.  If CLK_DIVIDER_ONE_BASED is set then the divider is
 *  the raw value read from the register, with the value of zero considered
 *  invalid, unless CLK_DIVIDER_ALLOW_ZERO is set.
 * CLK_DIVIDER_POWER_OF_TWO - clock divisor is 2 raised to the value read from
 *  the hardware register
 * CLK_DIVIDER_ALLOW_ZERO - Allow zero divisors.  For dividers which have
 *  CLK_DIVIDER_ONE_BASED set, it is possible to end up with a zero divisor.
 *  Some hardware implementations gracefully handle this case and allow a
 *  zero divisor by not modifying their input clock
 *  (divide by one / bypass).
 * CLK_DIVIDER_HIWORD_MASK - The divider settings are only in lower 16-bit
 *  of this register, and mask of divider bits are in higher 16-bit of this
 *  register.  While setting the divider bits, higher 16-bit should also be
 *  updated to indicate changing divider bits.
 * CLK_DIVIDER_ROUND_CLOSEST - Makes the best calculated divider to be rounded
 *  to the closest integer instead of the up one.
 * CLK_DIVIDER_READ_ONLY - The divider settings are preconfigured and should
 *  not be changed by the clock framework.
 * CLK_DIVIDER_MAX_AT_ZERO - For dividers which are like CLK_DIVIDER_ONE_BASED
 *  except when the value read from the register is zero, the divisor is
 *  2^width of the field.
 * CLK_DIVIDER_BIG_ENDIAN - By default little endian register accesses are used
 *  for the divider register.  Setting this flag makes the register accesses
 *  big endian.
 */
struct clk_divider
{
	struct clk_hw   hw;
	unsigned long   reg;
	u8      shift;
	u8      width;
	u8      flags;
	hal_spinlock_t lock;
	const struct clk_div_table  *table;
};

extern const struct clk_ops clk_divider_ops;
extern const struct clk_ops clk_divider_ro_ops;

#define CLK_DIVIDER_ONE_BASED       BIT(0)
#define CLK_DIVIDER_POWER_OF_TWO    BIT(1)
#define CLK_DIVIDER_ALLOW_ZERO      BIT(2)
#define CLK_DIVIDER_HIWORD_MASK     BIT(3)
#define CLK_DIVIDER_ROUND_CLOSEST   BIT(4)
#define CLK_DIVIDER_READ_ONLY       BIT(5)
#define CLK_DIVIDER_MAX_AT_ZERO     BIT(6)
#define CLK_DIVIDER_BIG_ENDIAN      BIT(7)

unsigned long divider_recalc_rate(struct clk_hw *hw, unsigned long parent_rate,
								  unsigned int val, const struct clk_div_table *table,
								  unsigned long flags, unsigned long width);
long divider_ro_round_rate_parent(struct clk_hw *hw, struct clk_hw *parent,
								  unsigned long rate, unsigned long *prate,
								  const struct clk_div_table *table, u8 width,
								  unsigned long flags, unsigned int val);
long divider_round_rate_parent(struct clk_hw *hw, struct clk_hw *parent,
							   unsigned long rate, unsigned long *prate,
							   const struct clk_div_table *table,
							   u8 width, unsigned long flags);

int divider_get_val(unsigned long rate, unsigned long parent_rate,
					const struct clk_div_table *table, u8 width,
					unsigned long flags);

#define clk_div_mask(width) ((1 << (width)) - 1)
#define to_clk_divider(_hw) container_of(_hw, struct clk_divider, hw)

#define CLK_HW_ELEMENT(_id, _struct_name) [_id] = &(_struct_name).hw
#define CCU_CLK_HW_ELEMENT(_id, _struct_name) [_id] = &(_struct_name).ccu_clk_hw.hw

#define CLK_HW_INIT(_name, _parent, _ops, _flags)       \
	(&(struct clk_init_data) {				\
		.flags		= _flags,			\
		.name 	  = _name,			  \
		.parents  = (const struct clk_desc *[]) { _parent }, \
		.num_parents	= 1,				\
		.ops		  = _ops,			  \
	})

#ifdef CLK_HW_INFO_STRUCT_HAS_NAME_MEMBER
#define CLK_HW_INFO(_name, _parent, _ops, _flags)       \
{				\
	.flags		= _flags,			\
	.name 	  = _name,			  \
	.parents  =  (const clk_number_t []){ _parent }, \
	.num_parents	= 1,				\
	.ops		  = _ops,			  \
}

#define MUX_CLK_HW_INFO(_name, _parents, _ops, _flags)  \
{              \
	.flags		= _flags,			\
	.name 	  = _name,			  \
	.parents  = _parents, \
	.num_parents	= ARRAY_SIZE(_parents),				\
	.ops		  = _ops,			  \
}
#else
#define CLK_HW_INFO(_name, _parent, _ops, _flags)       \
{				\
	.flags		= _flags,			\
	.parents  =  (const clk_number_t []){ _parent }, \
	.num_parents	= 1,				\
	.ops		  = _ops,			  \
}

#define MUX_CLK_HW_INFO(_name, _parents, _ops, _flags)  \
{              \
	.flags		= _flags,			\
	.parents  = _parents, \
	.num_parents	= ARRAY_SIZE(_parents),				\
	.ops		  = _ops,			  \
}
#endif
#define MUX_CLK_HW_INFO_OLD(_name, _parents, _ops, _flags)  \
{              \
	.flags		= _flags,			\
	.name 	  = _name,			  \
	.parents  = (const struct clk_desc *[]) { _parents }, \
	.num_parents	= ARRAY_SIZE(_parents),				\
	.ops		  = _ops,			  \
}

#define CLK_FIXED_FACTOR(_struct_name, _clk_name, _flags, _parent, \
		_mult_val, _div_val) \
struct clk_fixed_factor _struct_name = \
{ \
	.mult_val = _mult_val, \
	.div_val = _div_val, \
	.hw = \
	{ \
		.info = CLK_HW_INFO(_clk_name, _parent, &g_clk_fixed_factor_ops, _flags) \
	} \
}

int clk_core_get_clk_hw(clk_controller_id_t cc_id, clk_id_t clk_id, clk_hw_t **clk_hw);

static inline int clk_core_get_clk(clk_controller_id_t cc_id, clk_id_t clk_id, clk_t **clk)
{
	int ret;
	clk_hw_t *clk_hw;

	ret = clk_core_get_clk_hw(cc_id, clk_id, &clk_hw);
	if (ret)
		return ret;

	*clk = &clk_hw->clk;
	return 0;
}

static inline int clk_core_put_clk(clk_t *clk)
{
	return 0;
}

const char *clk_hw_get_name(const clk_hw_t *hw);

//int clk_core_get_id_info(clk_t *clk);

int clk_core_enable_clk(clk_t *clk);
int clk_core_disable_clk(clk_t *clk);
int clk_core_get_enable_state(clk_t *clk, int *is_enabled);

int clk_core_set_parent(clk_t *clk, clk_t *parent);
int clk_core_get_parent(clk_t *clk, clk_t **parent);

int clk_core_set_freq(clk_t *clk, uint32_t freq);
int clk_core_get_freq(clk_t *clk, uint32_t *freq);
int clk_core_round_freq(clk_t *clk, uint32_t *freq);

uint8_t clk_hw_get_num_parents(const struct clk_hw *hw);
int clk_hw_get_controller_reg_base(const struct clk_hw *hw, unsigned long *reg_base);
int clk_hw_get_controller(const struct clk_hw *hw, clk_controller_t **controller);

int clk_hw_init(clk_hw_t *hw);

unsigned long clk_hw_get_flags(const struct clk_hw *hw);

//reset releated
int clk_core_get_rst_hw(rst_controller_id_t rc_id, rst_id_t rst_id, reset_hw_t **rst_hw);
int rst_hw_get_controller(const reset_hw_t *hw, clk_controller_t **controller);


int clk_core_check_ctrl_field_info(uint8_t field_offset, uint8_t field_width, uint32_t max_value_in_sw);

//int clk_hw_get_name(const clk_hw_t *hw, const char **name);

int clk_core_init(void);

int clk_core_show_all_clk(void);


#endif /* __CLK_CORE_H__ */

