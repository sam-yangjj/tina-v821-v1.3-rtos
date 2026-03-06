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
#if 0

#include "ccu_mp.h"
#include "ccu_nm.h"
#endif

#include "sun55iw6_prcm_ccu.h"

#if 0

static const char * const ahbs_parents[] = { "dcxo24M", "ext-32k",
	"rc-16m", "pll-peri0-200m",
	"pll-peri0-300m" };

static SUNXI_CCU_MP_WITH_MUX(r_ahb_clk, "r-ahb",
		ahbs_parents, 0x000,
		0, 5,
		8, 2,
		24, 3,
		0);

static const char * const apbs_parents[] = { "dcxo24M", "ext-32k",
	"rc-16m", "pll-peri0-200m" };
static SUNXI_CCU_M_WITH_MUX(r_apbs0_clk, "r-apbs0",
		apbs_parents, 0x00c,
		0, 5,
		24, 3,
		0);

static SUNXI_CCU_M_WITH_MUX(r_apbs1_clk, "r-apbs1",
		apbs_parents, 0x010,
		0, 5,
		24, 3,
		0);

static const char * const r_timer0_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer0_clk, "r-timer0",
		r_timer0_parents, 0x0100,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_IGNORE_UNUSED);

static const char * const r_timer1_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer1_clk, "r-timer1",
		r_timer1_parents, 0x0104,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_IGNORE_UNUSED);

static const char * const r_timer2_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer2_clk, "r-timer2",
		r_timer2_parents, 0x0108,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const r_timer3_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_timer3_clk, "r-timer3",
		r_timer3_parents, 0x010C,
		1, 3,	/* M */
		4, 3,	/* mux */
		BIT(0),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(r_timer_clk, "r-timer",
		"dcxo24M",
		0x011C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_twd_clk, "r-twd",
		"dcxo24M",
		0x012C, BIT(0), 0);

static const char * const r_pwm_parents[] = { "dcxo24M", "rtc-32k", "rc16m" };

static SUNXI_CCU_MUX_WITH_GATE(r_pwm_clk, "r-pwm",
		r_pwm_parents, 0x0130,
		24, 2,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(r_pwm_bus_clk, "r-pwm-bus",
		"dcxo24M",
		0x013C, BIT(0), 0);

static const char * const r_spi_parents[] = { "dcxo24M", "pll-peri0-200m", "pll-peri0-300m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(r_spi_clk, "r-spi",
		r_spi_parents, 0x0150,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(r_spi_bus_clk, "r-spi-bus",
		"dcxo24M",
		0x015C, BIT(0), 0);

static SUNXI_CCU_GATE(r_mbox_clk, "r-mbox",
		"dcxo24M",
		0x017C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_uart1_clk, "r-uart1",
		"dcxo24M",
		0x018C, BIT(1), 0);

static SUNXI_CCU_GATE(r_uart0_clk, "r-uart0",
		"dcxo24M",
		0x018C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_twi1_clk, "r-twi1",
		"dcxo24M",
		0x019C, BIT(1), 0);

static SUNXI_CCU_GATE(r_twi0_clk, "r-twi0",
		"dcxo24M",
		0x019C, BIT(0), 0);

static SUNXI_CCU_GATE(r_ppu_clk, "r-ppu",
		"dcxo24M",
		0x01AC, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_tzma_clk, "r-tzma",
		"dcxo24M",
		0x01B0, BIT(0), 0);

static SUNXI_CCU_GATE(r_cpus_bist_clk, "r-cpus-bist",
		"dcxo24M",
		0x01BC, BIT(0), 0);

static const char * const r_irrx_parents[] = { "osc32k", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_irrx_clk, "r-irrx",
		r_irrx_parents, 0x01C0,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(r_irrx_bus_clk, "r-irrx-bus",
		"dcxo24M",
		0x01CC, BIT(0), 0);

static SUNXI_CCU_GATE(rtc_clk, "rtc",
		"dcxo24M",
		0x020C, BIT(0), 0);

static SUNXI_CCU_GATE(r_cpucfg_clk, "r-cpucfg",
		"dcxo24M",
		0x022C, BIT(0), CLK_IGNORE_UNUSED);

/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun55iw6_r_ccu_resets[] = {
	[RST_BUS_R_TIMER]		= { 0x011c, BIT(16) },
	[RST_BUS_R_PWM]			= { 0x013c, BIT(16) },
	[RST_BUS_R_SPI]			= { 0x015c, BIT(16) },
	[RST_BUS_R_MBOX]		= { 0x017c, BIT(16) },
	[RST_BUS_R_UART1]		= { 0x018c, BIT(17) },
	[RST_BUS_R_UART0]		= { 0x018c, BIT(16) },
	[RST_BUS_R_TWI1]		= { 0x019c, BIT(17) },
	[RST_BUS_R_TWI0]		= { 0x019c, BIT(16) },
	[RST_BUS_R_IRRX]		= { 0x01cc, BIT(16) },
	[RST_BUS_RTC]			= { 0x020c, BIT(16) },
	[RST_BUS_R_CPUCFG]		= { 0x022c, BIT(16) },
};
/* rst_def_end */
/* ccu_def_start */
static struct clk_hw_onecell_data sun55iw6_r_hw_clks = {
	.hws    = {
		[CLK_R_AHB]			= &r_ahb_clk.common.hw,
		[CLK_R_APBS0]			= &r_apbs0_clk.common.hw,
		[CLK_R_APBS1]			= &r_apbs1_clk.common.hw,
		[CLK_R_TIMER0]			= &r_timer0_clk.common.hw,
		[CLK_R_TIMER1]			= &r_timer1_clk.common.hw,
		[CLK_R_TIMER2]			= &r_timer2_clk.common.hw,
		[CLK_R_TIMER3]			= &r_timer3_clk.common.hw,
		[CLK_R_TIMER]			= &r_timer_clk.common.hw,
		[CLK_R_TWD]			= &r_twd_clk.common.hw,
		[CLK_R_PWM]			= &r_pwm_clk.common.hw,
		[CLK_R_BUS_PWM]			= &r_pwm_bus_clk.common.hw,
		[CLK_R_SPI]			= &r_spi_clk.common.hw,
		[CLK_R_BUS_SPI]			= &r_spi_bus_clk.common.hw,
		[CLK_R_MBOX]			= &r_mbox_clk.common.hw,
		[CLK_R_UART1]			= &r_uart1_clk.common.hw,
		[CLK_R_UART0]			= &r_uart0_clk.common.hw,
		[CLK_R_TWI1]			= &r_twi1_clk.common.hw,
		[CLK_R_TWI0]			= &r_twi0_clk.common.hw,
		[CLK_R_PPU]			= &r_ppu_clk.common.hw,
		[CLK_R_TZMA]			= &r_tzma_clk.common.hw,
		[CLK_R_CPUS_BIST]		= &r_cpus_bist_clk.common.hw,
		[CLK_R_IRRX]			= &r_irrx_clk.common.hw,
		[CLK_R_BUS_IRRX]		= &r_irrx_bus_clk.common.hw,
		[CLK_RTC]			= &rtc_clk.common.hw,
		[CLK_R_CPUCFG]			= &r_cpucfg_clk.common.hw,
	},
	.num = CLK_R_NUMBER,
};
/* ccu_def_end */

static struct aw_ccu_clk_hw *sun55iw6_r_ccu_clks[] = {
	&r_ahb_clk.common,
	&r_apbs0_clk.common,
	&r_apbs1_clk.common,
	&r_timer0_clk.common,
	&r_timer1_clk.common,
	&r_timer2_clk.common,
	&r_timer3_clk.common,
	&r_timer_clk.common,
	&r_twd_clk.common,
	&r_pwm_clk.common,
	&r_pwm_bus_clk.common,
	&r_spi_clk.common,
	&r_spi_bus_clk.common,
	&r_mbox_clk.common,
	&r_uart1_clk.common,
	&r_uart0_clk.common,
	&r_twi1_clk.common,
	&r_twi0_clk.common,
	&r_ppu_clk.common,
	&r_tzma_clk.common,
	&r_cpus_bist_clk.common,
	&r_irrx_clk.common,
	&r_irrx_bus_clk.common,
	&rtc_clk.common,
	&r_cpucfg_clk.common,
};

static const struct sunxi_ccu_desc sun55iw6_r_ccu_desc =
{
	.ccu_clks   = sun55iw6_r_ccu_clks,
	.num_ccu_clks   = ARRAY_SIZE(sun55iw6_r_ccu_clks),

	.hw_clks    = &sun55iw6_r_hw_clks,
	.clk_type   = HAL_SUNXI_R_CCU,

	.resets     = sun55iw6_r_ccu_resets,
	.reset_type = HAL_SUNXI_R_RESET,
	.num_resets = ARRAY_SIZE(sun55iw6_r_ccu_resets),
};
#endif
#if 0

int sunxi_r_ccu_init(void)
{


	unsigned long reg = (unsigned long)SUN55IW6_R_CCU_BASE;
	int ret;

	ret = aw_ccu_init(reg, &sun55iw6_r_ccu_desc);
	if (ret) {
		return ret;
	}


	return 0;
}
#endif


#if 0

struct ccu_gate g_test_clk = 
{
	.enable = (1 << 0), 
	.ccu_clk_hw = 
	{
		.reg_offset = 0x076C, 
		.features = 0, 
		.hw.info = 
		{ 
				.flags = 0, 
				.name = "msgbox-rv", 
				.parents = (const clk_number_t []){ ((0 << 16) | 63), 24, 33 }, 
				.num_parents = 1, 
				.ops = &ccu_gate_ops, 
		}
	}
};


AW_CCU_GATE(g_msgbox_rv_clk, "msgbox-rv", MAKE_CLKn(0, CLK_MSGBOX_RV), 0x076C, BIT(0), 0);


static SUNXI_CCU_GATE(r_uart1_clk, "r-uart1",
		"dcxo24M",
		0x018C, BIT(1), 0);

static SUNXI_CCU_GATE(r_uart0_clk, "r-uart0",
		"dcxo24M",
		0x018C, BIT(0), CLK_IGNORE_UNUSED);
#endif

//AW_CCU_MUX_WITH_DIV()

const clk_number_t g_apbsx_parents[] =
{
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_HOSC),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_LOSC),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_HF),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_HF)
};
AW_CCU_MUX(g_apbs1_bus_clk, "apbs1_bus", 0,
	g_apbsx_parents, 0x010, 24, 3);

AW_CCU_DIV(g_apbs1_div_clk, "apbs1_div", 0,
	MAKE_CLKn(AW_PRCM_CCU, CLK_PRCM_APBS1_BUS), 0x010, 0, 5);

#if 0
static const char * const apbs_parents[] = { "dcxo24M", "ext-32k",
	"rc-16m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX(r_apbs1_clk, "r-apbs1",
		apbs_parents, 0x010,
		0, 5,
		24, 3,
		0);

#endif
//AW_CCU_MUX(g_apbs1_bus_clk, "");

const clk_number_t g_r_spi_parents[] =
{
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_HOSC),
	MAKE_CLKn(AW_APP_CCU, CLK_APP_PLL_PERI0_200M_DIV),
	MAKE_CLKn(AW_APP_CCU, CLK_APP_PLL_PERI0_300M_DIV),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RC_HF)
};

AW_CCU_MUX_LINK_DIV_LINK_GATE(g_r_spi_func_clk, "r-spi-bus-clk", 0,
	g_r_spi_parents, 0x150, 24, 3, 8, 5, 31);

AW_CCU_GATE(g_r_spi_func_gate_clk, "r-spi-bus-clk", 0,
	MAKE_CLKn(AW_PRCM_CCU, CLK_PRCM_APBS1_BUS), 0x015C, 0);

AW_CCU_GATE(g_r_spi_bus_clk, "r-spi-bus-clk", 0,
	MAKE_CLKn(AW_PRCM_CCU, CLK_PRCM_APBS1_BUS), 0x015C, 0);

AW_CCU_GATE(g_r_uart0_bus_clk, "r-uart0-bus-clk", 0,
	MAKE_CLKn(AW_PRCM_CCU, CLK_PRCM_APBS1_BUS), 0x018C, 0);

AW_CCU_GATE(g_r_uart1_bus_clk, "r-uart1-bus-clk", 0,
	MAKE_CLKn(AW_PRCM_CCU, CLK_PRCM_APBS1_BUS), 0x018C, 0);


//[CLK_MSGBOX_CORE3] = &g_msgbox_rv_clk2.ccu_clk_hw.hw,

struct clk_hw *g_sun55iw6_prcm_clk_hws[] =
{
	//CLK_HW_ELEMENT(CLK_PRCM_AHBS_BUS, g_apbs0_bus_clk),
	//CLK_HW_ELEMENT(CLK_PRCM_APBS0_BUS, g_apbs0_bus_clk),
	CCU_CLK_HW_ELEMENT(CLK_PRCM_APBS1_BUS, g_apbs1_bus_clk),
	CCU_CLK_HW_ELEMENT(CLK_PRCM_APBS1_DIV, g_apbs1_div_clk),
	CCU_CLK_HW_ELEMENT(CLK_PRCM_R_SPI_FUNC, g_r_spi_func_clk),
	CCU_CLK_HW_ELEMENT(CLK_PRCM_R_SPI_BUS, g_r_spi_bus_clk),
	CCU_CLK_HW_ELEMENT(CLK_PRCM_R_UART0_BUS, g_r_uart0_bus_clk),
	CCU_CLK_HW_ELEMENT(CLK_PRCM_R_UART1_BUS, g_r_uart1_bus_clk),
};

reset_hw_t g_sun55iw6_prcm_reset_hws[] =
{
	//RESET_HW_ELEMENT(RST_BUS_MSGBOX_CORE3, 0x0764, 16),
	//[RST_BUS_MSGBOX_CORE3] = { 0x0764, 16 },
	RESET_HW_ELEMENT(RST_PRCM_R_SPI_BUS, 0x015C, 16),
	RESET_HW_ELEMENT(RST_PRCM_R_UART0_BUS, 0x018C, 16),
	RESET_HW_ELEMENT(RST_PRCM_R_UART1_BUS, 0x018C, 17),
};

clk_controller_t g_prcm_ccu =
{
	.id = AW_PRCM_CCU,
	.reg_base = PRCM_CCU_REG_BASE,

	.clk_num = ARRAY_SIZE(g_sun55iw6_prcm_clk_hws),
	.clk_hws = g_sun55iw6_prcm_clk_hws,

	.reset_num = ARRAY_SIZE(g_sun55iw6_prcm_reset_hws),
	.reset_hws = g_sun55iw6_prcm_reset_hws,
};

