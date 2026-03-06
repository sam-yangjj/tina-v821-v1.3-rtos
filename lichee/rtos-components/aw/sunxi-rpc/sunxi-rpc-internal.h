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
#ifndef SUNXI_RPC_INTERNAL_H
#define SUNXI_RPC_INTERNAL_H

#include <sunxi_hal_common.h>
#include <hal_msgbox.h>
#include <hal_time.h>
#include <hal_queue.h>
#include <hal_cache.h>
#include <hal_atomic.h>
#include <hal_mem.h>
#include <hal_waitqueue.h>
#include <aw_list.h>

#include <srpc_func.h>

#ifdef CONFIG_COMPONENTS_AW_RPC_CLIENT
struct msg_mempool {
	void *blks[MSG_MEMPBLK_MAX];
	unsigned long pa;
	unsigned long *bitmap;
	hal_spinlock_t lock;
};
#endif

struct sunxi_rpc {
	struct msg_endpoint ept;
	hal_queue_t recv_queue;
	void *task;
	int init;

#ifdef CONFIG_COMPONENTS_AW_RPC_CLIENT
	struct msg_mempool mempool;
#endif
};

#ifdef CONFIG_COMPONENTS_AW_RPC_DEBUG
#define rpc_dbg				printf
#else
#define rpc_dbg(...)		do { } while(0)
#endif
#define rpc_info			printf
#define rpc_err				printf

int sunxi_rpc_send_mbox_msg(struct sunxi_rpc *chip, u32 data);
void sunxi_rpc_set_mbox_rx_cb(struct sunxi_rpc *chip, void (*rx_callback)(u32 data));

int sunxi_rpc_msg_mem_init(struct sunxi_rpc *rpc);
void sunxi_rpc_msg_mem_exit(struct sunxi_rpc *rpc);
void *sunxi_rpc_msg_mem_alloc(struct sunxi_rpc *rpc, unsigned int size, unsigned long *pa);
void sunxi_rpc_msg_mem_free(struct sunxi_rpc *rpc, void *ptr, unsigned int size);

int sunxi_rpc_msg_core_init(struct sunxi_rpc *rpc);
void sunxi_rpc_msg_core_exit(struct sunxi_rpc *rpc);

int threadpool_init(void);
void threadpool_exit(void);
int threadpool_add_task(void (*func)(void *), void *data);

int rpc_servers_func_call(struct msg_header *header, unsigned long pa, bool direct);
int _srpc_send_ret(struct msg_header *header, unsigned long pa, uint32_t err);

#endif
