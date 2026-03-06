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
#include <stdlib.h>
#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <ccmu/common_ccmu.h>
#include "../../source/ccmu/sunxi-ng/ccu-sun20iw2-aon.h"
#ifdef CONFIG_OS_MELIS
#include <rtthread.h>
#define CHECK_FREESIZE
#endif

#ifndef CLK_MCU_NUMBER
#define CLK_MCU_NUMBER 0
#endif
#ifndef CLK_RTC_NUMBER
#define CLK_RTC_NUMBER 0
#endif
#ifndef CLK_R_NUMBER
#define CLK_R_NUMBER 0
#endif
#ifndef RST_R_BUS_NUMBER
#define RST_R_BUS_NUMBER 0
#endif

int clk_number[] = {
	CLK_SRC_NUMBER,
	CLK_RTC_NUMBER,
	CLK_NUMBER,
	CLK_AON_NUMBER,
	CLK_R_NUMBER,
	CLK_MCU_NUMBER,
	0
};

int reset_number[] = {
	RST_BUS_NUMBER,
	RST_R_BUS_NUMBER,
	0,
};

char *strict_clks[] = {
	"pll-ddr0",
	"riscv",
	"pll-cpux",
	"pll-periph0-parent",
	"riscv-axi",
	"apb1",
	"fanout-27m",
	"fix-losc",
	"rc-16m",
	"ext-32k",
	"rc-hf",
	"pclk-spc-1",
	"pclk-spc-2",
	"pclk-spc",
	NULL,
};

char *clk_type_name[] = {
	"HAL_SUNXI_FIXED_CCU",
	"HAL_SUNXI_RTC_CCU",
	"HAL_SUNXI_CCU",
	"HAL_SUNXI_AON_CCU",
	"HAL_SUNXI_R_CCU",
};

int is_strict_clk(hal_clk_t clk)
{
	int i;
	for (i = 0; strict_clks[i] != NULL; i++)
	{
		if (!strcmp(clk->name, strict_clks[i]))
			return 1;
	}

	return 0;
}

int is_dcxo_clk(hal_clk_t clk)
{
	if (!strncmp(clk->name, "dcxo", 4))
		return 1;
	return 0;
}

#ifdef CHECK_FREESIZE
static uint32_t esMEMS_FreeMemSize(void)
{
	uint32_t total, used, max_used;
	uint32_t aval;

	rt_memory_info(&total, &used, &max_used);
	aval = total - used;
	return aval;
}
#endif

int cmd_test_ng_ccmu(int argc, char **argv)
{
	int i, j;

	hal_clk_type_t clk_type;
	hal_clk_id_t   clk_id;
	hal_clk_t clk, p_clk;
	u32  old_rate;

#ifdef CCU_RESET_TSET
	hal_reset_type_t reset_type;
	hal_reset_id_t  reset_id;
	hal_clk_status_t clk_status;
	struct reset_control *reset;
	int reset_status;
	u32 new_rate;
#endif
	printf("clock\t\t\t\t\t type\t\t\t\t\t parent\t\t\t\t\t rate\n");
#ifdef CHECK_FREESIZE
	printf("free size = 0x%x\n", esMEMS_FreeMemSize());
#endif
	for (i = HAL_SUNXI_FIXED_CCU; i < HAL_SUNXI_CCU_NUMBER; i++)
	{
		clk_type = i;
		for (j = 0; j < clk_number[i]; j++)
		{
			clk_id = j;
			clk = hal_clock_get(clk_type, clk_id);
			if (!clk) {
				printf("fail to get clk\n");
				continue;
			}

			p_clk = hal_clk_get_parent(clk);

			old_rate = hal_clk_get_rate(clk);
			if (p_clk)
				printf("%-20s\t\t\t %-20s\t\t\t %-15s\t\t\t %u\n", clk->name, clk_type_name[i], p_clk->name, old_rate);
			else
				printf("%-20s\t\t\t %-20s\t\t\t NULL\t\t\t\t\t %u\n", clk->name, clk_type_name[i], old_rate);

		}
	}
	if (argc == 4) {
		clk_type = strtol(argv[1], NULL, 0);
		clk_id = strtol(argv[2], NULL, 0);
		old_rate = strtol(argv[3], NULL, 0);
		clk = hal_clock_get(clk_type, clk_id);
		hal_clk_set_rate(clk, old_rate);
		printf("set %s to %u\n", clk->name, old_rate);
	}

#ifdef CCU_RESET_TSET
#ifdef CHECK_FREESIZE
	printf("free size = 0x%x\n", esMEMS_FreeMemSize());
#endif
	for (i = HAL_SUNXI_RESET; i < HAL_SUNXI_RESET_NUMBER; i++)
	{
		reset_type = i;
		for (j = 0; j < reset_number[i]; j++)
		{
			reset_id = j;

			printf("reset: get reset control, type:%d, id: %d\n", reset_type, reset_id);
			reset = hal_reset_control_get(reset_type, reset_id);

			printf("reset: control deassert\n");
			hal_reset_control_deassert(reset);

			reset_status = hal_reset_control_status(reset);
			printf("reset status: %s", reset_status ? "assert" : "deassert");

			printf("reset: put reset control, type:%d, id: %d\n", reset_type, reset_id);
			hal_reset_control_put(reset);
		}
	}

#ifdef CHECK_FREESIZE
	printf("free size = 0x%x\n", esMEMS_FreeMemSize());
#endif
#endif
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_ng_ccmu, hal_ccmu, sunxi - ng ccmu hal APIs tests)
