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

#include <hal_uart.h>
#include <hal_interrupt.h>
#include <hal_queue.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <hal_gpio.h>
#ifdef CONFIG_DRIVER_SYSCONFIG
#include <hal_cfg.h>
#include <script.h>
#endif
#include "uart.h"
#ifdef CONFIG_STANDBY
#include <standby/standby.h>
#endif

#ifdef UART_PCLK
#include <hal_time.h>
#endif
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#include <pm_syscore.h>
#ifdef CONFIG_PM_STANDBY_MEMORY
#include <pm_mem.h>
#endif
#endif

#if (0)
#define UART_LOG_DEBUG
#endif
#define UART_INIT(fmt, ...) printf("uart: "fmt, ##__VA_ARGS__)
#define UART_ERR(fmt, ...)  printf("uart: "fmt, ##__VA_ARGS__)

#ifdef UART_LOG_DEBUG
#define UART_INFO(fmt, ...) printf("[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define UART_INFO_IRQ(fmt, ...) printf("[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define UART_INFO(fmt, ...)
#define UART_INFO_IRQ(fmt, ...)
#endif

#ifdef CONFIG_UART_DMA
/* temporarily avoid stack overruns and excessive memory usage in hal_dma_prep_cyclic */
#define	RX_BUF_LEN	4096
#define RX_TRIGGER_SIZE	1024
#define RX_DMAIRQ_QUIET_ACCEPT_TIME  20 //ms

#define DMA_PKG_NUM_MASK	0xFFFFFFFF
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))
#define CIRC_CNT_TO_END(head,tail,size) \
        ({int end = (size) - (tail); \
          int n = ((head) + end) & ((size)-1); \
          n < end ? n : end;})

#else
void hal_uart_dma_start(uart_port_t uart_port){};
void hal_uart_dma_stop(uart_port_t uart_port){};
void hal_uart_dma_send(uart_port_t uart_port, const uint8_t *temp_buf, uint32_t tx_buf_len){};
void hal_uart_dma_recv(uart_port_t uart_port, uint8_t *usr_buf, uint32_t usr_buf_len){};
uint32_t hal_uart_dma_recv_wait(uart_port_t uart_port, uint8_t *usr_buf, uint32_t usr_buf_len, int32_t timeout){return 0;};
#endif

#define _CONTACT(__STR_X__, __STR_Y__)	__STR_X__##__STR_Y__
#define CONTACT(__STR_X__, __STR_Y__)	_CONTACT(__STR_X__, __STR_Y__)

#define UART_PORT(__port__)		CONTACT(UART_, __port__)
#define SUNXI_UART_BASE(__port__)	CONTACT(CONTACT(SUNXI_UART, __port__), _BASE)
#define SUNXI_IRQ_UART(__port__)	CONTACT(SUNXI_IRQ_UART, __port__)
#define SUNXI_CLK_UART(__port__)	CONTACT(SUNXI_CLK_UART, __port__)
#define SUNXI_RST_UART(__port__)	CONTACT(SUNXI_RST_UART, __port__)
#define UART_TX(__port__)		CONTACT(CONTACT(UART, __port__), _TX)
#define UART_RX(__port__)		CONTACT(CONTACT(UART, __port__), _RX)
#define UART_GPIO_FUNCTION(__port__)	CONTACT(CONTACT(UART, __port__), _GPIO_FUNCTION)

#ifdef CONFIG_UART_DMA
#define DRQDST_UART_TX(__port__)	CONTACT(CONTACT(DRQDST_UART, __port__), _TX)
#define DRQSRC_UART_RX(__port__)	CONTACT(CONTACT(DRQSRC_UART, __port__), _RX)
/* define this value in the future */
#define UART_USE_DMA(__port__)		(false)
#else
#define DRQDST_UART_TX(__port__)	(-1)
#define DRQSRC_UART_RX(__port__)	(-1)
#define UART_USE_DMA(__port__)		(false)
#endif

#define SUNXI_UART_MSG_MEMBER(_port_)	\
	[UART_PORT(_port_)] = { \
		.base = SUNXI_UART_BASE(_port_), \
		.irqn = SUNXI_IRQ_UART(_port_), \
		.clk_id = SUNXI_CLK_UART(_port_), \
		.rst_id = SUNXI_RST_UART(_port_), \
		.uart_tx = UART_TX(_port_), \
		.uart_rx = UART_RX(_port_), \
		.func = UART_GPIO_FUNCTION(_port_), \
		.drqdst_tx = DRQDST_UART_TX(_port_), \
		.drqsrc_rx = DRQSRC_UART_RX(_port_), \
		.used_dma = UART_USE_DMA(_port_) \
	}

#if defined(CONFIG_PM_STANDBY_MEMORY) && !defined(CONFIG_PM_WAKESRC_LPUART)
__standby_unsaved_data
#endif
struct sunxi_uart_msg uart_msg[UART_MAX] =
{
#ifdef SUNXI_UART0_BASE
	SUNXI_UART_MSG_MEMBER(0),
#endif
#ifdef SUNXI_UART1_BASE
	SUNXI_UART_MSG_MEMBER(1),
#endif
#ifdef SUNXI_UART2_BASE
	SUNXI_UART_MSG_MEMBER(2),
#endif
#ifdef SUNXI_UART3_BASE
	SUNXI_UART_MSG_MEMBER(3),
#endif
#ifdef SUNXI_UART4_BASE
	SUNXI_UART_MSG_MEMBER(4),
#endif
#ifdef SUNXI_UART5_BASE
	SUNXI_UART_MSG_MEMBER(5),
#endif
#ifdef SUNXI_UART6_BASE
	SUNXI_UART_MSG_MEMBER(6),
#endif
#ifdef SUNXI_UART7_BASE
	SUNXI_UART_MSG_MEMBER(7),
#endif
#ifdef SUNXI_UART8_BASE
	SUNXI_UART_MSG_MEMBER(8),
#endif
#ifdef SUNXI_UART9_BASE
	SUNXI_UART_MSG_MEMBER(9),
#endif
#ifdef SUNXI_UART10_BASE
	SUNXI_UART_MSG_MEMBER(10),
#endif
#ifdef SUNXI_UART11_BASE
	SUNXI_UART_MSG_MEMBER(11),
#endif
#ifdef SUNXI_UART12_BASE
	SUNXI_UART_MSG_MEMBER(12),
#endif
#ifdef SUNXI_UART13_BASE
	SUNXI_UART_MSG_MEMBER(13),
#endif
#ifdef SUNXI_UART14_BASE
	SUNXI_UART_MSG_MEMBER(14),
#endif
#ifdef SUNXI_UART15_BASE
	SUNXI_UART_MSG_MEMBER(15),
#endif
#ifdef SUNXI_UART16_BASE
	SUNXI_UART_MSG_MEMBER(16),
#endif
};

#define UART_ERR_SHIFT       (8)
#define UART_ERR_VMASK       (0xffffff)
#define UART_ERR_RECV_TERM   ((1 & UART_ERR_VMASK) << UART_ERR_SHIFT)

static const sunxi_hal_version_t hal_uart_driver =
{
	SUNXI_HAL_UART_API_VERSION,
	SUNXI_HAL_UART_DRV_VERSION
};

#ifdef CONFIG_PM_STANDBY_MEMORY
__standby_unsaved_bss
#endif
static uart_priv_t g_uart_priv[UART_MAX];

static hal_mailbox_t uart_mailbox[UART_MAX];

static const uint32_t g_uart_baudrate_map[] =
{
	300,
	600,
	1200,
	2400,
	4800,
	9600,
	19200,
	38400,
	57600,
	115200,
	230400,
	460800,
	576000,
	921600,
	1000000,
	1500000,
	3000000,
	4000000,
};

//driver capabilities, support uart function only.
static const sunxi_hal_uart_capabilities_t driver_capabilities =
{
	1, /* supports UART (Asynchronous) mode */
	0, /* supports Synchronous Master mode */
	0, /* supports Synchronous Slave mode */
	0, /* supports UART Single-wire mode */
	0, /* supports UART IrDA mode */
	0, /* supports UART Smart Card mode */
	0, /* Smart Card Clock generator available */
	0, /* RTS Flow Control available */
	0, /* CTS Flow Control available */
	0, /* Transmit completed event: \ref ARM_UARTx_EVENT_TX_COMPLETE */
	0, /* Signal receive character timeout event: \ref ARM_UARTx_EVENT_RX_TIMEOUT */
	0, /* RTS Line: 0=not available, 1=available */
	0, /* CTS Line: 0=not available, 1=available */
	0, /* DTR Line: 0=not available, 1=available */
	0, /* DSR Line: 0=not available, 1=available */
	0, /* DCD Line: 0=not available, 1=available */
	0, /* RI Line: 0=not available, 1=available */
	0, /* Signal CTS change event: \ref ARM_UARTx_EVENT_CTS */
	0, /* Signal DSR change event: \ref ARM_UARTx_EVENT_DSR */
	0, /* Signal DCD change event: \ref ARM_UARTx_EVENT_DCD */
	0, /* Signal RI change event: \ref ARM_UARTx_EVENT_RI */
	0  /* Reserved */
};

#ifdef CONFIG_UART_DMA
void tx_uart_dma_callback(void *param)
{
	uart_priv_t *uart_priv = param;
	int hal_sem_ret;

	hal_sem_ret = hal_sem_post(uart_priv->hal_sem);
	if (hal_sem_ret)
		UART_ERR("tx callback wake up error\n");
	UART_INFO("tx_dma finished, callback to do something...\n");
}

void rx_uart_dma_callback(void *param)
{
	int hal_sem_ret;
	uint32_t size = 0, total = 0, count = 0;
	uart_priv_t *uart_priv = param;
	uart_ring_buf_t *rx_ring = &(uart_priv->ring_buf);

	hal_dcache_invalidate((unsigned long)uart_priv->rx_buf, uart_priv->uart_config->rx_buf_len);

	hal_dma_tx_status(uart_priv->rx_dma, &size);
	UART_INFO("dma left data: %d\n",size);

	rx_ring->head = uart_priv->uart_config->rx_buf_len - size;

	total = CIRC_CNT(rx_ring->head, rx_ring->tail, rx_ring->len);
	count = CIRC_CNT_TO_END(rx_ring->head, rx_ring->tail, rx_ring->len);

	UART_INFO("total = %d, count = %d\n", total, count);

	if (!uart_priv->rx_tmpnum) {
		memcpy(rx_ring->buf, uart_priv->rx_buf + rx_ring->tail, count);
		if (total > count)
			memcpy(rx_ring->buf + count, uart_priv->rx_buf, total - count);
	} else {
		memcpy(rx_ring->buf + uart_priv->rx_tmpnum, uart_priv->rx_buf + rx_ring->tail, count);
		if (total > count)
			memcpy(rx_ring->buf + uart_priv->rx_tmpnum + count, uart_priv->rx_buf, total - count);
	}

	rx_ring->tail = (rx_ring->tail + total) & (rx_ring->len - 1);
	uart_priv->rx_tmpnum += total;

	hal_sem_ret = hal_sem_post(uart_priv->hal_rxsem);
	if (hal_sem_ret)
		UART_ERR("rx callback wake up error\n");

	if (uart_priv->rx_tmpnum >= rx_ring->len)
		UART_ERR("rx data overrun\n");

	if (!IS_ERR_OR_NULL(uart_priv->uart_config->callback))
		uart_priv->uart_config->callback();
}

hal_uart_status_t hal_uart_init_rx_dma(uart_port_t uart_port)
{
	int ret;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uart_ring_buf_t *rx_ring = &(uart_priv->ring_buf);
	struct sunxi_dma_desc *sunxi_desc = &(uart_priv->sunxi_desc);

	if ((uart_priv->uart_config->uart_rx_trigger_size <= 0) || (uart_priv->uart_config->uart_rx_trigger_size > RX_TRIGGER_SIZE))
		uart_priv->uart_config->uart_rx_trigger_size = RX_TRIGGER_SIZE;

	if ((uart_priv->uart_config->rx_buf_len <= 0) ||(uart_priv->uart_config->rx_buf_len > RX_BUF_LEN))
		uart_priv->uart_config->rx_buf_len = RX_BUF_LEN;

	if (uart_priv->uart_config->rx_buf_len % uart_priv->uart_config->uart_rx_trigger_size) {
		uart_priv->uart_config->uart_rx_trigger_size = RX_TRIGGER_SIZE;
		uart_priv->uart_config->rx_buf_len = RX_BUF_LEN;
		UART_ERR("The uart_rx_trigger_size set by the user is not an integer multiple of rx_buf_len, restore the default value\n");
	}

	uart_priv->hal_rxsem = hal_sem_create(0);
	if (!uart_priv->hal_rxsem) {
		UART_ERR("uart-%d create rx sem failed\n", uart_port);
		return -1;
	}

	uart_priv->rx_buf = hal_malloc_coherent(uart_priv->uart_config->rx_buf_len);
	if (!uart_priv->rx_buf) {
		UART_ERR("hal_malloc_coherent rx_buf_len failed!\n");
		goto err0;
	}

	memset(uart_priv->rx_buf, 0xff, uart_priv->uart_config->rx_buf_len);
	hal_dcache_clean_invalidate((unsigned long)uart_priv->rx_buf,
					uart_priv->uart_config->rx_buf_len);

	/* request dma chan */
	ret = hal_dma_chan_request(&uart_priv->rx_dma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		UART_ERR("[uart_dma_start] dma channel busy!\n");
		goto err1;
	}

	/* register dma callback */
	ret = hal_dma_callback_install(uart_priv->rx_dma, rx_uart_dma_callback, uart_priv);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_start] register dma callback failed!");
		goto err2;
	}

	memset(&uart_priv->rx_slave_config, 0, sizeof(uart_priv->rx_slave_config));

	sunxi_desc->is_bmode = 1;
	sunxi_desc->is_timeout = 1;
	sunxi_desc->timeout_steps = 100;
	sunxi_desc->timeout_fun = 0x0;
	sunxi_desc->callback = rx_uart_dma_callback;
	sunxi_desc->callback_param = uart_priv;

	uart_priv->rx_dma->extend_desc = sunxi_desc;

	uart_priv->rx_slave_config.direction = DMA_DEV_TO_MEM;
	uart_priv->rx_slave_config.src_addr = (unsigned long)(uart_msg[uart_port].base + UART_RBR);
	uart_priv->rx_slave_config.dst_addr = (long unsigned int)uart_priv->rx_buf;
	uart_priv->rx_slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	uart_priv->rx_slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	uart_priv->rx_slave_config.dst_maxburst = DMA_SLAVE_BURST_1;
	uart_priv->rx_slave_config.src_maxburst = DMA_SLAVE_BURST_1;
	uart_priv->rx_slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM, uart_msg[uart_port].drqsrc_rx);
#ifdef CONFIG_DRIVERS_NDMA
	uart_priv->rx_slave_config.conti_mode = 1;
	uart_priv->rx_slave_config.bc_mode = 1;
#endif

	ret = hal_dma_slave_config(uart_priv->rx_dma, &uart_priv->rx_slave_config);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_start] uart_dma config error, ret:%d\n", ret);
		goto err2;
	}

	rx_ring->head = 0;
	rx_ring->tail = 0;
	rx_ring->len = uart_priv->uart_config->rx_buf_len;
	rx_ring->buf = (uint8_t *)malloc(rx_ring->len);
	if (!rx_ring->buf) {
		UART_ERR("malloc rx_ring buf errot\n");
		goto err3;
	}

	memset(rx_ring->buf, 0x0, rx_ring->len);
	UART_INFO("rx_ring->buf:0x%lx\n", (unsigned long)rx_ring->buf);
	uart_priv->rx_tmpnum = 0;

	ret = hal_dma_prep_cyclic(uart_priv->rx_dma, uart_priv->rx_slave_config.dst_addr,
				uart_priv->uart_config->rx_buf_len,
				uart_priv->uart_config->uart_rx_trigger_size,
				uart_priv->rx_slave_config.direction);

	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_start] dma prep error, ret:%d\n", ret);
		goto err3;
	}

	return HAL_UART_STATUS_OK;

err3:
	free(rx_ring->buf);
	rx_ring->buf = NULL;
err2:
	hal_dma_chan_free(uart_priv->rx_dma);
err1:
	hal_free_coherent(uart_priv->rx_buf);
err0:
	hal_sem_delete(uart_priv->hal_rxsem);
	uart_priv->rx_buf = NULL;
	UART_ERR("[uart_dma_start]recv_dma init failed!\n");
	return HAL_UART_STATUS_ERROR;
}

/* init and start the dma channel for uart_dma mode */
hal_uart_status_t hal_uart_dma_start(uart_port_t uart_port)
{
	int ret;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];

	if (uart_priv->rx_dma_inited) {
		UART_INFO("uart-%d dma rx is start ok, not need again\n", uart_port);
		return HAL_UART_STATUS_OK;
	}

	uart_priv->rx_dma_inited = 1;

	ret = hal_uart_init_rx_dma(uart_port);
	if (ret) {
		UART_ERR("[uart_dma_start]recv_dma init failed!\n");
		goto ERR;
	}

	ret = hal_dma_start(uart_priv->rx_dma);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_start] dma start error, ret:%d\n", ret);
		goto ERR;
	}

	UART_INFO("[UART_DEBUG]rx_dma chan_counnt:%d\n", uart_priv->rx_dma->chan_count);
	return HAL_UART_STATUS_OK;

ERR:
	uart_priv->rx_dma_inited = 0;
	return HAL_UART_STATUS_ERROR;
}

hal_uart_status_t hal_uart_dma_send(uart_port_t uart_port, const uint8_t *temp_buf, uint32_t tx_buf_len)
{
	int ret, hal_sem_ret;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];

	if (temp_buf == NULL) {
		UART_ERR("[uart_dma_tx]temp_buf is NULL!\n");
		return -1;
	}

	if (uart_priv->tx_dma_inited) {
		UART_ERR("uart-%d tx dma is using\n", uart_port);
		return -1;
	}

	uart_priv->hal_sem = hal_sem_create(0);
	if (!uart_priv->hal_sem) {
		UART_ERR("uart-%d create sem failed\n", uart_port);
		return -1;
	}

	uart_priv->tx_dma_inited = 1;

	uart_priv->tx_buf = hal_malloc_coherent(tx_buf_len);
	if (!uart_priv->tx_buf) {
		UART_ERR("hal_malloc_coherent tx_buf_len failed!\n");
		goto err0;
	}

	memset(uart_priv->tx_buf, 0, tx_buf_len);
	memcpy(uart_priv->tx_buf, temp_buf, tx_buf_len);

	UART_INFO("temp_buf:0x%lx tx_buf:0x%lx \n", (unsigned long)temp_buf, (unsigned long)uart_priv->tx_buf);

	hal_dcache_clean_invalidate((unsigned long)uart_priv->tx_buf, tx_buf_len);

	/* request dma chan */
	ret = hal_dma_chan_request(&uart_priv->tx_dma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		UART_ERR("[uart_dma_tx]dma channel busy!");
		goto err1;
	}

	/* register dma callback */
	ret = hal_dma_callback_install(uart_priv->tx_dma, tx_uart_dma_callback, uart_priv);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_tx]register dma callback failed!");
		goto err2;
	}

	memset(&uart_priv->tx_slave_config, 0, sizeof(uart_priv->tx_slave_config));
	uart_priv->tx_slave_config.direction = DMA_MEM_TO_DEV;
	uart_priv->tx_slave_config.dst_addr = (unsigned long)(uart_msg[uart_port].base + UART_THR);
	uart_priv->tx_slave_config.src_addr = (long unsigned int)uart_priv->tx_buf;
	uart_priv->tx_slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	uart_priv->tx_slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	uart_priv->tx_slave_config.dst_maxburst = DMA_SLAVE_BURST_1;
	uart_priv->tx_slave_config.src_maxburst = DMA_SLAVE_BURST_1;
	uart_priv->tx_slave_config.slave_id = sunxi_slave_id(uart_msg[uart_port].drqdst_tx, DRQSRC_SDRAM);

	ret = hal_dma_slave_config(uart_priv->tx_dma, &uart_priv->tx_slave_config);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_tx]uart_dma config error, ret:%d\n", ret);
		goto err2;
	}

	ret = hal_dma_prep_device(uart_priv->tx_dma, uart_priv->tx_slave_config.dst_addr,
					uart_priv->tx_slave_config.src_addr, tx_buf_len,
					uart_priv->tx_slave_config.direction);

	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_tx]dma prep error, ret:%d\n", ret);
		goto err2;
	}

	ret = hal_dma_start(uart_priv->tx_dma);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_tx]dma start error, ret:%d\n", ret);
		goto err2;
	}

	hal_dma_tx_status(uart_priv->tx_dma, &uart_priv->tx_size);

	hal_sem_ret = hal_sem_timedwait(uart_priv->hal_sem, 50000);
	if (hal_sem_ret)
		UART_ERR("wait sem error\n");

	hal_sem_delete(uart_priv->hal_sem);

	ret = hal_dma_stop(uart_priv->tx_dma);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_tx]dma stop error, ret:%d\n", ret);
		goto err2;
	}

	ret = hal_dma_chan_free(uart_priv->tx_dma);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_tx]dma free error, ret:%d\n", ret);
		goto err1;
	}

	hal_free_coherent(uart_priv->tx_buf);
	uart_priv->tx_dma_inited = 0;
	UART_INFO("[uart_dma_tx]transfer successful!\n");

	return HAL_UART_STATUS_OK;

err2:
	hal_dma_chan_free(uart_priv->tx_dma);
err1:
	hal_free_coherent(uart_priv->tx_buf);
err0:
	uart_priv->tx_dma_inited = 0;
	hal_sem_delete(uart_priv->hal_sem);
	UART_ERR("[uart_dma_tx]transfer failed!\n");
	return HAL_UART_STATUS_ERROR;
}

uint32_t hal_uart_dma_recv(uart_port_t uart_port, uint8_t *usr_buf, uint32_t usr_buf_len)
{
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uart_ring_buf_t *rx_ring = &(uart_priv->ring_buf);
	int out_len = uart_priv->rx_tmpnum;

	rx_ring->len = uart_priv->uart_config->rx_buf_len;

	if (usr_buf == NULL) {
		UART_ERR("[uart_dma_rx]usr_buf is NULL!\n");
		return -1;
	}

	if (!uart_priv->dma_used) {
		UART_ERR("[uart_dma_rx]not used dma\n");
		return -1;
	}

	UART_INFO("usr_buf:0x%lx rx_buf:0x%lx \n", (unsigned long)usr_buf, (unsigned long)uart_priv->rx_buf) ;

	if (usr_buf_len < out_len) {
		memcpy(usr_buf, rx_ring->buf, usr_buf_len);
		UART_ERR("The input parameter:%d is less than the received data: %d, resulting in data loss\n", usr_buf_len, out_len);
		return usr_buf_len;
	}

	memcpy(usr_buf, rx_ring->buf, out_len);

	uart_priv->rx_tmpnum = 0;

	return out_len;
}

uint32_t hal_uart_dma_recv_wait(uart_port_t uart_port, uint8_t *usr_buf, uint32_t usr_buf_len, int32_t timeout)
{
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uart_ring_buf_t *rx_ring = &(uart_priv->ring_buf);
	int out_len = 0;
	uint32_t size = 0, total = 0, count = 0;
	unsigned long flags;
	static u8 cnt = 0;

	rx_ring->len = uart_priv->uart_config->rx_buf_len;

	if (usr_buf == NULL) {
		UART_ERR("[uart_dma_rx]usr_buf is NULL!\n");
		return -1;
	}

	if (!uart_priv->dma_used) {
		UART_ERR("[uart_dma_rx]not used dma\n");
		return -1;
	}

	UART_INFO("usr_buf:0x%lx rx_buf:0x%lx \n", (unsigned long)usr_buf, (unsigned long)uart_priv->rx_buf) ;

	if (uart_priv->rx_tmpnum == 0) {
		hal_sem_timedwait(uart_priv->hal_rxsem, timeout);
	}

	if (uart_priv->rx_tmpnum == 0) {
		hal_dma_tx_status(uart_priv->rx_dma, &size);
		rx_ring->head = uart_priv->uart_config->rx_buf_len - size;
		total = CIRC_CNT(rx_ring->head, rx_ring->tail, rx_ring->len);
		if (total) {
			cnt ++;
			if ((OSTICK_TO_MS(timeout) * cnt) < RX_DMAIRQ_QUIET_ACCEPT_TIME)
				goto out;
			cnt = 0;
			hal_dcache_invalidate((unsigned long)uart_priv->rx_buf, uart_priv->uart_config->rx_buf_len);
			count = CIRC_CNT_TO_END(rx_ring->head, rx_ring->tail, rx_ring->len);
			out_len = (usr_buf_len < total) ? usr_buf_len : total;
			count = (usr_buf_len < count) ? usr_buf_len : count;
			memcpy(usr_buf, uart_priv->rx_buf + rx_ring->tail, count);
			if (out_len > count) {
				memcpy(usr_buf + count, uart_priv->rx_buf, out_len - count);
			}
			flags = hal_interrupt_disable_irqsave();
			rx_ring->tail = (rx_ring->tail + total) & (rx_ring->len - 1);
			hal_interrupt_enable_irqrestore(flags);
		} else {
			cnt = 0;
		}
		goto out;
	} else {
		cnt = 0;
	}

	out_len = uart_priv->rx_tmpnum;
	uart_priv->rx_tmpnum = 0;

	if (usr_buf_len < out_len) {
		memcpy(usr_buf, rx_ring->buf, usr_buf_len);
		UART_ERR("The input parameter:%d is less than the received data: %d, resulting in data loss\n", usr_buf_len, out_len);
		return usr_buf_len;
	}

	memcpy(usr_buf, rx_ring->buf, out_len);

out:
	return out_len;
}

hal_uart_status_t hal_uart_dma_stop(uart_port_t uart_port)
{
	int ret;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uart_ring_buf_t *rx_ring = &(uart_priv->ring_buf);

	ret = hal_dma_stop(uart_priv->rx_dma);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_stop] dma stop error, ret:%d", ret);
		goto ERR;
	}

	ret = hal_dma_chan_free(uart_priv->rx_dma);
	if (ret != HAL_DMA_STATUS_OK) {
		UART_ERR("[uart_dma_stop] dma free error, ret:%d", ret);
		goto ERR;
	}

	hal_free_coherent(uart_priv->rx_buf);
	hal_sem_delete(uart_priv->hal_rxsem);
	UART_INFO("hal_uart_dma_stop end!\n");
	uart_priv->rx_buf = NULL;
	free(rx_ring->buf);
	rx_ring->buf = NULL;
	uart_priv->rx_dma_inited = 0;

	return HAL_UART_STATUS_OK;

ERR:
	return HAL_UART_STATUS_ERROR;
}

#endif
#ifdef CONFIG_SUNXI_UART_SUPPORT_POLL

static poll_wakeup_func uart_drv_poll_wakeup = NULL;

int32_t hal_uart_check_poll_state(int32_t dev_id, short key)
{
	int ret = -1;
	int32_t mask = 0;

	if (key & POLLIN)
	{
		ret = hal_is_mailbox_empty((hal_mailbox_t)uart_mailbox[dev_id]);
		if (ret == 1)
		{
			mask = 0;
		}
		else
		{
			mask |= POLLIN;
		}
	}

	if (key & POLLOUT)
	{
		mask |= POLLOUT;
	}
	return mask;
}

int32_t hal_uart_poll_wakeup(int32_t dev_id, short key)
{
	int ret = -1;

	if (uart_drv_poll_wakeup)
	{
		ret = uart_drv_poll_wakeup(dev_id, key);
	}

	return ret;
}

int32_t hal_uart_register_poll_wakeup(poll_wakeup_func poll_wakeup)
{
	uart_drv_poll_wakeup = poll_wakeup;

	return 0;
}

#endif

static bool uart_port_is_valid(uart_port_t uart_port)
{
	return (uart_port < UART_MAX);
}

static bool uart_config_is_valid(const _uart_config_t *config)
{
	return ((config->baudrate < UART_BAUDRATE_MAX) &&
			(config->word_length <= UART_WORD_LENGTH_8) &&
			(config->stop_bit <= UART_STOP_BIT_2) &&
			(config->parity <= UART_PARITY_EVEN));
}

sunxi_hal_version_t hal_uart_get_version(int32_t dev)
{
	HAL_ARG_UNUSED(dev);
	return hal_uart_driver;
}

sunxi_hal_uart_capabilities_t hal_uart_get_capabilities(int32_t dev)
{
	HAL_ARG_UNUSED(dev);
	return driver_capabilities;
}

static void uart_set_format(uart_port_t uart_port, uart_word_length_t word_length,
		uart_stop_bit_t stop_bit, uart_parity_t parity)
{
	unsigned long irq_flags;

	const unsigned long uart_base = uart_msg[uart_port].base;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uint32_t value;

	irq_flags = hal_spin_lock_irqsave(&uart_priv->spinlock);

	value = hal_readb(uart_base + UART_LCR);

	/* set word length */
	value &= ~(UART_LCR_DLEN_MASK);
	switch (word_length)
	{
		case UART_WORD_LENGTH_5:
			value |= UART_LCR_WLEN5;
			break;
		case UART_WORD_LENGTH_6:
			value |= UART_LCR_WLEN6;
			break;
		case UART_WORD_LENGTH_7:
			value |= UART_LCR_WLEN7;
			break;
		case UART_WORD_LENGTH_8:
		default:
			value |= UART_LCR_WLEN8;
			break;
	}

	/* set stop bit */
	switch (stop_bit)
	{
		case UART_STOP_BIT_1:
		default:
			value &= ~(UART_LCR_STOP);
			break;
		case UART_STOP_BIT_2:
			value |= UART_LCR_STOP;
			break;
	}

	/* set parity bit */
	value &= ~(UART_LCR_PARITY_MASK);
	switch (parity)
	{
		case UART_PARITY_NONE:
			value &= ~(UART_LCR_PARITY);
			break;
		case UART_PARITY_ODD:
			value |= UART_LCR_PARITY;
			break;
		case UART_PARITY_EVEN:
			value |= UART_LCR_PARITY;
			value |= UART_LCR_EPAR;
			break;
	}

	uart_priv->lcr = value;
	hal_writeb(uart_priv->lcr, uart_base + UART_LCR);

	hal_spin_unlock_irqrestore(&uart_priv->spinlock, irq_flags);
}

#if defined(CONFIG_ARCH_SUN8IW18P1) || defined(CONFIG_STANDBY)

#define CCM_UART_RST_OFFSET       (16)
#define CCM_UART_GATING_OFFSET    (0)
static void uart_reset(uart_port_t uart_port)
{
	u32 reset_id = uart_msg[uart_port].rst_id;
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	struct reset_control *reset;

#ifdef SYS_UART_NUM
	if (uart_port > SYS_UART_NUM)
		reset_type = HAL_SUNXI_R_RESET;
#endif

       reset = hal_reset_control_get(reset_type, reset_id);
       hal_reset_control_reset(reset);
}
#endif

static void uart_set_baudrate(uart_port_t uart_port, uart_baudrate_t baudrate)
{
	const unsigned long uart_base = uart_msg[uart_port].base;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uint32_t actual_baudrate = g_uart_baudrate_map[baudrate];
	uint32_t quot, uart_clk;

	uart_clk = 24000000; /* FIXME: fixed to 24MHz */

	hal_disable_irq(uart_priv->irqn);

#ifdef UART_SCLK
	hal_clk_t clk;
	clk = hal_clock_get(HAL_SUNXI_AON_CCU, UART_SCLK);
	if (!clk)
		UART_ERR("uart%d fail to get SCLK\n", uart_port);
	uart_clk = hal_clk_get_rate(clk);
	if (!uart_clk) {
		UART_ERR("uart%d fail to get SCLK rate, use 24M\n", uart_port);
		uart_clk = 24000000; /* FIXME: fixed to 24MHz */
	}
#endif

	quot = (uart_clk + 8 * actual_baudrate) / (16 * actual_baudrate);

	UART_INFO("baudrate: %d, quot = %d\r\n", actual_baudrate, quot);

	uart_priv->dlh = quot >> 8;
	uart_priv->dll = quot & 0xff;

	/* hold tx so that uart will update lcr and baud in the gap of tx */
	hal_writeb(UART_HALT_HTX | UART_HALT_FORCECFG, uart_base + UART_HALT);
	hal_writeb(uart_priv->lcr | UART_LCR_DLAB, uart_base + UART_LCR);
	hal_writeb(uart_priv->dlh, uart_base + UART_DLH);
	hal_writeb(uart_priv->dll, uart_base + UART_DLL);
	hal_writeb(UART_HALT_HTX | UART_HALT_FORCECFG | UART_HALT_LCRUP, uart_base + UART_HALT);
	/* FIXME: implement timeout */
	while (hal_readb(uart_base + UART_HALT) & UART_HALT_LCRUP)
		;

	/* In fact there are two DLABs(DLAB and DLAB_BAK) in the hardware implementation.
	 * The DLAB_BAK is sellected only when SW_UART_HALT_FORCECFG is set to 1,
	 * and this bit can be access no matter uart is busy or not.
	 * So we select the DLAB_BAK always by leaving SW_UART_HALT_FORCECFG to be 1. */
	hal_writeb(uart_priv->lcr, uart_base + UART_LCR);
	hal_writeb(UART_HALT_FORCECFG, uart_base + UART_HALT);

	hal_enable_irq(uart_priv->irqn);
}

static void uart_set_fifo(uart_port_t uart_port, uint32_t value)
{
	const unsigned long uart_base = uart_msg[uart_port].base;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];

	uart_priv->fcr = value;
	hal_writeb(uart_priv->fcr, uart_base + UART_FCR);
}

void hal_uart_set_hardware_flowcontrol(uart_port_t uart_port)
{
	unsigned long irq_flags;
	const unsigned long uart_base = uart_msg[uart_port].base;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uint32_t value;

	if (!uart_priv->is_inited) {
		return;
	}
	irq_flags = hal_spin_lock_irqsave(&uart_priv->spinlock);

	value = hal_readb(uart_base + UART_MCR);
	value |= UART_MCR_DTR | UART_MCR_RTS | UART_MCR_AFE;
	uart_priv->mcr = value;
	hal_writeb(uart_priv->mcr, uart_base + UART_MCR);

	/* enable with modem status interrupts */
	value = hal_readb(uart_base + UART_IER);
	value |= UART_IER_MSI;
	uart_priv->ier = value;
	hal_writeb(uart_priv->ier, uart_base + UART_IER);

	hal_spin_unlock_irqrestore(&uart_priv->spinlock, irq_flags);
}

void hal_uart_disable_flowcontrol(uart_port_t uart_port)
{
	unsigned long irq_flags;
	const unsigned long uart_base = uart_msg[uart_port].base;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uint32_t value;

	if (!uart_priv->is_inited) {
		return;
	}
	irq_flags = hal_spin_lock_irqsave(&uart_priv->spinlock);

	value = hal_readb(uart_base + UART_MCR);
	value &= ~(UART_MCR_DTR | UART_MCR_RTS | UART_MCR_AFE);
	uart_priv->mcr = value;
	hal_writeb(uart_priv->mcr, uart_base + UART_MCR);

	/* disable with modem status interrupts */
	value = hal_readb(uart_base + UART_IER);
	value &= ~(UART_IER_MSI);
	uart_priv->ier = value;
	hal_writeb(uart_priv->ier, uart_base + UART_IER);

	hal_spin_unlock_irqrestore(&uart_priv->spinlock, irq_flags);
}

static void uart_force_idle(uart_port_t uart_port)
{
	const unsigned long uart_base = uart_msg[uart_port].base;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];

	if (uart_priv->fcr & UART_FCR_FIFO_EN)
	{
		hal_writeb(UART_FCR_FIFO_EN, uart_base + UART_FCR);
		hal_writeb(UART_FCR_TXFIFO_RST
				| UART_FCR_RXFIFO_RST
				| UART_FCR_FIFO_EN, uart_base + UART_FCR);
		hal_writeb(0, uart_base + UART_FCR);
	}

	hal_writeb(uart_priv->fcr, uart_base + UART_FCR);
	(void)hal_readb(uart_base + UART_FCR);
}

static void uart_handle_busy(uart_port_t uart_port)
{
	const unsigned long uart_base = uart_msg[uart_port].base;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];

	(void)hal_readb(uart_base + UART_USR);

	/*
	 * Before reseting lcr, we should ensure than uart is not in busy
	 * state. Otherwise, a new busy interrupt will be introduced.
	 * It is wise to set uart into loopback mode, since it can cut down the
	 * serial in, then we should reset fifo(in my test, busy state
	 * (UART_USR_BUSY) can't be cleard until the fifo is empty).
	 */
	hal_writeb(uart_priv->mcr | UART_MCR_LOOP, uart_base + UART_MCR);
	uart_force_idle(uart_port);
	hal_writeb(uart_priv->lcr, uart_base + UART_LCR);
	hal_writeb(uart_priv->mcr, uart_base + UART_MCR);
}

static uint32_t uart_handle_rx(uart_port_t uart_port, uint32_t lsr)
{
	const unsigned long uart_base = uart_msg[uart_port].base;
	uint8_t ch = 0;

	UART_INFO("IRQ uart%d handle rx \n", uart_port);
	do
	{
		if (lsr & UART_LSR_DR)
		{
			ch = hal_readb(uart_base + UART_RBR);
			if (!ch && (lsr & UART_LSR_BRK_ERROR_BITS) && (lsr & UART_LSR_BI))
				goto ignore_char;
			if (uart_mailbox[uart_port] != NULL)
				hal_mailbox_send((hal_mailbox_t)uart_mailbox[uart_port], ch);
			else
				UART_ERR("uart %d mailbox is null\n", uart_port);
#ifdef CONFIG_SUNXI_UART_SUPPORT_POLL
			hal_uart_poll_wakeup(uart_port, POLLIN);
#endif
		}
ignore_char:
		lsr = hal_readb(uart_base + UART_LSR);
	} while ((lsr & (UART_LSR_DR | UART_LSR_BI)));

	return lsr;
}


static hal_irqreturn_t uart_irq_handler(void *dev_id)
{
	uart_priv_t *uart_priv = dev_id;
	uart_port_t uart_port = uart_priv->uart_port;
	const unsigned long uart_base = uart_msg[uart_port].base;
	uint32_t iir, lsr;

	iir = hal_readb(uart_base + UART_IIR) & UART_IIR_IID_MASK;
	lsr = hal_readb(uart_base + UART_LSR);

	if (iir == UART_IIR_IID_NOIRQ)
		return 1;

	UART_INFO_IRQ("IRQ uart%d lsr is %08x \n", uart_port, lsr);
	if (iir == UART_IIR_IID_BUSBSY)
	{
		uart_handle_busy(uart_port);
	}
	else
	{
		if (lsr & (UART_LSR_DR | UART_LSR_BI))
		{
			lsr = uart_handle_rx(uart_port, lsr);
		}
		else if (iir & UART_IIR_IID_CHARTO)
			/* has charto irq but no dr lsr? just read and ignore */
		{
			hal_readb(uart_base + UART_RBR);
		}

		/* if (lsr & UART_LSR_THRE)
		   {
		   uart_handle_tx(uart_port);
		   }*/
	}
	return 0;
}

static void uart_enable_irq(uart_port_t uart_port, uint32_t irq_type)
{
	unsigned long irq_flags;
	const unsigned long uart_base = uart_msg[uart_port].base;
	uint32_t value;

	irq_flags = hal_spin_lock_irqsave(&g_uart_priv[uart_port].spinlock);

	value = hal_readb(uart_base + UART_IER);
	value |= irq_type;
	hal_writeb(value, uart_base + UART_IER);

	hal_spin_unlock_irqrestore(&g_uart_priv[uart_port].spinlock, irq_flags);
}

static void uart_enable_busy_cfg(uart_port_t uart_port)
{
	unsigned long irq_flags;
	const unsigned long uart_base = uart_msg[uart_port].base;
	uint32_t value;

	irq_flags = hal_spin_lock_irqsave(&g_uart_priv[uart_port].spinlock);

	value = hal_readb(uart_base + UART_HALT);
	value |= UART_HALT_FORCECFG;
	hal_writeb(value, uart_base + UART_HALT);

	hal_spin_unlock_irqrestore(&g_uart_priv[uart_port].spinlock, irq_flags);
}

static inline int is_uart_tx_fifo_empty(uart_port_t uart_port)
{
	volatile uint32_t *status = (uint32_t *)(uart_msg[uart_port].base + UART_USR);
	return !!(*status & UART_USR_TFE);
}

static int uart_clk_init(int bus, bool enable)
{
/* @TODO:delete CONFIG_ARCH_SUN60IW1 atfer clock adaptation complete */
#if defined(CONFIG_ARCH_SUN60IW1) || defined (CONFIG_ARCH_ARM_CORTEX_A55)
	if (enable) {
		hal_writel(0 ,0x0701018c);
		hal_writel(0x00010001, 0x0701018c);
	}
#else
	hal_clk_status_t ret;
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	u32  reset_id;
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_id_t clk_id;
	hal_clk_t clk;
	struct reset_control *reset;

	clk_id = uart_msg[bus].clk_id;
	reset_id = uart_msg[bus].rst_id;
	reset_type = HAL_SUNXI_RESET;
	clk_type = HAL_SUNXI_CCU;

#ifdef SYS_UART_NUM
	if (bus > SYS_UART_NUM) {
		reset_type = HAL_SUNXI_R_RESET;
		clk_type = HAL_SUNXI_R_CCU;
	}
#endif

	if (enable)
	{
		reset = hal_reset_control_get(reset_type, reset_id);
		hal_reset_control_deassert(reset);
		hal_reset_control_put(reset);

		clk = hal_clock_get(clk_type, clk_id);
		if (!clk) {
			UART_ERR("uart%d fail to get bus clk\n", bus);
			return -1;
		}
		ret = hal_clock_enable(clk);
		if (ret)
		{
			UART_ERR("[uart%d] couldn't enable clk!\n", bus);
			return -1;
		}
	}
	else
	{
		clk = hal_clock_get(clk_type, clk_id);
		if (!clk) {
			UART_ERR("uart%d fail to get bus clk\n", bus);
			return -1;
		}
		ret = hal_clock_disable(clk);
		if (ret)
		{
			UART_ERR("[uart%d] couldn't disable clk!\n", bus);
			return -1;
		}
	}

#ifdef UART_PCLK
/* We need wait TX FIFO empty before change UART clock frequency. For 9600 baudrate,
 * 64Byte TX FIFO will be empty after 0.053s(64 * 8 / 9600), so we wait 100ms at most. */
#define MAX_FIFO_EMPTY_CHECK_TIMES 100
	uint32_t check_times = 0;
	while (!is_uart_tx_fifo_empty(bus))
	{
		check_times++;
		hal_udelay(1000);
		if (check_times >= MAX_FIFO_EMPTY_CHECK_TIMES)
			break;
	}

	hal_clk_t pclk;
	clk = hal_clock_get(HAL_SUNXI_CCU, UART_SCLK);
	if (!clk) {
		UART_ERR("uart%d fail to get SCLK\n", bus);
		return -1;
	}
	pclk = hal_clock_get(HAL_SUNXI_AON_CCU, UART_PCLK);
	if (!pclk) {
		UART_ERR("uart%d fail to get PCLK\n", bus);
		return -1;
	}
	ret = hal_clk_set_parent(clk, pclk);
	if (ret != HAL_CLK_STATUS_OK) {
		UART_ERR("uart%d fail to setparent\n", bus);
		return -1;
	}
#endif

#endif
	return 0;
}

#ifdef CONFIG_DRIVER_SYSCONFIG
static int uart_pinctrl_init_by_sys_config(uart_port_t uart_port)
{
	user_gpio_set_t gpio_cfg[4];
	int count, i;
	char uart_name[16];
	gpio_pin_t uart_pin[4];
	gpio_muxsel_t uart_muxsel[4];

	memset(gpio_cfg, 0, sizeof(gpio_cfg));
	snprintf(uart_name, sizeof(uart_name), "uart%d", uart_port);
	count = hal_cfg_get_gpiosec_keycount(uart_name);
	if (!count)
	{
		return -1;
	}
	hal_cfg_get_gpiosec_data(uart_name, gpio_cfg, count);

	for (i = 0; i < count; i++)
	{
		uart_pin[i] = (gpio_cfg[i].port - 1) * 32 + gpio_cfg[i].port_num;
		uart_muxsel[i] = gpio_cfg[i].mul_sel;

		hal_gpio_pinmux_set_function(uart_pin[i], uart_muxsel[i]);
		hal_gpio_set_pull(uart_pin[i], GPIO_PULL_UP);
	}
	return 0;
}
#endif

static int uart_pinctrl_init_by_build_in_config(uart_port_t uart_port)
{
	hal_gpio_pinmux_set_function(uart_msg[uart_port].uart_tx, uart_msg[uart_port].func);//TX
	hal_gpio_pinmux_set_function(uart_msg[uart_port].uart_rx, uart_msg[uart_port].func);//RX
	hal_gpio_set_pull(uart_msg[uart_port].uart_tx, GPIO_PULL_UP);
	hal_gpio_set_pull(uart_msg[uart_port].uart_rx, GPIO_PULL_UP);
	switch (uart_port) {
	case UART_1:
#ifdef  CONFIG_ARCH_SUN8IW18P1
#ifdef UART1_RTX
		hal_gpio_pinmux_set_function(UART1_RTX, uart_msg[uart_port].func);
#endif
#ifdef UART1_CTX
		hal_gpio_pinmux_set_function(UART1_CTX, uart_msg[uart_port].func);
#endif
		break;
#endif
	case UART_2:
#ifdef  UART2_RTS
		hal_gpio_pinmux_set_function(UART2_RTS, uart_msg[uart_port].func);
#endif
#ifdef  UART2_CTS
		hal_gpio_pinmux_set_function(UART2_CTS, uart_msg[uart_port].func);
#endif
		break;
	default:
		break;
	}

	return 0;
}

static int uart_pinctrl_init(uart_port_t uart_port)
{
	int ret;

#ifdef CONFIG_DRIVER_SYSCONFIG
	ret = uart_pinctrl_init_by_sys_config(uart_port);
	if (!ret)
		return 0;
	UART_ERR("[uart%d] not support in sys_config, try build in config\n", uart_port);
#endif
	ret = uart_pinctrl_init_by_build_in_config(uart_port);
	if (!ret)
		return 0;
	UART_ERR("[uart%d] uart_pinctrl_init failed!\n", uart_port);

	return ret;
}

#ifdef CONFIG_DRIVER_SYSCONFIG
static int uart_pinctrl_uninit_by_sys_config(uart_port_t uart_port)
{
	user_gpio_set_t gpio_cfg[4];
	int count, i;
	char uart_name[16];
	gpio_pin_t uart_pin[4];

	memset(gpio_cfg, 0, sizeof(gpio_cfg));
	snprintf(uart_name, sizeof(uart_name), "uart%d", uart_port);
	count = hal_cfg_get_gpiosec_keycount(uart_name);
	if (!count)
	{
		return -1;
	}
	hal_cfg_get_gpiosec_data(uart_name, gpio_cfg, count);

	for (i = 0; i < count; i++)
	{
		uart_pin[i] = (gpio_cfg[i].port - 1) * 32 + gpio_cfg[i].port_num;
		hal_gpio_pinmux_set_function(uart_pin[i], GPIO_MUXSEL_DISABLED);
		hal_gpio_set_pull(uart_pin[i], GPIO_PULL_DOWN_DISABLED);
	}
	return 0;
}
#endif

static int uart_pinctrl_uninit_by_build_in_config(uart_port_t uart_port)
{
	hal_gpio_pinmux_set_function(uart_msg[uart_port].uart_tx, GPIO_MUXSEL_DISABLED);//TX
	hal_gpio_pinmux_set_function(uart_msg[uart_port].uart_rx, GPIO_MUXSEL_DISABLED);//RX
	hal_gpio_set_pull(uart_msg[uart_port].uart_tx, GPIO_PULL_DOWN_DISABLED);
	hal_gpio_set_pull(uart_msg[uart_port].uart_rx, GPIO_PULL_DOWN_DISABLED);
	switch (uart_port) {
	case UART_1:
#ifdef  CONFIG_ARCH_SUN8IW18P1
#ifdef UART1_RTX
		hal_gpio_pinmux_set_function(UART1_RTX, GPIO_PULL_DOWN_DISABLED);
#endif
#ifdef UART1_CTX
		hal_gpio_pinmux_set_function(UART1_CTX, GPIO_PULL_DOWN_DISABLED);
#endif
		break;
#endif
	case UART_2:
#ifdef  UART2_RTS
		hal_gpio_pinmux_set_function(UART2_RTS, GPIO_PULL_DOWN_DISABLED);
#endif
#ifdef  UART2_CTS
		hal_gpio_pinmux_set_function(UART2_CTS, GPIO_PULL_DOWN_DISABLED);
#endif
		break;
	default:
		break;
	}
	return 0;
}

static int uart_pinctrl_uninit(uart_port_t uart_port)
{
	int ret;

#ifdef CONFIG_DRIVER_SYSCONFIG
	ret = uart_pinctrl_uninit_by_sys_config(uart_port);
	if (!ret)
		return 0;
	UART_ERR("[uart%d] not support in sys_config, try build in config\n", uart_port);
#endif
	ret = uart_pinctrl_uninit_by_build_in_config(uart_port);
	if (!ret)
		return 0;

	UART_ERR("[uart%d] uart_pinctrl_uninit failed!\n", uart_port);
	return ret;
}

/* default uart config */
_uart_config_t uart_defconfig =
{
#ifdef CONFIG_CLI_UART_BAUD_1500000
	.baudrate		= UART_BAUDRATE_1500000,
#else
	.baudrate		= UART_BAUDRATE_115200,
#endif
	.word_length		= UART_WORD_LENGTH_8,
	.stop_bit		= UART_STOP_BIT_1,
	.parity			= UART_PARITY_NONE,
#ifdef CONFIG_UART_DMA
	.uart_rx_trigger_size   = RX_TRIGGER_SIZE,
	.rx_buf_len		= RX_BUF_LEN,
	.callback		= NULL,
#endif
};

#ifdef CONFIG_STANDBY
static int uart_suspend(void *data)
{
	int32_t uart_port = (int32_t)data;

	hal_log_debug("uart%d suspend\r\n", uart_port);
	return 0;
}

static int uart_resume(void *data)
{
	int32_t uart_port = (int32_t)data;

	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uint32_t irqn = uart_msg[uart_port].irqn;
	uint32_t value = 0;
	const _uart_config_t *uart_config = &uart_defconfig;
	char uart_name[12] = {0};

	if ((!uart_port_is_valid(uart_port)) ||
			(!uart_config_is_valid(uart_config)))
	{
		hal_log_err("error parameter\r\n");
		return -1;
	}

	/* enable clk */
	uart_clk_init(uart_port, true);

	/* request gpio */
	uart_pinctrl_init(uart_port);

	/* config uart attributes */
	uart_set_format(uart_port, uart_config->word_length,
			uart_config->stop_bit, uart_config->parity);

	/* force reset controller to disable transfer */
	uart_reset(uart_port);

	uart_set_baudrate(uart_port, uart_config->baudrate);

	value |= UART_FCR_RXTRG_1_2 | UART_FCR_TXTRG_1_2 | UART_FCR_FIFO_EN;
	uart_set_fifo(uart_port, value);

	/* set uart IER */
	uart_enable_irq(uart_port, UART_IER_RDI | UART_IER_RLSI);

	/* force config */
	uart_enable_busy_cfg(uart_port);

	hal_log_debug("uart%d resume\r\n", uart_port);

	return 0;
}
#endif

void uart_multiplex_lpuart(uart_port_t uart_port)
{
	uart_pinctrl_init(uart_port);
	return;
}

void uart_multiplex_lpuart_nosuspend(uart_port_t uart_port)
{
	uart_pinctrl_init(uart_port);
	if (uart_port == 0)
		uart_msg[uart_port].no_suspend = true;
	return;
}

#ifdef CONFIG_COMPONENTS_PM
static int hal_uart_suspend(void *data, suspend_mode_t mode)
{
	uart_port_t uart_port = (uart_port_t)data;
	uint32_t irqn = uart_msg[uart_port].irqn;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];

	UART_INFO("uart %d suspend\n", uart_port);

	hal_disable_irq(irqn);
	uart_enable_irq(uart_port, 0);
	if (uart_msg[uart_port].no_suspend == false) {
		uart_pinctrl_uninit(uart_port);
	}
	uart_clk_init(uart_port, false);
#ifdef CONFIG_PM_STANDBY_MEMORY
	hal_mailbox_delete(uart_mailbox[uart_port]);
	uart_mailbox[uart_port] = NULL;
#endif
	uart_priv->is_inited = 0;

	return 0;
}

static void hal_uart_resume(void *data, suspend_mode_t mode)
{

	uart_port_t uart_port = (uart_port_t)data;
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uint32_t irqn = uart_msg[uart_port].irqn;
	uint32_t value = 0;
#ifdef CONFIG_PM_STANDBY_MEMORY
	uart_priv->uart_config = &uart_defconfig;
	char uart_name[12] = {0};
#endif
	const _uart_config_t *uart_config = uart_priv->uart_config;

	if ((!uart_port_is_valid(uart_port)) ||
			(!uart_config_is_valid(uart_config)))
	{
		hal_log_err("error parameter\r\n");
		return;
	}

	/* enable clk */
	uart_clk_init(uart_port, true);

	/* request gpio */
	uart_pinctrl_init(uart_port);

	/* config uart attributes */
	uart_set_format(uart_port, uart_config->word_length,
			uart_config->stop_bit, uart_config->parity);

#ifdef CONFIG_ARCH_SUN8IW18P1
	/* force reset controller to disable transfer */
	uart_reset(uart_port);
#endif

	uart_set_baudrate(uart_port, uart_config->baudrate);

	value |= UART_FCR_RXTRG_1_2 | UART_FCR_TXTRG_1_2 | UART_FCR_FIFO_EN;
	uart_set_fifo(uart_port, value);

	/* set uart IER */
	uart_enable_irq(uart_port, UART_IER_RDI | UART_IER_RLSI);

	/* force config */
	uart_enable_busy_cfg(uart_port);

#ifdef CONFIG_PM_STANDBY_MEMORY
	uart_priv->uart_port = uart_port;
	uart_priv->irqn = irqn;
#endif
	hal_enable_irq(irqn);

#ifdef CONFIG_PM_STANDBY_MEMORY
	if (uart_mailbox[uart_port] == NULL) {
		uart_mailbox[uart_port] = hal_mailbox_create(uart_name, UART_FIFO_SIZE);
		if (uart_mailbox[uart_port] == NULL)
			UART_ERR("%s create mailbox fail\n", __func__);
	}
#endif

	uart_priv->is_inited = 1;
	UART_INFO("uart%d resume\n", uart_port);
}

#ifdef CONFIG_ARCH_SUN20IW2
#define UART_PM_SYSCORE_OPS(__port__) \
static struct syscore_ops pm_uart_ops_##__port__ = { \
	.name = "sunxi_uart" #__port__, \
	.suspend = hal_uart_suspend, \
	.resume = hal_uart_resume, \
	.common_syscore = COMMON_SYSCORE, \
}
#else
#define UART_PM_SYSCORE_OPS(__port__) \
static struct syscore_ops pm_uart_ops_##__port__ = { \
	.name = "sunxi_uart" #__port__, \
	.suspend = hal_uart_suspend, \
	.resume = hal_uart_resume, \
}
#endif

#ifdef SUNXI_UART0_BASE
	UART_PM_SYSCORE_OPS(0);
#endif
#ifdef SUNXI_UART1_BASE
	UART_PM_SYSCORE_OPS(1);
#endif
#ifdef SUNXI_UART2_BASE
	UART_PM_SYSCORE_OPS(2);
#endif
#ifdef SUNXI_UART3_BASE
	UART_PM_SYSCORE_OPS(3);
#endif
#ifdef SUNXI_UART4_BASE
	UART_PM_SYSCORE_OPS(4);
#endif
#ifdef SUNXI_UART5_BASE
	UART_PM_SYSCORE_OPS(5);
#endif
#ifdef SUNXI_UART6_BASE
	UART_PM_SYSCORE_OPS(6);
#endif
#ifdef SUNXI_UART7_BASE
	UART_PM_SYSCORE_OPS(7);
#endif
#ifdef SUNXI_UART8_BASE
	UART_PM_SYSCORE_OPS(8);
#endif
#ifdef SUNXI_UART9_BASE
	UART_PM_SYSCORE_OPS(9);
#endif
#ifdef SUNXI_UART10_BASE
	UART_PM_SYSCORE_OPS(10);
#endif
#ifdef SUNXI_UART11_BASE
	UART_PM_SYSCORE_OPS(11);
#endif
#ifdef SUNXI_UART12_BASE
	UART_PM_SYSCORE_OPS(12);
#endif
#ifdef SUNXI_UART13_BASE
	UART_PM_SYSCORE_OPS(13);
#endif
#ifdef SUNXI_UART14_BASE
	UART_PM_SYSCORE_OPS(14);
#endif
#ifdef SUNXI_UART15_BASE
	UART_PM_SYSCORE_OPS(15);
#endif
#ifdef SUNXI_UART16_BASE
	UART_PM_SYSCORE_OPS(16);
#endif

static struct syscore_ops *pm_uart_ops[] = {
#ifdef SUNXI_UART0_BASE
	&pm_uart_ops_0,
#endif
#ifdef SUNXI_UART1_BASE
	&pm_uart_ops_1,
#endif
#ifdef SUNXI_UART2_BASE
	&pm_uart_ops_2,
#endif
#ifdef SUNXI_UART3_BASE
	&pm_uart_ops_3,
#endif
#ifdef SUNXI_UART4_BASE
	&pm_uart_ops_4,
#endif
#ifdef SUNXI_UART5_BASE
	&pm_uart_ops_5,
#endif
#ifdef SUNXI_UART6_BASE
	&pm_uart_ops_6,
#endif
#ifdef SUNXI_UART7_BASE
	&pm_uart_ops_7,
#endif
#ifdef SUNXI_UART8_BASE
	&pm_uart_ops_8,
#endif
#ifdef SUNXI_UART9_BASE
	&pm_uart_ops_9,
#endif
#ifdef SUNXI_UART10_BASE
	&pm_uart_ops_10,
#endif
#ifdef SUNXI_UART11_BASE
	&pm_uart_ops_11,
#endif
#ifdef SUNXI_UART12_BASE
	&pm_uart_ops_12,
#endif
#ifdef SUNXI_UART13_BASE
	&pm_uart_ops_13,
#endif
#ifdef SUNXI_UART14_BASE
	&pm_uart_ops_14,
#endif
#ifdef SUNXI_UART15_BASE
	&pm_uart_ops_15,
#endif
#ifdef SUNXI_UART16_BASE
	&pm_uart_ops_16,
#endif
};
#endif

int32_t hal_uart_init(int32_t uart_port)
{
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uint32_t irqn = uart_msg[uart_port].irqn;
	uint32_t value = 0;
	const _uart_config_t *uart_config = &uart_defconfig;
	uart_priv->uart_config = &uart_defconfig;
	char uart_name[12] = {0};

	if (uart_priv->is_inited) {
		return HAL_UART_STATUS_ERROR_UNINITIALIZED;
	}
	if ((!uart_port_is_valid(uart_port)) ||
			(!uart_config_is_valid(uart_config)))
	{
		return HAL_UART_STATUS_ERROR_PARAMETER;
	}

	hal_spin_lock_init(&uart_priv->spinlock);

	/* enable clk */
	uart_clk_init(uart_port, true);

	/* request gpio */
/* @TODO:delete CONFIG_ARCH_ARM_CORTEX_A55 atfer gpio adaptation complete */
#ifdef CONFIG_ARCH_ARM_CORTEX_A55
	uint32_t val = 0xffff2222;
	hal_writel(val, 0x07096000);
	val = 0x00000055;
	hal_writel(val, 0x07096024);
#else
	uart_pinctrl_init(uart_port);
#endif

	/* config uart attributes */
	uart_set_format(uart_port, uart_config->word_length,
			uart_config->stop_bit, uart_config->parity);

#ifdef CONFIG_ARCH_SUN8IW18P1
	/* force reset controller to disable transfer */
	uart_reset(uart_port);
#endif

	uart_set_baudrate(uart_port, uart_config->baudrate);

	value |= UART_FCR_RXTRG_1_2 | UART_FCR_TXTRG_1_2 | UART_FCR_FIFO_EN;
	uart_set_fifo(uart_port, value);

	if (uart_mailbox[uart_port] == NULL)
		uart_mailbox[uart_port] = hal_mailbox_create(uart_name, UART_FIFO_SIZE);
	if (uart_mailbox[uart_port] == NULL)
	{
		UART_ERR("create mailbox fail\n");
		return HAL_UART_STATUS_ERROR;
	}

	snprintf(uart_name, sizeof(uart_name), "uart%d", (int)uart_port);

	if (uart_priv->uart_port == uart_port && uart_priv->irqn == irqn)
	{
		UART_ERR("irq for uart%ld already enabled\n", (long int)uart_port);
	}
	else
	{
		uart_priv->uart_port = uart_port;
		uart_priv->irqn = irqn;

		if (hal_request_irq(irqn, uart_irq_handler, uart_name, uart_priv) < 0)
		{
			UART_ERR("request irq error\n");
			return -1;
		}

		hal_enable_irq(irqn);

	}

#if defined(CONFIG_UART_DMA)
#ifdef CONFIG_DRIVER_SYSCONFIG
	int val;
	char uart_dma_name[20] = {0};

	snprintf(uart_dma_name, sizeof(uart_dma_name), "uart%d_dma_used", (int)uart_port);

	if (hal_cfg_get_keyvalue("uart_dma", uart_dma_name, (int32_t *)&val, 3) == 0) {
		if (val == 1) {
			uart_priv->dma_used = val;
			UART_INFO("%s: uart_port%d enable dma tran. value: %d dma_used: %d\n", __func__, uart_port, value, uart_priv->dma_used);
		}
	} else {
		uart_priv->dma_used = uart_msg[uart_port].used_dma;
	}
#else
	uart_priv->dma_used = uart_msg[uart_port].used_dma;
#endif
	UART_INFO("%s: uart dma used: %d\n", __func__, uart_priv->dma_used);
	/* set uart IER */
	if (!uart_priv->dma_used)
		uart_enable_irq(uart_port, UART_IER_RDI | UART_IER_RLSI);
#else
		uart_enable_irq(uart_port, UART_IER_RDI | UART_IER_RLSI);
#endif
	/* force config */
	uart_enable_busy_cfg(uart_port);

#ifdef CONFIG_STANDBY
	uart_priv->pm = register_pm_dev_notify(uart_suspend, uart_resume, (void *)uart_port);
#endif

#ifdef CONFIG_COMPONENTS_PM
	pm_uart_ops[uart_port]->data = (void *)((uintptr_t)uart_port);
	pm_syscore_register(pm_uart_ops[uart_port]);
#endif

	uart_priv->is_inited = 1;
	return SUNXI_HAL_OK;
}

int32_t hal_uart_init_for_amp_cli(int32_t uart_port)
{
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uint32_t irqn = uart_msg[uart_port].irqn;
	char uart_name[12] = {0};

	if (!uart_port_is_valid(uart_port))
	{
		return HAL_UART_STATUS_ERROR_PARAMETER;
	}

	if (uart_mailbox[uart_port] == NULL)
		uart_mailbox[uart_port] = hal_mailbox_create(uart_name, UART_FIFO_SIZE);
	if (uart_mailbox[uart_port] == NULL)
	{
		UART_ERR("create mailbox fail\n");
		return HAL_UART_STATUS_ERROR;
	}

	snprintf(uart_name, sizeof(uart_name), "uart%d", (int)uart_port);
	if (uart_priv->uart_port == uart_port && uart_priv->irqn == irqn)
	{
		UART_ERR("irq for uart%ld already enabled\n", (long int)uart_port);
	}
	else
	{
		uart_priv->uart_port = uart_port;
		uart_priv->irqn = irqn;

		if (hal_request_irq(irqn, uart_irq_handler, uart_name, uart_priv) < 0)
		{
			UART_ERR("request irq error\n");
			return -1;
		}

		hal_enable_irq(irqn);

	}

	if (uart_priv->dma_used)
		hal_uart_dma_start(uart_port);

	uart_priv->is_inited = 1;
	return SUNXI_HAL_OK;
}

int32_t hal_uart_deinit(int32_t uart_port)
{
	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uint32_t irqn = uart_msg[uart_port].irqn;

	if (!uart_priv->is_inited) {
		return HAL_UART_STATUS_ERROR_UNINITIALIZED;
	}
#ifdef CONFIG_STANDBY
	unregister_pm_dev_notify(uart_priv->pm);
	uart_priv->pm = NULL;
#endif

#ifdef CONFIG_COMPONENTS_PM
	pm_syscore_unregister(pm_uart_ops[uart_port]);
#endif
	/* disable clk */
	uart_clk_init(uart_port, false);

	uart_pinctrl_uninit(uart_port);
	uart_enable_irq(uart_port, 0);
	hal_disable_irq(irqn);
	hal_free_irq(irqn);
	hal_mailbox_delete(uart_mailbox[uart_port]);
	uart_mailbox[uart_port] = NULL;

	uart_priv->uart_port = UART_MAX;
	uart_priv->irqn = 0;

	if (uart_priv->dma_used)
		hal_uart_dma_stop(uart_port);

	hal_spin_lock_deinit(&uart_priv->spinlock);
	uart_priv->is_inited = 0;
	return SUNXI_HAL_OK;
}

int32_t hal_uart_disable_rx(int32_t uart_port)
{
	uint32_t irqn = uart_msg[uart_port].irqn;
	hal_disable_irq(irqn);
	return 0;
}

int32_t hal_uart_enable_rx(int32_t uart_port)
{
	uint32_t irqn = uart_msg[uart_port].irqn;
	hal_enable_irq(irqn);
	return 0;
}

int32_t hal_uart_power_control(int32_t dev, sunxi_hal_power_state_e state)
{
	return SUNXI_HAL_OK;
}

static int __attribute__((no_instrument_function)) _uart_putc(int devid, char c)
{
	volatile uint32_t *sed_buf;
	volatile uint32_t *sta;

	sed_buf = (uint32_t *)(uart_msg[devid].base + UART_THR);
	sta = (uint32_t *)(uart_msg[devid].base + UART_USR);

	/* FIFO status, contain valid data */
	while (!(*sta & 0x02));
	*sed_buf = c;

	return 1;
}

int32_t __attribute__((no_instrument_function)) hal_uart_put_char(int32_t dev, char c)
{
	uart_priv_t *uart_priv = &g_uart_priv[dev];
	if (!uart_priv->is_inited) {
		return HAL_UART_STATUS_ERROR_UNINITIALIZED;
	}
	return _uart_putc(dev, c);
}

#if !defined(CONFIG_UART_DMA)
static inline
#endif
int32_t hal_uart_cpu_send(int32_t dev, const uint8_t *data, uint32_t num)
{
	int size;
	uart_priv_t *uart_priv = &g_uart_priv[dev];
	if (!uart_priv->is_inited) {
		return HAL_UART_STATUS_ERROR_UNINITIALIZED;
	}

	hal_assert(data != NULL);

	size = num;
	while (num)
	{
		_uart_putc(dev, *data);

		++ data;
		-- num;
	}

	return size - num;
}

int32_t hal_uart_send(int32_t dev, const uint8_t *data, uint32_t num)
{
	uart_priv_t *uart_priv = &g_uart_priv[dev];
	if (!uart_priv->is_inited) {
		return HAL_UART_STATUS_ERROR_UNINITIALIZED;
	}

	if (uart_priv->dma_used) {
		hal_uart_dma_send(dev, data, num);
		return num;
	}

	return hal_uart_cpu_send(dev, data, num);
}

static int _uart_getc(int devid)
{
	int ch = -1;
	volatile uint32_t *rec_buf;
	volatile uint32_t *sta;
	volatile uint32_t *fifo;

	rec_buf = (uint32_t *)(uart_msg[devid].base + UART_RHB);
	sta = (uint32_t *)(uart_msg[devid].base + UART_USR);
	fifo = (uint32_t *)(uart_msg[devid].base + UART_RFL);

	while (!(*fifo & 0x1ff));

	/* Receive Data Available */
	if (*sta & 0x08)
	{
		ch = *rec_buf & 0xff;
	}

	return ch;
}

uint8_t hal_uart_get_char(int32_t dev)
{
	uart_priv_t *uart_priv = &g_uart_priv[dev];
	if (!uart_priv->is_inited) {
		return 0;
	}
	return _uart_getc(dev);
}

int32_t hal_uart_receive_polling(int32_t dev, uint8_t *data, uint32_t num)
{
	uart_priv_t *uart_priv = &g_uart_priv[dev];
	if (!uart_priv->is_inited) {
		return HAL_UART_STATUS_ERROR_UNINITIALIZED;
	}
	if (uart_priv->dma_used) {
		hal_uart_dma_recv(dev, data, num);
		return num;
	}

	int ch;
	int size;

	hal_assert(data != NULL);
	size = num;

	while (num)
	{
		ch = _uart_getc(dev);
		if (ch == -1)
		{
			break;
		}

		*data = ch;
		data ++;
		num --;

		/* FIXME: maybe only used for console? move it away! */
		if (ch == '\n')
		{
			break;
		}
	}

	return size - num;
}

int32_t hal_uart_receive(int32_t dev, uint8_t *data, uint32_t num)
{
	uart_priv_t *uart_priv = &g_uart_priv[dev];
	if (!uart_priv->is_inited) {
		return HAL_UART_STATUS_ERROR_UNINITIALIZED;
	}
	if (uart_priv->dma_used) {
		hal_uart_dma_recv(dev, data, num);
		return num;
	}

	unsigned int data_rev;
	int i = 0;
	int32_t ret = -1, rev_count = 0;

	hal_assert(data != NULL);

	for (i = 0; i < num; i++)
	{
		ret = hal_mailbox_recv((hal_mailbox_t)uart_mailbox[dev], &data_rev, -1);
		if (ret == 0)
		{
			if (data_rev == UART_ERR_RECV_TERM) {
				/* clear all data in mailbox */
				while (hal_mailbox_recv((hal_mailbox_t)uart_mailbox[dev], &data_rev, 0) == 0);
				return HAL_UART_STATUS_RECEIVE_BREAK;
			}
			rev_count++;
			*(data + i) = (uint8_t)data_rev;
		}
		else
		{
			UART_ERR("receive error");
			break;
		}
	}

	return rev_count;
}

int32_t hal_uart_receive_no_block(int32_t dev, uint8_t *data, uint32_t num, int32_t timeout)
{
	uart_priv_t *uart_priv = &g_uart_priv[dev];
	if (!uart_priv->is_inited) {
		return HAL_UART_STATUS_ERROR_UNINITIALIZED;
	}
	if (uart_priv->dma_used) {
		num = hal_uart_dma_recv_wait(dev, data, num, timeout);
		return num;
	}

	unsigned int data_rev;
	int i = 0;
	int32_t ret = -1, rev_count = 0;

	hal_assert(data != NULL);

	for (i = 0; i < num; i++)
	{
		ret = hal_mailbox_recv((hal_mailbox_t)uart_mailbox[dev], &data_rev, timeout);
		if (ret == 0)
		{
			if (data_rev == UART_ERR_RECV_TERM) {
				/* clear all data in mailbox */
				while (hal_mailbox_recv((hal_mailbox_t)uart_mailbox[dev], &data_rev, 0) == 0);
				return HAL_UART_STATUS_RECEIVE_BREAK;
			}
			rev_count++;
			*(data + i) = (uint8_t)data_rev;
		}
		else
		{
			break;
		}
	}

	return rev_count;
}

int hal_uart_terminate_receive(int32_t dev)
{
	hal_mailbox_send((hal_mailbox_t)uart_mailbox[dev], UART_ERR_RECV_TERM);
	return 0;
}

int32_t hal_uart_transfer(int32_t dev, const void *data_out,
		void *data_in, uint32_t num)
{
	return SUNXI_HAL_OK;
}

uint32_t hal_uart_get_tx_count(int32_t dev)
{
	/* TODO: need verify */
	return 0;
}

uint32_t hal_uart_get_rx_count(int32_t dev)
{
	/* TODO: need verify */
	return 0;
}

int32_t hal_uart_control(int32_t uart_port, int cmd, void *args)
{
	_uart_config_t *uart_config;
	uart_config = (_uart_config_t *)args;

	uart_priv_t *uart_priv = &g_uart_priv[uart_port];
	uart_priv->uart_config = (_uart_config_t *)args;
	if (!uart_priv->is_inited) {
		return HAL_UART_STATUS_ERROR_UNINITIALIZED;
	}
	/* config uart attributes */
	uart_set_format(uart_port, uart_config->word_length,
			uart_config->stop_bit, uart_config->parity);
	uart_set_baudrate(uart_port, uart_config->baudrate);

	if (uart_priv->dma_used)
		hal_uart_dma_start(uart_port);

	return SUNXI_HAL_OK;
}

sunxi_hal_uart_status_t hal_uart_get_status(int32_t dev)
{
	sunxi_hal_uart_status_t status = {1, 1, 0, 0, 0, 0, 0, 0};

	return status;
}

int32_t hal_uart_set_modem_control(int32_t dev,
		sunxi_hal_uart_modem_control_e control)
{
	return SUNXI_HAL_OK;
}

sunxi_hal_uart_modem_status_t hal_uart_get_modem_status(int32_t dev)
{
	sunxi_hal_uart_modem_status_t status = {0, 0, 0, 0, 0};

	return status;
}

void hal_uart_set_loopback(uart_port_t uart_port, bool enable)
{
	unsigned long irq_flags;
	const unsigned long uart_base = uart_msg[uart_port].base;
	uint32_t value;

	irq_flags = hal_spin_lock_irqsave(&g_uart_priv[uart_port].spinlock);

	value = hal_readb(uart_base + UART_MCR);
	if (enable)
		value |= UART_MCR_LOOP;
	else
		value &= ~(UART_MCR_LOOP);
	hal_writeb(value, uart_base + UART_MCR);

	hal_spin_unlock_irqrestore(&g_uart_priv[uart_port].spinlock, irq_flags);
}

int serial_driver_init(void)
{
	UART_INFO("serial hal driver init");
	return 0;
}
