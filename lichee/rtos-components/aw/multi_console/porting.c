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
#include <stdlib.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <hal_thread.h>
#include <hal_interrupt.h>

#include "multi_console_internal.h"

#define LOCAL_STORAGE_CONSOLE_INDEX (1)

void *cli_port_current_task(void)
{
	return xTaskGetCurrentTaskHandle();
}

void *cli_port_get_thread_data(void *task)
{
	if (!task)
		return NULL;
	return pvTaskGetThreadLocalStoragePointer(task, LOCAL_STORAGE_CONSOLE_INDEX);
}

void *cli_port_set_thread_data(void *task, void *console)
{
	void *last = NULL;

	if (!task)
		return NULL;

	last = pvTaskGetThreadLocalStoragePointer(task, LOCAL_STORAGE_CONSOLE_INDEX);
	vTaskSetThreadLocalStoragePointer(task, LOCAL_STORAGE_CONSOLE_INDEX, console);

	return last;
}

void *cli_port_thread_cteate(const char *name, console_thread_t entry,
				uint32_t stack_size, uint32_t priority, void *param)
{
	return hal_thread_create(entry, param, name, stack_size, priority);
}

int cli_port_thread_destory(void *task)
{
	return hal_thread_stop(task);
}

int cli_port_is_in_irq(void)
{
	return in_interrupt();
}

char *cli_port_get_task_name(void *tcb)
{
	return pcTaskGetName(tcb);
}

extern BaseType_t FreeRTOS_CLIProcessCommand( const char *pcCommandInput, char *pcWriteBuffer, size_t xWriteBufferLen);

int cli_port_parse_string(const char *input, char *output, size_t len)
{
	return FreeRTOS_CLIProcessCommand(input, output, len);
}
