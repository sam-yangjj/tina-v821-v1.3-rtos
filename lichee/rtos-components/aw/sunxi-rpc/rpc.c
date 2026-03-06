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
#include <compiler.h>

#include <console.h>
#include "sunxi-rpc-internal.h"

#define MSGBOX_E907    0   /* correspond to msgbox driver MSGBOX_RISCV */
#define MSGBOX_A27L2   1   /* correspond to msgbox driver MSGBOX_ARM */

#define RPC_REMOVE_ID  MSGBOX_A27L2

#ifndef printfFromISR
#define printfFromISR printf
#endif

#define RECEIVE_QUEUE_LENGTH 16
#define RECEIVE_QUEUE_WAIT_MS 100

#define RPC_MSG_RECV_CHANNEL CONFIG_COMPONENTS_AW_RPC_CHANNEL
#define RPC_MSG_SEND_CHANNEL CONFIG_COMPONENTS_AW_RPC_CHANNEL

#ifndef BIT
#define BIT(x)                          (1 << (x))
#endif

#define MSG_DATA_ADDR_OFS               (10)
#define MSG_DATA_FIELD_ALIGN            (BIT(MSG_DATA_ADDR_OFS))

#define MSG_TYPE_MAST                   (BIT(2) - 1)
#define MSG_TYPE_FUNC                   (0)
#define MSG_TYPE_EXT_ADDR               (1)

#define MSG_FLAGS_MASK                  ((BIT(5) - 1) & ~(BIT(2) - 1))

#define MSG_ATOMIC_FLAGS_OFFSET         BIT(2)
#define MSG_ERR_FLAGS_OFFSET           BIT(3)
#define MSG_DEBUG_FLAGS_OFFSET          BIT(4)
#define MSG_RET_FLAGS_OFFSET            BIT(5)

#ifdef HOST_64BIT
#define MSG_HEADRER_MAGIC               (0xffee6464)
#else
#define MSG_HEADRER_MAGIC               (0xffee3232)
#endif

typedef struct msg_data {
	union {
		uint32_t raw;
		struct {
			uint32_t type    : 2;
			uint32_t atomic  : 1;
			uint32_t err     : 1;
			uint32_t debug   : 1;
			uint32_t ret     : 1;
			uint32_t reserved : 4; /* must 0 */
			uint32_t pa_addr : 22;
		} d;
	} u;
} __packed msg_data_t;

static struct sunxi_rpc g_srpc;

#ifdef CONFIG_COMPONENTS_AW_RPC_DEBUG
static void print_header_hex(struct msg_header *header, const char *prefix)
{
	int size, i = 0;
	uint32_t *p = (void *)header;

	size = sizeof(*header) + header->args_total + header->nr_args * sizeof(uint32_t);
	if (size > MSG_MEMPBLK_SIZE)
		size = MSG_MEMPBLK_SIZE;

	size /= 4;
	while (size) {
		if (size >= 4) {
			printf("%s: 0x%08x 0x%08x 0x%08x 0x%08x\n", prefix, p[i], p[i + 1], p[i + 2], p[i + 3]);
			i += 4;
			size -= 4;
		} else if (size >= 3) {
			printf("%s: 0x%08x 0x%08x 0x%08x\n", prefix, p[i], p[i + 1], p[i + 2]);
			i += 3;
			size -= 3;
		} else if (size >= 2) {
			printf("%s: 0x%08x 0x%08x\n", prefix, p[i], p[i + 1]);
			i += 2;
			size -= 2;
		} else {
			printf("%s: 0x%08x\n", prefix, p[i]);
			i++;
			size--;
		}
	}
}
#else
static void print_header_hex(struct msg_header *header, const char *prefix) { }
#endif /* CONFIG_COMPONENTS_AW_RPC_DEBUG */

static int recv_from_cpu_callback(uint32_t data, void *priv)
{
	unsigned long pa;
	struct msg_header *header = NULL;
	struct srpc_cookie __maybe_unused *cookie;
	struct msg_data *msg_d = (struct msg_data *)&data;

	rpc_dbg("rx msg: 0x%08x\n", data);

	if (msg_d->u.d.reserved != 0) {
		rpc_err("data corrupt: 0x%x\n", data);
		return 0;
	}

	pa = msg_d->u.d.pa_addr << MSG_DATA_ADDR_OFS;

	if (unlikely(msg_d->u.d.type != MSG_TYPE_FUNC)) {
		rpc_err("unsupport data type: %d\n", msg_d->u.d.type);
		goto err_out;
	}

	if (likely((msg_d->u.d.ret))) { /* function return */
#ifdef CONFIG_COMPONENTS_AW_RPC_CLIENT
		header = local_pa2va(pa);

		hal_dcache_invalidate((unsigned long)header, sizeof(*header));
		hal_dcache_invalidate((unsigned long)header, sizeof(*header) + header->args_total + header->nr_args * sizeof(uint32_t));

		if (header->magic != MSG_HEADRER_MAGIC || header->cookie != ~header->cookie_inverse) {
			rpc_err("data header corrupt: 0x%p, msg: 0x%x\n", header, msg_d->u.raw);
			goto err_out;
		}

		cookie = (void *)(unsigned long)header->cookie;

		if (unlikely((cookie->msg & (~(MSG_DATA_FIELD_ALIGN - 1))) != pa)) {
			rpc_err("data addr corrupt: receive 0x%lx, record: 0x%lx\n", pa,
							(long)(cookie->msg & (~(MSG_DATA_FIELD_ALIGN - 1))));
			goto err_out;
		}

		if (msg_d->u.d.err)
			cookie->error = header->ret_code;
		else
			cookie->error = 0;
		rpc_dbg("wake up msg waiter [%p]: :%p\n", header, cookie->self);
		cookie->done = 1;
		hal_wake_up(&cookie->wq);
#else
		rpc_err("unsupport func ret from remote: 0x%x\n", data);
		goto err_out;
#endif
	} else { /* func call */
#ifdef CONFIG_COMPONENTS_AW_RPC_SERVER
		header = remote_pa2va(pa);
		hal_dcache_invalidate((unsigned long)header, sizeof(*header));
		hal_dcache_invalidate((unsigned long)header, sizeof(*header) + header->args_total + header->nr_args * sizeof(uint32_t));

		if (header->magic != MSG_HEADRER_MAGIC || header->cookie != ~header->cookie_inverse) {
			rpc_err("data header corrupt: 0x%p, msg: 0x%x\n", header, msg_d->u.raw);
			goto err_out;
		}

		rpc_dbg("func call from remote: 0x%x, header: %p!\n", data, header);
		if (rpc_servers_func_call(header, pa, msg_d->u.d.atomic))
			rpc_err("func call from remote: 0x%x failed!\n", data);
#else
		rpc_err("unsupport func call from remote: 0x%x\n", data);
		goto err_out;
#endif
	}

	return 0;

err_out:
	if (header)
		print_header_hex(header, "rpc call ");

	return 0;
}

int _srpc_send_ret(struct msg_header *header, unsigned long pa, uint32_t err)
{
	struct msg_data msg = {0};

	/* assume header is in the line mapping addrspace */
#if BITS_PER_LONG == 64
	// TODO: current assume pa is < 4G
#endif
	msg.u.d.pa_addr = pa >> MSG_DATA_ADDR_OFS;
	header->ret_code = err;

	/* default is func type */

	if (err)
		msg.u.d.err = 1;

	msg.u.d.reserved = 0;
	msg.u.d.ret = 1;

	rpc_dbg("send ret msg [0x%x]: header:%p\n", msg.u.raw, header);

	print_header_hex(header, "rpc ret ");

	hal_dcache_clean((unsigned long)header, sizeof(*header));
	hal_msgbox_channel_send_data(&g_srpc.ept, msg.u.raw);

	return 0;
}

#ifdef CONFIG_COMPONENTS_AW_RPC_CLIENT
struct msg_header *_srpc_alloc_header(unsigned int size, struct srpc_cookie *cookie)
{
	struct msg_header *header;

	header = sunxi_rpc_msg_mem_alloc(&g_srpc, size, &cookie->pa);
	if (!header)
		return NULL;

	cookie->header = header;

	header->magic = MSG_HEADRER_MAGIC;
	header->cookie = (uint64_t)(unsigned long)cookie;
	header->cookie_inverse = ~(header->cookie);

	hal_waitqueue_head_init(&cookie->wq);

	rpc_dbg("prep msg header [%p]: phys:0x%lx, cookie:%p\n", header, cookie->pa , cookie);

	return header;

}
void _srpc_header_init(struct msg_header *header, int class_id,
                int func_id, int nr_args, uint32_t arg_size[])
{
	header->class_id = class_id;
	header->func_id = func_id;
	header->nr_args = nr_args;

	memcpy(header->data, arg_size, nr_args * sizeof(uint32_t));

	rpc_dbg("init msg header [%p]: class_id:%d, func_id:%d, nr_args:%d\n", header, class_id, func_id, nr_args);
}

void _srpc_free_header(struct msg_header *header, unsigned int size)
{
	struct srpc_cookie *cookie = (struct srpc_cookie *)(long)header->cookie;

	hal_waitqueue_head_deinit(&cookie->wq);
	memset(header, 0, sizeof(header) + header->args_total + header->nr_args * sizeof(uint32_t));
	sunxi_rpc_msg_mem_free(&g_srpc, header, size);
}


int _srpc_call(struct msg_header *header, bool atomic)
{
	struct msg_data msg = { 0 };
	struct srpc_cookie *cookie = (struct srpc_cookie *)(long)header->cookie;

#if BITS_PER_LONG == 64
	// TODO: current assume pa is < 4G
#endif
	msg.u.d.pa_addr = (unsigned long)cookie->pa >> MSG_DATA_ADDR_OFS;

	/* default is func type */
	/* pa |= MSG_TYPE_FUNC; */

	if (atomic)
		msg.u.d.atomic = 1;

	msg.u.d.reserved = 0;

	rpc_dbg("send msg [0x%x]: header:%p, pa:0x%lx, atomic:%d\n", msg.u.raw, header, cookie->pa, atomic);

	print_header_hex(header, "rpc call ");

	cookie->self = hal_thread_self();
	cookie->msg = msg.u.raw;
	cookie->error = -ENOSYS;
	cookie->done = 0;

	hal_dcache_clean((unsigned long)header, sizeof(*header) + header->args_total + header->nr_args * sizeof(uint32_t));
	hal_msgbox_channel_send_data(&g_srpc.ept, msg.u.raw);

	hal_wait_event(cookie->wq, cookie->done);

	if (cookie->error)
		rpc_err("srpc call func %d error: %d\n", header->func_id, cookie->error);

	return cookie->error;
}
#endif /* CONFIG_COMPONENTS_AW_RPC_CLIENT */

int sunxi_rpc_init(void)
{
	int ret = 0;
	struct sunxi_rpc *rpc = &g_srpc;
	struct msg_endpoint *medp = &rpc->ept;

	memset(rpc, 0, sizeof(g_srpc));

#ifdef CONFIG_COMPONENTS_AW_RPC_CLIENT
	sunxi_rpc_msg_mem_init(rpc);
#endif
#ifdef CONFIG_COMPONENTS_AW_RPC_SERVER
	threadpool_init();
#endif

	medp->private = rpc;
	medp->rec = (void *)recv_from_cpu_callback;
	ret = hal_msgbox_alloc_channel(medp, RPC_REMOVE_ID, RPC_MSG_RECV_CHANNEL, RPC_MSG_SEND_CHANNEL);
	if(ret != 0) {
		printf("Failed to allocate msgbox channel\n");
		goto out;
	}

#if defined(CONFIG_COMPONENTS_AW_RPC_SERVER) && defined(CONFIG_COMPONENTS_AW_RPC_TEST_CMD)
	extern void srpc_server_test_init(void);
	srpc_server_test_init();
#endif

out:
	return ret;
}

void sunxi_rpc_exit(void)
{
	struct sunxi_rpc *rpc = &g_srpc;
	struct msg_endpoint *medp = &rpc->ept;

#if defined(CONFIG_COMPONENTS_AW_RPC_SERVER) && defined(CONFIG_COMPONENTS_AW_RPC_TEST_CMD)
	extern void srpc_server_test_exit(void);
	srpc_server_test_exit();
#endif

#ifdef CONFIG_COMPONENTS_AW_RPC_SERVER
	threadpool_exit();
#endif

#ifdef CONFIG_COMPONENTS_AW_RPC_CLIENT
	sunxi_rpc_msg_mem_exit(rpc);
#endif

	hal_msgbox_free_channel(medp);
	memset(&rpc, 0, sizeof(g_srpc));
}
