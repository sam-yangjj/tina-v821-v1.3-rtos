/*
 * A V4L2 driver for imx319 Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Cao FuMing <caofuming@allwinnertech.com>
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
#define V4L2_IDENT_SENSOR 0x0319

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The IMX319 i2c address
 */
#define I2C_ADDR 		0x20

#define SENSOR_NUM     0x2
#define SENSOR_NAME    "imx319_mipi"
#define SENSOR_NAME_2  "imx319_mipi_2"

#define DOL_RHS1	64
#define DOL_RATIO	16
#define V_SIZE		(0x44E)
#define OUT_SIZE	(V_SIZE + DOL_RHS1)

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];
static struct sensor_format_struct *current_switch_win[2];

/*
 * The default register settings
 */
static struct regval_list sensor_default_regs[] = {

};

/*
************* Sensor Information *****************
@Sensor	  			: imx319
@Date		  		: 2017-04-06
@Image size	  		: 3264x2448
@MCLK/PCLK	  		: 24MHz /288Mhz
@MIPI speed(Mbps)	: 720Mbps x 4Lane
@Frame Length	  	: 2492
@Line Length 	 	: 3800
@line Time       	: 13194
@Max Fps 	  		: 30.00fps
@Pixel order 	  	: Green 1st (=GB)
@X/Y-flip        	: X-flip
@BLC offset	   	 	: 64code
@Firmware Ver.   	: v1.0
**************************************************
*/
static struct regval_list sensor_3264_2448_30_regs[] = {
	{0x0100, 0x00},
	{0x0101, 0x00},
	{0x0136, 0x18},
	{0x0137, 0x00},
	{0x3c7e, 0x02},
	{0x3c7f, 0x00},

	{0x0112, 0x0a},
	{0x0113, 0x0a},
	{0x0114, 0x03},

	{0x0342, 0x0f},//linelength 3968
	{0x0343, 0x80},

	{0x0340, 0x0a},//framelength 2688
	{0x0341, 0x80},

	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x08},
	{0x0348, 0x0c},
	{0x0349, 0xcf},
	{0x034a, 0x09},
	{0x034b, 0x97},

	{0x0220, 0x00},
	{0x0221, 0x11},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x00},
	{0x0901, 0x11},
	{0x0902, 0x0a},
	{0x3140, 0x02},
	{0x3141, 0x00},
	{0x3f0d, 0x0a},
	{0x3f14, 0x01},
	{0x3f3c, 0x01},
	{0x3f4d, 0x01},
	{0x3f4c, 0x01},
	{0x4254, 0x7f},

	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x08},
	{0x040a, 0x00},
	{0x040b, 0x00},
	{0x040c, 0x0c},
	{0x040d, 0xc0},
	{0x040e, 0x09},
	{0x040f, 0x90},
	{0x034c, 0x0c},
	{0x034d, 0xc0},
	{0x034e, 0x09},
	{0x034f, 0x90},
	{0x3261, 0x00},
	{0x3264, 0x00},
	{0x3265, 0x10},

	{0x0301, 0x06},
	{0x0303, 0x04},
	{0x0305, 0x04},
	{0x0306, 0x01},
	{0x0307, 0x40},
	{0x0309, 0x0a},
	{0x030b, 0x02},
	{0x030d, 0x03},
	{0x030e, 0x00},
	{0x030f, 0xc8},
	{0x0310, 0x01},
	{0x0820, 0x0c},
	{0x0821, 0x80},
	{0x0822, 0x00},
	{0x0823, 0x00},

	{0x3e20, 0x01},
	{0x3e37, 0x01},
	{0x3e3b, 0x01},

	{0x3603, 0x00},

	{0x38a3, 0x01},
	{0x38a8, 0x00},
	{0x38a9, 0x00},
	{0x38aa, 0x00},
	{0x38ab, 0x00},

	{0x7485, 0x08},
	{0x7487, 0x0c},
	{0x7488, 0x0c},
	{0x7489, 0xc7},
	{0x748a, 0x09},
	{0x748b, 0x8b},

	{0x3234, 0x00},
	{0x3fc1, 0x00},

	{0x3235, 0x00},
	{0x3802, 0x00},
	{0x3143, 0x04},
	{0x360a, 0x00},
	{0x0b00, 0x00},

	{0x3237, 0x00},
	{0x3900, 0x00},
	{0x3901, 0x00},
	{0x3902, 0x00},
	{0x3904, 0x00},
	{0x3905, 0x00},
	{0x3906, 0x00},
	{0x3907, 0x00},
	{0x3908, 0x00},
	{0x3909, 0x00},
	{0x3912, 0x00},
	{0x3930, 0x00},
	{0x3931, 0x00},
	{0x3933, 0x00},
	{0x3934, 0x00},
	{0x3935, 0x00},
	{0x3936, 0x00},
	{0x3937, 0x00},
	{0x3614, 0x00},
	{0x3616, 0x0d},
	{0x3617, 0x56},
	{0x0106, 0x00},
	{0x0b05, 0x01},
	{0x0b06, 0x01},
	{0x3230, 0x00},
	{0x3602, 0x01},
	{0x3607, 0x01},
	{0x3c00, 0x00},
	{0x3c01, 0x48},
	{0x3c02, 0xc8},
	{0x3c03, 0xaa},
	{0x3c04, 0x91},
	{0x3c05, 0x54},
	{0x3c06, 0x26},
	{0x3c07, 0x20},
	{0x3c08, 0x51},
	{0x3d80, 0x00},
	{0x3f50, 0x00},
	{0x3f56, 0x00},
	{0x3f57, 0x30},

	{0x3f78, 0x01},
	{0x3f79, 0x18},
	{0x3f7c, 0x00},
	{0x3f7d, 0x00},
	{0x3fba, 0x00},
	{0x3fbb, 0x00},
	{0xa081, 0x00},
	{0xe014, 0x00},

	{0x0202, 0x0a},
	{0x0203, 0x6e},
	{0x0224, 0x01},
	{0x0225, 0xf4},

	{0x0204, 0x03},
	{0x0205, 0xc0},
	{0xe216, 0x00},
	{0x0217, 0x00},
	{0x020e, 0x01},
	{0x020f, 0x01},
	{0x0210, 0x10},
	{0x0211, 0x00},
	{0x0212, 0x01},
	{0xe213, 0x00},
	{0x0214, 0x01},
	{0x0215, 0x00},
	{0x0218, 0x01},
	{0x0219, 0x00},

	{0x0100, 0x01},
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
 * if not support the follow function ,retrun -EINVAL
 */

static int sensor_s_exp(int id, unsigned int exp_val)
{
	data_type explow, exphigh;

	exphigh = (unsigned char)((exp_val >> 12) & 0xff);
	explow  = (unsigned char)((exp_val >> 4)  & 0xff);

	sensor_write(id, 0x0203, explow);/* coarse integration time */
	sensor_write(id, 0x0202, exphigh);

	sensor_dbg("sensor_s_exp info->exp %d\n", exp_val);

	return 0;
}

static int sensor_s_gain(int id, int gain_val)
{
	data_type gainlow = 0;
	data_type gainhigh = 0;
	long gaindigi = 0;
	int gainana = 0;

	if (gain_val < 16) {
		gainana = 0;
		gaindigi = 256;
	} else if (gain_val <= 256) {
		gainana = 1024 - 16384/gain_val;
		gaindigi = 256;
	} else {
		gainana = 960;
		gaindigi = gain_val;
	}

	gainlow = (unsigned char)(gainana&0xff);
	gainhigh = (unsigned char)((gainana>>8)&0xff);

	sensor_write(id, 0x0205, gainlow);
	sensor_write(id, 0x0204, gainhigh);

	sensor_write(id, 0x020f, (unsigned char)(gaindigi & 0xff));
	sensor_write(id, 0x020e, (unsigned char)(gaindigi >> 8));

	sensor_dbg("sensor_set_gain = %d, Done!\n", gain_val);

	return 0;
}

static int imx319_sensor_vts;
static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, shutter, frame_length;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;


	shutter = exp_val>>4;
	if (shutter > imx319_sensor_vts - 18)
		frame_length = shutter;
	else
		frame_length = imx319_sensor_vts;

	sensor_write(id, 0x0104, 0x01);
	sensor_write(id, 0x0341, frame_length & 0xff);
	sensor_write(id, 0x0340, frame_length >> 8);
	sensor_s_exp(id, exp_val);
	sensor_s_gain(id, gain_val);
	sensor_write(id, 0x0104, 0x00);

	sensor_dbg("sensor_set_gain gain = %d, exp = %d, frame_length = %d Done!\n", gain_val, exp_val, frame_length);

	return 0;
}

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
		//hal_usleep(1000);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		hal_usleep(1000);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		hal_usleep(1000);
		vin_set_mclk_freq(id, MCLK);
		//hal_usleep(1000);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		vin_set_mclk(id, 0);
		vin_gpio_set_status(id, RESET, 0);
		vin_gpio_set_status(id, PWDN, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


static int sensor_set_ir(int id, int status)
{
#if 0
	vin_gpio_set_status(id, IR_CUT0, 1);
	vin_gpio_set_status(id, IR_CUT1, 1);
	vin_gpio_set_status(id, IR_LED, 1);
	switch (status) {
	case IR_DAY:
		vin_gpio_write(id, IR_CUT0, CSI_GPIO_HIGH);
		vin_gpio_write(id, IR_CUT1, CSI_GPIO_LOW);
		vin_gpio_write(id, IR_LED, CSI_GPIO_LOW);
		break;
	case IR_NIGHT:
		vin_gpio_write(id, IR_CUT0, CSI_GPIO_LOW);
		vin_gpio_write(id, IR_CUT1, CSI_GPIO_HIGH);
		vin_gpio_write(id, IR_LED, CSI_GPIO_HIGH);
		break;
	default:
		return -1;
	}
#endif
	return 0;
}

#if 0
static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(100, 120);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
#endif

static int sensor_detect(int id)
{
	data_type rdval = 0;
	int ret;

	ret = sensor_read(id, 0x0016, &rdval);
	sensor_dbg("0x0016 0x%x\n", rdval);

	if (rdval != (V4L2_IDENT_SENSOR >> 8) && (ret < 0 || rdval != 0x0)) {
		sensor_err(" read 0x0016 return 0x%2x\n", rdval);
		return -ENODEV;
	}

	ret = sensor_read(id, 0x0017, &rdval);
	sensor_dbg("0x0017 0x%x\n", rdval);
	if (rdval != (V4L2_IDENT_SENSOR & 0xff) && (ret < 0 || rdval != 0x0)) {
		sensor_err(" read 0x0016 return 0x%2x\n", rdval);
		return -ENODEV;
	}

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

static struct sensor_format_struct sensor_formats[] = {
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC
	{
	.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
	.width = 3264,
	.height = 2448,
	.hoffset = 0,
	.voffset = 0,
	.hts = 3968,
	.vts = 2688,
	.pclk = 320 * 1000 * 1000,
	.mipi_bps = 720 * 1000 * 1000,
	.fps_fixed = 30,
	.bin_factor = 1,
	.intg_min = 4 << 4,
	.intg_max = (2492 - 18) << 4,
	.gain_min = 1 << 4,
	.gain_max = 256 << 4,
	.switch_regs = sensor_3264_2448_30_regs,
	.switch_regs_size = ARRAY_SIZE(sensor_3264_2448_30_regs),
	},
#endif
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

static struct sensor_format_struct switch_sensor_formats[] = {
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	{
	.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
	.width = 3264,
	.height = 2448,
	.hoffset = 0,
	.voffset = 0,
	.hts = 3968,
	.vts = 2688,
	.pclk = 320 * 1000 * 1000,
	.mipi_bps = 720 * 1000 * 1000,
	.fps_fixed = 30,
	.bin_factor = 1,
	.intg_min = 4 << 4,
	.intg_max = (2492 - 18) << 4,
	.gain_min = 1 << 4,
	.gain_max = 256 << 4,
	.switch_regs = sensor_3264_2448_30_regs,
	.switch_regs_size = ARRAY_SIZE(sensor_3264_2448_30_regs),
	},
#endif
};

static struct sensor_format_struct *sensor_get_switch_format(int id, int isp_id, int vinc_id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	struct sensor_format_struct *sensor_format = NULL;

	if (current_switch_win[id])
		return current_switch_win[id];

	sensor_format = sensor_find_format(isp_id, vinc_id, switch_sensor_formats, ARRAY_SIZE(switch_sensor_formats));
	current_switch_win[id] = sensor_format;
	sensor_print("switch format fine wdr is %d, use fps is %d, width:%d, height:%d\n", sensor_format->wdr_mode, sensor_format->fps_fixed, sensor_format->width, sensor_format->height);

	return sensor_format;

#else
	return NULL;
#endif
}

static int sensor_g_mbus_config(int id, struct v4l2_mbus_config *cfg, struct mbus_framefmt_res *res)
{
	cfg->type = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{
	int ret = -1;
	struct sensor_exp_gain exp_gain;
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);

	sensor_dbg("ARRAY_SIZE(sensor_default_regs)=%d\n",
			(unsigned int)ARRAY_SIZE(sensor_default_regs));

	ret = sensor_write_array(id, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	if (current_win[id]->switch_regs) {
		sensor_dbg("%s, line:%d\n", __func__, __LINE__);
		ret = sensor_write_array(id, current_win[id]->switch_regs, current_win[id]->switch_regs_size);
		sensor_dbg("%s, line:%d\n", __func__, __LINE__);
	}
	if (ret < 0)
		return ret;

	imx319_sensor_vts = current_win[id]->vts;
//#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_FAST_CONVERGENCE
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, 2474 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 256 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, 2474 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 256 << 4);
	}
	sensor_s_exp_gain(id, &exp_gain);

	sensor_write(id, 0x3000, 0); /*sensor mipi stream on*/;
	sensor_dbg("%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

	sensor_dbg("%s on = %d, 3264 * 2448 fps: 30\n", __func__, enable);

	if (!enable)
		return 0;

	return sensor_reg_init(id, isp_id);
}

static int sensor_s_switch(int id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	// struct sensor_exp_gain exp_gain;
	// int ret = -1;

	// full_size = 1;
	// gc4663_sensor_vts = current_switch_win[id]->vts;

	// sensor_write(id, 0x0100, 0x00);
	// if (glb_exp_gain.exp_val && glb_exp_gain.gain_val) {
	// 	exp_gain.exp_val = glb_exp_gain.exp_val;
	// 	exp_gain.exp_mid_val = glb_exp_gain.exp_mid_val;
	// 	exp_gain.gain_val = glb_exp_gain.gain_val;
	// } else {
	// 	exp_gain.exp_val = 15408;
	// 	exp_gain.exp_mid_val = 16;
	// 	exp_gain.gain_val = 32;
	// }
	// //sensor_s_exp_gain(id, &exp_gain); /* make switch_regs exp&gain normal */

	// if (current_switch_win[id]->switch_regs)
	// 	ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	// else
	// 	sensor_err("cannot find 720p120fps to 2k%dfps reg\n", current_switch_win[id]->fps_fixed);
	// if (ret < 0)
	// 	return ret;
	// sensor_write(id, 0x0451, 0x00);/* RGAIN */
	// sensor_write(id, 0x0452, 0x00);/* BGAIN */
	// sensor_s_exp_gain(id, &exp_gain); /* make switch_regs firstframe  */
	// sensor_write(id, 0x03fe, 0x10);
	// sensor_write(id, 0x03fe, 0x00);
	// sensor_write(id, 0x0100, 0x09); /* stream_on */
#endif
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

struct sensor_fuc_core imx319_core  = {
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
