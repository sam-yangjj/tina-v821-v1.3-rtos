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
#include <stdint.h>
#include <stdio.h>

#include <hal_time.h>
#include <hal_cmd.h>

#include <os_test_common.h>
#include <os_test_log.h>
#include "kernel_test.h"

#ifndef CONFIG_HZ
#error Macro CONFIG_HZ is undefined! OS tick frequency test depends on it.
#endif

/* OS tick period is normally in the range of 1us - 1s. Generally 1ms */
#define MIN_OS_TICK_FREQ 1
#define MAX_OS_TICK_FREQ 1000000

static inline int get_ts_when_tick_add(hal_tick_t last_tick, uint64_t *timestamp)
{
	hal_tick_t current_tick;

	while (1)
	{
		current_tick = hal_tick_get();

		if ((current_tick - last_tick) == 1)
		{
			*timestamp = os_test_get_timestamp();
			break;
		}

		if ((current_tick - last_tick) > 1)
		{
			os_test_err("tick is not continuous! last_tick: %u, current_tick: %u",
						last_tick, current_tick);
			return -KERNEL_TEST_RET_NOT_CONTINUOUS_TICK;
		}
	}

	return 0;
}

int os_tick_test(void)
{
	int ret;
	uint32_t tick_period;
	hal_tick_t last_tick;
	uint64_t timestamp1, timestamp2, ts_diff, real_tick_freq, min_period, max_period;

	if ((CONFIG_HZ < MIN_OS_TICK_FREQ) && (CONFIG_HZ > MAX_OS_TICK_FREQ))
	{
		os_test_err("invalid OS tick frequency: %uHz", CONFIG_HZ);
		return -KERNEL_TEST_RET_INVALID_OS_TICK_FREQ;
	}

	os_test_info("Expected OS tick frequency: %uHz", CONFIG_HZ);

	last_tick = hal_tick_get();

	ret = get_ts_when_tick_add(last_tick, &timestamp1);
	if (ret)
		return ret;

	last_tick++;
	ret = get_ts_when_tick_add(last_tick, &timestamp2);
	if (ret)
		return ret;

	ts_diff = timestamp2 - timestamp1;
	if (!ts_diff)
	{
		os_test_err("invalid tick interval! perhaps the API which gets timestamp is abnormal.");
		return -KERNEL_TEST_RET_INVALID_TIMESTAMP_DIFF;
	}

	real_tick_freq = 1000000000 / ts_diff;

	tick_period = 1000000000 / CONFIG_HZ;//unit: ns
	min_period = (tick_period / 10) * 8;
	max_period = (tick_period / 10) * 12;

	os_test_info("OS tick info after measure, period: %lluns, freq: %lluHz",
				 ts_diff, real_tick_freq);

	if ((ts_diff < min_period) || (ts_diff > max_period))
	{
		os_test_err("imprecise OS tick frequency: %lluHz", real_tick_freq);
		return -KERNEL_TEST_RET_IMPRECISE_OS_TICK_FREQ;
	}

	return KERNEL_TEST_RET_OK;
}

int cmd_os_tick_test(int argc, char **argv)
{
	int ret = 0;

	ret = os_tick_test();
	if (!ret)
		os_test_info("OS tick frequency test success!");
	else
		os_test_err("OS tick frequency test failed, ret: %d", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_os_tick_test, os_tick_test, OS tick frequency test);
