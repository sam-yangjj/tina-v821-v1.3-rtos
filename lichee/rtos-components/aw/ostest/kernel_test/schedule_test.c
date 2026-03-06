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
#include <hal_thread.h>
#include <hal_interrupt.h>
#include <hal_cmd.h>

#include "os_test_log.h"
#include "kernel_test.h"

#define SCHEDULE_TEST_THREAD_NUM 2
#define SCHEDULE_TEST_THREAD_SLEEP_TIME 100
#define SCHEDULE_TEST_DIS_INT_TIME 3000

typedef struct
{
	int sleep_time;
	int thread_exec_cnt;
} test_thread_info_t;

static test_thread_info_t s_test_thread_info[SCHEDULE_TEST_THREAD_NUM];

void schedule_test_thread_func(void *param)
{
	test_thread_info_t *thread_info = (test_thread_info_t *)param;
	const char *thread_name = pcTaskGetName(NULL);

	kernel_test_info("thread '%s' runing, sleep_time: %d, exec_cnt: %d",
					 thread_name, thread_info->sleep_time, thread_info->thread_exec_cnt);

	while (1)
	{
		thread_info->thread_exec_cnt++;
		hal_msleep(thread_info->sleep_time);
	}
	hal_thread_stop(NULL);
}

#define SCHEDULE_TEST_THREAD_PRIORITY (configMAX_PRIORITIES / 2)
static void *s_thread_handle[SCHEDULE_TEST_THREAD_NUM];

static int init_schedule_test_thread(int thread_num)
{
	int i, j;
	char name_buf[configMAX_TASK_NAME_LEN];
	for (i = 0; i < thread_num; i++)
	{
		snprintf(name_buf, configMAX_TASK_NAME_LEN, "schedule_test%d", i);
		s_test_thread_info[i].sleep_time = (i + 1) * SCHEDULE_TEST_THREAD_SLEEP_TIME;
		s_thread_handle[i] = hal_thread_create(schedule_test_thread_func, &s_test_thread_info[i],
											   name_buf, 1024, SCHEDULE_TEST_THREAD_PRIORITY);
		if (!s_thread_handle[i])
			goto exit_fallback;

		hal_thread_start(s_thread_handle[i]);
	}

	return 0;

exit_fallback:
	for (j = 0; j < i; j++)
	{
		hal_thread_stop(s_thread_handle[j]);
	}
	return -(i + 1);
}

static void deinit_schedule_test_thread(int thread_num)
{
	int i;
	for (i = 0; i < thread_num; i++)
	{
		hal_thread_stop(s_thread_handle[i]);
		s_test_thread_info[i].thread_exec_cnt = 0;
	}
}

static void read_thread_exec_cnt(int thread_num, int *exec_cnt_array)
{
	int i;
	for (i = 0; i < thread_num; i++)
	{
		exec_cnt_array[i] = s_test_thread_info[i].thread_exec_cnt;
	}
}

static void dump_thread_exec_cnt(int thread_num, int *exec_cnt_array)
{
	int i;

	for (i = 0; i < thread_num; i++)
	{
		os_test_info("thread%d exec cnt: %d", i, exec_cnt_array[i]);
	}
}

#define SCHEDULE_TEST_CASE_NAME "can not schedule when global interrupt is disabled"

static int schedule_when_interrupt_disable(void)
{
	int ret = 0, i, is_test_pass = 1;
	unsigned long flag;

	int thread_exec_cnt_before[SCHEDULE_TEST_THREAD_NUM];
	int thread_exec_cnt_after[SCHEDULE_TEST_THREAD_NUM];

	int thread_num = SCHEDULE_TEST_THREAD_NUM;
	int delay_time = SCHEDULE_TEST_DIS_INT_TIME;

	os_test_info("schedule test case: '%s'", SCHEDULE_TEST_CASE_NAME);

	ret = init_schedule_test_thread(thread_num);
	if (ret)
	{
		os_test_err("init_schedule_test_thread failed ,ret: %d", ret);
		return -KERNEL_TEST_RET_CREATE_THREAD_FAILED;
	}

	//let the schedule test threads to run at least twice
	hal_msleep(SCHEDULE_TEST_THREAD_SLEEP_TIME * 2);

	os_test_info("global interrupt disabled: %ld", hal_interrupt_is_disable());
	os_test_dbg("disable global interrupt begin");
	flag = hal_interrupt_disable_irqsave();
	os_test_dbg("disable global interrupt end\n");
	os_test_info("global interrupt disabled: %ld", hal_interrupt_is_disable());

	read_thread_exec_cnt(thread_num, thread_exec_cnt_before);
	dump_thread_exec_cnt(thread_num, thread_exec_cnt_before);

	os_test_info("sleep %dms begin", delay_time);

	hal_msleep(delay_time);

	os_test_info("sleep %dms end", delay_time);

	read_thread_exec_cnt(thread_num, thread_exec_cnt_after);
	dump_thread_exec_cnt(thread_num, thread_exec_cnt_after);

	os_test_info("global interrupt disabled: %ld", hal_interrupt_is_disable());
	os_test_dbg("enable global interrupt begin");
	hal_interrupt_enable_irqrestore(flag);
	os_test_dbg("enable global interrupt end");
	os_test_info("global interrupt disabled: %ld", hal_interrupt_is_disable());

	for (i = 0; i < thread_num; i++)
	{
		if (thread_exec_cnt_before[i] != thread_exec_cnt_after[i])
		{
			is_test_pass = 0;
			os_test_err("schedule test case: '%s' failed!", SCHEDULE_TEST_CASE_NAME);
			os_test_err("the execute count of schedule test thread%d is not same, before: %d, after: %d",
						i, thread_exec_cnt_before[i], thread_exec_cnt_after[i]);
			break;
		}
	}

	hal_msleep(SCHEDULE_TEST_THREAD_SLEEP_TIME * 4);
	os_test_info("thread exec cnt after enable interrupt and sleep some time:");
	read_thread_exec_cnt(thread_num, thread_exec_cnt_after);
	dump_thread_exec_cnt(thread_num, thread_exec_cnt_after);

	deinit_schedule_test_thread(thread_num);

	if (is_test_pass)
		os_test_info("schedule test case: '%s' success", SCHEDULE_TEST_CASE_NAME);
	else
		return -KERNEL_TEST_RET_SCHEDULE_WHEN_INT_DIS;

	return KERNEL_TEST_RET_OK;
}

int os_schedule_test(void)
{
	int ret;
	ret = schedule_when_interrupt_disable();
	if (ret)
		return ret;

	return KERNEL_TEST_RET_OK;
}

int cmd_os_schedule_test(int argc, const char **argv)
{
	int ret = 0;

	ret = os_schedule_test();
	if (!ret)
		os_test_info("OS schedule test success!");
	else
		os_test_err("OS schedule test failed, ret: %d", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_os_schedule_test, os_schedule_test, OS schedule test);
