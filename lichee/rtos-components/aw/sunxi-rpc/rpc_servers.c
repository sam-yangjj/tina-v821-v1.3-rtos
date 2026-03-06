/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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

#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <console.h>
#include "sunxi-rpc-internal.h"

#define MAX_FUNC_CLASS		(CONFIG_COMPONENTS_AW_RPC_SERVER_MAX_CLASS)

struct func_class {
	const struct func_entry *fn;
	int max_fn;
};

static struct func_class g_class[MAX_FUNC_CLASS] = { NULL };

static int rpc_func_call(struct msg_header *header)
{
	int class_id = header->class_id;
	int func_id = header->func_id;
	int ret;

	if (class_id >= MAX_FUNC_CLASS || !g_class[class_id].fn) {
		rpc_err("invalid class_id: %d\n", class_id);
		return -ENOSYS;
	}

	if (func_id >= g_class[class_id].max_fn || !(g_class[class_id].fn[func_id].fn)) {
		rpc_err("invalid func_id: %d\n", func_id);
		return -ENOSYS;
	}

	ret = (g_class[class_id].fn[func_id].fn)(header);
	if (ret)
		return ret;

	return 0;
}

static void dump_header(struct msg_header *header)
{
	int i, j;
	u8 *p;

	rpc_info("MAGIC: 0x%x\n", header->magic);
	rpc_info("cookie: 0x%08x%08x\n", (int)(header->cookie >> 32), (int)(header->cookie & ~0UL));
	rpc_info("class_id: %d\n", header->class_id);
	rpc_info("func_id: %d\n", header->func_id);
	rpc_info("nr_args: %d\n", header->nr_args);
	rpc_info("args_total: %d\n", header->args_total);

	p = (u8 *)(&header->data[header->nr_args]);
	for (i = 0; i < header->nr_args; i++) {

		rpc_info("arg%d: size=%d\n", i, header->data[i]);
		for (j = 0; j < header->data[i]; j++)
			rpc_info("0x%02x ", p[j]);
		p += header->data[i];
	}
}

static int __rpc_servers_func_call(struct msg_header *header)
{
	int ret;
	unsigned long pa;

	pa = header->cookie;
	header->cookie = ~header->cookie_inverse;

	ret = rpc_func_call(header);
	if (ret) {
		rpc_err("func_call failed ret=%d...\n", ret);
		dump_header(header);
	}
	_srpc_send_ret(header, pa, !!ret);

	return ret;
}

int rpc_servers_func_call(struct msg_header *header, unsigned long pa, bool direct)
{
	header->cookie = pa;

	if (direct)
		return __rpc_servers_func_call(header);
	else
		return threadpool_add_task((void (*)(void *))__rpc_servers_func_call, header);
}

int sunxi_rpc_register_class(int class_id, int max_fn, const struct func_entry fn[])
{
	if (class_id > MAX_FUNC_CLASS) {
		rpc_err("class_id exceed scope, MAX:%d\n", MAX_FUNC_CLASS);
		return -EINVAL;
	}

	if (g_class[class_id].fn) {
		rpc_err("class_id %d already registered\n", class_id);
		return -EEXIST;
	}

	/* TODO: make a lock to protected it? */
	g_class[class_id].fn = fn;
	g_class[class_id].max_fn = max_fn;

	return 0;
}

int sunxi_rpc_unregister_class(int class_id, const struct func_entry fn[])
{
	if (class_id > MAX_FUNC_CLASS) {
		rpc_err("class_id exceed scope, MAX:%d\n", MAX_FUNC_CLASS);
		return -EINVAL;
	}

	if (g_class[class_id].fn != fn) {
		rpc_err("class_id %d invalid fn pointer, ignore...\n", class_id);
		return -EFAULT;
	}

	g_class[class_id].fn = NULL;
	g_class[class_id].max_fn = 0;

	return 0;
}
