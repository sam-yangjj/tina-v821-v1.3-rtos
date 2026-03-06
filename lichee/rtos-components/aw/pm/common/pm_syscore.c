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
#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#endif
#include "pm_adapt.h"
#include "pm_base.h"
#include "pm_syscore.h"
#include "pm_debug.h"
#include "pm_init.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE PM_DEBUG_SYSCORE

#define syscore_containerof(ptr_module) \
        __containerof(ptr_module, struct syscore_ops, node)

typedef enum {
	PM_SYSCORE_TYPE_SUSPEND = 0,
	PM_SYSCORE_TYPE_RESUME,
	PM_SYSCORE_TYPE_MAX,
} pm_syscore_type_t;

static SemaphoreHandle_t syscore_mutex = NULL;
static LIST_HEAD(syscore_list);
static LIST_HEAD(common_syscore_list);
static struct pm_suspend_stat syscore_stat = {
	.name = "syscore_stat",
	.failed_unit = NULL,
	.cnt = 0,
};

static struct pm_suspend_stat common_syscore_stat = {
	.name = "common_syscore_stat",
	.failed_unit = NULL,
	.cnt = 0,
};

static int pm_syscore_call_enter(pm_syscore_type_t type, suspend_mode_t mode)
{
	switch (type) {
	case PM_SYSCORE_TYPE_SUSPEND:
		pm_time_record_start(PM_TIME_RECORD_SYSCORE_SUSPEND);
		pm_heap_record_start(PM_HEAP_RECORD_SYSCORE_SUSPEND);
		break;
	case PM_SYSCORE_TYPE_RESUME:
		pm_time_record_start(PM_TIME_RECORD_SYSCORE_RESUME);
		pm_heap_record_start(PM_HEAP_RECORD_SYSCORE_RESUME);
		break;
	default:
		break;
	}
	return 0;
}

static int pm_syscore_call_exit(pm_syscore_type_t type, suspend_mode_t mode)
{
	switch (type) {
	case PM_SYSCORE_TYPE_SUSPEND:
		pm_time_record_end(PM_TIME_RECORD_SYSCORE_SUSPEND);
		pm_heap_record_end(PM_HEAP_RECORD_SYSCORE_SUSPEND);
		break;
	case PM_SYSCORE_TYPE_RESUME:
		pm_time_record_end(PM_TIME_RECORD_SYSCORE_RESUME);
		pm_heap_record_end(PM_HEAP_RECORD_SYSCORE_RESUME);
		break;
	default:
		break;
	}
	return 0;
}

static int pm_syscore_in_list(struct list_head *syscore_node)
{
	struct list_head *node = NULL;
	struct list_head *list = &syscore_list;
	struct list_head *list_save;
	struct syscore_ops *ops;

	list_for_each_safe(node, list_save, list) {
		ops = syscore_containerof(node);
		if (syscore_node == &ops->node)
			return 1;
	}

	node = NULL;
	list = &common_syscore_list;
	list_save = NULL;
	list_for_each_safe(node, list_save, list) {
		ops = syscore_containerof(node);
		if (syscore_node == &ops->node)
			return 1;
	}

	return 0;
}

int pm_syscore_register(struct syscore_ops *ops)
{
	if (!ops || !ops->name) {
		pm_invalid();
		return -EINVAL;
	}

	if (pm_syscore_in_list(&ops->node)) {
		pm_err("syscore:  %s(%p) has already registered\n", ops->name, ops);
		return -EINVAL;
	}

	if (pdTRUE != xSemaphoreTake(syscore_mutex, portMAX_DELAY)) {
		pm_semapbusy(syscore_mutex);
		return -EBUSY;
	}

	if (ops->common_syscore == COMMON_SYSCORE)
		list_add_tail(&ops->node, &common_syscore_list);
	else
		list_add_tail(&ops->node, &syscore_list);
	xSemaphoreGive(syscore_mutex);

	pm_dbg("syscore: register %s(%p) ok.\n", ops->name, ops);

	return 0;
}

int pm_syscore_unregister(struct syscore_ops *ops)
{
	if (!ops || !ops->name) {
		pm_invalid();
		return -EINVAL;
	}

	if (!pm_syscore_in_list(&ops->node)) {
		pm_err("syscore:  %s(%p) is not registered\n", ops->name, ops);
		return -EINVAL;
	}

	if (pdTRUE != xSemaphoreTake(syscore_mutex, portMAX_DELAY)) {
		pm_semapbusy(syscore_mutex);
		return -EBUSY;
	}

	list_del(&ops->node);
	xSemaphoreGive(syscore_mutex);

	pm_dbg("syscore: unregister %s(%p) done.\n", ops->name, ops);

	return 0;
}

int pm_common_syscore_suspend(suspend_mode_t mode)
{
	int ret = 0;
	struct syscore_ops *ptr = NULL;
	struct list_head *node = NULL;
	struct list_head *head = &common_syscore_list;

	pm_trace_func("%d", mode);

	if (common_syscore_stat.cnt) {
		pm_warn("common_syscore_stat cnt: %d\n", common_syscore_stat.cnt);
		common_syscore_stat.cnt = 0;
	}
	common_syscore_stat.failed_unit = NULL;

	if (pdTRUE != xSemaphoreTake(syscore_mutex, portMAX_DELAY)) {
		pm_semapbusy(syscore_mutex);
		return -EBUSY;
	}

	list_for_each_prev(node, head) {
		ptr = syscore_containerof(node);
		if (ptr && ptr->suspend) {
			if (ptr->data) {
				ret = ptr->suspend(ptr->data, mode);
			} else {
				ret = ptr->suspend(NULL, mode);
			}
			if (!ret) {
				common_syscore_stat.cnt ++;
			} else {
				common_syscore_stat.failed_unit = ptr;
				pm_err("syscore suspend failed: %s(%p)\n",\
						ptr->name, ptr);
				break;
			}
		}
	}

	if (common_syscore_stat.failed_unit) {
		list_for_each(node, head) {
			ptr = syscore_containerof(node);
			if (common_syscore_stat.cnt && ptr && ptr->resume) {
				if (ptr->data) {
					ptr->resume(ptr->data, mode);
				} else {
					ptr->resume(NULL, mode);
				}
				common_syscore_stat.cnt --;
			}
		}
	}

	xSemaphoreGive(syscore_mutex);

	if (mode == PM_MODE_HIBERNATION) {
		ret = 0;
	}
	return ret;
}

int pm_common_syscore_resume(suspend_mode_t mode)
{
	struct syscore_ops *ptr = NULL;
	struct list_head *node = NULL;
	struct list_head *head = &common_syscore_list;

	pm_trace_func("%d", mode);

	if (pdTRUE != xSemaphoreTake(syscore_mutex, portMAX_DELAY)) {
		pm_semapbusy(syscore_mutex);
		return -EBUSY;
	}

	list_for_each(node, head) {
		ptr = syscore_containerof(node);
		if (ptr && ptr->resume) {
			if (ptr->data) {
				ptr->resume(ptr->data, mode);
			} else {
				ptr->resume(NULL, mode);
			}
		}
	}
	xSemaphoreGive(syscore_mutex);

	return 0;
}

int pm_syscore_suspend(suspend_mode_t mode)
{
	int ret = 0;
	struct syscore_ops *ptr = NULL;
	struct list_head *node = NULL;
	struct list_head *head = &syscore_list;

	pm_syscore_call_enter(PM_SYSCORE_TYPE_SUSPEND, mode);

	pm_trace_func("%d", mode);

	if (syscore_stat.cnt) {
		pm_warn("syscore_stat cnt: %d\n", syscore_stat.cnt);
		syscore_stat.cnt = 0;
	}
	syscore_stat.failed_unit = NULL;

	if (pdTRUE != xSemaphoreTake(syscore_mutex, portMAX_DELAY)) {
		pm_semapbusy(syscore_mutex);
		ret = -EBUSY;;
		goto exit;
	}

	list_for_each_prev(node, head) {
		ptr = syscore_containerof(node);
		if (ptr && ptr->suspend) {
			if (ptr->data) {
				ret = ptr->suspend(ptr->data, mode);
			} else {
				ret = ptr->suspend(NULL, mode);
			}
			if (!ret) {
				syscore_stat.cnt ++;
			} else {
				syscore_stat.failed_unit = ptr;
				pm_err("syscore suspend failed: %s(%p)\n",\
						ptr->name, ptr);
				break;
			}
		}
	}

	if (syscore_stat.failed_unit) {
		list_for_each(node, head) {
			ptr = syscore_containerof(node);
			if (syscore_stat.cnt && ptr && ptr->resume) {
				if (ptr->data) {
					ptr->resume(ptr->data, mode);
				} else {
					ptr->resume(NULL, mode);
				}
				syscore_stat.cnt --;
			}
		}
	}

	xSemaphoreGive(syscore_mutex);

exit:
	if (mode == PM_MODE_HIBERNATION) {
		ret = 0;
	}
	pm_syscore_call_exit(PM_SYSCORE_TYPE_SUSPEND, mode);
	return ret;
}

int pm_syscore_resume(suspend_mode_t mode)
{
	int ret = 0;
	struct syscore_ops *ptr = NULL;
	struct list_head *node = NULL;
	struct list_head *head = &syscore_list;

	pm_syscore_call_enter(PM_SYSCORE_TYPE_RESUME, mode);

	pm_trace_func("%d", mode);

	if (pdTRUE != xSemaphoreTake(syscore_mutex, portMAX_DELAY)) {
		pm_semapbusy(syscore_mutex);
		ret = -EBUSY;;
		goto exit;
	}

	list_for_each(node, head) {
		ptr = syscore_containerof(node);
		if (ptr && ptr->resume) {
			if (ptr->data) {
				ptr->resume(ptr->data, mode);
			} else {
				ptr->resume(NULL, mode);
			}
		}
	}
	xSemaphoreGive(syscore_mutex);

exit:
	pm_syscore_call_exit(PM_SYSCORE_TYPE_RESUME, mode);
	return ret;
}

int pm_syscore_init(void)
{
	syscore_mutex = xSemaphoreCreateMutex();
	return 0;
}

int pm_syscore_exit(void)
{
	vSemaphoreDelete(syscore_mutex);
	return 0;
}

