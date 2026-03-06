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
#include "port_misc.h"
#include <trace_event.h>

int hal_mutex_init(hal_mutex_t mutex)
{
	trace_event_count(EV_MUTEX, "", mutex, ARG_INT_RENAME(mutex, 1));
	mutex->ptr = xSemaphoreCreateMutexStatic(&mutex->entry);
	if (mutex->ptr == NULL) {
		return HAL_ERROR;
	}
	return HAL_OK;
}

int hal_mutex_detach(hal_mutex_t mutex)
{
	vSemaphoreDelete(mutex->ptr);
	mutex->ptr = NULL;
	return HAL_OK;
}

hal_mutex_t hal_mutex_create(void)
{
	/* default not support recursive */
	int ret = 0;
	hal_mutex_t mutex = hal_malloc(sizeof(*mutex));
	if (!mutex)
		return NULL;

	ret = hal_mutex_init(mutex);
	if (ret == HAL_ERROR) {
		free(mutex);
		mutex = NULL;
		return NULL;
	}
	return mutex;
}

int hal_mutex_delete(hal_mutex_t mutex)
{
	trace_event_count(EV_MUTEX, "", mutex, ARG_INT_RENAME(mutex, 0));
	if (mutex == NULL || mutex->ptr == NULL) {
		return HAL_ERROR;
	}
	hal_mutex_detach(mutex);
	hal_free(mutex);

	return HAL_OK;
}

int hal_mutex_unlock(hal_mutex_t mutex)
{
	BaseType_t ret;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (mutex == NULL || mutex->ptr == NULL) {
		return HAL_INVALID;
	}
	trace_event_end(EV_MUTEX, "unlock", ARG_PTR(mutex));

	if (hal_interrupt_get_nest()) {
		ret = xSemaphoreGiveFromISR(mutex->ptr, &xHigherPriorityTaskWoken);
		if (ret == pdPASS) {
			portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		ret = xSemaphoreGive(mutex->ptr);

	if (ret != pdPASS) {
		return HAL_ERROR;
	}

	return HAL_OK;
}

int hal_mutex_timedwait(hal_mutex_t mutex, int ticks)
{
	TickType_t xDelay = (TickType_t)ticks;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t ret;

	if (mutex == NULL || mutex->ptr == NULL) {
		return HAL_INVALID;
	}

	trace_event_begin(EV_MUTEX, "lock", ARG_PTR(mutex), ARG_INT(ticks));

	if (hal_interrupt_get_nest()) {
		ret = xSemaphoreTakeFromISR(mutex->ptr, &xHigherPriorityTaskWoken);
		if (ret == pdPASS) {
			portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		ret = xSemaphoreTake(mutex->ptr, xDelay);

	if (ret != pdPASS) {
		trace_event_end(EV_MUTEX, "lock_fail");
		return HAL_ERROR;
	}

	return HAL_OK;
}

int hal_mutex_lock(hal_mutex_t mutex)
{
	TickType_t xDelay = portMAX_DELAY;

	return hal_mutex_timedwait(mutex, xDelay);
}

int hal_mutex_trylock(hal_mutex_t mutex)
{
	TickType_t xDelay = 0;

	return hal_mutex_timedwait(mutex, xDelay);
}

