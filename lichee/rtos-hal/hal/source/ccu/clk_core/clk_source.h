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

#ifndef __CLK_SOURCE_H__
#define __CLK_SOURCE_H__

#include "clk_core.h"

typedef struct clk_source
{
	uint32_t rate;
	clk_hw_t hw;
} clk_source_t;

#define to_clk_source(_hw) container_of(_hw, struct clk_source, hw)

#ifdef CLK_HW_INFO_STRUCT_HAS_NAME_MEMBER
#define CLK_SOURCE_WITH_FLAGS(_struct_name, _clk_name, _flags, _rate) \
clk_source_t _struct_name = \
{ \
	.rate = _rate, \
	.hw = \
	{ \
		.info = \
		{				\
			.flags = _flags, \
			.name = _clk_name, \
			.parents =  NULL, \
			.num_parents = 0, \
			.ops = &g_clk_source_ops, \
		} \
	} \
}
#else
#define CLK_SOURCE_WITH_FLAGS(_struct_name, _clk_name, _flags, _rate) \
clk_source_t _struct_name = \
{ \
	.rate = _rate, \
	.hw = \
	{ \
		.info = \
		{				\
			.flags = _flags, \
			.parents =	NULL, \
			.num_parents = 0, \
			.ops = &g_clk_source_ops, \
		} \
	} \
}
#endif

#define CLK_SOURCE(_struct_name, _clk_name, _rate) \
	CLK_SOURCE_WITH_FLAGS(_struct_name, _clk_name, 0, _rate)

extern const clk_hw_ops_t g_clk_source_ops;

#endif /* __CLK_SOURCE_H__ */
