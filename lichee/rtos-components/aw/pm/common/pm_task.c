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
#include <string.h>
#include <osal/hal_interrupt.h>
#include <errno.h>
#include <hal/aw_list.h>
#include <osal/hal_timer.h>
#include <osal/hal_mem.h>
#include <hal_atomic.h>
#include <osal/hal_timer.h>
#include <osal/hal_thread.h>

#include <pm_adapt.h>
#include <pm_debug.h>
#include <pm_task.h>
#include <pm_wakelock.h>
#ifdef CONFIG_PM_STANDBY_MEMORY
#include <pm_mem.h>
#endif

#ifdef CONFIG_COMPONENTS_PM
typedef enum {
	PM_TASK_OPT_SUSPEND = 0,
	PM_TASK_OPT_RESUME,
	PM_TASK_OPT_MAX,
}pm_task_opt_t;

#define pm_task_containerof(ptr_module) \
        __containerof(ptr_module, struct pm_task, node)

#define TASK_IDLE_NAME  "IDLE"
#define TASK_TIMR_NAME  "Tmr Svc"

static struct list_head pm_task_list = LIST_HEAD_INIT(pm_task_list);
static struct list_head pm_save_list = LIST_HEAD_INIT(pm_save_list);

static struct pm_task *pm_task_check_inlist(hal_thread_t thread)
{
	struct list_head *list = &pm_task_list;
	struct list_head *pos = NULL;
	struct pm_task *task = NULL;

	if (thread == NULL) {
		return NULL;
	}

	list_for_each(pos, list) {
		task = pm_task_containerof(pos);
		if(task->thread == thread) {
			return task;
		}
	}
	return NULL;
}

int pm_task_register(hal_thread_t thread, pm_task_type_t type)
{
	struct pm_task *task = NULL;

	if (thread == NULL || type >= PM_TASK_TYPE_MAX) {
		return -EINVAL;
	}
	if (type == PM_TASK_TYPE_APP) {
		return 0;
	}
	if (pm_task_check_inlist(thread)) {
		return -EINVAL;
	}
	task = (struct pm_task *)hal_malloc(sizeof(*task));
	if(task == NULL) {
		return -ENOMEM;
	}
	task->thread = thread;
	list_add_tail(&task->node, &pm_task_list);
	return 0;
}

int pm_task_unregister(hal_thread_t thread)
{
	struct pm_task *task = NULL;

	if (thread == NULL) {
		return -EINVAL;
	}
	task = pm_task_check_inlist(thread);
	if(task) {
		list_del(&task->node);
		hal_free(task);
	}
	return 0;
}

int pm_task_freeze(pm_task_type_t type)
{
	uint32_t i;
	int ret = 0;
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	struct pm_task *task = NULL;
	uint32_t task_num;
	uint32_t rtask_num;
	hal_thread_suspend_t *task_status;
	struct pm_task *save_task;
	bool need_suspend;

	hal_thread_scheduler_suspend();
	pm_time_record_start(PM_TIME_RECORD_TASK_FREEZE);
	pm_log("pm task start freeze\n");

	if (type >= PM_TASK_TYPE_MAX) {
		pm_err("pm task type err\n");
		ret = -EINVAL;
		goto exit;
	}

	task_num = hal_thread_get_task_num();
	pm_log("pm task task num: %u\n", task_num);
	task_status = (hal_thread_suspend_t *)hal_malloc(sizeof(*task_status) * task_num);
	if (task_status == NULL) {
		pm_err("pm task malloc task status fail\n");
		ret = -ENOMEM;
		goto exit;
	}
	rtask_num = hal_thread_get_suspend_status(task_status, task_num);
	if (!rtask_num) {
		pm_err("task num changed during get task status\n");
	}
	for (i = 0; i < rtask_num; i++) {
		if (!task_status[i].allow_suspend) {
			continue;
		}
		need_suspend = 1;
		list_for_each(pos, &pm_task_list) {
			task = pm_task_containerof(pos);
			if (task->thread == task_status[i].thread) {
				need_suspend = 0;
				break;
			}
		}
		if (need_suspend) {
			save_task = (struct pm_task *)hal_malloc(sizeof(*save_task));
			if (save_task == NULL) {
				pm_err("pm save task malloc fail\n");
				goto save_err;
			}
			save_task->thread = task_status[i].thread;
			list_add_tail(&save_task->node, &pm_save_list);
			hal_thread_suspend(save_task->thread);
			pm_log("freeze task: %s\n", hal_thread_get_name(task_status[i].thread));
		} else {
			pm_log("skip suspend task: %s\n", hal_thread_get_name(task_status[i].thread));
		}
	}
	goto finish;

save_err:
	list_for_each_safe(pos, n, &pm_save_list) {
		save_task = pm_task_containerof(pos);
		hal_thread_resume(save_task->thread);
		list_del(&save_task->node);
		hal_free(save_task);
		pm_err("restore task: %s\n", hal_thread_get_name(save_task->thread));
	}
finish:
	hal_free(task_status);
exit:
	hal_thread_scheduler_resume();
	pm_log("pm task end freeze\n");
	pm_time_record_end(PM_TIME_RECORD_TASK_FREEZE);
	return ret;
}

int pm_task_restore(pm_task_type_t type)
{
	uint32_t i;
	int ret = 0;
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	uint32_t task_num;
	uint32_t rtask_num;
	hal_thread_suspend_t *task_status;
	struct pm_task *save_task;
	bool need_resume;

	hal_thread_scheduler_suspend();
	pm_time_record_start(PM_TIME_RECORD_TASK_RESTORE);
	pm_log("pm task start restore\n");
	if (type >= PM_TASK_TYPE_MAX) {
		ret = -EINVAL;
		goto exit;
	}
	task_num = hal_thread_get_task_num();
	pm_log("pm task num: %u\n", task_num);
	task_status = (hal_thread_suspend_t *)hal_malloc(sizeof(*task_status) * task_num);
	if (task_status == NULL) {
		pm_err("pm task malloc task status fail\n");
		ret = -ENOMEM;
		goto exit;
	}
	rtask_num = hal_thread_get_suspend_status(task_status, task_num);
	if (!rtask_num) {
		pm_err("task num changed during get task status\n");
	}
	list_for_each_safe(pos, n, &pm_save_list) {
		save_task = pm_task_containerof(pos);
		need_resume = 0;
		for (i = 0; i < rtask_num; i++) {
			if (save_task->thread == task_status[i].thread) {
				need_resume = 1;
				break;
			}
		}
		if (need_resume) {
			hal_thread_resume(save_task->thread);
			pm_log("restore task: %s\n", hal_thread_get_name(save_task->thread));
		}
		list_del(&save_task->node);
		hal_free(save_task);
	}
	hal_free(task_status);
exit:
	hal_thread_scheduler_resume();
	pm_log("pm task end restore\n");
	pm_time_record_end(PM_TIME_RECORD_TASK_RESTORE);
	return ret;
}

int pm_task_init(void)
{
	uint32_t i;

	/* protect Idle/Timer task */
	TaskHandle_t  xtask;
	const char *sys_task[] = {TASK_IDLE_NAME, TASK_TIMR_NAME};
	for (i=0; i<sizeof(sys_task)/sizeof(sys_task[0]); i++) {
		xtask = xTaskGetHandle(sys_task[i]);
		if (NULL == xtask) {
			pm_err("Can't find task(%s)\n", sys_task[i]);
			continue;
		}
		pm_task_register(xtask, PM_TASK_TYPE_SYS);
	}
	return 0;
}

int pm_task_deinit(void)
{
	return 0;
}
#endif
