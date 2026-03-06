/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#include "spif.h"
#include "spinor.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <hal_time.h>
#include <hal_cache.h>
#include <hal_hwspinlock.h>

/* porting defination */
#define writel(v, addr)                 (*((volatile uint32_t  *)(addr)) = (uint32_t)(v))
#define readl(addr)                     (*((volatile uint32_t  *)(addr)))

extern void data_sync_barrier(void);

#define flush_dcache(addr, size)        \
	do {                                \
		hal_dcache_clean(addr, size);   \
		data_sync_barrier();            \
	} while(0)

#define invalidate_dcache(addr, size)        \
	do {                                     \
		hal_dcache_invalidate(addr, size);   \
		data_sync_barrier();                 \
	} while(0)

#define SUNXI_SPIF_DEFAULT_CLK  (30000000)

#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
#define HWLOCK_IDX				(CONFIG_COMPONENTS_AW_FLASH_HWLOCK_IDX)
#endif

extern spinor_info_t *spinor_info;

__attribute__((section(".data"))) __aligned(64)
struct sunxi_spif_slave g_sspif;

struct sunxi_spif_slave *get_sspif(void)
{
	return &g_sspif;
}

#ifdef SPIF_DEBUG

int snprintf(char *buf, size_t size, const char *fmt, ...);
static void spif_print_info(void __iomem *base_addr)
{
	char buf[1024] = {0};
	snprintf(buf, sizeof(buf)-1,
			"\n"
			"base_addr = 0x%p, the SPIF control register:\n"
			"[VER] 0x%02x = 0x%08x, [GC]  0x%02x = 0x%08x, [GCA] 0x%02x = 0x%08x\n"
			"[TCR] 0x%02x = 0x%08x, [TDS] 0x%02x = 0x%08x, [INT] 0x%02x = 0x%08x\n"
			"[STA] 0x%02x = 0x%08x, [CSD] 0x%02x = 0x%08x, [PHC] 0x%02x = 0x%08x\n"
			"[TCF] 0x%02x = 0x%08x, [TCS] 0x%02x = 0x%08x, [TNM] 0x%02x = 0x%08x\n"
			"[PSR] 0x%02x = 0x%08x, [PSA] 0x%02x = 0x%08x, [PEA] 0x%02x = 0x%08x\n"
			"[PMA] 0x%02x = 0x%08x, [DMA] 0x%02x = 0x%08x, [DSC] 0x%02x = 0x%08x\n"
			"[DFT] 0x%02x = 0x%08x, [CFT] 0x%02x = 0x%08x, [CFS] 0x%02x = 0x%08x\n"
			"[BAT] 0x%02x = 0x%08x, [BAC] 0x%02x = 0x%08x, [TB]  0x%02x = 0x%08x\n"
			"[RB]  0x%02x = 0x%08x\n",
			base_addr,
			SPIF_VER_REG, readl(base_addr + SPIF_VER_REG),
			SPIF_GC_REG, readl(base_addr + SPIF_GC_REG),
			SPIF_GCA_REG, readl(base_addr + SPIF_GCA_REG),

			SPIF_TC_REG, readl(base_addr + SPIF_TC_REG),
			SPIF_TDS_REG, readl(base_addr + SPIF_TDS_REG),
			SPIF_INT_EN_REG, readl(base_addr + SPIF_INT_EN_REG),

			SPIF_INT_STA_REG, readl(base_addr + SPIF_INT_STA_REG),
			SPIF_CSD_REG, readl(base_addr + SPIF_CSD_REG),
			SPIF_PHC_REG, readl(base_addr + SPIF_PHC_REG),

			SPIF_TCF_REG, readl(base_addr + SPIF_TCF_REG),
			SPIF_TCS_REG, readl(base_addr + SPIF_TCS_REG),
			SPIF_TNM_REG, readl(base_addr + SPIF_TNM_REG),

			SPIF_PS_REG, readl(base_addr + SPIF_PS_REG),
			SPIF_PSA_REG, readl(base_addr + SPIF_PSA_REG),
			SPIF_PEA_REG, readl(base_addr + SPIF_PEA_REG),

			SPIF_PMA_REG, readl(base_addr + SPIF_PMA_REG),
			SPIF_DMA_CTL_REG, readl(base_addr + SPIF_DMA_CTL_REG),
			SPIF_DSC_REG, readl(base_addr + SPIF_DSC_REG),

			SPIF_DFT_REG, readl(base_addr + SPIF_DFT_REG),
			SPIF_CFT_REG, readl(base_addr + SPIF_CFT_REG),
			SPIF_CFS_REG, readl(base_addr + SPIF_CFS_REG),

			SPIF_BAT_REG, readl(base_addr + SPIF_BAT_REG),
			SPIF_BAC_REG, readl(base_addr + SPIF_BAC_REG),
			SPIF_TB_REG, readl(base_addr + SPIF_TB_REG),

			SPIF_RB_REG, readl(base_addr + SPIF_RB_REG));
			SPIF_DBG("%s\n\n", buf);
}

void spif_print_descriptor(struct spif_descriptor_op *spif_op)
{
	char buf[512] = {0};
	while (spif_op) {
		snprintf(buf, sizeof(buf)-1,
				"\n"
				"hburst_rw_flag        : 0x%x\n"
				"block_data_len        : 0x%x\n"
				"data_addr             : 0x%x\n"
				"next_des_addr         : 0x%x\n"
				"trans_phase	       : 0x%x\n"
				"flash_addr	       : 0x%x\n"
				"cmd_mode_buswidth     : 0x%x\n"
				"addr_dummy_data_count : 0x%x\n",
				spif_op->hburst_rw_flag,
				spif_op->block_data_len,
				spif_op->data_addr,
				spif_op->next_des_addr,
				spif_op->trans_phase,
				spif_op->flash_addr,
				spif_op->cmd_mode_buswidth,
				spif_op->addr_dummy_data_count);
		SPIF_DBG("%s", buf);
		SPIF_DBG("spif_op addr [%x]...\n\n", (uint32_t)spif_op);
		spif_op = (struct spif_descriptor_op *)(spif_op->next_des_addr << 2);
	}
}

#else

static void spif_print_info(void __iomem *base_addr)
{

}

void spif_print_descriptor(struct spif_descriptor_op *spif_op)
{

}

#endif

static int32_t sunxi_spif_gpio_request(void)
{
	uint32_t reg_val;
#if defined(CONFIG_ARCH_SUN300IW1)
	reg_val = readl(GPIOC_CFG_REG0);
	reg_val &= ~(0xff << 24);
	reg_val |= 0x22 << 24;
	writel(reg_val, GPIOC_CFG_REG0);

	reg_val = readl(GPIOC_CFG_REG1);
	reg_val &= ~(0xffff << 0);
	reg_val |= 0x2222 << 0;
	writel(reg_val, GPIOC_CFG_REG1);

	reg_val = readl(GPIOC_PULL_REG0);
	reg_val &= ~(0xfff << 12);
	reg_val |= 0x105 << 12;
	writel(reg_val, GPIOC_PULL_REG0);
#else
	#error "spi pinctrl not available for this architecture"
#endif
	return 0;
}

static int32_t sunxi_spif_gpio_exit(void)
{
	uint32_t reg_val;
#if defined(CONFIG_ARCH_SUN300IW1)
	reg_val = readl(GPIOC_CFG_REG0);
	reg_val &= ~(0xff << 24);
	reg_val |= 0xff << 24;
	writel(reg_val, GPIOC_CFG_REG0);

	reg_val = readl(GPIOC_CFG_REG1);
	reg_val &= ~(0xffff << 0);
	reg_val |= 0xffff << 0;
	writel(reg_val, GPIOC_CFG_REG1);

	reg_val = readl(GPIOC_PULL_REG0);
	reg_val &= ~(0xfff << 12);
	reg_val |= 0x410 << 12;
	writel(reg_val, GPIOC_PULL_REG0);
#else
	#error "spi pinctrl not available for this architecture"
#endif
	return 0;
}

uint32_t sunxi_spif_get_version(void)
{
	return readl(SUNXI_SPIF_BASE + SPIF_VER_REG);
}

static void spif_big_little_endian(bool endian, void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	if (endian == LSB_FIRST)
		reg_val |= (SPIF_GC_RX_CFG_FBS | SPIF_GC_TX_CFG_FBS);
	else
		reg_val &= ~(SPIF_GC_RX_CFG_FBS | SPIF_GC_TX_CFG_FBS);
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_clean_mode_en(void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	reg_val &= ~(SPIF_GC_NMODE_EN | SPIF_GC_PMODE_EN);
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_wp_en(bool enable, void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	if (enable)
		reg_val |= SPIF_GC_WP_EN;
	else
		reg_val &= ~SPIF_GC_WP_EN;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_hold_en(bool enable, void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	if (enable)
		reg_val |= SPIF_GC_HOLD_EN;
	else
		reg_val &= ~SPIF_GC_HOLD_EN;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_set_cs_pol(bool pol, void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	if (pol)
		reg_val |= SPIF_GC_CS_POL;
	else
		reg_val &= ~SPIF_GC_CS_POL;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

/* spif config chip select */
static int32_t spif_set_cs(uint32_t chipselect, void __iomem *base_addr)
{
	int ret;
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	if (chipselect < 4) {
		reg_val &= ~SPIF_GC_SS_MASK;/* SS-chip select, clear two bits */
		reg_val |= chipselect << SPIF_GC_SS_BIT_POS;/* set chip select */
		reg_val |= SPIF_GC_CS_POL;/* active low polarity */
		writel(reg_val, base_addr + SPIF_GC_REG);
		ret = SUNXI_SPIF_OK;
	} else {
		SPIF_ERR("Chip Select set fail! cs = %d\n", chipselect);
		ret = SUNXI_SPIF_FAIL;
	}

	return ret;
}

static void spif_set_mode(uint32_t spi_mode, void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	reg_val &= ~SPIF_MASK;
	reg_val |= spi_mode;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

void spif_samp_dl_sw_rx_status(void __iomem  *base_addr, unsigned int status)
{
	unsigned int rval = readl(base_addr + SPIF_TC_REG);

	if (status)
		rval |= SPIF_ANALOG_DL_SW_RX_EN;
	else
		rval &= ~SPIF_ANALOG_DL_SW_RX_EN;

	writel(rval, base_addr +SPIF_TC_REG);
}

void spif_samp_mode(void __iomem  *base_addr, unsigned int status)
{
	unsigned int rval = readl(base_addr + SPIF_TC_REG);

	if (status)
		rval |= SPIF_DIGITAL_ANALOG_EN;
	else
		rval &= ~SPIF_DIGITAL_ANALOG_EN;

	writel(rval, base_addr + SPIF_TC_REG);
}

void spif_set_sample_mode(void __iomem *base_addr, unsigned int mode)
{
	unsigned int rval = readl(base_addr + SPIF_TC_REG);

	rval &= (~SPIF_DIGITAL_DELAY_MASK);
	rval |= mode << SPIF_DIGITAL_DELAY;
	writel(rval, base_addr + SPIF_TC_REG);
}

void spif_set_sample_delay(void __iomem  *base_addr, unsigned int sample_delay)
{
	unsigned int rval = readl(base_addr + SPIF_TC_REG);

	rval &= (~SPIF_ANALOG_DELAY_MASK);
	rval |= sample_delay << SPIF_ANALOG_DELAY;
	writel(rval, base_addr + SPIF_TC_REG);
	hal_mdelay(1);
}

static void spif_config_tc(void __iomem  *base_addr)
{
	if (spinor_info->sample_mode != SAMP_MODE_DL_DEFAULT) {
		spif_samp_mode(base_addr, 1);
		spif_samp_dl_sw_rx_status(base_addr, 1);
		spif_set_sample_mode(base_addr, spinor_info->sample_mode);
		spif_set_sample_delay(base_addr, spinor_info->sample_delay);
	}
}
/*
static void spif_set_dqs(void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_DFT_REG);

	reg_val |= SPIF_DFT_DQS;
	writel(reg_val, base_addr + SPIF_DFT_REG);
}

static void spif_set_cdc(void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_CFT_REG);

	reg_val = SPIF_CFT_CDC;
	writel(reg_val, base_addr + SPIF_CFT_REG);
}
*/
static void spif_set_csd(void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_CSD_REG);

	reg_val |= SPIF_CSD_DEF;
	writel(reg_val, base_addr + SPIF_CSD_REG);
}

/* soft reset spif controller */
static void spif_soft_reset_fifo(void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GCA_REG);

	reg_val |= SPIF_GCA_SRST;
	writel(reg_val, base_addr + SPIF_GCA_REG);
}

static void spif_reset_fifo(void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GCA_REG);

	reg_val |= SPIF_FIFO_SRST;
	writel(reg_val, base_addr + SPIF_GCA_REG);
}

static void spif_set_trans_mode(uint8_t mode, void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	if (mode)
		reg_val |= SPIF_GC_CFG_MODE;
	else
		reg_val &= ~SPIF_GC_CFG_MODE;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

/* set first descriptor start addr */
static void spif_set_des_start_addr(struct spif_descriptor_op *spif_op,
				void __iomem *base_addr)
{
	/* addr word alignment */
	writel((uint32_t)spif_op >> 2, base_addr + SPIF_DSC_REG);
}

/* set descriptor len */
static void spif_set_des_len(int len, void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_DMA_CTL_REG);
	reg_val &= ~(0xff << 4);
	reg_val |= len;
	writel(reg_val, base_addr + SPIF_DMA_CTL_REG);
}

/* DMA start Signal */
static void spif_dma_start_signal(void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_DMA_CTL_REG);

	reg_val |= CFG_DMA_START;
	writel(reg_val, base_addr + SPIF_DMA_CTL_REG);
}

static void spif_trans_type_enable(uint32_t type_phase, void __iomem *base_addr)
{
	writel(type_phase, base_addr + SPIF_PHC_REG);
}

static void spif_set_flash_addr(uint32_t flash_addr, void __iomem *base_addr)
{
	writel(flash_addr, base_addr + SPIF_TCF_REG);
}

static void spif_set_buswidth(uint32_t cmd_mode_buswidth, void __iomem *base_addr)
{
	writel(cmd_mode_buswidth, base_addr + SPIF_TCS_REG);
}

static void spif_set_data_count(uint32_t addr_dummy_data_count, void __iomem *base_addr)
{
	writel(addr_dummy_data_count, base_addr + SPIF_TNM_REG);
}

static void spif_cpu_start_transfer(void __iomem *base_addr)
{
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	reg_val |= SPIF_GC_NMODE_EN;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_set_output_clk(void __iomem *base_addr, uint32_t status)
{
	uint32_t reg_val = readl(base_addr + SPIF_TC_REG);

	if (status)
		reg_val |= SPIF_CLK_SCKOUT_SRC_SEL;
	else
		reg_val &= ~SPIF_CLK_SCKOUT_SRC_SEL;
	writel(reg_val, base_addr + SPIF_TC_REG);
}

static void spif_set_dtr(void __iomem *base_addr, uint32_t status)
{
	uint32_t reg_val = readl(base_addr + SPIF_GC_REG);

	if (status)
		reg_val |= SPIF_GC_DTR_EN;
	else
		reg_val &= ~SPIF_GC_DTR_EN;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static int sunxi_spif_clk_init(uint32_t mod_clk)
{
	unsigned long mclk_base = (unsigned long)SPIF_CLK_CFG;
	uint32_t source_clk = 0;
	uint32_t rval = 0;
	uint32_t m, n, div;
	uint32_t clk[4] = {40000000, 512000000, 384000000, 307000000};

	/* SCLK = src/M/N */
	/* N: 00:1 01:2 10:4 11:8 */
#ifdef FPGA_PLATFORM
	n = 0;
	m = 1;
	rval = CCM_SPIF_CTRL_ENABLE | CCM_SPIF_CTRL_N(n) | CCM_SPIF_CTRL_M(m);;
	source_clk = 24000000;
#else

	source_clk = GET_SPIF_CLK_SOURECS(clk, CCM_SPIF_CTRL_PERI);
	SPIF_INFO("source_clk: %d Hz, mod_clk: %d Hz\n", source_clk, mod_clk);

	div = (source_clk + mod_clk - 1) / mod_clk;
	div = div == 0 ? 1 : div;
	if (div > 128) {
		m = 1;
		n = 0;
		return -1;
	} else if (div > 64) {
		n = 3;
		m = div >> 3;
	} else if (div > 32) {
		n = 2;
		m = div >> 2;
	} else if (div > 16) {
		n = 1;
		m = div >> 1;
	} else {
		n = 0;
		m = div;
	}

	rval = CCM_SPIF_CTRL_ENABLE | CCM_SPIF_CTRL_PERI |
			CCM_SPIF_CTRL_N(n) | CCM_SPIF_CTRL_M(m);

#endif
	writel(rval, (volatile void __iomem *)mclk_base);
	writel(readl((volatile void __iomem *)PLL_PERI_CTL_REG) | 1 << 5, (volatile void __iomem *)PLL_PERI_CTL_REG);

	/* spif reset */
	writel(readl((volatile void __iomem *)SPIF_RST_CFG) & ~(1 << SPIF_RESET_SHIFT), (volatile void __iomem *)SPIF_RST_CFG);
	writel(readl((volatile void __iomem *)SPIF_RST_CFG) | 1 << SPIF_RESET_SHIFT, (volatile void __iomem *)SPIF_RST_CFG);

	/* spif gating */
	writel(readl((volatile void __iomem *)SPIF_GAT_CFG) | 1 << SPIF_GATING_SHIFT, (volatile void __iomem *)SPIF_GAT_CFG);

	SPIF_INFO("src:%d Hz, spif clk:%d Hz, n=%d, m=%d (spif clk = src/(1<<n)/m)\n", source_clk, source_clk/ (1 << n) / m, n, m);
	SPIF_INFO("set reg : [mlck][0x%08x] = 0x%08x; [src][0x%08x] = 0x%08x; [gat][0x%08x] = [0x%08x]; [rst][0x%08x] = [0x%08x]\n",
									SPIF_CLK_CFG, readl((volatile void __iomem *)mclk_base),
									PLL_PERI_CTL_REG, readl((volatile void __iomem *)PLL_PERI_CTL_REG),
									SPIF_GAT_CFG, readl((volatile void __iomem *)SPIF_GAT_CFG),
									SPIF_RST_CFG, readl((volatile void __iomem *)SPIF_RST_CFG));

	return 0;
}
/*
static int sunxi_get_spif_clk(int bus)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned long mclk_base =
		(unsigned long)&ccm->spif_clk_cfg + bus * 0x4;
	uint32_t reg_val = 0;
	uint32_t src = 0, clk = 0, sclk_freq = 0;
	uint32_t n, m;

	reg_val = readl((volatile void __iomem *)mclk_base);
	src = (reg_val >> 24) & 0x7;
	n = (reg_val >> 8) & 0x3;
	m = ((reg_val >> 0) & 0xf) + 1;

	switch(src) {
		case 0:
			clk = 24000000;
			break;
		case 1:
			clk = GET_SPIF_CLK_SOURECS(CCM_SPIF_CTRL_PERI400M);
			break;
		case 2:
			clk = GET_SPIF_CLK_SOURECS(CCM_SPIF_CTRL_PERI300M);
			break;
		default:
			clk = 0;
			break;
	}
	sclk_freq = clk / (1 << n) / m;
	SPIF_INF("sclk_freq= %d Hz,reg_val: %x , n=%d, m=%d\n",
			sclk_freq, reg_val, n, m);
	return sclk_freq;

}
*/
static int sunxi_spif_clk_exit(void)
{
	/* SPIF CCU workaround */
	/*
	writel(0x00, SPIF_CLK_CFG);
	writel(readl(SPIF_RST_CFG) & ~(1 << SPIF_RESET_SHIFT),SPIF_RST_CFG);
	writel(readl(SPIF_GAT_CFG) & ~(1 << SPIF_GATING_SHIFT),SPIF_GAT_CFG);
	*/
	return 0;
}

int set_spif_clk(uint32_t mod_clk)
{
	unsigned long mclk_base = (unsigned long)SPIF_CLK_CFG;
	uint32_t source_clk = 0;
	uint32_t rval = 0;
	uint32_t m, n, div;
	uint32_t clk[4] = {40000000, 512000000, 384000000, 307000000};

	/* clock */
	if (!mod_clk) {
		return -1;
	}
	if (mod_clk > 100000000) { // > 100Mhz
		SPIF_INFO("[Warn]-Force 100Mhz when the SPIF clock exceeds 100 MHz!\n");
		mod_clk = 100000000;
	}
	/* close spif gating */
	writel(readl((volatile void __iomem *)SPIF_GAT_CFG) | 0 << SPIF_GATING_SHIFT, (volatile void __iomem *)SPIF_GAT_CFG);

	/* SCLK = src/M/N */
	/* N: 00:1 01:2 10:4 11:8 */
#ifdef FPGA_PLATFORM
	n = 0;
	m = 1;
	rval = CCM_SPIF_CTRL_ENABLE | CCM_SPIF_CTRL_N(n) | CCM_SPIF_CTRL_M(m);;
	source_clk = 24000000;
#else

	source_clk = GET_SPIF_CLK_SOURECS(clk, CCM_SPIF_CTRL_PERI);
	SPIF_INFO("source_clk: %d Hz, mod_clk: %d Hz\n", source_clk, mod_clk);

	div = (source_clk + mod_clk - 1) / mod_clk;
	div = div == 0 ? 1 : div;
	if (div > 128) {
		m = 1;
		n = 0;
		return -1;
	} else if (div > 64) {
		n = 3;
		m = div >> 3;
	} else if (div > 32) {
		n = 2;
		m = div >> 2;
	} else if (div > 16) {
		n = 1;
		m = div >> 1;
	} else {
		n = 0;
		m = div;
	}

	rval = CCM_SPIF_CTRL_ENABLE | CCM_SPIF_CTRL_PERI |
			CCM_SPIF_CTRL_N(n) | CCM_SPIF_CTRL_M(m);

#endif
	writel(rval, (volatile void __iomem *)mclk_base);
	writel(readl((volatile void __iomem *)PLL_PERI_CTL_REG) | 1 << 5, (volatile void __iomem *)PLL_PERI_CTL_REG);

	/* open spif gating */
	writel(readl((volatile void __iomem *)SPIF_GAT_CFG) | 1 << SPIF_GATING_SHIFT, (volatile void __iomem *)SPIF_GAT_CFG);

	SPIF_INFO("src:%d Hz, spif clk:%d Hz, n=%d, m=%d (spif clk = src/(1<<n)/m)\n", source_clk, source_clk/ (1 << n) / m, n, m);
	SPIF_INFO("set reg : [mlck][0x%08x] = 0x%08x; [src][0x%08x] = 0x%08x; [gat][0x%08x] = [0x%08x]; [rst][0x%08x] = [0x%08x]\n",
									SPIF_CLK_CFG, readl((volatile void __iomem *)mclk_base),
									PLL_PERI_CTL_REG, readl((volatile void __iomem *)PLL_PERI_CTL_REG),
									SPIF_GAT_CFG, readl((volatile void __iomem *)SPIF_GAT_CFG),
									SPIF_RST_CFG, readl((volatile void __iomem *)SPIF_RST_CFG));

	return 0;
}

static void spif_dtr_enable(struct spif_descriptor_op *spif_op)
{
	struct sunxi_spif_slave *sspif = get_sspif();
	void __iomem *base_addr = (void *)SUNXI_SPIF_BASE;
	unsigned int clk = sspif->max_hz;
	unsigned int dtr_double_clk = clk * 2;
	static int double_clk_flag;

	if (!sspif->rx_dtr_en && !sspif->tx_dtr_en)
		return;

	if ((spif_op->cmd_mode_buswidth >> SPIF_ADDR_TRANS_POS) & 0x3) {
		if ((spif_op->trans_phase & SPIF_RX_TRANS_EN) &&
				sspif->rx_dtr_en) {
			if (!double_clk_flag) {
				set_spif_clk(dtr_double_clk);
				set_delay_para();
				double_clk_flag = 1;
			}
			spif_set_output_clk(base_addr, 1);
			spif_set_dtr(base_addr, 1);
		} else if (spif_op->trans_phase & SPIF_TX_TRANS_EN &&
				sspif->tx_dtr_en) {
			if (!dtr_double_clk) {
				set_spif_clk(dtr_double_clk);
				set_delay_para();
				double_clk_flag = 1;
			}
			spif_set_output_clk(base_addr, 1);
			spif_set_dtr(base_addr, 1);
		}
	} else {
		spif_set_output_clk(base_addr, 0);
		spif_set_dtr(base_addr, 0);
		if (double_clk_flag) {
			set_spif_clk(clk);
			double_clk_flag = 0;
		}
	}
}

int spif_claim_bus(void *base_addr)
{
	SPIF_ENTER();

	/* 1. reset all tie logic & fifo */
	spif_soft_reset_fifo(base_addr);
	spif_clean_mode_en(base_addr);

	/* 2. interface first transmit bit select */
	spif_big_little_endian(MSB_FIRST, base_addr);

	/* 3. disable wp & hold */
	spif_wp_en(0, base_addr);
	spif_hold_en(0, base_addr);

	/* 4. disable DTR */
	spif_set_output_clk(base_addr, 0);
	spif_set_dtr(base_addr, 0);

	/* 5. set the default chip select */
	spif_set_cs(0, base_addr);
	spif_set_cs_pol(1, base_addr);

	/* 6. set spi CPOL and CPHA */
	spif_set_mode(SPIF_MODE0, base_addr);

	/* 7. set reg defauld count */
	//spif_set_dqs(base_addr);
	//spif_set_cdc(base_addr);
	spif_set_csd(base_addr);

	return 0;
}

int spif_flash_lock(void);
void spif_flash_unlock(void);

int spif_init(void)
{
	SPIF_ENTER();
#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
	if (spif_flash_lock())
		return -1;
#endif
	/* gpio */
	sunxi_spif_gpio_request();

	/* clock */
	if (sunxi_spif_clk_init(SUNXI_SPIF_DEFAULT_CLK))
		goto err;

	if (spif_claim_bus((void *)SUNXI_SPIF_BASE))
		goto err;
#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
	spif_flash_unlock();
#endif
	return 0;

err:
#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
	spif_flash_unlock();
#endif
	return -1;
}

void spif_exit(void)
{
#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
	if (spif_flash_lock())
		return ;
#endif
	/* disable module clock */
	sunxi_spif_clk_exit();

	sunxi_spif_gpio_exit();

#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
	spif_flash_unlock();
#endif
}
#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
static int g_is_hwspinlock_locked = 0;
#endif

int spif_flash_lock(void)
{
#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
	int ret;
	int timeout = 0xfffffff;

	if (HWLOCK_IDX >= 0) {
		if (g_is_hwspinlock_locked)
			return 0;

		while (1) {
			ret = hal_hwspinlock_get(HWLOCK_IDX);
			if (ret == -444) {
				SPIF_ERR("SPIF hwlock reentrant!\n");
				return -2;
			}
			if (ret == HWSPINLOCK_OK) {
				g_is_hwspinlock_locked = 1;
				break;
			}
			timeout--;
			if (!timeout) {
				SPIF_ERR("SPIF hwlock time_out\n");
				return -1;
			}
		}
	}
#endif
	return 0;
}

void spif_flash_unlock(void)
{
#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
        if (HWLOCK_IDX >= 0) {
                int ret;

                if (!g_is_hwspinlock_locked)
                        return;

                ret = hal_hwspinlock_put(HWLOCK_IDX);
                if (ret) {
                        printf("SFC unlock error, ret: %d\n", ret);
                } else {
                        g_is_hwspinlock_locked = 0;
                }
        }
#endif
}

static void spif_ctr_recover(void)
{
	spif_exit();
	spif_init();
	spif_config_tc((void *)SUNXI_SPIF_BASE);
}

int spif_xfer(struct spif_descriptor_op *spif_op, unsigned int data_len)
{
	int timeout = 0xfffffff;
	void __iomem *base_addr = (void *)SUNXI_SPIF_BASE;
	uint desc_count = ((data_len + SPIF_MAX_TRANS_NUM - 1) / SPIF_MAX_TRANS_NUM) + 1;
	uint desc_size = desc_count * sizeof(struct spif_descriptor_op);
	unsigned int data_addr = (uint32_t)spif_op->data_addr << 2;

	spif_reset_fifo(base_addr);
	spif_dtr_enable(spif_op);
	if ((spif_op->block_data_len & DMA_DATA_LEN) == 0) {
		spif_set_trans_mode(SPIF_GC_CPU_MODE, base_addr);

		spif_trans_type_enable(spif_op->trans_phase, base_addr);

		spif_set_flash_addr(spif_op->flash_addr, base_addr);

		spif_set_buswidth(spif_op->cmd_mode_buswidth, base_addr);

		spif_set_data_count(spif_op->addr_dummy_data_count, base_addr);

		spif_cpu_start_transfer(base_addr);

		while ((readl(base_addr + SPIF_GC_REG) & SPIF_GC_NMODE_EN)) {
			timeout--;
			if (!timeout) {
#ifdef CONFIG_COMPONENTS_AW_FLASH_USE_HWLOCK
				if (HWLOCK_IDX >= 0)
					hal_hwspinlock_put(HWLOCK_IDX);
#endif
				spif_ctr_recover();
				SPIF_ERR("SPIF DMA transfer time_out\n");
				return -1;
			}
		}
		spif_print_info(base_addr);
	} else {
#if 0 //CFG_SPI_USE_DMA
		spif_set_trans_mode(SPIF_GC_CPU_MODE, base_addr);

		spif_trans_type_enable(spif_op->trans_phase, base_addr);

		spif_set_flash_addr(spif_op->flash_addr, base_addr);

		spif_set_buswidth(spif_op->cmd_mode_buswidth, base_addr);

		spif_set_data_count(spif_op->addr_dummy_data_count, base_addr);
#else
		spif_set_trans_mode(SPIF_GC_DMA_MODE, base_addr);

#endif
		/* flush data addr */

		flush_dcache(data_addr, ROUND_UP(data_len, CONFIG_SYS_CACHELINE_SIZE));
		flush_dcache((uint32_t)spif_op, ROUND_UP(desc_size, CONFIG_SYS_CACHELINE_SIZE));

		spif_set_des_start_addr(spif_op, base_addr);

		spif_set_des_len(DMA_DESCRIPTOR_LEN, base_addr);

		spif_print_info(base_addr);

		spif_dma_start_signal(base_addr);

		spif_print_descriptor(spif_op);
		/*
		 *  The SPIF move data through DMA, and DMA and CPU modes
		 *  differ only between actively configuring registers and
		 *  configuring registers through the DMA descriptor
		 */
#if 0 //CFG_SPI_USE_DMA
		spif_cpu_start_transfer(base_addr);
#endif

		/* waiting DMA finish */
		while (!(readl(base_addr + SPIF_INT_STA_REG) &
				DMA_TRANS_DONE_INT)) {
			timeout--;
			if (!timeout) {
				spif_ctr_recover();
				SPIF_ERR("SPIF DMA transfer time_out\n");
				return -1;
			}
		}

		timeout = 0xfffffff;
		if (spif_op->trans_phase | SPIF_TX_TRANS_EN) {
			while ((readl(base_addr + SPIF_GC_REG) & SPIF_GC_NMODE_EN)) {
				timeout--;
				if (!timeout) {
					spif_ctr_recover();
					SPIF_ERR("SPIF DMA Write or Erase time_out\n");
					return -1;
				}
			}
		}

		invalidate_dcache(data_addr, ROUND_UP(data_len, CONFIG_SYS_CACHELINE_SIZE));
		writel(DMA_TRANS_DONE_INT, base_addr + SPIF_INT_STA_REG);
	}

	return 0;
}

