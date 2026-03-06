/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <hal_reset.h>
#include <hal_cache.h>
#include <hal_mem.h>
#include <hal_atomic.h>
#include <hal_clk.h>
#include <hal_interrupt.h>
#include <interrupt.h>
#include <sunxi_hal_common.h>
#include <hal_dma.h>
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#include <pm_syscore.h>
#include <pm_debug.h>
#endif

#ifdef DMA_PKGNUM
#define DMA_PKG_NUM_MAX	0xFFFFFFFF
#endif
#define DMA_ERR(fmt, arg...) printf("%s()%d " fmt, __func__, __LINE__, ##arg)

#ifdef DMA_START_CHAN
#define SUNXI_DMA_CHAN_START DMA_START_CHAN
#else
#define SUNXI_DMA_CHAN_START 0
#endif
#define SET_DESC_HIGH_ADDR(x) ((((u64)x >> 32) & 0x3UL) | (x & 0xFFFFFFFC))

static struct sunxi_dma_chan    dma_chan_source[NR_MAX_CHAN];
static hal_spinlock_t dma_lock;
/* Avoid repeated requests irq, which may led to unkown problem such as memory leak */
static uint32_t irq_inited = 0;

static inline void hal_dma_irq_set(uint32_t chan, bool enable)
{
	if (!enable) {
    	uint32_t high = 0;
       	high = (chan >= HIGH_CHAN) ? 1 : 0;
        /*disable all dma irq*/
       	hal_writel(0, DMA_IRQ_EN(high));
	}

}

static inline uint32_t hal_dma_irq_status(uint32_t chan_num, bool enable)
{
    uint32_t status = 0;
	if (enable) {
    	uint32_t high = 0;
       	high = (chan_num >= HIGH_CHAN) ? 1 : 0;
        /*clear all dma irq pending*/
        hal_writel(0xffffffff, DMA_IRQ_STAT(high));
	} else {
    	uint32_t status_l = 0, status_h = 0;
#if START_CHAN_OFFSET < HIGH_CHAN
    	status_l = hal_readl(DMA_IRQ_STAT(0));
#endif
#if NR_MAX_CHAN + START_CHAN_OFFSET > HIGH_CHAN
    	status_h = hal_readl(DMA_IRQ_STAT(1));
#endif
#if START_CHAN_OFFSET < HIGH_CHAN
    	hal_writel(status_l, DMA_IRQ_STAT(0));
#endif
#if NR_MAX_CHAN + START_CHAN_OFFSET > HIGH_CHAN
    	hal_writel(status_h, DMA_IRQ_STAT(1));
#endif
		status = (chan_num + START_CHAN_OFFSET >= HIGH_CHAN) \
                	 ? (status_h >> ((chan_num + START_CHAN_OFFSET - HIGH_CHAN) << DMA_IRQ_STAT_OFFSET)) \
                 	: (status_l >> ((chan_num + START_CHAN_OFFSET) << DMA_IRQ_STAT_OFFSET));
		return status;
	}
	return 0;

}

static inline void sunxi_cfg_lli(struct sunxi_dma_lli *lli, uint32_t src_addr,
                            uint32_t dst_addr, uint32_t len,
                            struct dma_slave_config *config, enum dma_transfer_direction dir)
{
    if (NULL == lli && NULL == config)
    {
        return;
    }

    if(convert_buswidth(&config->src_addr_width))
		DMA_ERR("[dma] src addr width is over, use default val\n");

    if(convert_buswidth(&config->dst_addr_width))
		DMA_ERR("[dma] dst addr width is over, use default val\n");

    lli->cfg = SRC_CONTI(config->conti_mode)   | \
			   SRC_BURST(config->src_maxburst) | \
               SRC_WIDTH(config->src_addr_width) | \
               DST_BURST(config->dst_maxburst) | \
               DST_WIDTH(config->dst_addr_width) | \
			   DMA_BCMODE(config->bc_mode);

    if (dir == DMA_MEM_TO_MEM) {
    	lli->src = src_addr;
    	lli->dst = dst_addr;
    } else if (dir == DMA_MEM_TO_DEV) {
    	lli->src = src_addr;
    	lli->dst = dst_addr;
    } else if (dir == DMA_DEV_TO_MEM) {
    	lli->src = src_addr;
    	lli->dst = dst_addr;
    } else {
    	lli->src = src_addr;
    	lli->dst = dst_addr;
    }
    lli->len = len;
    lli->para = NORMAL_WAIT;
}

static void sunxi_dump_lli(struct sunxi_dma_chan *chan, struct sunxi_dma_lli *lli)
{
#ifdef DMA_DEBUG
    printf("channum:%x\n"
           "\t\tdesc:desc - 0x%08x desc p - 0x%08x desc v - 0x%08x\n"
           "\t\tlli: v- 0x%08x v_lln - 0x%08x s - 0x%08x d - 0x%08x c - 0x%08x\n"
           "\t\tdist - 0x%08x len - 0x%08x para - 0x%08x p_lln - 0x%08x\n",
           chan->chan_count,
	   (uint32_t)chan->desc, (uint32_t)chan->desc->p_lln, (uint32_t)chan->desc->vlln,
	   (uint32_t)lli, (uint32_t)lli->vlln, (uint32_t)lli->src, (uint32_t)lli->dst, (uint32_t)lli->cfg,
           (uint32_t)lli->dst, (uint32_t)lli->len, (uint32_t)lli->para, (uint32_t)lli->p_lln);
#endif
}

static void *sunxi_lli_list(struct sunxi_dma_lli *prev, struct sunxi_dma_lli *next,
                        struct sunxi_dma_chan *chan)
{
    uint32_t temp_desc;

    if ((!prev && !chan) || !next)
    {
        return NULL;
    }

    temp_desc = __va_to_pa((unsigned long)next);
    if (!prev)
    {
        chan->desc = next;
        chan->desc->p_lln = temp_desc;
        chan->desc->vlln = next;
    }
    else
    {
        prev->p_lln = temp_desc;
        prev->vlln = next;
    }

    next->p_lln = LINK_END;
    next->vlln = NULL;

    return next;
}

static hal_irqreturn_t sunxi_dma_irq_handle(void *ptr)
{
    int i = 0;
    uint32_t status_l = 0, status_h = 0;
#if START_CHAN_OFFSET < HIGH_CHAN
    status_l = hal_readl(DMA_IRQ_STAT(0));
#endif
#if NR_MAX_CHAN + START_CHAN_OFFSET > HIGH_CHAN
    status_h = hal_readl(DMA_IRQ_STAT(1));
#endif
#if START_CHAN_OFFSET < HIGH_CHAN
    hal_writel(status_l, DMA_IRQ_STAT(0));
#endif
#if NR_MAX_CHAN + START_CHAN_OFFSET > HIGH_CHAN
    hal_writel(status_h, DMA_IRQ_STAT(1));
#endif

    for (i = SUNXI_DMA_CHAN_START; i < NR_MAX_CHAN; i++)
    {
        unsigned long __cpsr;
        struct sunxi_dma_chan *chan = &dma_chan_source[i];

        uint32_t chan_num = chan->chan_count;
        uint32_t status = 0;

        if (chan->used == 0)
        {
            continue;
        }

        __cpsr = hal_spin_lock_irqsave(&chan->lock);

        status = (chan_num + START_CHAN_OFFSET >= HIGH_CHAN) \
                 ? (status_h >> ((chan_num + START_CHAN_OFFSET - HIGH_CHAN) << DMA_IRQ_STAT_OFFSET)) \
                 : (status_l >> ((chan_num + START_CHAN_OFFSET) << DMA_IRQ_STAT_OFFSET));
        if (!(chan->irq_type & status))
        {
            hal_spin_unlock_irqrestore(&chan->lock, __cpsr);
            continue;
        }

        if (chan->cyclic)
        {
            dma_callback cb = NULL;
            void *cb_data = NULL;

            chan->periods_pos ++;
            if (chan->periods_pos * chan->desc->len >= chan->buf_len)
            {
                chan->periods_pos = 0;
            }
            cb = chan->callback;
            cb_data = chan->callback_param;

            hal_spin_unlock_irqrestore(&chan->lock, __cpsr);
            if (cb)
            {
                cb(cb_data);
            }
        }
        else
        {
            dma_callback cb = NULL;
            void *cb_data = NULL;

            cb = chan->callback;
            cb_data = chan->callback_param;

            hal_spin_unlock_irqrestore(&chan->lock, __cpsr);
            if (cb)
            {
                cb(cb_data);
            }
        }
    }
    return 0;
}

static int sunxi_dma_clk_init(bool enable)
{
    hal_clk_status_t ret;
    u32  reset_id;
    hal_clk_id_t clk_id, mbus_clk_id, mcu_clk_id;
    hal_clk_t clk;
    struct reset_control *reset;

    clk_id = SUNXI_CLK_DMA;
    reset_id = SUNXI_RST_DMA;
    mbus_clk_id = SUNXI_CLK_MBUS_DMA;
    hal_reset_type_t reset_type = HAL_SUNXI_RESET;
    hal_clk_type_t clk_type = HAL_SUNXI_CCU;
    mcu_clk_id = 0;

    DMA_ERR("enter dma clk init\n");
    if (enable)
    {
	if (reset_id) {
		reset = hal_reset_control_get(reset_type, reset_id);
		hal_reset_control_deassert(reset);
		hal_reset_control_put(reset);
	}
	if (mbus_clk_id)
		hal_clock_enable(hal_clock_get(clk_type, SUNXI_CLK_MBUS_DMA));

	clk = hal_clock_get(clk_type, clk_id);
	ret = hal_clock_enable(clk);
	if (ret != HAL_CLK_STATUS_OK)
	    DMA_ERR("DMA clock enable failed.\n");
	if (mcu_clk_id)
	    hal_clock_enable(hal_clock_get(clk_type, mcu_clk_id));
    }
    else
    {
	clk = hal_clock_get(clk_type, clk_id);
	ret = hal_clock_disable(clk);
	if (ret != HAL_CLK_STATUS_OK)
	    DMA_ERR("DMA clock disable failed.\n");
	if (mbus_clk_id)
		hal_clock_disable(hal_clock_get(clk_type, SUNXI_CLK_MBUS_DMA));
	hal_clock_put(clk);
	if (reset_id) {
		reset = hal_reset_control_get(reset_type, reset_id);
		hal_reset_control_assert(reset);
		hal_reset_control_put(reset);
	}
    }
    return ret;
}

void sunxi_dma_free_ill(struct sunxi_dma_chan *chan)
{
    struct sunxi_dma_lli *li_adr = NULL, *next = NULL;
    unsigned long __cpsr;

    if (NULL == chan)
    {
        DMA_ERR("[dma] chan is NULL\n");
        return ;
    }

    __cpsr = hal_spin_lock_irqsave(&chan->lock);
    li_adr = chan->desc;
    chan->desc = NULL;
    chan->callback = NULL;
    chan->callback_param = NULL;
    hal_spin_unlock_irqrestore(&chan->lock, __cpsr);

    while (li_adr)
    {
        next = li_adr->vlln;
        hal_free_coherent(li_adr);
        li_adr = next;
    }
}

hal_dma_chan_status_t hal_dma_chan_request(struct sunxi_dma_chan **dma_chan)
{
    int i = 0;
    struct sunxi_dma_chan *chan;
    unsigned long __cpsr;

    __cpsr = hal_spin_lock_irqsave(&dma_lock);
    for (i = SUNXI_DMA_CHAN_START; i < NR_MAX_CHAN; i++)
    {
        chan = &dma_chan_source[i];
        if (chan->used == 0)
        {
            chan->used = 1;
            chan->chan_count = i;
            hal_spin_unlock_irqrestore(&dma_lock, __cpsr);
            *dma_chan = &dma_chan_source[i];
            return HAL_DMA_CHAN_STATUS_FREE;
        }
    }
    hal_spin_unlock_irqrestore(&dma_lock, __cpsr);

    return HAL_DMA_CHAN_STATUS_BUSY;
}

hal_dma_status_t hal_dma_prep_memcpy(struct sunxi_dma_chan *chan,
				       uint32_t dest, uint32_t src, uint32_t len)
{
    struct sunxi_dma_lli *l_item = NULL;
    struct dma_slave_config *config = NULL;
    unsigned long __cpsr;

    if ((NULL == chan) || (dest == 0 || src == 0))
    {
        DMA_ERR("[dma] chan is NULL\n");
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    hal_assert(!hal_interrupt_is_disable());
    hal_assert(!in_interrupt());

    l_item = (struct sunxi_dma_lli *)hal_malloc_coherent(sizeof(struct sunxi_dma_lli));
    if (!l_item)
        return HAL_DMA_STATUS_NO_MEM;
    memset(l_item, 0, sizeof(struct sunxi_dma_lli));

    __cpsr = hal_spin_lock_irqsave(&chan->lock);

    config = &chan->cfg;
    dest = __va_to_pa(dest);
    src = __va_to_pa(src);
    sunxi_cfg_lli(l_item, src, dest, len, config, DMA_MEM_TO_MEM);

    l_item->cfg |= SRC_DRQ(DRQSRC_SDRAM) \
                   | DST_DRQ(DRQDST_SDRAM) \
                   | DST_INCREMENT_MODE \
                   | SRC_INCREMENT_MODE;
    sunxi_lli_list(NULL, l_item, chan);
    sunxi_dump_lli(chan, l_item);

    hal_spin_unlock_irqrestore(&chan->lock, __cpsr);

    return HAL_DMA_STATUS_OK;
}

hal_dma_status_t hal_dma_prep_device(struct sunxi_dma_chan *chan,
				       uint32_t dest, uint32_t src,
				       uint32_t len, enum dma_transfer_direction dir)
{
    struct sunxi_dma_lli *l_item = NULL;
    struct dma_slave_config *config = NULL;
    unsigned long __cpsr;

    if ((NULL == chan) || (dest == 0 || src == 0))
    {
        DMA_ERR("[dma] chan is NULL\n");
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    hal_assert(!hal_interrupt_is_disable());
    hal_assert(!in_interrupt());

    l_item = (struct sunxi_dma_lli *)hal_malloc_coherent(sizeof(struct sunxi_dma_lli));
    if (!l_item)
        return HAL_DMA_STATUS_NO_MEM;
    memset(l_item, 0, sizeof(struct sunxi_dma_lli));

    __cpsr = hal_spin_lock_irqsave(&chan->lock);

    config = &chan->cfg;

    if (dir == DMA_MEM_TO_DEV)
    {
        src = __va_to_pa(src);
        sunxi_cfg_lli(l_item, src, dest, len, config, dir);
        l_item->cfg |= GET_DST_DRQ(config->slave_id) \
                       | SRC_INCREMENT_MODE \
                       | DST_NOCHANGE_MODE \
                       | SRC_DRQ(DRQSRC_SDRAM);
    }
    else if (dir == DMA_DEV_TO_MEM)
    {
        dest = __va_to_pa(dest);
        sunxi_cfg_lli(l_item, src, dest, len, config, dir);
        l_item ->cfg |= GET_SRC_DRQ(config->slave_id)  \
                        | DST_INCREMENT_MODE \
                        | SRC_NOCHANGE_MODE \
                        | DST_DRQ(DRQSRC_SDRAM);
    }
    else if (dir == DMA_DEV_TO_DEV)
    {
        sunxi_cfg_lli(l_item, src, dest, len, config, dir);
        l_item->cfg |= GET_SRC_DRQ(config->slave_id) \
                       | DST_NOCHANGE_MODE \
                       | SRC_NOCHANGE_MODE \
                       | GET_DST_DRQ(config->slave_id);
    }

    sunxi_lli_list(NULL, l_item, chan);

    sunxi_dump_lli(chan, l_item);

    hal_spin_unlock_irqrestore(&chan->lock, __cpsr);

    return HAL_DMA_STATUS_OK;
}

hal_dma_status_t hal_dma_prep_cyclic(struct sunxi_dma_chan *chan,
				     uint32_t buf_addr, uint32_t buf_len,
				     uint32_t period_len, enum dma_transfer_direction dir)
{
    struct dma_slave_config *config = NULL;
    config = &chan->cfg;
	if(dir == DMA_MEM_TO_MEM)
	{
		return hal_dma_prep_memcpy(chan, config->dst_addr, config->src_addr, buf_len);
	}
    else
    {
		return hal_dma_prep_device(chan, config->dst_addr, config->src_addr, buf_len, dir);
	}
}

hal_dma_status_t hal_dma_callback_install(struct sunxi_dma_chan *chan,
					  dma_callback callback,
					  void *callback_param)
{
    unsigned long __cpsr;

    if (NULL == chan)
    {
        DMA_ERR("[dma] chan is NULL\n");
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    if (NULL == callback)
    {
        DMA_ERR("[dma] callback is NULL\n");
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    if (NULL == callback_param)
    {
        DMA_ERR("[dma] callback_param is NULL\n");
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    __cpsr = hal_spin_lock_irqsave(&chan->lock);
    chan->callback = callback;
    chan->callback_param = callback_param;
    hal_spin_unlock_irqrestore(&chan->lock, __cpsr);

    return HAL_DMA_STATUS_OK;
}

hal_dma_status_t hal_dma_slave_config(struct sunxi_dma_chan *chan,
				      struct dma_slave_config *config)
{
    unsigned long __cpsr;

    if (NULL == config || NULL == chan)
    {
        DMA_ERR("[dma] dma config is NULL\n");
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    __cpsr = hal_spin_lock_irqsave(&chan->lock);
    if(convert_burst(&config->src_maxburst))
		DMA_ERR("[dma] src bst len is over, use default val\n");
    if(convert_burst(&config->dst_maxburst))
		DMA_ERR("[dma] dst bst len is over, use default val\n");

    memcpy((void *) & (chan->cfg), (void *)config, sizeof(struct dma_slave_config));
    hal_spin_unlock_irqrestore(&chan->lock, __cpsr);

    return HAL_DMA_STATUS_OK;
}

hal_dma_status_t hal_dma_tx_pkgnum(struct sunxi_dma_chan *chan)
{
	int pkgnum;

#ifndef DMA_PKGNUM
	pkgnum = HAL_DMA_STATUS_ERR_PERM;
#else
	pkgnum = hal_readl(DMA_PKGNUM(chan->chan_count));
#endif
	return  pkgnum;
}


uint32_t hal_dma_pkg_status(struct sunxi_dma_chan *chan, int buf_size, uint32_t *total)
{
	size_t bytes;

#ifndef DMA_PKGNUM
	bytes = 0;
#else
	chan->new_val = hal_readl(DMA_PKGNUM(chan->chan_count));

	if (chan->new_val < chan->old_val)
		chan->val += DMA_PKG_NUM_MAX - chan->old_val + 1 + chan->new_val;
	else
		chan->val += chan->new_val - chan->old_val;

	chan->old_val = chan->new_val;
	chan->val = chan->val % buf_size;
	bytes = buf_size - chan->val;

	*total = chan->new_val;
#endif
	return bytes;
}

enum dma_status hal_dma_tx_status(struct sunxi_dma_chan *chan, uint32_t *left_size)
{
    uint32_t cfg_val = 0;
    enum dma_status status = DMA_INVALID_PARAMETER;
	*left_size = hal_readl(DMA_CNT(chan->chan_count));
	cfg_val = hal_readl(DMA_CFG(chan->chan_count));
	if(cfg_val & DMA_BUSY_STATUS)
		return status;
	status = DMA_COMPLETE;
    return status;
}

hal_dma_status_t hal_dma_start(struct sunxi_dma_chan *chan)
{
    uint32_t irq_val = 0;
    //uint32_t high = 0;
    struct sunxi_dma_lli *prev = NULL;
    unsigned long __cpsr, __ccpsr;
    uint32_t desc_addr;

    if (NULL == chan)
    {
        DMA_ERR("[dma] chan is NULL\n");
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    __cpsr = hal_spin_lock_irqsave(&chan->lock);

    if (!chan->irq_type)
        chan->irq_type = IRQ_PKG;

    uint32_t high = 0;
    high = (chan->chan_count + START_CHAN_OFFSET >= HIGH_CHAN) ? 1 : 0;

    __ccpsr = hal_spin_lock_irqsave(&dma_lock);
    irq_val = hal_readl(DMA_IRQ_EN(high));
    irq_val |= SHIFT_IRQ_MASK(chan->irq_type, chan->chan_count);
    hal_writel(irq_val, DMA_IRQ_EN(high));
    hal_spin_unlock_irqrestore(&dma_lock, __ccpsr);

    /* FlashCtrl cannot support handshake mode  */
    /* SET_OP_MODE(chan->chan_count, SRC_HS_MASK | DST_HS_MASK); */

    for (prev = chan->desc; prev != NULL; prev = prev->vlln)
    {
        hal_dcache_clean((unsigned long)prev, sizeof(*prev));
        /* k_dcache_clean(prev, sizeof(*prev)); */
        //k_dcache_clean(prev->src, prev->len);
        //k_dcache_clean_invalidate(prev->dst, prev->len);
    }
    desc_addr = __va_to_pa((unsigned long)chan->desc);

    uint32_t cfg_val = 0;
	struct sunxi_dma_lli * chan_desc = (struct sunxi_dma_lli *)desc_addr;
    hal_writel(chan_desc->src, DMA_CUR_SRC(chan->chan_count));
    hal_writel(chan_desc->dst, DMA_CUR_DST(chan->chan_count));
	cfg_val = hal_readl(DMA_CFG(chan->chan_count));
	cfg_val &= ~(DMA_WAIT(NDMA_WAIT_STATE_MASK)
			   | DST_WIDTH(NDMA_DEST_DATA_WIDTH_MASK)
			   | DST_BURST(NDMA_DEST_DATA_WIDTH_MASK)
			   | DST_ADDR_MODE(NDMA_DEST_ADDR_TYPE_MASK)
			   | DST_DRQ(NDMA_DEST_DRQ_TYPE_MASK)
			   | SRC_WIDTH(NDMA_SRC_DATA_WIDTH_MASK)
			   | SRC_BURST(NDMA_SRC_BST_LEN_MASK)
			   | SRC_ADDR_MODE(NDMA_SRC_ADDR_TYPE_MASK)
			   | SRC_DRQ(NDMA_SRC_DRQ_TYPE_MASK)
			   | SRC_CONTI(NDMA_CONTI_MODE_MASK)
			   | DMA_BCMODE(NDMA_BC_MODE_SEL_MASK)
			   );
	cfg_val |= chan_desc->cfg | chan_desc->para;
    hal_writel(cfg_val, DMA_CFG(chan->chan_count));
    hal_writel(chan_desc->len & NDMA_BYTE_COUNTER_REG_MASK, DMA_CNT(chan->chan_count));
	cfg_val = hal_readl(DMA_CFG(chan->chan_count));
	cfg_val |= DMA_LOADING;
    hal_writel(cfg_val, DMA_CFG(chan->chan_count));

    sunxi_dump_chan_regs(chan->chan_count);
    hal_spin_unlock_irqrestore(&chan->lock, __cpsr);
    return HAL_DMA_STATUS_OK;
}

hal_dma_status_t hal_dma_stop(struct sunxi_dma_chan *chan)
{
    unsigned long __cpsr;

    if (NULL == chan)
    {
        DMA_ERR("[dma] chan is NULL\n");
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    __cpsr = hal_spin_lock_irqsave(&chan->lock);
    /*We should entry PAUSE state first to avoid missing data
    * count witch transferring on bus.
    */
#ifdef CONFIG_ARCH_SUN300IW1
    uint32_t cfg_val = 0;
	cfg_val = hal_readl(DMA_CFG(chan->chan_count));
	cfg_val &= ~DMA_LOADING;
    hal_writel(cfg_val, DMA_CFG(chan->chan_count));
#endif

    if (chan->cyclic)
    {
        chan->cyclic = false;
    }
    hal_spin_unlock_irqrestore(&chan->lock, __cpsr);

    return HAL_DMA_STATUS_OK;
}

hal_dma_status_t hal_dma_chan_free(struct sunxi_dma_chan *chan)
{
    unsigned long irq_val = 0;
    //uint32_t high = 0;
    unsigned long __cpsr, __ccpsr;

    if (NULL == chan)
    {
        DMA_ERR("[dma] chan is NULL\n");
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    if (!chan->used)
    {
        return HAL_DMA_STATUS_INVALID_PARAMETER;
    }

    __cpsr = hal_spin_lock_irqsave(&chan->lock);

    uint32_t high = 0;
    high = (chan->chan_count + START_CHAN_OFFSET >= HIGH_CHAN) ? 1 : 0;

    __ccpsr = hal_spin_lock_irqsave(&dma_lock);
    irq_val = hal_readl(DMA_IRQ_EN(high));
    irq_val &= ~(SHIFT_IRQ_MASK(chan->irq_type, chan->chan_count));
    hal_writel(irq_val, DMA_IRQ_EN(high));
    hal_spin_unlock_irqrestore(&dma_lock, __ccpsr);

    sunxi_dma_free_ill(chan);
    chan->used = 0;
    hal_spin_unlock_irqrestore(&chan->lock, __cpsr);

    return HAL_DMA_STATUS_OK;
}

hal_dma_status_t hal_dma_chan_desc_free(struct sunxi_dma_chan *chan)
{
    hal_assert(!hal_interrupt_is_disable());
    hal_assert(!in_interrupt());

    sunxi_dma_free_ill(chan);

    return HAL_DMA_STATUS_OK;
}

void hal_dma_resource_get(void)
{
    uint32_t i = 0;
#ifdef DMA_SECURE
    uint32_t secure = 0;
#endif
    struct sunxi_dma_chan *chan;

    hal_spin_lock_init(&dma_lock);

    /* Detect that if there is a master that has not freed DMA, it will not memset! */
    for (i = SUNXI_DMA_CHAN_START; i < NR_MAX_CHAN; i++)
    {
        chan = &dma_chan_source[i];
        hal_spin_lock_init(&chan->lock);
        if (chan->used == 0)
        {
            memset(chan, 0, sizeof(struct sunxi_dma_chan));
        }
    }

    for (i = START_CHAN_OFFSET + SUNXI_DMA_CHAN_START; i < START_CHAN_OFFSET + NR_MAX_CHAN; i++)
    {
	 hal_dma_irq_set(i, 0);
	 hal_dma_irq_status(i, 1);
#ifdef DMA_SECURE
	/*set non-secure*/
	secure = hal_readl(DMA_SECURE);
	if ((secure & (1 << i)) == 0)
		hal_writel(secure | (1 << i), DMA_SECURE);
#endif
    }
    /* disable auto gating */
#ifdef CONFIG_ARCH_SUN300IW1
    uint32_t gate = hal_readl(DMA_GATE);
    hal_writel(gate | DMA_AUTO_CLK_GATE, DMA_GATE);
#endif
    sunxi_dma_clk_init(true);

    /*request dma irq*/
    if (!(irq_inited & 0x1)) {
        if (hal_request_irq(DMA_IRQ_NUM, sunxi_dma_irq_handle, "dma", NULL) < 0)
            DMA_ERR("[dma] request irq error\n");
        else
		irq_inited |= 0x1;
    }
    hal_enable_irq(DMA_IRQ_NUM);
#ifdef ENABLE_SECURE_DMA
    if (!(irq_inited & 0x10)) {
        if (hal_request_irq(DMA_SECURE_IRQ_NUM, sunxi_dma_irq_handle, "dma-s", NULL) < 0)
            DMA_ERR("[dma] request secure irq error\n");
	else
	    irq_inited |= 0x10;
    }
    hal_enable_irq(DMA_SECURE_IRQ_NUM);
#endif
}

#ifdef CONFIG_COMPONENTS_PM
static int hal_dma_suspend(void *date, suspend_mode_t mode)
{
    uint32_t i = 0;
    hal_disable_irq(DMA_IRQ_NUM);

    for (i = START_CHAN_OFFSET + SUNXI_DMA_CHAN_START; i < START_CHAN_OFFSET + NR_MAX_CHAN; i++)
    {
        hal_dma_irq_set(i, 0);
        hal_dma_irq_status(i, 1);
#ifdef CONFIG_ARCH_SUN300IW1
		uint32_t cfg_val = 0;
		cfg_val = hal_readl(DMA_CFG(i));
		cfg_val &= ~DMA_LOADING;
		hal_writel(cfg_val, DMA_CFG(i));
#endif
        /*clear all dma irq pending*/
        hal_dma_irq_status(i, 1);
    }

    sunxi_dma_clk_init(false);

    for (i = SUNXI_DMA_CHAN_START; i < NR_MAX_CHAN; i++)
        hal_spin_lock_deinit(&dma_chan_source[i].lock);

    hal_spin_lock_deinit(&dma_lock);

    pm_inf("support dma suspend\n");
    return 0;
}
static void hal_dma_resume(void *data, suspend_mode_t mode)
{
    hal_dma_resource_get();
    pm_inf("support dma resume\n");
}
struct syscore_ops pm_dma_ops = {
    .name = "sunxi_pm_dma",
    .suspend = hal_dma_suspend,
    .resume = hal_dma_resume,
};
#endif

/* only need to be executed once */
void hal_dma_init(void)
{
    hal_dma_resource_get();
#ifdef CONFIG_COMPONENTS_PM
    pm_syscore_register(&pm_dma_ops);
#endif
}

