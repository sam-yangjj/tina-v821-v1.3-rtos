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
#include <string.h>
#include <hal_cmd.h>
#include <hal_time.h>
#include <hal_queue.h>
#include <hal_thread.h>

#include "queue_test.h"

static void q_thread_task(void *param)
{
	QueueParams *thread_para = (QueueParams *)param;
	int ret = -1;

	printf("running thread name: %s\n", hal_thread_get_name(thread_para->q_thread));

	ret = hal_queue_send_wait(thread_para->queue, &thread_para->q_data_send, 1000);
	if (ret) {
		printf("%s(%d): hal_mailbox_send_wait failed, ret = %d\n", __func__, __LINE__, ret);
		thread_para->is_complete1 = 1;
		hal_thread_stop(NULL);
	}

	thread_para->q_val++;
	thread_para->is_complete1 = 1;

	hal_thread_stop(NULL);
}

static void m_thread_task(void *param)
{
	QueueParams *thread_para = (QueueParams *)param;
	int ret = -1;

	printf("running thread name: %s\n", hal_thread_get_name(thread_para->m_thread));

	ret = hal_mailbox_send_wait(thread_para->mailbox, thread_para->m_data_send, 1000);
	if (ret) {
		printf("%s(%d): hal_mailbox_send_wait failed, ret = %d\n", __func__, __LINE__, ret);
		thread_para->is_complete2 = 1;
		hal_thread_stop(NULL);
	}

	thread_para->m_val++;
	thread_para->is_complete2 = 1;

	hal_thread_stop(NULL);
}

static void exit_thread1(void *param)
{
	QueueParams *thread_para = (QueueParams *)param;
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
	QueueParams *thread_para = (QueueParams *)param;
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

int cmd_osal_queue_test(int argc, char **argv)
{
	QueueParams param;
	param.q_val = 0;
	param.q_data_send = 456;
	param.is_complete1 = 0;
	int ret = -1;
	int data_send0 = 123;
	int data_recv0 = 0, data_recv1 = 0;
	int val0, val1;

	strcpy(param.q_data, "q_test");

	param.queue = hal_queue_create("queue_test", sizeof(int), 0x1000);
	if (param.queue == NULL) {
		printf("%s(%d): hal_queue_create failed!\n", __func__, __LINE__);
		return -QUEUE_TEST_RET_CREATE_QUEUE_FAILED;
	}
	printf("hal_queue_create success!\n");

	ret = hal_queue_send(param.queue, &data_send0);
	if (ret) {
		printf("%s(%d): hal_queue_send failed, ret = %d\n", __func__, __LINE__, ret);
		hal_queue_delete(param.queue);
		return -QUEUE_TEST_RET_SEND_FAILED;
	}
	printf("hal_queue_send success!\n");

	ret = hal_is_queue_empty(param.queue);
	if (ret) {
		printf("%s(%d): hal_is_queue_empty failed!, ret: %d\n", __func__, __LINE__, ret);
		hal_queue_delete(param.queue);
		return -QUEUE_TEST_RET_QUEUE_EMPTY_FAILED;
	}
	printf("hal_is_queue_empty success!\n");

	ret = hal_queue_recv(param.queue, &data_recv0, 0);
	if (ret) {
		printf("%s(%d): hal_queue_recv1 failed, ret = %d\n", __func__, __LINE__, ret);
		hal_queue_delete(param.queue);
		return -QUEUE_TEST_RET_RECV_FAILED;
	}
	if (data_recv0 != 123) {
		printf("%s(%d): hal_queue_recv: recv data is failed!\n", __func__, __LINE__);
		hal_queue_delete(param.queue);
		return -QUEUE_TEST_RET_RECV_ABNORMAL;
	}
	printf("hal_queue_recv success!\n");

	val0 = param.q_val;

	param.q_thread = hal_thread_create(q_thread_task, (void *)&param, "q_thread_test", 0x1000, 1);
	if (param.q_thread == NULL) {
		printf("hal_thread_create failed!\n");
		hal_queue_delete(param.queue);
		return -QUEUE_TEST_RET_CREATE_THREAD_FAILED;
	}

	ret = hal_thread_start(param.q_thread);
	if (ret) {
		printf("%s(%d): hal_thread_start failed!, ret: %d\n", __func__, __LINE__, ret);
		hal_queue_delete(param.queue);
		return -QUEUE_TEST_RET_START_THREAD_FAILED;
	}

	exit_thread1((void *)&param);

	val1 = param.q_val;

	if (val0 == val1) {
		printf("%s(%d): hal_queue_send_wait failed!\n", __func__, __LINE__);
		hal_queue_delete(param.queue);
		return -QUEUE_TEST_RET_SEND_WAIT_FAILED;
	}
	printf("hal_queue_send_wait success!\n");

	ret = hal_queue_recv(param.queue, &data_recv1, 0);
	if (ret) {
		printf("%s(%d): hal_queue_recv2 failed, ret = %d\n", __func__, __LINE__, ret);
		hal_queue_delete(param.queue);
		return -QUEUE_TEST_RET_RECV_FAILED;
	}
	if (data_recv1 != 456) {
		printf("%s(%d): hal_queue_recv: recv data is failed!\n", __func__, __LINE__);
		hal_queue_delete(param.queue);
		return -QUEUE_TEST_RET_RECV_ABNORMAL;
	}
	printf("hal_queue_recv success!\n");

	ret = hal_queue_delete(param.queue);
	if (ret) {
		printf("%s(%d): hal_queue_delete failed, ret = %d\n", __func__, __LINE__, ret);
		return -QUEUE_TEST_RET_QUEUE_DELETE_FAILED;
	}
	printf("hal_queue_delete success!\n");

	return QUEUE_TEST_RET_OK;

}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_queue_test, osal_queue_test, osal queue test);

int cmd_osal_mailbox_test(int argc, char **argv)
{
	QueueParams param;
	param.m_val = 0;
	param.m_data_send = 456;
	param.is_complete2 = 0;
	int ret = -1;
	int data_send0 = 123;
	int data_recv0 = 0, data_recv1 = 0;
	int val0, val1;

	strcpy(param.m_data, "m_test");

	param.mailbox = hal_mailbox_create("mailbox_test", 0x1000);
	if (param.mailbox == NULL) {
		printf("%s(%d): hal_mailbox_create failed!\n", __func__, __LINE__);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_CREATE_MALIBOX_FAILED;
	}
	printf("hal_mailbox_create success!\n");

	ret = hal_mailbox_send(param.mailbox, data_send0);
	if (ret) {
		printf("%s(%d): hal_mailbox_send failed, ret = %d\n", __func__, __LINE__, ret);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_SEND_FAILED;
	}
	printf("hal_mailbox_send success!\n");

	ret = hal_is_mailbox_empty(param.mailbox);
	if (ret) {
		printf("%s(%d): hal_is_mailbox_empty failed!, ret: %d\n", __func__, __LINE__, ret);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_MAILBOX_EMPTY_FAILED;
	}
	printf("hal_is_mailbox_empty success!\n");

	ret = hal_mailbox_recv(param.mailbox, (void *)&data_recv0, 0);
	if (ret) {
		printf("%s(%d): hal_mailbox_recv failed, ret = %d\n", __func__, __LINE__, ret);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_RECV_FAILED;
	}
	if (data_recv0 != 123) {
		printf("%s(%d): hal_mailbox_recv: recv data is failed!\n", __func__, __LINE__);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_RECV_ABNORMAL;
	}
	printf("hal_mailbox_recv success!\n");

	val0 = param.m_val;

	param.m_thread = hal_thread_create(m_thread_task, (void *)&param, "m_thread_test", 0x1000, 1);
	if (param.m_thread == NULL) {
		printf("%s(%d): hal_thread_create failed!\n", __func__, __LINE__);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_CREATE_THREAD_FAILED;
	}

	ret = hal_thread_start(param.m_thread);
	if (ret) {
		printf("%s(%d): hal_thread_start failed!, ret: %d\n", __func__, __LINE__, ret);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_START_THREAD_FAILED;
	}

	exit_thread2((void *)&param);

	val1 = param.m_val;

	if (val0 == val1) {
		printf("%s(%d): hal_mailbox_send_wait failed!\n", __func__, __LINE__);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_SEND_WAIT_FAILED;
	}
	printf("hal_mailbox_send_wait success!\n");

	ret = hal_mailbox_recv(param.mailbox, (void *)&data_recv1, 0);
	if (ret) {
		printf("%s(%d): hal_mailbox_recv failed, ret = %d\n", __func__, __LINE__, ret);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_RECV_FAILED;
	}
	if (data_recv1 != 456) {
		printf("%s(%d): hal_mailbox_recv: recv data is failed!\n", __func__, __LINE__);
		hal_mailbox_delete(param.mailbox);
		return -MAILBOX_TEST_RET_RECV_ABNORMAL;
	}
	printf("hal_mailbox_recv success!\n");

	ret = hal_mailbox_delete(param.mailbox);
	if (ret) {
		printf("%s(%d): hal_mailbox_delete failed, ret = %d\n", __func__, __LINE__, ret);
		return -MAILBOX_TEST_RET_MAILBOX_DELETE_FAILED;
	}
	printf("hal_mailbox_delete success!\n");

	return MAILBOX_TEST_RET_OK;

}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_mailbox_test, osal_mailbox_test, osal mailbox test);

