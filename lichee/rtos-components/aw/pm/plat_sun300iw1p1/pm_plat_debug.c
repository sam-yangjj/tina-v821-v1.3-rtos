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
* Copyright (c) 2025-2029 Allwinner Technology Co., Ltd. ALL rights reserved.
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

#include <pm_plat_debug.h>
#ifdef CONFIG_PM_DEBUG_WATCHDOG_RESET
#include <sunxi_hal_rtc_watchdog.h>
#endif
#ifdef CONFIG_PM_DEBUG_POWERON_REGS_SHOW
#include <stdio.h>
#include <string.h>
#include <sunxi_hal_common.h>
#endif

/* watchdog */
#ifdef CONFIG_PM_DEBUG_WATCHDOG_RESET
#define DBG_WDT_TIMEOUT		CONFIG_PM_DEBUG_WATCHDOG_RESET_TIME
void pm_dbg_start_watchdog(void)
{
	uint32_t period;

	period = (DBG_WDT_TIMEOUT * 2) & 0x3ff;
	hal_rtc_watchdog_init();
	hal_rtc_watchdog_sel_clk(RTC_WDG_CLK_32000);
	hal_rtc_watchdog_set_period(period);
	hal_rtc_watchdog_set_mode(RTC_WDG_MODE_RST_SYS);
	hal_rtc_watchdog_start();
}

void pm_dbg_stop_watchdog(void)
{
	hal_rtc_watchdog_stop();
}
#endif

/* pwron regs*/
#ifdef CONFIG_PM_DEBUG_POWERON_REGS_SHOW
#define CCMU_AON_REGS_SHOW_NUM	(35)
#define PRCM_REGS_SHOW_NUM	(36)
#define PWRCTRL_RTC_SHOW_NUM	(19)
#define PWRCTRL_AON_SHOW_NUM	(14)
#define PWRCTRL_APP_SHOW_NUM	(4)
#define RCOSC_CAL_SHOW_NUM	(3)
#define PWRCTRL_REGS_SHOW_NUM	(PWRCTRL_RTC_SHOW_NUM + PWRCTRL_AON_SHOW_NUM + PWRCTRL_APP_SHOW_NUM)

static uint32_t dbg_regs_rec[CCMU_AON_REGS_SHOW_NUM + PRCM_REGS_SHOW_NUM + PWRCTRL_REGS_SHOW_NUM + RCOSC_CAL_SHOW_NUM] = {0};
void pm_pwron_regs_record(uint32_t rec)
{
	volatile uint32_t addr;
	volatile uint32_t tmp;
	uint32_t cnt;
	uint32_t index;
	int i;

	addr = (uint32_t)0x4a010000;
	if (rec == 0) {
		cnt = 0;
		printf("\nccmu_aon regs:");
	} else {
		memset(dbg_regs_rec, 0, sizeof(dbg_regs_rec));
	}
	for (i = 0; i < CCMU_AON_REGS_SHOW_NUM; i++) {
		tmp = addr + i * 0x4;
		if (rec == 0) {
			if ((cnt % 4) == 0)
				printf("\n0x%x  ", tmp);
			printf("0x%08x  ", dbg_regs_rec[i]);
			cnt++;
		} else {
			dbg_regs_rec[i] = hal_readl(tmp);
		}

		if (tmp == 0x4a010000) {
			addr += 0x1c;
		} else if (tmp == 0x4a010024) {
			addr += 0x18;
		} else if (tmp == 0x4a010040) {
			addr += 0x4;
		} else if (tmp == 0x4a010048) {
			addr += 0x34;
		} else if (tmp == 0x4a010080) {
			addr += 0x9c;
		} else if (tmp == 0x4a010124) {
			addr += 0x18;
		} else if (tmp == 0x4a01014c) {
			addr += 0x30;
		} else if (tmp == 0x4a010184) {
			addr += 0x178;
		} else if (tmp == 0x4a010300) {
			addr += 0x1c;
		} else if (tmp == 0x4a010320) {
			addr += 0x1c;
		} else if (tmp == 0x4a010340) {
			addr += 0x4;
		} else if (tmp == 0x4a010348) {
			addr += 0x34;
		} else if (tmp == 0x4a010380) {
			addr += 0x7c;
		} else if (tmp == 0x4a01040c) {
			addr += 0xf0;
		} else if (tmp == 0x4a010508) {
			addr += 0x4;
		} else if (tmp == 0x4a010510) {
			addr += 0x4;
		} else if (tmp == 0x4a010518) {
			addr += 0x34;
		} else if (tmp == 0x4a010550) {
			addr += 0x1c;
		} else if (tmp == 0x4a010574) {
			addr += 0x8;
		}
		if ((rec == 0) && ((addr + i * 0x4) != tmp))
			cnt = 0;
	}

	addr = (uint32_t)0x4a000000;
	index = CCMU_AON_REGS_SHOW_NUM;
	if (rec == 0) {
		cnt = 0;
		printf("\nprcm regs:");
	}
	for (i = 0; i < PRCM_REGS_SHOW_NUM; i++) {
		tmp = addr + i * 0x4;
		if (rec == 0) {
			if ((cnt % 4) == 0)
				printf("\n0x%x  ", tmp);
			printf("0x%08x  ", dbg_regs_rec[index + i]);
			cnt++;
		} else {
			dbg_regs_rec[index + i] = hal_readl(tmp);
		}

		if (tmp == 0x4a000004) {
			addr += 0x4;
		} else if (tmp == 0x4a000040) {
			addr += 0xc;
		} else if (tmp == 0x4a000068) {
			addr += 0x4;
		} else if (tmp == 0x4a000070) {
			addr += 0xc;
		} else if (tmp == 0x4a000088) {
			addr += 0x4;
		} else if (tmp == 0x4a000090) {
			addr += 0x6c;
		} else if (tmp == 0x4a000104) {
			addr += 0x84;
		} else if (tmp == 0x4a00018c) {
			addr += 0x30;
		} else if (tmp == 0x4a0001c8) {
			addr += 0x4;
		}
		if ((rec == 0) && ((addr + i * 0x4) != tmp))
			cnt = 0;
	}

	addr = (uint32_t)0x4a000800;
	index = CCMU_AON_REGS_SHOW_NUM + PRCM_REGS_SHOW_NUM;
	if (rec == 0) {
		cnt = 0;
		printf("\npowerctrl_rtc regs:");
	}
	for (i = 0; i < PWRCTRL_RTC_SHOW_NUM; i++) {
		tmp = addr + i * 0x4;
		if (rec == 0) {
			if ((cnt % 4) == 0)
				printf("\n0x%x  ", tmp);
			printf("0x%08x  ", dbg_regs_rec[index + i]);
			cnt++;
		} else {
			dbg_regs_rec[index + i] = hal_readl(tmp);
		}

		if (tmp == 0x4a000808) {
			addr += 0x4;
		} else if (tmp == 0x4a000814) {
			addr += 0x18;
		} else if (tmp == 0x4a000844) {
			addr += 0x8;
		} else if (tmp == 0x4a000850) {
			addr += 0xc;
		} else if (tmp == 0x4a000864) {
			addr += 0x8;
		} else if (tmp == 0x4a000874) {
			addr += 0x8;
		} else if (tmp == 0x4a000880) {
			addr += 0xc;
		}
		if ((rec == 0) && ((addr + i * 0x4) != tmp))
			cnt = 0;
	}

	addr = (uint32_t)0x4a011000;
	index = CCMU_AON_REGS_SHOW_NUM + PRCM_REGS_SHOW_NUM + PWRCTRL_RTC_SHOW_NUM;
	if (rec == 0) {
		cnt = 0;
		printf("\npowerctrl_aon regs:");
	}
	for (i = 0; i < PWRCTRL_AON_SHOW_NUM; i++) {
		tmp = addr + i * 0x4;
		if (rec == 0) {
			if ((cnt % 4) == 0)
				printf("\n0x%x  ", tmp);
			printf("0x%08x  ", dbg_regs_rec[index + i]);
			cnt++;
		} else {
			dbg_regs_rec[index + i] = hal_readl(tmp);
		}

		if (tmp == 0x4a011010) {
			addr += 0xc;
		} else if (tmp == 0x4a011020) {
			addr += 0x1c;
		} else if (tmp == 0x4a011044) {
			addr += 0x18;
		} else if (tmp == 0x4a011070) {
			addr += 0x10;
		}
		if ((rec == 0) && ((addr + i * 0x4) != tmp))
			cnt = 0;
	}

	addr = (uint32_t)0x43045000;
	index = CCMU_AON_REGS_SHOW_NUM + PRCM_REGS_SHOW_NUM + PWRCTRL_RTC_SHOW_NUM + PWRCTRL_AON_SHOW_NUM;
	if (rec == 0) {
		cnt = 0;
		printf("\npowerctrl_app regs:");
	}
	for (i = 0; i < PWRCTRL_APP_SHOW_NUM; i++) {
		tmp = addr + i * 0x4;
		if (rec == 0) {
			if ((cnt % 4) == 0)
				printf("\n0x%x  ", tmp);
			printf("0x%08x  ", dbg_regs_rec[index + i]);
			cnt++;
		} else {
			dbg_regs_rec[index + i] = hal_readl(tmp);
		}
	}

	addr = (uint32_t)0x4a001408;
	index = CCMU_AON_REGS_SHOW_NUM + PRCM_REGS_SHOW_NUM + PWRCTRL_RTC_SHOW_NUM + PWRCTRL_AON_SHOW_NUM + PWRCTRL_APP_SHOW_NUM;
	if (rec == 0) {
		cnt = 0;
		printf("\nrcosc_cal regs:");
	}
	for (i = 0; i < RCOSC_CAL_SHOW_NUM; i++) {
		tmp = addr + i * 0x4;
		if (rec == 0) {
			if ((cnt % 4) == 0)
				printf("\n0x%x  ", tmp);
			printf("0x%08x  ", dbg_regs_rec[index + i]);
			cnt++;
		} else {
			dbg_regs_rec[index + i] = hal_readl(tmp);
		}
	}
	if (rec == 0)
		printf("\n");
}
#endif /* CONFIG_PM_DEBUG_POWERON_REGS_SHOW */
