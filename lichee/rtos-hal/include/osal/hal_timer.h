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
#ifndef SUNXI_HAL_TIMER_H
#define SUNXI_HAL_TIMER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include <hal_status.h>
#include <hal_time.h>

#ifdef CONFIG_KERNEL_FREERTOS

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

typedef TimerHandle_t osal_timer_t;
typedef void (*timeout_func)(void *parameter);

#define OSAL_TIMER_FLAG_PERIODIC     (1 << 0)
#define OSAL_TIMER_FLAG_ONE_SHOT     (1 << 1)
#define OSAL_TIMER_FLAG_HARD_TIMER   (1 << 2)
#define OSAL_TIMER_FLAG_SOFT_TIMER   (1 << 3)

#define OSAL_TIMER_CTRL_SET_TIME     (1 << 1)

#elif defined(CONFIG_RTTKERNEL)

#include <rtthread.h>

typedef rt_timer_t osal_timer_t;
typedef void (*timeout_func)(void *parameter);

#define OSAL_TIMER_FLAG_DEACTIVATED  RT_TIMER_FLAG_DEACTIVATED
#define OSAL_TIMER_FLAG_ACTIVATED    RT_TIMER_FLAG_ACTIVATED
#define OSAL_TIMER_FLAG_ONE_SHOT     RT_TIMER_FLAG_ONE_SHOT
#define OSAL_TIMER_FLAG_PERIODIC     RT_TIMER_FLAG_PERIODIC

#define OSAL_TIMER_FLAG_HARD_TIMER   RT_TIMER_FLAG_HARD_TIMER
#define OSAL_TIMER_FLAG_SOFT_TIMER   RT_TIMER_FLAG_SOFT_TIMER

#define OSAL_TIMER_CTRL_SET_TIME     RT_TIMER_CTRL_SET_TIME
#define OSAL_TIMER_CTRL_GET_TIME     RT_TIMER_CTRL_GET_TIME
#define OSAL_TIMER_CTRL_SET_ONESHOT  RT_TIMER_CTRL_SET_ONESHOT
#define OSAL_TIMER_CTRL_SET_PERIODIC RT_TIMER_CTRL_SET_PERIODIC
#define OSAL_TIMER_CTRL_GET_STATE    RT_TIMER_CTRL_GET_STATE

#else
#error "can not support unknown platform"
#endif

osal_timer_t osal_timer_create(const char *name,
                               timeout_func timeout,
                               void *parameter,
                               unsigned int time,
                               unsigned char flag);

hal_status_t osal_timer_delete_timedwait(osal_timer_t timer, int ticks);
hal_status_t osal_timer_start_timedwait(osal_timer_t timer, int ticks);
hal_status_t osal_timer_stop_timedwait(osal_timer_t timer, int ticks);
hal_status_t osal_timer_delete(osal_timer_t timer);
hal_status_t osal_timer_start(osal_timer_t timer);
hal_status_t osal_timer_stop(osal_timer_t timer);
hal_status_t osal_timer_control(osal_timer_t timer, int cmd, void *arg);

hal_status_t osal_timer_remain(osal_timer_t timer, uint32_t remain);
hal_status_t osal_timer_start_all(void);
hal_status_t osal_timer_stop_all(void);

int hal_sleep(unsigned int secs);
int hal_usleep(unsigned int usecs);
int hal_msleep(unsigned int msecs);
void hal_udelay(unsigned int us);
uint64_t hal_gettime_ns(void);
uint64_t hal_gettime_us(void);

#ifdef __cplusplus
}
#endif
#endif
