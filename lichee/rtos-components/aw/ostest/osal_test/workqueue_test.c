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
#include <hal_time.h>
#include <hal_cmd.h>
#include <hal_workqueue.h>

#include "workqueue_test.h"

static int var = 0;

static void work_func(hal_work *work, void *data)
{
	var++;

	if (data != &var) {
		printf("%s(%d): hal_work_init failed!\n", __func__, __LINE__);
	}
}

int cmd_osal_workqueue_test(int argc, char **argv)
{
	hal_workqueue *wqueue;
	hal_work work;
	int ret = -1;
	var = 0;

	// Test Workqueue init API
	hal_work_init(&work, work_func, &var);

	// Test Workqueue create API
	wqueue = hal_workqueue_create("workqueue_test", 0x1000, 6);
	if (wqueue == NULL) {
		printf("%s(%d): hal_workqueue_create failed!\n", __func__, __LINE__);
		return -WORKQUEUE_TEST_RET_CREATE_WORK_FAILED;
	}
	printf("hal_workqueue_create success!\n");

	// Test Workqueue Submit Work API
	ret = hal_workqueue_submit_work(wqueue, &work, 100);
	if (ret) {
		printf("%s(%d): hal_workqueue_submit_work failed, ret: %d\n", __func__, __LINE__, ret);
		hal_workqueue_destroy(wqueue);
		return -WORKQUEUE_TEST_RET_SUBMIT_WORK_FAILED;
	}
	printf("hal_workqueue_submit_work success!\n");

	// Test Workqueue Cancel Work API
	ret = hal_workqueue_cancel_work(wqueue, &work);
	if (ret) {
		printf("%s(%d): hal_workqueue_cancel_work failed, ret: %d\n", __func__, __LINE__, ret);
		hal_workqueue_destroy(wqueue);
		return -WORKQUEUE_TEST_RET_CANCEL_WORK_FAILED;
	}
	if (var) {
		printf("%s(%d): hal_workqueue_cancel_work failed, var: %d\n", __func__, __LINE__, var);
		hal_workqueue_destroy(wqueue);
		return -WORKQUEUE_TEST_RET_CANCEL_WORK_ABNORMAL;
	}
	printf("hal_workqueue_cancel_work success!\n");

	ret = hal_workqueue_submit_work(wqueue, &work, 100);
	if (ret) {
		printf("%s(%d): hal_workqueue_submit_work failed, ret: %d\n", __func__, __LINE__, ret);
		hal_workqueue_destroy(wqueue);
		return -WORKQUEUE_TEST_RET_SUBMIT_WORK_FAILED;
	}
	printf("hal_workqueue_submit_work success!\n");

	// Test Workqueue Cancel Work Sync API
	ret = hal_workqueue_cancel_work_sync(wqueue, &work);
	if (ret) {
		printf("%s(%d): hal_workqueue_cancel_work_sync failed, ret: %d\n",
				__func__, __LINE__, ret);
		hal_workqueue_destroy(wqueue);
		return -WORKQUEUE_TEST_RET_CANCEL_WORK_SYNC_FAILED;
	}
	printf("hal_workqueue_cancel_work_sync success!\n");

	// Test Workqueue Do Work API
	ret = hal_workqueue_dowork(wqueue, &work);
	if (ret) {
		printf("%s(%d): hal_workqueue_dowork failed, ret: %d\n", __func__, __LINE__, ret);
		hal_workqueue_destroy(wqueue);
		return -WORKQUEUE_TEST_RET_DOWORK_FAILED;
	}
	hal_msleep(10);

	if (var == 1)
		printf("hal_workqueue_dowork success!\n");
	else {
		printf("%s(%d): hal_workqueue_dowork failed!\n", __func__, __LINE__);
		hal_workqueue_destroy(wqueue);
		return -WORKQUEUE_TEST_RET_DOWORK_ABNORMAL;
	}

	ret = hal_workqueue_submit_work(wqueue, &work, 0);
	if (ret) {
		printf("%s(%d): hal_workqueue_submit_work failed, ret: %d\n", __func__, __LINE__, ret);
		hal_workqueue_destroy(wqueue);
		return -WORKQUEUE_TEST_RET_SUBMIT_WORK_FAILED;
	}
	hal_msleep(10);
	if (var == 2)
		printf("hal_workqueue_submit_work success!\n");
	else {
		printf("%s(%d): hal_workqueue_submit_work failed!, var: %d\n", __func__, __LINE__, var);
		hal_workqueue_destroy(wqueue);
		return -WORKQUEUE_TEST_RET_SUBMIT_WORK_ABNORMAL;
	}

	// Test Workqueue Destory API
	ret = hal_workqueue_destroy(wqueue);
	if (ret) {
		printf("%s(%d): hal_workqueue_destroy failed, ret: %d\n", __func__, __LINE__, ret);
		return -WORKQUEUE_TEST_RET_DESTORY_WORK_FAILED;
	}
	printf("hal_workqueue_destroy success!\n");

	return WORKQUEUE_TEST_RET_OK;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_workqueue_test, osal_workqueue_test, osal workqueue test);

