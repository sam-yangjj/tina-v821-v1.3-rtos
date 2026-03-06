/*
 * A V4L2 driver for Raw cameras.
 *
 * Copyright (c) 2022 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Liu Chensheng <liuchensheng@allwinnertech.com>
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

#define MCLK            (24*1000*1000)
#define V4L2_IDENT_SENSOR   0xce1f

#define SENSOR_FRAME_RATE   30

/*
 * The SC500AI i2c address
 */
#define I2C_ADDR 0x60

#define SENSOR_NUM 0x2
#define SENSOR_NAME "sc500ai_mipi"
#define SENSOR_NAME_2 "sc500ai_mipi_2"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_exp_gain glb_exp_gain;
static struct sensor_format_struct *current_win[2];
static struct sensor_format_struct *current_switch_win[2];
static int sensor_wdr_mode[2];

#if defined CONFIG_ISP_FAST_CONVERGENCE  || defined CONFIG_ISP_HARD_LIGHTADC
static int full_size = 0;
#else // CONFIG_ISP_ONLY_HARD_LIGHTADC & CONFIG_ISP_READ_THRESHOLD
static int full_size = 1;
#endif

#define SENSOR_30FPS 1
/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#if SENSOR_30FPS
static struct regval_list sensor_2880x1620p30_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x36f9, 0x80},
	{0x301f, 0xdd},
	{0x320e, 0x08},
	{0x320f, 0x34},
	{0x3250, 0x40},
	{0x3253, 0x0a},
	{0x3301, 0x0a},
	{0x3302, 0x18},
	{0x3303, 0x10},
	{0x3304, 0x60},
	{0x3306, 0x60},
	{0x3308, 0x10},
	{0x3309, 0x70},
	{0x330a, 0x00},
	{0x330b, 0xf0},
	{0x330d, 0x18},
	{0x330e, 0x20},
	{0x330f, 0x02},
	{0x3310, 0x02},
	{0x331c, 0x04},
	{0x331e, 0x51},
	{0x331f, 0x61},
	{0x3320, 0x09},
	{0x3333, 0x10},
	{0x334c, 0x08},
	{0x3356, 0x09},
	{0x3364, 0x17},
	{0x336d, 0x03},
	{0x3390, 0x08},
	{0x3391, 0x18},
	{0x3392, 0x38},
	{0x3393, 0x0a},
	{0x3394, 0x20},
	{0x3395, 0x20},
	{0x3396, 0x08},
	{0x3397, 0x18},
	{0x3398, 0x38},
	{0x3399, 0x0a},
	{0x339a, 0x20},
	{0x339b, 0x20},
	{0x339c, 0x20},
	{0x33ac, 0x10},
	{0x33ae, 0x10},
	{0x33af, 0x19},
	{0x360f, 0x01},
	{0x3622, 0x03},
	{0x363a, 0x1f},
	{0x363c, 0x40},
	{0x3651, 0x7d},
	{0x3670, 0x0a},
	{0x3671, 0x07},
	{0x3672, 0x17},
	{0x3673, 0x1e},
	{0x3674, 0x82},
	{0x3675, 0x64},
	{0x3676, 0x66},
	{0x367a, 0x48},
	{0x367b, 0x78},
	{0x367c, 0x58},
	{0x367d, 0x78},
	{0x3690, 0x34},
	{0x3691, 0x34},
	{0x3692, 0x54},
	{0x369c, 0x48},
	{0x369d, 0x78},
	{0x36ea, 0x39},
	{0x36eb, 0x0c},
	{0x36ec, 0x1a},
	{0x36ed, 0x14},
	{0x36fa, 0x39},
	{0x36fb, 0x35},
	{0x36fc, 0x00},
	{0x36fd, 0x34},
	{0x3904, 0x04},
	{0x3908, 0x41},
	{0x391d, 0x04},
	{0x39c2, 0x30},
	{0x3e00, 0x01},
	{0x3e01, 0x05},
	{0x3e02, 0xe0},
	{0x3e16, 0x00},
	{0x3e17, 0x80},
	{0x4500, 0x88},
	{0x4509, 0x20},
	{0x4800, 0x04},
	{0x4837, 0x1f},
	{0x5799, 0x00},
	{0x59e0, 0x60},
	{0x59e1, 0x08},
	{0x59e2, 0x3f},
	{0x59e3, 0x18},
	{0x59e4, 0x18},
	{0x59e5, 0x3f},
	{0x59e7, 0x02},
	{0x59e8, 0x38},
	{0x59e9, 0x20},
	{0x59ea, 0x0c},
	{0x59ec, 0x08},
	{0x59ed, 0x02},
	{0x59ee, 0xa0},
	{0x59ef, 0x08},
	{0x59f4, 0x18},
	{0x59f5, 0x10},
	{0x59f6, 0x0c},
	{0x59f9, 0x02},
	{0x59fa, 0x18},
	{0x59fb, 0x10},
	{0x59fc, 0x0c},
	{0x59ff, 0x02},
	{0x36e9, 0x57},
	{0x36f9, 0x53},
	//{0x0100, 0x01},
};

static struct regval_list sensor_2880x1620p30_wdr_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x36f9, 0x80},
	{0x301f, 0x3b},
	{0x3106, 0x01},
	{0x320e, 0x0d},
	{0x320f, 0x30},
	{0x3220, 0x53},
	{0x3250, 0xff},
	{0x3253, 0x0a},
	{0x3301, 0x0b},
	{0x3302, 0x20},
	{0x3303, 0x10},
	{0x3304, 0x70},
	{0x3306, 0x50},
	{0x3308, 0x18},
	{0x3309, 0x80},
	{0x330a, 0x00},
	{0x330b, 0xe8},
	{0x330d, 0x30},
	{0x330e, 0x30},
	{0x330f, 0x02},
	{0x3310, 0x02},
	{0x331c, 0x08},
	{0x331e, 0x61},
	{0x331f, 0x71},
	{0x3320, 0x11},
	{0x3333, 0x10},
	{0x334c, 0x10},
	{0x3356, 0x11},
	{0x3364, 0x17},
	{0x336d, 0x03},
	{0x3390, 0x08},
	{0x3391, 0x18},
	{0x3392, 0x38},
	{0x3393, 0x0a},
	{0x3394, 0x0a},
	{0x3395, 0x12},
	{0x3396, 0x08},
	{0x3397, 0x18},
	{0x3398, 0x38},
	{0x3399, 0x0a},
	{0x339a, 0x0a},
	{0x339b, 0x0a},
	{0x339c, 0x12},
	{0x33ac, 0x10},
	{0x33ae, 0x20},
	{0x33af, 0x21},
	{0x360f, 0x01},
	{0x3621, 0xe8},
	{0x3622, 0x06},
	{0x3630, 0x82},
	{0x3633, 0x33},
	{0x3634, 0x64},
	{0x3637, 0x50},
	{0x363a, 0x1f},
	{0x363c, 0x40},
	{0x3651, 0x7d},
	{0x3670, 0x0a},
	{0x3671, 0x06},
	{0x3672, 0x16},
	{0x3673, 0x17},
	{0x3674, 0x82},
	{0x3675, 0x62},
	{0x3676, 0x44},
	{0x367a, 0x48},
	{0x367b, 0x78},
	{0x367c, 0x48},
	{0x367d, 0x58},
	{0x3690, 0x34},
	{0x3691, 0x34},
	{0x3692, 0x54},
	{0x369c, 0x48},
	{0x369d, 0x78},
	{0x36ea, 0x37},
	{0x36eb, 0x04},
	{0x36ec, 0x0a},
	{0x36ed, 0x24},
	{0x36fa, 0x37},
	{0x36fb, 0x04},
	{0x36fc, 0x00},
	{0x36fd, 0x26},
	{0x3904, 0x04},
	{0x3908, 0x41},
	{0x391f, 0x10},
	{0x39c2, 0x30},
	{0x3e00, 0x01},
	{0x3e01, 0x8c},
	{0x3e02, 0x00},
	{0x3e04, 0x18},
	{0x3e05, 0xc0},
	{0x3e23, 0x00},
	{0x3e24, 0xcc},
	{0x4500, 0x88},
	{0x4509, 0x20},
	{0x4800, 0x04},
	{0x4837, 0x14},
	{0x4853, 0xfd},
	{0x36e9, 0x53},
	{0x36f9, 0x53},
	//{0x0100, 0x01},
};
#endif

#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
static struct regval_list sensor_1440_400p130_regs[] = {
	//h sum + v binning
};
#endif
/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {

};

static int sensor_s_exp(int id, unsigned int exp_val, unsigned int exp_short)
{
	unsigned int explow, expmid, exphigh;
	int isp_wdr_mode;

	if (!full_size) {
		isp_wdr_mode = ISP_NORMAL_MODE;
	} else {
		if (sensor_wdr_mode[id])
			isp_wdr_mode = ISP_DOL_WDR_MODE;
		else
			isp_wdr_mode = ISP_NORMAL_MODE;
	}

	if (exp_val < 16) {
		exp_val = 16;
	}
	if (exp_short < 16) {
		exp_short = 16;
	}

	if (isp_wdr_mode == ISP_DOL_WDR_MODE) {
		exphigh = (unsigned char) (0x0f & (exp_val >> 15));
		expmid = (unsigned char) (0xff & (exp_val >> 7));
		explow = (unsigned char) (0xf0 & (exp_val << 1));

		sensor_write(id, 0x3e02, explow);
		sensor_write(id, 0x3e01, expmid);
		sensor_write(id, 0x3e00, exphigh);

		expmid = (unsigned char) (0xff & (exp_short >> 7));
		explow = (unsigned char) (0xf0 & (exp_short << 1));

		sensor_write(id, 0x3e05, explow);
		sensor_write(id, 0x3e04, expmid);
	} else {
		exphigh = (unsigned char) (0x0f & (exp_val >> 15));
		expmid = (unsigned char) (0xff & (exp_val >> 7));
		explow = (unsigned char) (0xf0 & (exp_val << 1));

		sensor_write(id, 0x3e02, explow);
		sensor_write(id, 0x3e01, expmid);
		sensor_write(id, 0x3e00, exphigh);
	}

	return 0;
}

static int sensor_s_gain(int id, int gain_val)
{
	int gainlow = 0;
	int gainhigh = 0;
	int gaindiglow = 0x80;
	int gaindighigh = 0x00;
	int isp_wdr_mode;

	if (!full_size) {
		isp_wdr_mode = ISP_NORMAL_MODE;
	} else {
		if (sensor_wdr_mode[id])
			isp_wdr_mode = ISP_DOL_WDR_MODE;
		else
			isp_wdr_mode = ISP_NORMAL_MODE;
	}

	int gainana = gain_val << 2;

	if (gainana <= 96) {
		gainhigh = 0x03; //  x64
		gainlow = gainana;
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana <= 192) {// 3.008 * 64=192.512
		gainhigh = 0x23;
		gainlow = gainana * 63 / (96) + 1;
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana <= 385) {// 6.017 * 64= 385.088
		gainhigh = 0x27;
		gainlow = gainana * 63 / (192) + 1;
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana <= 770) {// 12.033 * 64= 770.112
		gainhigh = 0x2F;
		gainlow = gainana * 63 / (384) + 1;
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana <= 1540) { // 24.067 * 64= 1540.288
		gainhigh = 0x3F;
		gainlow = gainana * 63 / (772) + 1;
		gaindighigh = 0x00;
		gaindiglow = 0x80;
	} else if (gainana <= 2 * 1540) { // digital gain, 24.067 * 64 * 2
		gainhigh = 0x3F;
		gainlow = 0x7F;
		gaindighigh = 0x00;
		gaindiglow = 127 * gainana  / (1540) + 1 ;
		gaindiglow = gaindiglow > 0xFE ? 0xFE : gaindiglow;
	} else if (gainana <= 4 * 1540) {// 24.067 * 64 * 4
	     gainhigh = 0x3F;
	     gainlow = 0x7F;
	     gaindighigh = 0x01;
	     gaindiglow = 127 * gainana / (1540 * 2) +1  ;
		 gaindiglow = gaindiglow > 0xFE ? 0xFE : gaindiglow;
	} else if (gainana <= 8 * 1540) { // 24.067 * 64 * 8
	     gainhigh = 0x3F;
	     gainlow = 0x7F;
	     gaindighigh = 0x03;
	     gaindiglow = 127 * gainana  / (1540 * 4) +1 ;
		 gaindiglow = gaindiglow > 0xFE ? 0xFE : gaindiglow;
	} else if (gainana <= 16 * 1540) { // 24.067 * 64 * 16
	     gainhigh = 0x3F;
	     gainlow = 0x7F;
	     gaindighigh = 0x07;
	     gaindiglow = 127 * gainana  / (1540 * 8) + 1;
		 gaindiglow = gaindiglow > 0xFE ? 0xFE : gaindiglow;
	} else if (gainana <= 3175 * 1540 / 100) { // 24.067 * 64 * 32
	     gainhigh = 0x3F;
	     gainlow = 0x7F;
	     gaindighigh = 0x0f;
	     gaindiglow = 127 * gainana  / (1540 * 16) + 1;
		 gaindiglow = gaindiglow > 0xFE ? 0xFE : gaindiglow;
	} else {
		 gainhigh = 0x3F;
	     gainlow = 0x7F;
	     gaindighigh = 0x0f;
	     gaindiglow = 0xFE;
	}

	if (isp_wdr_mode == ISP_DOL_WDR_MODE) {
		sensor_write(id, 0x3e13, (unsigned char)gainlow);
		sensor_write(id, 0x3e12, (unsigned char)gainhigh);
		sensor_write(id, 0x3e11, (unsigned char)gaindiglow);
		sensor_write(id, 0x3e10, (unsigned char)gaindighigh);
	}

	sensor_write(id, 0x3e09, (unsigned char)gainlow);
	sensor_write(id, 0x3e08, (unsigned char)gainhigh);
	sensor_write(id, 0x3e07, (unsigned char)gaindiglow);
	sensor_write(id, 0x3e06, (unsigned char)gaindighigh);

	return 0;

}

static int sc500ai_sensor_vts = 2100;
static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, short_exp_val, gain_val;
	int shutter = 0, frame_length = 0;
	int isp_wdr_mode;

	if (!full_size) {
		isp_wdr_mode = ISP_NORMAL_MODE;
	} else {
		if (sensor_wdr_mode[id])
			isp_wdr_mode = ISP_DOL_WDR_MODE;
		else
			isp_wdr_mode = ISP_NORMAL_MODE;
	}

	exp_val = exp_gain->exp_val;
	short_exp_val = exp_gain->exp_mid_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	if (isp_wdr_mode == ISP_DOL_WDR_MODE) {
		shutter = (exp_val + short_exp_val) >> 4;
	} else {
		shutter = exp_val >> 4;
	}
	if (shutter > sc500ai_sensor_vts - 5)
		frame_length = shutter + 5;
	else
		frame_length = sc500ai_sensor_vts;
	sensor_write(id, 0x320f, (frame_length & 0xff));
	sensor_write(id, 0x320e, (frame_length >> 8));

	//sensor_write(id, 0x3812, 0x00);//group_hold
	sensor_s_exp(id, exp_val, short_exp_val);
	sensor_s_gain(id, gain_val);
	//sensor_write(id, 0x3812, 0x30);

	glb_exp_gain.exp_val = exp_val;
	glb_exp_gain.exp_mid_val = short_exp_val;
	glb_exp_gain.gain_val = gain_val;

	sensor_print("gain_val:%d, exp_val:%d, exp_mid_val:%d\n", gain_val, exp_val, short_exp_val);
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
	return 0;
}

static int sensor_detect(int id){
	data_type rdval;

	sensor_read(id, 0x3107, &rdval);
	sensor_print("0x3107 = 0x%x\n", rdval);
	if (rdval != (V4L2_IDENT_SENSOR>>8))
		return -ENODEV;
	sensor_read(id, 0x3108, &rdval);
	sensor_print("0x3108 = 0x%x\n", rdval);
	if (rdval != (V4L2_IDENT_SENSOR&0xff))
		return -ENODEV;

	sensor_dbg("Done!\n");

	return 0;
}

static int sensor_init(int id)
{
	int ret;

	sensor_dbg("sensor_init\n");
	ret = sensor_detect(id);
	if(ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	return 0;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC
#if SENSOR_30FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 2880,
		.height     = 1620,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3200,
		.vts        = 2100,
		.pclk       = 201600000,
		.mipi_bps   = 504000000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (2100 - 5) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 128 << 4,
		.field      = V4L2_FIELD_NONE,
		.regs       = sensor_2880x1620p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_2880x1620p30_regs),
	},

	{
		.mbus_code  = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 2880,
		.height     = 1620,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3200,
		.vts        = 3376,
		.pclk       = 324096000,
		.mipi_bps   = 810000000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.wdr_mode   = ISP_DOL_WDR_MODE,
		.intg_min   = 3 << 4,
		.intg_max   = (3376 - 204 - 9) << 4,
		.intg_mid_min = 3 << 4,
		.intg_mid_max = (204 - 7) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 128 << 4,
		.field      = V4L2_FIELD_NONE,
		.regs       = sensor_2880x1620p30_wdr_regs,
		.regs_size  = ARRAY_SIZE(sensor_2880x1620p30_wdr_regs),
	},
#endif

#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1440,
		.height     = 400,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3028,
		.vts        = 439,
		.pclk       = 172800000,
		.mipi_bps   = 432 * 1000 * 1000,
		.fps_fixed  = 130,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (439 - 8) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 512 << 4,
		.regs       = sensor_1440_400p130_regs,
		.regs_size  = ARRAY_SIZE(sensor_1440_400p130_regs),
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
#if SENSOR_30FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 2880,
		.height     = 1620,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3200,
		.vts        = 2100,
		.pclk       = 201600000,
		.mipi_bps   = 504000000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (2100 - 5) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 128 << 4,
		.switch_regs       = sensor_2880x1620p30_regs,
		.switch_regs_size  = ARRAY_SIZE(sensor_2880x1620p30_regs),
	},

	{
		.mbus_code  = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 2880,
		.height     = 1620,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3200,
		.vts        = 3376,
		.pclk       = 324096000,
		.mipi_bps   = 810000000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.wdr_mode   = ISP_DOL_WDR_MODE,
		.intg_min   = 3 << 4,
		.intg_max   = (3376 - 204 - 9) << 4,
		.intg_mid_min = 3 << 4,
		.intg_mid_max = (204 - 7) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 128 << 4,
		.switch_regs       = sensor_2880x1620p30_wdr_regs,
		.switch_regs_size  = ARRAY_SIZE(sensor_2880x1620p30_wdr_regs),
	},
#endif
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
	cfg->type  = V4L2_MBUS_CSI2;
	int isp_wdr_mode;

	if (!full_size) {
		isp_wdr_mode = ISP_NORMAL_MODE;
	} else {
		if (sensor_wdr_mode[id])
			isp_wdr_mode = ISP_DOL_WDR_MODE;
		else
			isp_wdr_mode = ISP_NORMAL_MODE;
	}

	if (isp_wdr_mode == ISP_DOL_WDR_MODE) {
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
		res->res_combo_mode = MIPI_VC_WDR_MODE;
		res->res_time_hs = 0x20;
		res->deskew = 0x2;
	} else {
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
		res->res_time_hs = 0x20;
		res->deskew = 0x2;
	}

	res->res_wdr_mode = isp_wdr_mode;

	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{
	int ret;
	struct sensor_exp_gain exp_gain;
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);

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

	sc500ai_sensor_vts = current_win[id]->vts;
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, sc500ai_sensor_vts << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 120 << 4);
		exp_gain.exp_mid_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 4), 16, sc500ai_sensor_vts << 4);
		exp_gain.gain_mid_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 3), 16, 120 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, sc500ai_sensor_vts << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 120 << 4);
		exp_gain.exp_mid_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 4), 16, sc500ai_sensor_vts << 4);
		exp_gain.gain_mid_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 3), 16, 120 << 4);
	}
	sensor_s_exp_gain(id, &exp_gain);

	sensor_write(id, 0x0100, 0x01);

	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

	sensor_dbg("%s on = %d, 2880*1620 fps: 30\n", __func__, enable);

	if (!enable)
		return 0;

	return sensor_reg_init(id, isp_id);
}

static int sensor_s_switch(int id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	struct sensor_exp_gain exp_gain;
	int ret = -1;

	full_size = 1;
	sc500ai_sensor_vts = current_switch_win[id]->vts;

	sensor_write(id, 0x0100, 0x00);
	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_err("cannot find 1440x400p130fps to 1620p%dfps reg\n", current_switch_win[id]->fps_fixed);
	if (ret < 0)
		return ret;

	if (glb_exp_gain.exp_val && glb_exp_gain.gain_val) {
		exp_gain.exp_val = glb_exp_gain.exp_val;
		exp_gain.exp_mid_val = glb_exp_gain.exp_mid_val;
		exp_gain.gain_val = glb_exp_gain.gain_val;
	} else {
		exp_gain.exp_val = 1024;
		exp_gain.exp_mid_val = 64;
		exp_gain.gain_val = 32;
	}

	sensor_s_exp_gain(id, &exp_gain); /* make switch_regs firstframe  */
	sensor_write(id, 0x0100, 0x01);
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

struct sensor_fuc_core sc500ai_core  = {
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

