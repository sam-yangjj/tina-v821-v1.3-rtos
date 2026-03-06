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
#ifndef _RPMSG_CONSOLE_H_
#define _RPMSG_CONSOLE_H_

#include <hal_queue.h>
#include <hal_thread.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/rpmsg_master.h>

#include "../multi_console_internal.h"

#include <hal_sem.h>

struct rpmsg_message {
	unsigned int command;
	unsigned int data_length;
};

struct rpmsg_packet {
	struct rpmsg_message msg;
	char data[512 - 16 - sizeof(struct rpmsg_message)];
};

struct rpmsg_shell {
	struct rpmsg_endpoint *ept;
	cli_dev_ops raw;
	hal_mailbox_t mb_rx;
	cli_console *console;
	char name[32];
#ifdef CONFIG_RPMSG_CONSOLE_CACHE
	uint32_t cache_len;
	char cmd_cache[CLI_CONSOLE_MAX_INPUT_SIZE];
#endif
};

struct rpmsg_service {
	struct hal_sem ready_sem;
	struct rpmsg_shell *shell;
};

struct rpmsg_shell *
rpmsg_console_create(struct rpmsg_endpoint *ept, uint32_t id);
void rpmsg_console_delete(struct rpmsg_shell *shell);

#define rpmsg_err(fmt, args...) \
    printf("[RPMSG_ERR][%s:%d]" fmt, __FUNCTION__, __LINE__, ##args)

#ifdef RPMSG_DEBUG
#define rpmsg_debug(fmt, args...) \
    printf("[RPMSG_DBG][%s:%d]" fmt, __FUNCTION__, __LINE__, ##args)
#else
#define rpmsg_debug(fmt, args...)
#endif

#endif
