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
#include <sunxi_hal_common.h>
#include <sunxi_hal_avs.h>
#include <interrupt.h>
#include <stdlib.h>
#include <stdio.h>

static hal_sunxi_avs hal_avs[AVS_NUM];

int hal_avs_continue(hal_avs_id_t id)
{
    u32 val;
    hal_sunxi_avs *avs;

    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }
    avs = &hal_avs[id];

    val = hal_readl(avs->base + AVS_CNT_CTRL_REG);
    val &= ~(1 << (id + 8));
    hal_writel(val, avs->base + AVS_CNT_CTRL_REG);
    return 0;
}

int hal_avs_pause(hal_avs_id_t id)
{
    u32 val;
    hal_sunxi_avs *avs;

    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }
    avs = &hal_avs[id];

    val = hal_readl(avs->base + AVS_CNT_CTRL_REG);
    val |= (1 << (id + 8));
    hal_writel(val, avs->base + AVS_CNT_CTRL_REG);
    return 0;
}

int hal_avs_disable(hal_avs_id_t id)
{
    u32 val;
    hal_sunxi_avs *avs;

    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }
    avs = &hal_avs[id];

    val = hal_readl(avs->base + AVS_CNT_CTRL_REG);
    val &= ~(1 << id);
    hal_writel(val, avs->base + AVS_CNT_CTRL_REG);
    return 0;
}

int hal_avs_enable(hal_avs_id_t id)
{
    u32 val;
    hal_sunxi_avs *avs;

    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }
    avs = &hal_avs[id];

    val = hal_readl(avs->base + AVS_CNT_CTRL_REG);
    val |= (1 << id);
    hal_writel(val, avs->base + AVS_CNT_CTRL_REG);
    return 0;
}

int hal_avs_get_counter(hal_avs_id_t id, u32 *counter)
{
    hal_sunxi_avs *avs;

    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }

    avs = &hal_avs[id];
    *counter = hal_readl(avs->base + AVS_CNT_REG(id));
    return 0;
}

int hal_avs_set_counter(hal_avs_id_t id, u32 counter)
{
    hal_sunxi_avs *avs;

    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }

    avs = &hal_avs[id];

    hal_writel(counter, avs->base + AVS_CNT_REG(id));

    return 0;
}

int hal_avs_set_cnt_div(hal_avs_id_t id, u32 div)
{
    u32 val;
    hal_sunxi_avs *avs;

    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }

    avs = &hal_avs[id];

    val = hal_readl(avs->base + AVS_CNT_DIV_REG);
    val &= ~(AVS_DIV_MASK << (16 * id));
    div &= AVS_DIV_MASK;
    val |= (div << (16 * id));
    hal_writel(val, avs->base + AVS_CNT_DIV_REG);

    return 0;
}

int hal_avs_init(hal_avs_id_t id)
{
    hal_sunxi_avs *avs;

    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }

    avs = &hal_avs[id];
    avs->id = id;
    avs->base = SUNXI_TMR_PBASE;
    avs->enable = 1;
    avs->clk = hal_clock_get(SUNXI_AVS_CLK_TYPE, SUNXI_AVS_CLK);

#if defined(CONFIG_DRIVERS_SUNXI_CLK)
    AVS_INFO("avs_clk:%u", avs->clk);
#endif
    hal_clock_enable(avs->clk);

    return 0;
}

int hal_avs_uninit(hal_avs_id_t id)
{
    hal_sunxi_avs *avs;
    hal_avs_id_t i;

    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }

    avs = &hal_avs[id];
    avs->enable = 0;

    for (i = 0; i < AVS_NUM; i++)
    {
        if (hal_avs[i].enable)
        {
            break;
        }
    }

    if (i == AVS_NUM)
    {
        hal_clock_disable(avs->clk);
    }

    return 0;
}

int hal_avs_control(hal_avs_id_t id, hal_avs_cmd_t cmd, void *arg)
{
    hal_sunxi_avs *avs;
    u32 *counter, *div;


    if (id >= AVS_NUM)
    {
        AVS_ERR("avs id is too big!!!\n");
        return -1;
    }

    avs = &hal_avs[id];

    switch (cmd)
    {
        case AVS_ENABLE:
            return hal_avs_enable(id);
        case AVS_DISABLE:
            return hal_avs_disable(id);
        case AVS_PAUSE:
            return hal_avs_pause(id);
        case AVS_CONTINUE:
            return hal_avs_continue(id);
        case AVS_SET_COUNTER:
            counter = (u32 *)arg;
            return hal_avs_set_counter(id, *counter);
        case AVS_GET_COUNTER:
            counter = (u32 *)arg;
            return hal_avs_get_counter(id, counter);
        case AVS_SET_DIV:
            div = (u32 *)arg;
            return hal_avs_set_cnt_div(id, *div);
        default:
            return -1;
    }

    return 0;
}
