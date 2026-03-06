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

#ifndef __AW_CCU_H__
#define __AW_CCU_H__

#include "clk_core.h"

#define CCU_FEATURE_FRACTIONAL      BIT(0)
#define CCU_FEATURE_VARIABLE_PREDIV BIT(1)
#define CCU_FEATURE_FIXED_PREDIV    BIT(2)
#define CCU_FEATURE_FIXED_POSTDIV   BIT(3)
#define CCU_FEATURE_ALL_PREDIV      BIT(4)
#define CCU_FEATURE_LOCK_REG        BIT(5)
#define CCU_FEATURE_MMC_TIMING_SWITCH   BIT(6)
#define CCU_FEATURE_SIGMA_DELTA_MOD BIT(7)

/* Support key-field reg setting */
#define CCU_FEATURE_KEY_FIELD_MOD   BIT(8)

/* New formula support in MP: clk = parent / M / P */
#define CCU_FEATURE_MP_NO_INDEX_MODE    BIT(9)

/* Support fixed rate in gate-clk */
#define CCU_FEATURE_FIXED_RATE_GATE BIT(10)

/* In R328, the high and low bits of the fractional division part are reversed */
#define CCU_FEATURE_DPLL_FRAC_REVERSE	BIT(11)
/* MMC timing mode switch bit */
#define CCU_MMC_NEW_TIMING_MODE     BIT(30)

/* Some clks need two gate register */
#define CCU_FEATURE_GATE_DOUBLE_REG     BIT(13)

#define UPDATE_BIT_ACTIVE_HIGH

#ifdef UPDATE_BIT_ACTIVE_HIGH
#define CCU_CLK_HW_VALIDATE_CFG_VALUE 1
#else
#define CCU_CLK_HW_VALIDATE_CFG_VALUE 0
#endif

typedef struct aw_ccu_clk_hw
{
	uint16_t reg_offset;
	uint16_t features; /* AW CCU level flags */
	struct clk_hw hw;
} aw_ccu_clk_hw_t;

static inline aw_ccu_clk_hw_t *hw_to_ccu_clk_hw(clk_hw_t *hw)
{
	return container_of(hw, aw_ccu_clk_hw_t, hw);
}

void dump_aw_ccu_clk_hw(const aw_ccu_clk_hw_t *hw);

int ccu_clk_hw_set_control_bit(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t ctrl_bit_offset, int is_set, int is_need_lock);

static inline int ccu_clk_hw_set_control_bit_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	uint8_t ctrl_bit_offset, int is_set)
{
	return ccu_clk_hw_set_control_bit(ccu_clk_hw, ctrl_bit_offset, is_set, 1);
}

static inline int ccu_clk_hw_set_control_bit_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	uint8_t ctrl_bit_offset, int is_set)
{
	return ccu_clk_hw_set_control_bit(ccu_clk_hw, ctrl_bit_offset, is_set, 0);
}

int ccu_clk_hw_get_control_bit(aw_ccu_clk_hw_t *ccu_clk_hw, uint8_t ctrl_bit_offset, int *is_set, int is_need_lock);

static inline int ccu_clk_hw_get_control_bit_with_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	uint8_t ctrl_bit_offset, int *is_set)
{
	return ccu_clk_hw_get_control_bit(ccu_clk_hw, ctrl_bit_offset, is_set, 1);
}

static inline int ccu_clk_hw_get_control_bit_without_lock(aw_ccu_clk_hw_t *ccu_clk_hw,
	uint8_t ctrl_bit_offset, int *is_set)
{
	return ccu_clk_hw_get_control_bit(ccu_clk_hw, ctrl_bit_offset, is_set, 0);
}

#endif /* __AW_CCU_H__ */
