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
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <hal_sem.h>
#include <hal_time.h>
#include <hal_interrupt.h>
#include <hal_event.h>
#include <hal_osal.h>
#include <hal_status.h>


int hal_event_init(hal_event_t ev)
{
	hal_event_t pevent;

	pevent = xEventGroupCreateStatic((StaticEventGroup_t *)ev);
	if (!pevent)
		return HAL_NOMEM;
	return HAL_OK;
}

int hal_event_datach(hal_event_t ev)
{
	(void)ev;
	return HAL_OK;
}

hal_event_t hal_event_create(void)
{
	return xEventGroupCreate();
}

int hal_event_delete(hal_event_t ev)
{
	if (!ev)
		return HAL_INVALID;

	vEventGroupDelete(ev);
	return HAL_OK;
}

hal_event_bits_t hal_event_wait(hal_event_t ev, hal_event_bits_t evs, uint8_t option, unsigned long timeout)
{
	int clear, and;

	if (!ev)
		return HAL_INVALID;

	if (option & HAL_EVENT_OPTION_AND) {
		and = 1;
	} else if (option & HAL_EVENT_OPTION_OR) {
		and = 0;
	} else {
		return HAL_INVALID;
	}

	if (option & HAL_EVENT_OPTION_CLEAR)
		clear = 1;
	else
		clear = 0;

	return xEventGroupWaitBits(ev, evs, clear, and, timeout);
}

int hal_event_set_bits(hal_event_t ev, hal_event_bits_t evs)
{
	if (!ev)
		return HAL_INVALID;

	if (hal_interrupt_get_nest()) {
		BaseType_t xHigherPriorityTaskWoken, xResult;

		xResult = xEventGroupSetBitsFromISR(ev, evs, &xHigherPriorityTaskWoken);
		if (xResult != pdFAIL)
			return HAL_OK;
		else
			return HAL_ERROR;
	} else {
		hal_event_bits_t bits;
		bits = xEventGroupSetBits(ev, evs);
		if (bits > 0)
			return HAL_OK;
		else
			return HAL_ERROR;
	}
}

hal_event_bits_t hal_event_get(hal_event_t ev)
{
	if (hal_interrupt_get_nest())
		return xEventGroupGetBitsFromISR(ev);
	else
		return xEventGroupGetBits(ev);
}

hal_event_bits_t hal_event_clear_bits(hal_event_t ev, hal_event_bits_t evs)
{
	if (hal_interrupt_get_nest())
		return xEventGroupClearBitsFromISR(ev, evs);
	else
		return xEventGroupClearBits(ev, evs);
}
