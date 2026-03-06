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
#include <hal_uart.h>
#include <stdint.h>
#include <spinlock.h>

#include "multi_console_internal.h"

int uart_console_write(const void *buf, size_t len, void *priv)
{
	int i;
	const char *pc = buf;
	for (i = 0; i < len; i++) {
		if (pc[i] == '\n')
			hal_uart_put_char(CONFIG_CLI_UART_PORT, '\r');
		hal_uart_put_char(CONFIG_CLI_UART_PORT, pc[i]);
	}

	return len;
}

int uart_console_read(void *buf, size_t len, void *priv, uint32_t timeout)
{
	return hal_uart_receive_no_block(CONFIG_CLI_UART_PORT, (uint8_t *)buf, len, timeout);
}

int uart_console_terminate_read(void *priv)
{
	return hal_uart_terminate_receive(CONFIG_CLI_UART_PORT);
}

static int uart_console_init(void *private_data)
{
	//return hal_uart_init(CONFIG_CLI_UART_PORT);
	return 0;
}

static cli_dev_ops uart_console_ops = {
	.write = uart_console_write,
	.read = uart_console_read,
	.terminate_read = uart_console_terminate_read,
	.init = uart_console_init,
	.deinit = NULL,
	.priv = NULL,
#ifdef CONFIG_UART_MULTI_CONSOLE_AS_MAIN
	.echo = 1,
	.task = 1,
	.prefix = "uart>",
#else
	.echo = 0,
	.task = 0,
	.prefix = NULL,
#endif
};
static cli_console *console;

int uart_multi_console_register(void)
{
	if (console)
		return -EEXIST;
	console = cli_console_create(&uart_console_ops, "uart");
	if (!console)
		return -ENOMEM;

	set_default_console(console);
	return 0;
}

int uart_multi_console_unregister(void)
{
	int ret = 0;

	if (console) {
		ret = cli_console_destory(console);
		console = NULL;
	}
	return ret;
}
