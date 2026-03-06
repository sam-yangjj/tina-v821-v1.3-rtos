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


#include "aw_ccu.h"
#include "clk_core.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mux.h"
#include "ccu_pll.h"

#include "sun300iw1_aon_ccu.h"

static division_map_t g_pll_common_pre_div_map[] =
{
	{ .field_value = 0, .div_value = 1 },
	{ .field_value = 1, .div_value = 2 },
	{ .field_value = 2, .div_value = 4 },
	{ .field_value = 3, .div_value = 4 },
	{ /* Sentinel */ },
};

static AW_CCU_DIV_WITH_TABLE(g_cpu_pll_pre_div_clk, "cpu-pll-pre-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX), 0x0000, 2, 2, g_pll_common_pre_div_map);

static AW_CCU_PLL_WITH_DEFAULT_STABLE_TIME(g_cpu_pll_clk, "cpu-pll", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CPU_PLL_PRE_DIV), 0x0000, 8, 8, 30, 31, 27, 29, 28);

static division_map_t g_peri_pll_pre_div_map[] =
{
	{ .field_value = 0, .div_value = 1 },
	{ .field_value = 1, .div_value = 2 },
	{ .field_value = 2, .div_value = 3 },
	{ .field_value = 4, .div_value = 5 },
	{ /* Sentinel */ },
};


static AW_CCU_DIV_WITH_TABLE(g_peri_pll_pre_div_clk, "peri-pll-pre-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX), 0x0020, 0, 3, g_peri_pll_pre_div_map);

static AW_CCU_PLL_WITH_DEFAULT_STABLE_TIME(g_peri_pll_clk, "peri-pll", CLK_IS_CRITICAL,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_PRE_DIV), 0x0020, 8, 8, 30, 31, 27, 29, 28);

CLK_FIXED_FACTOR(g_peri_pll_post_fixed_mult_clk, "peri-pll-post-fixed-mult", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL), 2, 1);

CLK_FIXED_FACTOR(g_peri_pll_1536m_fixed_div_clk, "peri-pll-1536m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 2);

CLK_FIXED_FACTOR(g_peri_pll_1024m_fixed_div_clk, "peri-pll-1024m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 3);

CLK_FIXED_FACTOR(g_peri_pll_768m_fixed_div_clk, "peri-pll-768m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 4);

CLK_FIXED_FACTOR(g_peri_pll_614m_fixed_div_clk, "peri-pll-614m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 5);

CLK_FIXED_FACTOR(g_peri_pll_512m_fixed_div_clk, "peri-pll-512m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 6);

CLK_FIXED_FACTOR(g_peri_pll_384m_fixed_div_clk, "peri-pll-384m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 8);

CLK_FIXED_FACTOR(g_peri_pll_341m_fixed_div_clk, "peri-pll-341m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 9);

CLK_FIXED_FACTOR(g_peri_pll_307m_fixed_div_clk, "peri-pll-307m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 10);

CLK_FIXED_FACTOR(g_peri_pll_236m_fixed_div_clk, "peri-pll-236m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 13);

CLK_FIXED_FACTOR(g_peri_pll_219m_fixed_div_clk, "peri-pll-219m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 14);

CLK_FIXED_FACTOR(g_peri_pll_192m_fixed_div_clk, "peri-pll-192m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 16);

CLK_FIXED_FACTOR(g_peri_pll_118m_fixed_div_clk, "peri-pll-118m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 26);

CLK_FIXED_FACTOR(g_peri_pll_96m_fixed_div_clk, "peri-pll-96m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 32);

CLK_FIXED_FACTOR(g_peri_pll_48m_fixed_div_clk, "peri-pll-48m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 64);

CLK_FIXED_FACTOR(g_peri_pll_24m_fixed_div_clk, "peri-pll-24m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 128);

CLK_FIXED_FACTOR(g_peri_pll_12m_fixed_div_clk, "peri-pll-12m-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_POST_FIXED_MULT), 1, 256);


static AW_CCU_DIV_WITH_TABLE(g_video_pll_pre_div_clk, "video-pll-pre-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX), 0x0040, 1, 2, g_pll_common_pre_div_map);

static AW_CCU_PLL_WITH_DEFAULT_STABLE_TIME(g_video_pll_clk, "video-pll", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL_PRE_DIV), 0x0040, 8, 8, 30, 31, 27, 29, 28);

CLK_FIXED_FACTOR(g_video_pll_2x_fixed_div_clk, "video-pll-2x-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL), 1, 2);

CLK_FIXED_FACTOR(g_video_pll_1x_fixed_div_clk, "video-pll-1x-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL), 1, 4);


static AW_CCU_DIV_WITH_TABLE(g_csi_pll_pre_div_clk, "csi-pll-pre-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX), 0x0048, 1, 2, g_pll_common_pre_div_map);

static AW_CCU_PLL_WITH_DEFAULT_STABLE_TIME(g_csi_pll_int_out_clk, "csi-pll-integer-out", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL_PRE_DIV), 0x0048, 8, 8, 30, 31, 27, 29, 28);

/* Currently CSI PLL is fixed to 675M output */
CLK_FIXED_FACTOR(g_csi_pll_fraction_out_clk, "csi-pll-fraction-out", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_LF), 675, 1);

CLK_FIXED_FACTOR(g_csi_pll_2x_fixed_div_clk, "csi-pll-2x-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL), 1, 2);

CLK_FIXED_FACTOR(g_csi_pll_1x_fixed_div_clk, "csi-pll-1x-fixed-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL), 1, 4);

CLK_FIXED_FACTOR(g_div32k_half_clk, "div32k_half", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX), 1, 2);

AW_CCU_DIV(g_hosc_div_32k_clk, "hosc_div_32k", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_DIV32K_HALF), 0x0510, 0, 10);

static AW_CCU_DIV_WITH_TABLE(g_ddr_pll_pre_div_clk, "ddr-pll-pre-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX), 0x0080, 1, 2, g_pll_common_pre_div_map);

static AW_CCU_PLL_WITH_DEFAULT_STABLE_TIME(g_ddr_pll_clk, "ddr-pll", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_DDR_PLL_PRE_DIV), 0x0080, 8, 8, 30, 31, 27, 29, 28);

static AW_CCU_LDIV(g_ddr_pll_post_div_clk, "ddr-pll-post-div", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_DDR_PLL), 0x0080, 0, 1);


static const clk_number_t g_hosc_fake_mux_parents[] =
{
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_HOSC_40M),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_HOSC_24M),
};
static AW_CCU_MUX(g_hosc_fake_mux_clk, "hosc-fake-mux", 0,
	g_hosc_fake_mux_parents, 0x0404, 31, 1);


static const clk_number_t g_ahb_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_768M_FIXED_DIV),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_LF),
};

static AW_CCU_MUX_LINK_LDIV(g_ahb_clk, "ahb", 0,
	g_ahb_parents, 0x0500, 24, 2, 0, 5);


static const clk_number_t g_apb_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_384M_FIXED_DIV),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_LF),
};

static AW_CCU_MUX_LINK_LDIV(g_apb_clk, "apb", 0,
	g_apb_parents, 0x0504, 24, 2, 0, 5);

static const clk_number_t g_apb_rtc_parents[] =
{
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_LF),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_96M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
};

static AW_CCU_MUX_LINK_LDIV(g_apb_rtc_clk, "apb-rtc", 0,
	g_apb_rtc_parents, 0x0508, 24, 2, 0, 5);


static AW_CCU_GATE(g_bus_pwrctrl_gate_clk, "bus-pwrctrl-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0550, 6);

static AW_CCU_GATE(g_bus_rccal_gate_clk, "bus-rccal-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0550, 2);


static const clk_number_t g_apb_spc_clk_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_LF),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_192M_FIXED_DIV),
};

static AW_CCU_MUX_LINK_LDIV(g_apb_spc_clk, "apb-spc", 0,
	g_apb_spc_clk_parents, 0x0580, 24, 2, 0, 5);

static const clk_number_t g_e907_clk_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL_2X_FIXED_DIV),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_LF),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_LF),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CPU_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_1024M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_614M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_614M_FIXED_DIV),
};

static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_e907_clk, "e907", 0,
	g_e907_clk_parents, 0x0584, 24, 3, 0, 5, 31);


static const clk_number_t g_a27l2_clk_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL_2X_FIXED_DIV),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_LF),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_LF),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CPU_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_1024M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_768M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_768M_FIXED_DIV),
};

static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_a27l2_clk, "a27l2", 0,
	g_a27l2_clk_parents, 0x0588, 24, 3, 0, 5, 31);

static struct clk_hw *g_sun300iw1_aon_clk_hws[] =
{
	CCU_CLK_HW_ELEMENT(CLK_AON_CPU_PLL_PRE_DIV, g_cpu_pll_pre_div_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_CPU_PLL, g_cpu_pll_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_PERI_PLL_PRE_DIV, g_peri_pll_pre_div_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_PERI_PLL, g_peri_pll_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_POST_FIXED_MULT, g_peri_pll_post_fixed_mult_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_1536M_FIXED_DIV, g_peri_pll_1536m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_1024M_FIXED_DIV, g_peri_pll_1024m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_768M_FIXED_DIV, g_peri_pll_768m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_614M_FIXED_DIV, g_peri_pll_614m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_512M_FIXED_DIV, g_peri_pll_512m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_384M_FIXED_DIV, g_peri_pll_384m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_341M_FIXED_DIV, g_peri_pll_341m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_307M_FIXED_DIV, g_peri_pll_307m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_236M_FIXED_DIV, g_peri_pll_236m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_219M_FIXED_DIV, g_peri_pll_219m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_192M_FIXED_DIV, g_peri_pll_192m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_118M_FIXED_DIV, g_peri_pll_118m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_96M_FIXED_DIV, g_peri_pll_96m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_48M_FIXED_DIV, g_peri_pll_48m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_24M_FIXED_DIV, g_peri_pll_24m_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_PERI_PLL_12M_FIXED_DIV, g_peri_pll_12m_fixed_div_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_VIDEO_PLL_PRE_DIV, g_video_pll_pre_div_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_VIDEO_PLL, g_video_pll_clk),
	CLK_HW_ELEMENT(CLK_AON_VIDEO_PLL_2X_FIXED_DIV, g_video_pll_2x_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_VIDEO_PLL_1X_FIXED_DIV, g_video_pll_1x_fixed_div_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_CSI_PLL_PRE_DIV, g_csi_pll_pre_div_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_CSI_PLL_INT_OUT, g_csi_pll_int_out_clk),
	CLK_HW_ELEMENT(CLK_AON_CSI_PLL, g_csi_pll_fraction_out_clk),
	CLK_HW_ELEMENT(CLK_AON_CSI_PLL_2X_FIXED_DIV, g_csi_pll_2x_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_CSI_PLL_1X_FIXED_DIV, g_csi_pll_1x_fixed_div_clk),
	CLK_HW_ELEMENT(CLK_AON_DIV32K_HALF, g_div32k_half_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_HOSC_DIV_32K, g_hosc_div_32k_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_DDR_PLL_PRE_DIV, g_ddr_pll_pre_div_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_DDR_PLL, g_ddr_pll_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_DDR_PLL_POST_DIV, g_ddr_pll_post_div_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_HOSC_FAKE_MUX, g_hosc_fake_mux_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_AHB, g_ahb_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_APB, g_apb_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_APB_RTC, g_apb_rtc_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_BUS_PWRCTRL_GATE, g_bus_pwrctrl_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_BUS_RCCAL_GATE, g_bus_rccal_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_APB_SPC, g_apb_spc_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_E907, g_e907_clk),
	CCU_CLK_HW_ELEMENT(CLK_AON_A27L2, g_a27l2_clk),
};

static reset_hw_t g_sun300iw1_aon_reset_hws[] =
{
	RESET_HW_ELEMENT(RST_AON_BUS_WLAN, 0x0518, 0),
};

clk_controller_t g_aon_ccu =
{
	.id = AW_AON_CCU,
	.reg_base = AON_CCU_REG_BASE,

	.clk_num = ARRAY_SIZE(g_sun300iw1_aon_clk_hws),
	.clk_hws = g_sun300iw1_aon_clk_hws,

	.reset_num = ARRAY_SIZE(g_sun300iw1_aon_reset_hws),
	.reset_hws = g_sun300iw1_aon_reset_hws,
};

