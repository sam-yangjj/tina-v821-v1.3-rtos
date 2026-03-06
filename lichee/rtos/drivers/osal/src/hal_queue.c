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
#include <stdio.h>
#include <fcntl.h>
#include <hal_osal.h>
#include <string.h>
#include <port_misc.h>

hal_queue_t hal_queue_create(const char *name, unsigned int item_size, unsigned int queue_size)
{
	return xQueueCreate(queue_size, item_size);
}
int hal_queue_delete(hal_queue_t queue)
{
	int ret;

	if (hal_interrupt_get_nest())
		ret = uxQueueMessagesWaitingFromISR(queue);
	else
		ret = uxQueueMessagesWaiting(queue);

	if (ret > 0) {
		return HAL_BUSY;
	}

	vQueueDelete(queue);

	return HAL_OK;
}

int hal_queue_send_wait(hal_queue_t queue, void *buffer, hal_tick_t timeout)
{
	BaseType_t ret;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (hal_interrupt_get_nest()) {
		ret = xQueueSendFromISR(queue, buffer, &xHigherPriorityTaskWoken);
		if (ret == pdPASS) {
			portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		ret = xQueueSend(queue, buffer, timeout);

	if (ret != pdPASS) {
		printf("send queue fail\n");
		return HAL_ERROR;
	}
	return HAL_OK;
}

int hal_queue_send(hal_queue_t queue, void *buffer)
{
	return hal_queue_send_wait(queue, buffer, 0);
}

int hal_queue_recv(hal_queue_t queue, void *buffer, hal_tick_t timeout)
{
	BaseType_t ret;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (hal_interrupt_get_nest()) {
		ret = xQueueReceiveFromISR(queue, buffer, &xHigherPriorityTaskWoken);
		if (ret == pdPASS) {
			portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		ret = xQueueReceive(queue, buffer, timeout);

	if (ret != pdPASS) {
		return HAL_ERROR;
	}

	return HAL_OK;
}

int hal_is_queue_empty(hal_queue_t queue)
{
	int arg;

	if (hal_interrupt_get_nest())
		arg = uxQueueMessagesWaitingFromISR(queue);
	else
		arg = uxQueueMessagesWaiting(queue);

	return (arg == 0) ? 1 : 0;
}

hal_mailbox_t hal_mailbox_create(const char *name, unsigned int size)
{
	return (hal_mailbox_t)hal_queue_create(name, sizeof(unsigned int), size);
}


int hal_mailbox_delete(hal_mailbox_t mailbox)
{
	return hal_queue_delete((hal_queue_t)mailbox);
}

int hal_mailbox_send_wait(hal_mailbox_t mailbox, unsigned int value, int timeout)
{
	return hal_queue_send_wait((hal_queue_t)mailbox, &value, timeout);
}

int hal_mailbox_send(hal_mailbox_t mailbox, unsigned int value)
{
	return hal_mailbox_send_wait(mailbox, value, 0);
}

int hal_mailbox_recv(hal_mailbox_t mailbox, unsigned int *value, int timeout)
{
	return hal_queue_recv((hal_queue_t)mailbox, value, timeout);
}

int hal_is_mailbox_empty(hal_mailbox_t mailbox)
{
	return hal_is_queue_empty((hal_queue_t)mailbox);
}

