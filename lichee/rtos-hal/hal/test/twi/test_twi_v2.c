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
#include <unistd.h>

#include <hal_log.h>
#include <hal_cmd.h>
#include <sunxi_hal_twi.h>

#include <hal_time.h>
#define TEST_WRITE_READ 0
#define TEST_WRITE 1
#define TEST_READ 2 // test not support
#define TEST_NONE -1

#define MAX_TEST_BUF_SIZE	(64)

static uint8_t g_twi_addr = 0x50;
static uint8_t g_twi_reg = 0x0;
static twi_port_t g_twi_port = 5;

static uint8_t g_twi_wbuf[MAX_TEST_BUF_SIZE];

static void hex_dump(uint8_t *addr, unsigned long len)
{
	int i = 0;
	char buf[32+4];
	char *ptr = buf;

	for (i = 0; i < len; i++) {
		sprintf(ptr, "%02x", addr[i]);
		ptr += 2;
		if ((ptr - buf) >= 32) {
			*ptr = 0;
			printf("%s\n", buf);
			ptr = buf;
		}
	}
	if (ptr != buf) {
		*ptr = 0;
		printf("%s\n", buf);
	}
}

static int cmd_twi_v2_test_data(int argc, const char **argv)
{
	int i, count, offset = 0;

	if (argc < 3) {
		hal_log_info("usage: %s <offset> <data0> [data1] ... [dataN]", argv[0]);
		return -1;
	}

	offset = strtoul(argv[1], NULL, 0);

	if ((argc - 2) > (MAX_TEST_BUF_SIZE - offset))
		count = MAX_TEST_BUF_SIZE - offset;
	else
		count = argc - 2;

	for (i = 0; i < count; i++) {
		g_twi_wbuf[i + offset] = strtoul(argv[i + 2], NULL, 0);
	}

	hal_log_info("data:");
	hex_dump(g_twi_wbuf, MAX_TEST_BUF_SIZE);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_twi_v2_test_data, twi_v2_test_data, twi hal APIs v2 tests set data)

static int cmd_twi_v2_test_para(int argc, const char **argv)
{
	int c;

	if (argc < 2) {
		hal_log_info("usage: %s", argv[0]);
		hal_log_info("\t -i:        show default para");
		hal_log_info("\t -m <num>:  set default mode, 0: engine, 1: drv");
		hal_log_info("\t -p <port>: set default twi port");
		hal_log_info("\t -a <addr>: set default slave addr");
		hal_log_info("\t -r <reg>:  set default slave reg offset");
		return -1;
	}

	while ((c = getopt(argc, (char *const *)argv, "im:p:a:r:")) != -1) {
		switch (c) {
		case 'i':
			hal_log_info("twi port: %u", (unsigned int)g_twi_port);
			hal_log_info("twi mode: %s",
				     hal_twi_get_mode(g_twi_port) ? "drv" : "engine");
			hal_log_info("twi slave addr: %x", g_twi_addr);
			hal_log_info("twi slave reg: %x", g_twi_reg);
			break;
		case 'm':
			hal_twi_set_mode(g_twi_port, atoi(optarg));
			hal_log_info("twi mode: %s",
				     hal_twi_get_mode(g_twi_port) ? "drv" : "engine");
			break;
		case 'p':
			g_twi_port = strtoul(optarg, NULL, 0);
			hal_log_info("twi port: %u", (unsigned int)g_twi_port);
			break;
		case 'a':
			g_twi_addr = strtoul(optarg, NULL, 0);
			hal_log_info("twi slave addr: %x", g_twi_addr);
			break;
		case 'r':
			g_twi_reg = strtoul(optarg, NULL, 0);
			hal_log_info("twi slave reg: %x", g_twi_reg);
			break;
		default:
			hal_log_err("invalid param!");
		}
	}
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_twi_v2_test_para, twi_v2_test_para, twi hal APIs v2 tests set para)

static int get_rw_from_arg(const char *arg)
{
	if (arg[0] == 'r')
		return TEST_WRITE_READ;
	if (arg[0] == 'w')
		return TEST_WRITE;
	if (arg[0] == 'c')
		return TEST_READ;
	return TEST_NONE;
}

static uint8_t *choose_buf(int dir, uint8_t *rbuf, uint8_t *wbuf)
{
	if (dir == TEST_WRITE_READ)
		return rbuf;
	else if (dir == TEST_WRITE)
		return wbuf;
	return NULL;
}

static inline
int twi_test_once(twi_port_t port, int dir, uint8_t addr, uint8_t reg, uint8_t *data, int len,
		  int sleep_ms, int verbose, int cur_loop)
{
	int ret;

	ret = hal_twi_control_v2(port, I2C_SLAVE, &addr);
	if (ret) {
		hal_log_info("%d hal_twi_control ret: %d", cur_loop, ret);
	}

	if (dir == TEST_WRITE_READ) {
		ret = hal_twi_read_v2(port, reg, data, len);
		if (verbose || ret)
			hal_log_info("%d hal_twi_read ret: %d", cur_loop, ret);
		if (verbose)
			hex_dump(data, len);
	} else if (dir == TEST_WRITE) {
		if (verbose)
			hex_dump(data, len);
		ret = hal_twi_write_v2(port, reg, data, len);
		if (verbose || ret)
			hal_log_info("%d hal_twi_write ret: %d", cur_loop, ret);
	}

	if (sleep_ms > 0)
		hal_msleep(sleep_ms);

	return ret;
}

static int cmd_twi_v2_test(int argc, const char **argv)
{
	int ret;
	twi_port_t port = g_twi_port;
	uint8_t addr = g_twi_addr;
	uint8_t reg = g_twi_reg;
	int loop, verbose;
	uint8_t rbuf[MAX_TEST_BUF_SIZE];
	uint8_t *data;
	int len1 = 0, len2 = 0, sleep_ms1, sleep_ms2, dir1 = TEST_NONE, dir2 = TEST_NONE;

	if (argc != 6 && argc != 9) {
		hal_log_info("usage: %s <loop> <verbose> <r/w/n> <len1> <sleep_ms1> [<r/w/n> <len2> <sleep_ms2>]", argv[0]);
		hal_log_info("\t loop:     test loop times");
		hal_log_info("\t r/w/n:    read ops, write ops or sleep only");
		hal_log_info("\t len:      transmission length(Byte)");
		hal_log_info("\t sleep_ms: sleep time(ms)");
		return 0;
	}

	loop = atoi(argv[1]);
	verbose = atoi(argv[2]);
	dir1 = get_rw_from_arg(argv[3]);
	len1 = atoi(argv[4]);
	sleep_ms1 = atoi(argv[5]);
	if (argc == 9) {
		dir2 = get_rw_from_arg(argv[6]);
		len2 = atoi(argv[7]);
		sleep_ms2 = atoi(argv[8]);
	}

	hal_log_info("loop: %d", loop);
	hal_log_info("verbose: %d", verbose);

	hal_log_info("dir1: %d", dir1);
	hal_log_info("len1: %d", len1);
	hal_log_info("sleep_ms1: %d", sleep_ms1);
	if (argc == 9) {
		hal_log_info("dir2: %d", dir2);
		hal_log_info("len2: %d", len2);
		hal_log_info("sleep_ms2: %d", sleep_ms2);
	}

	if (loop <= 0) {
		hal_log_info("loop error");
		return 0;
	}

	if (len1 < 0 || len1 > 64) {
		hal_log_info("len1 error");
		return 0;
	}
	if (sleep_ms1 < 0) {
		hal_log_info("sleep_ms1 error");
		return 0;
	}

	if (argc == 9) {
		if (len2 < 0 || len2 > 64) {
		hal_log_info("len2 error");
		return 0;
		}
		if (sleep_ms2 < 0) {
		hal_log_info("sleep_ms2 error");
		return 0;
		}
	}

	hal_log_info("twi%d mode: %s", port, hal_twi_get_mode(port) ? "drv" : "engine");

	ret = hal_twi_init_v2(port, TWI_FREQUENCY_100K);
	if (ret) {
		hal_log_info("hal_twi_init ret: %d", ret);
		return 0;
	}

	memset(rbuf, 0, MAX_TEST_BUF_SIZE);
again:
	data = choose_buf(dir1, rbuf, g_twi_wbuf);
	twi_test_once(port, dir1, addr, reg, data, len1, sleep_ms1, verbose, loop);
	if (argc == 9) {
		data = choose_buf(dir2, rbuf, g_twi_wbuf);
		twi_test_once(port, dir2, addr, reg, data, len2, sleep_ms2, verbose, loop);
		if ((dir2 == TEST_WRITE_READ) && (dir1 == TEST_WRITE) && (len1 == len2)) {
			if (memcmp(rbuf, g_twi_wbuf, len1)) {
				hal_log_info("data check failed");
				hal_log_info("write data:");
				hex_dump(g_twi_wbuf, len1);
				hal_log_info("read data:");
				hex_dump(rbuf, len1);
			}
		}
	}

	if (--loop)
		goto again;

	hal_twi_uninit(port);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_twi_v2_test, twi_v2_test, twi hal APIs v2 tests)

static int cmd_test_twi_v2_reg(int argc, const char **argv)
{
	twi_port_t port;
	uint16_t addr;
	char reg_addr, rw = TEST_WRITE_READ;
	uint8_t buf[4];
	int c;

	if (argc < 5) {
		hal_log_info("Usage:");
		hal_log_info("\ttwi [port] [slave_addr] [reg] -r");
		hal_log_info("\t                              -w [val]");
		return -1;
	}

	hal_log_info("Run twi test");

	port = strtol(argv[1], NULL, 0);
	addr = strtol(argv[2], NULL, 0);
	reg_addr = strtol(argv[3], NULL, 0);
	if (argv[5])
		buf[0] = strtol(argv[5], NULL, 0);

	while ((c = getopt(argc, (char *const *)argv, "rw")) != -1) {
		switch (c) {
		case 'r':
			hal_log_info("twi read test");
			rw = TEST_WRITE_READ;
			break;
		case 'w':
			hal_log_info("twi write test");
			rw = TEST_WRITE;
			break;
		default:
			hal_log_err("invalid param!");
		}
	}

	hal_log_info("twi%d init mode: %s", port, hal_twi_get_mode(port) ? "drv" : "engine");
	hal_twi_init_v2(port, TWI_FREQUENCY_100K);

	twi_test_once(port, rw, addr, reg_addr, buf, 1, 0, 0, 0);
	if (rw == TEST_WRITE_READ){
		hal_log_info("reg_val: 0x%x", buf[0]);
	}

	hal_log_info("Twi test finish");

	hal_twi_uninit(port);
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_twi_v2_reg, twi_v2_reg, twi hal APIs v2 reg tests)
