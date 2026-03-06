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
#include <cli_console.h>
#include <hal_uart.h>
#include <stdint.h>
#include <hal_atomic.h>

#ifdef CONFIG_ARCH_SUN8IW20
#define CONFIG_IRQ_CLI_USE_SPINLOCK
#endif

int uart_console_write_no_lock(const void *buffer, size_t len, void *privata_data)
{
	const uint8_t *buf = buffer;
#ifndef CONFIG_DISABLE_ALL_UART_LOG
	hal_uart_send(CONFIG_CLI_UART_PORT, buf, len);
#endif
    return 0;
}

#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
static freert_spinlock_t uart_twlock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
static pthread_mutex_t uart_tw_mutex = FREERTOS_POSIX_MUTEX_INITIALIZER;
#endif

int uart_console_write(const void *buffer, size_t len, void *privata_data)
{
	int ret;

	extern int hal_thread_is_in_critical_context(void);
#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
	hal_spin_lock(&uart_twlock);
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
    if (!hal_thread_is_in_critical_context())
        pthread_mutex_lock(&uart_tw_mutex);
#endif

	ret = uart_console_write_no_lock(buffer, len, privata_data);

#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
	hal_spin_unlock(&uart_twlock);
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
    if (!hal_thread_is_in_critical_context())
	    pthread_mutex_unlock(&uart_tw_mutex);
#endif

    return ret;
}

#if defined(CONFIG_IRQ_CLI_USE_SPINLOCK)
static freert_spinlock_t uart_iwlock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};
#endif

int uart_console_write_in_irq(const void *buffer, size_t len, void *privata_data)
{
	int ret;

#if defined(CONFIG_IRQ_CLI_USE_SPINLOCK)
	hal_spin_lock(&uart_iwlock);
#endif

	ret = uart_console_write_no_lock(buffer, len, privata_data);

#if defined(CONFIG_IRQ_CLI_USE_SPINLOCK)
	hal_spin_unlock(&uart_iwlock);
#endif
    return ret;
}

int uart_console_read(void *buf, size_t len, void *privata_data)
{
#ifndef CONFIG_DISABLE_ALL_UART_LOG
    return hal_uart_receive(CONFIG_CLI_UART_PORT, (uint8_t *)buf, len);
#else
    return 0;
#endif
}

static int uart_console_init(void *private_data)
{
#ifndef CONFIG_DISABLE_ALL_UART_LOG
    int ret = hal_uart_init(CONFIG_CLI_UART_PORT);
    if (ret) {
        return 0;
    }
#endif
    return 1;
}

static int uart_console_deinit(void *private_data)
{
    return 1;
}

static device_console uart_console =
{
    .name = "uart-console",
    .write = uart_console_write,
    .read = uart_console_read,
    .init = uart_console_init,
    .deinit = uart_console_deinit,
};

cli_console cli_uart_console =
{
    .name = "cli-uart-console",
    .dev_console = &uart_console,
    .init_flag = 0,
    .exit_flag = 0,
    .alive = 1,
    .private_data = NULL,
};
