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
#include "sunxi-rpc-internal.h"
#include "bitmap.h"

int sunxi_rpc_msg_mem_init(struct sunxi_rpc *rpc)
{
	struct msg_mempool *mempool = &rpc->mempool;
	int i;

	mempool->blks[0] = hal_malloc_align(MSG_MEMPBLK_SIZE * MSG_MEMPBLK_MAX, MSG_MEMPBLK_SIZE);
	if (!mempool->blks[0])
		return -ENOMEM;
	mempool->pa = (typeof(mempool->pa))(mempool->blks[0]);

	for (i = 1; i < MSG_MEMPBLK_MAX; i++)
		mempool->blks[i] = mempool->blks[0] + i * MSG_MEMPBLK_SIZE;

	mempool->bitmap = bitmap_alloc(MSG_MEMPBLK_MAX);
	if (!mempool->bitmap) {
		hal_free_align(mempool->blks[0]);
		return -ENOMEM;
	}

	hal_spin_lock_init(&mempool->lock);

	return 0;
}

void sunxi_rpc_msg_mem_exit(struct sunxi_rpc *rpc)
{
	struct msg_mempool *mempool = &rpc->mempool;

	bitmap_free(mempool->bitmap);
	hal_free_align(mempool->blks[0]);
	memset(mempool, 0, sizeof(*mempool));
	hal_spin_lock_deinit(&mempool->lock);
}

void *sunxi_rpc_msg_mem_alloc(struct sunxi_rpc *rpc, unsigned int size, unsigned long *pa)
{
	unsigned long flags;
	struct msg_mempool *mempool = &rpc->mempool;
	int start, nr;

	if (!mempool->bitmap)
		return NULL;

	nr = ALIGN_UP(size, MSG_MEMPBLK_SIZE) / MSG_MEMPBLK_SIZE;

	flags = hal_spin_lock_irqsave(&mempool->lock);

	start = bitmap_find_next_zero_area(mempool->bitmap, MSG_MEMPBLK_MAX, 0, nr, 0);
	__bitmap_set(mempool->bitmap, start, nr);

	hal_spin_unlock_irqrestore(&mempool->lock, flags);

	*pa = mempool->pa + start * MSG_MEMPBLK_SIZE;

	rpc_dbg("rpc alloc msg: va=%p, pa=0x%lx, size=0x%x\n", mempool->blks[start], *pa, size);

	return mempool->blks[start];
}

void sunxi_rpc_msg_mem_free(struct sunxi_rpc *rpc, void *ptr, unsigned int size)
{
	unsigned long flags;
	struct msg_mempool *mempool = &rpc->mempool;
	int start = -1, nr, i;

	WARN_ON(mempool->bitmap == NULL);

	if (!mempool->bitmap)
		return;

	rpc_dbg("rpc free msg: va=%p, size=0x%x\n", ptr, size);

	nr = ALIGN_UP(size, MSG_MEMPBLK_SIZE) / MSG_MEMPBLK_SIZE;

	for (i = 0; i < MSG_MEMPBLK_MAX; i++) {
		if (mempool->blks[i] == ptr) {
			start = i;
			break;
		}
	}

	WARN_ON(start < 0);

	flags = hal_spin_lock_irqsave(&mempool->lock);

	bitmap_clear(mempool->bitmap, start, nr);

	hal_spin_unlock_irqrestore(&mempool->lock, flags);
}
