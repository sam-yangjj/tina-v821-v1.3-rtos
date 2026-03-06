/*
 * vin_supply.h
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zequn Zheng <zequnzhengi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __VIN_SUPPLY_H__
#define __VIN_SUPPLY_H__

#include <include.h>


typedef unsigned short addr_type;
typedef unsigned char data_type;

#define SENSOR_TWI_SPEED	400000
#define REG_DLY             0xffff
#define bool _Bool

enum gpio_type {
	PWDN = 0,
	RESET,
};

struct regval_list {
	addr_type addr;
	data_type data;
};


int sunxi_twi_init(int id);
int sunxi_twi_exit(int id);
int sensor_write_array(int id, struct regval_list *regs, int array_size);
int sensor_write(int id, addr_type reg, data_type value);
int sensor_read(int id, addr_type reg, data_type *value);
int vin_gpio_set_status(int id, enum gpio_type gpio_id, unsigned int status);
int vin_gpio_write(int id, enum gpio_type gpio_id, unsigned int out_value);
int vin_set_mclk(int id, unsigned int on_off, unsigned long freq);
int sensor_set_twi_addr(int id, int addr);

#endif

