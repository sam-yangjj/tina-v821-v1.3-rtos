/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
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
/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                         clock control unit module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : ccu.c
* By      : Sunny
* Version : v1.0
* Date    : 2012-5-7
* Descript: clock control unit module.
* Update  : date                auther      ver     notes
*           2012-5-7 8:43:10    Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#include "ccu_i.h"
#include "platform.h"
#include "cpucfg_regs.h"
#include "hal_prcm.h"
#include "compiler_attributes.h"
#include "aw_io.h"

#define DO_NOT_CALIBRATION
/* ccu module registers base address */
struct ccu_reg_list *ccu_reg_addr;
struct ccu_pll_c0_cpux_reg0000 *ccu_pll_c0_cpux_reg_addr;
struct ccu_pll_ddr0_reg0010 *ccu_pll_ddr0_reg_addr;
struct ccu_pll_periph_reg0010 *ccu_pll_periph0_reg_addr;
struct ccu_pll_audio0_reg0020 *ccu_pll_audio0_reg_addr;
/* struct ccu_pll_periph1_reg0028 *ccu_pll_periph1_reg_addr; */

/* apb clock change notifier list */
struct notifier *apbs2_notifier_head;
u32 iosc_freq = 16000000;
u32 losc_freq = 32768;

#ifndef DO_NOT_CALIBRATION
static u32 filter_channel[10] = {0};
static u32 filter_count;
#endif

void dcxo_cali_start(u32 __maybe_unused *bk)
{
#ifndef DO_NOT_CALIBRATION
	u32 calibration_status, xo_ctrl;

	calibration_status = readl(IOSC_CLK_AUTO_CALI);
	xo_ctrl = readl(XO_CTRL);
	writel(readl(XO_CTRL) | (0xa), XO_CTRL);
	writel(0x7, IOSC_CLK_AUTO_CALI);

	bk[0] = calibration_status;
	bk[1] = xo_ctrl;
#endif
}

void dcxo_cali_end(__maybe_unused u32 *bk)
{
#ifndef DO_NOT_CALIBRATION
	u32 calibration_status, xo_ctrl;

	calibration_status = bk[0];
	xo_ctrl = bk[1];

	writel(xo_ctrl, XO_CTRL);
	writel(calibration_status, IOSC_CLK_AUTO_CALI);
#endif
}

void osc_freq_init(void)
{
#ifndef DO_NOT_CALIBRATION
	u32 count = 0;
	u32 value, sum = 0;
	u32 integer, decimal;
	u32 dcxo_status_bk[2] = {0};

	filter_count = 0;
	dcxo_cali_start(dcxo_status_bk);
	time_mdelay(50);

	while (1) {
		count++;
		if (count > 20)
			break;
		value = readl(IOSC_CLK_AUTO_CALI);
		time_mdelay(16);
		integer = (value >> DCXO_CALI_INTEGER_OFFSET);
		decimal = (value >> DCXO_CALI_DECIMAL_OFFSET) & 0xffff;
		value = integer * losc_freq + (losc_freq  * 65535 / decimal);
		if (value > 24000000 || value < 8000000)
			continue;
		sum = value + sum;
		filter_channel[filter_count] = value;
		filter_count++;
		if (filter_count == 5)
			break;

	}
	if (filter_count == 0)
		iosc_freq = 16000000;
	else
		iosc_freq = sum / filter_count;

	dcxo_cali_end(dcxo_status_bk);
#endif
}

static u32 __maybe_unused filter_sliding(u32 *channel, u32 value, u32 max_count)
{
	u32 *bk, *ch = channel + max_count - 1;
	u32 sum = 0;

	do {
		bk = ch;
		ch--;
		*bk = *ch;
		sum += *bk;
	} while (ch >= (channel + 1));

	*ch = value;
	sum = sum + *ch;

	return sum / max_count;
}

void osc_freq_filter(void)
{
#ifndef DO_NOT_CALIBRATION
	u32 integer, decimal;
	u32 value, sum;

	time_mdelay(16);
	value = readl(IOSC_CLK_AUTO_CALI);
	integer = (value >> DCXO_CALI_INTEGER_OFFSET);
	decimal = (value >> DCXO_CALI_DECIMAL_OFFSET) & 0xffff;
	value = integer * losc_freq + (losc_freq  * 65535 / decimal);

	if (value > 24000000 || value < 8000000)
		return ;

	if (filter_count < 10) {
		sum = iosc_freq * filter_count;
		filter_channel[filter_count] = value;
		sum = sum + value;
		filter_count++;
		iosc_freq = sum / filter_count;
	} else {
		iosc_freq = filter_sliding(&filter_channel[0], value, filter_count);
	}
#endif
}


/*
*********************************************************************************************************
*                                       INITIALIZE CCU
*
* Description:  initialize clock control unit.
*
* Arguments  :  none.
*
* Returns    :  OK if initialize ccu succeeded, others if failed.
*********************************************************************************************************
*/
s32 ccu_init(void)
{

	/* initialize ccu register address */
	ccu_reg_addr = (struct ccu_reg_list *)R_PRCM_REG_BASE;
	ccu_pll_c0_cpux_reg_addr = (struct ccu_pll_c0_cpux_reg0000 *)CCU_PLL_C0_REG;
	ccu_pll_ddr0_reg_addr = (struct ccu_pll_ddr0_reg0010 *)CCU_PLL_DDR0_REG;
	ccu_pll_periph0_reg_addr = (struct ccu_pll_periph_reg0010 *)CCU_PLL_PERIPH0_REG;
	ccu_pll_audio0_reg_addr = (struct ccu_pll_audio0_reg0020 *)CCU_PLL_AUDIO0_REG;
	/* ccu_pll_periph1_reg_addr = (struct ccu_pll_periph1_reg0028 *)CCU_PLL_PERIPH1_REG; */
#ifndef CFG_FPGA_PLATFORM
	/* setup cpus post div source to 200M(CCU_CPUS_POST_DIV) */
	/* FIXME: board in fix */
	/* u32 value;
	value = (ccu_get_sclk_freq(CCU_SYS_CLK_PLL3)) / CCU_CPUS_POST_DIV;
	if (value < 1) {
		[>to avoid PLL5 freq less than CCU_CPUS_POST_DIV<]
		value = 1;
	}
	ccu_reg_addr->cpus_clk_cfg.factor_m = value - 1;
	[>set ar100 clock source to PLL5<]
	ccu_set_mclk_src(CCU_MOD_CLK_CPUS, CCU_SYS_CLK_PLL3); */
#endif

	/* initialize apb notifier list */
	apbs2_notifier_head = NULL;

	/* ccu initialize succeeded */
	return OK;
}

/*
*********************************************************************************************************
*                                       EXIT CCU
*
* Description:  exit clock control unit.
*
* Arguments  :  none.
*
* Returns    :  OK if exit ccu succeeded, others if failed.
*********************************************************************************************************
*/
s32 ccu_exit(void)
{
	ccu_pll_c0_cpux_reg_addr = NULL;
	ccu_reg_addr = NULL;
	ccu_pll_periph0_reg_addr = NULL;
	ccu_pll_audio0_reg_addr = NULL;

	return OK;
}

void write_rtc_domain_reg(u32 reg, u32 value)
{
	writel((unsigned long)value, (unsigned long)reg);
}

u32 read_rtc_domain_reg(u32 reg)
{
	return readl((unsigned long)reg);
}

