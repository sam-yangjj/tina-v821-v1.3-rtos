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

#define MCLK            (27*1000*1000)
#define V4L2_IDENT_SENSOR   0xcc41

#define SENSOR_FRAME_RATE   20

/*
 * The SC3336 i2c address
 */
#define I2C_ADDR 0x60

#define SENSOR_NUM 0x2
#define SENSOR_NAME "sc3336_mipi"
#define SENSOR_NAME_2 "sc3336_mipi_2"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_exp_gain glb_exp_gain;
static struct sensor_format_struct *current_win[2];
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
static struct sensor_format_struct *current_switch_win[2];
#endif

#define SENSOR_20FPS 1
/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#if SENSOR_20FPS
static struct regval_list sensor_2304_1296p20_regs[] = {
//window_size=2304*1296 mipi@2lane
//row_time=35.55us,frame_rate=20fps
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x301f, 0x01},
	{0x30b8, 0x33},
	//20FPS
	{0x320c, 0x04},//1250	2*channel-->2500
	{0x320d, 0xe2},
	{0x320e, 0x07},//vts=2025
	{0x320f, 0xe9},

	{0x3253, 0x10},
	{0x325f, 0x20},
	{0x3301, 0x04},
	{0x3306, 0x50},
	{0x330a, 0x00},
	{0x330b, 0xd8},
	{0x3314, 0x13},
	{0x3333, 0x10},
	{0x3334, 0x40},
	{0x335e, 0x06},
	{0x335f, 0x0a},
	{0x3364, 0x5e},
	{0x337c, 0x02},
	{0x337d, 0x0e},
	{0x3390, 0x01},
	{0x3391, 0x03},
	{0x3392, 0x07},
	{0x3393, 0x04},
	{0x3394, 0x04},
	{0x3395, 0x04},
	{0x3396, 0x08},
	{0x3397, 0x0b},
	{0x3398, 0x1f},
	{0x3399, 0x04},
	{0x339a, 0x0a},
	{0x339b, 0x3a},
	{0x339c, 0xc4},
	{0x33a2, 0x04},
	{0x33ac, 0x08},
	{0x33ad, 0x1c},
	{0x33ae, 0x10},
	{0x33af, 0x30},
	{0x33b1, 0x80},
	{0x33b3, 0x48},
	{0x33f9, 0x60},
	{0x33fb, 0x74},
	{0x33fc, 0x4b},
	{0x33fd, 0x5f},
	{0x349f, 0x03},
	{0x34a6, 0x4b},
	{0x34a7, 0x5f},
	{0x34a8, 0x20},
	{0x34a9, 0x18},
	{0x34ab, 0xe8},
	{0x34ac, 0x01},
	{0x34ad, 0x00},
	{0x34f8, 0x5f},
	{0x34f9, 0x18},
	{0x3630, 0xc0},
	{0x3631, 0x84},
	{0x3632, 0x64},
	{0x3633, 0x32},
	{0x363b, 0x03},
	{0x363c, 0x08},
	{0x3641, 0x38},
	{0x3670, 0x4e},
	{0x3674, 0xc0},
	{0x3675, 0xc0},
	{0x3676, 0xc0},
	{0x3677, 0x84},
	{0x3678, 0x8a},
	{0x3679, 0x8c},
	{0x367c, 0x48},
	{0x367d, 0x49},
	{0x367e, 0x4b},
	{0x367f, 0x5f},
	{0x3690, 0x33},
	{0x3691, 0x33},
	{0x3692, 0x44},
	{0x369c, 0x4b},
	{0x369d, 0x5f},
	{0x36b0, 0x87},
	{0x36b1, 0x90},
	{0x36b2, 0xa1},
	{0x36b3, 0xd8},
	{0x36b4, 0x49},
	{0x36b5, 0x4b},
	{0x36b6, 0x4f},
	{0x370f, 0x01},
	{0x3722, 0x09},
	{0x3724, 0x41},
	{0x3725, 0xc1},
	{0x3771, 0x09},
	{0x3772, 0x09},
	{0x3773, 0x05},
	{0x377a, 0x48},
	{0x377b, 0x5f},
	{0x3904, 0x04},
	{0x3905, 0x8c},
	{0x391d, 0x04},
	{0x3921, 0x20},
	{0x3926, 0x21},
	{0x3933, 0x80},
	{0x3934, 0x0a},
	{0x3935, 0x00},
	{0x3936, 0x2a},
	{0x3937, 0x6a},
	{0x3938, 0x6f},
	{0x39dc, 0x02},
	{0x3e01, 0x53},
	{0x3e02, 0xe0},
	{0x3e09, 0x00},
	{0x440e, 0x02},
	{0x4509, 0x20},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x38},
	{0x5ae3, 0x30},
	{0x5ae4, 0x28},
	{0x5ae5, 0x38},
	{0x5ae6, 0x30},
	{0x5ae7, 0x28},
	{0x5ae8, 0x3f},
	{0x5ae9, 0x34},
	{0x5aea, 0x2c},
	{0x5aeb, 0x3f},
	{0x5aec, 0x34},
	{0x5aed, 0x2c},
	{0x36e9, 0x54},
	{0x37f9, 0x27},
	//{0x0100, 0x01},
};
#endif

#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
static struct regval_list sensor_1152_320p120_regs[] = {
	//h sum + v binning
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x301f, 0x2d},
	{0x30b8, 0x33},
	{0x3200, 0x00},
	{0x3201, 0x00},
	{0x3202, 0x01},
	{0x3203, 0x48},
	{0x3204, 0x09},
	{0x3205, 0x07},
	{0x3206, 0x03},
	{0x3207, 0xcf},
	{0x3208, 0x04},
	{0x3209, 0x80},
	{0x320a, 0x01},
	{0x320b, 0x40},
	{0x320c, 0x04},//hts 1264*2=2528
	{0x320d, 0xf0},
	{0x320e, 0x01},//vts 356
	{0x320f, 0x64},
	{0x3210, 0x00},
	{0x3211, 0x02},
	{0x3212, 0x00},
	{0x3213, 0x02},
	{0x3215, 0x31},
	{0x3220, 0x01},
	{0x3250, 0x40},
	{0x3253, 0x10},
	{0x325f, 0x20},
	{0x3301, 0x04},
	{0x3306, 0x50},
	{0x3309, 0xa8},
	{0x330a, 0x00},
	{0x330b, 0xd8},
	{0x3314, 0x13},
	{0x331f, 0x99},
	{0x3333, 0x10},
	{0x3334, 0x40},
	{0x335e, 0x06},
	{0x335f, 0x0a},
	{0x3364, 0x5e},
	{0x337c, 0x02},
	{0x337d, 0x0e},
	{0x3390, 0x01},
	{0x3391, 0x03},
	{0x3392, 0x07},
	{0x3393, 0x04},
	{0x3394, 0x04},
	{0x3395, 0x04},
	{0x3396, 0x08},
	{0x3397, 0x0b},
	{0x3398, 0x1f},
	{0x3399, 0x04},
	{0x339a, 0x0a},
	{0x339b, 0x3a},
	{0x339c, 0xa0},
	{0x33a2, 0x04},
	{0x33ac, 0x08},
	{0x33ad, 0x1c},
	{0x33ae, 0x10},
	{0x33af, 0x30},
	{0x33b1, 0x80},
	{0x33b3, 0x48},
	{0x33f9, 0x60},
	{0x33fb, 0x74},
	{0x33fc, 0x4b},
	{0x33fd, 0x5f},
	{0x349f, 0x03},
	{0x34a6, 0x4b},
	{0x34a7, 0x5f},
	{0x34a8, 0x20},
	{0x34a9, 0x18},
	{0x34ab, 0xe8},
	{0x34ac, 0x01},
	{0x34ad, 0x00},
	{0x34f8, 0x5f},
	{0x34f9, 0x18},
	{0x3630, 0xc0},
	{0x3631, 0x84},
	{0x3632, 0x64},
	{0x3633, 0x32},
	{0x363b, 0x03},
	{0x363c, 0x08},
	{0x3641, 0x38},
	{0x3670, 0x4e},
	{0x3674, 0xc0},
	{0x3675, 0xc0},
	{0x3676, 0xc0},
	{0x3677, 0x86},
	{0x3678, 0x86},
	{0x3679, 0x86},
	{0x367c, 0x48},
	{0x367d, 0x49},
	{0x367e, 0x4b},
	{0x367f, 0x5f},
	{0x3690, 0x32},
	{0x3691, 0x32},
	{0x3692, 0x42},
	{0x369c, 0x4b},
	{0x369d, 0x5f},
	{0x36b0, 0x87},
	{0x36b1, 0x90},
	{0x36b2, 0xa1},
	{0x36b3, 0xd8},
	{0x36b4, 0x49},
	{0x36b5, 0x4b},
	{0x36b6, 0x4f},
	{0x36ea, 0x08},
	{0x36eb, 0x0d},
	{0x36ec, 0x2c},
	{0x36ed, 0x26},
	{0x370f, 0x01},
	{0x3722, 0x09},
	{0x3724, 0x41},
	{0x3725, 0xc1},
	{0x3771, 0x09},
	{0x3772, 0x09},
	{0x3773, 0x05},
	{0x377a, 0x48},
	{0x377b, 0x5f},
	{0x37fa, 0x08},
	{0x37fb, 0x33},
	{0x37fc, 0x11},
	{0x37fd, 0x18},
	{0x3904, 0x04},
	{0x3905, 0x8c},
	{0x391d, 0x04},
	{0x3921, 0x20},
	{0x3926, 0x21},
	{0x3933, 0x80},
	{0x3934, 0x0a},
	{0x3935, 0x00},
	{0x3936, 0x2a},
	{0x3937, 0x6a},
	{0x3938, 0x6a},
	{0x39dc, 0x02},
	{0x3e01, 0x15},
	{0x3e02, 0xc0},
	{0x3e09, 0x00},
	{0x440e, 0x02},
	{0x4509, 0x20},
	{0x4800, 0x44},
	{0x4816, 0x51},
	{0x4819, 0x04},
	{0x481b, 0x03},
	{0x481d, 0x08},
	{0x481f, 0x02},
	{0x4821, 0x07},
	{0x4823, 0x02},
	{0x4825, 0x02},
	{0x4827, 0x02},
	{0x4829, 0x03},
	{0x5000, 0x46},
	{0x5900, 0xf1},
	{0x5901, 0x04},
	{0x5988, 0x70},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x38},
	{0x5ae3, 0x30},
	{0x5ae4, 0x28},
	{0x5ae5, 0x38},
	{0x5ae6, 0x30},
	{0x5ae7, 0x28},
	{0x5ae8, 0x3f},
	{0x5ae9, 0x34},
	{0x5aea, 0x2c},
	{0x5aeb, 0x3f},
	{0x5aec, 0x34},
	{0x5aed, 0x2c},
	{0x36e9, 0x24},
	{0x37f9, 0x24},
	//{0x0100, 0x01},
};
#endif

static int sensor_s_exp(int id, unsigned int exp_val)
{
	unsigned int explow, expmid, exphigh;

	exphigh = (unsigned char) (0x0f & (exp_val>>16));
	expmid = (unsigned char) (0xff & (exp_val>>8));
	explow = (unsigned char) (0xf0 & (exp_val));

	sensor_write(id, 0x3e02, explow);
	sensor_write(id, 0x3e01, expmid);
	sensor_write(id, 0x3e00, exphigh);

	return 0;
}

static int sensor_s_gain(int id, int gain_val)
{
	int anagain = 0x00;
	int gaindiglow = 0x80;
	int gaindighigh = 0x00;
	int gain_factor;

	gain_factor = (gain_val * 1000) >> 4;

	if (gain_factor < 1000)
		gain_factor = 1000;
	if (gain_factor > 766*1000)
		gain_factor = 766*1000;

	if (gain_factor < 1520) {
		anagain = 0x00;
		gaindighigh = 0x00;
		gaindiglow  = (gain_factor << 7) / 1000;
	} else if (gain_factor < 3040) {
		anagain  = 0x40;
		gaindighigh = 0x00;
		gaindiglow = (gain_factor << 7) / 1520;
	} else if (gain_factor < 6080) {
		anagain = 0x48;
		gaindighigh = 0x00;
		gaindiglow = (gain_factor << 7) / 3040;
	} else if (gain_factor < 12160) {
		anagain = 0x49;
		gaindighigh = 0x00;
		gaindiglow = (gain_factor << 7) / 6080;
	} else if (gain_factor < 24320) {
		anagain = 0x4b;
		gaindighigh = 0x00;
		gaindiglow = (gain_factor << 7) / 12160;
	} else if (gain_factor < 48640) {
		anagain = 0x4f;
		gaindighigh = 0x00;
		gaindiglow = (gain_factor << 7) / 24320;
	} else if (gain_factor < 48640*2) { //open dgain begin  max digital gain 4X
		anagain = 0x5f;
		gaindighigh = 0x00;
		gaindiglow = (gain_factor << 7) / 48640;
	} else if (gain_factor < 48640*4) {
		anagain = 0x5f;
		gaindighigh = 0x01;
		gaindiglow = (gain_factor << 7) / 48640 / 2;
	} else if (gain_factor < 48640*8) {
		 anagain = 0x5f;
		 gaindighigh = 0x03;
		 gaindiglow = (gain_factor << 7) / 48640 / 4;
	} else if (gain_factor < 48640*16) {
		 anagain = 0x5f;
		 gaindighigh = 0x07;
		 gaindiglow = (gain_factor << 7) / 48640 / 8;
	}

	sensor_write(id, 0x3e09, (unsigned char)anagain);
	sensor_write(id, 0x3e07, (unsigned char)gaindiglow);
	sensor_write(id, 0x3e06, (unsigned char)gaindighigh);

	return 0;

}

static int sc3336_sensor_vts;
static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val,gain_val;
	int shutter = 0, frame_length = 0;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;
	if (gain_val < 1 * 16)
		gain_val = 16;
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	shutter = exp_val >> 4;
	if (shutter > sc3336_sensor_vts - 12) {
		frame_length = shutter + 12;
	} else
		frame_length = sc3336_sensor_vts;
	sensor_write(id, 0x320f, (frame_length & 0xff));
	sensor_write(id, 0x320e, (frame_length >> 8));

	//sensor_write(id, 0x3812, 0x00);//group_hold
	sensor_s_exp(id, exp_val);
	sensor_s_gain(id, gain_val);
	//sensor_write(id, 0x3812, 0x30);

	glb_exp_gain.exp_val = exp_val;
	glb_exp_gain.gain_val = gain_val;

	sensor_print("gain_val:%d, exp_val:%d\n", gain_val, exp_val);
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
	unsigned int SENSOR_ID = 0;
	data_type rdval;
	int cnt = 0;

	sensor_read(id, 0x3107, &rdval);
	SENSOR_ID |= (rdval << 8);
	sensor_read(id, 0x3108, &rdval);
	SENSOR_ID |= (rdval);
	sensor_print("V4L2_IDENT_SENSOR = 0x%x\n", SENSOR_ID);

	while ((SENSOR_ID != V4L2_IDENT_SENSOR) && (cnt < 5)) {
		sensor_read(id, 0x3107, &rdval);
		SENSOR_ID |= (rdval << 8);
		sensor_read(id, 0x3108, &rdval);
		SENSOR_ID |= (rdval);
		sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n",
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
#if SENSOR_20FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 2304,
		.height     = 1296,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2500,
		.vts        = 2025,
		.pclk       = 101250000,
		.mipi_bps   = 506250000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (2025 - 10) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 766 << 4,
		.regs       = sensor_2304_1296p20_regs,
		.regs_size  = ARRAY_SIZE(sensor_2304_1296p20_regs),
	},
#endif

#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1152,
		.height     = 320,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2528,
		.vts        = 356,
		.pclk       = 107996160,
		.mipi_bps   = 270 * 1000 * 1000,
		.fps_fixed  = 120,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (356 - 10) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 766 << 4,
		.regs       = sensor_1152_320p120_regs,
		.regs_size  = ARRAY_SIZE(sensor_1152_320p120_regs),
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

#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
static struct sensor_format_struct switch_sensor_formats[] = {
#if SENSOR_20FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 2304,
		.height     = 1296,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2500,
		.vts        = 2025,
		.pclk       = 101250000,
		.mipi_bps   = 506250000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (2025 - 10) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 766 << 4,
		.switch_regs       = sensor_2304_1296p20_regs,
		.switch_regs_size  = ARRAY_SIZE(sensor_2304_1296p20_regs),
	},
#endif
};
#endif

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
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	res->res_time_hs = 0x23;

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

	sc3336_sensor_vts = current_win[id]->vts;
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, sc3336_sensor_vts << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 766 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, sc3336_sensor_vts << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 766 << 4);
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

	sensor_dbg("%s on = %d, 2304*1296 fps: 20\n", __func__, enable);

	if (!enable)
		return 0;

	return sensor_reg_init(id, isp_id);
}

static int sensor_s_switch(int id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	struct sensor_exp_gain exp_gain;
	int ret = -1;

	sc3336_sensor_vts = current_switch_win[id]->vts;
	sensor_write(id, 0x0100, 0x00);
	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_err("cannot find 1152x320p120fps to 1296p%dfps reg\n", current_switch_win[id]->fps_fixed);
	if (ret < 0)
		return ret;

	if (glb_exp_gain.exp_val && glb_exp_gain.gain_val) {
		exp_gain.exp_val = glb_exp_gain.exp_val;
		exp_gain.gain_val = glb_exp_gain.gain_val;
	} else {
		exp_gain.exp_val = 1000;
		exp_gain.gain_val = 16;
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

struct sensor_fuc_core sc3336_core  = {
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

