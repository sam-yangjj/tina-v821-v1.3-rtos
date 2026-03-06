/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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
#include <stdlib.h>
#include <stdlib.h>
#include "kernel/os/os_thread.h"
#include "kernel/os/os_time.h"
#include "kernel/os/os_debug.h"
#include "kernel/os/os_stby.h"
#include "FreeRTOS.h"

#if (defined CONFIG_DRIVER_V821) && (defined CONFIG_STANDBY_HEAP)
void *xr_unsaved_malloc(uint32_t siz)
{
	return hal_malloc_unsaved(siz);
}

void xr_unsaved_free(void *ptr)
{
	hal_free_unsaved(ptr);
}

XR_OS_Status XR_OS_ThreadCreate_unsaved(XR_OS_Thread_t *thread, const char *name,
                          XR_OS_ThreadEntry_t entry, void *arg,
                          XR_OS_Priority priority, uint32_t stackSize)
{
	UBaseType_t prio;

	switch (priority) {
	case XR_OS_PRIORITY_IDLE:
		prio = HAL_THREAD_PRIORITY_LOWEST;
		break;
	case XR_OS_PRIORITY_LOW:
		prio = HAL_THREAD_PRIORITY_LOWEST + 1;
		break;
	case XR_OS_PRIORITY_BELOW_NORMAL:
		prio = HAL_THREAD_PRIORITY_MIDDLE - 1;
		break;
	case XR_OS_PRIORITY_NORMAL:
		prio = HAL_THREAD_PRIORITY_MIDDLE;
		break;
	case XR_OS_PRIORITY_NEAR_ABOVE_NORMAL:
		prio = HAL_THREAD_PRIORITY_MIDDLE + 1;
		break;
	case XR_OS_PRIORITY_ABOVE_NORMAL:
		prio = HAL_THREAD_PRIORITY_MIDDLE + 2;
		break;
	case XR_OS_PRIORITY_BELOW_HIGH:
		prio = HAL_THREAD_PRIORITY_HIGHEST - 2;
		break;
	case XR_OS_PRIORITY_HIGH:
		prio = HAL_THREAD_PRIORITY_HIGHEST - 1;
		break;
	case XR_OS_PRIORITY_REAL_TIME:
		prio = HAL_THREAD_PRIORITY_HIGHEST;
		break;
	default:
		prio = HAL_THREAD_PRIORITY_MIDDLE;
		break;
	}

	XR_OS_HANDLE_ASSERT(!XR_OS_ThreadIsValid(thread), thread->handle);

	thread->handle = hal_thread_create_unsaved(entry, arg, name, stackSize / sizeof(StackType_t), prio);
	if (thread->handle == NULL) {
		XR_OS_ERR("err %"XR_OS_BASETYPE_F"\n", errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY);
		XR_OS_ThreadSetInvalid(thread);
		return XR_OS_FAIL;
	}
	return XR_OS_OK;
}

XR_OS_Status XR_OS_ThreadDelete_unsaved(XR_OS_Thread_t *thread)
{
	TaskHandle_t handle;
	TaskHandle_t curHandle;

	if (thread == NULL) {
		vTaskDelete(NULL); /* delete self */
		return XR_OS_OK;
	}

	XR_OS_HANDLE_ASSERT(XR_OS_ThreadIsValid(thread), thread->handle);

	handle = thread->handle;
	curHandle = xTaskGetCurrentTaskHandle();
	if (handle == curHandle) {
		/* delete self */
		XR_OS_ThreadSetInvalid(thread);
		hal_thread_stop_unsaved(NULL);
	} else {
		/* delete other thread */
		XR_OS_WRN("thread %"XR_OS_HANDLE_F" delete %"XR_OS_HANDLE_F"\n", curHandle, handle);
		hal_thread_stop_unsaved(handle);
		XR_OS_ThreadSetInvalid(thread);
	}

	return XR_OS_OK;
}
#else
void *xr_unsaved_malloc(uint32_t siz)
{
	return hal_malloc(siz);
}

void xr_unsaved_free(void *ptr)
{
	hal_free(ptr);
}

XR_OS_Status XR_OS_ThreadCreate_unsaved(XR_OS_Thread_t *thread, const char *name,
                          XR_OS_ThreadEntry_t entry, void *arg,
                          XR_OS_Priority priority, uint32_t stackSize)
{
	return XR_OS_ThreadCreate(thread, name, entry, arg, priority, stackSize);
}

XR_OS_Status XR_OS_ThreadDelete_unsaved(XR_OS_Thread_t *thread)
{
	return XR_OS_ThreadDelete(thread);
}
#endif



