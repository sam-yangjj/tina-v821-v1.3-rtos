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
#include <hal_thread.h>

#include "thread_test.h"

static void thread_task(void *param)
{
	ThreadParams *thread_para = (ThreadParams *)param;

	// Test whether the thread passes parameters correctly
	if (strcmp(thread_para->data, "thread_test")) {
		printf("%s(%d): hal_thread_create send para failed!\n", __func__, __LINE__);
	}

	thread_para->thread_self = hal_thread_self();

	while (1) {
		if (thread_para->is_need_exit)
			break;

		thread_para->var++;
		hal_msleep(1);
	}

	thread_para->is_complete = 1;

	hal_thread_stop(NULL);
}

static void exit_thread(void *param)
{
	ThreadParams *thread_para = (ThreadParams *)param;
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

int cmd_osal_thread_test(int argc, char **argv)
{
	ThreadParams param;
	param.var = 0;
	param.is_need_exit = 0;
	int ret = -1;
	int var0, var1;
	int i;

	strcpy(param.data, "thread_test");

	// Test Create Thread API
	param.thread = hal_thread_create(thread_task, (void *)&param, "thread_test", 0x1000, 1);
	if (param.thread == NULL) {
		printf("%s(%d): hal_thread_create failed!\n", __func__, __LINE__);
		return -THREAD_TEST_RET_THREAD_CREATE_FAILED;
	}
	printf("hal_thread_create success!\n");

	// Test Start Thread
	ret = hal_thread_start(param.thread);
	if (ret) {
		printf("%s(%d): hal_thread_start failed, ret: %d\n", __func__, __LINE__, ret);
		param.is_need_exit = 1;
		return -THREAD_TEST_RET_THREAD_START_FAILED;
	}
	hal_msleep(10);
	if (param.var > 0) {
		printf("hal_thread_start success!\n");
	} else {
		printf("%s(%d): hal_thread_start failed!\n", __func__, __LINE__);
		param.is_need_exit = 1;
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_START_ABNORMAL;
	}

	printf("running thread name: %s\n", hal_thread_get_name(param.thread));

	// Test Thread Self API
	if (param.thread != param.thread_self) {
		printf("%s(%d): hal_thread_self failed!\n", __func__, __LINE__);
		param.is_need_exit = 1;
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_SELF_FAILED;
	} else {
		printf("hal_thread_self success!\n");
	}

	// Test Suspend Thread API
	ret = hal_thread_suspend(param.thread);
	var0 = param.var;
	hal_msleep(10);
	var1 = param.var;
	if (ret) {
		printf("%s(%d): hal_thread_suspend failed, ret: %d\n", __func__, __LINE__, ret);
		param.is_need_exit = 1;
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_SUSPEND_FAILED;
	}
	if (var0 != var1) {
		printf("%s(%d): hal_thread_suspend failed!\n", __func__, __LINE__);
		param.is_need_exit = 1;
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_SUSPEND_FAILED;
	} else
		printf("hal_thread_suspend success!\n");

	// Test Resume Thread API
	ret = hal_thread_resume(param.thread);
	var0 = param.var;
	hal_msleep(10);
	var1 = param.var;
	if (ret) {
		printf("%s(%d): hal_thread_resume failed, ret: %d\n", __func__, __LINE__, ret);
		param.is_need_exit = 1;
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_RESUME_FAILED;
	}
	if (var0 != var1)
		printf("hal_thread_resume success!\n");
	else {
		printf("%s(%d): hal_thread_resume failed!\n", __func__, __LINE__);
		param.is_need_exit = 1;
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_RESUME_FAILED;
	}

	// Test Thread Scheduler Suspend
	ret = hal_thread_scheduler_suspend();
	if (ret) {
		printf("%s(%d): hal_thread_scheduler_suspend failed, ret: %d\n",
				__func__, __LINE__, ret);
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_SCHEDULER_SUSPEND_FAILED;
	}

	var0 = param.var;
	for (i = 0; i < 1000; i++) {
		var1 = param.var;
		if (var0 != var1) {
			printf("%s(%d): hal_thread_scheduler_suspend failed!\n", __func__, __LINE__);
			param.is_need_exit = 1;
			exit_thread((void *)&param);
			return -THREAD_TEST_RET_THREAD_SCHEDULER_SUSPEND_ABNORMAL;
		}
	}
	printf("hal_thread_scheduler_suspend success!\n");

	// Test Thread Is In Critical Context API
	ret = hal_thread_is_in_critical_context();
	if (ret != 1) {
		printf("%s(%d): hal_thread_is_in_critical_context failed, ret: %d\n",
				__func__, __LINE__, ret);
		param.is_need_exit = 1;
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_IS_IN_CRITICAL_FAILED;
	}
	printf("hal_thread_is_in_critical_context success!\n");

	// Test Thread Scheduler Resume
	var0 = param.var;
	ret = hal_thread_scheduler_resume();
	printf("%s(%d): hal_thread_scheduler_resume ret: %d\n", __func__, __LINE__, ret);

	hal_msleep(10);
	var1 = param.var;

	if (var0 == var1) {
		printf("%s(%d): hal_thread_scheduler_resume failed!\n", __func__, __LINE__);
		param.is_need_exit = 1;
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_SCHEDULER_RESUME_FAILED;
	}
	printf("hal_thread_scheduler_resume success!\n");

	// Test Thread Scheduler Is Running API
	ret = hal_thread_scheduler_is_running();
	if (ret != 1) {
		printf("%s(%d): hal_thread_scheduler_is_running failed!\n", __func__, __LINE__);
		param.is_need_exit = 1;
		exit_thread((void *)&param);
		return -THREAD_TEST_RET_THREAD_SCHEDULER_IS_RUNNING_FAILED;
	}
	printf("hal_thread_scheduler_is_running success!\n");

	// Test Stop Thread API
	ret = hal_thread_stop(param.thread);
	var0 = param.var;
	hal_msleep(10);
	var1 = param.var;
	if (var0 != var1) {
		printf("%s(%d): hal_thread_stop failed!\n", __func__, __LINE__);
		return -THREAD_TEST_RET_THREAD_STOP_FAILED;
	} else
		printf("hal_thread_stop success!\n");

	return THREAD_TEST_RET_OK;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_thread_test, osal_thread_test, osal thread test);

