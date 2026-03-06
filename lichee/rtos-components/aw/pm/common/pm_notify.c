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
#include <errno.h>

#include "pm_base.h"
#include "pm_notify.h"
#include "pm_debug.h"
#include "pm_base.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_NOTIFY

static SemaphoreHandle_t pm_notify_mutex = NULL;
static int          pm_notify_used[PM_NOTIFY_ARRAY_MAX]  = {0};
static pm_notify_t *pm_notify_array[PM_NOTIFY_ARRAY_MAX] = {0};

static int pm_notify_call_enter(suspend_mode_t mode, pm_event_t event)
{
	switch (event) {
	case PM_EVENT_SYS_PERPARED:
		pm_time_record_start(PM_TIME_RECORD_NOTIFY_SYS_PERPARED);
		pm_heap_record_start(PM_HEAP_RECORD_NOTIFY_SYS_PERPARED);
		break;
	case PM_EVENT_PERPARED:
		pm_time_record_start(PM_TIME_RECORD_NOTIFY_PERPARED);
		pm_heap_record_start(PM_HEAP_RECORD_NOTIFY_PERPARED);
		break;
	case PM_EVENT_FINISHED:
		pm_time_record_start(PM_TIME_RECORD_NOTIFY_FINISHED);
		pm_heap_record_start(PM_HEAP_RECORD_NOTIFY_FINISHED);
		break;
	case PM_EVENT_SYS_FINISHED:
		pm_time_record_start(PM_TIME_RECORD_NOTIFY_SYS_FINISHED);
		pm_heap_record_start(PM_HEAP_RECORD_NOTIFY_SYS_FINISHED);
		break;
	default:
		break;
	}
	return 0;
}

static int pm_notify_call_exit(suspend_mode_t mode, pm_event_t event)
{
	switch (event) {
	case PM_EVENT_SYS_PERPARED:
		pm_time_record_end(PM_TIME_RECORD_NOTIFY_SYS_PERPARED);
		pm_heap_record_end(PM_HEAP_RECORD_NOTIFY_SYS_PERPARED);
		break;
	case PM_EVENT_PERPARED:
		pm_time_record_end(PM_TIME_RECORD_NOTIFY_PERPARED);
		pm_heap_record_end(PM_HEAP_RECORD_NOTIFY_PERPARED);
		break;
	case PM_EVENT_FINISHED:
		pm_time_record_end(PM_TIME_RECORD_NOTIFY_FINISHED);
		pm_heap_record_end(PM_HEAP_RECORD_NOTIFY_FINISHED);
		break;
	case PM_EVENT_SYS_FINISHED:
		pm_time_record_end(PM_TIME_RECORD_NOTIFY_SYS_FINISHED);
		pm_heap_record_end(PM_HEAP_RECORD_NOTIFY_SYS_FINISHED);
		break;
	default:
		break;
	}
	return 0;
}

int pm_notify_init(void)
{
	pm_notify_mutex = xSemaphoreCreateMutex();
	return 0;
}

int pm_notify_exit(void)
{
		vSemaphoreDelete(pm_notify_mutex);
		return 0;
}


static int pm_notify_check_invalid(pm_notify_t *nt)
{
	if (!nt || !nt->name || !nt->pm_notify_cb) {
		pm_invalid();
		return -EINVAL;
	}

	return 0;
}

int pm_notify_register(pm_notify_t *nt)
{
	int i   = 0;
	int ret = 0;

	ret = pm_notify_check_invalid(nt);
	if (ret) {
		pm_invalid();
		return ret;
	}

	if (pdTRUE != xSemaphoreTake(pm_notify_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_notify_mutex);
		return -EBUSY;
	}

	for (i=0; i < PM_NOTIFY_ARRAY_MAX; i++) {
		if (PM_NOTIFY_USED_MAGIC == pm_notify_used[i])
			continue;

		pm_notify_array[i] = nt;
		pm_notify_used[i]  = PM_NOTIFY_USED_MAGIC;
		break;
	}

	if (PM_NOTIFY_ARRAY_MAX == i) {
		pm_err("Alloc failed, pm_notify more than %d.\n", PM_NOTIFY_ARRAY_MAX);
		ret = -EPERM;
	} else
		ret = i;

	xSemaphoreGive(pm_notify_mutex);

	return ret;
}

int pm_notify_unregister(int id)
{
	int ret = 0;

	if (id < 0 || id >= PM_NOTIFY_ARRAY_MAX)
		return -EINVAL;

	ret = pm_notify_check_invalid(pm_notify_array[id]);
	if (ret || PM_NOTIFY_USED_MAGIC != pm_notify_used[id]) {
		pm_warn("pm_notify invalid when unregister.\n");
	}

	if (pdTRUE != xSemaphoreTake(pm_notify_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_notify_mutex);
		return -EBUSY;
	}

	pm_notify_used[id] = 0;
	pm_notify_array[id] = NULL;

	xSemaphoreGive(pm_notify_mutex);

	return 0;
}

int pm_notify_event(suspend_mode_t mode, pm_event_t event)
{
	int i     = 0;
	int ret   = 0;
	int fail  = 0;
	int restore_event = -1;

	pm_notify_call_enter(mode, event);

	if (pdTRUE != xSemaphoreTake(pm_notify_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_notify_mutex);
		ret = -EBUSY;
		goto exit;
	}

	if (!pm_event_valid(event)) {
		pm_err("invalid pm notify event: %d\n", event);
		ret = -EINVAL;
		goto exit;
	}

	for (i=0; i < PM_NOTIFY_ARRAY_MAX; i++) {
		if (PM_NOTIFY_USED_MAGIC == pm_notify_used[i]) {
			ret = pm_notify_array[i]->pm_notify_cb(mode, event, pm_notify_array[i]->arg);
			if (ret) {
				fail = 1;
				pm_err("Execute pm_notify(%s) event(%d) failed.\n",
						pm_notify_array[i]->name ? pm_notify_array[i]->name :"unkown-notify", event);
				break;
			}
			pm_notify_array[i]->has_notify |= (1 << event);
		}
	}

	if (fail) {
		switch (event) {
		case PM_EVENT_SYS_PERPARED:
			restore_event = PM_EVENT_SYS_FINISHED;
			break;
		case PM_EVENT_PERPARED:
			restore_event = PM_EVENT_FINISHED;
			break;
		default:
			pm_err("unknown notify event type failed\n");
			break;
		}

		if (restore_event >= 0) {
			for (i=0; i < PM_NOTIFY_ARRAY_MAX; i++) {
				if ((PM_NOTIFY_USED_MAGIC != pm_notify_used[i]) || !(pm_notify_array[i]->has_notify & (1 << event)))
					continue;
				pm_notify_array[i]->pm_notify_cb(mode, restore_event, pm_notify_array[i]->arg);
				if (ret)
					pm_err("Execute pm_notify(%s) event(%d) failed.\n",
						pm_notify_array[i]->name ? pm_notify_array[i]->name :"unkown-notify", restore_event);
			}
		}
	}

	for (i=0; i < PM_NOTIFY_ARRAY_MAX; i++) {
		if ((PM_NOTIFY_USED_MAGIC != pm_notify_used[i]) || !(pm_notify_array[i]->has_notify & (1 << event)))
			continue;
		pm_notify_array[i]->has_notify &= ~(1 << event);
	}

	xSemaphoreGive(pm_notify_mutex);

	ret = fail?-EPERM:0;

exit:
	if (mode == PM_MODE_HIBERNATION) {
		ret = 0;
	}
	pm_notify_call_exit(mode, event);
	return ret;
}

