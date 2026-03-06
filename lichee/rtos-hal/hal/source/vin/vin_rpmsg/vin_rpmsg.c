/*
 * mipi subdev driver module
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "vin_rpmsg.h"

#define VIN_RPMSG_SERVICE_NAME			"sunxi,notify"
#define VIN_RPMSG_NOTIFY_MAX_LEN		32
#define VIN_RPMSG_MAX_LEN			(512 -16)

#define VIN_PACKET_MAGIC				0x10244025

static struct rpmsg_endpoint *vin_ept = NULL;

static struct rpmsg_vin_private rpmsg_vin = {
	.control = GET_BY_RTOS,
};

static void rpmsg_unbind_callback(struct rpmsg_endpoint *ept)
{
	vin_print("%s is destroyed\n", ept->name);
	vin_ept = NULL;
}

int vin_rpmsg_send(char *name, void *data, int len)
{
	uint8_t buf[VIN_RPMSG_MAX_LEN];

	if (!vin_ept) {
		vin_err("rpmsg notify module not init or init failed previously!\n");
		return -ENXIO;
	}

	memcpy(buf, name, strlen(name));
	memset(buf + strlen(name), 0, VIN_RPMSG_NOTIFY_MAX_LEN - strlen(name));

	if (data)
		memcpy(buf + VIN_RPMSG_NOTIFY_MAX_LEN, data, len);
	else
		len = 0;
#ifdef CONFIG_VIN_USE_PM
	if (pm_state_get() != PM_STATUS_RUNNING)
		return 0;
#endif
	openamp_rpmsg_send(vin_ept, buf, VIN_RPMSG_NOTIFY_MAX_LEN + len);

	return 0;
}

void vin_return_control(int type)
{
	char * msg = NULL;
	int len = 0;

	if(!type) {
		msg = "get";
		len = 3;
	} else {
		msg = "return";
		len = 6;
	}

	vin_rpmsg_send("twi2", msg, len);
	vin_rpmsg_send("twi3", msg, len);
	vin_rpmsg_send("isp0", msg, len);
	vin_rpmsg_send("scaler0", msg, len);
	vin_rpmsg_send("scaler4", msg, len);
	vin_rpmsg_send("scaler8", msg, len);
	vin_rpmsg_send("scaler12", msg, len);
	vin_rpmsg_send("vinc0", msg, len);
	vin_rpmsg_send("vinc4", msg, len);
	vin_rpmsg_send("vinc8", msg, len);
	vin_rpmsg_send("vinc12", msg, len);
	vin_rpmsg_send("vinc16", msg, len);
	vin_rpmsg_send("vinc17", msg, len);
}

int vin_rpmsg_status_on_off(enum vin_packet_type type)
{
	char * msg = NULL;
	int len = 0;

	if (RV_VIN_START == type) {
		msg = "rv_start";
		len = 8;
	} else if (RV_VIN_STOP == type) {
		msg = "rv_stop";
		len = 7;
	} else if (ARM_VIN_START_ACK == type) {
		msg = "arm_start";
		len = 9;
	} else if (ARM_VIN_STOP_ACK == type) {
		msg = "arm_stop";
		len = 8;
	} else {
		vin_err("no define packet type,exit!\n");
		return -1;
	}

	vin_rpmsg_send("vinc0", msg, len);
	vin_rpmsg_send("vinc4", msg, len);
	vin_rpmsg_send("vinc8", msg, len);
	vin_rpmsg_send("vinc12", msg, len);
	vin_rpmsg_send("vinc16", msg, len);
	vin_rpmsg_send("vinc17", msg, len);

	return 0;
}

static int rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	struct vin_packet *pack = data;
	int ret;

	if (pack->magic != VIN_PACKET_MAGIC || len != sizeof(*pack)) {
		vin_err("packet invalid magic or size %d %d %x\n", (int)len,
			(int)sizeof(*pack), pack->magic);
		return 0;
	}

	if (pack->type == RV_VIN_START_ACK) {
		rpmsg_vin.control = GET_BY_RTOS;
	} else if (pack->type == RV_VIN_STOP_ACK) {
		rpmsg_vin.control = GET_BY_NONE;
	} else if (pack->type == ARM_VIN_START) {
		rpmsg_vin.control = GET_BY_LINUX;
		vin_rpmsg_status_on_off(ARM_VIN_START_ACK);
	} else if (pack->type == ARM_VIN_STOP) {
		rpmsg_vin.control = GET_BY_NONE;
		vin_rpmsg_status_on_off(ARM_VIN_STOP_ACK);
	} else if (pack->type == ARM_RPMSG_READY) {
		ret = hal_sem_post(rpmsg_vin.finish);
		if (ret != 0) {
			vin_err("vin failed to post sem\n");
			ret = -1;
			return ret;
		}
	}

	vin_print("rtos ept callback type:%d control by:%d\n",
					pack->type, rpmsg_vin.control);

	return 0;
}

int rpmsg_vin_get_status(void)
{
	return rpmsg_vin.control;
}

int vin_rpmsg_sem_timewait(int ms)
{
	int ret;
	ret = hal_sem_timedwait(rpmsg_vin.finish, MS_TO_OSTICK(ms));

	return ret;
}

int vin_rpmsg_init(void)
{
	struct rpmsg_endpoint *ept;

	rpmsg_vin.finish = hal_sem_create(0);
	if (!rpmsg_vin.finish) {
		printf("rpmsg vin failed to create sem\r\n");
		return -1;
	}

	ept = openamp_ept_open(VIN_RPMSG_SERVICE_NAME, 0, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
					NULL, rpmsg_ept_callback, rpmsg_unbind_callback);
	if (ept == NULL) {
		vin_err("vin rpmsg Failed to Create Endpoint\r\n");
		return -1;
	}

	vin_ept = ept;
	rpmsg_vin.ept = ept;
	rpmsg_vin.control = GET_BY_RTOS;

	return 0;
}
