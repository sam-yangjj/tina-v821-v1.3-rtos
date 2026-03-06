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
#include "sunxi-rpc-internal.h"

#define THREAD_POOL_SIZE	(CONFIG_COMPONENTS_AW_RPC_SERVER_MAX_THREAD)

struct thread_pool {
	void *threads[THREAD_POOL_SIZE];
	hal_waitqueue_head_t wq;
	int stop;
} threadpool_t;

static LIST_HEAD(task_list);
static HAL_SPIN_LOCK_INIT(task_lock);
static struct thread_pool g_pool;

struct task {
	struct list_head list;
	void (*func)(void *data);
	void *data;
};

static void thread_function(void *arg)
{
	struct thread_pool *pool = arg;
	unsigned long flags;
	struct task *t;

	while (!pool->stop) {
		hal_wait_event(pool->wq, !list_empty(&task_list) || pool->stop);

		if (pool->stop)
			break;

		flags = hal_spin_lock_irqsave(&task_lock);
		if (!list_empty(&task_list)) {
			t = list_first_entry(&task_list, struct task, list);
			list_del(&t->list);
		} else {
			t = NULL;
		}
		hal_spin_unlock_irqrestore(&task_lock, flags);

		if (t) {
			t->func(t->data);
			hal_free(t);
		}
	}

	hal_thread_stop(NULL);

	return;
}

int threadpool_add_task(void (*func)(void *), void *data)
{
	struct task *t;
	unsigned long flags;

	t = hal_malloc(sizeof(struct task));
	if (!t)
		return -ENOMEM;

	t->func = func;
	t->data = data;

	flags = hal_spin_lock_irqsave(&task_lock);
	list_add_tail(&t->list, &task_list);
	hal_spin_unlock_irqrestore(&task_lock, flags);

	hal_wake_up(&g_pool.wq);

	return 0;
}

int threadpool_init(void)
{
	int i;
	char buf[32];

	g_pool.stop = 0;
	hal_waitqueue_head_init(&g_pool.wq);
	for (i = 0; i < THREAD_POOL_SIZE; i++) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "rpc-thread%d", i);
		g_pool.threads[i] = hal_thread_run(thread_function, &g_pool, buf);
		if (!g_pool.threads[i]) {
			printf("Failed to create thread %d\n", i);
			return -ENOMEM;
		}
	}

	return 0;
}

void threadpool_exit(void)
{
	struct task *t, *tmp;
	unsigned long flags;

	g_pool.stop = 1;
	hal_wake_up_all(&g_pool.wq);

	flags = hal_spin_lock_irqsave(&task_lock);
	list_for_each_entry_safe(t, tmp, &task_list, list) {
		list_del(&t->list);
		hal_free(t);
	}
	hal_spin_unlock_irqrestore(&task_lock, flags);

	hal_waitqueue_head_deinit(&g_pool.wq);
}
