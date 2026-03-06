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
#include <hal_osal.h>

void *hal_thread_create(void (*threadfn)(void *data), void *data, const char *namefmt, int stacksize, int priority)
{
    TaskHandle_t pxCreatedTask = NULL;
    BaseType_t ret = 0;;
    ret = xTaskCreate(threadfn, namefmt, stacksize, data, priority, &pxCreatedTask);
    if (ret != pdPASS)
    {
        hal_log_err("create task failed!\n");
    }
    return pxCreatedTask;
}

void *hal_thread_self(void)
{
    return (void *)xTaskGetCurrentTaskHandle();
}

int hal_thread_start(void *thread)
{
    return HAL_OK;
}

int hal_thread_stop(void *thread)
{
    vTaskDelete(thread);
    return HAL_OK;
}

int hal_thread_resume(void *task)
{
    vTaskResume(task);
    return HAL_OK;
}

int hal_thread_suspend(void *task)
{
    vTaskSuspend(task);
    return HAL_OK;
}

int hal_thread_scheduler_is_running(void)
{
	return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
}

int hal_thread_is_in_critical_context(void)
{
    if (hal_interrupt_get_nest() == 0
			&& hal_thread_scheduler_is_running()
			&& !hal_interrupt_is_disable())
    {
        return 0;
    }
    return 1;
}

char *hal_thread_get_name(void *thread)
{
    return pcTaskGetName(thread);
}

int hal_thread_scheduler_suspend(void)
{
    vTaskSuspendAll();
    return HAL_OK;
}

int hal_thread_scheduler_resume(void)
{
    return xTaskResumeAll();
}

uint32_t hal_thread_get_task_num(void)
{
	return (uint32_t)uxTaskGetNumberOfTasks();
}

uint32_t hal_thread_get_suspend_status(hal_thread_suspend_t *status, uint32_t num)
{
	uint32_t i;
	TaskStatus_t *task_status;
	uint32_t rtask_num;

	if (status == NULL || num == 0) {
		hal_log_err("hal thread suspend status param err\n");
		return 0;
	}

	task_status = (TaskStatus_t *)hal_malloc(sizeof(*task_status) * num);
	if (task_status == NULL) {
		hal_log_err("hal thread suspend status malloc fail\n");
		return 0;
	}
	rtask_num = uxTaskGetSystemState(task_status, num, NULL);
	for (i = 0; i < rtask_num; i++) {
		status[i].thread = task_status[i].xHandle;
		if (task_status[i].eCurrentState == eRunning ||
			task_status[i].eCurrentState == eReady ||
			task_status[i].eCurrentState == eBlocked) {
			status[i].allow_suspend = 1;
		} else {
			status[i].allow_suspend = 0;
		}
	}
	hal_free(task_status);
	return rtask_num;
}

