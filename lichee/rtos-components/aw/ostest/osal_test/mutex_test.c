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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_mutex.h>
#include <hal_thread.h>
#include <hal_cmd.h>
#include <hal_osal.h>
#include <sunxi_hal_common.h>

#include "mutex_test.h"

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

static void thread1_run(void* arg)
{
    int ret;
    MutexParams* thread_para = (MutexParams* )arg;

    ret = hal_mutex_lock(thread_para->mutex);
    if (ret != HAL_OK)
    {
        printf("Mutex Lock error!\n");
        thread_para->ret = -MUTEX_TEST_RET_LOCK_ERR;
        thread_para->thread1_is_complete = 1;
        hal_thread_stop(NULL);
    }

    thread_para->flags = 1;

    printf("Thread1 Lock ok, sleep 2s ...\n");
    hal_msleep(2000);

    printf("Thread1 Unlock\n");
    ret = hal_mutex_unlock(thread_para->mutex);
    if (ret != HAL_OK)
    {
        printf("Mutex Unlock error!\n");
        thread_para->ret = -MUTEX_TEST_RET_UNLOCK_ERR;
        thread_para->thread1_is_complete = 1;
        hal_thread_stop(NULL);
    }
    thread_para->flags = 1;
    thread_para->ret = MUTEX_TEST_RET_OK;
    thread_para->thread1_is_complete = 1;
    hal_thread_stop(NULL);
}

static void thread2_run(void* arg)
{
    int ret;
    MutexParams* thread_para = (MutexParams* )arg;

    wait_thread_end(&thread_para->flags);
    thread_para->flags = 0;

    ret = hal_mutex_trylock(thread_para->mutex);
    if (ret) {
        printf("Thread2 try lock fail , wait mutex...\n");
    } else {
        printf("Thread2 try lock success, Lock fail!\n");
        thread_para->ret = -MUTEX_TEST_RET_TRYLOCK_ERR;
        thread_para->thread2_is_complete = 1;
        hal_thread_stop(NULL);
    }

    wait_thread_end(&thread_para->flags);

    ret = hal_mutex_trylock(thread_para->mutex);
    if (ret) {
        printf("Thread2 try lock fail agin, Unlock fail!\n");
        thread_para->ret = -MUTEX_TEST_RET_TRYLOCK_ERR;
    } else {
        printf("Thread2 try lock agin success!\n");
        thread_para->ret = MUTEX_TEST_RET_OK;
    }

    thread_para->thread2_is_complete = 1;
    hal_thread_stop(NULL);

}

/*
test api :
    hal_mutex_create/hal_mutex_lock/hal_mutex_unlock/
    hal_mutex_trylock/hal_mutex_delete
*/
static int test_mutex1(void)
{
    void *task1_handle, *task2_handle;
    int ret;
    MutexParams params;
    params.thread1_is_complete = 0;
    params.thread2_is_complete = 0;
    params.flags = 0;
    params.ret = -1;

    params.mutex = hal_mutex_create();
    if (params.mutex == NULL) {
        printf("Mutex create failed!\n");
        return -MUTEX_TEST_RET_CREATE_FAILED;
    }
    printf("Mutex create successful!\n");

    task1_handle = hal_thread_create(thread1_run, (void *)&params, "Thread1", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1);
    if (!task1_handle)
    {
        printf("create thread1 failed!\n");
        ret = -MUTEX_TEST_RET_CREATE_THREAD_FAILED;
        goto out1;
    }
	printf("thread1 create and start!\n");
    ret = hal_thread_start(task1_handle);
	if (ret)
	{
		printf("start thread1 failed, ret: %d", ret);
        ret = -MUTEX_TEST_RET_START_THREAD_FAILED;
		goto out1;
	}

    task2_handle = hal_thread_create(thread2_run, (void *)&params, "Thread2", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1);
    if (!task2_handle)
    {
        printf("create thread2 failed!\n");
        ret = -MUTEX_TEST_RET_CREATE_THREAD_FAILED;
        goto out2;
    }
	printf("thread2 create and start!\n");
    ret = hal_thread_start(task2_handle);
	if (ret)
	{
		printf("start thread2 failed, ret: %d", ret);
        ret = -MUTEX_TEST_RET_START_THREAD_FAILED;
		goto out2;
	}

    wait_thread_end(&params.thread1_is_complete);
    wait_thread_end(&params.thread2_is_complete);

    if (!params.ret) {
        printf("Mutex Lock API test success\n");
    } else {
        printf("Mutex Lock API test fail\n");
        ret = params.ret;
        goto out1;
    }

    ret = hal_mutex_delete(params.mutex);
    if(!ret) {
        printf("Metex delete success!\n");
        return MUTEX_TEST_RET_OK;
    }
    else {
        printf("Metex delete fail!\n");
        return -MUTEX_TEST_RET_DELETE_FAILED;
    }

out2:
    wait_thread_end(&params.thread1_is_complete);

out1:
     hal_mutex_delete(params.mutex);
     return ret;

}

static void thread3_run(void* arg)
{
    int ret;
    MutexParams* thread_para = (MutexParams* )arg;

    ret = hal_mutex_lock(thread_para->mutex);
    if (ret != HAL_OK)
    {
        printf("Mutex Lock error!\n");
        thread_para->ret = -MUTEX_TEST_RET_LOCK_ERR;
        thread_para->thread1_is_complete = 1;
        hal_thread_stop(NULL);
    }
    thread_para->flags = 1;
    printf("Thread3 Lock ok, sleep 2s ...\n");

    hal_msleep(2000);

    printf("Thread3 Unlock \n");
    ret = hal_mutex_unlock(thread_para->mutex);
    if (ret != HAL_OK)
    {
        printf("Mutex Unlock error!\n");
        thread_para->ret = -MUTEX_TEST_RET_UNLOCK_ERR;
        thread_para->thread1_is_complete = 1;
        hal_thread_stop(NULL);
    }
    thread_para->flags = 1;
    thread_para->ret = MUTEX_TEST_RET_OK;
    thread_para->thread1_is_complete = 1;
    hal_thread_stop(NULL);
}

static void thread4_run(void* arg)
{
    int ret;
    MutexParams* thread_para = (MutexParams* )arg;
    TickType_t xDelay = 1000;

    wait_thread_end(&thread_para->flags);
    thread_para->flags = 0;

    ret = hal_mutex_timedwait(thread_para->mutex, xDelay);
    if (ret) {
        printf("Thread4 try lock time out fail , wait mutex...\n");
    } else {
        printf("Thread4 try lock success, Lock fail!\n");
        thread_para->ret = -MUTEX_TEST_RET_TRYLOCK_ERR;
        thread_para->thread2_is_complete = 1;
        hal_thread_stop(NULL);
    }

    wait_thread_end(&thread_para->flags);

    ret = hal_mutex_timedwait(thread_para->mutex, 0);
    if (ret) {
        printf("Thread4 try lock fail agin, Unlock fail!\n");
        thread_para->ret = -MUTEX_TEST_RET_TRYLOCK_ERR;
    } else {
        printf("Thread4 try lock agin success!\n");
        thread_para->ret = MUTEX_TEST_RET_OK;
    }

    thread_para->thread2_is_complete = 1;
    hal_thread_stop(NULL);
}

/*
test api :
    hal_mutex_init/hal_mutex_timedwait
    hal_mutex_trylock/hal_mutex_detach
*/
static int test_mutex2(void)
{
    void *task3_handle, *task4_handle;
    int ret;
    MutexParams params2;
    params2.thread1_is_complete = 0;
    params2.thread2_is_complete = 0;
    params2.ret = -1;

    //create Mutex
    params2.mutex = hal_malloc(sizeof(*params2.mutex));
	if (!params2.mutex)
		return HAL_ERROR;
    ret = hal_mutex_init(params2.mutex);
	if (ret == HAL_ERROR) {
		hal_free(params2.mutex);
        printf("Mutex init fail\n");
		params2.mutex = NULL;
		return -MUTEX_TEST_RET_INIT_FAILED;
	}

    printf("Mutex init successful!\n");

    task3_handle = hal_thread_create(thread3_run, (void *)&params2, "Thread3", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1);
    if (!task3_handle)
    {
        printf("create thread3 failed!\n");
        ret = -MUTEX_TEST_RET_CREATE_THREAD_FAILED;
        goto out1;
    }
	printf("thread3 create and start!\n");
    ret = hal_thread_start(task3_handle);
	if (ret)
	{
		printf("start thread3 failed, ret: %d", ret);
        ret = -MUTEX_TEST_RET_START_THREAD_FAILED;
		goto out1;
	}

    task4_handle = hal_thread_create(thread4_run, (void *)&params2, "Thread4", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1);
    if (!task4_handle)
    {
        printf("create thread4 failed!\n");
        ret = -MUTEX_TEST_RET_CREATE_THREAD_FAILED;
        goto out2;
    }
	printf("thread4 create and start!\n");
    ret = hal_thread_start(task4_handle);
	if (ret)
	{
		printf("start thread4 failed, ret: %d", ret);
        ret = -MUTEX_TEST_RET_START_THREAD_FAILED;
		goto out2;
	}

    wait_thread_end(&params2.thread1_is_complete);
    wait_thread_end(&params2.thread2_is_complete);

    if (!params2.ret) {
        printf("Mutex Lock API test success\n");
    } else {
        printf("Mutex Lock API test fail\n");
        ret = params2.ret;
        goto out1;
    }

    //delete Mutex
    ret = hal_mutex_detach(params2.mutex);
    if (ret != HAL_OK)
    {
        printf("Mutex detach fail!\n");
        return -MUTEX_TEST_RET_DELETE_FAILED;
    }
	hal_free(params2.mutex);

    printf("Metex detach success!\n");
    return MUTEX_TEST_RET_OK;

out2:
    wait_thread_end(&params2.thread1_is_complete);

out1:
    hal_mutex_detach(params2.mutex);
    hal_free(params2.mutex);
    return ret;

}

int cmd_test_mutex(int argc, char **argv)
{
    int ret;

    ret = test_mutex1();
    if (ret)
        return ret;

    ret = test_mutex2();
    if (ret)
        return ret;

    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_mutex, test_mutex, mutex tests api);
