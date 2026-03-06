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
#ifndef SUNXI_HAL_ATOMIC_H
#define SUNXI_HAL_ATOMIC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef CONFIG_KERNEL_FREERTOS
#ifdef CONFIG_CORE_DSP0
#include <spinlock.h>
typedef unsigned int hal_spinlock_t;
#else
#include <spinlock.h>
typedef freert_spinlock_t hal_spinlock_t;
#endif /* CONFIG_CORE_DSP0 */
#elif defined(CONFIG_OS_MELIS)
#include <arch.h>
typedef unsigned int melis_spinlock_t;
typedef melis_spinlock_t hal_spinlock_t;
#else
#error "can not support unknown platform"
#endif

#define HAL_SPIN_LOCK_INIT(_lock) \
	hal_spinlock_t _lock = {0}


void hal_spin_lock(hal_spinlock_t *lock);
void hal_spin_unlock(hal_spinlock_t *lock);

unsigned long hal_spin_lock_irqsave(hal_spinlock_t *lock);
void hal_spin_unlock_irqrestore(hal_spinlock_t *lock, unsigned long __cpsr);

int hal_spin_lock_init(hal_spinlock_t *lock);
int hal_spin_lock_deinit(hal_spinlock_t *lock);
#ifdef CONFIG_OS_MELIS
void hal_enter_critical(void);
void hal_exit_critical(void);
#else
unsigned long hal_enter_critical(void);
void hal_exit_critical(unsigned long flag);
#endif


#ifdef __cplusplus
}
#endif
#endif
