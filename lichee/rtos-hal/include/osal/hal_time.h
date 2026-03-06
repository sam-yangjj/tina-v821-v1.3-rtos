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
#ifndef SUNXI_HAL_TIME_H
#define SUNXI_HAL_TIME_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Parameters used to convert the timespec values: */
#define MSEC_PER_SEC    1000L
#define USEC_PER_MSEC   1000L
#define NSEC_PER_USEC   1000L
#define NSEC_PER_MSEC   1000000L
#define USEC_PER_SEC    1000000L
#define NSEC_PER_SEC    1000000000L
#define FSEC_PER_SEC    1000000000000000LL

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <task.h>
#include <portmacro.h>

#undef HAL_WAIT_FOREVER
#define HAL_WAIT_FOREVER portMAX_DELAY
#define HAL_WAIT_NO      (0)

#define OSTICK_TO_MS(x) (x * (MSEC_PER_SEC / CONFIG_HZ))
#define MS_TO_OSTICK(x) (x / (MSEC_PER_SEC / CONFIG_HZ))

#define hal_tick_get()  xTaskGetTickCount()

typedef TickType_t hal_tick_t;

#elif defined(CONFIG_RTTKERNEL)

#include <rtthread.h>

#undef HAL_WAIT_FOREVER
#define HAL_WAIT_FOREVER RT_WAITING_FOREVER
#define HAL_WAIT_NO      RT_WAITING_NO

#define OSTICK_TO_MS(x) (x * (MSEC_PER_SEC / CONFIG_HZ))
#define MS_TO_OSTICK(x) (x / (MSEC_PER_SEC / CONFIG_HZ))

#define hal_tick_get()  rt_tick_get()
typedef rt_tick_t hal_tick_t;

#else
#error "can not support the RTOS!!"
#endif

int hal_sleep(unsigned int secs);
int hal_usleep(unsigned int usecs);
int hal_msleep(unsigned int msecs);
void hal_udelay(unsigned int us);
void hal_mdelay(unsigned int ms);
void hal_sdelay(unsigned int s);

uint64_t hal_get_timestamp(void);
uint64_t hal_get_timestamp_ns(void);
uint64_t hal_get_count_value(void);
uint32_t hal_get_count_freq(void);

uint64_t hal_gettime_ns(void);

#ifdef __cplusplus
}
#endif

#endif
