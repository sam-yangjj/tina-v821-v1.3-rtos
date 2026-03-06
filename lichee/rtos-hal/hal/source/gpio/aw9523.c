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
#include <string.h>
/* #include <init.h> */

#include <hal_mem.h>
#include <hal_gpio.h>
#include <sunxi_hal_twi.h>
#include <hal_log.h>
#include "aw9523.h"
//static gpio_direction_t test_dir = 0;
//static gpio_data_t test_data = 0;
static u16 slave_addr = 0x58;
static twi_port_t port = TWI_MASTER_1;
static bool aw9523_exsit = false;

static int aw95xx_twi_byte_read(u8 reg, u8 *reg_val)
{
	int ret = -1, try_times = 10;

TRY_READ:
	ret = hal_twi_read(port, I2C_SLAVE, slave_addr, reg, reg_val, 1);
	if (ret == TWI_STATUS_OK) {
		// printf("aw95xx read reg %02x = %02x\n", reg, *reg_val);
		return 0;
	} else if(try_times-- > 0) {
		goto TRY_READ;
	}
	
	return ret;
}

static int aw95xx_twi_byte_write(u8 reg, u8 reg_val)
{
	/* hal_twi_write bug workaround */
	twi_msg_t msg;
	int ret = -1, try_times = 10;
	uint8_t buf[2];
TRY_WRITE:
	buf[0] = reg;
	buf[1] = reg_val;
	msg.flags = 0;
	msg.addr = slave_addr;
	msg.len = 2;
	msg.buf = buf;

	ret = hal_twi_write(port, &msg, 1);
	if (ret == TWI_STATUS_OK) {
		// printf("aw95xx write reg %02x = %02x\n", reg, reg_val);
		return 0;
	} else if(try_times-- > 0) {
		goto TRY_WRITE;
	}
	return ret;
}


int aw9523_gpio_get_data(gpio_pin_t pin, gpio_data_t *data)
{
	u8 buff, index, reg;
	if(!aw9523_exsit || pin < GPIOEXP0(0) || pin > GPIOEXP1(7))
		return -1;
	else if(pin < GPIOEXP1(0)) {
		index = pin - GPIOEXP0(0);
		reg = 0x00;
	} else {
		index = pin - GPIOEXP1(0);
		reg = 0x01;
	}

	if(aw95xx_twi_byte_read(reg, &buff))
		return -1;

	*data = buff & (1 << index) ? GPIO_DATA_HIGH : GPIO_DATA_LOW;
	return 0;
}

int aw9523_gpio_set_data(gpio_pin_t pin, gpio_data_t data)
{
	u8 buff, reg, index;
	if(!aw9523_exsit || pin < GPIOEXP0(0) || pin > GPIOEXP1(7))
		return -1;
	else if(pin < GPIOEXP1(0)) {
		index = pin - GPIOEXP0(0);
		reg = 0x02;
	} else {
		index = pin - GPIOEXP1(0);
		reg = 0x03;
	}

	if(aw95xx_twi_byte_read(reg, &buff))
		return -1;

	if(data == GPIO_DATA_HIGH)
		buff |= 1 << index;
	else
		buff &= ~(1 << index);

	if(aw95xx_twi_byte_write(reg, buff))
		return -1;

	return 0;
}

int aw9523_gpio_set_direction(gpio_pin_t pin, gpio_direction_t direction)
{
	u8 buff, reg, index;
	if(!aw9523_exsit || pin < GPIOEXP0(0) || pin > GPIOEXP1(7))
		return -1;
	else if(pin < GPIOEXP1(0)) {
		index = pin - GPIOEXP0(0);
		reg = 0x04;
	} else {
		index = pin - GPIOEXP1(0);
		reg = 0x05;
	}

	if(aw95xx_twi_byte_read(reg, &buff))
		return -1;

	if(direction == GPIO_DIRECTION_INPUT)
		buff |= 1 << index;
	else
		buff &= ~(1 << index);

	if(aw95xx_twi_byte_write(reg, buff))
		return -1;

	return 0;
}

int aw9523_gpio_get_direction(gpio_pin_t pin, gpio_direction_t *direction)
{
	u8 buff, index, reg;
	if(!aw9523_exsit || pin < GPIOEXP0(0) || pin > GPIOEXP1(7))
		return -1;
	else if(pin < GPIOEXP1(0)) {
		index = pin - GPIOEXP0(0);
		reg = 0x04;
	} else {
		index = pin - GPIOEXP1(0);
		reg = 0x05;
	}

	if(aw95xx_twi_byte_read(reg, &buff))
		return -1;

	*direction = buff & (1 << index) ? GPIO_DIRECTION_INPUT : GPIO_DIRECTION_OUTPUT;
	return 0;
}

int aw9523_gpio_get_num(int port_num)
{
	if(port_num / 10)
		return GPIOEXP1(port_num % 10);
	else
		return GPIOEXP0(port_num % 10);
}

int aw9523_gpio_init(void)
{
	u8 read_data = 0;

    twi_status_t ret = hal_twi_init(port);
	if(ret != TWI_STATUS_OK) {
		printf("aw9523 i2c init error!!!\n");
		return -1;
	}

	// read id
	ret = aw95xx_twi_byte_read(0x10, &read_data);
	if(ret == TWI_STATUS_OK && read_data == 0x23) {
		aw9523_exsit = true;
	} else {
		printf("aw9523 setup error\n");
		return -1;
	}

	// set Push-Pull
	if(aw95xx_twi_byte_write(0x11, 0x10))
		return -1;

	printf("aw95xx init ok\n");
	return 0;
}
