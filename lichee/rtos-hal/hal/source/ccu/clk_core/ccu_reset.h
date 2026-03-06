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

#ifndef _CCU_RESET_H_
#define _CCU_RESET_H_

#include "reset_common.h"

#define RESET_STATE_FIELD_VALUE_0

#ifdef RESET_STATE_FIELD_VALUE_0
#define RST_HW_ASSERT_VALUE 0
#define FIELD_VALUE_TO_RESET_STATUS(value) (!(value))
#else
#define RST_HW_ASSERT_VALUE 1
#define FIELD_VALUE_TO_RESET_STATUS(value) (value)
#endif

#define RST_HW_DEASSERT_VALUE (!RST_HW_ASSERT_VALUE)

typedef struct reset_hw
{
	uint16_t reg_offset;
	uint8_t bit_offset;
	rst_controller_id_t rc_id;
} reset_hw_t;

#define RESET_HW_ELEMENT(_id, _reg_offset, _bit_offset) [_id] = { _reg_offset, _bit_offset }

int ccu_rst_hw_set_ctrl_bit(reset_hw_t *rst_hw, int is_set);
int ccu_rst_hw_get_ctrl_bit(reset_hw_t *rst_hw, int *is_set);

static inline int ccu_rst_hw_assert(reset_hw_t *rst_hw)
{
	return ccu_rst_hw_set_ctrl_bit(rst_hw, RST_HW_ASSERT_VALUE);
}

static inline int ccu_rst_hw_deassert(reset_hw_t *rst_hw)
{
	return ccu_rst_hw_set_ctrl_bit(rst_hw, RST_HW_DEASSERT_VALUE);
}

static inline int ccu_rst_hw_get_status(reset_hw_t *rst_hw, int *status)
{
	int ret, is_set;

	ret = ccu_rst_hw_get_ctrl_bit(rst_hw, &is_set);
	if (ret)
		return ret;

	*status = FIELD_VALUE_TO_RESET_STATUS(is_set);
	return 0;
}

#endif /* _CCU_RESET_H_ */
