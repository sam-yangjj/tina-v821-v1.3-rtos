/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.
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

#include <stdio.h>
#include "spi.h"
#include "spinand_common.h"

#define SPI_NOT_INIT  0
#define SPI_ALREADY_INIT 1

extern unsigned int def_freq;

//Temporary use
unsigned int nand_frequencePar = 0;
unsigned int nand_sample_mode = 0xaaaaffff;
unsigned int nand_sample_delay = 0xaaaaffff;

static int spi_clk_on(u32 index)
{
	u32 cfg;

	/*spi0 clk reset*/
	cfg = readl(CCMU_BASE_ADDR + 0x0094);
	cfg &= (~(0x1<<4));
	writel(cfg, CCMU_BASE_ADDR + 0x0094);

	cfg = readl(CCMU_BASE_ADDR + 0x0094);
	cfg |= (0x1<<4);
	writel(cfg, CCMU_BASE_ADDR + 0x0094);

	/*spi0 clk gatting*/
	cfg = readl(CCMU_BASE_ADDR + 0x0084);
	cfg |= (0x1<<4);
	writel(cfg, CCMU_BASE_ADDR + 0x0084);

	return 0;
}

static void spi_clk_off(u32 index)
{
    return ;
}

s32 spi_set_clk(u32 index, u32 clock)
{
	u32 reg_val;
	u32 sclk0_src_sel, sclk0, sclk0_src, sclk0_pre_ratio_n, sclk0_src_t,
	    sclk0_ratio_m;
	u32 sclk0_reg_adr;

	sclk0_reg_adr = (CCMU_BASE_ADDR + 0x001C); /*SPI_CLK_REG*/
	/*close dclk and cclk*/
	if (clock == 0) {
		reg_val = readl(sclk0_reg_adr);
		reg_val &= (~(0x1U << 31));
		writel(reg_val, sclk0_reg_adr);

		return 0;
	}

	sclk0_src_sel = 1; /* PLL_PERI(1X) */
	sclk0 = clock;

	sclk0_src = SPI_CLOCK_SOURCE_FREQUENCY/ 1000000;
	/*sclk0_pre_ratio_n*/

	sclk0_pre_ratio_n = 3;
	if (sclk0_src > 4 * 16 * sclk0)
		sclk0_pre_ratio_n = 3;
	else if (sclk0_src > 2 * 16 * sclk0)
		sclk0_pre_ratio_n = 2;
	else if (sclk0_src > 1 * 16 * sclk0)
		sclk0_pre_ratio_n = 1;
	else
		sclk0_pre_ratio_n = 0;

	sclk0_src_t = sclk0_src >> sclk0_pre_ratio_n;

	/*sclk0_ratio_m*/
	sclk0_ratio_m = (sclk0_src_t / (sclk0)) - 1;
	if (sclk0_src_t % (sclk0))
		sclk0_ratio_m += 1;

	/*close clock*/
	reg_val = readl(sclk0_reg_adr);
	reg_val &= (~(0x1U << 31));
	writel(reg_val, sclk0_reg_adr);

	/*configure*/
	reg_val = readl(sclk0_reg_adr);
	/*clock source select*/
	reg_val &= (~(0x7 << 24));
	reg_val |= (sclk0_src_sel & 0x7) << 24;
	/*clock pre-divide ratio(N)*/
	reg_val &= (~(0x3 << 8));
	reg_val |= (sclk0_pre_ratio_n & 0x3) << 8;
	/*clock divide ratio(M)*/
	reg_val &= ~(0xf << 0);
	reg_val |= (sclk0_ratio_m & 0xf) << 0;
	writel(reg_val, sclk0_reg_adr);

	/* open clock*/
	reg_val = readl(sclk0_reg_adr);
	reg_val |= 0x1U << 31;
	writel(reg_val, sclk0_reg_adr);

	return 0;
}

void spi_gpio_init(u32 index)
{
	u32 reg_val;

	/* spi0 gipo
	spi0_miso:pc11 ([offset->0x0064,bit->15:12]:select func:3)
	spi0_cs0:pc10  ([offset->0x0064,bit->11:8]:select func:3)
	spi0_sclk:pc9  ([offset->0x0064,bit->7:4]:select func:3)
	spi0_mosi:pc8  ([offset->0x0064,bit->3:0]:select func:3)
	spi0_hold:pc7 ([offset->0x0060,bit->31:28]:select func:3)
	spi0_wp:pc6 ([offset->0x0060,bit->27:24]:select func:3)
	*/

	/*set pc6/pc7 func*/
	reg_val = readl(SPI_GPIO_BASE_ADDR + 0x60);
	reg_val &= ~(0xf << 24);
	reg_val &= ~(0xf << 28);
	reg_val |= (3 << 24);
	reg_val |= (3 << 28);
	writel(reg_val, SPI_GPIO_BASE_ADDR + 0x60);

	/*set pc8/pc9/pc10/pc11 func*/
	reg_val = readl(SPI_GPIO_BASE_ADDR + 0x64);
	reg_val &= ~(0xf << 0);
	reg_val &= ~(0xf << 4);
	reg_val &= ~(0xf << 8);
	reg_val &= ~(0xf << 12);
	reg_val |= (3 << 0);
	reg_val |= (3 << 4);
	reg_val |= (3 << 8);
	reg_val |= (3 << 12);
	writel(reg_val, SPI_GPIO_BASE_ADDR + 0x64);

	/*
	spi0_miso:pc11 (drv[offset->0x0078,bit->13:12]: 01)
	spi0_cs0:pc10  (drv[offset->0x0078,bit->9:8]: 01)
	spi0_sclk:pc9  (drv[offset->0x0078,bit->5:4]: 01)
	spi0_mosi:pc8  (drv[offset->0x0078,bit->1:0]: 01)
	spi0_hold:pc7 (drv[offset->0x0074,bit->29:28]: 01)
	spi0_wp:pc6 (drv[offset->0x0074,bit->25:24]: 01)
	*/

	/*set pc6/pc7 drv dirve strength*/
	reg_val = readl(SPI_GPIO_BASE_ADDR + 0x74);
	reg_val &= ~(0x3 << 24);
	reg_val &= ~(0x3 << 28);
	reg_val |= (0x1 << 24);
	reg_val |= (0x1 << 28);
	writel(reg_val, SPI_GPIO_BASE_ADDR + 0x74);

	/*set pc8/pc9/pc10/pc11 dirve strength*/
	reg_val = readl(SPI_GPIO_BASE_ADDR + 0x78);
	reg_val &= ~(0x3 << 0);
	reg_val &= ~(0x3 << 4);
	reg_val &= ~(0x3 << 8);
	reg_val &= ~(0x3 << 12);
	reg_val |= (0x1 << 0);
	reg_val |= (0x1 << 4);
	reg_val |= (0x1 << 8);
	reg_val |= (0x1 << 12);
	writel(reg_val, SPI_GPIO_BASE_ADDR + 0x78);

	/*spi0_cs0:pc10, spi0_hold:pc7, spi0_wp:pc6 pull*/
	reg_val = readl(SPI_GPIO_BASE_ADDR + 0x84);
	reg_val &= ~(0x3 << 12);
	reg_val &= ~(0x3 << 14);
	reg_val &= ~(0x3 << 20);
	reg_val |= (0x1 << 12);
	reg_val |= (0x1 << 14);
	reg_val |= (0x1 << 20);
	writel(reg_val, SPI_GPIO_BASE_ADDR + 0x84);
}

static void spi_gpio_deinit(u32 index)
{
	return;
}


static s32 wait_tc_complete(void)
{
	u32 timeout = 0xffffff;

	/*wait transfer complete*/
	while (!(readl(SPI_ISR) & (0x1 << 12))) {
		timeout--;
		if (!timeout)
			break;
	}
	if (timeout == 0) {
		printf("wait xfer complete timeout!\n");
		return -ERR_TIMEOUT;
	}

	return 0;
}

static void spi_set_sdm(unsigned int smod)
{
	unsigned int rval = readl(SPI_TCR) & (~(1 << 13));

	rval |= smod << 13;
	writel(rval, SPI_TCR);
}

static void spi_set_sdc(unsigned int sample)
{
	unsigned int rval = readl(SPI_TCR) & (~(1 << 11));

	rval |= sample << 11;
	writel(rval, SPI_TCR);
}

static void spi_set_sdc1(unsigned int sample)
{
	unsigned int rval = readl(SPI_TCR) & (~(1 << 15));

	rval |= sample << 15;
	writel(rval, SPI_TCR);
}

static void spi_set_sample_mode(unsigned int mode)
{
	unsigned int sample_mode[7] = {
		DELAY_NORMAL_SAMPLE, DELAY_0_5_CYCLE_SAMPLE,
		DELAY_1_CYCLE_SAMPLE, DELAY_1_5_CYCLE_SAMPLE,
		DELAY_2_CYCLE_SAMPLE, DELAY_2_5_CYCLE_SAMPLE,
		DELAY_3_CYCLE_SAMPLE
	};
	spi_set_sdm((sample_mode[mode] >> 8) & 0xf);
	spi_set_sdc((sample_mode[mode] >> 4) & 0xf);
	spi_set_sdc1(sample_mode[mode] & 0xf);
}

static void spi_set_sample_delay(unsigned char sample_delay)
{
	unsigned int rval = readl(SPI_SDC)&(~(0x3f << 0));

	rval |= sample_delay;
	writel(rval, SPI_SDC);
}

static void spi_samp_dl_sw_status(unsigned int status)
{
	unsigned int rval = readl(SPI_SDC);

	if (status)
		rval |= SPI_SAMP_DL_SW_EN;
	else
		rval &= ~SPI_SAMP_DL_SW_EN;

	writel(rval, SPI_SDC);
}

static void spi_samp_mode(unsigned int status)
{
	unsigned int rval = readl(SPI_GCR);

	if (status)
		rval |= SPI_SAMP_MODE_EN;
	else
		rval &= ~SPI_SAMP_MODE_EN;

	writel(rval, SPI_GCR);
}

void spi_sample_point_delay_set_legacy(u32 freq)
{
	unsigned int rval = 0;
	rval = readl(SPI_TCR);

	if (freq >= 60) {
		rval &= ~(SPI_SAMPLE_CTRL | SPI_SAMPLE_MODE);
		rval |= SPI_SAMPLE_CTRL;
	} else if (freq <= 24) {
		rval &= ~(SPI_SAMPLE_CTRL | SPI_SAMPLE_MODE);
		rval |= SPI_SAMPLE_MODE;
	} else {
		rval &= ~(SPI_SAMPLE_CTRL | SPI_SAMPLE_MODE);
	}

	writel(rval, SPI_TCR);
}

s32 spi_init(u32 spi_no)
{
	u32 rval;
	static int early_init_flag = SPI_NOT_INIT;

	if (early_init_flag == SPI_ALREADY_INIT)
		goto skip_hardware_init;

	spi_gpio_init(spi_no);
	spi_clk_on(spi_no);

	rval = SPI_SOFT_RST | SPI_TXPAUSE_EN | SPI_MASTER | SPI_ENABLE;
	writel(rval, SPI_GCR);

	writel(SPI_TXFIFO_RST | (SPI_TX_WL << 16) | (SPI_RX_WL), SPI_FCR);
	writel(SPI_ERROR_INT, SPI_IER);

skip_hardware_init:
	if (nand_frequencePar)
		spi_set_clk(spi_no, nand_frequencePar);
	else
		spi_set_clk(spi_no, def_freq);

	/*set ss to high,discard unused burst,SPI select signal
	 * polarity(low,1=idle)*/
	rval = SPI_SET_SS_1 | SPI_DHB | SPI_SS_ACTIVE0;
	printf("mode:0x%x sample delay:0x%x\n", nand_sample_mode, nand_sample_delay);
	if (nand_sample_delay == 0xaaaaffff || (nand_sample_delay == 0
				&& nand_sample_mode == 0)) {
#ifdef FPGA_PLATFORM
		nand_frequencePar = 24;
		rval &= ~(SPI_SAMPLE_CTRL | SPI_SAMPLE_MODE);
#else
		if (nand_frequencePar == 0)
			nand_frequencePar = def_freq;
		if (nand_frequencePar >= 60) {
			rval &= ~(SPI_SAMPLE_CTRL | SPI_SAMPLE_MODE);
			rval |= SPI_SAMPLE_CTRL;
		} else if (nand_frequencePar <= 24) {
			rval &= ~(SPI_SAMPLE_CTRL | SPI_SAMPLE_MODE);
			rval |= SPI_SAMPLE_MODE;
		} else
			rval &= ~(SPI_SAMPLE_CTRL | SPI_SAMPLE_MODE);
#endif
	} else {
		spi_samp_mode(1);
		spi_samp_dl_sw_status(1);
		spi_set_sample_mode(nand_sample_mode);
		spi_set_sample_delay(nand_sample_delay);
	}
	writel(rval | readl(SPI_TCR), SPI_TCR);
	early_init_flag = SPI_ALREADY_INIT;

	return 0;
}

s32 spi_deinit(u32 spi_no)
{
	u32 rval;

	rval = readl(SPI_GCR);
	rval &= (~(SPI_SOFT_RST | SPI_MASTER | SPI_ENABLE));
	writel(rval, SPI_GCR);

	spi_clk_off(spi_no);
	spi_gpio_deinit(spi_no);

	/* set ss to high,discard unused burst,SPI select signal polarity(low,1=idle) */
	rval = SPI_SET_SS_1 | SPI_DHB | SPI_SS_ACTIVE0;
	writel(rval, SPI_TCR);

	return 0;
}

void spi_sel_ss(u32 spi_no, u32 ssx)
{
	u32 rval = readl(SPI_TCR) & (~(3 << 4));
	rval |= ssx << 4;
	writel(rval, SPI_TCR);
}

void spi_config_dual_mode(u32 spi_no, u32 rxdual, u32 dbc, u32 stc)
{
	writel((rxdual << 28) | (dbc << 24) | (stc), SPI_BCC);
}


static s32 transmit_by_cpu(u32 spi_no, u32 tcnt, u8 *txbuf, u32 rcnt, u8 *rxbuf, u32 dummy_cnt)
{
	u32 i = 0, fcr;

	writel(0, SPI_IER);
	writel(0xffffffff, SPI_ISR); /*clear status register*/

	writel(tcnt, SPI_MTC);
	writel(tcnt + rcnt + dummy_cnt, SPI_MBC);

	/*read and write by cpu operation*/
	if (tcnt) {
		i = 0;
		while (i < tcnt) {
			if (((readl(SPI_FSR) >> 16) & 0x7f) == SPI_FIFO_SIZE)
				printf("TX FIFO size error!\n");
			writeb(*(txbuf + i), SPI_TXD);
			i++;
		}
	}
	/* start transmit */
	writel(readl(SPI_TCR) | SPI_EXCHANGE, SPI_TCR);
	if (rcnt) {
		i = 0;
		while (i < rcnt) {
			/*receive valid data*/
			while (((readl(SPI_FSR)) & 0xff) == 0)
				;
			*(rxbuf + i) = readb(SPI_RXD);
			i++;
		}
	}

	if (wait_tc_complete()) {
		printf("wait tc complete timeout!\n");
		return -ERR_TIMEOUT;
	}

	fcr = readl(SPI_FCR);
	fcr &= ~(SPI_TXDMAREQ_EN | SPI_RXDMAREQ_EN);
	writel(fcr, SPI_FCR);
	/* (1U << 11) | (1U << 10) | (1U << 9) | (1U << 8)) */
	if (readl(SPI_ISR) & (0xf << 8)) {
		printf("FIFO status error: 0x%lx!\n", readl(SPI_ISR));
		return NAND_OP_FALSE;
	}

	if (readl(SPI_TCR) & SPI_EXCHANGE) {
		printf("XCH Control Error!!\n");
	}

	writel(0xffffffff, SPI_ISR); /* clear  flag */
	return NAND_OP_TRUE;
}

s32 spi_rw(u32 spi_no, u32 tcnt, u8 *txbuf, u32 rcnt, u8 *rxbuf, u32 dummy_cnt)
{
	return transmit_by_cpu(spi_no, tcnt, txbuf, rcnt, rxbuf, dummy_cnt);
}