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

struct twi_slave_eeprom_data {
    uint8_t buffer_idx;
    uint8_t address_mask;
    uint8_t num_address_bytes;
    uint8_t idx_write_cnt;
    uint8_t buffer[256];
    hal_spinlock_t buffer_lock;
};

static struct twi_slave_eeprom_data twi_slave_eeprom[TWI_MASTER_MAX];

static int i2c_slave_eeprom_slave_cb(uint8_t port, enum i2c_slave_event event, u8 *val)
{
    struct twi_slave_eeprom_data *eeprom = &twi_slave_eeprom[port];

    switch (event) {
	case I2C_SLAVE_WRITE_RECEIVED:
		if (eeprom->idx_write_cnt < eeprom->num_address_bytes) {
			if (eeprom->idx_write_cnt == 0)
				eeprom->buffer_idx = 0;
			eeprom->buffer_idx = *val | (eeprom->buffer_idx << 8);
			eeprom->idx_write_cnt++;
		} else {
			hal_spin_lock(&eeprom->buffer_lock);
            eeprom->buffer[eeprom->buffer_idx++ & eeprom->address_mask] = *val;
            hal_spin_unlock(&eeprom->buffer_lock);
		}
		break;

	case I2C_SLAVE_READ_PROCESSED:
		/* The previous byte made it to the bus, get next one */
		eeprom->buffer_idx++;
	case I2C_SLAVE_READ_REQUESTED:
		hal_spin_lock(&eeprom->buffer_lock);
		*val = eeprom->buffer[eeprom->buffer_idx & eeprom->address_mask];
		hal_spin_unlock(&eeprom->buffer_lock);
		/*
		 * Do not increment buffer_idx here, because we don't know if
		 * this byte will be actually used. Read Linux I2C slave docs
		 * for details.
		 */
		break;

	case I2C_SLAVE_STOP:
	case I2C_SLAVE_WRITE_REQUESTED:
		eeprom->idx_write_cnt = 0;
		break;

	default:
		break;
	}

	return 0;
}

static void twi_slave_dump_data(uint8_t *buf, uint32_t offset, uint32_t len)
{
	int col = 16;
	int line = len / col;
	int last = len % col;
	int i, j;
	uint8_t *buffer = buf + offset;

	for (i = 0; i < line; i++) {
		printf("%08X: ", i + offset);
		for (j = 0; j < col; j++) {
			printf("%02x ", buffer[col * i + j]);
		}
		printf("\n");
	}

	printf("%08X: ", col * line + offset);
	for (j = 0; j < last; j++) {
		printf("%02x ", buffer[col * line + j]);
	}
	printf("\n");
}

static int twi_slave_driver_probe(int port, uint32_t freq, uint16_t addr, uint8_t ten_bit_en)
{
    struct twi_slave_eeprom_data *eeprom = &twi_slave_eeprom[port];

    hal_twi_init(port);
    hal_twi_reg_slave(port, addr, ten_bit_en, freq, i2c_slave_eeprom_slave_cb);

    eeprom->buffer_idx = 0;
    eeprom->address_mask = sizeof(eeprom->buffer) - 1;
    eeprom->num_address_bytes = 1;
    eeprom->idx_write_cnt = 0;
    memset(eeprom->buffer, 0, sizeof(eeprom->buffer));
    hal_spin_lock_init(&eeprom->buffer_lock);

    return 0;
}

static int twi_slave_driver_remove(int port)
{
    struct twi_slave_eeprom_data *eeprom = &twi_slave_eeprom[port];

    hal_spin_lock_deinit(&eeprom->buffer_lock);
    hal_twi_unreg_slave(port);
    hal_twi_uninit(port);

    return 0;
}

static int twi_slave_driver_dump(int port, int addr, int size)
{
	struct twi_slave_eeprom_data *eeprom = &twi_slave_eeprom[port];

	if (addr > sizeof(eeprom->buffer) || addr + size > sizeof(eeprom->buffer)) {
		hal_log_err("dump addr/size out of bounds");
		return -1;
	}

	twi_slave_dump_data(eeprom->buffer, addr, size);

	return 0;
}

static void print_usage(const char *name)
{
	hal_log_info("Usage:");
	hal_log_info("\t%s probe <port> <freq> <addr> <ten_bit_en>", name);
	hal_log_info("\t%s remove <port>", name);
	hal_log_info("\t%s reset <port>", name);
	hal_log_info("\t%s dump <port> <reg> <size>", name);
}

static int cmd_twi_slave_driver(int argc, const char **argv)
{
	int port;
    uint8_t ten_bit_en;
    uint16_t addr;
	uint32_t freq;
	int reg, size;

	if (argc < 3) {
		print_usage(argv[0]);
		return -1;
	}

	port = strtol(argv[2], NULL, 0);
	if (port >= TWI_MASTER_MAX) {
		hal_log_err("twi port %d not exist", port);
		return -1;
	}

	if (!strcmp(argv[1], "probe")) {
		freq = strtoul(argv[3], NULL, 0);
        addr = strtoul(argv[4], NULL, 16);
        ten_bit_en = strtoul(argv[5], NULL, 0);
        twi_slave_driver_probe(port, freq, addr, ten_bit_en);
	}
	else if (!strcmp(argv[1], "remove")) {
        twi_slave_driver_remove(port);
    }
	else if (!strcmp(argv[1], "reset")) {
		hal_twi_slave_reset(port);
    }
	else if (!strcmp(argv[1], "dump")) {
		reg = strtol(argv[3], NULL, 16);
		size = strtol(argv[4], NULL, 0);
		twi_slave_driver_dump(port, reg, size);
	}
	else
		print_usage(argv[0]);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_twi_slave_driver, hal_twi_slave_driver, twi hal slave driver test)
