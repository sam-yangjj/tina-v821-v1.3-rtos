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
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <hal_cmd.h>
#include <hal_time.h>
#include "kernel_test.h"

#define TEST_THREAD_NAME		"TestThread2"
#define TEST_ATTR_STACK_SIZE    8192

static pthread_barrier_t barrier;
static pthread_mutex_t mutex;
static pthread_cond_t condition;

static int var1 = 0, var2 = 0;
static int thread_exit = 0;
static int condition_met = 0;
static int thread_cond_var = 0;
static int thread1_barrier_ret = 0, thread2_barrier_ret = 0;
static int mutex_var1 = 0, mutex_var2 = 0;
static int mutex_ret = 0;

extern int clock_gettime(clockid_t clock_id, struct timespec *tp);

static void *thread1_func(void *arg)
{
	printf("Thread1 is running! Thread1 id is: %lu\n", (long int)pthread_self());
	var1++;
	printf("Thread1 Will Exiting...!\n");

	pthread_exit(NULL);

	return NULL;
}

static void *thread2_func(void *arg)
{
	printf("Thread2 is running! Thread2 id is: %lu\n", (long int)pthread_self());
	while (!thread_exit) {
		var2++;
		hal_msleep(100);
	}
	printf("Thread2 cancellation requested. Exiting...!\n");

	pthread_exit(NULL);

	return NULL;
}

static void *thread_barrier_func1(void *arg)
{
	printf("thread1 is waiting for the barrier!\n");

	pthread_barrier_wait(&barrier);

	thread1_barrier_ret = 1;
	printf("thread1 has passed through the barrier!\n");
	pthread_exit(NULL);

	return NULL;
}

static void *thread_barrier_func2(void *arg)
{
	printf("thread2 is waiting for the barrier!\n");

	pthread_barrier_wait(&barrier);

	thread2_barrier_ret = 1;
	printf("thread2 has passed through the barrier!\n");
	pthread_exit(NULL);

	return NULL;
}

static void *thread_cond_func(void *arg)
{
	struct timespec ts;

	printf("Thread waiting for condition!\n");

	thread_cond_var++;
	pthread_mutex_lock(&mutex);
	while (!condition_met) {
		pthread_cond_wait(&condition, &mutex);
		hal_msleep(100);
	}
	pthread_mutex_unlock(&mutex);

	printf("Condition met, thread exiting!\n");

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 2;
	pthread_mutex_lock(&mutex);
	printf("Thread waiting 2s for condition...\n");
	pthread_cond_timedwait(&condition, &mutex, &ts);
	pthread_mutex_unlock(&mutex);

	printf("Thread test complete, will exit!\n");

	pthread_exit(NULL);

	return NULL;
}

static void *thread_mutex_func1(void *arg)
{
	printf("Thread lock the mutex...\n");
	pthread_mutex_lock(&mutex);

	hal_msleep(3000);

	pthread_mutex_unlock(&mutex);
	printf("Thread released the lock!\n");
	mutex_var1 = 1;

	pthread_exit(NULL);

	return NULL;
}

static void *thread_mutex_func2(void *arg)
{
	int trylock_result;
	int loop_cnt = 0;

	printf("Thread trying to lock the mutex...\n");

	trylock_result = pthread_mutex_trylock(&mutex);
	if (trylock_result != 0) {
		printf("Success! Thread cannot acquired the lock! Because mutex locked!\n");
	} else {
		printf("%s(%d): Thread can acquire the lock! Function abnormal!\n", __func__, __LINE__);
		mutex_ret = -1;
		return NULL;
	}

	printf("Wait thread_mutex_func1 unlocked mutex! Wait 3s...\n");
	while (!mutex_var1) {
		if (loop_cnt == 500) {
			printf("%s(%d) Thread wait timeout\n", __func__, __LINE__);
			return NULL;
		}

		loop_cnt++;
		hal_msleep(10);
	}

	printf("Thread trying to lock the mutex again...\n");

	trylock_result = pthread_mutex_trylock(&mutex);
	if (trylock_result == 0) {
		printf("Thread successfully acquired the lock!\n");
		pthread_mutex_unlock(&mutex);
		printf("Thread released the lock!\n");
	} else {
		printf("%s(%d): Thread failed to acquire the lock!\n", __func__, __LINE__);
		mutex_ret = -1;
		return NULL;
	}

	pthread_exit(NULL);

	return NULL;
}

static void *thread_mutex_func3(void *arg)
{
	int timedlock_result;
	struct timespec timeout;

	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_nsec += 500000000;

	timedlock_result = pthread_mutex_timedlock(&mutex, &timeout);
	if (timedlock_result != 0) {
		printf("Success! Thread cannot acquired the lock with timedlock!\n");
		mutex_var2++;
	} else {
		printf("%s(%d): Failed! Thread acquire the lock within the timeout!\n", __func__, __LINE__);
		pthread_mutex_unlock(&mutex);
		printf("Thread released the lock!\n");
		mutex_ret = -1;
		return NULL;
	}

	pthread_exit(NULL);

	return NULL;
}

static int pthread_test(void)
{
	pthread_t thread_id1, thread_id2;
	pthread_attr_t xThreadAttr;
	char thread_name[16];
	int ret = 0, var0 = 0;
	size_t stacksize = 0;
	thread_exit = 0;

	ret = pthread_attr_init(&xThreadAttr);
	if (ret) {
		printf("%s(%d): pthread_attr_init failed! ret: %d\n", __func__, __LINE__, ret);
		ret = -KERNEL_TEST_RET_PTHREAD_ATTR_INIT_FAILED;
		return ret;
	}

	ret = pthread_attr_setdetachstate(&xThreadAttr, PTHREAD_CREATE_JOINABLE);
	if (ret) {
		printf("%s(%d): pthread_attr_setdetachstate failed! ret: %d\n", __func__, __LINE__, ret);
		pthread_attr_destroy(&xThreadAttr);
		ret = -KERNEL_TEST_RET_PTHREAD_ATTR_INIT_FAILED;
		return ret;
	}

	ret = pthread_create(&thread_id1, &xThreadAttr, thread1_func, NULL);
	if (ret) {
		printf("%s(%d): pthread_create failed!\n", __func__, __LINE__);
		pthread_attr_destroy(&xThreadAttr);
		ret = -KERNEL_TEST_RET_CREATE_PTHREAD_FAILED;
		return ret;
	}
	printf("Thread1 created successfully!\n");

	ret = pthread_join(thread_id1, NULL);
	if (ret) {
		printf("%s(%d): pthread_join failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_JOIN_FAILED;
		goto pthread_err;
	}
	if (var1 != 1) {
		printf("%s(%d): pthread_join abnormal!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_JOIN_ABNORMAL;
		goto pthread_err;
	}
	var1 = 0;

	ret = pthread_create(&thread_id2, &xThreadAttr, thread2_func, NULL);
	if (ret) {
		printf("%s(%d): pthread_create failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_CREATE_PTHREAD_FAILED;
		return ret;
	}
	printf("Thread2 created successfully!\n");

	hal_msleep(300);

	ret = pthread_cancel(thread_id2);
	if (ret) {
		printf("%s(%d): pthread_cancel failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_CANCEL_FAILED;
		goto pthread_err;
	}
	var0 = var2;

	hal_msleep(300);

	if (var2 != var0) {
		printf("%s(%d): pthread_cancel abnormal!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_CANCEL_ABNORMAL;
		goto pthread_err;
	}
	printf("Thread2 canceled successfully!\n");
	var2 = 0;

	ret = pthread_getname_np(thread_id2, thread_name, sizeof(thread_name));
	if (ret) {
		printf("%s(%d): pthread_getname_np failed!", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_GETNAME_NP_FAILED;
		goto pthread_err;
	}
	printf("Thread2 name is: %s\n", thread_name);

	ret = pthread_setname_np(thread_id2, TEST_THREAD_NAME);
	if (ret) {
		printf("%s(%d): pthread_setname_np failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_SETNAME_NP_FAILED;
		goto pthread_err;
	}
	printf("Thread2 name set to: TestThread2!\n");

	ret = pthread_getname_np(thread_id2, thread_name, sizeof(thread_name));
	if (ret) {
		printf("%s(%d): pthread_getname_np failed!", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_GETNAME_NP_FAILED;
		goto pthread_err;
	}
	printf("Thread2 name is: %s\n", thread_name);
	if (strcmp(TEST_THREAD_NAME, thread_name)) {
		printf("%s(%d): pthread_setname_np failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_SETNAME_NP_ABNORMAL;
		goto pthread_err;
	}

	ret = pthread_attr_getstacksize(&xThreadAttr, &stacksize);
	if (ret) {
		printf("%s(%d): pthread_attr_getstacksize failed! ret: %d\n", __func__, __LINE__, ret);
		ret = -KERNEL_TEST_RET_PTHREAD_ATTR_GET_STACKSIZE_FAILED;
		goto pthread_err;
	}
	printf("Stack size get: %zu bytes!\n", stacksize);

	ret = pthread_attr_setstacksize(&xThreadAttr, TEST_ATTR_STACK_SIZE);
	if (ret) {
		printf("%s(%d): pthread_attr_setstacksize failed! ret: %d\n", __func__, __LINE__, ret);
		ret = -KERNEL_TEST_RET_PTHREAD_ATTR_SET_STACKSIZE_FAILED;
		goto pthread_err;
	}

	ret = pthread_attr_getstacksize(&xThreadAttr, &stacksize);
	if (ret) {
		printf("%s(%d): pthread_attr_getstacksize failed! ret: %d\n", __func__, __LINE__, ret);
		ret = -KERNEL_TEST_RET_PTHREAD_ATTR_GET_STACKSIZE_FAILED;
		goto pthread_err;
	}
	printf("Stack size get: %zu bytes!\n", stacksize);
	if (stacksize != TEST_ATTR_STACK_SIZE) {
		printf("%s(%d): pthread_attr_setstacksize failed! ret: %d\n", __func__, __LINE__, ret);
		ret = -KERNEL_TEST_RET_PTHREAD_ATTR_SET_STACKSIZE_ABNORMAL;
		goto pthread_err;
	}

	ret = pthread_join(thread_id2, NULL);
	if (ret) {
		printf("%s(%d): pthread_join failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_JOIN_FAILED;
		goto pthread_err;
	}

	/* Destroy the thread attribute object if it was created. */
	pthread_attr_destroy(&xThreadAttr);

	return KERNEL_TEST_RET_OK;

pthread_err:
	pthread_attr_destroy(&xThreadAttr);
	thread_exit = 1;
	return ret;
}

static int pthread_barrier_test(void)
{
	pthread_t thread1, thread2;
	int ret = 0;

	ret = pthread_barrier_init(&barrier, NULL, 3);
	if (ret) {
		printf("%s(%d): pthread_barrier_init failed! ret: %d\n", __func__, __LINE__, ret);
		ret = -KERNEL_TEST_RET_PTHREAD_BARRIER_INIT_FAILED;
		return ret;
	}

	ret = pthread_create(&thread1, NULL, thread_barrier_func1, NULL);
	if (ret) {
		printf("%s(%d): pthread_create failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_CREATE_PTHREAD_FAILED;
		goto pthread_barrier_err;
	}

	ret = pthread_create(&thread2, NULL, thread_barrier_func2, NULL);
	if (ret) {
		printf("%s(%d): pthread_create failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_CREATE_PTHREAD_FAILED;
		goto pthread_barrier_err;
	}

	hal_msleep(500);

	if (thread1_barrier_ret != 0 || thread2_barrier_ret != 0) {
		printf("%s(%d): pthread_barrier failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_BARRIER_ABNORMAL;
		goto pthread_barrier_err;
	}

	pthread_barrier_wait(&barrier);
	printf("Main thread wait barrier after 500 ms!\n");

	hal_msleep(500);

	if (thread1_barrier_ret != 1 || thread2_barrier_ret != 1) {
		printf("%s(%d): pthread_barrier_func failed! ret: %d\n", __func__, __LINE__, ret);
		ret = -KERNEL_TEST_RET_PTHREAD_BARRIER_ABNORMAL;
		goto pthread_barrier_err;
	}

	printf("Main thread wait barrier OK!\n");

	ret = pthread_join(thread1, NULL);
	if (ret) {
		printf("%s(%d): pthread_join failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_JOIN_FAILED;
		goto pthread_barrier_err;
	}

	ret = pthread_join(thread2, NULL);
	if (ret) {
		printf("%s(%d): pthread_join failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_JOIN_FAILED;
		goto pthread_barrier_err;
	}

	thread1_barrier_ret = 0;
	thread2_barrier_ret = 0;

	ret = pthread_barrier_destroy(&barrier);
	if (ret) {
		printf("%s(%d): pthread_barrier_destroy failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_BARRIER_DESTORY_FAILED;
		return ret;
	}
	printf("Pthread barrier destroyed successfully!\n");

	return KERNEL_TEST_RET_OK;

pthread_barrier_err:
	thread1_barrier_ret = 0;
	thread2_barrier_ret = 0;
	pthread_barrier_destroy(&barrier);
	return ret;
}

int pthread_cond_test(void)
{
	pthread_t thread_id;
	int ret = 0;

	ret = pthread_cond_init(&condition, NULL);
	if (ret) {
		printf("%s(%d): pthread_cond_init failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_CONF_INIT_FAILED;
		return ret;
	}

	ret = pthread_create(&thread_id, NULL, thread_cond_func, NULL);
	if (ret) {
		printf("%s(%d): pthread_create failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_CREATE_PTHREAD_FAILED;
		goto pthread_cond_err;
	}
	printf("Thread created successfully!\n");

	hal_msleep(100);

	pthread_mutex_lock(&mutex);

	condition_met = 1;

	ret = pthread_cond_signal(&condition);
	if (ret) {
		printf("%s(%d): pthread_cond_signal failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_COND_SIGNAL_FAILED;
		goto pthread_cond_err;
	}
	printf("Main thread signaling condition!\n");
	if (!thread_cond_var) {
		printf("%s(%d): pthread_cond_signal abnormal!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_COND_SIGNAL_ABNORMAL;
		goto pthread_cond_err;
	}
	thread_cond_var = 0;

	pthread_mutex_unlock(&mutex);

	ret = pthread_join(thread_id, NULL);
	if (ret) {
		printf("%s(%d): pthread_join failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_JOIN_FAILED;
		goto pthread_cond_err;
	}
	condition_met = 0;

	ret = pthread_cond_destroy(&condition);
	if (ret) {
		printf("%s(%d): pthread_cond_destory failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_CONF_DESTROY_FAILED;
		return ret;
	}

	return KERNEL_TEST_RET_OK;

pthread_cond_err:
	thread_cond_var = 0;
	pthread_cond_destroy(&condition);
	return ret;
}

int pthread_mutex_test(void)
{
	pthread_t thread1, thread2, thread3;
	int ret = 0;

	ret = pthread_mutex_init(&mutex, NULL);
	if (ret) {
		printf("%s(%d): pthread_mutex_init failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_MUTEX_INIT_FAILED;
		return ret;
	}

	ret = pthread_create(&thread1, NULL, thread_mutex_func1, NULL);
	if (ret) {
		printf("%s(%d): pthread_create failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_CREATE_PTHREAD_FAILED;
		goto pthread_mutex_err;
	}
	printf("Thread created successfully!\n");

	hal_msleep(100);

	ret = pthread_create(&thread2, NULL, thread_mutex_func2, NULL);
	if (ret) {
		printf("%s(%d): pthread_create failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_CREATE_PTHREAD_FAILED;
		goto pthread_mutex_err;
	}

	ret = pthread_join(thread1, NULL);
	if (ret) {
		printf("%s(%d): pthread_join failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_JOIN_FAILED;
		goto pthread_mutex_err;
	}

	ret = pthread_join(thread2, NULL);
	if (ret) {
		printf("%s(%d): pthread_join failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_JOIN_FAILED;
		goto pthread_mutex_err;
	}

	if (mutex_ret) {
		printf("%s(%d): pthread_mutex_trylock failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_MUTEX_TRYLOCK_FAILED;
		goto pthread_mutex_err;
	}

	pthread_mutex_lock(&mutex);
	printf("Main thread lock the mutex...\n");

	ret = pthread_create(&thread3, NULL, thread_mutex_func3, NULL);
	if (ret) {
		printf("%s(%d): pthread_create failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_CREATE_PTHREAD_FAILED;
		goto pthread_mutex_err;
	}

	ret = pthread_join(thread3, NULL);
	if (ret) {
		printf("%s(%d): pthread_join failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_THREAD_JOIN_FAILED;
		goto pthread_mutex_err;
	}

	hal_msleep(600);

	if (mutex_var2 == 0 || mutex_ret != 0) {
		printf("%s(%d): pthread_mutex_timedlock failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_MUTEX_TIMEDLOCK_FAILED;
		goto pthread_mutex_err;
	}

	pthread_mutex_unlock(&mutex);
	printf("Main thread unlock the mutex...\n");

	mutex_var1 = 0;
	mutex_var2 = 0;
	mutex_ret = 0;

	ret = pthread_mutex_destroy(&mutex);
	if (ret) {
		printf("%s(%d): pthread_mutex_init failed!\n", __func__, __LINE__);
		ret = -KERNEL_TEST_RET_PTHREAD_MUTEX_DESTROY_FAILED;
		return ret;
	}

	return KERNEL_TEST_RET_OK;

pthread_mutex_err:
	mutex_var1 = 0;
	mutex_var2 = 0;
	mutex_ret = 0;
	pthread_mutex_destroy(&mutex);
	return ret;
}

int os_pthread_test(void)
{
	int ret = -1;

	ret = pthread_test();
	if (ret) {
		printf("%s(%d): pthread_test failed! ret: %d\n", __func__, __LINE__, ret);
		return ret;
	}
	printf("====== Pthread_test API successful! ======\n");

	ret = pthread_barrier_test();
	if (ret) {
		printf("%s(%d): pthread_barrier_test failed! ret: %d\n", __func__, __LINE__, ret);
		return ret;
	}
	printf("====== Pthread_barrier_test API successful! ======\n");

	ret = pthread_cond_test();
	if (ret) {
		printf("%s(%d): pthread_cond_test failed! ret: %d\n", __func__, __LINE__, ret);
		return ret;
	}
	printf("====== Pthread_cond_test API successful! ======\n");

	ret = pthread_mutex_test();
	if (ret) {
		printf("%s(%d): pthread_mutex_test failed! ret: %d\n", __func__, __LINE__, ret);
		return ret;
	}
	printf("====== Pthread_mutex_test API successful! ======\n");

	return KERNEL_TEST_RET_OK;
}

static int cmd_os_pthread_test(int argc, const char **argv)
{
	os_pthread_test();

	return KERNEL_TEST_RET_OK;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_os_pthread_test, os_pthread_test, OS pthread test);

