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
#include <hal_time.h>
#include <hal_cmd.h>
#include <sunxi_hal_twi.h>

#define TEST_READ 0
#define TEST_WRITE 1

static uint8_t g_addr_len = 0x0;
static twi_port_t g_twi_port = 0;
static uint8_t g_twi_freq = 4;
static uint32_t g_twi_loop = 1;

static void cmd_twi_usage(void)
{
	hal_log_info("usage: twi_test_para");
	hal_log_info("\t -i:        show default para");
	hal_log_info("\t -h:        show help");
	hal_log_info("\t -m <num>:  set default mode, 0: engine, 1: drv");
	hal_log_info("\t -f <freq>:  set slave freq(hz), 1: 100K, 2: 200K, 4/other: 400k");
	hal_log_info("\t -b <bit_len>:  set slave addr len, 0: 7-Bit, 1: 10-Bit");
	hal_log_info("\t -p <port>: set default twi port");
	hal_log_info("\t -l <cnt>: twi test loop cnt");
}

static void hex_dump(uint8_t *addr, unsigned long len)
{
	int i = 0;

	if(!len) return;
	for (i = 0; i < len; i++) {
		if(!(i % 16) && (i > 0))
			printf("\n");
		printf("%02x ", addr[i]);
	}
	if(len % 16)
		printf("\n");
}

static int cmd_twi_test_para(int argc, const char **argv)
{
	int c;

	if (argc < 2) {
		cmd_twi_usage();
		return -1;
	}

	while ((c = getopt(argc, (char *const *)argv, "ihm:f:b:p:l:")) != -1) {
		switch (c) {
		case 'i':
			hal_log_info("twi mode: %s",
				     hal_twi_get_mode(g_twi_port) ? "drv" : "engine");
			hal_log_info("twi freq: %d hz",
				     hal_twi_get_freq(g_twi_port));
			hal_log_info("twi slave addr len: %s", g_addr_len ? "10-Bit" : "7-Bit");
			hal_log_info("twi loop cnt: %d", (unsigned int)g_twi_loop);
			break;
		case 'h':
			cmd_twi_usage();
			break;
		case 'm':
			hal_twi_set_mode(g_twi_port, atoi(optarg));
			hal_log_info("twi mode: %s",
				     hal_twi_get_mode(g_twi_port) ? "drv" : "engine");
			break;
		case 'f':
			g_twi_freq = atoi(optarg);
			switch (g_twi_freq) {
			case 1:
				hal_twi_set_freq(g_twi_port, TWI_FREQUENCY_100K);
				break;
			case 2:
				hal_twi_set_freq(g_twi_port, TWI_FREQUENCY_200K);
				break;
			case 4:
				hal_twi_set_freq(g_twi_port, TWI_FREQUENCY_400K);
				break;
			default:
				hal_twi_set_freq(g_twi_port, TWI_FREQUENCY_400K);
				break;
			}
			hal_log_info("twi freq: %d hz",
				     hal_twi_get_freq(g_twi_port));
			break;
		case 'b':
			g_addr_len = strtoul(optarg, NULL, 0);
			hal_log_info("twi slave addr len: %s", g_addr_len ? "10-Bit" : "7-Bit");
			break;
		case 'p':
			g_twi_port = strtoul(optarg, NULL, 0);
			hal_log_info("twi port: %u", (unsigned int)g_twi_port);
			break;
		case 'l':
			g_twi_loop = strtoul(optarg, NULL, 0);
			hal_log_info("twi loop cnt: %d", (unsigned int)g_twi_loop);
			break;
		default:
			hal_log_err("invalid param!");
		}
	}
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_twi_test_para, hal_twi_para, twi hal APIs tests set para)

static int cmd_test_twi(int argc, const char **argv)
{
    twi_msg_t msg;
    twi_port_t port;
    uint16_t addr;
    char reg_addr, reg_val = 0, rw = TEST_READ;
	uint8_t twi_status;
    int c;

    if (argc < 5)
	{
        hal_log_info("Usage:");
        hal_log_info("\ttwi [port] [slave_addr] [reg] -r [len of read buf]");
        hal_log_info("\t                              -w [val]");
        return -1;
    }

	hal_log_info("Run twi test");

    port = strtol(argv[1], NULL, 0);
    addr = strtol(argv[2], NULL, 0);
    reg_addr = strtol(argv[3], NULL, 0);
    if (argv[5])
    {
        reg_val = strtol(argv[5], NULL, 0);
    }

    while ((c = getopt(argc, (char *const *)argv, "rw")) != -1)
	{
        switch (c)
        {
            case 'r':
				hal_log_info("twi read test");
                rw = TEST_READ;
                break;
            case 'w':
				hal_log_info("twi write test");
                rw = TEST_WRITE;
                //reg_val = strtol(argv[5], NULL, 0);
                break;
			default:
				hal_log_err("invalid param!");
        }
    }

    hal_twi_init(port);

    if (rw == TEST_READ)
    {
		uint8_t *buffer;
		reg_val = reg_val ? reg_val : 1;
		buffer = (uint8_t *)malloc(reg_val * sizeof(uint8_t));
		for(int i = 0; i < g_twi_loop; i++)
		{
			if (g_addr_len)
				twi_status = hal_twi_read(port, I2C_TENBIT, addr, reg_addr, buffer, reg_val);
			else
				twi_status = hal_twi_read(port, I2C_SLAVE, addr, reg_addr, buffer, reg_val);
			if (twi_status) {
				hal_log_info("loop%d: addr[%x] reg[%x] read failed!\n", i, addr, reg_addr);
				break;
			} else {
				hal_log_info("loop%d:", i);
				hex_dump(buffer, reg_val);
			}
			hal_msleep(1000);
		}
		free(buffer);
    }
    else if (rw == TEST_WRITE)
    {
        /*
         * hal_twi_write bug workaround
         */
        uint8_t *buf;
		uint8_t w_len;

		if (argc == 7) {
			w_len = strtol(argv[6], NULL, 0);
		} else {
			w_len = 1;
		}

		buf = (uint8_t *)malloc((w_len + 1) * sizeof(uint8_t));
        buf[0] = reg_addr;
		memset(buf + 1, reg_val, w_len);

		if (g_addr_len)
			msg.flags |= TWI_M_TEN;
		else
			msg.flags &= ~TWI_M_TEN;
        msg.addr =  addr;
        msg.len = w_len + 1;
        msg.buf = buf;
		for(int i = 0; i < g_twi_loop; i++)
		{
			twi_status = hal_twi_write(port, &msg, 1);
			if (twi_status) {
				hal_log_info("loop%d: addr[%x] reg[%x] write %x failed!\n", i, addr, reg_addr, reg_val);
				break;
			} else {
				hal_log_info("loop%d: write: 0x%x", i, reg_val);
			}
			hal_msleep(1000);
		}
		free(buf);
    }

	hal_log_info("Twi test finish");

	hal_twi_uninit(port);

	hal_log_info("Twi test1 finish");
    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_twi, hal_twi, twi hal APIs tests)
