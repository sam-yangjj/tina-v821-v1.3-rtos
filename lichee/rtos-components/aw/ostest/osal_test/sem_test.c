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
#include <hal_sem.h>
#include <hal_time.h>
#include <hal_thread.h>

#include "sem_test.h"

static void thread_task(void *param)
{
	SemParams *thread_para = (SemParams *)param;
	int ret = -1;

	printf("running thread name: %s\n", hal_thread_get_name(thread_para->thread));

	ret = hal_sem_timedwait(thread_para->sem, 1000);
	if (!ret) {
		thread_para->is_complete1 = 1;
		hal_thread_stop(NULL);
	}

	thread_para->var++;
	thread_para->is_complete1 = 1;

	hal_thread_stop(NULL);
}

static void wait_thread_task(void *param)
{
	SemParams *thread_para = (SemParams *)param;

	printf("Thread waiting for sem!\n");
	hal_sem_wait(thread_para->sem);
	printf("Thread acquired sem, continuing execution!\n");

	thread_para->is_complete3 = 1;

	hal_thread_stop(NULL);
}

static void release_thread_task(void *param)
{
	SemParams *thread_para = (SemParams *)param;

	printf("Releasing sem!\n");
	hal_sem_post(thread_para->sem);

	thread_para->is_complete2 = 1;

	hal_thread_stop(NULL);
}

static void exit_thread1(void *param)
{
	SemParams *thread_para = (SemParams *)param;
	int loop_cnt = 0;

	while (1) {
		loop_cnt++;
		if (loop_cnt == 1000) {
			printf("%s(%d): Thread exited abnormally!\n", __func__, __LINE__);
			break;
		}

		hal_msleep(10);

		if (!thread_para->is_complete1)
			continue;

		break;
	}
}

static void exit_thread2(void *param)
{
	SemParams *thread_para = (SemParams *)param;
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
	SemParams *thread_para = (SemParams *)param;
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

int cmd_osal_sem_test(int argc, char **argv)
{
	SemParams param;
	param.var = 0;
	param.is_complete1 = 0;
	param.is_complete2 = 0;
	param.is_complete3 = 0;
	int val = 0;
	int ret = 0;

	// Test Sem Create API
	param.sem = hal_sem_create(1);
	if (param.sem == NULL) {
		printf("%s(%d): hal_sem_create failed!\n", __func__, __LINE__);
		return -SEM_TEST_RET_CREATE_SEM_FAILED;
	}
	printf("hal_sem_create success!\n");

	// Test sem getvalue API
	ret = hal_sem_getvalue(param.sem, &val);
	if (ret) {
		printf("%s(%d): hal_sem_getvalue ret: %d failed!\n", __func__, __LINE__, ret);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_GETVALUE_FAILED;
	}
	if (val != 1) {
		printf("%s(%d): hal_sem_getvalue failed!\n", __func__, __LINE__);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_GETVALUE_ABNORMAL;
	}
	printf("hal_sem_getvalue success!\n");

	ret = hal_sem_wait(param.sem);
	if (ret) {
		printf("%s(%d): hal_sem_wait failed!\n", __func__, __LINE__);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_WAIT_FAILED;
	}
	printf("hal_sem_wait success!\n");

	// Test Sem timedwait API
	param.thread = hal_thread_create(thread_task, (void *)&param, "s_thread_test", 0x1000, 1);
	if (param.thread == NULL) {
		printf("%s(%d): hal_thread_create failed!\n", __func__, __LINE__);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_CREATE_THREAD_FAILED;
	}

	ret = hal_thread_start(param.thread);
	if (ret) {
		printf("%s(%d): hal_thread_start failed, ret: %d\n", __func__, __LINE__, ret);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_START_THREAD_FAILED;
	}

	exit_thread1((void *)&param);

	val = param.var;
	if (val != 1) {
		printf("%s(%d): hal_sem_timedwait failed!\n", __func__, __LINE__);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_TIMEDWAIT_FAILED;
	}
	printf("hal_sem_timedwait success!\n");

	// Test Sem Post API
	ret = hal_sem_post(param.sem);
	if (ret) {
		printf("%s(%d): hal_sem_post failed!\n", __func__, __LINE__);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_POST_FAILED;
	}

	// Test Sem Trywait API
	ret = hal_sem_trywait(param.sem);
	if (ret) {
		printf("%s(%d): hal_sem_trywait failed!\n", __func__, __LINE__);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_TRYWAIT_FAILED;
	}
	printf("hal_sem_trywait success!\n");

	// Test Sem Post API
	ret = hal_sem_post(param.sem);
	if (ret) {
		printf("%s(%d): hal_sem_post failed!\n", __func__, __LINE__);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_POST_FAILED;
	}

	ret = hal_sem_getvalue(param.sem, &val);
	if (ret) {
		printf("%s(%d): hal_sem_getvalue ret: %d failed!\n", __func__, __LINE__, ret);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_GETVALUE_FAILED;
	}

	if (val != 1) {
		printf("%s(%d): post sem failed, val: %d\n", __func__, __LINE__, val);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_GETVALUE_ABNORMAL;
	}
	printf("hal_sem_post success!\n");

	// Test Sem Clear API
	ret = hal_sem_clear(param.sem);
	if (ret) {
		printf("%s(%d): hal_sem_clear failed!\n", __func__, __LINE__);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_SEM_CLEAR_FAILED;
	}
	printf("hal_sem_clear success!\n");

	param.wait_thread = hal_thread_create(wait_thread_task, (void *)&param,
			"wait_thread_test", 0x1000, 1);
	if (param.wait_thread == NULL) {
		printf("%s(%d): hal_thread_create failed!\n", __func__, __LINE__);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_CREATE_THREAD_FAILED;
	}

	ret = hal_thread_start(param.wait_thread);
	if (ret) {
		printf("%s(%d): wait_thread start failed, ret: %d\n", __func__, __LINE__, ret);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_START_THREAD_FAILED;
	}

	hal_msleep(10);

	param.release_thread = hal_thread_create(release_thread_task, (void *)&param,
			"release_thread_test", 0x1000, 1);
	if (param.release_thread == NULL) {
		printf("%s(%d): hal_thread_create failed!\n", __func__, __LINE__);
		exit_thread3((void *)&param);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_CREATE_THREAD_FAILED;
	}

	ret = hal_thread_start(param.release_thread);
	if (ret) {
		printf("%s(%d): release_thread start failed, ret: %d\n", __func__, __LINE__, ret);
		exit_thread3((void *)&param);
		hal_sem_delete(param.sem);
		return -SEM_TEST_RET_START_THREAD_FAILED;
	}

	exit_thread2((void *)&param);
	exit_thread3((void *)&param);

	// Test Sem Delete API
	ret = hal_sem_delete(param.sem);
	if (ret) {
		printf("%s(%d): hal_sem_delete failed!\n", __func__, __LINE__);
		return -SEM_TEST_RET_SEM_DELETE_FAILED;
	}
	printf("hal_sem_delete success!\n");

	return SEM_TEST_RET_OK;

}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_sem_test, osal_sem_test, osal sem test);

