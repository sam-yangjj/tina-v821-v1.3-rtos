/*
 * A V4L2 driver for Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <hal_timer.h>

#include "../../vin_mipi/combo_common.h"
#include "camera.h"
#include "../../utility/sunxi_camera_v2.h"
#include "../../utility/media-bus-format.h"
#include "../../utility/vin_supply.h"

#define MCLK              (27*1000*1000)
#define V4L2_IDENT_SENSOR  0x5028

/*
 * The TP9950 i2c address
 */
#define I2C_ADDR 0x88

#define SENSOR_NUM 0x1
#define SENSOR_NAME "tp9950"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];
static struct sensor_format_struct *current_switch_win[2];

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list tp9950_yuv_1080p_HD25[] = {
	{ 0x40, 0x08 },/* MIPI page */
	{ 0x01, 0xf8 },
	{ 0x02, 0x01 },
	{ 0x08, 0x03 },
	{ 0x20, 0x12 },
	{ 0x39, 0x00 },
	{ 0x40, 0x00 },/* decoder page */
	{ 0x4c, 0x40 },
	{ 0x4e, 0x00 },
	{ 0x27, 0x2d },
	{ 0xfd, 0x80 },
	{ 0x02, 0x40 },
	{ 0x07, 0xc0 },
	{ 0x0b, 0xc0 },
	{ 0x0c, 0x03 },
	{ 0x0d, 0x50 },
	{ 0x15, 0x03 },
	{ 0x16, 0xd2 },
	{ 0x17, 0x80 },
	{ 0x18, 0x29 },
	{ 0x19, 0x38 },
	{ 0x1a, 0x47 },
	{ 0x1c, 0x0a }, /* 1920*1080, 25fps */
	{ 0x1d, 0x50 },
	{ 0x20, 0x30 },
	{ 0x21, 0x84 },
	{ 0x22, 0x36 },
	{ 0x23, 0x3c },
	{ 0x2b, 0x60 },
	{ 0x2c, 0x0a },
	{ 0x2d, 0x30 },
	{ 0x2e, 0x70 },
	{ 0x30, 0x48 },
	{ 0x31, 0xbb },
	{ 0x32, 0x2e },
	{ 0x33, 0x90 },
	{ 0x35, 0x05 },
	{ 0x38, 0x00 },
	{ 0x39, 0x1C },
	{ 0x02, 0x44 },
	{ 0x0d, 0x73 },
	{ 0x15, 0x01 },
	{ 0x16, 0xf0 },
	{ 0x18, 0x2a },
	{ 0x20, 0x3c },
	{ 0x21, 0x46 },
	{ 0x25, 0xfe },
	{ 0x26, 0x0d },
	{ 0x2c, 0x3a },
	{ 0x2d, 0x54 },
	{ 0x2e, 0x40 },
	{ 0x30, 0xa5 },
	{ 0x31, 0x86 },
	{ 0x32, 0xfb },
	{ 0x33, 0x60 },
	{ 0x40, 0x08 },
	{ 0x23, 0x02 },
	{ 0x13, 0x04 },
	{ 0x14, 0x46 },
	{ 0x15, 0x09 },
	{ 0x25, 0x10 },
	{ 0x26, 0x02 },
	{ 0x27, 0x16 },
	{ 0x10, 0x88 },
	{ 0x10, 0x08 },
	{ 0x23, 0x00 },
	{ 0x40, 0x00 },

};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {

};


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function , retrun -EINVAL
 */

static int sensor_s_exp(int id, unsigned int exp_val)
{
	int tmp_exp_val = exp_val / 16;

	sensor_dbg("exp_val:%d\n", exp_val);
	sensor_write(id, 0x03, (tmp_exp_val >> 8) & 0xFF);
	sensor_write(id, 0x04, (tmp_exp_val & 0xFF));

	return 0;
}

static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	return 0;
}

/*
* Stuff that knows about the sensor.
*/
static int sensor_power(int id, int on)
{
	if (on && (sensor_power_count[id])++ > 0)
		return 0;
	else if (!on && (sensor_power_count[id] == 0 || --(sensor_power_count[id]) > 0))
		return 0;

	switch (on) {
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		//vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(1000);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(1000);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!do nothing\n");
		vin_set_mclk(id, 0);
		hal_usleep(1000);
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_set_ir(int id, int status)
{
	sensor_dbg("sensor_set_ir\n");
	return 0;
}

static int sensor_detect(int id)
{
	data_type rdval, rdval1, rdval2;
	int cnt = 0;

	rdval = 0;
	rdval1 = 0;
	rdval2 = 0;

	sensor_read(id, 0xfe, &rdval1);
	sensor_read(id, 0xff, &rdval2);
	rdval = ((rdval2<<8) & 0xff00) | rdval1;
	sensor_err("V4L2_IDENT_SENSOR = 0x%x\n", rdval);

	while ((rdval != V4L2_IDENT_SENSOR) && (cnt < 5)) {
		sensor_read(id, 0xfe, &rdval1);
		sensor_read(id, 0xff, &rdval2);
		rdval = ((rdval2<<8) & 0xff00) | rdval1;
		sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n", cnt,
				 rdval);
		cnt++;
	}

	if (rdval != V4L2_IDENT_SENSOR){
		return -ENODEV;
	}
	return 0;
}

static int sensor_init(int id)
{
	int ret;
	sensor_err("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(id);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}
	sensor_err("sensor_init end\n");
	return 0;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.mbus_code  = MEDIA_BUS_FMT_UYVY8_2X8,
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
//		.hts        = 1696,
//		.vts        = 525,
//		.pclk       = 106848000,
//		.mipi_bps   = 648 * 1000 * 1000,
		.fps_fixed  = 25,
//		.bin_factor = 1,
//		.intg_min   = 1 << 4,
//		.intg_max   = (525 - 16) << 4,
//		.gain_min   = 1 << 4,
//		.gain_max   = 110 << 4,
//		.offs_h     = 0,
//		.offs_v     = 0,
		.regs	    = tp9950_yuv_1080p_HD25,
		.regs_size  = ARRAY_SIZE(tp9950_yuv_1080p_HD25),
	}
};

static struct sensor_format_struct *sensor_get_format(int id, int isp_id, int vinc_id)
{
	if (current_win[id])
		return current_win[id];

	current_win[id] = &sensor_formats[0];
	sensor_print("fine wdr is %d, fps is %d\n", sensor_formats[0].wdr_mode, sensor_formats[0].fps_fixed);
	return &sensor_formats[0];
}

static struct sensor_format_struct switch_sensor_formats[] = {

};

static struct sensor_format_struct *sensor_get_switch_format(int id, int isp_id, int vinc_id)
{
	return NULL;
}

static int sensor_g_mbus_config(int id, struct v4l2_mbus_config *cfg, struct mbus_framefmt_res *res)
{
	//struct sensor_info *info = to_state(sd);

	cfg->type  = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{
	int ret = 0;
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);
	struct sensor_exp_gain exp_gain;
	sensor_err("sensor id %d-------------\n",id);
	ret = sensor_write_array(id, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	if (current_win[id]->regs)
		ret = sensor_write_array(id, current_win[id]->regs, current_win[id]->regs_size);
	if (ret < 0)
		return ret;

	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
    sensor_err("tp9950 sensor_s_stream----\n");
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

	sensor_dbg("%s on = %d, 1920*1080 fps: 25\n", __func__, enable);

	if (!enable)
		return 0;

	return sensor_reg_init(id, isp_id);
}

static int sensor_s_switch(int id)
{
	return 0;
}

static int sensor_test_i2c(int id)
{
	int ret;
	sensor_power(id, PWR_ON);
	ret = sensor_init(id);
	sensor_power(id, PWR_OFF);

	return ret;
}

struct sensor_fuc_core tp9950_mipi_core  = {
	.g_mbus_config = sensor_g_mbus_config,
	.sensor_test_i2c = sensor_test_i2c,
	.sensor_power = sensor_power,
	.s_ir_status = sensor_set_ir,
	.s_stream = sensor_s_stream,
	.s_switch = sensor_s_switch,
	.s_exp_gain = sensor_s_exp_gain,
	.sensor_g_format = sensor_get_format,
	.sensor_g_switch_format = sensor_get_switch_format,
};
