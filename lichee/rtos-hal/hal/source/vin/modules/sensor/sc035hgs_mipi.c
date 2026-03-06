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

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x00310b

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 120

/*
 * The sc035hgs_mipi sits on i2c with ID 0x60
 */
#define I2C_ADDR 0x60
#define SENSOR_NAME "sc035hgs_mipi"

struct cfg_array { /* coming later */
	struct regval_list *regs;
	int size;
};

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];

/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {

};

/* 640x480 RAW 120fps 24MHz */
static struct regval_list sensor_VGA_120fps_2lane_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x36f9, 0x80},
	{0x3000, 0x00},
	{0x3001, 0x00},
	{0x300f, 0x0f},
	{0x3018, 0x33},
	{0x3019, 0xfc},
	{0x301c, 0x78},
	{0x301f, 0x8c},
	{0x3031, 0x0a},
	{0x3037, 0x20},
	{0x303f, 0x01},
	{0x320c, 0x04},
	{0x320d, 0x70},
	{0x320e, 0x02},
	{0x320f, 0x10},
	{0x3217, 0x00},
	{0x3218, 0x00},
	{0x3220, 0x10},
	{0x3223, 0x48},
	{0x3226, 0x74},
	{0x3227, 0x07},
	{0x323b, 0x00},
	{0x3250, 0xf0},
	{0x3251, 0x02},
	{0x3252, 0x02},
	{0x3253, 0x08},
	{0x3254, 0x02},
	{0x3255, 0x07},
	{0x3304, 0x48},
	{0x3305, 0x00},
	{0x3306, 0x98},
	{0x3309, 0x50},
	{0x330a, 0x01},
	{0x330b, 0x18},
	{0x330c, 0x18},
	{0x330f, 0x40},
	{0x3310, 0x10},
	{0x3314, 0x6b},
	{0x3315, 0x30},
	{0x3316, 0x68},
	{0x3317, 0x14},
	{0x3329, 0x5c},
	{0x332d, 0x5c},
	{0x332f, 0x60},
	{0x3335, 0x64},
	{0x3344, 0x64},
	{0x335b, 0x80},
	{0x335f, 0x80},
	{0x3366, 0x06},
	{0x3385, 0x31},
	{0x3387, 0x39},
	{0x3389, 0x01},
	{0x33b1, 0x03},
	{0x33b2, 0x06},
	{0x33bd, 0xe0},
	{0x33bf, 0x10},
	{0x3621, 0xa4},
	{0x3622, 0x05},
	{0x3624, 0x47},
	{0x3630, 0x4a},
	{0x3631, 0x58},
	{0x3633, 0x52},
	{0x3635, 0x03},
	{0x3636, 0x25},
	{0x3637, 0x8a},
	{0x3638, 0x0f},
	{0x3639, 0x08},
	{0x363a, 0x00},
	{0x363b, 0x48},
	{0x363c, 0x86},
	{0x363e, 0xf8},
	{0x3640, 0x00},
	{0x3641, 0x01},
	{0x36ea, 0x36},
	{0x36eb, 0x0e},
	{0x36ec, 0x1e},
	{0x36ed, 0x00},
	{0x36fa, 0x36},
	{0x36fb, 0x10},
	{0x36fc, 0x00},
	{0x36fd, 0x00},
	{0x3908, 0x91},
	{0x391b, 0x81},
	{0x3d08, 0x01},
//exp
	{0x3e01, 0x01},
	{0x3e02, 0x0a},
	{0x3e03, 0x2b},
//gain
	{0x3e08, 0x1c},
	{0x3e09, 0x1f},
	{0x3e06, 0x00},
	{0x3e07, 0xea},
	{0x3f04, 0x03},
	{0x3f05, 0x80},
	{0x4500, 0x59},
	{0x4501, 0xc4},
	{0x4603, 0x00},
	{0x4800, 0x64},
	{0x4809, 0x01},
	{0x4810, 0x00},
	{0x4811, 0x01},
	{0x4837, 0x38},
	{0x5011, 0x00},
	{0x5988, 0x02},
	{0x598e, 0x04},
	{0x598f, 0x30},
	{0x36e9, 0x03},
	{0x36f9, 0x03},
	{0x0100, 0x01},
//	delay in sensor_reg_init
	{0x4418, 0x0a},
	{0x363d, 0x10},
	{0x4419, 0x80},
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */
__attribute__((__unused__)) static struct regval_list sensor_fmt_raw[] = {

};


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function , retrun -EINVAL
 */
static int sc035hgs_sensor_vts;
static int sensor_s_exp(int id, unsigned int exp_val)
{
	data_type explow, exphigh;
	data_type get_value;
//	static int frame_cnt = 0;
	if (exp_val > ((sc035hgs_sensor_vts - 6) << 4))
		exp_val = (sc035hgs_sensor_vts - 6) << 4;
	if (exp_val < 16)
		exp_val = 16;

	exphigh = (unsigned char)(exp_val >> 8);
	explow = (unsigned char)(exp_val & 0xFF);
	sensor_write(id, 0x3e01, exphigh);
	sensor_write(id, 0x3e02, explow);
	sensor_read(id, 0x3e01, &get_value);
	sensor_read(id, 0x3e02, &get_value);
	sensor_print("sensor_s_exp exp_val %d\n", exp_val);

	return 0;
}

static int sensor_s_gain(int id, unsigned int gain_val)
{
	data_type gain_ana = gain_val;
	data_type gain_dig_low = 0x80;
	data_type gain_dig_high = 0x00;
	data_type gain_high = 0;
	data_type gain_low = 0;
//	static int frame_cnt = 0;

	if (gain_val < 1 * 16)
		gain_val = 16;

//	gain_ana = 80;
	if (gain_ana < 0x20) {
		gain_high = 0x0;
		gain_low = gain_ana;
		sensor_write(id, 0x3314, 0x6b);
		sensor_write(id, 0x3317, 0x14);
		sensor_write(id, 0x3631, 0x58);
		sensor_write(id, 0x3630, 0x4a);
	} else if (gain_ana < 2 * 0x20) {
		gain_high = 0x01;
		gain_low = gain_ana >> 1;
		sensor_write(id, 0x3314, 0x4f);
		sensor_write(id, 0x3317, 0x10);
		sensor_write(id, 0x3631, 0x48);
		sensor_write(id, 0x3630, 0x4c);
	} else if (gain_ana < 4 * 0x20) {
		gain_high = 0x03;
		gain_low = gain_ana >> 2;
	} else if (gain_ana < 8 * 0x20) {
		gain_high = 0x07;
		gain_low = gain_ana >> 3;
		sensor_write(id, 0x3314, 0x74);
		sensor_write(id, 0x3317, 0x15);
		sensor_write(id, 0x3631, 0x48);
		sensor_write(id, 0x3630, 0x4c);
	} else {
		sensor_write(id, 0x3314, 0x74);
		sensor_write(id, 0x3317, 0x15);
		sensor_write(id, 0x3631, 0x48);
		sensor_write(id, 0x3630, 0x4c);
		gain_high = 0x07;
		gain_low = 0x1f;
		if (gain_ana < 16 * 0x20) {
			gain_dig_high = 0x00;
			gain_dig_low = gain_ana >> 1;
		} else if (gain_ana < 32 * 0x20) {
			gain_dig_high = 0x01;
			gain_dig_low = gain_ana >> 2;
		} else if (gain_ana < 64 * 0x20) {
			gain_dig_high = 0x03;
			gain_dig_low = gain_ana >> 3;
		} else {
			gain_dig_high = 0x03;
			gain_dig_low = 0xf8;
		}
	}

	sensor_write(id, 0x3e08, (unsigned char)(gain_high << 2));
	sensor_write(id, 0x3e09, (unsigned char)gain_low);
	sensor_write(id, 0x3e06, (unsigned char)gain_dig_high);
	sensor_write(id, 0x3e07, (unsigned char)gain_dig_low);
	sensor_print("sensor_s_gain gain_val %d\n", gain_val);

	return 0;
}

static int sensor_s_exp_gain(int id,
			     struct sensor_exp_gain *exp_gain)
{
	sensor_s_exp(id, exp_gain->exp_val);
	sensor_s_gain(id, exp_gain->gain_val);

	return 0;
}

// fix unused-function
__attribute__((__unused__)) static int sensor_get_fmt_mbus_core(int id, int *code)
{
	*code = MEDIA_BUS_FMT_SRGGB10_1X10;
	return 0;
}

static int sc035hgs_flip_status;
// fix unused-function
__attribute__((__unused__)) static int sensor_s_hflip(int id, int enable)
{
	data_type get_value;
	data_type set_value;

	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(id, 0x3221, &get_value);
	sensor_dbg("===> ready to hflip, regs_data = 0x%x\n", get_value);
	if (enable) {
		set_value = get_value | 0x06;
		sc035hgs_flip_status = get_value | 0x06;
	} else {
		set_value = get_value & 0xF9;
		sc035hgs_flip_status = get_value & 0xF9;
	}
	sensor_write(id, 0x3221, set_value);

	return 0;
}

// fix unused-function
__attribute__((__unused__)) static int sensor_s_vflip(int id, int enable)
{
	data_type get_value;
	data_type set_value;

	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(id, 0x3221, &get_value);
	sensor_print("===> ready to vflip, regs_data = 0x%x\n", get_value);
	if (enable) {
		set_value = get_value | 0x60;
		sc035hgs_flip_status = get_value | 0x60;
	} else {
		set_value = get_value & 0x9F;
		sc035hgs_flip_status = get_value & 0x9F;
	}
	sensor_write(id, 0x3221, set_value);

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
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(1000);
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(10000);

		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		vin_set_mclk(id, 0);
		vin_gpio_set_status(id, RESET, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#if 0
static int sensor_reset(int id, u32 val)
{
	sensor_dbg("%s val %d\n", __func__, val);

	switch (val) {
	case 0:
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(10000);
		break;
	case 1:
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(10000);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
#endif

static int sensor_detect(int id)
{
	unsigned int SENSOR_ID = 0;
	data_type rdval;
	int cnt = 0;

	sensor_read(id, 0x3107, &rdval);
	SENSOR_ID |= (rdval << 16);
	sensor_read(id, 0x3108, &rdval);
	SENSOR_ID |= (rdval << 8);
	sensor_read(id, 0x3109, &rdval);
	SENSOR_ID |= (rdval);
	printk(KERN_ERR "V4L2_IDENT_SENSOR = 0x%x, 0x3109=0x%x\n", SENSOR_ID, rdval);

	while ((SENSOR_ID != V4L2_IDENT_SENSOR) && (cnt < 5)) {
		sensor_read(id, 0x3107, &rdval);
		SENSOR_ID |= (rdval << 16);
		sensor_read(id, 0x3108, &rdval);
		SENSOR_ID |= (rdval << 8);
		sensor_read(id, 0x3109, &rdval);
		SENSOR_ID |= (rdval);
		sensor_dbg("retry = %d, V4L2_IDENT_SENSOR = %x\n",
			cnt, SENSOR_ID);
		cnt++;
		}
	if (SENSOR_ID != V4L2_IDENT_SENSOR)
		return -ENODEV;

	return 0;
}

static int sensor_init(int id)
{
	int ret;

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(id);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	return 0;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
		.width      = 640,
		.height     = 480,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1136,
		.vts        = 528,
		.pclk       = 72 * 1000 * 1000,
		.mipi_bps   = 360 * 1000 * 1000,
		.fps_fixed  = 120,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (528 - 6) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 1440 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_VGA_120fps_2lane_regs,
		.regs_size  = ARRAY_SIZE(sensor_VGA_120fps_2lane_regs),
	}
};

static struct sensor_format_struct *sensor_get_format(int id, int isp_id, int vinc_id)
{
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);
	struct sensor_format_struct *sensor_format = NULL;
	int wdr_on = isp_get_cfg[ispid].sensor_wdr_on;
	int fps = isp_get_cfg[ispid].sensor_get_fps;
	int i;
	if (current_win[id])
		return current_win[id];
	for (i = 0; i < ARRAY_SIZE(sensor_formats); i++) {
		if (sensor_formats[i].wdr_mode == wdr_on) {
			if (sensor_formats[i].fps_fixed == fps) {
				sensor_format = &sensor_formats[i];
				sensor_print("fine wdr is %d, fine fps is %d\n", wdr_on, fps);
				goto done;
			}
		}
	}
	if (sensor_format == NULL) {
		for (i = 0; i < ARRAY_SIZE(sensor_formats); i++) {
			if (sensor_formats[i].wdr_mode == wdr_on) {
				sensor_format = &sensor_formats[i];
				isp_get_cfg[ispid].sensor_get_fps = sensor_format->fps_fixed;
				sensor_print("fine wdr is %d, use fps is %d\n", wdr_on, sensor_format->fps_fixed);
				goto done;
			}
		}
	}
	if (sensor_format == NULL) {
		sensor_format = &sensor_formats[0];
		isp_get_cfg[ispid].sensor_wdr_on = sensor_format->wdr_mode;
		isp_get_cfg[ispid].sensor_get_fps = sensor_format->fps_fixed;
		sensor_print("use wdr is %d, use fps is %d\n", sensor_format->wdr_mode, sensor_format->fps_fixed);
	}
done:
	current_win[id] = sensor_format;
	return sensor_format;
#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
	if (current_win[id])
		return current_win[id];

	current_win[id] = &sensor_formats[0];
	sensor_print("fine wdr is %d, fps is %d\n", sensor_formats[0].wdr_mode, sensor_formats[0].fps_fixed);
	return &sensor_formats[0];
#endif
}

static int sensor_g_mbus_config(int id, struct v4l2_mbus_config *cfg, struct mbus_framefmt_res *res)
{
	//struct sensor_info *info = to_state(sd);

	cfg->type  = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	res->res_time_hs = 0x28;

	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{
	int ret = 0;
	struct sensor_exp_gain exp_gain;

	ret = sensor_write_array(id, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	if (current_win[id]->regs)
		ret = sensor_write_array(id, current_win[id]->regs, current_win[id]->regs_size);
	if (ret < 0) {
		return ret;
	}

	sc035hgs_sensor_vts = current_win[id]->vts;
#if defined ISP_PARA_READ
	exp_gain.exp_val = clamp(*((unsigned int *)ISP_PARA_READ + 3), 16, 522 << 4);
	exp_gain.gain_val = clamp(*((unsigned int *)ISP_PARA_READ + 2), 16, 110 << 4);
#else
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_FAST_CONVERGENCE
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, 522 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 110 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, 522 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 110 << 4);
	}
#else
	exp_gain.exp_val = 5000;
	exp_gain.gain_val = 16;
#endif
#endif
	sensor_s_exp_gain(id, &exp_gain);
	sensor_write(id, 0x3e, 0x91);

	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

	sensor_dbg("%s on = %d\n", __func__, enable);

	if (!enable)
		return 0;

	return sensor_reg_init(id, isp_id);
}

static int sensor_test_i2c(int id)
{
	int ret;
	sensor_power(id, PWR_ON);
	ret = sensor_init(id);
	sensor_power(id, PWR_OFF);

	return ret;
}

struct sensor_fuc_core sc035hgs_core  = {
	.g_mbus_config = sensor_g_mbus_config,
	.sensor_test_i2c = sensor_test_i2c,
	.sensor_power = sensor_power,
	.s_stream = sensor_s_stream,
	.s_exp_gain = sensor_s_exp_gain,
	.sensor_g_format = sensor_get_format,
};
