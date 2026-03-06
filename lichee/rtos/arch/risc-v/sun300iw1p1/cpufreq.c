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
#include "cpufreq.h"
#include <hal_clk.h>
#include <stdio.h>
/*
 * new pattern to set cpu frequency
 */
#define E907_CLOCK_REGISTER (0x4a010584UL)
#define E907_CLOCK_SOURCE_BW (3)
#define E907_CLOCK_SOURCE_BS (24)
#define E907_CLOCK_DIV_BW (5)
#define E907_CLOCK_DIV_BS (0)
#define E907_DEFAULT_VOLTAGE (900) //900mv, cpu voltage adjust is not supported

typedef enum{
	CLK_SRC_HOSC         = 0U,
	CLK_SRC_VIDEOPLL2X   = 1U,
	CLK_SRC_RC1M         = 2U,
	CLK_SRC_CPUPLL       = 4U,
	CLK_SRC_PERIPLL1024M = 5U,
	CLK_SRC_PERIPLL614M  = 6U,
} e907_clock_source_t;

typedef struct cpu_freq_setting
{
	const uint32_t freq;
	const e907_clock_source_t clk_source;
	const uint32_t clk_div;
} cpu_freq_setting_t;

static cpu_freq_setting_t cpu_freq_table[] =
{
	{ 40000000, CLK_SRC_HOSC, 1},
	{ 64000000, CLK_SRC_PERIPLL1024M, 16},
	{ 128000000, CLK_SRC_PERIPLL1024M, 8},
	{ 256000000, CLK_SRC_PERIPLL1024M, 4},
	{ 307000000, CLK_SRC_PERIPLL614M, 2},
	{ 512000000, CLK_SRC_PERIPLL1024M, 2},
	{ 614000000, CLK_SRC_PERIPLL614M, 1},
};

static void set_reg(unsigned long addr, uint32_t val, uint8_t bw, uint8_t bs)
{
    uint32_t mask = (1UL << bw) - 1UL;
    uint32_t tmp = 0;

    tmp = readl(addr);
    tmp &= ~(mask << bs);

    writel(tmp | ((val & mask) << bs), addr);
}

uint32_t get_available_cpu_freq_num(void)
{
	return sizeof(cpu_freq_table)/sizeof(cpu_freq_table[0]);
}

int get_available_cpu_freq(uint32_t freq_index, uint32_t *cpu_freq)
{
	return cpu_freq_table[freq_index].freq;
}

int get_available_cpu_freq_info(uint32_t freq_index, uint32_t *cpu_freq, uint32_t *cpu_voltage)
{
	if (freq_index < 0 || freq_index >= get_available_cpu_freq_num())
		return -1;

	*cpu_freq = cpu_freq_table[freq_index].freq;
	*cpu_voltage = E907_DEFAULT_VOLTAGE;
	return 0;
}

int get_cpu_voltage(uint32_t *cpu_voltage)
{
	*cpu_voltage = E907_DEFAULT_VOLTAGE;
	return 0;
}

int get_cpu_freq(uint32_t *cpu_freq)
{
	hal_clk_t clk = NULL;
	uint32_t clk_freq;

	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_E907);
	if (!clk)
	{
		return -1;
	}

	clk_freq = hal_clk_get_rate(clk);
	hal_clock_put(clk);

	if (clk_freq == 0)
	{
		return -2;
	}

	*cpu_freq = clk_freq;
	return 0;
}

int set_cpu_freq(uint32_t target_freq)
{
	uint32_t i = 0, size = sizeof(cpu_freq_table)/sizeof(cpu_freq_table[0]);
	cpu_freq_setting_t *freq_setting;
	uint32_t cur_cpu_freq = 0;
	uint8_t enable_videopll2x_flag = 0;
	hal_clk_t clk_videopll2x;

	for (i = 0; i < size; i++) {
		if (cpu_freq_table[i].freq == target_freq) {
			freq_setting = &cpu_freq_table[i];
			break;
		}
	}

	if (i == size) {
		return -1;
	}

	if (get_cpu_freq(&cur_cpu_freq)) {
		return -2;
	}
	printf("%s: %u Hz->%u Hz\n", __func__, cur_cpu_freq, target_freq);

	/*
	 * if old or new clock source is dcxo as videopll2x is
	 * disabled, cpu will hang-up while switching clock source.
	 * keep videopll2x enabled while switch can make it through.
	 */
	clk_videopll2x = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_PLL_VIDEO_2X);
	if (!clk_videopll2x) {
		return -3;
	}

	if (HAL_CLK_STATUS_DISABLED == hal_clock_is_enabled(clk_videopll2x)) {
		hal_clock_enable(clk_videopll2x);

		if (HAL_CLK_STATUS_ENABLED != hal_clock_is_enabled(clk_videopll2x)) {
			hal_clock_put(clk_videopll2x);
			return -4;
		}
		enable_videopll2x_flag = 1;
	}

	/* upper or lower clk_src freq has different configuration order, to make sure not exceed dc restrict freq */
	if (target_freq > cur_cpu_freq) {
		set_reg(E907_CLOCK_REGISTER, freq_setting->clk_div - 1, E907_CLOCK_DIV_BW, E907_CLOCK_DIV_BS);
		set_reg(E907_CLOCK_REGISTER, freq_setting->clk_source, E907_CLOCK_SOURCE_BW, E907_CLOCK_SOURCE_BS);
	} else if (target_freq < cur_cpu_freq) {
		set_reg(E907_CLOCK_REGISTER, freq_setting->clk_source, E907_CLOCK_SOURCE_BW, E907_CLOCK_SOURCE_BS);
		set_reg(E907_CLOCK_REGISTER, freq_setting->clk_div - 1, E907_CLOCK_DIV_BW, E907_CLOCK_DIV_BS);
	}

	if (enable_videopll2x_flag) {
		hal_clock_disable(clk_videopll2x);
	}
	hal_clock_put(clk_videopll2x);

	return 0;
}