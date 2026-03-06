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
#include <FreeRTOS.h>
#include <task.h>
#include <stdlib.h>
#include <string.h>
#include <osal/hal_interrupt.h>
#include <errno.h>
#include <console.h>

#include <pm_adapt.h>
#include <pm_debug.h>
#include <pm_suspend.h>
#include <pm_wakesrc.h>
#include <pm_wakelock.h>
#include <pm_testlevel.h>
#include <pm_devops.h>
#include <pm_syscore.h>
#include <pm_notify.h>
#include <pm_task.h>
#include <pm_platops.h>
#include <pm_subsys.h>
#include <pm_state.h>
#ifdef CONFIG_ARCH_RISCV_E907
#include <pm_plat.h>
#endif
#ifdef CONFIG_COMPONENTS_VIRT_LOG
#include <virt_log.h>
#endif
#ifdef CONFIG_ARCH_DSP
#include <aw_io.h>
#endif
#ifdef CONFIG_PM_DEBUG_BACKTRACE_PERIODICALLY
#include <backtrace.h>
#endif

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_SUSPEND

#if !defined(CONFIG_COMPONENTS_VIRT_LOG) || defined(CONFIG_PM_DISABLE_ALL_LOG)
extern void printf_lock(void);
extern void printf_unlock(void);
#endif

static void pm_suspend_task(void *nouse);
static int pm_suspend_devices_and_enter(suspend_mode_t mode);
static int pm_suspend_enter(suspend_mode_t mode);

static suspend_mode_t suspend_mode = PM_MODE_ON;
static pm_standby_mode_t standby_mode = PM_STANDBY_MODE_SUPER;

typedef enum {
	PM_VIRT_LOG_EARLY_STAGE = 0,
	PM_VIRT_LOG_LATE_STAGE,

	PM_VIRT_LOG_STAGE_MAX,
	PM_VIRT_LOG_STAGE_BASE = PM_VIRT_LOG_EARLY_STAGE,
} pm_virt_log_stage_t;
static pm_virt_log_stage_t virt_log_stage = PM_VIRT_LOG_LATE_STAGE;

/* Standby stage and time record */
void pm_record_stage(uint32_t val, uint8_t cover)
{
	uint32_t rec_reg;
	uint32_t rec_val;

	if (!pm_test_standby_recording())
		return;

	rec_reg = PM_STANDBY_STAGE_REC_REG;

	rec_val = (cover) ? (val) : (readl(rec_reg) | val);
	writel(rec_val, rec_reg);
}

void pm_set_standby_mode(pm_standby_mode_t mode)
{
	standby_mode = mode;
}

pm_standby_mode_t pm_get_standby_mode(void)
{
	return standby_mode;
}

void pm_suspend_mode_change(suspend_mode_t mode)
{
	suspend_mode_t tmp_mode = suspend_mode;

	if (!pm_suspend_mode_valid(mode))
		return;

	suspend_mode = mode;
	pm_warn("suspend_mode(%d) changes to mode(%d)\n", tmp_mode, suspend_mode);
}

static void pm_virt_log(int virt)
{
#ifndef CONFIG_PM_DISABLE_ALL_LOG
	if (virt) {
		pm_time_record_start(PM_TIME_RECORD_VIRTLOG_ENABLE);
#ifdef CONFIG_COMPONENTS_VIRT_LOG
		virt_log_enable(1);
#else
		printf_lock();
#endif
		pm_time_record_end(PM_TIME_RECORD_VIRTLOG_ENABLE);
	} else {
		pm_time_record_start(PM_TIME_RECORD_VIRTLOG_DISABLE);
#ifdef CONFIG_COMPONENTS_VIRT_LOG
		virt_log_enable(0);
#else
		printf_unlock();
#endif
		pm_time_record_end(PM_TIME_RECORD_VIRTLOG_DISABLE);
	}
#endif
}

static int cmd_pm_set_virt_log(int argc, char **argv)
{
	if ((argc != 2) || (atoi(argv[1]) < PM_VIRT_LOG_STAGE_BASE) || (atoi(argv[1]) >= PM_VIRT_LOG_STAGE_MAX)) {
		pm_err("%s: Invalid params for pm_set_virt_log\n", __func__);
		return -EINVAL;
	}

	virt_log_stage = atoi(argv[1]);

	pm_warn("PM set virt_log_stage to %d\n", virt_log_stage);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_pm_set_virt_log, pm_set_virt_log, pm_test_tools)

static struct pm_suspend_thread_t *pm_suspend_thread = NULL;
#ifdef CONFIG_PM_DEBUG_BACKTRACE_PERIODICALLY
static hal_thread_t pm_trace_thread;
static void pm_trace_task(void *nouse)
{
	while (1) {
		hal_msleep(10000);
		if (pm_suspend_thread) {
			printf("PM process backtrace:\n");
			backtrace(hal_thread_get_name(pm_suspend_thread->thread), NULL, 0, 0, printf);
		}
	}
}
#endif

int  pm_suspend_init(void)
{
	if (pm_suspend_thread) {
		pm_err("thread start again\n");
		return -EPERM;
	}

#ifdef CONFIG_PM_DEBUG_BACKTRACE_PERIODICALLY
	pm_trace_thread = hal_thread_create(pm_trace_task, NULL, "pm_trace", 384, HAL_THREAD_PRIORITY_HIGHEST);
	if (pm_trace_thread == NULL) {
		pm_err("create trace thread failed\n");
		return -EPERM;
	}
	pm_task_register(pm_trace_thread, PM_TASK_TYPE_PM);
#endif

	pm_suspend_thread = malloc(sizeof(struct pm_suspend_thread_t));
	if (!pm_suspend_thread) {
		pm_err("thread malloc failed\n");
		return -EPERM;
	}

	memset(pm_suspend_thread, 0, sizeof(struct pm_suspend_thread_t));
	pm_suspend_thread->queue = hal_queue_create("pm_queue", sizeof( uint32_t ), 1);
	if (NULL == pm_suspend_thread->queue) {
		pm_err("create queue failed\n");
		return -EPERM;
	}
#ifdef CONFIG_ARCH_SUN300IW1
	pm_suspend_thread->thread = hal_thread_create(pm_suspend_task, NULL, "pm_suspend", 384, PM_TASK_PRIORITY);
#else
	pm_suspend_thread->thread = hal_thread_create(pm_suspend_task, NULL, "pm_suspend", 1024, PM_TASK_PRIORITY);
#endif
	if (pm_suspend_thread->thread == NULL) {
		pm_err("create thread failed\n");
		return -EPERM;
	}

	/* the task can not freeze. */
	pm_task_register(pm_suspend_thread->thread, PM_TASK_TYPE_PM);

	return 0;
}

int pm_suspend_exit(void)
{
	if (!pm_suspend_thread) {
		pm_err("thread stop again\n");
		return -EPERM;
	}
	if (pm_suspend_thread->thread) {
		hal_thread_stop(pm_suspend_thread->thread);
	}
	hal_queue_delete(pm_suspend_thread->queue);

	free(pm_suspend_thread);
	pm_suspend_thread = NULL;

	return 0;
}

static void pm_suspend_task(void *nouse)
{
	int ret = -1;
	uint32_t mode;
	uint32_t t_cnt = 0, t_inpr = 0, t_inpr_intr = 0;

	while (1) {
		if (hal_queue_recv(pm_suspend_thread->queue, &mode, HAL_WAIT_FOREVER) != HAL_OK) {
			continue;
		}
#ifdef CONFIG_PM_DISABLE_ALL_LOG
		printf_lock();
#endif
		ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_PREPARED_NOTIFY, mode);
		if (ret) {
			goto exit;
		}

		if (mode != PM_MODE_HIBERNATION) {
			pm_wakesrc_get_inpr_cnt(&t_cnt, &t_inpr, &t_inpr_intr);
			if (t_inpr) {
				goto exit;
			}
			pm_wakesrc_save_cnt(t_cnt);
		}
		pm_trace_info("suspend begin.");
		pm_state_set(PM_STATUS_SLEEPING);
		suspend_mode = mode;

#ifdef CONFIG_COMPONENTS_PM_CORE_M33
		/* notify system suspend. */
		ret = pm_notify_event(mode, PM_EVENT_SYS_PERPARED);
		if (ret) {
			pm_err("pm entry notify returns return: %d, suspend abort\n", ret);
			ret = 0;
			pm_state_set(PM_STATUS_RUNNING);
			pm_trace_info("suspend end.");
			continue;
		}
#endif
		switch (mode) {
		case PM_MODE_ON:
			break;
		case PM_MODE_SLEEP:
		case PM_MODE_STANDBY:
		case PM_MODE_HIBERNATION:
			ret = pm_suspend_devices_and_enter(mode);
			pm_report_stats();
			break;
		default:
			pm_err("%s: Undefined suspend mode(%d)\n", __func__, mode);
			break;
		}
#ifdef CONFIG_COMPONENTS_PM_CORE_M33
		pm_notify_event(mode, PM_EVENT_SYS_FINISHED);
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_RISCV
               if (ret) {
                       writel(PM_SUBSYS_ACTION_FAILED_FLAG, RV_ACTION_REC_REG);
                       pm_err("riscv suspend failed\n");
               } else
                       writel(RV_RESUME_OK, RV_ACTION_REC_REG);
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_DSP
               if (ret) {
                       writel(PM_SUBSYS_ACTION_FAILED_FLAG, DSP_ACTION_REC_REG);
                       pm_err("dsp suspend failed\n");
               } else
                       writel(DSP_RESUME_OK, DSP_ACTION_REC_REG);
#endif

exit:
		pm_platops_call(PM_SUSPEND_OPS_TYPE_FINISHED_NOTIFY, mode);
		pm_state_set(PM_STATUS_RUNNING);
		pm_trace_info("suspend end.");
#if defined(CONFIG_PM_TIME_RECORD)
		//pm_time_record_show();
		//pm_time_record_reset();
#endif
#ifdef CONFIG_PM_DISABLE_ALL_LOG
		printf_unlock();
#endif
	}

	return ;
}

static int pm_suspend_devices_and_enter(suspend_mode_t src_mode)
{
	int ret = 0;
	int ret_again = 0;
	int mode = src_mode;

	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x0, 1);

	/* notify pm prepare event */
	ret = pm_notify_event(mode, PM_EVENT_PERPARED);
	if (ret)
		goto label_pre_begin_out;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x1, 1);

	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_PRE_BEGIN, mode);
	if (ret)
		goto label_notify_sys_finish;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x2, 1);

	/*freeze all task except PM/WLAN/BT is registered etc...*/
	ret = pm_task_freeze(PM_TASK_TYPE_APP);
	if (ret)
		goto label_restore_app;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x3, 1);

	/*debug*/
	if (pm_suspend_test(PM_SUSPEND_TEST_FREEZER))
		goto label_restore_app;

lable_suspend_again_late:
	if (suspend_mode != mode) {
		pm_warn("suspend mode switch to %d, last mode: %d\n", suspend_mode, mode);
		mode = suspend_mode;
	}

	/* subsys suspend */
	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_BEGIN, mode);
	if (ret)
		goto label_restore_app;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x4, 1);

	if (suspend_mode != mode) {
		pm_warn("suspend mode switch to %d, last mode: %d\n", suspend_mode, mode);
		mode = suspend_mode;
	}

	if (virt_log_stage == PM_VIRT_LOG_EARLY_STAGE) {
		pm_log("swtich to virtual log\n");
		pm_virt_log(1);
	}

	/*try to suspend some devices.*/
	ret = pm_devops_prepared(mode);
	if (ret)
		goto label_ops_end;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x5, 1);

	ret = pm_devops_suspend(mode);
	if (ret)
		goto label_ops_recover;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x6, 1);

	/*debug*/
	if (pm_suspend_test(PM_SUSPEND_TEST_DEVICE))
		goto label_dev_resume;

	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_PREPARE, mode);
	if (ret)
		goto label_dev_resume;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x7, 1);

lable_suspend_again:
	ret = pm_suspend_enter(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x700, 1);
	/*
	 * again when all things occurred:
	 * 1, there isn't an error occurred, such as ret==0.
	 * 2, pm_platops_again think we should again, such as return true.
	 * or resume system whatever:
	 * 1, an error occurred.
	 * 2, pm_platops_again think can resume.
	 */
	ret_again = pm_platops_call(PM_SUSPEND_OPS_TYPE_AGAIN, mode);
	if ((ret == PM_SUSPEND_OK || ret == PM_SUSPEND_AGAIN) && ret_again) {
		pm_trace_info("suspend again.");
		pm_state_set(PM_STATUS_AGAINING);
		goto lable_suspend_again;
	}

	pm_trace_info("suspend resume.");
	pm_state_set(PM_STATUS_WAKEUPING);
	pm_platops_call(PM_SUSPEND_OPS_TYPE_FINISH, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x800, 1);

label_dev_resume:
	pm_devops_resume(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x900, 1);

label_dev_complete:
	pm_devops_complete(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xa00, 1);

label_ops_end:
	if (virt_log_stage == PM_VIRT_LOG_EARLY_STAGE) {
		pm_virt_log(0);
		pm_log("restore form virtual log\n");
	}
	pm_platops_call(PM_SUSPEND_OPS_TYPE_END, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xb00, 1);

	if (!ret && pm_platops_call(PM_SUSPEND_OPS_TYPE_AGAIN_LATE, mode)) {
		pm_trace_info("suspend again late.");
		pm_state_set(PM_STATUS_AGAINING);
		goto lable_suspend_again_late;
	}
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xc00, 1);

label_restore_app:
	pm_task_restore(PM_TASK_TYPE_APP);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xd00, 1);
	pm_platops_call(PM_SUSPEND_OPS_TYPE_POST_END, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xe00, 1);

label_notify_sys_finish:
	pm_notify_event(mode, PM_EVENT_FINISHED);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xf00, 1);

label_pre_begin_out:
	return ret;

label_ops_recover:
	pm_platops_call(PM_SUSPEND_OPS_TYPE_RECOVER, mode);
	goto label_dev_complete;
}


#ifndef CONFIG_COMPONENTS_PM_CORE_DSP
extern void arch_disable_all_irq(void);
extern void arch_enable_all_irq(void);
#endif
static int pm_suspend_enter(suspend_mode_t mode)
{
	int  ret = 0;

	/*there try to suspend all devices.*/
	ret = pm_devops_suspend_late(mode);
	if (ret)
		goto lable_out;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x8, 1);

	pm_time_record_start(PM_TIME_RECORD_IRQ_DISABLE);
	/* hal interrupt should be called in pairs */
	hal_interrupt_disable();
#ifndef CONFIG_COMPONENTS_PM_CORE_DSP
	/* Ensure that global interrupt is disabled when poweron.
	 * hal_interrupt_disable() only adjusts the priority in the M33/C906.
	 */
	arch_disable_all_irq();
#endif
	pm_time_record_end(PM_TIME_RECORD_IRQ_DISABLE);

	pm_moment_clear((0x1 << PM_MOMENT_IRQ_DISABLE) | (0x1 << PM_MOMENT_SUSPEND_NOIRQ_END) \
			| (0x1 << PM_MOMENT_ENTER_WFI) | (0x1 << PM_MOMENT_EXIT_WFI) \
			| (0x1 << PM_MOMENT_RESUME_NOIRQ_BEGIN) | (0x1 << PM_MOMENT_IRQ_ENABLE) );
	pm_moment_record(PM_MOMENT_IRQ_DISABLE, 0);

	/*there try to close all devices irq except wakeupsrc irq.*/
	ret = pm_devops_suspend_noirq(mode);
	if (ret)
		goto lable_enable_interrupt;
	pm_devops_stop_report();
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x9, 1);

	/*debug*/
	if (pm_suspend_test(PM_SUSPEND_TEST_PLATFORM))
		goto lable_dev_resume_noirq;

	if (virt_log_stage == PM_VIRT_LOG_LATE_STAGE) {
		pm_log("swtich to virtual log\n");
		pm_virt_log(1);
	}

	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xa, 1);
	ret = pm_syscore_suspend(mode);
	if (ret)
		goto lable_dev_resume_noirq;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xb, 1);
	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_PREPARE_LATE, mode);
	if (ret)
		goto lable_syscore_resume;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xc, 1);

	/*debug*/
	if (pm_suspend_test(PM_SUSPEND_TEST_CPU))
		goto lable_ops_wake;

	/* no debug and no pending*/
	pm_state_set(PM_STATUS_SLEEPED);
	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_ENTER, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x100, 1);

lable_ops_wake:
	pm_state_set(PM_STATUS_ACTIVING);
	pm_platops_call(PM_SUSPEND_OPS_TYPE_WAKE, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x200, 1);

lable_syscore_resume:
	pm_syscore_resume(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x300, 1);

lable_dev_resume_noirq:
	if (virt_log_stage == PM_VIRT_LOG_LATE_STAGE) {
		pm_virt_log(0);
		pm_log("restore form virtual log\n");
	}
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x400, 1);
	pm_devops_start_report();
	pm_devops_resume_noirq(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x500, 1);

lable_enable_interrupt:
	pm_moment_record(PM_MOMENT_IRQ_ENABLE, 0);
	pm_time_record_start(PM_TIME_RECORD_IRQ_ENABLE);
#ifndef CONFIG_COMPONENTS_PM_CORE_DSP
	arch_enable_all_irq();
#endif
	hal_interrupt_enable();
	pm_time_record_end(PM_TIME_RECORD_IRQ_ENABLE);
	pm_devops_resume_early(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x600, 1);
lable_out:
	return ret;
}

int pm_suspend_request(suspend_mode_t mode)
{
	uint32_t xmode = mode;
	BaseType_t xReturned;

	if (pm_platops_call(PM_SUSPEND_OPS_TYPE_VALID, mode))
		return -EINVAL;
	xReturned = hal_queue_send(pm_suspend_thread->queue, &xmode);
	return (HAL_OK == xReturned) ? 0 : -EBUSY;
}

