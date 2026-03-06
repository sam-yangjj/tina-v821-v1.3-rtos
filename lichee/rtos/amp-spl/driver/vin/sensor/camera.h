/*
 * header for cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *	Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CAMERA__H__
#define __CAMERA__H__

#include <include.h>


#define CSI_GPIO_HIGH     1
#define CSI_GPIO_LOW     0

#define DEV_DBG_EN   0

#if (DEV_DBG_EN == 1)
#define sensor_dbg(x, arg...) printk("[%s_debug]" x, SENSOR_NAME, ##arg)
#else
#define sensor_dbg(x, arg...)
#endif
#define sensor_err(x, arg...) printk("[%s_ERR]%s, line: %d," x, SENSOR_NAME, \
	__FUNCTION__, __LINE__, ##arg)
#define sensor_print(x, arg...) printk("[%s]" x, SENSOR_NAME, ##arg)

enum power_seq_cmd {
	PWR_OFF = 0,
	PWR_ON = 1,
};

struct sensor_fuc_core {
	int (*sensor_test_i2c)(int id);
	int (*s_stream)(int id, int enable);
};

extern struct sensor_fuc_core ov7251_core;
extern struct sensor_fuc_core gc1084_core;

#endif /* __CAMERA__H__ */