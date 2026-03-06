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
#ifndef _TIMER_H
#define _TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __MELIS__

#elif (defined(CONFIG_ARCH_STM32))

#elif (defined(CONFIG_CHIP_XRADIO))

#define XR_DRV_TIMER_ONCE XR_OS_TIMER_ONCE

typedef XR_OS_Timer_t XR_DRV_Timer_t;

#define xr_drv_timer_init XR_OS_TimerCreate
#define xr_drv_timer_destroy XR_OS_TimerDelete
#define xr_drv_timer_active XR_OS_TimerIsActive

#elif (defined(CONFIG_CHIP_XRADIO))

#include "kernel/os/os_timer.h"

#elif (defined(CONFIG_OS_RTTHREAD) || defined(CONFIG_OS_YUNOS) || (defined(CONFIG_OS_NUTTX)) || \
       defined(CONFIG_OS_TINA))

#include "kernel/os/os_timer.h"

typedef XR_OS_Timer_t XR_DRV_Timer_t;

#define XR_DRV_TimerCallback_t	XR_OS_TimerCallback_t
#define XR_DRV_TimerType		XR_OS_TimerType
#define XR_DRV_TIMER_ONCE		XR_OS_TIMER_ONCE
#define XR_DRV_TIMER_PERIODIC	XR_OS_TIMER_PERIODIC

#define xr_drv_timer_init		XR_OS_TimerCreate
#define xr_drv_timer_destroy	XR_OS_TimerDelete
#define xr_drv_timer_active		XR_OS_TimerIsActive
#define xr_drv_timer_change_period		XR_OS_TimerChangePeriod
#define xr_drv_timer_stop		XR_OS_TimerStop
/*
#elif (defined(CONFIG_OS_NUTTX))

#include <signal.h>
#define  MY_TIMER_SIGNAL SIGUSR1 //用户自定义信号???

#define NX_TIMER_LOGE sinfo

typedef uint32_t nx_time_t;

typedef enum {
	NX_TIMER_ONCE = 0,
	NX_TIMER_PERIODIC = 1
}nx_timer_type_t;

typedef void (*nx_timer_cb_t)(int signo, FAR siginfo_t * info,
		FAR void *ucontext);

typedef struct nuttx_timer {
	timer_t handle;
	nx_timer_type_t type;
	nx_time_t init_time_ms;
}nx_timer_t;

#define MAX_TIME_DELAY  0xffffffffU

typedef nx_timer_t XR_DRV_Timer_t;

int nx_timer_delete(nx_timer_t *timer);
int nx_timer_stop(nx_timer_t *timer);
int nx_timer_start(nx_timer_t *timer);
int nx_timer_change_period(nx_timer_t *timer, nx_time_t ms);
int nx_timer_create(nx_timer_t *timer, nx_timer_type_t type,
		nx_timer_cb_t handler, void *ptr_param, nx_time_t ms);
int nx_timer_is_active(nx_time_t *timer);

#define XR_DRV_TimerCallback_t	nx_timer_cb_t
#define XR_DRV_TimerType		nx_timer_type_t
#define XR_DRV_TIMER_ONCE		NX_TIMER_ONCE
#define XR_DRV_TIMER_PERIODIC	NX_TIMER_PERIODIC

#define xr_drv_timer_init			nx_timer_create
#define xr_drv_timer_destroy		nx_timer_delete
#define xr_drv_timer_active			nx_timer_is_active
#define xr_drv_timer_change_period	nx_timer_change_period
#define xr_drv_timer_stop			nx_timer_stop
*/
#endif /* __MELIS__ */

#ifdef __cplusplus
}
#endif

#endif /*_TIMER_H */
