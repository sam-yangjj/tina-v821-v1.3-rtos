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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_dma.h>
#include <hal_osal.h>
#include <hal_atomic.h>
#include <hw_timestamp.h>
#include <sunxi_hal_common.h>

#include "cache_test.h"

static void dma_test_cb(void *param)
{
}

__attribute__((optimize("O0"))) static uint64_t test_icache_func(void)
{
	return hw_get_timestamp_ns();
}

int cmd_test_cache(int argc, char **argv)
{
    int ret, i;
    struct sunxi_dma_chan *hdma = NULL;
    char *src = NULL, *dst = NULL;
    struct dma_slave_config config = {0};
    uint32_t size = 0;
    int loop_cnt = 0;
    unsigned long flags;
    uint64_t t1, t2;

    dst = hal_malloc_coherent(DCACHE_TEST_LEN);
    src = hal_malloc_coherent(DCACHE_TEST_LEN);

    if (src == NULL || dst == NULL)
    {
        printf("malloc src error!");
        ret = -CACHE_TEST_MALLOC_FAILED;
        goto end;
    }

    flags = hal_enter_critical();
    /* make hw_get_timestamp_ns code into icache */
    t1 = hw_get_timestamp_ns();
    t2 = 0;
    for (i = 0; i < 1024 * 1024; i++) {
        memset(src, 0x11, DCACHE_TEST_LEN);
        t1 = hw_get_timestamp_ns();
        /* TODO: remove 'sync' instruction spend ? */
        hal_dcache_clean((unsigned long)src, DCACHE_TEST_LEN);
        t1 = hw_get_timestamp_ns() - t1;
        t2 += t1;
    }
    hal_exit_critical(flags);

    t2 = t2 >> 20;    // t2 = t2 / (1024 * 1024);
    t2 /= (DCACHE_TEST_LEN / CACHELINE_LEN);
    printf("dcache writeback latency: %lu ns\n", (unsigned long)t2);

    flags = hal_enter_critical();
    t1 = hw_get_timestamp_ns();
    t2 = 0;
    for (i = 0; i < 1024 * 1024; i++) {
        memset(src, 0x22, DCACHE_TEST_LEN);
        t1 = hw_get_timestamp_ns();
        /* TODO: remove 'sync' instruction spend ? */
        hal_dcache_invalidate((unsigned long)src, DCACHE_TEST_LEN);
        t1 = hw_get_timestamp_ns() - t1;
        t2 += t1;
        memcpy(dst, src, DCACHE_TEST_LEN);
    }
    hal_exit_critical(flags);

    t2 = t2 >> 20;    // t2 = t2 / (1024 * 1024);
    t2 /= (DCACHE_TEST_LEN / CACHELINE_LEN);
    printf("dcache invalidate latency: %lu ns\n", (unsigned long)t2);

    flags = hal_enter_critical();
    t1 = hw_get_timestamp_ns();
    t2 = 0;
    for (i = 0; i < 4096; i++) {
        hal_icache_invalidate((unsigned long)test_icache_func, CACHELINE_LEN);
        t1 = hw_get_timestamp_ns();
        t1 = test_icache_func() - t1;
        t2 += t1;
    }
    hal_exit_critical(flags);

    t2 = t2 >> 12;
    printf("icache missing latency: %lu ns\n", (unsigned long)t2);

    /* request dma chan */
    ret = hal_dma_chan_request(&hdma);
    if (ret == -HAL_DMA_CHAN_STATUS_BUSY)
    {
        printf("dma channel busy!");
        ret = -CACHE_TEST_DMA_BUSY;
        goto end;
    }

    /* register dma callback */
    ret = hal_dma_callback_install(hdma, dma_test_cb, hdma);
    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("register dma callback failed!");
        ret = -CACHE_TEST_DMA_CALLBACK_FAILED;
        goto end;
    }

    config.direction = DMA_MEM_TO_MEM;
    config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    config.dst_maxburst = DMA_SLAVE_BURST_4;
    config.src_maxburst = DMA_SLAVE_BURST_4;
    config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_SDRAM);

    ret = hal_dma_slave_config(hdma, &config);

    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("dma config error, ret:%d", ret);
        ret = -CACHE_TEST_DMA_CONFIG_ERR;
        goto end;
    }

    ret = hal_dma_prep_memcpy(hdma, (uintptr_t)dst, (uintptr_t)src, DCACHE_TEST_LEN);
    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("dma prep error, ret:%d", ret);
        ret = -CACHE_TEST_DMA_PREP_ERR;
        goto end;
    }

    /* avoid dcache missing caused by context switch */
    flags = hal_enter_critical();

    memset(src, 0, DCACHE_TEST_LEN);
    memset(dst, 0, DCACHE_TEST_LEN);

    for (i = 0; i < DCACHE_TEST_LEN; i++)
    {
        src[i] = i & 0xff;
    }

    hal_dcache_clean((unsigned long)src, DCACHE_TEST_LEN);

    /* read dst buffer to make sure dst buffer has been allocated in dcache */
    for (i = 0; i < DCACHE_TEST_LEN; i++)
    {
        if (dst[i] != 0) {
            printf("dst buffer check error\n");
            ret = -CACHE_TEST_FAILED;
            goto exit_critical;
        }
    }

    ret = hal_dma_start(hdma);
    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("dma start error, ret:%d", ret);
        ret = -CACHE_TEST_DMA_START_ERR;
        goto exit_critical;
    }

    while (hal_dma_tx_status(hdma, &size) != 0) {
        loop_cnt ++;
        if (loop_cnt == 1000 * 100) {
            printf("%s(%d) dms wait timeout\n", __func__, __LINE__);
            ret = -CACHE_TEST_DMA_WAIT_ERR;
            goto exit_critical;
        }
        hal_usleep(10);
    }

    ret = hal_dma_stop(hdma);
    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("dma stop error, ret:%d", ret);
        ret = -CACHE_TEST_DMA_STOP_ERR;
        goto exit_critical;
    }

    /*
     * dst the data in dcache is 0
     * dst the data in dram data is the same as src
     * */
    if (!memcmp(dst, src, DCACHE_TEST_LEN))
    {
        printf("test1: meet error, dcache maybe not open or dma work faild !!!!\n");
        printf("src buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", src[i]);
        }

        printf("\ndst buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", dst[i]);
        }
        printf("\n\n");
        goto exit_critical;
    }
    else
    {
        for (i = 0; i < DCACHE_TEST_LEN; i++) {
            if (dst[i] == 0) {
                continue;
            } else {
                printf("test1: data error, dcache updata!\n");
                ret = -CACHE_TEST_FAILED;
                goto exit_critical;
            }

        }
        printf("test1: dcache open ok, src & dst data all 0\n");
    }

    hal_dcache_invalidate((unsigned long)src, DCACHE_TEST_LEN);
    hal_dcache_invalidate((unsigned long)dst, DCACHE_TEST_LEN);

    if (memcmp(dst, src, DCACHE_TEST_LEN))
    {
        printf("test2: meet error, invalidate dcache failed !!!!\n");
        printf("src buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", src[i]);
        }

        printf("\ndst buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", dst[i]);
        }
        printf("\n\n");
        ret = -CACHE_TEST_FAILED;
        goto exit_critical;
    }
    else
    {
        for (i = 0; i < DCACHE_TEST_LEN; i++) {
            if (dst[i] == src[i]){
                continue;
            } else {
                printf("test2: data error, dst[%d]:%x data error!\n", i, dst[i]);
                for (i = 0; i < DCACHE_TEST_LEN; i++){
                    printf("0x%x ", dst[i]);
                }
                ret = -CACHE_TEST_FAILED;
                goto exit_critical;
            }

        }
        printf("test2: dcache clean & invalidate range successfully!\n");
        ret = CACHE_TEST_RET_OK;
    }

    printf("Cache test success!\n");

exit_critical:
    hal_exit_critical(flags);
end:
    if (hdma) {
        ret = hal_dma_chan_free(hdma);
        if (ret != HAL_DMA_STATUS_OK)
        {
            printf("dma free error, ret:%d", ret);
            ret = -CACHE_TEST_DMA_FREE_ERR;
        }
        hdma = NULL;
    }

    if (src)
    {
        hal_free_coherent(src);
    }

    if (dst)
    {
        hal_free_coherent(dst);
    }

    return ret;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_cache, test_cache, cache tests);
