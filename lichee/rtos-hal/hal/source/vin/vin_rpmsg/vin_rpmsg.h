/*
 * vin rpmsg init driver
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _VIN_RPMSG__H_
#define _VIN_RPMSG__H_

#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_thread.h>
#include <hal_timer.h>
#include <hal_sem.h>
#include <openamp/sunxi_helper/openamp.h>
#include "../vin.h"

struct vin_rpmsg {
	int magic;
	int type;
	int car_status;
	int arm_car_reverse_status;
	int rv_car_status;
};

enum vin_packet_type {
	RV_VIN_START,
	RV_VIN_START_ACK,
	RV_VIN_STOP,
	RV_VIN_STOP_ACK,

	ARM_VIN_START,
	ARM_VIN_START_ACK,
	ARM_VIN_STOP,
	ARM_VIN_STOP_ACK,

	ARM_RPMSG_READY,
};

enum vin_control {
	GET_BY_LINUX,
	GET_BY_RTOS,
	GET_BY_NONE,
};

struct vin_packet {
	u32 magic;
	u32 type;
};

struct rpmsg_vin_private {
	struct vin_packet ack;
	int status;
	struct rpmsg_endpoint *ept;
	bool rpmsg_connecting;
	enum vin_control control;
	hal_sem_t finish;
};

int vin_rpmsg_send(char *name, void *data, int len);
int vin_rpmsg_init(void);
int vin_rpmsg_status_send(int type);

#endif /*_VIN_RPMSG__H_*/