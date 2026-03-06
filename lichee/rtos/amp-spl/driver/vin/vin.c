/*
 * vin.c
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
#include "platform/platform_cfg.h"
#include "sensor/sensor_register.h"
#include "utility/vin_supply.h"

unsigned int vin_log_mask; /* = 0xffff - VIN_LOG_ISP - VIN_LOG_STAT - VIN_LOG_STAT1; */

int csi_init(void)
{
	int i, ret = 0;
	vin_log(VIN_LOG_MD, "CSI start!\n");
	for (i = 0; i < VIN_MAX_CSI; i++) {
		if (global_sensors[i].used == 1) {
			global_sensors[i].sensor_core = find_sensor_func(global_sensors[i].sensor_name);
			if (!global_sensors[i].sensor_core) {
				vin_err("find sensor error\n");
				return -1;
			}
		} else {
			continue;
		}

		sunxi_twi_init(i);

		if (global_sensors[i].sensor_core->s_stream) {
			ret = global_sensors[i].sensor_core->s_stream(i, 1);
			if (ret)
				return ret;
		}

		sunxi_twi_exit(i);
	}

	vin_log(VIN_LOG_MD, "GoodBye CSI!\n");
	return ret;
}
