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
#include <hal_atomic.h>
#include <hal_cmd.h>
#include <hal_osal.h>
#include <sunxi_hal_common.h>

#include "atomic_test.h"

static int wait_thread_end(int *flags)
{
    int loop_cnt = 0;

    while (1)
	{
        loop_cnt ++;
        if (loop_cnt == 1000) {
            printf("%s(%d) Tread wait timeout\n", __func__, __LINE__);
            return -1;
        }
		hal_msleep(10);
		if (!(*flags))
			continue;
        return 0;
	}
}

static void thread1_run(void* arg)
{
    TestParams* spinlock_para = (TestParams* )arg;
    int ret, ret1, ret2;
    unsigned long cpsr;

    ret = wait_thread_end(&spinlock_para->flags);
    if (ret) {
        printf("%s(%d) Tread1 wait timeout\n", __func__, __LINE__);
        spinlock_para->ret = -ATOMIC_TEST_RET_FAILED;
        goto exit;
    }

    /* test spinlock irqsave api */
    printf("Thread1 Lock ok, sleep 1s ...\n");
    cpsr = hal_spin_lock_irqsave(&spinlock_para->spinlock);
    ret1 = spinlock_para->cnt;

    hal_mdelay(1000);

    ret2 = spinlock_para->cnt;
    hal_spin_unlock_irqrestore(&spinlock_para->spinlock, cpsr);
    printf("Thread1 Unlock\n");
    if (ret1 != ret2) {
        printf("Thread1 spinlock irqsave fail ret1:%d  rett2:%d\n", ret1, ret2);
        spinlock_para->ret = -ATOMIC_TEST_RET_SPINLOCK_IRQSAVE_FAILED;
        goto exit;
    }
    printf("Thread1 spinlock irqsave test success ok\n");

    /* test spinlock api */
    printf("Thread1 Lock ok, sleep 1s ...\n");
    hal_spin_lock(&spinlock_para->spinlock);
    ret1 = spinlock_para->cnt;

    hal_mdelay(1000);

    ret2 = spinlock_para->cnt;
    hal_spin_unlock(&spinlock_para->spinlock);
    printf("Thread1 Unlock\n");
    if (ret1 != ret2) {
        printf("Thread1 spinlock fail ret1:%d  rett2:%d\n", ret1, ret2);
        spinlock_para->ret = -ATOMIC_TEST_RET_SPINLOCK_FAILED;
        goto exit;
    }
    printf("Thread1 spinlock test success ok\n");

    /* test critical api */
    printf("Thread1 enter critical ok, sleep 1s ...\n");
    cpsr = hal_enter_critical();
    ret1 = spinlock_para->cnt;

    hal_mdelay(1000);

    ret2 = spinlock_para->cnt;
     hal_exit_critical(cpsr);
    printf("Thread1 exit critical\n");
    if (ret1 != ret2) {
        printf("Thread1 critical fail ret1:%d  rett2:%d\n", ret1, ret2);
        spinlock_para->ret = -ATOMIC_TEST_RET_CRITICAL_FAILED;
        goto exit;
    }
    printf("Thread1 critical test success ok\n");
    spinlock_para->ret = ATOMIC_TEST_RET_OK;

exit:
    spinlock_para->thread1_is_complete = 1;
    hal_thread_stop(NULL);
}

static void thread2_run(void* arg)
{
    TestParams* spinlock_para = (TestParams* )arg;

    /* test spinlock irqsave api */
    printf("Thread2 satrt run, set cnt++\n");
    spinlock_para->flags = 1;

    while(1) {
        hal_msleep(10);
        spinlock_para->cnt ++;
        if (spinlock_para->thread1_is_complete)
            break;
    }

    spinlock_para->thread2_is_complete = 1;
    hal_thread_stop(NULL);
}

/*
test api :
    hal_spin_lock_init/hal_spin_lock_irqsave/hal_spin_unlock_irqrestore/
    hal_spin_lock_deinit
*/
int cmd_test_atomic(int argc, char **argv)
{
    void *task1_handle, *task2_handle;
    int ret;
    TestParams params;
    params.thread1_is_complete = 0;
    params.thread2_is_complete = 0;
    params.flags = 0;
    params.cnt = 0;
    params.ret = -1;

    ret = hal_spin_lock_init(&params.spinlock);
    if (!ret) {
        printf("Spinlock init success!\n");
    } else {
        printf("Spinlock init fail :%d\n", ret);
        return -ATOMIC_TEST_RET_SPINLOCK_INIT_FAILED;
    }

    task1_handle = hal_thread_create(thread1_run, (void *)&params, "Thread1", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1);
    if (!task1_handle)
    {
        printf("create thread1 failed!\n");
        ret = -ATOMIC_TEST_RET_CREATE_THREAD_FAILED;
        goto out1;
    }
	printf("thread1 create and start!\n");
    ret = hal_thread_start(task1_handle);
	if (ret)
	{
		printf("start thread1 failed, ret: %d", ret);
        ret = -ATOMIC_TEST_RET_START_THREAD_FAILED;
		goto out1;
	}

    task2_handle = hal_thread_create(thread2_run, (void *)&params, "Thread2", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1);
    if (!task2_handle)
    {
        printf("create thread2 failed!\n");
        ret = -ATOMIC_TEST_RET_CREATE_THREAD_FAILED;
        goto out2;
    }
	printf("thread2 create and start!\n");
    ret = hal_thread_start(task2_handle);
	if (ret)
	{
		printf("start thread2 failed, ret: %d", ret);
        ret = -ATOMIC_TEST_RET_START_THREAD_FAILED;
		goto out2;
	}

    wait_thread_end(&params.thread1_is_complete);
    wait_thread_end(&params.thread2_is_complete);

    if (!params.ret) {
        printf("atomic API test success\n");
    } else {
        printf("atomic API test fail\n");
        hal_spin_lock_deinit(&params.spinlock);
        return params.ret;
    }

    ret = hal_spin_lock_deinit(&params.spinlock);
    if(!ret) {
        printf("Spinlock deinit success!\n");
        return ATOMIC_TEST_RET_OK;
    }
    else {
        printf("Spinlock deinit fail!\n");
        return -ATOMIC_TEST_RET_SPINLOCK_DEINIT_FAILED;
    }

out2:
    wait_thread_end(&params.thread1_is_complete);

out1:
    hal_spin_lock_deinit(&params.spinlock);
    return ret;

}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_atomic, test_atomic, atomic api tests);
