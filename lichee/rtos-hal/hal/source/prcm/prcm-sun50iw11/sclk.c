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
*                                	  	  clock control unit module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : sclk.c
* By      : Sunny
* Version : v1.0
* Date    : 2012-5-7
* Descript: system clock management.
* Update  : date                auther      ver     notes
*           2012-5-7 8:43:10	Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#include <delay.h>
#include "ccu_i.h"
#include "errno.h"
#include "hal_prcm.h"
#include "stdio.h"
#include "aw_common.h"
#include "compiler_attributes.h"


static ccu_pll_audio0_reg_t *pll_audio0 = (ccu_pll_audio0_reg_t *)CCU_PLL_AUDIO0_REG;
static ccu_pll_audio1_reg_t *pll_audio1 = (ccu_pll_audio1_reg_t *)CCU_PLL_AUDIO1_REG;
static ccu_pll_audio0_pat0_reg_t *pll_audio0_pat0 = (ccu_pll_audio0_pat0_reg_t *)CCU_PLL_AUDIO0_PAT0_REG;
static ccu_pll_audio1_pat0_reg_t *pll_audio1_pat0 = (ccu_pll_audio1_pat0_reg_t *)CCU_PLL_AUDIO1_PAT0_REG;

static s32 ccu_set_pll_audio0(u32 freq)
{
	static u32 old_freq = 0;
	s32 loop = 0;

	if (freq == old_freq)
		return 0;

	UNUSED(pll_audio0_pat0);

	old_freq = freq;

	/* make sure pll new mode is disable */
	pll_audio0->lock_en = 0;
	pll_audio0->lock_st = 0;

	switch (freq) {
	default:
		printf("not support freq:%d, now set freq:24576000Hz.\n", freq);
	case 24576000:
		#if 0
		/* *pll_audio0_pat0 = 0xc00126e9;*/
		/* bit0 - bit16, wave bottom */
		pll_audio0_pat0->wave_bot = 0x126e9;
		/* bit17 - bit18, frequency */
		pll_audio0_pat0->freq = 0;
		/* bit19, 0:24M, 1:12M, SDM clk select */
		pll_audio0_pat0->clk_sel = 0;
		/* bit20 - bit28, wave step */
		pll_audio0_pat0->wave_step = 0;
		/* bit29 - bit30, Spread frequency mode */
		pll_audio0_pat0->freq_mode = 0;
		/* bit31, 0-disable, 1-enable, Sigma-Delta Pattern Enable */
		pll_audio0_pat0->enable = 1;

		/* fix for arch32 or arch64 */
		*((unsigned long *)pll_audio0) = 0x890b1701;
		#endif
		break;
	case 22579200:
		#if 0
		/* *pll_audio1_pat0 = 0xc001288d;*/
		/* bit0 - bit16, wave bottom */
		pll_audio1_pat0->wave_bot = 0x1288d;
		/* bit17 - bit18, frequency */
		pll_audio1_pat0->freq = 0;
		/* bit19, 0:24M, 1:12M, SDM clk select */
		pll_audio1_pat0->clk_sel = 0;
		/* bit20 - bit28, wave step */
		pll_audio1_pat0->wave_step = 0;
		/* bit29 - bit30, Spread frequency mode */
		pll_audio1_pat0->freq_mode = 0;
		/* bit31, 0-disable, 1-enable, Sigma-Delta Pattern Enable */
		pll_audio1_pat0->enable = 1;

		*((unsigned long *)pll_audio1) = 0x890b1501;
		#endif
		break;
	}

	if (pll_audio0->enable) {
		pll_audio0->lock_st = 1;
		pll_audio0->lock_en = 1;
		while (pll_audio0->lock_st && (--loop > 0)) {
			udelay(1);
		}
		pll_audio0->lock_en = 0;
		pll_audio0->lock_st = 0;
	}

	return 0;
}

static s32 ccu_set_pll_audio1(u32 freq)
{
	static u32 old_freq = 0;
	s32 loop = 0;

	if (freq == old_freq)
		return 0;

	old_freq = freq;

	/* make sure pll new mode is disable */
	pll_audio1->lock_en = 0;
	pll_audio1->lock_st = 0;

	switch (freq) {
	default:
		printf("not support freq:%d, now set freq:24576000Hz.\n", freq);
	case 24576000:
		/* *pll_audio1_pat0 = 0xc00126e9;*/
		/* bit0 - bit16, wave bottom */
		pll_audio1_pat0->wave_bot = 0x126e9;
		/* bit17 - bit18, frequency */
		pll_audio1_pat0->freq = 0;
		/* bit19, 0:24M, 1:12M, SDM clk select */
		pll_audio1_pat0->clk_sel = 0;
		/* bit20 - bit28, wave step */
		pll_audio1_pat0->wave_step = 0;
		/* bit29 - bit30, Spread frequency mode */
		pll_audio1_pat0->freq_mode = 0;
		/* bit31, 0-disable, 1-enable, Sigma-Delta Pattern Enable */
		pll_audio1_pat0->enable = 1;

		/* fix for arch32 or arch64 */
		*((unsigned long *)pll_audio1) = 0x890b1701;
		break;
	case 22579200:
		/* *pll_audio1_pat0 = 0xc001288d;*/
		/* bit0 - bit16, wave bottom */
		pll_audio1_pat0->wave_bot = 0x1288d;
		/* bit17 - bit18, frequency */
		pll_audio1_pat0->freq = 0;
		/* bit19, 0:24M, 1:12M, SDM clk select */
		pll_audio1_pat0->clk_sel = 0;
		/* bit20 - bit28, wave step */
		pll_audio1_pat0->wave_step = 0;
		/* bit29 - bit30, Spread frequency mode */
		pll_audio1_pat0->freq_mode = 0;
		/* bit31, 0-disable, 1-enable, Sigma-Delta Pattern Enable */
		pll_audio1_pat0->enable = 1;

		*((unsigned long *)pll_audio1) = 0x890b1501;
		break;
	case 90316800:
		/* *pll_audio1_pat0 = 0xc001288d;*/
		/* bit0 - bit16, wave bottom */
		pll_audio1_pat0->wave_bot = 0x1288d;
		/* bit17 - bit18, frequency */
		pll_audio1_pat0->freq = 0;
		/* bit19, 0:24M, 1:12M, SDM clk select */
		pll_audio1_pat0->clk_sel = 0;
		/* bit20 - bit28, wave step */
		pll_audio1_pat0->wave_step = 0;
		/* bit29 - bit30, Spread frequency mode */
		pll_audio1_pat0->freq_mode = 0;
		/* bit31, 0-disable, 1-enable, Sigma-Delta Pattern Enable */
		pll_audio1_pat0->enable = 1;

		*((unsigned long *)pll_audio1) = 0x89021501;
		break;
	case 98304000:
		/* *pll_audio1_pat0 = 0xc00126e9;*/
		/* bit0 - bit16, wave bottom */
		pll_audio1_pat0->wave_bot = 0x126e9;
		/* bit17 - bit18, frequency */
		pll_audio1_pat0->freq = 0;
		/* bit19, 0:24M, 1:12M, SDM clk select */
		pll_audio1_pat0->clk_sel = 0;
		/* bit20 - bit28, wave step */
		pll_audio1_pat0->wave_step = 0;
		/* bit29 - bit30, Spread frequency mode */
		pll_audio1_pat0->freq_mode = 0;
		/* bit31, 0-disable, 1-enable, Sigma-Delta Pattern Enable */
		pll_audio1_pat0->enable = 1;

		*((unsigned long *)pll_audio1) = 0x89021701;
		break;
	}
	if (pll_audio1->enable) {
		pll_audio1->lock_st = 1;
		pll_audio1->lock_en = 1;
		while (pll_audio1->lock_st && (--loop > 0)) {
			udelay(1);
		}
		pll_audio1->lock_en = 0;
		pll_audio1->lock_st = 0;
	}

	return 0;
}

/*
*********************************************************************************************************
*                                      SET SOURCE FREQUENCY
*
* Description: 	set the frequency of a specific source clock.
*
* Arguments  : 	sclk : the source clock ID which we want to set frequency.
*				freq : the frequency which we want to set.
*
* Returns    : 	OK if set source frequency succeeded, others if failed.
*********************************************************************************************************
*/
s32 ccu_set_sclk_freq(u32 sclk, __maybe_unused u32 freq)
{
	switch (sclk) {
	case CCU_SYS_CLK_AUDIO0:
		return ccu_set_pll_audio0(freq);
	case CCU_SYS_CLK_AUDIO1:
		return ccu_set_pll_audio1(freq);
	default:
		pr_warning("invaid clock id (%d) when set freq(%d)\n", sclk);
		return -EINVAL;
	}
	/* un-reached */
}
/*
*********************************************************************************************************
*                                     GET SOURCE FREQUENCY
*
* Description: 	get the frequency of a specific source clock.
*
* Arguments  : 	sclk : the source clock ID which we want to get frequency.
*
* Returns    : 	frequency of the specific source clock.
*********************************************************************************************************
*/
u32 ccu_get_sclk_freq(u32 sclk)
{
	switch (sclk) {
	case CCU_SYS_CLK_LOSC: {
		return losc_freq;
	}
	case CCU_SYS_CLK_HOSC: {
		return CCU_HOSC_FREQ;
	}
	case CCU_SYS_CLK_CPUX: {
		/* maybe should delete */
		ccu_pll_c0_cpux_reg0000_t pll_c0 = *(ccu_pll_c0_cpux_reg_addr);
		return (CCU_HOSC_FREQ * (pll_c0.factor_n + 1)) /
		       ((pll_c0.factor_m + 1) * (1 << pll_c0.factor_p));
	}
	case CCU_SYS_CLK_CPUS: {
		switch (ccu_reg_addr->cpus_clk_cfg.src_sel) {
		case 0: {
			/* cpus clock source is losc */
			return CCU_HOSC_FREQ;
		}
		case 1: {
			/* cpus clock source is hosc */
			return losc_freq;
		}
		case 2: {
			/* cpus clock source is internal-osc */
			return iosc_freq;
		}
		case 3: {
			/* cpus clock source is pll_peri0(2x) */
			return ccu_get_sclk_freq(CCU_SYS_CLK_PERI_2X) /
			       (1 << ccu_reg_addr->cpus_clk_cfg.factor_n) /
			       (ccu_reg_addr->cpus_clk_cfg.factor_m + 1);
		}
		case 4: {
			return ccu_get_sclk_freq(CCU_SYS_CLK_AUDIO0_DIV2) /
			       (1 << ccu_reg_addr->cpus_clk_cfg.factor_n) /
			       (ccu_reg_addr->cpus_clk_cfg.factor_m + 1);
		}
		default: {
			return 0;
		}
		}
	}
	case CCU_SYS_CLK_AHBS: {
		return ccu_get_sclk_freq(CCU_SYS_CLK_CPUS);
	}
	case CCU_SYS_CLK_APBS1: {
		return ccu_get_sclk_freq(CCU_SYS_CLK_AHBS) /
		       ccu_get_mclk_div(CCU_MOD_CLK_APBS1);
	}
	case CCU_SYS_CLK_APBS2: {
		switch (ccu_reg_addr->apbs2_cfg.src_sel) {
		case 0: {
			/* cpus clock source is losc */
			return CCU_HOSC_FREQ;
		}
		case 1: {
			/* cpus clock source is hosc */
			return losc_freq;
		}
		case 2: {
			/* cpus clock source is internal-osc */
			return iosc_freq;
		}
		case 3: {
			/* cpus clock source is pll6 */
			return ccu_get_sclk_freq(CCU_SYS_CLK_PLL3) /
			       (ccu_reg_addr->apbs2_cfg.factor_m + 1) /
			       (1 << ccu_reg_addr->apbs2_cfg.factor_n);
		}
		default: {
			return 0;
		}
		}
	}
	case CCU_SYS_CLK_PERI_1X: {
		/* output=24M*N*K/2 */
		ccu_pll_periph_reg0010_t pll_periph0 =
			*(ccu_pll_periph0_reg_addr);

		return ((long long)CCU_HOSC_FREQ * (pll_periph0.factor_n + 1) /
			(pll_periph0.factor_m + 1) /
			(pll_periph0.factor_p0 + 1) / 2);
	}
	case CCU_SYS_CLK_PERI_2X: {
		ccu_pll_periph_reg0010_t pll_periph0 =
			*(ccu_pll_periph0_reg_addr);
		return ((long long)CCU_HOSC_FREQ * (pll_periph0.factor_n + 1) /
			(pll_periph0.factor_m + 1) /
			(pll_periph0.factor_p0 + 1));
	}
	case CCU_SYS_CLK_AUDIO0_DIV2: {
		ccu_pll_audio0_reg0020_t pll_audio0 =
			*(ccu_pll_audio0_reg_addr);
		return ((long long)CCU_HOSC_FREQ * (pll_audio0.factor_n + 1) /
			(pll_audio0.factor_m + 1) / (pll_audio0.factor_p0 + 1));
	}
	}
	pr_warning("invalid clock id for get source freq\n");
	return 0;
}

s32 ccu_set_sclk_onoff(u32 sclk, s32 onoff)
{
	switch (sclk) {
	case CCU_SYS_CLK_C0:
		{
			ccu_pll_c0_cpux_reg_addr->enable = onoff;
			return OK;
		}
	case CCU_SYS_CLK_DDR0:
		{
			ccu_pll_ddr0_reg_addr->enable = onoff;
			return OK;
		}
	case CCU_SYS_CLK_PERI0:
		{
			ccu_pll_periph0_reg_addr->enable = onoff;
			return OK;
		}
	case CCU_SYS_CLK_AUDIO0:
		{
			pll_audio0->enable = onoff;
			return OK;
		}
	case CCU_SYS_CLK_AUDIO1:
		{
			pll_audio1->enable = onoff;
			return OK;
		}
	default:
		pr_warning("invalid clock id for get source freq\n");
	}

	return 0;
}
