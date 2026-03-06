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
#ifndef _PM_ADAPT_H_
#define _PM_ADAPT_H_

#include <hal_thread.h>
#include <hal/aw_common.h>

typedef enum  {
    OS_PRIORITY_IDLE            = HAL_THREAD_PRIORITY_LOWEST,
    OS_PRIORITY_LOW             = HAL_THREAD_PRIORITY_LOWEST + 1,
    OS_PRIORITY_BELOW_NORMAL    = HAL_THREAD_PRIORITY_MIDDLE - 1,
    OS_PRIORITY_NORMAL          = HAL_THREAD_PRIORITY_MIDDLE,
    OS_PRIORITY_ABOVE_NORMAL    = HAL_THREAD_PRIORITY_MIDDLE + 1,
    OS_PRIORITY_HIGH            = HAL_THREAD_PRIORITY_HIGHEST - 1,
    OS_PRIORITY_REAL_TIME       = HAL_THREAD_PRIORITY_HIGHEST
} OS_Priority;

#ifndef __DEQUALIFY
#define __DEQUALIFY(type, var) ((type)(uintptr_t)(const volatile void *)(var))
#endif

#ifndef offsetof
#define offsetof(type, field) \
	((size_t)(uintptr_t)((const volatile void *)&((type *)0)->field))
#endif

#ifndef __offsetof
#define __offsetof(type, field)	offsetof(type, field)
#endif

#ifndef __containerof
#define __containerof(ptr, type, field) \
	__DEQUALIFY(type *, (const volatile char *)(ptr) - offsetof(type, field))
#endif

#ifndef container_of
#define container_of(ptr, type, field) __containerof(ptr, type, field)
#endif


/* Macro used to convert OS_Priority to the kernel's real priority */
#define OS_KERNEL_PRIO(prio) (prio)
/* Set priority level the same if necessary */
#define PM_TASK_PRIORITY	(OS_KERNEL_PRIO(OS_PRIORITY_NORMAL))


typedef uint32_t  OS_Time_t;
/* Parameters used to convert the time values */
#define OS_MSEC_PER_SEC     1000U       /* milliseconds per second */
#define OS_USEC_PER_MSEC    1000U       /* microseconds per millisecond */
#define OS_USEC_PER_SEC     1000000U    /* microseconds per second */

/* system clock's frequency, OS ticks per second */
#define OS_HZ               (CONFIG_HZ)//configTICK_RATE_HZ

/* microseconds per OS tick (1000000 / OS_HZ) */
#define OS_USEC_PER_TICK    (OS_USEC_PER_SEC / OS_HZ)

/** @brief Get the number of ticks since OS start */
/* Due to portTICK_TYPE_IS_ATOMIC is 1, calling xTaskGetTickCount() in ISR is
 * safe also.
 */
#define OS_GetTicks()       ((OS_Time_t)xTaskGetTickCount())

/** @brief Get the number of seconds since OS start */
#define OS_GetTime()        (OS_GetTicks() / OS_HZ)

/**
 * @brief Macros used to compare time values
 *
 *  These inlines deal with timer wrapping correctly. You are
 *  strongly encouraged to use them
 *  1. Because people otherwise forget
 *  2. Because if the timer wrap changes in future you won't have to
 *     alter your code.
 *
 * XR_OS_TimeAfter(a,b) returns true if the time a is after time b.
 *
 * Do this with "<0" and ">=0" to only test the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 */
#define OS_TimeAfter(a, b)          ((uint32_t)(a) >  (uint32_t)(b))
#define OS_TimeBefore(a, b)         OS_TimeAfter(b, a)
#define OS_TimeAfterEqual(a, b)     ((uint32_t)(a) >= (uint32_t)(b))
#define OS_TimeBeforeEqual(a, b)    OS_TimeAfterEqual(b, a)


#define OS_SecsToTicks(sec)     ((OS_Time_t)(sec) * OS_HZ)
#define OS_MSecsToTicks(msec)   ((OS_Time_t)(msec) * OS_HZ / OS_MSEC_PER_SEC)
#define OS_TicksToMSecs(t)      ((uint32_t)(t) * OS_MSEC_PER_SEC / OS_HZ)
#define OS_TicksToSecs(t)       ((uint32_t)(t) / OS_HZ)

extern int irq_is_pending(uint32_t irq_no);
extern void mdelay(uint32_t ms);

#endif


