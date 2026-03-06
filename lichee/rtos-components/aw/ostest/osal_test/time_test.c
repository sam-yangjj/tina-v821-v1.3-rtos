/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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
#include <hal_cmd.h>
#include <hal_time.h>
#include <inttypes.h>
#include <os_test_common.h>
#include <os_test_log.h>
#include <hw_timestamp.h>

#include "time_test.h"

int cmd_osal_time_delay_test(int argc, char **argv)
{
	uint64_t time1, time2, ts_diff;
	int accuracy = CONFIG_COMPONENTS_OSTEST_OSAL_TIME_ACCURACY;
	int min_interval, max_interval;

	/* swap hal_udelay code into icache */
	hal_udelay(1);

	// Test hal_udelay API, unit us.
	time1 = hw_get_count_value();
	hal_udelay(accuracy);
	time2 = hw_get_count_value();

	min_interval = 1;
	max_interval = 2 * accuracy;

	ts_diff = time2 - time1;
	ts_diff = ts_diff / (hw_get_count_freq() / 1000000);
	if (ts_diff < min_interval || ts_diff > max_interval) {
		os_test_err("delay %d us test failed!", accuracy);
		os_test_err("start_count: %" PRIu64 ", end_count: %" PRIu64 ", diff: %" PRIu64 ", ts_diff: %" PRIu64,
					time1, time2, time2 - time1, ts_diff);
		return -TIME_TEST_RET_UDELAY_FAILED;
	} else {
		printf("hal_udelay %d us, success!\n", accuracy);
	}

	time1 = hw_get_count_value();
	hal_udelay(10 * accuracy);
	time2 = hw_get_count_value();

	min_interval = (10 - 1) * accuracy;
	max_interval = (10 + 1) * accuracy;

	ts_diff = time2 - time1;
	ts_diff = ts_diff / (hw_get_count_freq() / 1000000);

	if (ts_diff < min_interval || ts_diff > max_interval) {
		os_test_err("delay %d us test failed!", 10 * accuracy);
		os_test_err("start_count: %" PRIu64 ", end_count: %" PRIu64 ", diff: %" PRIu64 ", ts_diff: %" PRIu64,
					time1, time2, time2 - time1, ts_diff);
		return -TIME_TEST_RET_UDELAY_FAILED;
	} else {
		printf("hal_udelay %d us, success!\n", 10 * accuracy);
	}

	// Test hal_mdelay API, unit ms.
	time1 = hw_get_count_value();
	hal_mdelay(accuracy);
	time2 = hw_get_count_value();

	min_interval = (1000 - 1) * accuracy;
	max_interval = (1000 + 1) * accuracy;

	ts_diff = time2 - time1;
	ts_diff = ts_diff / (hw_get_count_freq() / 1000000);

	if (ts_diff < min_interval || ts_diff > max_interval) {
		os_test_err("delay %d ms test failed!", accuracy);
		os_test_err("start_count: %" PRIu64 ", end_count: %" PRIu64 ", diff: %" PRIu64 ", ts_diff: %" PRIu64,
					time1, time2, time2 - time1, ts_diff);
		return -TIME_TEST_RET_MDELAY_FAILED;
	} else {
		printf("hal_mdelay %d ms, success!\n", accuracy);
	}

	time1 = os_test_get_timestamp();
	hal_mdelay(10 * accuracy);
	time2 = os_test_get_timestamp();

	min_interval = (1000 - 1) * 10 * accuracy;
	max_interval = (1000 + 1) * 10 * accuracy;

	if ((time2 - time1) / 1000 < min_interval || (time2 - time1) / 1000 > max_interval) {
		os_test_err("delay %d ms test failed!", 10 * accuracy);
		os_test_err("start: %" PRIu64 ", end: %" PRIu64 ", ts_diff: %" PRIu64,
					time1, time2, time2 - time1);
		return -TIME_TEST_RET_MDELAY_FAILED;
	} else {
		printf("hal_mdelay %d ms, success!\n", 10 * accuracy);
	}

	// Test hal_sdelay API, unit 1s.
	time1 = os_test_get_timestamp();
	hal_sdelay(accuracy);
	time2 = os_test_get_timestamp();

	min_interval = (1000 - 1) * 1000 * accuracy;
	max_interval = (1000 + 1) * 1000 * accuracy;

	if ((time2 - time1) / 1000 < min_interval || (time2 - time1) / 1000 > max_interval) {
		os_test_err("delay %d s test failed!", accuracy);
		os_test_err("start: %" PRIu64 ", end: %" PRIu64 ", ts_diff: %" PRIu64,
					time1, time2, time2 - time1);
		return -TIME_TEST_RET_SDELAY_FAILED;
	} else {
		printf("hal_sdelay %d s, success!\n", accuracy);
	}

	return TIME_TEST_RET_OK;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_time_delay_test, osal_time_delay_test, osal time test);

int cmd_osal_time_sleep_test(int argc, char **argv)
{
	uint64_t time1, time2;
	int sys_tick_period = 1000000 / CONFIG_HZ;
	int min_interval, max_interval;

	// Test hal_usleep API, unit us, test greater than sys_tick.
	time1 = os_test_get_timestamp();
	hal_usleep(10 * sys_tick_period);
	time2 = os_test_get_timestamp();

	min_interval = (10 - 1) * sys_tick_period;
	max_interval = (10 + 1) * sys_tick_period;

	if ((time2 - time1) / 1000 < min_interval || (time2 - time1) / 1000 > max_interval) {
		printf("%s(%d): hal_usleep %d us, failed\n", __func__, __LINE__, 10 * sys_tick_period);
		printf("time1: %" PRIu64 "\n", time1);
		printf("time2: %" PRIu64 "\n", time2);
		return -TIME_TEST_RET_USLEEP_FAILED;
	} else {
		printf("hal_usleep %d us, success!\n", 10 * sys_tick_period);
	}

	time1 = os_test_get_timestamp();
	hal_usleep(100 * sys_tick_period);
	time2 = os_test_get_timestamp();

	min_interval = (100 - 1) * sys_tick_period;
	max_interval = (100 + 1) * sys_tick_period;

	if ((time2 - time1) / 1000 < min_interval || (time2 - time1) / 1000 > max_interval) {
		printf("%s(%d): hal_usleep %d us, failed\n", __func__, __LINE__, 100 * sys_tick_period);
		printf("time1: %" PRIu64 "\n", time1);
		printf("time2: %" PRIu64 "\n", time2);
		return -TIME_TEST_RET_USLEEP_FAILED;
	} else {
		printf("hal_usleep %d us, success!\n", 100 * sys_tick_period);
	}

	// Test hal_msleep API, unit ms, test greater than sys_tick.
	time1 = os_test_get_timestamp();
	hal_msleep((sys_tick_period / 1000) * 10);
	time2 = os_test_get_timestamp();

	min_interval = (10 - 1) * sys_tick_period;
	max_interval = (10 + 1) * sys_tick_period;

	if ((time2 - time1) / 1000 < min_interval || (time2 - time1) / 1000 > max_interval) {
		printf("%s(%d): hal_msleep %d ms, failed\n", __func__, __LINE__, sys_tick_period / 100);
		printf("time1: %" PRIu64 "\n", time1);
		printf("time2: %" PRIu64 "\n", time2);
		return -TIME_TEST_RET_MSLEEP_FAILED;
	} else {
		printf("hal_msleep %d ms, success!\n", sys_tick_period / 100);
	}

	time1 = os_test_get_timestamp();
	hal_msleep((sys_tick_period / 1000) * 100);
	time2 = os_test_get_timestamp();

	min_interval = (100 - 1) * sys_tick_period;
	max_interval = (100 + 1) * sys_tick_period;

	if ((time2 - time1) / 1000 < min_interval || (time2 - time1) / 1000 > max_interval) {
		printf("%s(%d): hal_msleep %d ms, failed\n", __func__, __LINE__, sys_tick_period / 10);
		printf("time1: %" PRIu64 "\n", time1);
		printf("time2: %" PRIu64 "\n", time2);
		return -TIME_TEST_RET_MSLEEP_FAILED;
	} else {
		printf("hal_msleep %d ms, success!\n", sys_tick_period / 10);
	}

	// Test hal_sleep API, unit 1s.
	time1 = os_test_get_timestamp();
	hal_sleep(1);
	time2 = os_test_get_timestamp();

	min_interval = (1000 - 1) * sys_tick_period;
	max_interval = (1000 + 1) * sys_tick_period;

	if ((time2 - time1) / 1000 < min_interval || (time2 - time1) / 1000 > max_interval) {
		printf("%s(%d): hal_sleep 1 s, failed\n", __func__, __LINE__);
		printf("time1: %" PRIu64 "\n", time1);
		printf("time2: %" PRIu64 "\n", time2);
		return -TIME_TEST_RET_SLEEP_FAILED;
	} else {
		printf("hal_sleep 1 s, success!\n");
	}

	return TIME_TEST_RET_OK;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_time_sleep_test, osal_time_sleep_test, osal time test);

