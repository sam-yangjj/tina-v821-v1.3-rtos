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
#ifndef AW_MULTI_CONSOLE_INTERNEL_H
#define AW_MULTI_CONSOLE_INTERNEL_H

#include <aw_list.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <hal_atomic.h>
#include "include/multi_console.h"

#define CLI_CONSOLE_DEFAULT_STACK_SIZE				(2 * 1024 + (((configCOMMAND_INT_MAX_OUTPUT_SIZE) + (1023)) & ~(1023)))
#define CLI_CONSOLE_DEFAULT_PRIORITY				(15)

typedef unsigned long cpu_cpsr_t;

typedef void (*console_thread_t)(void * private_data);

struct _wraper_task {
	struct list_head node;
	void *task;
};

struct _device_ops {
	int (*read)(void *buf, size_t len, void *priv, uint32_t timeout);
	int (*write)(const void *buf, size_t len, void *priv);
	int (*terminate_read)(void *priv);
	/* return 0 if init success */
	int (*init)(void *priv);
	int (*deinit)(void *priv);
	void *priv;
	const char *prefix;
	unsigned int echo   :1; /* = 0, will not echo char back */
	unsigned int task   :1;  /* = 1, will not create shell thread */
};

struct _cli_console {
	char name[CLI_CONSOLE_MAX_NAME_LEN];
	struct list_head i_list; /* mount on gCliConsolelist */
	cli_dev_ops *dev_ops;

	unsigned int init :1; /* = 1, init ok */
	unsigned int exit :1; /* = 1, console will exit */
	unsigned int alive:1; /* = 1, console is running */
	unsigned int ready:1; /* 1: console create complete */
	void *shell_task;

	hal_spinlock_t lock;
	struct list_head task_list;
};

#ifdef CONFIG_MULTI_CONSOLE_DEBUG

#define CONSOLE_LOG_COLOR_NONE		"\e[0m"
#define CONSOLE_LOG_COLOR_RED		"\e[31m"
#define CONSOLE_LOG_COLOR_GREEN		"\e[32m"
#define CONSOLE_LOG_COLOR_YELLOW	"\e[33m"
#define CONSOLE_LOG_COLOR_BLUE		"\e[34m"

int console_printk(const char *fmt, ...);
#define console_debug(fmt, args...) \
	console_printk(CONSOLE_LOG_COLOR_GREEN "[CONSOLE_DBG][%s:%d]" \
		CONSOLE_LOG_COLOR_NONE fmt, __func__, __LINE__, ##args)
#else
#define console_debug(fmt, args...)            do { } while(0)
#endif

#define HEXDUMP(ptr, size) \
do { \
	int i, j; \
	char *p = (char *)ptr; \
	console_printk("\r\n"); \
	for (i = 0; i < size; i++) { \
		if ((i % 16) == 0) \
			console_printk("0x%08x:", (uint32_t)(p + i)); \
		console_printk("%02x ", *(p + i)); \
		if ((i + 1) % 16 == 0) { \
			console_printk("\t|"); \
			for (j = 0; j < 16; j++) { \
				if (*(p + i + 1 - 16 + j) >= ' ' && *(p + i + 1 - 16 + j) < '~') \
					console_printk("%c", *(p + i + 1 - 16 + j)); \
				else \
					console_printk("."); \
			} \
			console_printk("|\r\n"); \
		} \
	} \
	console_printk("\r\n"); \
} while (0)

#define HEXDUMP32(ptr, size) \
do { \
	int i, j; \
	char *p = (char *)ptr; \
	console_printk("\r\n"); \
	for (i = 0; i < size; i += 4) { \
		if ((i % 16) == 0) \
			console_printk("0x%08" PRIx32 ":", (uint32_t)(p + i)); \
		console_printk("%08" PRIx32 " ", *((uint32_t *)(p + i))); \
		if ((i + 4) % 16 == 0) { \
			console_printk("\t|"); \
			for (j = 0; j < 16; j++) { \
				if (*(p + i + 4 - 16 + j) >= ' ' && *(p + i + 4 - 16 + j) < '~') \
					console_printk("%c", *(p + i + 4 - 16 + j)); \
				else \
					console_printk("."); \
			} \
			console_printk("|\r\n"); \
		} \
	} \
	console_printk("\r\n"); \
} while (0)

#endif
