/*
 * vin_supply.c
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zequn Zheng <zequnzhengi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <include.h>

#include "vin_supply.h"
#include "../platform/platform_cfg.h"



int sensor_read(int id, addr_type reg, data_type *value)
{
	int ret = 0, cnt = 0;
	int port;

	port = global_sensors[id].sensor_twi_id;
	ret = twi_read(twi_address[port], global_sensors[id].sensor_twi_addr >> 1, reg, global_sensors[id].addr_width / 8, value, 1);
	while ((ret != 0) && (cnt < 2)) {
		ret = twi_read(twi_address[port], global_sensors[id].sensor_twi_addr >> 1, reg, global_sensors[id].addr_width / 8, value, 1);
		cnt++;
	}
	if (cnt > 0)
		vin_warn("[%s]sensor read retry = %d\n", global_sensors[id].sensor_name, cnt);

	return ret;
}

int sensor_write(int id, addr_type reg, data_type value)
{
	int ret = 0, cnt = 0;
	int port;

	port = global_sensors[id].sensor_twi_id;
	ret = twi_write(twi_address[port], global_sensors[id].sensor_twi_addr >> 1, reg, global_sensors[id].addr_width / 8, &value, 1);
	while ((ret != 0) && (cnt < 2)) {
		ret = twi_write(twi_address[port], global_sensors[id].sensor_twi_addr >> 1, reg, global_sensors[id].addr_width / 8, &value, 1);
		cnt++;
	}
	if (cnt > 0)
		vin_warn("[%s]sensor write retry = %d\n", global_sensors[id].sensor_name, cnt);

	return ret;
}

int sensor_write_array(int id, struct regval_list *regs, int array_size)
{
	int ret = 0, i = 0;
	int port;
	//data_type rval = 0;
	port = global_sensors[id].sensor_twi_id;

	if (!regs)
		return -1;

	while (i < array_size) {
		if (regs->addr == REG_DLY) {
			udelay(regs->data * 1000);
		} else {
			ret = twi_write(twi_address[port], global_sensors[id].sensor_twi_addr >> 1,  regs->addr,  global_sensors[id].addr_width / 8, &(regs->data), 1);
			if (ret < 0) {
				vin_err("%s sensor write array error!\n", global_sensors[id].sensor_name);
				return -1;
			}
			//sensor_read(id, regs->addr, &rval);
			//vin_print("addr0x%x write data is 0x%x, read data is 0x%x\n", regs->addr, regs->data, rval);
		}
		i++;
		regs++;
	}
	return 0;
}

int sunxi_twi_init(int id)
{
	int slave_addr;
	int port;

	slave_addr = global_sensors[id].sensor_twi_addr >> 1;
	port = global_sensors[id].sensor_twi_id;

	sunxi_gpio_set_cfgpin(twi_gpio[port].pin[0], twi_gpio[port].pin_func[0]);
	sunxi_gpio_set_cfgpin(twi_gpio[port].pin[1], twi_gpio[port].pin_func[0]);
	twi_init(twi_address[port], SENSOR_TWI_SPEED);

	vin_log(VIN_LOG_POWER, "%s:id=%d, slave=0x%x\n", __func__, port, slave_addr);

	return 0;
}

int sunxi_twi_exit(int id)
{
	int port;

	port = global_sensors[id].sensor_twi_id;
	twi_exit(twi_address[port]);
	sunxi_gpio_set_cfgpin(twi_gpio[port].pin[0], twi_gpio[port].pin_func[1]);
	sunxi_gpio_set_cfgpin(twi_gpio[port].pin[1], twi_gpio[port].pin_func[1]);

	return 0;
}

int vin_set_pmu_channel(int on)
{
	return 0;
}

int vin_gpio_set_status(int id, enum gpio_type gpio_id, unsigned int status)
{
	u32 gpio;

	if (gpio_id == PWDN)
		gpio = global_sensors[id].pwdn_gpio;
	else if (gpio_id == RESET)
		gpio = global_sensors[id].reset_gpio;
	else
		return -1;

	if (gpio == 0xffff)
		return -1;

	if (status == 1) { //output
		sunxi_gpio_set_pull(gpio, GPIO_PULL_UP);
		sunxi_gpio_set_cfgpin(gpio, GPIO_DIRECTION_OUTPUT);
		sunxi_gpio_set_data(gpio, GPIO_DATA_LOW);
	} else if (status == 0) {
		sunxi_gpio_set_pull(gpio, GPIO_PULL_DOWN);
		sunxi_gpio_set_cfgpin(gpio, GPIO_DIRECTION_INPUT);
		sunxi_gpio_set_data(gpio, GPIO_DATA_LOW);
	}

	return 0;
}

int vin_gpio_write(int id, enum gpio_type gpio_id, unsigned int out_value)
{
	u32 gpio;

	if (gpio_id == PWDN)
		gpio = global_sensors[id].pwdn_gpio;
	else if (gpio_id == RESET)
		gpio = global_sensors[id].reset_gpio;
	else
		return -1;

	if (gpio == 0xffff)
		return -1;

	sunxi_gpio_set_cfgpin(gpio, GPIO_DIRECTION_OUTPUT);
	if (out_value) {
		sunxi_gpio_set_data(gpio, GPIO_DATA_HIGH);
	} else {
		sunxi_gpio_set_data(gpio, GPIO_DATA_LOW);
	}

	return 0;
}

int vin_set_mclk(int id, unsigned int on_off, unsigned long freq)
{

	int mclk_id = global_sensors[id].mclk_id;
	u32 mclk_reg = mclk_address[mclk_id];

	if (on_off) {
		if (freq == 24000000 || freq == 12000000 || freq == 6000000) {

		} else {
			writel(0xf9004205, 0x4a010048);
			writel(0x81000018, mclk_reg);
		}
		sunxi_gpio_set_cfgpin(vind_default_mclk[id].pin, vind_default_mclk[id].pin_func[0]);
	} else {
		sunxi_gpio_set_cfgpin(vind_default_mclk[id].pin, vind_default_mclk[id].pin_func[1]);
	}

	return 0;
}

int sensor_set_twi_addr(int id, int addr)
{
	global_sensors[id].sensor_twi_addr = addr;

	return 0;
}
