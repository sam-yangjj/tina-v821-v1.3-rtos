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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <hal_cmd.h>
#include <hal_timer.h>

#include "timer_test.h"

static void timeout(void *param)
{
	TimerParams *timer_para = (TimerParams *)param;

	if (strcmp(timer_para->data, "osal_timer_test")) {
		printf("%s(%d): hal_timer_create send para failed!\n", __func__, __LINE__);
	}

	timer_para->var++;
}

int cmd_osal_timer_test(int argc, char **argv)
{
	TimerParams param;
	param.var = 0;
	int ret = -1;
	int var0, var1;
	int a_val = 10;
	unsigned long b_val = 20;

	strcpy(param.data, "osal_timer_test");

	// Test Timer Create API
	param.timer = osal_timer_create("osal_timer_test", timeout, (void *)&param,
			a_val, OSAL_TIMER_FLAG_PERIODIC);
	if (param.timer == NULL) {
		printf("%s(%d): osal_timer_create failed!\n", __func__, __LINE__);
		return -TIMER_TEST_RET_TIMER_CREATE_FAILED;
	}

	// Test Timer Start API
	ret = osal_timer_start(param.timer);
	if (ret) {
		printf("%s(%d): osal_timer_start failed, ret: %d\n", __func__, __LINE__, ret);
		osal_timer_delete(param.timer);
		return -TIMER_TEST_RET_TIMER_START_FAILED;
	}

	hal_msleep(a_val);

	if (param.var == 1)
		printf("osal_timer_start success!\n");
	else {
		printf("%s(%d): osal_timer_start failed!\n", __func__, __LINE__);
		osal_timer_delete(param.timer);
		return -TIMER_TEST_RET_TIMER_START_ABNORMAL;
	}

	// Test Timer control API
	ret = osal_timer_control(param.timer, OSAL_TIMER_CTRL_SET_TIME, &b_val);
	if (ret) {
		printf("%s(%d): osal_timer_control failed, ret: %d\n", __func__, __LINE__, ret);
		osal_timer_delete(param.timer);
		return -TIMER_TEST_RET_TIMER_CONTROL_FAILED;
	}

	var0 = param.var;
	hal_msleep(b_val * 5);

	if (param.var < 5 || param.var > 6) {
		printf("%s(%d): periodic change to 20 failed!\n", __func__, __LINE__);
		osal_timer_delete(param.timer);
		return -TIMER_TEST_RET_TIMER_CONTROL_ABNORMAL;
	}
	printf("osal_timer_control success!\n");

	// Test Timer Stop API
	ret = osal_timer_stop(param.timer);
	if (ret) {
		printf("%s(%d): osal_timer_stop failed, ret: %d\n", __func__, __LINE__, ret);
		osal_timer_delete(param.timer);
		return -TIMER_TEST_RET_TIMER_STOP_FAILED;
	}

	var0 = param.var;
	hal_msleep(100);
	var1 = param.var;

	if (var0 != var1) {
		printf("%s(%d): osal_timer_stop failed!\n", __func__, __LINE__);
		osal_timer_delete(param.timer);
		return -TIMER_TEST_RET_TIMER_STOP_ABNORMAL;
	}
	printf("osal_timer_stop success!\n");

	// Test Timer Delete API
	ret = osal_timer_delete(param.timer);
	if (ret) {
		printf("%s(%d): osal_timer_delete failed!\n", __func__, __LINE__);
		return -TIMER_TEST_RET_TIMER_DELETE_FAILED;
	} else {
		printf("osal_timer_delete success!\n");
	}

	return TIMER_TEST_RET_OK;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_timer_test, osal_timer_test, osal timer test);
