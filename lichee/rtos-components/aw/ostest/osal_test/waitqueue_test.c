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
#include <stdlib.h>
#include <stdio.h>
#include <hal_cmd.h>
#include <hal_waitqueue.h>
#include <hal_time.h>
#include <hal_thread.h>

#include "waitqueue_test.h"

static void wqtest_task1(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;

	hal_wait_event_timeout(thread_para->wqueue, thread_para->i == 2, 1000);

	thread_para->var++;
	thread_para->is_complete = 1;

	hal_thread_stop(NULL);
}

static void wqtest_task2(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;

	for (thread_para->i = 0; thread_para->i <= 2; thread_para->i++) {
		hal_wake_up(&thread_para->wqueue);
		hal_msleep(10);
	}

	thread_para->is_complete2 = 1;

	hal_thread_stop(NULL);
}

static void wqtest_task3(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	thread_para->nr3 = 0;

	hal_wait_event_timeout_exclusive(thread_para->wqueue, thread_para->nr3, -1);

	thread_para->var0++;
	thread_para->is_complete3 = 1;

	printf("wqtest_task3 wake_up by wqtest_task7!\n");

	hal_thread_stop(NULL);
}

static void wqtest_task4(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	thread_para->nr4 = 0;

	hal_wait_event_timeout_exclusive(thread_para->wqueue, thread_para->nr4, -1);

	thread_para->var0++;
	thread_para->is_complete4 = 1;

	printf("wqtest_task4 wake_up by cmd_osal_waitqueue_test!\n");

	hal_thread_stop(NULL);
}

static void wqtest_task5(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	thread_para->nr5 = 0;

	hal_wait_event_timeout(thread_para->wqueue, thread_para->nr5, -1);

	thread_para->var0++;
	thread_para->is_complete5 = 1;

	printf("wqtest_task5 wake_up by wqtest_task7!\n");

	hal_thread_stop(NULL);
}

static void wqtest_task6(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	thread_para->nr6 = 0;

	hal_wait_event_timeout(thread_para->wqueue, thread_para->nr6, -1);

	thread_para->var0++;
	thread_para->is_complete6 = 1;

	printf("wqtest_task6 wake_up by wqtest_task7!\n");

	hal_thread_stop(NULL);
}

static void wqtest_task7(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;

	thread_para->nr3 = 1;
	thread_para->nr4 = 1;
	thread_para->nr5 = 1;
	thread_para->nr6 = 1;

	hal_wake_up_nr(&thread_para->wqueue, 1);

	thread_para->is_complete7 = 1;

	hal_thread_stop(NULL);
}

static void exit_thread1(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	int loop_cnt = 0;

	while (1) {
		loop_cnt++;
		if (loop_cnt == 1000) {
			printf("%s(%d): Thread exited abnormally!\n", __func__, __LINE__);
			break;
		}

		hal_msleep(10);

		if (!thread_para->is_complete)
			continue;

		break;
	}
}

static void exit_thread2(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	int loop_cnt = 0;

	while (1) {
		loop_cnt++;
		if (loop_cnt == 1000) {
			printf("%s(%d): Thread exited abnormally!\n", __func__, __LINE__);
			break;
		}

		hal_msleep(10);

		if (!thread_para->is_complete2)
			continue;

		break;
	}
}

static void exit_thread3(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	int loop_cnt = 0;

	while (1) {
		loop_cnt++;
		if (loop_cnt == 1000) {
			printf("%s(%d): Thread exited abnormally!\n", __func__, __LINE__);
			break;
		}

		hal_msleep(10);

		if (!thread_para->is_complete3)
			continue;

		break;
	}
}

static void exit_thread4(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	int loop_cnt = 0;

	while (1) {
		loop_cnt++;
		if (loop_cnt == 1000) {
			printf("%s(%d): Thread exited abnormally!\n", __func__, __LINE__);
			break;
		}

		hal_msleep(10);

		if (!thread_para->is_complete4)
			continue;

		break;
	}
}

static void exit_thread5(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	int loop_cnt = 0;

	while (1) {
		loop_cnt++;
		if (loop_cnt == 1000) {
			printf("%s(%d): Thread exited abnormally!\n", __func__, __LINE__);
			break;
		}

		hal_msleep(10);

		if (!thread_para->is_complete5)
			continue;

		break;
	}
}

static void exit_thread6(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	int loop_cnt = 0;

	while (1) {
		loop_cnt++;
		if (loop_cnt == 1000) {
			printf("%s(%d): Thread exited abnormally!\n", __func__, __LINE__);
			break;
		}

		hal_msleep(10);

		if (!thread_para->is_complete6)
			continue;

		break;
	}
}

static void exit_thread7(void *param)
{
	WaitqueueParams *thread_para = (WaitqueueParams *)param;
	int loop_cnt = 0;

	while (1) {
		loop_cnt++;
		if (loop_cnt == 1000) {
			printf("%s(%d): Thread exited abnormally!\n", __func__, __LINE__);
			break;
		}

		hal_msleep(10);

		if (!thread_para->is_complete7)
			continue;

		break;
	}
}

int cmd_osal_waitqueue_test(int argc, char **argv)
{
	WaitqueueParams param;
	param.var = 0;
	param.var0 = 0;
	param.is_complete = 0;
	param.is_complete2 = 0;
	param.is_complete3 = 0;
	param.is_complete4 = 0;
	param.is_complete5 = 0;
	param.is_complete6 = 0;
	param.is_complete7 = 0;
	void *thread1, *thread2, *thread3, *thread4, *thread5, *thread6, *thread7;
	int ret;

	// Test Waitqueue init API
	hal_waitqueue_head_init(&param.wqueue);

	// Test Waitqueue timeout API
	thread1 = hal_thread_create(wqtest_task1, (void *)&param, "waitqueue_test1", 0x1000, 1);
	if (thread1 == NULL) {
		printf("%s(%d): thread1 create failed!\n", __func__, __LINE__);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_CREATE_THREAD_FAILED;
	}
	printf("thread: %s created success!\n", hal_thread_get_name(thread1));

	ret = hal_thread_start(thread1);
	if (ret) {
		printf("%s(%d): %s start failed!, ret: %d\n", __func__, __LINE__,
				hal_thread_get_name(thread1), ret);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_START_THREAD_FAILED;
	}
	printf("%s start success!\n", hal_thread_get_name(thread1));

	// Test Waitqueue wake up API
	thread2 = hal_thread_create(wqtest_task2, (void *)&param, "waitqueue_test2", 0x1000, 1);
	if (thread2 == NULL) {
		printf("%s(%d): thread2 create failed!\n", __func__, __LINE__);
		exit_thread1((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_CREATE_THREAD_FAILED;
	}
	printf("thread: %s created success!\n", hal_thread_get_name(thread2));

	ret = hal_thread_start(thread2);
	if (ret) {
		printf("%s(%d): %s start failed!, ret: %d\n", __func__, __LINE__,
				hal_thread_get_name(thread2), ret);
		exit_thread1((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_START_THREAD_FAILED;
	}
	printf("%s start success!\n", hal_thread_get_name(thread2));

	exit_thread1((void *)&param);
	exit_thread2((void *)&param);

	if (param.var == 1)
		printf("hal_wake_up success!\n");
	else {
		printf("%s(%d): hal_wake_up failed!\n", __func__, __LINE__);
		exit_thread1((void *)&param);
		exit_thread2((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_WAKE_UP_FAILED;
	}

	// Test Waitqueue timeout (exclusive) wake up API
	thread3 = hal_thread_create(wqtest_task3, (void *)&param, "waitqueue_test3", 0x1000, 1);
	if (thread3 == NULL) {
		printf("%s(%d): thread3 create failed!\n", __func__, __LINE__);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_CREATE_THREAD_FAILED;
	}
	printf("thread: %s created success!\n", hal_thread_get_name(thread3));

	ret = hal_thread_start(thread3);
	if (ret) {
		printf("%s(%d): %s start failed!, ret: %d\n", __func__, __LINE__,
				hal_thread_get_name(thread3), ret);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_START_THREAD_FAILED;
	}
	printf("%s start success!\n", hal_thread_get_name(thread3));

	// Test Waitqueue timeout (exclusive) wake up API
	thread4 = hal_thread_create(wqtest_task4, (void *)&param, "waitqueue_test4", 0x1000, 1);
	if (thread4 == NULL) {
		printf("%s(%d): thread4 create failed!\n", __func__, __LINE__);
		exit_thread3((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_CREATE_THREAD_FAILED;
	}
	printf("thread: %s created success!\n", hal_thread_get_name(thread4));

	ret = hal_thread_start(thread4);
	if (ret) {
		printf("%s(%d): %s start failed!, ret: %d\n", __func__, __LINE__,
				hal_thread_get_name(thread4), ret);
		exit_thread3((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_START_THREAD_FAILED;
	}
	printf("%s start success!\n", hal_thread_get_name(thread4));

	// Test Waitqueue timeout (exclusive) wake up API
	thread5 = hal_thread_create(wqtest_task5, (void *)&param, "waitqueue_test5", 0x1000, 1);
	if (thread5 == NULL) {
		printf("%s(%d): thread5 create failed!\n", __func__, __LINE__);
		exit_thread3((void *)&param);
		exit_thread4((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_CREATE_THREAD_FAILED;
	}
	printf("thread: %s created success!\n", hal_thread_get_name(thread5));

	ret = hal_thread_start(thread5);
	if (ret) {
		printf("%s(%d): %s start failed!, ret: %d\n", __func__, __LINE__,
				hal_thread_get_name(thread5), ret);
		exit_thread3((void *)&param);
		exit_thread4((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_START_THREAD_FAILED;
	}
	printf("%s start success!\n", hal_thread_get_name(thread5));

	// Test Waitqueue timeout (exclusive) wake up API
	thread6 = hal_thread_create(wqtest_task6, (void *)&param, "waitqueue_test6", 0x1000, 1);
	if (thread6 == NULL) {
		printf("%s(%d): thread6 create failed!\n", __func__, __LINE__);
		exit_thread3((void *)&param);
		exit_thread4((void *)&param);
		exit_thread5((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_CREATE_THREAD_FAILED;
	}
	printf("thread: %s created success!\n", hal_thread_get_name(thread6));

	ret = hal_thread_start(thread6);
	if (ret) {
		printf("%s(%d): %s start failed!, ret: %d\n", __func__, __LINE__,
				hal_thread_get_name(thread6), ret);
		exit_thread3((void *)&param);
		exit_thread4((void *)&param);
		exit_thread5((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_START_THREAD_FAILED;
	}
	printf("%s start success!\n", hal_thread_get_name(thread6));

	hal_msleep(100);

	thread7 = hal_thread_create(wqtest_task7, (void *)&param, "waitqueue_test7", 0x1000, 1);
	if (thread7 == NULL) {
		printf("%s(%d): thread7 create failed!\n", __func__, __LINE__);
		exit_thread3((void *)&param);
		exit_thread4((void *)&param);
		exit_thread5((void *)&param);
		exit_thread6((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_CREATE_THREAD_FAILED;
	}
	printf("thread: %s created success!\n", hal_thread_get_name(thread7));

	ret = hal_thread_start(thread7);
	if (ret) {
		printf("%s(%d): %s start failed!, ret: %d\n", __func__, __LINE__,
				hal_thread_get_name(thread7), ret);
		exit_thread3((void *)&param);
		exit_thread4((void *)&param);
		exit_thread5((void *)&param);
		exit_thread6((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_START_THREAD_FAILED;
	}
	printf("%s start success!\n", hal_thread_get_name(thread7));

	exit_thread3((void *)&param);
	exit_thread5((void *)&param);
	exit_thread6((void *)&param);

	if (param.var0 != 3) {
		printf("%s(%d): hal_wait_event_timeout_exclusive failed!\n", __func__, __LINE__);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_TIMEOUT_EXCLUSIVE_FAILED;
	}
	printf("hal_wait_event_timeout_exclusive success!\n");

	// wake up another exclusive waitqueue
	hal_wake_up_nr(&param.wqueue, 1);

	exit_thread4((void *)&param);

	if (param.var0 != 4) {
		printf("%s(%d): wake up exclusive waitqueue failed!\n", __func__, __LINE__);
		exit_thread4((void *)&param);
		exit_thread7((void *)&param);
		hal_waitqueue_head_deinit(&param.wqueue);
		return -WAITQUEUE_TEST_RET_TIMEOUT_EXCLUSIVE_FAILED;
	}

	exit_thread7((void *)&param);

	// Test Waitqueue deinit API
	hal_waitqueue_head_deinit(&param.wqueue);
	printf("run hal_waitqueue_head_deinit!\n");

	return WAITQUEUE_TEST_RET_OK;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_waitqueue_test, osal_waitqueue_test, osal waitqueue test);

