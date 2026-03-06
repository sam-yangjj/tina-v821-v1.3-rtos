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

#ifndef _PM_NOTIFY_H_
#define _PM_NOTIFY_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#else
#error "PM do not support the RTOS!!"
#endif

/* The type SYS_PREPARED and SYS_FINISHED are used to notify the pm entry/exit event.
 * The rest are used to notify that the pm prepare/finish to run in the specific core.
 */
typedef enum {
	PM_EVENT_SYS_PERPARED = 0,
	PM_EVENT_PERPARED,
	PM_EVENT_FINISHED,
	PM_EVENT_SYS_FINISHED,

	PM_EVENT_TYPE_MAX,
	PM_EVENT_TYPE_BASE = PM_EVENT_SYS_PERPARED,
} pm_event_t;

#define pm_event_valid(_t) \
	((_t) >= PM_EVENT_TYPE_BASE ||\
	 (_t) <  PM_EVENT_TYPE_MAX)

#define PM_NOTIFY_USED_MAGIC   0xdeadbeaf
#define PM_NOTIFY_ARRAY_MAX    32


typedef int (*pm_notify_cb_t)(suspend_mode_t mode, pm_event_t event, void *arg);
typedef struct pm_notify {
	const char *name;
	uint32_t has_notify;
	pm_notify_cb_t pm_notify_cb;
	void          *arg;
} pm_notify_t;


int pm_notify_init(void);
int pm_notify_exit(void);
/*
 * failed, return error <0
 * succeed, return id(0~PM_NOTIFY_ARRAY_MAX).
 */
int pm_notify_register(pm_notify_t *nt);
/*
 * id is pm_notify_register returned value.
 */
int pm_notify_unregister(int id);
int pm_notify_event(suspend_mode_t mode, pm_event_t event);

#endif

