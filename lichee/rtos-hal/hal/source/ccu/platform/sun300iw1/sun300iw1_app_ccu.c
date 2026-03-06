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


#include "ccu_gate.h"
#include "ccu_mux.h"
#include "ccu_div.h"
#include "ccu_pll.h"

#include "sun300iw1_app_ccu.h"
#include "sun300iw1_rtc_ccu.h"

static const clk_number_t g_dram_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_DDR_PLL_POST_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_1024M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_768M_FIXED_DIV),
};
static AW_CCU_MUX_WITH_UPD_LINK_LDIV_PDIV_LINK_GATE(g_dram_clk, "dram", 0,
	g_dram_parents, 0x0004, 24, 3, 27, 0, 5, 16, 2, 31);

static const clk_number_t g_e907_a27l2_mt_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_LOSC),
};
static AW_CCU_MUX_LINK_GATE(g_e907_ts_clk, "e907-ts", 0,
	g_e907_a27l2_mt_parents, 0x000c, 24, 1, 31);

static AW_CCU_MUX_LINK_GATE(g_a27l2_mt_clk, "a27l2-mt", 0,
	g_e907_a27l2_mt_parents, 0x0010, 24, 1, 31);

static const clk_number_t g_smhcx_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_192M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_219M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_219M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_DDR_PLL_POST_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL_2X_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_DDR_PLL_POST_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL_2X_FIXED_DIV),
};
static AW_CCU_MUX_LINK_LDIV_LDIV_LINK_GATE(g_smhc0_clk, "smhc0", 0,
	g_smhcx_parents, 0x014, 24, 3, 0, 5, 16, 5, 31);

static const clk_number_t g_ss_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_118M_FIXED_DIV),
};
static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_ss_clk, "ss", 0,
	g_ss_parents, 0x018, 24, 1, 0, 5, 31);

static const clk_number_t g_spi_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_307M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_236M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_236M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_48M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL_2X_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_48M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL_2X_FIXED_DIV),
};
static AW_CCU_MUX_LINK_LDIV_PDIV_LINK_GATE(g_spi_clk, "spi", 0,
	g_spi_parents, 0x01c, 24, 3, 0, 4, 16, 2, 31);

static const clk_number_t g_spif_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_512M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_384M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_307M_FIXED_DIV),
};
static AW_CCU_MUX_LINK_LDIV_PDIV_LINK_GATE(g_spif_clk, "spif", 0,
	g_spif_parents, 0x020, 24, 2, 0, 4, 16, 2, 31);

static const clk_number_t g_mcsi_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_236M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_307M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_384M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_384M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL),
};
static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_mcsi_clk, "mcsi", 0,
	g_mcsi_parents, 0x024, 24, 3, 0, 5, 31);

static const clk_number_t g_csi_master_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_1024M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_24M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_1024M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_24M_FIXED_DIV),
};
static AW_CCU_MUX_LINK_LDIV_PDIV_LINK_GATE(g_csi_master0_clk, "csi-master0", 0,
	g_csi_master_parents, 0x028, 24, 3, 0, 5, 16, 2, 31);

static AW_CCU_MUX_LINK_LDIV_PDIV_LINK_GATE(g_csi_master1_clk, "csi-master1", 0,
	g_csi_master_parents, 0x02C, 24, 3, 0, 5, 16, 2, 31);

static AW_CCU_MUX_LINK_LDIV_PDIV_LINK_GATE(g_spi2_clk, "spi2", 0,
	g_spi_parents, 0x0030, 24, 3, 0, 4, 16, 2, 31);

static const clk_number_t g_tconlcd_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_512M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL),
};
static AW_CCU_MUX_LINK_LDIV_PDIV_LINK_GATE(g_tconlcd_clk, "tconlcd", 0,
	g_tconlcd_parents, 0x0034, 24, 2, 0, 4, 16, 2, 31);

static const clk_number_t g_de_g2d_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_307M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL_1X_FIXED_DIV),
};

static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_de_clk, "de", 0,
	g_de_g2d_parents, 0x038, 24, 1, 0, 5, 31);

static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_g2d_clk, "g2d", 0,
	g_de_g2d_parents, 0x03c, 24, 1, 0, 5, 31);

static const clk_number_t g_gpadc_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_24M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_LOSC),
};

static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_gpadc_clk, "gpadc", 0,
	g_gpadc_parents, 0x040, 24, 2, 0, 5, 31);

static const clk_number_t g_ve_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_219M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_341M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_614M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_768M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_1024M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL_2X_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL),
};

static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_ve_clk, "ve", 0,
	g_ve_parents, 0x044, 24, 3, 0, 3, 31);

static const clk_number_t g_audio_dac_adc_i2s_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_APP_PLL_AUDIO_1X),
	MAKE_CLKn(AW_AON_CCU, CLK_APP_PLL_AUDIO_4X),
};

static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_audio_dac_clk, "audio-dac", 0,
	g_audio_dac_adc_i2s_parents, 0x048, 24, 1, 0, 4, 31);


static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_audio_adc_clk, "audio-adc", 0,
	g_audio_dac_adc_i2s_parents, 0x04c, 24, 1, 0, 4, 31);

static AW_CCU_MUX_LINK_LDIV_LINK_GATE(g_i2s0_clk, "i2s0", 0,
	g_audio_dac_adc_i2s_parents, 0x054, 24, 1, 0, 4, 31);

static AW_CCU_MUX_LINK_LDIV_LDIV_LINK_GATE(g_smhc1_clk, "smhc1", 0,
	g_smhcx_parents, 0x05c, 24, 3, 0, 5, 16, 5, 31);

static const clk_number_t g_pll_audio_4x_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_1536M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CPU_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL_2X_FIXED_DIV),
};

static AW_CCU_MUX_LINK_LDIV(g_pll_audio_4x_clk, "pll-audio-4x", 0,
	g_pll_audio_4x_parents, 0x0060, 26, 2, 5, 5);

static const clk_number_t g_pll_audio_1x_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_PERI_PLL_614M_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CPU_PLL),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_VIDEO_PLL_2X_FIXED_DIV),
};
static AW_CCU_MUX_LINK_LDIV(g_pll_audio_1x_clk, "pll-audio-1x", 0,
	g_pll_audio_1x_parents, 0x0060, 24, 2, 0, 5);

static AW_CCU_MUX_LINK_LDIV_PDIV_LINK_GATE(g_spi1_clk, "spi1", 0,
	g_spi_parents, 0x064, 24, 3, 0, 4, 16, 2, 31);

static AW_CCU_GATE(g_a27l2_cfg_gate_clk, "a27l2-cfg-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x007c, 7);

static AW_CCU_GATE(g_a27l2_msgbox_gate_clk, "a27l2-msgbox-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x007c, 6);

static AW_CCU_GATE(g_usb_24m_gate_clk, "usb-24m-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x007c, 3);

static AW_CCU_GATE(g_usb_12m_gate_clk, "usb-12m-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x007c, 2);

static AW_CCU_GATE(g_usb_48m_gate_clk, "usb-48m-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x007c, 1);

static AW_CCU_GATE(g_avs_gate_clk, "avs-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x007c, 0);

static const clk_number_t g_gmac_25m_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_FAKE_MUX),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CSI_PLL_2X_FIXED_DIV),
	MAKE_CLKn(AW_AON_CCU, CLK_AON_CPU_PLL),
};

static AW_CCU_MUX_LINK_LDIV_LDIV_LINK_GATE(g_gmac_25m_clk, "gmac-25m", 0,
	g_gmac_25m_parents, 0x074, 24, 2, 0, 5, 16, 5, 31);

static AW_CCU_GATE(g_bus_dpss_gate_clk, "bus-dpss-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 31);

static AW_CCU_GATE(g_bus_gpio_gate_clk, "bus-gpio-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 30);

static AW_CCU_GATE(g_bus_mcsi_ahb_gate_clk, "bus-mcsi-ahb-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 28);

static AW_CCU_GATE(g_mbus_mcsi_gate_clk, "mbus-mcsi-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 27);

static AW_CCU_GATE(g_bus_vid_out_ahb_gate_clk, "bus-vid-out-ahb-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 26);

static AW_CCU_GATE(g_mbus_vid_out_gate_clk, "mbus-vid-out-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 25);

static AW_CCU_GATE(g_bus_gmac_ahb_gate_clk, "bus-gmac-ahb-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 24);

static AW_CCU_GATE(g_bus_usb_ohci_gate_clk, "bus-usb-ohci-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 22);

static AW_CCU_GATE(g_bus_usb_ehci_gate_clk, "bus-usb-ehci-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 21);

static AW_CCU_GATE(g_bus_usb_otg_gate_clk, "bus-usb-otg-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 20);

static AW_CCU_GATE(g_bus_usb_ahb_gate_clk, "bus-usb-ahb-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 19);

static AW_CCU_GATE(g_bus_uart3_gate_clk, "bus-uart3-gate", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_APB_SPC), 0x0080, 18);

static AW_CCU_GATE(g_bus_uart2_gate_clk, "bus-uart2-gate", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_APB_SPC), 0x0080, 17);

static AW_CCU_GATE(g_bus_uart1_gate_clk, "bus-uart1-gate", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_APB_SPC), 0x0080, 16);

static AW_CCU_GATE(g_bus_uart0_gate_clk, "bus-uart0-gate", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_APB_SPC), 0x0080, 15);

static AW_CCU_GATE(g_bus_twi0_gate_clk, "bus-twi0-gate", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_APB_SPC), 0x0080, 14);

static AW_CCU_GATE(g_bus_pwm_gate_clk, "bus-pwm-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 13);

static AW_CCU_GATE(g_bus_trng_gate_clk, "bus-trng-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 11);

static AW_CCU_GATE(g_bus_timer_gate_clk, "bus-timer-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 10);

static AW_CCU_GATE(g_bus_sg_dma_gate_clk, "bus-sg-dma-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 9);

static AW_CCU_GATE(g_bus_dma_gate_clk, "bus-dma-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 8);

static AW_CCU_GATE(g_bus_syscrtl_gate_clk, "bus-sysctrl-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 7);

static AW_CCU_GATE(g_bus_ce_gate_clk, "bus-ce-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 6);

static AW_CCU_GATE(g_bus_hstimer_gate_clk, "bus-hstimer-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 5);

static AW_CCU_GATE(g_bus_splock_gate_clk, "bus-splock-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 4);

static AW_CCU_GATE(g_bus_dram_gate_clk, "bus-dram-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 3);

static AW_CCU_GATE(g_bus_e907_msgbox_gate_clk, "bus-e907-msgbox-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 2);

static AW_CCU_GATE(g_bus_e907_cfg_gate_clk, "bus-e907-cfg-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0080, 0);

static AW_CCU_GATE(g_mbus_g2d_gate_clk, "mbus-g2d-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 31);

static AW_CCU_GATE(g_bus_g2d_gate_clk, "bus-g2d-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 30);

static AW_CCU_GATE(g_bus_g2d_hb_gate_clk, "bus-g2d-hb-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 29);

static AW_CCU_GATE(g_bus_twi2_gate_clk, "bus-twi2-gate", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_APB_SPC), 0x0084, 25);

static AW_CCU_GATE(g_bus_twi1_gate_clk, "bus-twi1-gate", 0,
	MAKE_CLKn(AW_AON_CCU, CLK_AON_APB_SPC), 0x0084, 24);

static AW_CCU_GATE(g_bus_spi2_gate_clk, "bus-spi2-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 23);

static AW_CCU_GATE(g_bus_gmac_hbus_gate_clk, "bus-gmac-hbus-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 22);

static AW_CCU_GATE(g_bus_smhc1_gate_clk, "bus-smhc1-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 21);

static AW_CCU_GATE(g_bus_smhc0_gate_clk, "bus-smhc0-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 20);

static AW_CCU_GATE(g_bus_spi1_gate_clk, "bus-spi1-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 19);

static AW_CCU_GATE(g_bus_gbgsys_gate_clk, "bus-dbgsys-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 18);

static AW_CCU_GATE(g_mbus_gmac_gate_clk, "mbus-gmac-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 17);

static AW_CCU_GATE(g_mbus_smhc1_gate_clk, "mbus-smhc1-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 16);

static AW_CCU_GATE(g_mbus_smhc0_gate_clk, "mbus-smhc0-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 15);

static AW_CCU_GATE(g_mbus_usb_gate_clk, "mbus-usb-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 14);

static AW_CCU_GATE(g_mbus_sg_dma_gate_clk, "mbus-sg-dma-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 13);

static AW_CCU_GATE(g_bus_i2s0_gate_clk, "bus-i2s0-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 8);

static AW_CCU_GATE(g_bus_adda_gate_clk, "bus-adda-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 6);

static AW_CCU_GATE(g_bus_spif_gate_clk, "bus-spif-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 5);

static AW_CCU_GATE(g_bus_spi_gate_clk, "bus-spi-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 4);

static AW_CCU_GATE(g_bus_ve_ahb_gate_clk, "bus-ve-ahb-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 3);

static AW_CCU_GATE(g_mbus_ve_gate_clk, "mbus-ve-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 2);

static AW_CCU_GATE(g_bus_ths_gate_clk, "bus-ths-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 1);

static AW_CCU_GATE(g_bus_gpa_gate_clk, "bus-gpa-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0084, 0);

static AW_CCU_GATE(g_bus_mcsi_gate_clk, "bus-mcsi-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 28);

static AW_CCU_GATE(g_bus_mcsi_sclk_gate_clk, "bus-mcsi-sclk-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 27);

static AW_CCU_GATE(g_bus_misp_sclk_gate_clk, "bus-misp-sclk-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 26);

static AW_CCU_GATE(g_bus_res_dcap_24m_gate_clk, "bus-res-dcap-24m-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 10);

static AW_CCU_GATE(g_bus_sd_monitor_gate_clk, "bus-sd-monitor-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 9);

static AW_CCU_GATE(g_bus_ahb_monitor_gate_clk, "bus-ahb-monitor-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 8);

static AW_CCU_GATE(g_bus_ve_sclk_gate_clk, "bus-ve-sclk-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 5);

static AW_CCU_GATE(g_bus_ve_gate_clk, "bus-ve-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 4);

static AW_CCU_GATE(g_bus_tcon_gate_clk, "bus-tcon-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 3);

static AW_CCU_GATE(g_bus_sg_dma_mclk_gate_clk, "bus-sg-dma-mclk-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 2);

static AW_CCU_GATE(g_bus_de_gate_clk, "bus-de-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 1);

static AW_CCU_GATE(g_bus_de_hb_gate_clk, "bus-de-hb-gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x0088, 0);

static const clk_number_t g_bt_parents[] =
{
	MAKE_CLKn(AW_AON_CCU, CLK_AON_HOSC_DIV_32K),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_RCCAL_32K),
	MAKE_CLKn(AW_RTC_CCU, CLK_RTC_RCDIV_32K),
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_XO32K),
};

static AW_CCU_MUX_LINK_GATE(g_bt_clk, "bt", 0,
	g_bt_parents, 0x00A0, 4, 2, 3);

static AW_CCU_GATE(g_bus_asw_clk32m_gate_clk, "bus-asw_clk32m_gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x00A0, 2);

static AW_CCU_GATE(g_bus_bt_clk32m_gate_clk, "bus-bt_clk32m_gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x00A0, 1);

static AW_CCU_GATE(g_bus_bt_clk48m_gate_clk, "bus-bt_clk48m_gate", 0,
	MAKE_CLKn(AW_SRC_CCU, CLK_SRC_UNKNOWN), 0x00A0, 0);

static struct clk_hw *g_sun300iw1_clk_hws[] = {
	CCU_CLK_HW_ELEMENT(CLK_APP_DRAM, g_dram_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_E907_TS, g_e907_ts_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_AL27_MT, g_a27l2_mt_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_SMHC0, g_smhc0_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_SS, g_ss_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_SPI, g_spi_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_SPIF, g_spif_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MCSI, g_mcsi_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_CSI_MASTER0, g_csi_master0_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_CSI_MASTER1, g_csi_master1_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_SPI2, g_spi2_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_TCONLCD, g_tconlcd_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_DE, g_de_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_G2D, g_g2d_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_GPADC, g_gpadc_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_VE, g_ve_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_AUDIO_DAC, g_audio_dac_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_AUDIO_ADC, g_audio_adc_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_I2S0, g_i2s0_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_SMHC1, g_smhc1_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_PLL_AUDIO_4X, g_pll_audio_4x_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_PLL_AUDIO_1X, g_pll_audio_1x_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_SPI1, g_spi1_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_A27L2_CFG_GATE, g_a27l2_cfg_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_A27L2_MSGBOX_GATE, g_a27l2_msgbox_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_USB_24M_GATE, g_usb_24m_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_USB_12M_GATE, g_usb_12m_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_USB_48M_GATE, g_usb_48m_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_AVS_GATE, g_avs_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_GMAC_25M, g_gmac_25m_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_DPSS_GATE, g_bus_dpss_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_GPIO_GATE, g_bus_gpio_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_MCSI_AHB_GATE, g_bus_mcsi_ahb_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MBUS_MCSI_GATE, g_mbus_mcsi_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_VID_OUT_GATE, g_bus_vid_out_ahb_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MBUS_VID_OUT_GATE, g_mbus_vid_out_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_GMAC_AHB_GATE, g_bus_gmac_ahb_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_USB_OHCI_GATE, g_bus_usb_ohci_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_USB_EHCI_GATE, g_bus_usb_ehci_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_USB_OTG_GATE, g_bus_usb_otg_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_USB_GATE, g_bus_usb_ahb_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_UART3_GATE, g_bus_uart3_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_UART2_GATE, g_bus_uart2_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_UART1_GATE, g_bus_uart1_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_UART0_GATE, g_bus_uart0_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_TWI0_GATE, g_bus_twi0_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_PWM_GATE, g_bus_pwm_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_TRNG_GATE, g_bus_trng_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_TIMER_GATE, g_bus_timer_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SG_DMA_GATE, g_bus_sg_dma_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_DMA_GATE, g_bus_dma_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SYSCRTL_GATE, g_bus_syscrtl_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_CE_GATE, g_bus_ce_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_HSTIMER_GATE, g_bus_hstimer_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SPLOCK_GATE, g_bus_splock_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_DRAM_GATE, g_bus_dram_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_E907_MSGBOX_GATE, g_bus_e907_msgbox_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_E907_CFG_GATE, g_bus_e907_cfg_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MBUS_G2D_GATE, g_mbus_g2d_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_G2D_GATE, g_bus_g2d_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_G2D_HB_GATE, g_bus_g2d_hb_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_TWI2_GATE, g_bus_twi2_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_TWI1_GATE, g_bus_twi1_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SPI2_GATE, g_bus_spi2_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_GMAC_HBUS_GATE, g_bus_gmac_hbus_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SMHC1_GATE, g_bus_smhc1_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SMHC0_GATE, g_bus_smhc0_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SPI1_GATE, g_bus_spi1_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_GBGSYS_GATE, g_bus_gbgsys_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MBUS_GMAC_GATE, g_mbus_gmac_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MBUS_SMHC1_GATE, g_mbus_smhc1_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MBUS_SMHC0_GATE, g_mbus_smhc0_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MBUS_USB_GATE, g_mbus_usb_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MBUS_SG_DMA_GATE, g_mbus_sg_dma_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_I2S0_GATE, g_bus_i2s0_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_ADDA_GATE, g_bus_adda_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SPIF_GATE, g_bus_spif_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SPI_GATE, g_bus_spi_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_VE_AHB_GATE, g_bus_ve_ahb_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_MBUS_VE_GATE, g_mbus_ve_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_THS_GATE, g_bus_ths_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_GPA_GATE, g_bus_gpa_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_MCSI_GATE, g_bus_mcsi_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_MCSI_SCLK_GATE, g_bus_mcsi_sclk_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_MISP_SCLK_GATE, g_bus_misp_sclk_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_RES_DCAP_24M_GATE, g_bus_res_dcap_24m_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SD_MONITOR_GATE, g_bus_sd_monitor_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_AHB_MONITOR_GATE, g_bus_ahb_monitor_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_VE_SCLK_GATE, g_bus_ve_sclk_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_VE_GATE, g_bus_ve_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_TCON_GATE, g_bus_tcon_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_SG_DMA_MCLK_GATE, g_bus_sg_dma_mclk_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_DE_GATE, g_bus_de_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_DE_HB_GATE, g_bus_de_hb_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BT, g_bt_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_ASW_CLK32M_GATE, g_bus_asw_clk32m_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_BT_CLK32M_GATE, g_bus_bt_clk32m_gate_clk),
	CCU_CLK_HW_ELEMENT(CLK_APP_BUS_BT_CLK48M_GATE, g_bus_bt_clk48m_gate_clk),
};


static reset_hw_t g_sun300iw1_reset_hws[] =
{
	RESET_HW_ELEMENT(RST_APP_BUS_DPSSTOP, 0x0090, 31),
	RESET_HW_ELEMENT(RST_APP_BUS_MCSI, 0x0090, 28),
	RESET_HW_ELEMENT(RST_APP_BUS_HRESETN_MCSI, 0x0090, 27),
	RESET_HW_ELEMENT(RST_APP_BUS_G2D, 0x0090, 26),
	RESET_HW_ELEMENT(RST_APP_BUS_DE, 0x0090, 25),
	RESET_HW_ELEMENT(RST_APP_BUS_GMAC, 0x0090, 24),
	RESET_HW_ELEMENT(RST_APP_BUS_USBPHY, 0x0090, 23),
	RESET_HW_ELEMENT(RST_APP_BUS_USBOHCI, 0x0090, 22),
	RESET_HW_ELEMENT(RST_APP_BUS_USBEHCI, 0x0090, 21),
	RESET_HW_ELEMENT(RST_APP_BUS_USBOTG, 0x0090, 20),
	RESET_HW_ELEMENT(RST_APP_BUS_USB, 0x0090, 19),
	RESET_HW_ELEMENT(RST_APP_BUS_UART3, 0x0090, 18),
	RESET_HW_ELEMENT(RST_APP_BUS_UART2, 0x0090, 17),
	RESET_HW_ELEMENT(RST_APP_BUS_UART1, 0x0090, 16),
	RESET_HW_ELEMENT(RST_APP_BUS_UART0, 0x0090, 15),
	RESET_HW_ELEMENT(RST_APP_BUS_TWI0, 0x0090, 14),
	RESET_HW_ELEMENT(RST_APP_BUS_PWM, 0x0090, 13),
	RESET_HW_ELEMENT(RST_APP_BUS_TRNG, 0x0090, 11),
	RESET_HW_ELEMENT(RST_APP_BUS_TIMER, 0x0090, 10),
	RESET_HW_ELEMENT(RST_APP_BUS_SG_DMA, 0x0090, 9),
	RESET_HW_ELEMENT(RST_APP_BUS_DMA, 0x0090, 8),
	RESET_HW_ELEMENT(RST_APP_BUS_SYSCTRL, 0x0090, 7),
	RESET_HW_ELEMENT(RST_APP_BUS_CE, 0x0090, 6),
	RESET_HW_ELEMENT(RST_APP_BUS_HSTIMER, 0x0090, 5),
	RESET_HW_ELEMENT(RST_APP_BUS_SPLOCK, 0x0090, 4),
	RESET_HW_ELEMENT(RST_APP_BUS_DRAM, 0x0090, 3),
	RESET_HW_ELEMENT(RST_APP_BUS_RV_MSGBOX, 0x0090, 2),
	RESET_HW_ELEMENT(RST_APP_BUS_RV_SYS_APB, 0x0090, 1),
	RESET_HW_ELEMENT(RST_APP_BUS_RV, 0x0090, 0),
	RESET_HW_ELEMENT(RST_APP_BUS_A27_CFG, 0x0094, 28),
	RESET_HW_ELEMENT(RST_APP_BUS_A27_MSGBOX, 0x0094, 27),
	RESET_HW_ELEMENT(RST_APP_BUS_A27, 0x0094, 26),
	RESET_HW_ELEMENT(RST_APP_BUS_TWI2, 0x0094, 25),
	RESET_HW_ELEMENT(RST_APP_BUS_TWI1, 0x0094, 24),
	RESET_HW_ELEMENT(RST_APP_BUS_SPI2, 0x0094, 23),
	RESET_HW_ELEMENT(RST_APP_BUS_SMHC1, 0x0094, 21),
	RESET_HW_ELEMENT(RST_APP_BUS_SMHC0, 0x0094, 20),
	RESET_HW_ELEMENT(RST_APP_BUS_SPI1, 0x0094, 19),
	RESET_HW_ELEMENT(RST_APP_BUS_DGBSYS, 0x0094, 18),
	RESET_HW_ELEMENT(RST_APP_BUS_MBUS, 0x0094, 12),
	RESET_HW_ELEMENT(RST_APP_BUS_TCONLCD, 0x0094, 11),
	RESET_HW_ELEMENT(RST_APP_BUS_TCON, 0x0094, 10),
	RESET_HW_ELEMENT(RST_APP_BUS_I2S0, 0x0094, 8),
	RESET_HW_ELEMENT(RST_APP_BUS_AUDIO, 0x0094, 6),
	RESET_HW_ELEMENT(RST_APP_BUS_SPIF, 0x0094, 5),
	RESET_HW_ELEMENT(RST_APP_BUS_SPI, 0x0094, 4),
	RESET_HW_ELEMENT(RST_APP_BUS_VE, 0x0094, 3),
	RESET_HW_ELEMENT(RST_APP_BUS_THS, 0x0094, 1),
	RESET_HW_ELEMENT(RST_APP_BUS_GPADC, 0x0094, 0),
	RESET_HW_ELEMENT(RST_APP_BUS_A27WFG, 0x0098, 2),
	RESET_HW_ELEMENT(RST_APP_BUS_GPIOWDG, 0x0098, 1),
	RESET_HW_ELEMENT(RST_APP_BUS_RV_WDG, 0x0098, 0),
	RESET_HW_ELEMENT(RST_APP_BUS_E907, 0x009C, 0),
	RESET_HW_ELEMENT(RST_APP_BUS_BT, 0x00A4, 1),
	RESET_HW_ELEMENT(RST_APP_BUS_BT_RTC, 0x00A4, 0),
};

clk_controller_t g_app_ccu =
{
	.id = AW_APP_CCU,
	.reg_base = APP_CCU_REG_BASE,

	.clk_num = ARRAY_SIZE(g_sun300iw1_clk_hws),
	.clk_hws = g_sun300iw1_clk_hws,

	.reset_num = ARRAY_SIZE(g_sun300iw1_reset_hws),
	.reset_hws = g_sun300iw1_reset_hws,
};

