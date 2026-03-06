/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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
#include <unistd.h>

#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_dma.h>
#include <hal_timer.h>

#include <sunxi_hal_common.h>

#define DMA_TEST_LEN	1024
//#define USE_CYCLIC_TEST

#ifdef USE_CYCLIC_TEST

#define DMA_CYCLIC_TEST_LEN 4096
#define DMA_CYCLIC_PERIOD_LEN  1024

int cyclic_time = 0;

static void cyclic_dma_callback(void *param)
{
	hal_log_info("cyclic DMA finished, callback to do something...\n");
	cyclic_time++;
}

int cmd_test_dma_cyclic(int args, char **argv)
{
	// dma_addr_t dma_buf;
	char *dma_buf_send = NULL;
	char *dma_buf_recv = NULL;
	struct sunxi_dma_chan *chan = NULL;
	struct dma_slave_config slave_config = {0};
	int ret, i = 0;
	uint32_t size = 0;

	/* 申请内存 */
	dma_buf_send = hal_malloc_coherent(DMA_CYCLIC_TEST_LEN);
	dma_buf_recv = hal_malloc_coherent(DMA_CYCLIC_TEST_LEN);

	/* 初始化内存 */
	memset(dma_buf_send, 0, DMA_CYCLIC_TEST_LEN);
	memset(dma_buf_recv, 0, DMA_CYCLIC_TEST_LEN);

	for (i = 0;i < DMA_CYCLIC_TEST_LEN; i++)
		dma_buf_send[i] = i & 0xff;

	/* 申请通道 */
	ret = hal_dma_chan_request(&chan);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		printf("dma channel busy!\n");
	}

	/* playback */
	slave_config.direction = DMA_MEM_TO_MEM;
	slave_config.dst_addr = (unsigned long)dma_buf_recv;
	slave_config.src_addr = (unsigned long)dma_buf_send;
	/* 16bit*/
	slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	slave_config.dst_maxburst = DMA_SLAVE_BURST_4;
	slave_config.src_maxburst = DMA_SLAVE_BURST_4;
	/* playback */
	slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_SDRAM);

	ret = hal_dma_slave_config(chan, &slave_config);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("[%s, %d] failed!\n", __func__, __LINE__);
	}

	/* flush ringbuffer */
	hal_dcache_clean_invalidate((unsigned long)dma_buf_send, DMA_CYCLIC_TEST_LEN);
	hal_dcache_clean_invalidate((unsigned long)dma_buf_recv, DMA_CYCLIC_TEST_LEN);

	/* 传输描述符 */
	ret = hal_dma_prep_cyclic(chan, (unsigned long)dma_buf_send, DMA_CYCLIC_TEST_LEN, DMA_CYCLIC_PERIOD_LEN, DMA_MEM_TO_MEM);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("[%s, %d] failed!\n", __func__, __LINE__);
		goto end;
	}

	ret = hal_dma_callback_install(chan, cyclic_dma_callback, chan);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("[%s, %d] failed!\n", __func__, __LINE__);
		goto end;
	}

	/* 启动 */
	ret = hal_dma_start(chan);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("[%s, %d] failed!\n", __func__, __LINE__);
		goto end;
	}

	while (hal_dma_tx_status(chan, &size) != 0) {
		printf("size left is %x\n",size);
		if (cyclic_time == 4) {
			cyclic_time = 0;
			break;
		}
	}

	hal_dcache_invalidate((unsigned long)dma_buf_recv, DMA_CYCLIC_TEST_LEN);

	hal_log_info("src buf:\n");
	for (i = 0;i < DMA_CYCLIC_TEST_LEN; i++) {
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", dma_buf_send[i]);
	}
	printf("\n\n\n");
	hal_log_info("dst buf:\n");
	for (i = 0;i < DMA_CYCLIC_TEST_LEN; i++) {
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", dma_buf_recv[i]);
	}
	printf("\n\n\n");

	if (memcmp(dma_buf_recv, dma_buf_send, DMA_CYCLIC_TEST_LEN) != 0)
		printf("dma test fail\n");
	else
		printf("dma test pass\n");


	ret = hal_dma_stop(chan);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("[%s, %d] failed!\n", __func__, __LINE__);
	}

	ret = hal_dma_chan_free(chan);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("[%s, %d] failed!\n", __func__, __LINE__);
		goto end;
	}

end:
	hal_free_coherent(dma_buf_send);
	hal_free_coherent(dma_buf_recv);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_dma_cyclic, hal_dma_cyclic, dma hal cyclic APIs tests)

#endif

static struct test_dma_para {
	uint8_t conti_mode;
	uint8_t irq_type;
} test_dma_para_t;

static void show_help(void)
{
	printf("Usage: hal_dma [-h]/[-i]/[-c conti_mode] [-q irq_type] \n");
	printf("\n");
	printf("\t-h : show this massage and exit\n");
	printf("\t-i : show this dma info\n");
	printf("\t-c : set dma conti_mode; 1: enable; 0: disable\n");
	printf("\t-q : set dma irq_type; 1: IRQ_PKG; 0: IRQ_HALF\n");
	printf("\n");
}

static int test_dma_parse(int argc, char **argv)
{
    int c;
    while ((c = getopt(argc, (char *const *)argv, "hic:q:")) != -1)
	{
        switch (c)
        {
			case 'h':
				show_help();
				return -1;
            case 'i':
				hal_log_info("dma continue mode: %s",
					     test_dma_para_t.conti_mode ? "enable" : "disable");
				hal_log_info("dma irq type: %s",
					     test_dma_para_t.irq_type ? "IRQ_PKG" : "IRQ_HALF");
				return -1;
            case 'c':
#ifdef CONFIG_ARCH_SUN300IW1
				test_dma_para_t.conti_mode = atoi(optarg);
#else
				hal_log_info("dma dont support conti_mode\n");
#endif
                break;
            case 'q':
				test_dma_para_t.irq_type = atoi(optarg);
                break;
			default:
				hal_log_err("invalid param!");
				return -1;
        }
    }
	return 0;
}

char *buf1 = NULL, *buf2 = NULL;
struct sunxi_dma_chan *hdma = NULL;
static int cir_test_end = 0;
static void dma_test_cb(void *param)
{
	static int cir_cnt = 0;
	int ret;
	hal_log_info("DMA finished, callback to do something...");
	if (test_dma_para_t.conti_mode) {
		if (cir_test_end) {
			return;
		}
		hal_log_info("circle test %d :", cir_cnt);
		if (memcmp(buf1, buf2, DMA_TEST_LEN) != 0)
			hal_log_info("dma test fail\n");
		else
			hal_log_info("dma test pass\n");
		memset(buf2, 0, DMA_TEST_LEN);
		cir_cnt ++;
		hal_dcache_invalidate((unsigned long)buf2, DMA_TEST_LEN);
		if(cir_cnt >= 10) {
			cir_cnt = 0;
			cir_test_end = 1;
			ret = hal_dma_stop(hdma);
			if (ret != HAL_DMA_STATUS_OK) {
				hal_log_err("dma stop error, ret:%d", ret);
				return;
			}

			ret = hal_dma_chan_free(hdma);
			if (ret != HAL_DMA_STATUS_OK) {
				hal_log_err("dma free error, ret:%d", ret);
				return;
			}
		}
	}
}

int cmd_test_dma(int argc, char **argv)
{
	int ret, i;
	struct dma_slave_config config = {0};
	uint32_t size = 0;
	uint32_t timeout = 0xff;

	hal_log_info("run in dma test");
	if (test_dma_parse(argc, argv))
		return 0;

	buf2 = hal_malloc_coherent(DMA_TEST_LEN);
	buf1 = hal_malloc_coherent(DMA_TEST_LEN);

	if (buf1 == NULL) {
		hal_log_err("malloc buf1 error!");
		goto end;
	}

	if (buf2 == NULL) {
		hal_log_err("malloc buf2 error!");
		goto end;
	}

	memset(buf1, 0, DMA_TEST_LEN);
	memset(buf2, 0, DMA_TEST_LEN);

	for (i = 0;i < DMA_TEST_LEN; i++)
		buf1[i] = i & 0xff;

	hal_dcache_clean_invalidate((unsigned long)buf1, DMA_TEST_LEN);
	hal_dcache_clean_invalidate((unsigned long)buf2, DMA_TEST_LEN);

	/* request dma chan */
	ret = hal_dma_chan_request(&hdma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		hal_log_err("dma channel busy!");
		goto end;
	}

	/* register dma callback */
	ret = hal_dma_callback_install(hdma, dma_test_cb, hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("register dma callback failed!");
		goto end;
	}

	config.direction = DMA_MEM_TO_MEM;
	config.dst_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
	config.src_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
	config.dst_maxburst = DMA_SLAVE_BURST_16;
	config.src_maxburst = DMA_SLAVE_BURST_16;
	if (test_dma_para_t.conti_mode)
		config.conti_mode = DMA_CONTI_ENABLE;
	config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_SDRAM);

	ret = hal_dma_slave_config(hdma, &config);

	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma config error, ret:%d", ret);
		goto end;
	}

	ret = hal_dma_prep_memcpy(hdma, (unsigned long)buf2, (unsigned long)buf1, DMA_TEST_LEN);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma prep error, ret:%d", ret);
		goto end;
	}

	hdma->irq_type = (test_dma_para_t.irq_type ? IRQ_PKG : IRQ_HALF);
	ret = hal_dma_start(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma start error, ret:%d", ret);
		goto end;
	}

	hal_sleep(1);

	if (test_dma_para_t.conti_mode) {
		while(!cir_test_end);
		cir_test_end = 0;
		goto end;
	}

	while (hal_dma_tx_status(hdma, &size)!= 0 && timeout --)
	{
		printf("wait dma transfer finash ... \n");
	}

	ret = hal_dma_stop(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma stop error, ret:%d", ret);
		goto end;
	}

	ret = hal_dma_chan_free(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma free error, ret:%d", ret);
		goto end;
	}

	hal_dcache_invalidate((unsigned long)buf2, DMA_TEST_LEN);

	hal_log_info("src buf:\n");
	for (i = 0;i < DMA_TEST_LEN; i++) {
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", buf1[i]);
	}
	printf("\n\n\n");
	hal_log_info("dst buf:\n");
	for (i = 0;i < DMA_TEST_LEN; i++) {
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", buf2[i]);
	}
	printf("\n\n\n");

	if (memcmp(buf1, buf2, DMA_TEST_LEN) != 0)
		printf("dma test fail\n");
	else
		printf("dma test pass\n");

end:
	hal_free_coherent(buf1);
	hal_free_coherent(buf2);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_dma, hal_dma, dma hal APIs tests)
