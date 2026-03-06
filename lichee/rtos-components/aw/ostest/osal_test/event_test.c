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
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <hal_event.h>
#include <hal_cmd.h>
#include <hal_osal.h>

#include "event_test.h"

static void wait_thread_end(int *flags)
{
    int loop_cnt = 0;

    while (1)
	{
        loop_cnt ++;
        if (loop_cnt == 1000) {
            printf("%s(%d) Tread wait timeout\n", __func__, __LINE__);
            return;
        }
		hal_msleep(10);
		if (!(*flags))
			continue;
        return;
	}
}

void taskA(void* arg) {
    EventParams *para = (EventParams *)arg;

    // 等待事件位 1 和 2 都被设置
    EventBits_t bits = hal_event_wait(para->event_group, (EVENT_BIT_1 | EVENT_BIT_2), HAL_EVENT_OPTION_AND, 50000);

    // 如果等待成功，则执行某些操作
    if ((bits & (EVENT_BIT_1 | EVENT_BIT_2)) == (EVENT_BIT_1 | EVENT_BIT_2)) {
        printf("Task A: Received event bits 1 and 2\r\n");
    } else {
        printf("Task A: time out Received event bits fail\r\n");
        para->ret = -EVENT_TEST_RET_RECEIVE_BIT_FAILED;
        para->thread1_is_complete = 1;
        hal_thread_stop(NULL);
    }

    hal_event_clear_bits(para->event_group, EVENT_BIT_1 | EVENT_BIT_2);
    bits = hal_event_get(para->event_group);
    if (bits & (EVENT_BIT_1 | EVENT_BIT_2)) {
        printf("Task A: Clean event bits 1 and 2 fail\r\n");
        para->ret = -EVENT_TEST_RET_CLEAN_BIT_FAILED;
    } else {
        printf("Task A: Clean event bits 1 and 2 success\r\n");
        para->ret = EVENT_TEST_RET_OK;
    }
    para->thread1_is_complete = 1;
    hal_thread_stop(NULL);
}

void taskB(void* arg) {
    EventParams *para = (EventParams *)arg;
    int ret = -1;

    // 设置事件位 1
    ret = hal_event_set_bits(para->event_group, EVENT_BIT_1);
    if(!ret)
        printf("Task B: Set event bit 1\r\n");
    else {
        printf("Task B: Set event bit 1 fail\r\n");
        para->ret = -EVENT_TEST_RET_SET_BIT_FAILED;
        para->thread2_is_complete = 1;
        hal_thread_stop(NULL);
    }

    // 延迟一段时间
    hal_msleep(1000);

    // 设置事件位 2
    ret = hal_event_set_bits(para->event_group, EVENT_BIT_2);
    if(!ret) {
        printf("Task B: Set event bit 2\r\n");
        para->ret = EVENT_TEST_RET_OK;
    }
    else {
        printf("Task B: Set event bit 2 fail\r\n");
        para->ret = -EVENT_TEST_RET_SET_BIT_FAILED;
        para->thread2_is_complete = 1;
        hal_thread_stop(NULL);
    }

     // 延迟一段时间
    para->thread2_is_complete = 1;
    hal_thread_stop(NULL);
}

int cmd_event_test(int argc, char **argv)
{
    int ret;
    EventParams params;
    void *task1_handle, *task2_handle;
    params.thread1_is_complete = 0;
    params.thread2_is_complete = 0;

    // 创建事件组
    params.event_group = hal_event_create();
    if (params.event_group == NULL) {
        printf("Create event group fail\n");
        return -EVENT_TEST_RET_CREATE_FAILED;
    }
    printf("Create event group success\n");

    // 创建任务 A、B
    task1_handle = hal_thread_create(taskA, (void *)&params, "Task A", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1);
    if (!task1_handle)
    {
        printf("create Task A failed!\n");
        ret = -EVENT_TEST_RET_CREATE_THREAD_FAILED;
        goto out1;
    }
    ret = hal_thread_start(task1_handle);
	if (ret)
	{
		printf("start Task A failed, ret: %d", ret);
        ret = -EVENT_TEST_RET_START_THREAD_FAILED;
		goto out1;
	}
    task2_handle = hal_thread_create(taskB, (void *)&params, "Task B", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1);
    if (!task2_handle)
    {
        printf("create Task B failed!\n");
        ret = -EVENT_TEST_RET_CREATE_THREAD_FAILED;
        goto out2;
    }
    ret = hal_thread_start(task2_handle);
    if (ret)
	{
		printf("start Task B failed, ret: %d", ret);
        ret = -EVENT_TEST_RET_START_THREAD_FAILED;
		goto out2;
	}

    wait_thread_end(&params.thread1_is_complete);
    wait_thread_end(&params.thread2_is_complete);

    ret = hal_event_delete(params.event_group);
    if (!ret) {
        printf("Event delet success!\n");
    } else {
        printf("Event delete fail\n");
        ret = -EVENT_TEST_RET_DELETE_FAILED;
        return ret;
    }

    if (!params.ret) {
        printf("Event api test success!\n");
        ret = EVENT_TEST_RET_OK;
    } else {
        printf("Event api test fail\n");
        return params.ret;
    }

    return ret;

out2:
    wait_thread_end(&params.thread1_is_complete);

out1:
    hal_event_delete(params.event_group);
    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_event_test, test_event, event tests);
