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
#include <string.h>
#include <stdarg.h>
#include <hal_uart.h>
#include <hal_mem.h>

#include "multi_console_internal.h"

int console_printk(const char *fmt, ...)
{
#if defined(CONFIG_DRIVERS_UART) && defined(CONFIG_CLI_UART_PORT)
#define CONSOLEBUF_SIZE 256
	va_list args;
	size_t length;
	int i = 0;
	char log_buf[CONSOLEBUF_SIZE];

	memset(&log_buf, 0, CONSOLEBUF_SIZE);

	va_start(args, fmt);
	length = vsnprintf(log_buf, sizeof(log_buf) - 1, fmt, args);
	if (length > CONSOLEBUF_SIZE - 1)
		length = CONSOLEBUF_SIZE - 1;
	while (length--)
		hal_uart_put_char(CONFIG_CLI_UART_PORT, log_buf[i++]);

	va_end(args);
#endif

	return 0;
}

int multiple_console_early_init(void)
{
	int ret;

	extern int console_core_init(void);
	ret = console_core_init();
	if (ret)
		return ret;

	console_debug("register null console device.\r\n");
	extern int null_multi_console_register(void);
	ret = null_multi_console_register();
	if (ret)
		return ret;

#ifdef CONFIG_UART_MULTI_CONSOLE
	console_debug("register uart console device.\r\n");
	extern int uart_multi_console_register(void);
	ret = uart_multi_console_register();
	if (ret)
		return ret;
#endif

	return 0;
}

int multiple_console_early_deinit(void)
{
	int ret;

#ifdef CONFIG_UART_MULTI_CONSOLE
	console_debug("unregister uart console device.\r\n");
	extern int uart_multi_console_unregister(void);
	ret = uart_multi_console_unregister();
	if (ret)
		return ret;
#endif

	console_debug("unregister null console device.\r\n");
	extern int null_multi_console_unregister(void);
	ret = null_multi_console_unregister();
	if (ret)
		return ret;

	extern int console_core_deinit(void);
	ret = console_core_deinit();
	if (ret)
		return ret;

	return 0;
}

int multiple_console_init(void)
{
#ifdef CONFIG_RPMSG_MULTI_CONSOLE
	int ret;
	console_debug("register rpmsg console device.\r\n");
	extern int rpmsg_multi_console_register(void);
	ret = rpmsg_multi_console_register();
	if (ret)
		return ret;
#endif

	return 0;
}

int multiple_console_deinit(void)
{
#ifdef CONFIG_RPMSG_MULTI_CONSOLE
	int ret;
	console_debug("unregister rpmsg console device.\r\n");
	extern int rpmsg_multi_console_unregister(void);
	ret = rpmsg_multi_console_unregister();
	if (ret)
		return ret;
#endif

	return 0;
}
