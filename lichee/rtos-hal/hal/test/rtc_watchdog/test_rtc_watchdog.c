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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_time.h>
#include <sunxi_hal_rtc_watchdog.h>

extern void udelay(unsigned int us);

static int cmd_rtc_watchdog_callback(void)
{
	hal_log_info("rtc_watchdog enter irq\n");
	hal_rtc_watchdog_deinit();
	return 0;
}

int cmd_test_rtc_watchdog_irq(int argc, char **argv)
{
	uint32_t sec;
	uint32_t period;

	hal_log_info("cmd test rtc watchdog irq\n");

	if (argc != 2) {
		hal_log_info("param error\n");
		return -1;
	}
	sec = strtol(argv[1], NULL, 0);
	period = sec * 2;
	hal_rtc_watchdog_init();
	hal_rtc_watchdog_register_callback(cmd_rtc_watchdog_callback);
	hal_rtc_watchdog_sel_clk(RTC_WDG_CLK_32000);
	hal_rtc_watchdog_set_period(period);
	hal_rtc_watchdog_set_mode(RTC_WDG_MODE_INTERRUPT);
	hal_rtc_watchdog_start();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_rtc_watchdog_irq, test_rtc_watchdog_irq, rtc_watchdog_test_tools);

int cmd_test_rtc_watchdog_timeout_reboot(int argc, char **argv)
{
	uint32_t to;
	uint32_t period;

	hal_log_info("cmd test rtc watchdog timeout reboot\n");

	if (argc != 1) {
		hal_log_info("param error\n");
		return -1;
	}

	/* 5s timeout */
	period = 10;
	hal_rtc_watchdog_init();
	hal_rtc_watchdog_sel_clk(RTC_WDG_CLK_32000);
	hal_rtc_watchdog_set_period(period);
	hal_rtc_watchdog_set_mode(RTC_WDG_MODE_RST_SYS);
	hal_rtc_watchdog_start();

	printf("\nTEST1: Feeding watchdog, timeout=5s. Should not reset...\n");
	to = 5;
	while (to--) {
		hal_rtc_watchdog_feed();
		udelay(1*1000*1000);
	}

	printf("\nTEST2: Watchdog bark, timeout=5s. Waiting for CPU reset...\n");
	udelay(6*1000*1000);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_rtc_watchdog_timeout_reboot, test_rtc_watchdog_to_reboot, rtc_watchdog_test_tools);

int cmd_test_rtc_watchdog_reboot(int argc, char **argv)
{
	hal_log_info("cmd test rtc watchdog reboot\n");

	hal_rtc_watchdog_reboot();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_rtc_watchdog_reboot, test_rtc_watchdog_reboot, rtc_watchdog_test_tools);

