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
#ifndef _THREADS_H
#define _THREADS_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __MELIS__

#elif (defined(CONFIG_ARCH_STM32))

#include "rtos/xr_rtos/xr_thread.h"

typedef struct xr_thread xr_drv_thread_t;

#define xr_drv_thread_valid(thread) XR_THREAD_IS_VALID(thread)
#define xr_drv_thread_init(thread, func, arg, priority, stack_size, stack_pointer) \
	xr_thread_create(thread, (os_pthread)func, arg, priority, stack_size, stack_pointer)
#define xr_drv_thread_destroy(thread) xr_thread_exit(thread)

#elif (defined(CONFIG_CHIP_XRADIO))

#include "kernel/os/os_thread.h"

typedef XR_OS_Thread_t xr_drv_thread_t;

#define schedule()              XR_OS_ThreadYield()

#define xr_drv_thread_valid(thread) XR_OS_ThreadIsValid(thread)
#define xr_drv_thread_init(thread, name, entry, arg, priority, stackSize) \
	XR_OS_ThreadCreate(thread, name, (XR_OS_ThreadEntry_t)entry, (void *)arg, (XR_OS_Priority)priority, stackSize)
#define xr_drv_thread_destroy(thread) XR_OS_ThreadDelete(thread)

#elif (defined(CONFIG_OS_RTTHREAD) || defined(CONFIG_OS_YUNOS) || defined(CONFIG_OS_NUTTX) || \
       defined(CONFIG_OS_TINA))
#include "kernel/os/os_thread.h"

#define XR_DRV_Priority XR_OS_Priority
#define XR_DRV_THREAD_PRIO_DRV_BH       XR_OS_THREAD_PRIO_DRV_BH
#define XR_DRV_THREAD_PRIO_DRV_WORK     XR_OS_THREAD_PRIO_DRV_WORK
#define XR_DRV_THREAD_PRIO_DRV_RX       XR_OS_THREAD_PRIO_DRV_RX

#define xr_drv_thread_t         XR_OS_Thread_t
#define XR_DRV_ThreadEntry_t    XR_OS_ThreadEntry_t

#define schedule()              XR_OS_ThreadYield()

#define xr_drv_thread_valid(thread) XR_OS_ThreadIsValid(thread)
#define xr_drv_thread_init(thread, name, entry, arg, priority, stackSize) \
	XR_OS_ThreadCreate(thread, name, (XR_OS_ThreadEntry_t)entry, (void *)arg, (XR_OS_Priority)priority, stackSize)
#define xr_drv_thread_destroy(thread) XR_OS_ThreadDelete(thread)

#endif /* __MELIS__ */

#ifdef __cplusplus
}
#endif

#endif /*_THREADS_H */
