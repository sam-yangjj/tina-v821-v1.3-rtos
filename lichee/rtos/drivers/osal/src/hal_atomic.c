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
#include <string.h>
#include <hal_osal.h>
#include <hal_atomic.h>
#include <trace_event.h>

unsigned long hal_spin_lock_irqsave(hal_spinlock_t *lock)
{
	unsigned long cpu_sr;

	cpu_sr = freert_spin_lock_irqsave((freert_spinlock_t *)lock);
	if (hal_interrupt_get_nest() == 0)
		hal_thread_scheduler_suspend();
	trace_event_begin(EV_SPIN, "", ARG_PTR(lock), ARG_ULONG_RENAME(flags, cpu_sr));

	return cpu_sr;
}

int hal_spin_lock_init(hal_spinlock_t *lock)
{
    trace_event(EV_SPIN, "init", ARG_PTR(lock));
    memset(lock, 0, sizeof(hal_spinlock_t));
    return HAL_OK;
}

int hal_spin_lock_deinit(hal_spinlock_t *lock)
{
    trace_event(EV_SPIN, "deinit", ARG_PTR(lock));
    return HAL_OK;
}

void hal_spin_unlock_irqrestore(hal_spinlock_t *lock, unsigned long __cpsr)
{
	trace_event_end(EV_SPIN, "", ARG_PTR(lock), ARG_ULONG_RENAME(flags, __cpsr));
	if (hal_interrupt_get_nest() == 0)
		hal_thread_scheduler_resume();
	freert_spin_unlock_irqrestore((freert_spinlock_t *)lock, __cpsr);
}

void hal_spin_lock(hal_spinlock_t *lock)
{
	trace_event_begin(EV_SPIN, "", ARG_PTR(lock));
	freert_spin_lock((freert_spinlock_t *)lock);
}

void hal_spin_unlock(hal_spinlock_t *lock)
{
	trace_event_end(EV_SPIN, "", ARG_PTR(lock));
	freert_spin_unlock((freert_spinlock_t *)lock);
}

unsigned long hal_enter_critical(void)
{
	unsigned long flag = 0;

	trace_event_begin(EV_SYS, "critical");
	if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
		taskENTER_CRITICAL();
#else
		taskENTER_CRITICAL(flag);
#endif
	} else {
		flag = taskENTER_CRITICAL_FROM_ISR();
	}
	return flag;
}

void hal_exit_critical(unsigned long flag)
{
	trace_event_end(EV_SYS, "critical");
	if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
		taskEXIT_CRITICAL();
#else
		taskEXIT_CRITICAL(flag);
#endif
	} else {
		taskEXIT_CRITICAL_FROM_ISR(flag);
	}
}
