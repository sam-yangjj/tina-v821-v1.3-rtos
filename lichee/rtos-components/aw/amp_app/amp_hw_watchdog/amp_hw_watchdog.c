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
#include <sunxi_hal_watchdog.h>
#include <console.h>
#include <FreeRTOS.h>
#include <task.h>

#include <hal_thread.h>
#include <hal_time.h>

#define AMP_HW_WATCHDOG_FEED_THREAD_NAME "amp_hw_wdog_feed"

#if CONFIG_AMP_HW_WATCHDOG_THREAD_PRIORITY > (configMAX_PRIORITIES - 1)
#define AMP_HW_WATCHDOG_FEED_THREAD_PRIORITY (configMAX_PRIORITIES - 1)
#else
#define AMP_HW_WATCHDOG_FEED_THREAD_PRIORITY CONFIG_AMP_HW_WATCHDOG_THREAD_PRIORITY
#endif

static int g_feed_thread_running;

static void amp_hw_wdog_feed_thread(void *arg)
{
	TickType_t xLastWakeTime;
	BaseType_t xWasDelayed;
	int feed_interval_tick;

	g_feed_thread_running = 1;

	feed_interval_tick = MS_TO_OSTICK(CONFIG_AMP_HW_WATCHDOG_FEED_INTERVAL);

	printf("%s enter, feed_interval: %ums, feed_interval_tick: %d\n",
		   __func__, CONFIG_AMP_HW_WATCHDOG_FEED_INTERVAL, feed_interval_tick);

	xLastWakeTime = xTaskGetTickCount();

	// update the better timeout
	if (CONFIG_AMP_HW_WATCHDOG_TIMEOUT)
	{
		hal_watchdog_reset(CONFIG_AMP_HW_WATCHDOG_TIMEOUT);
		printf("AMP hardware watchdog's new timeout: %us\n", CONFIG_AMP_HW_WATCHDOG_TIMEOUT);
	}

	// waiting host start wdt
	while(g_feed_thread_running && !hal_watchdog_is_enable())
	{
		xWasDelayed = xTaskDelayUntil(&xLastWakeTime, (TickType_t)feed_interval_tick);
		if (xWasDelayed != pdTRUE)
		{
			printf("Thread stall when waiting remote enable AMP hardware watchdog!\n");
			xLastWakeTime = xTaskGetTickCount();
		}
	}

	// feeding
	printf("Begin to feed AMP hardware watchdog!\n");
	while(g_feed_thread_running)
	{
		hal_watchdog_feed();
		xWasDelayed = xTaskDelayUntil(&xLastWakeTime, (TickType_t)feed_interval_tick);
		if (xWasDelayed != pdTRUE)
		{
			printf("AMP hardware watchdog's feed thread stall!\n");
			xLastWakeTime = xTaskGetTickCount();
		}
	}

	g_feed_thread_running = 0;
	printf("%s exit\n", __func__);
	vTaskDelete(NULL);
}

int amp_hw_wdog_init(void)
{
	void *thread;

	if (g_feed_thread_running)
	{
		printf("AMP hardware watchdog's feed thread already created!\n");
		return -1;
	}

#ifdef CONFIG_KASAN
	thread = hal_thread_create(amp_hw_wdog_feed_thread, NULL,
							   AMP_HW_WATCHDOG_FEED_THREAD_NAME, 1024, AMP_HW_WATCHDOG_FEED_THREAD_PRIORITY);
#else
	thread = hal_thread_create(amp_hw_wdog_feed_thread, NULL,
							   AMP_HW_WATCHDOG_FEED_THREAD_NAME, 256, AMP_HW_WATCHDOG_FEED_THREAD_PRIORITY);
#endif

	if (!thread)
	{
		printf("create thread '%s' for feeding AMP hardware watchdog failed!\n", AMP_HW_WATCHDOG_FEED_THREAD_NAME);
		return -2;
	}
	hal_thread_start(thread);

	return 0;
}
