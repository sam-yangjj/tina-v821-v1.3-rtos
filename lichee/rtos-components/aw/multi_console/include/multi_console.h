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
#ifndef AW_MULTI_CONSOLE_H
#define AW_MULTI_CONSOLE_H

#include <aw_list.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <hal_atomic.h>

#define CLI_CONSOLE_MAX_NAME_LEN					(32)
#define CLI_CONSOLE_MAX_INPUT_SIZE					(256)
#define CLI_CONSOLE_MAX_OUTPUT_SIZE					(configCOMMAND_INT_MAX_OUTPUT_SIZE)

struct _wraper_task;
struct _device_ops;
struct _cli_console;

typedef struct _wraper_task wraper_task;
typedef struct _device_ops cli_dev_ops;
typedef struct _cli_console cli_console;

void cli_console_set_task_console(void *current_console, void *task);
void cli_console_clear_task_console(cli_console *console, void *task);

int cli_console_read(cli_console *console, void *buf, size_t nbytes);
int cli_console_read_timeout(cli_console *console, void *buf, size_t nbytes, uint32_t timeout);
int cli_console_write(cli_console *console, const void *buf, size_t nbytes);

cli_console *get_current_console(void);
cli_console *set_current_console(cli_console *console);
cli_console *get_clitask_console(void);

cli_console *get_default_console(void);
cli_console *set_default_console(void *console);

cli_console *get_global_console(void);
cli_console *set_global_console(void *console);

cli_console *cli_console_create(cli_dev_ops *dev_ops, const char *name);
int cli_console_destory(cli_console * console);
void cli_console_current_task_destory(void);
char *cli_console_get_name(cli_console *console);

int cli_console_check_invalid(cli_console *console);
int cli_console_task_check_exit(void);
void cli_console_set_exit(cli_console *console);
void check_console_task_exit(void);

int multiple_console_init(void);

#endif
