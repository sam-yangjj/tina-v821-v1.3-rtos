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
#include <hal_cmd.h>
#include <sunxi_hal_watchdog.h>
#include <pthread.h>

#include <hal_time.h>

extern void udelay(unsigned int us);

int cmd_watchdog(int argc, char **argv)
{
	int to;

	printf("========WATCHDOG TEST========\n");

	hal_watchdog_init();

	hal_watchdog_info();

	printf("\nTEST1: Feeding watchdog, timeout=5s. Should not reset...\n");
	hal_watchdog_reset(5);
	to = 5;
	while (to--) {
		hal_watchdog_feed();
		udelay(1*1000*1000);
	}
	udelay(1*1000*1000);

	printf("\nTEST2: Disabling watchdog, timeout=3s. Should not reset...\n");
	hal_watchdog_reset(3);
	hal_watchdog_disable();
	udelay(5*1000*1000);

	printf("\nTEST3: Watchdog bark, timeout=1s. Waiting for CPU reset...\n");
	hal_watchdog_reset(1);
	udelay(2*1000*1000);
	return 0;
}

int cmd_watchdog_reset(int argc, char **argv)
{
	printf("========WATCHDOG RESET TEST========\n");
	printf("TEST1: reset_cpu(). Waiting for CPU reset...\n");
	hal_watchdog_init();
	hal_watchdog_restart();

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_watchdog, hal_watchdog, Watchdog hal APIs tests);

FINSH_FUNCTION_EXPORT_CMD(cmd_watchdog_reset, watchdog_reset_test, Watchdog reset test code);
