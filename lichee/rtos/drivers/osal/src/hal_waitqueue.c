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
#include <stdbool.h>
#include <aw_list.h>
#include <hal_interrupt.h>
#include <hal_thread.h>
#include <hal_waitqueue.h>

static int autoremove_wake_function(hal_waitqueue_t *wait, void *key)
{
	/* return 0 if wake up successed */
	if (hal_sem_post(&wait->sem) == HAL_OK)
		return 0;

	return 1;
}

void init_wait_entry(hal_waitqueue_t *wait, unsigned int flags)
{
	wait->flags = flags;
	wait->func = autoremove_wake_function;
	INIT_LIST_HEAD(&wait->task_list);
	hal_sem_init(&wait->sem, 0);
}

long prepare_to_wait_event(hal_waitqueue_head_t *q, hal_waitqueue_t *wait)
{
	unsigned long flags;
	long ret = 0;

	flags = hal_spin_lock_irqsave(&q->lock);
	if (list_empty(&wait->task_list)) {
		if (wait->flags & HAL_WQ_FLAG_EXCLUSIVE)
			__add_hal_waitqueue_tail(q, wait);
		else
			__add_wait_queue(q, wait);
	}
	else {
		ret = -EALREADY;
	}
	hal_spin_unlock_irqrestore(&q->lock, flags);

	return ret;
}

void finish_wait(hal_waitqueue_head_t *q, hal_waitqueue_t *wait)
{
	unsigned long flags;

	if (!list_empty_careful(&wait->task_list)) {
		flags = hal_spin_lock_irqsave(&q->lock);
		list_del_init(&wait->task_list);
		hal_sem_deinit(&wait->sem);
		hal_spin_unlock_irqrestore(&q->lock, flags);
	}
}

static void __wake_up_common(hal_waitqueue_head_t *q, int nr_exclusive,
				void *key)
{
	hal_waitqueue_t *curr, *next;

	list_for_each_entry_safe(curr, next, &q->head, task_list) {
	unsigned flags = curr->flags;

	if (!curr->func(curr, key) &&
		(flags & HAL_WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
		break;
	}
}

void __hal_wake_up(hal_waitqueue_head_t *q, int nr_exclusive, void *key)
{
	unsigned long flags;

	if (!hal_interrupt_get_nest())
		hal_thread_scheduler_suspend();

	flags = hal_spin_lock_irqsave(&q->lock);
	__wake_up_common(q, nr_exclusive, key);
	hal_spin_unlock_irqrestore(&q->lock, flags);

	if (!hal_interrupt_get_nest())
		hal_thread_scheduler_resume();
}
