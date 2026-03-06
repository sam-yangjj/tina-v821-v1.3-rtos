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
#ifndef SUNXI_HAL_THREAD_H
#define SUNXI_HAL_THREAD_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(CONFIG_KERNEL_FREERTOS)
#include <FreeRTOS.h>
#include <task.h>
typedef TaskHandle_t hal_thread_t;

#define HAL_THREAD_PRIORITY_APP     ((configMAX_PRIORITIES >> 1))
#define HAL_THREAD_PRIORITY_CLI     ((configMAX_PRIORITIES >> 1) + 2)
#define HAL_THREAD_PRIORITY_SYS     ((configMAX_PRIORITIES >> 1) + 1)
#define HAL_THREAD_PRIORITY_NET     ((configMAX_PRIORITIES >> 1) + 1)

#define HAL_THREAD_PRIORITY_HIGHEST    (configMAX_PRIORITIES - 1)
#define HAL_THREAD_PRIORITY_LOWEST     (0)
#define HAL_THREAD_PRIORITY_MIDDLE     (configMAX_PRIORITIES >> 1)
#define HAL_THREAD_PRIORITY_HIGH       (HAL_THREAD_PRIORITY_HIGHEST - 1)
#define HAL_THREAD_PRIORITY_BELOW_HIGH (HAL_THREAD_PRIORITY_HIGHEST - 2)

#else
#include <rtthread.h>
typedef rt_thread_t hal_thread_t;
typedef struct rt_thread hal_thread;

#define HAL_THREAD_PRIORITY_APP     (4)
#define HAL_THREAD_PRIORITY_CLI     (3)
#define HAL_THREAD_PRIORITY_SYS     (3)
#define HAL_THREAD_PRIORITY_NET     (3)

#define HAL_THREAD_PRIORITY_HIGHEST (0)
#define HAL_THREAD_PRIORITY_LOWEST  (31)
#define HAL_THREAD_PRIORITY_MIDDLE  (15)

#endif

#define HAL_THREAD_STACK_SIZE    (0x2000)
#define HAL_THREAD_TIMESLICE     (    10)

#define HAL_THREAD_PRIORITY      HAL_THREAD_PRIORITY_APP

typedef struct {
	hal_thread_t thread;
	uint8_t allow_suspend;
} hal_thread_suspend_t;

void *hal_thread_create(void (*threadfn)(void *data), void *data, const char *namefmt, int stacksize, int priority);
int hal_thread_stop(void *thread);
int hal_thread_start(void *thread);
void *hal_thread_self(void);
int hal_thread_resume(void *thread);
int hal_thread_suspend(void *thread);
int hal_thread_msleep(int ms);
int hal_thread_sleep(int tick);
int hal_thread_scheduler_is_running(void);
int hal_thread_is_in_critical_context(void);
void hal_thread_tick_increase(void);
char *hal_thread_get_name(void *thread);
int hal_thread_scheduler_suspend(void);
int hal_thread_scheduler_resume(void);
uint32_t hal_thread_get_task_num(void);
uint32_t hal_thread_get_suspend_status(hal_thread_suspend_t *status, uint32_t num);

#define hal_thread_run(threadfn, data, namefmt, ...)			   \
({									   \
	void *__k						   \
		= hal_thread_create(threadfn, data, namefmt, HAL_THREAD_STACK_SIZE, HAL_THREAD_PRIORITY_SYS); \
	if (__k)				   \
		hal_thread_start(__k);					   \
	__k;								   \
})

#ifdef __cplusplus
}
#endif
#endif
