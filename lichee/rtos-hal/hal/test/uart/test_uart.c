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
#include <stdint.h>
#include <string.h>

#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_timer.h>
#include <hal_uart.h>
#include <hal_mem.h>

#ifdef CONFIG_OS_MELIS
#include <kapi.h>
#endif

#ifdef CONFIG_UART_DMA
#include <hal_cache.h>
#include <hal_dma.h>
#endif

#ifdef CONFIG_UART_DMA
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))
#define CIRC_CNT_TO_END(head,tail,size) \
        ({int end = (size) - (tail); \
          int n = ((head) + end) & ((size)-1); \
          n < end ? n : end;})

typedef struct
{
	uart_port_t port;
	int data_len;
	int rx_trigger_size;
	int test_num;
} uart_two_dma_test_t;

uart_two_dma_test_t first_test_struct;
uart_two_dma_test_t second_test_struct;

#endif

static void cmd_usage(void)
{
	printf("Usage:\n"
			"\t hal_uart <port> <baudrate>\n");
}

int cmd_test_uart(int argc, char **argv)
{
	uint8_t tbuf[6] = {"hello"};
	uint8_t rbuf[10] = {0};
	uart_port_t port;
	uint32_t baudrate;
	_uart_config_t uart_config;
	int i;

	hal_log_info("Testing UART in loopback mode");

	if (argc != 3) {
		cmd_usage();
		return -1;
	}

	port = strtol(argv[1], NULL, 0);
	baudrate = strtol(argv[2], NULL, 0);

#ifdef CONFIG_OS_MELIS
	int uart_debug_port;
	esCFG_GetKeyValue("uart_para", "uart_debug_port", (int32_t *)&uart_debug_port, 1);

	if(uart_debug_port == port){
#else
	if(CONFIG_CLI_UART_PORT == port){
#endif
		hal_log_info("uart0 can't test, please use other port!");
		return -1;
	}
	memset(rbuf, 0, 10 * sizeof(uint8_t));
	switch (baudrate) {
		case 4800:
			uart_config.baudrate = UART_BAUDRATE_4800;
			break;

		case 9600:
			uart_config.baudrate = UART_BAUDRATE_9600;
			break;

		case 38400:
			uart_config.baudrate = UART_BAUDRATE_38400;
			break;

		case 115200:
			uart_config.baudrate = UART_BAUDRATE_115200;
			break;

		case 230400:
			uart_config.baudrate = UART_BAUDRATE_230400;
			break;

		case 460800:
			uart_config.baudrate = UART_BAUDRATE_460800;
			break;

		case 921600:
			uart_config.baudrate = UART_BAUDRATE_921600;
			break;

		case 1500000:
			uart_config.baudrate = UART_BAUDRATE_1500000;
			break;

		case 4000000:
			uart_config.baudrate = UART_BAUDRATE_4000000;
			break;
		default:
			hal_log_info("Using default baudrate: 115200");
			uart_config.baudrate = UART_BAUDRATE_115200;
			break;
	}

	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;

	hal_uart_init(port);
	hal_uart_control(port, 0, &uart_config);
	hal_uart_disable_flowcontrol(port);
	hal_uart_set_loopback(port, 1);

	/* send */
	hal_uart_send(port, tbuf, 5);

	/* loopback receive */
	hal_uart_receive_no_block(port, rbuf, 5, MS_TO_OSTICK(1000));

	printf("Sending:");
	for (i = 0; i < 5; i++)
		printf("%c", tbuf[i]);
	printf("\n");

	printf("Receiving:");
	for (i = 0; i < 5; i++)
		printf("%c", rbuf[i]);
	printf("\n");

	/* verify data */
	for (i = 0; i < 5; i++) {
		if (tbuf[i] != rbuf[i])
			break;
	}
	if (i == 5) {
		hal_log_info("Test hal_uart_init API success!");
		hal_log_info("Test hal_uart_control API success!");
		hal_log_info("Test hal_uart_disable_flowcontrol API success!");
		hal_log_info("Test hal_uart_set_loopback API success!");
		hal_log_info("Test hal_uart_send API success!");
		hal_log_info("Test hal_uart_receive API success!");
		hal_log_info("Test hal_uart_deinit API success!");
		hal_log_info("Test uart hal APIs success!");
	} else {
		hal_log_info("Test uart hal APIs failed!");
	}

	hal_msleep(1000);
	hal_uart_deinit(port);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_uart, hal_uart, uart hal APIs tests)

#define BUFFSIZE 4096

static void cmd_stress_usage(void)
{
	printf("Usage:\n"
			"\t hal_uart <port> <baudrate> <flowctrl> <loopback> <len>\n");
}

int cmd_test_uart_stress(int argc, char **argv)
{
	uint8_t *tbuf = malloc(BUFFSIZE);
	uint8_t *rbuf = malloc(BUFFSIZE);
	uart_port_t port;
	uint32_t baudrate;
	_uart_config_t uart_config;
	int i;
	int flowctrl, loopback, testlen;

	hal_log_info("Testing UART in loopback mode with stress");

	if (argc != 6) {
		cmd_stress_usage();
		free(tbuf);
		free(rbuf);
		return -1;
	}

	port = strtol(argv[1], NULL, 0);
	baudrate = strtol(argv[2], NULL, 0);
	flowctrl = strtol(argv[3], NULL, 0);
	loopback = strtol(argv[4], NULL, 0);
	testlen = strtol(argv[5], NULL, 0);

	for (i = 0; i < BUFFSIZE; i++) {
		tbuf[i] = ('a' + i) & 0xff;
	}
	memset(rbuf, 0, BUFFSIZE * sizeof(uint8_t));

	switch (baudrate) {
		case 4800:
			uart_config.baudrate = UART_BAUDRATE_4800;
			break;

		case 9600:
			uart_config.baudrate = UART_BAUDRATE_9600;
			break;

		case 38400:
			uart_config.baudrate = UART_BAUDRATE_38400;
			break;

		case 115200:
			uart_config.baudrate = UART_BAUDRATE_115200;
			break;

		case 230400:
			uart_config.baudrate = UART_BAUDRATE_230400;
			break;

		case 460800:
			uart_config.baudrate = UART_BAUDRATE_460800;
			break;

		case 921600:
			uart_config.baudrate = UART_BAUDRATE_921600;
			break;

		case 1500000:
			uart_config.baudrate = UART_BAUDRATE_1500000;
			break;

		case 4000000:
			uart_config.baudrate = UART_BAUDRATE_4000000;
			break;
		default:
			hal_log_info("Using default baudrate: 115200");
			uart_config.baudrate = UART_BAUDRATE_115200;
			break;
	}

	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;

	hal_uart_init(port);
	hal_uart_control(port, 0, &uart_config);
	printf("flow:%d, loopback:%d len:%d\n", flowctrl, loopback, testlen);
	if (flowctrl)
		hal_uart_set_hardware_flowcontrol(port);
	else
		hal_uart_disable_flowcontrol(port);

	if (loopback)
		hal_uart_set_loopback(port, 1);
	else
		hal_uart_set_loopback(port, 0);

	/* send */
	printf("send\n");
	hal_uart_send(port, tbuf, testlen);
	printf("send done\n");

	printf("recv\n");
	/* loopback receive */
	hal_uart_receive(port, rbuf, testlen);
	//hal_uart_receive_no_block(port, rbuf, testlen, 1000);
	printf("recv done\n");

#if 0
	printf("Sending:");
	for (i = 0; i < testlen; i++) {
		if (i % 16 == 0)
			printf("\n");
		printf("0x%x ", tbuf[i]);
	}
	printf("\n");

	printf("Receiving:");
	for (i = 0; i < testlen; i++) {
		if (i % 16 == 0)
			printf("\n");
		printf("0x%x ", rbuf[i]);
	}
	printf("\n");
#endif

	/* verify data */
	for (i = 0; i < testlen; i++) {
		if (tbuf[i] != rbuf[i]) {
			printf("check %d fail, 0x%x != 0x%x\n", i, tbuf[i], rbuf[i]);
			break;
		}
	}
	if (i == testlen) {
		hal_log_info("Test hal_uart_init API success!");
		hal_log_info("Test hal_uart_control API success!");
		hal_log_info("Test hal_uart_disable_flowcontrol API success!");
		hal_log_info("Test hal_uart_set_loopback API success!");
		hal_log_info("Test hal_uart_send API success!");
		hal_log_info("Test hal_uart_receive API success!");
		hal_log_info("Test hal_uart_deinit API success!");
		hal_log_info("Test uart hal APIs success!");
	} else {
		hal_log_info("Test uart hal APIs failed!");
	}

	hal_msleep(1000);
	hal_uart_deinit(port);
	free(tbuf);
	free(rbuf);


	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_uart_stress, hal_uart_stress, uart hal APIs tests)

#ifdef CONFIG_UART_DMA
void test_user_callback()
{
	printf("this is test_user_callback\n");
}

static void cmd_dma_usage(void)
{
	printf("Usage:\n"
		"\t hal_uart_dma <port> <baudrate> <len> <num> [loop] [use_tx_dma]\n");
}

int cmd_test_uart_dma(int argc, char **argv)
{
	int i;
	uart_port_t port;
	uint32_t baudrate, len;
	_uart_config_t uart_config;
	uint8_t *tbuf, *rbuf;
	int num, ret, loop = 1, use_tx_dma = 1;
	static int total_num = 0;

	if (argc < 5) {
		cmd_dma_usage();
		return -1;
	}

	port = strtol(argv[1], NULL, 0);
	baudrate = strtol(argv[2], NULL, 0);
	len = strtol(argv[3], NULL, 0);
	num = strtol(argv[4], NULL, 0);
	if (argc >= 6)
		loop = strtol(argv[5], NULL, 0);

	if (argc >= 7)
		use_tx_dma = strtol(argv[6], NULL, 0);

	tbuf = malloc(len);
	rbuf = malloc(len);

	memset(tbuf, 0, len * sizeof(uint8_t));
	memset(rbuf, 1, len * sizeof(uint8_t));

#ifdef CONFIG_OS_MELIS
	int uart_debug_port;
	esCFG_GetKeyValue("uart_para", "uart_debug_port", (int32_t *)&uart_debug_port, 1);

	if(uart_debug_port == port){
#else
	if(CONFIG_CLI_UART_PORT == port){
#endif
		printf("uart0 can't test, please use other port!");
		free(tbuf);
		free(rbuf);
		return -1;
	}

	switch (baudrate) {
	case 4800:
		uart_config.baudrate = UART_BAUDRATE_4800;
		break;

	case 9600:
		uart_config.baudrate = UART_BAUDRATE_9600;
		break;

	case 115200:
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;

	default:
		printf("Using default baudrate: 115200");
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;
	}

	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;
#ifdef CONFIG_DRIVERS_NDMA
	uart_config.uart_rx_trigger_size = len;
	uart_config.rx_buf_len = len;
#else
	uart_config.uart_rx_trigger_size = strtol(argv[7], NULL, 0);
	uart_config.rx_buf_len = 4096;
#endif
	uart_config.callback = test_user_callback;

again:
	num--;

	hal_uart_init(port);

	hal_uart_control(port, 0, &uart_config);
	hal_uart_disable_flowcontrol(port);

	/* loopback to check tx and rx*/
	hal_uart_set_loopback(port, loop);

	for (i = 0;i < len; i++)
		tbuf[i] = (i + total_num) & 0xff;

	/* send */
	printf("total_num:%d\r\n", total_num);

	if (use_tx_dma) {
		printf("starting tx_dma test\n");
		hal_uart_dma_send(port, tbuf, len);
		//for (i = 0;i < len; i++)
		//	hal_uart_put_char(port, tbuf[i]);
		printf("finishing tx_dma test\n");
	} else {
		printf("starting tx_cpu test\n");
		hal_uart_cpu_send(port, tbuf, len);
		//for (i = 0;i < len; i++)
		//	hal_uart_put_char(port, tbuf[i]);
		printf("finishing tx_cpu test\n");
	}

	hal_msleep(100);

	/* receive */
	printf("starting rx_dma test\n");
	ret = hal_uart_dma_recv(port, rbuf, len);
	//for (i = 0;i < len; i++)
	//	rbuf[i] = hal_uart_get_char(1);
	printf("finishing rx_dma test\n");
	printf("recv len:%d\r\n", ret);

	/* print send and recv */
	printf("Sending:");
	for (i = 0; i < len; i++){
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", tbuf[i]);
	}
	printf("\n");

	printf("Receiving:");
	for (i = 0; i < ret; i++){
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", rbuf[i]);
	}
	printf("\n");

	/* verify data */
	for (i = 0; i < len; i++) {
		if (tbuf[i] != rbuf[i])
			break;
	}
	if (i == len) {
		printf("Test hal_uart_dma API success!\n");
	} else {
		printf("Test uart_dma hal APIs failed!\n");
		num = 0;
	}

	hal_uart_deinit(port);
	total_num ++;
	if (num > 0)
		goto again;

	free(tbuf);
	free(rbuf);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_uart_dma, hal_uart_dma, uart hal_dma APIs tests)

static void first_uart_dma_test_thread(void *param)
{
	uart_two_dma_test_t *test_struct = (uart_two_dma_test_t *)param;
	uart_port_t port = test_struct->port;
	int num = test_struct->test_num;
	uint32_t len = test_struct->data_len;
	int i;
	uint32_t baudrate = 115200;
	_uart_config_t uart_config;
	uint8_t *tbuf, *rbuf;
	int ret, loop = 1;
	static int total_num = 3;

	tbuf = malloc(len);
	rbuf = malloc(len);

	memset(tbuf, 0, len * sizeof(uint8_t));
	memset(rbuf, 1, len * sizeof(uint8_t));

	if(CONFIG_CLI_UART_PORT == port){
		printf("uart0 can't test, please use other port!");
		free(tbuf);
		free(rbuf);
	}

	switch (baudrate) {
	case 4800:
		uart_config.baudrate = UART_BAUDRATE_4800;
		break;

	case 9600:
		uart_config.baudrate = UART_BAUDRATE_9600;
		break;

	case 115200:
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;

	default:
		printf("Using default baudrate: 115200");
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;
	}

	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;
	uart_config.uart_rx_trigger_size = test_struct->rx_trigger_size;
	uart_config.rx_buf_len = 4096;
	uart_config.callback = test_user_callback;

again:
	num--;

	hal_uart_init(port);

	hal_uart_control(port, 0, &uart_config);
	hal_uart_disable_flowcontrol(port);

	/* loopback to check tx and rx*/
	hal_uart_set_loopback(port, loop);

	for (i = 0;i < len; i++)
		tbuf[i] = (i + total_num) & 0xff;

	/* send */
	printf("UART-%d: total_num:%d\r\n", port, total_num);

	printf("UART-%d: starting tx_dma test\n", port);
	hal_uart_dma_send(port, tbuf, len);
	printf("UART-%d: finishing tx_dma test\n", port);

	hal_msleep(100);

	/* receive */
	printf("UART-%d: starting rx_dma test\n", port);
	ret = hal_uart_dma_recv(port, rbuf, len);
	printf("UART-%d: finishing rx_dma test\n", port);
	printf("UART-%d: recv len:%d\r\n", port, ret);

	/* print send and recv */
	printf("UART-%d: Sending:", port);
	for (i = 0; i < len; i++){
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", tbuf[i]);
	}
	printf("\n");

	printf("UART-%d: Receiving:", port);
	for (i = 0; i < ret; i++){
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", rbuf[i]);
	}
	printf("\n");

	/* verify data */
	for (i = 0; i < len; i++) {
		if (tbuf[i] != rbuf[i])
			break;
	}
	if (i == len) {
		printf("UART-%d: Test hal_uart_dma API success!\n", port);
	} else {
		printf("UART-%d: Test uart_dma hal APIs failed!\n", port);
		num = 0;
	}

	hal_uart_deinit(port);
	total_num ++;
	if (num > 0)
		goto again;

	free(tbuf);
	free(rbuf);

	hal_thread_stop(NULL);
}

static void second_uart_dma_test_thread(void *param)
{
	uart_two_dma_test_t *test_struct = (uart_two_dma_test_t *)param;
	uart_port_t port = test_struct->port;
	int num = test_struct->test_num;
	uint32_t len = test_struct->data_len;
	int i;
	uint32_t baudrate = 115200;
	_uart_config_t uart_config;
	uint8_t *txbuf, *rxbuf;
	int ret, loop = 1;
	static int total_num = 6;

	txbuf = malloc(len);
	rxbuf = malloc(len);

	memset(txbuf, 0, len * sizeof(uint8_t));
	memset(rxbuf, 1, len * sizeof(uint8_t));

	if(CONFIG_CLI_UART_PORT == port){
		printf("uart0 can't test, please use other port!");
		free(txbuf);
		free(rxbuf);
	}

	switch (baudrate) {
	case 4800:
		uart_config.baudrate = UART_BAUDRATE_4800;
		break;

	case 9600:
		uart_config.baudrate = UART_BAUDRATE_9600;
		break;

	case 115200:
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;

	default:
		printf("Using default baudrate: 115200");
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;
	}

	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;
	uart_config.uart_rx_trigger_size = test_struct->rx_trigger_size;
	uart_config.rx_buf_len = 4096;
	uart_config.callback = test_user_callback;

again:
	num--;

	hal_uart_init(port);

	hal_uart_control(port, 0, &uart_config);
	hal_uart_disable_flowcontrol(port);

	hal_uart_set_loopback(port, loop);


	for (i = 0;i < len; i++)
		txbuf[i] = (i + total_num) & 0xff;

	printf("UART-%d: total_num:%d\r\n", port, total_num);

	printf("UART-%d: starting tx_dma test\n", port);
	hal_uart_dma_send(port, txbuf, len);
	printf("UART-%d: finishing tx_dma test\n", port);

	hal_msleep(100);

	printf("UART-%d: starting rx_dma test\n", port);
	ret = hal_uart_dma_recv(port, rxbuf, len);
	printf("UART-%d: finishing rx_dma test\n", port);
	printf("UART-%d: recv len:%d\r\n", port, ret);

	printf("UART-%d: Sending:", port);
	for (i = 0; i < len; i++){
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", txbuf[i]);
	}
	printf("\n");

	printf("UART-%d: Receiving:", port);
	for (i = 0; i < ret; i++){
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", rxbuf[i]);
	}
	printf("\n");

	for (i = 0; i < len; i++) {
		if (txbuf[i] != rxbuf[i])
			break;
	}
	if (i == len) {
		printf("UART-%d: Test hal_uart_dma API success!\n", port);
	} else {
		printf("UART-%d: Test uart_dma hal APIs failed!\n", port);
		num = 0;
	}

	hal_uart_deinit(port);

	total_num ++;
	if (num > 0)
		goto again;

	free(txbuf);
	free(rxbuf);

	hal_thread_stop(NULL);
}

int cmd_test_uart_two_dma(int argc, char **argv)
{
	void *first_test_thread;
	void *second_test_thread;

	if (argc != 6) {
	printf("Usage:\n"
		"\t hal_uart_two_dma <port1> <port2> <data_len> <rx_trigger_size> <test_num>\n");
		return -1;
	}

	first_test_struct.port = strtol(argv[1], NULL, 0);
	first_test_struct.data_len = strtol(argv[3], NULL, 0);
	first_test_struct.rx_trigger_size = strtol(argv[4], NULL, 0);
	first_test_struct.test_num = strtol(argv[5], NULL, 0);

	second_test_struct.port = strtol(argv[2], NULL, 0);
	second_test_struct.data_len = strtol(argv[3], NULL, 0);
	second_test_struct.rx_trigger_size = strtol(argv[4], NULL, 0);
	second_test_struct.test_num = strtol(argv[5], NULL, 0);

	first_test_thread = hal_thread_create(first_uart_dma_test_thread, &first_test_struct,
				"test first uart dma", 8 * 1024, HAL_THREAD_PRIORITY_SYS);
	if (first_test_thread)
		hal_thread_start(first_test_thread);

	second_test_thread = hal_thread_create(second_uart_dma_test_thread, &second_test_struct,
				"test second uart dma", 8 * 1024, HAL_THREAD_PRIORITY_SYS);
	if (second_test_thread)
		hal_thread_start(second_test_thread);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_uart_two_dma, hal_uart_two_dma, two uart hal_dma APIs tests)
#endif
