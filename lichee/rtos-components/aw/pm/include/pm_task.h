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
#ifndef _PM_TASK_H_
#define _PM_TASK_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <task.h>
#else
#error "PM do not support the RTOS!!"
#endif
#include <osal/hal_thread.h>
#include <osal/hal_timer.h>
#include <hal/aw_list.h>

#if (INCLUDE_vTaskSuspend != 1)
#error "ERROR: please define macro INCLUDE_vTaskSuspend by 1"
#endif

typedef enum {
	PM_TASK_TYPE_APP = 0,

	PM_TASK_TYPE_PM,
	PM_TASK_TYPE_SYS,
	PM_TASK_TYPE_DSP,
	PM_TASK_TYPE_RISCV,
	PM_TASK_TYPE_BT,
	PM_TASK_TYPE_WLAN,
	PM_TASK_TYPE_FREEZE_AT_ONCE,

	PM_TASK_TYPE_MAX,

	PM_TASK_TYPE_BASE = PM_TASK_TYPE_PM,
} pm_task_type_t;

#define pm_task_type_app(_tpy) \
	((_tpy) == PM_TASK_TYPE_APP)

#define pm_task_type_notapp(_tpy) \
	((_tpy) >= PM_TASK_TYPE_PM || (_tpy) < PM_TASK_TYPE_MAX)

#define pm_task_type_valid(_tpy) \
	((_tpy) >= PM_TASK_TYPE_APP || (_tpy) < PM_TASK_TYPE_MAX)

#define PM_TASK_MAX                32
#define PM_TASK_TOTAL_MAX          64

struct pm_task {
	hal_thread_t thread;
	struct list_head node;
};

int pm_task_init(void);
int pm_task_exit(void);
int pm_task_freeze(pm_task_type_t type);
int pm_task_restore(pm_task_type_t type);
int pm_task_register(TaskHandle_t xHandle, pm_task_type_t type);
int pm_task_unregister(TaskHandle_t xHandle);

#if 0
/* need pthread.h
 * means to get TaskHandle_t from pthread_t.
 */
int pm_pthread_register(pthread_t thread, pm_task_type_t type);
int pm_pthread_unregister(pthread_t thread);
#endif

#endif


