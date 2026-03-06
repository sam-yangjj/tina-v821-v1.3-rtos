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
#include <_ansi.h>
#include <reent.h>
#include <stdio.h>
#include <stdarg.h>
#include <hal_hwspinlock.h>
#include <spinlock.h>
#ifdef CONFIG_UART_CLI_USE_MUTEX
#include <hal_thread.h>
#include <pthread.h>
#endif



#ifdef CONFIG_COMPONENTS_PM
static hal_spinlock_t pm_printf_lock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};

static uint32_t printf_lock_cnt = 0;
void printf_lock(void)
{
	unsigned long flags;
	flags = hal_spin_lock_irqsave(&pm_printf_tiny_lock);
	printf_lock_cnt += 1;
	hal_spin_unlock_irqrestore(&pm_printf_tiny_lock, flags);
}

void printf_unlock(void)
{
	unsigned long flags;
	flags = hal_spin_lock_irqsave(&pm_printf_tiny_lock);
	if (printf_lock_cnt > 0)
		printf_lock_cnt -= 1;
	hal_spin_unlock_irqrestore(&pm_printf_tiny_lock, flags);
}
#endif

int _printf_r(struct _reent *ptr,
              const char *__restrict fmt, ...)
{
    int ret;
    va_list ap;

    _REENT_SMALL_CHECK_INIT(ptr);
    va_start(ap, fmt);
    ret = _vfprintf_r(ptr, _stdout_r(ptr), fmt, ap);
    va_end(ap);
    return ret;
}

#ifndef _REENT_ONLY

#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
static hal_spinlock_t io_lock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
static pthread_mutex_t io_mutex = FREERTOS_POSIX_MUTEX_INITIALIZER;
#endif

static unsigned long enter_io_critical(void)
{
#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
    return hal_spin_lock_irqsave(&io_lock);
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
    if (!hal_thread_is_in_critical_context())
        pthread_mutex_lock(&io_mutex);
    return 0;
#endif
}

static void exit_io_critical(unsigned long cpu_sr)
{
#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
    hal_spin_unlock_irqrestore(&io_lock, cpu_sr);
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
    (void)cpu_sr;
    if (!hal_thread_is_in_critical_context())
        pthread_mutex_unlock(&io_mutex);
#endif
}

int printf(const char *__restrict fmt, ...)
{
#ifdef CONFIG_CLI_UART_PORT_LOCK
	int lock_ret = HWSPINLOCK_ERR;
#endif
#ifdef CONFIG_COMPONENTS_PM
	if (printf_lock_cnt)
		return 0;
#endif

    int ret;
    va_list ap;
    uint32_t flags;

    flags = enter_io_critical();
    struct _reent *ptr = _REENT;

#ifdef CONFIG_CLI_UART_PORT_LOCK
    if (!hal_thread_is_in_critical_context())
        lock_ret = hal_hwspin_lock_timeout(SPINLOCK_CLI_UART_LOCK_BIT, 100);
#endif

    _REENT_SMALL_CHECK_INIT(ptr);
    va_start(ap, fmt);
    ret = _vfprintf_r(ptr, _stdout_r(ptr), fmt, ap);
    va_end(ap);

#ifdef CONFIG_CLI_UART_PORT_LOCK
    if (lock_ret == HWSPINLOCK_OK)
        hal_hwspin_unlock(SPINLOCK_CLI_UART_LOCK_BIT);
#endif

    exit_io_critical(flags);

    return ret;
}

#ifdef _NANO_FORMATTED_IO
int _iprintf_r(struct _reent *, const char *, ...) _ATTRIBUTE((__alias__("_printf_r")));
int iprintf(const char *, ...) _ATTRIBUTE((__alias__("printf")));
#endif
#endif /* ! _REENT_ONLY */
