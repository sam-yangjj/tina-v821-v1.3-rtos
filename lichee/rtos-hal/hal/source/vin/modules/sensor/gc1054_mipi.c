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
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_FALLING
#define V4L2_IDENT_SENSOR  0x1054

#define EXP_HIGH		0xff
#define EXP_MID			0x03
#define EXP_LOW			0x04
#define VB_HIGH			0x07
#define VB_LOW			0x08
#define GAIN_HIGH		0xff
#define GAIN_LOW		0x24

#define ID_REG_HIGH		0xf0
#define ID_REG_LOW		0xf1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)


//Set Gain
#define ANALOG_GAIN_1 64 //1.00x
#define ANALOG_GAIN_2 91 //1.42x
#define ANALOG_GAIN_3 127//1.99x
#define ANALOG_GAIN_4 182//2.85x
#define ANALOG_GAIN_5 258//4.03x
#define ANALOG_GAIN_6 369//5.77x
#define ANALOG_GAIN_7 516//8.06x
#define ANALOG_GAIN_8 738//11.53x
#define ANALOG_GAIN_9 1032//16.12x
#define ANALOG_GAIN_10 1491//23.3x
#define ANALOG_GAIN_11 2084//32.57x

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 20

/*
 * The gc1054 i2c address
 */
#define I2C_ADDR 0x42

#define SENSOR_NUM 0x2
#define SENSOR_NAME "gc1054_mipi"
#define SENSOR_NAME_2 "gc1054_mipi_2"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];
static struct sensor_format_struct *current_switch_win[2];

#if defined CONFIG_ISP_FAST_CONVERGENCE  || defined CONFIG_ISP_HARD_LIGHTADC
int full_size = 0;
#else
int full_size = 1;
#endif

#define SENSOR_30FPS 0
#define SENSOR_25FPS 0
#define SENSOR_20FPS 1
#define SENSOR_15FPS 0

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC//FULL_SIZE
static struct regval_list sensor_720p20_regs[] = {
//////////////////////   SYS   //////////////////////
	{0xf2, 0x00},
	{0xf6, 0x00},
	{0xfc, 0x04},
	{0xf7, 0x01},
	{0xf8, 0x0c},
	{0xf9, 0x06},
	{0xfa, 0x80},
	{0xfc, 0x4e},
/////    ANALOG & CISCTL   ////////////////
	{0xfe, 0x00},
	{0x03, 0x02},
	{0x04, 0xa6},
	{0x05, 0x02},//HB
	{0x06, 0x07},
	{0x07, 0x01},//VB
	{0x08, 0x81},
	{0x09, 0x00},
	{0x0a, 0x04},//row start
	{0x0b, 0x00},
	{0x0c, 0x00},//col start
	{0x0d, 0x02},
	{0x0e, 0xd4},//height 724
	// {0x0e, 0xe0},//height 724
	{0x0f, 0x05},
	{0x10, 0x08},//width 1288
	{0x17, 0xc0},
	{0x18, 0x02},
	{0x19, 0x08},
	{0x1a, 0x18},
	{0x1d, 0x12},
	{0x1e, 0x50},
	{0x1f, 0x80},
	{0x21, 0x30},
	{0x23, 0xf8},
	{0x25, 0x10},
	{0x28, 0x20},
	{0x34, 0x08}, //data low
	{0x3c, 0x10},
	{0x3d, 0x0e},
	{0xcc, 0x8e},
	{0xcd, 0x9a},
	{0xcf, 0x70},
	{0xd0, 0xa9},
	{0xd1, 0xc5},
	{0xd2, 0xed},//data high
	{0xd8, 0x3c},//dacin offset
	{0xd9, 0x7a},
	{0xda, 0x12},
	{0xdb, 0x50},
	{0xde, 0x0c},
	{0xe3, 0x60},
	{0xe4, 0x78},
	{0xfe, 0x01},
	{0xe3, 0x01},
	{0xe6, 0x10},//ramps offset
////// /////   ISP   //////////////////////
	{0xfe, 0x01},
	{0x80, 0x50},
	{0x88, 0x73},
	{0x89, 0x03},
	{0x90, 0x01},
	{0x92, 0x02},//crop win 2<=y<=4
	{0x94, 0x03},//crop win 2<=x<=5
	{0x95, 0x02},//crop win height
	{0x96, 0xd0},
	// {0x96, 0xd8},
	{0x97, 0x05},//crop win width
	{0x98, 0x00},
////// /////   BLK   //////////////////////
	{0xfe, 0x01},
	{0x40, 0x22},
	{0x43, 0x03},
	{0x4e, 0x3c},
	{0x4f, 0x00},
	{0x60, 0x00},
	{0x61, 0x80},
////// /////   GAIN   /////////////////////
	{0xfe, 0x01},
	{0xb0, 0x48},
	{0xb1, 0x01},
	{0xb2, 0x00},
	{0xb6, 0x00},
	{0xfe, 0x02},
	{0x01, 0x00},
	{0x02, 0x01},
	{0x03, 0x02},
	{0x04, 0x03},
	{0x05, 0x04},
	{0x06, 0x05},
	{0x07, 0x06},
	{0x08, 0x0e},
	{0x09, 0x16},
	{0x0a, 0x1e},
	{0x0b, 0x36},
	{0x0c, 0x3e},
	{0x0d, 0x56},
	{0xfe, 0x02},
	{0xb0, 0x00},//col_gain[11:8]
	{0xb1, 0x00},
	{0xb2, 0x00},
	{0xb3, 0x11},
	{0xb4, 0x22},
	{0xb5, 0x54},
	{0xb6, 0xb8},
	{0xb7, 0x60},
	{0xb9, 0x00},//col_gain[12]
	{0xba, 0xc0},
	{0xc0, 0x20},//col_gain[7:0]
	{0xc1, 0x2d},
	{0xc2, 0x40},
	{0xc3, 0x5b},
	{0xc4, 0x80},
	{0xc5, 0xb5},
	{0xc6, 0x00},
	{0xc7, 0x6a},
	{0xc8, 0x00},
	{0xc9, 0xd4},
	{0xca, 0x00},
	{0xcb, 0xa8},
	{0xcc, 0x00},
	{0xcd, 0x50},
	{0xce, 0x00},
	{0xcf, 0xa1},
////// ///   DARKSUN   ////////////////////
	{0xfe, 0x02},
	{0x54, 0xf7},
	{0x55, 0xf0},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x58, 0x00},
	{0x5a, 0x04},
////// //////   DD   //////////////////////
	{0xfe, 0x04},
	{0x81, 0x8a},
////// /////	 MIPI	/////////////////////
	{0xfe, 0x03},
	{0x01, 0x03},
	{0x02, 0x11},
	{0x03, 0x90},
	{0x10, 0x90},
	{0x11, 0x2b},
	{0x12, 0x40}, //lwc 1280*5/4
	{0x13, 0x06},
	{0x15, 0x00},
	{0x21, 0x02},
	{0x22, 0x02},
	{0x23, 0x08},
	{0x24, 0x02},
	{0x25, 0x10},
	{0x26, 0x04},
	{0x29, 0x03},
	{0x2a, 0x02},
	{0x2b, 0x08},
	{0xfe, 0x00},

	{0xfe, 0x01}, //RGB
	{0x92, 0x03},//crop win 2<=y<=4
	{0x94, 0x02},//crop win 2<=x<=5
	{0xfe, 0x00},
};
#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
static struct regval_list sensor_240p120_regs[] = {
//////////////////////   SYS   //////////////////////
	{0xfe, 0xf0},
	{0xfe, 0xf0},
	{0xfe, 0xf0},

	{0xf2, 0x00},
	{0xf6, 0x00},
	{0xfc, 0x04},
	{0xf7, 0x01},
	{0xf8, 0x0c},
	{0xf9, 0x06},
	{0xfa, 0x80},
	{0xfc, 0x0e},
/////    ANALOG & CISCTL   ////////////////
	{0xfe, 0x00},
	{0x03, 0x02},
	{0x04, 0xa6},
	{0x05, 0x02},//HB
	{0x06, 0x07},
	{0x07, 0x00},//VB
	{0x08, 0x14},

	{0x09, 0x00},
	{0x0a, 0xf4},//row start
	{0x0b, 0x01},
	{0x0c, 0xe0},//col start

	{0x0d, 0x00},
	{0x0e, 0xf8},//height 248
	{0x0f, 0x01},
	{0x10, 0x48},//width 328

	{0x17, 0xc0},
	{0x18, 0x02},
	{0x19, 0x08},
	{0x1a, 0x18},

	{0x1d, 0x12},
	{0x1e, 0x50},
	{0x1f, 0x80},
	{0x21, 0x30},
	{0x23, 0xf8},
	{0x25, 0x10},
	{0x28, 0x20},
	{0x34, 0x0a}, //data low
	{0x3c, 0x10},
	{0x3d, 0x0e},
	{0xcc, 0x8e},
	{0xcd, 0x9a},
	{0xcf, 0x70},
	{0xd0, 0xa9},
	{0xd1, 0xc5},
	{0xd2, 0xed},//data high
	{0xd8, 0x3c},//dacin offset
	{0xd9, 0x7a},
	{0xda, 0x12},
	{0xdb, 0x50},
	{0xde, 0x0c},
	{0xe3, 0x60},
	{0xe4, 0x78},
	{0xfe, 0x01},
	{0xe3, 0x01},
	{0xe6, 0x10},//ramps offset
////// /////   ISP   //////////////////////
	{0xfe, 0x01},
	{0x80, 0x50},
	{0x88, 0x73},
	{0x89, 0x03},
	{0x90, 0x01},
	{0x92, 0x03},//crop win 2<=y<=4
	{0x94, 0x02},//crop win 2<=x<=5

	{0x95, 0x00},//crop win height
	{0x96, 0xf0},
	{0x97, 0x01},//crop win width
	{0x98, 0x40},

////// /////   BLK   //////////////////////
	{0xfe, 0x01},
	{0x40, 0x22},
	{0x43, 0x03},
	{0x4e, 0x3c},
	{0x4f, 0x00},
	{0x60, 0x00},
	{0x61, 0x80},
////// /////   GAIN   /////////////////////
	{0xfe, 0x01},
	{0xb0, 0x48},
	{0xb1, 0x01},
	{0xb2, 0x00},
	{0xb6, 0x00},
	{0xfe, 0x02},
	{0x01, 0x00},
	{0x02, 0x01},
	{0x03, 0x02},
	{0x04, 0x03},
	{0x05, 0x04},
	{0x06, 0x05},
	{0x07, 0x06},
	{0x08, 0x0e},
	{0x09, 0x16},
	{0x0a, 0x1e},
	{0x0b, 0x36},
	{0x0c, 0x3e},
	{0x0d, 0x56},
	{0xfe, 0x02},
	{0xb0, 0x00},//col_gain[11:8]
	{0xb1, 0x00},
	{0xb2, 0x00},
	{0xb3, 0x11},
	{0xb4, 0x22},
	{0xb5, 0x54},
	{0xb6, 0xb8},
	{0xb7, 0x60},
	{0xb9, 0x00},//col_gain[12]
	{0xba, 0xc0},
	{0xc0, 0x20},//col_gain[7:0]
	{0xc1, 0x2d},
	{0xc2, 0x40},
	{0xc3, 0x5b},
	{0xc4, 0x80},
	{0xc5, 0xb5},
	{0xc6, 0x00},
	{0xc7, 0x6a},
	{0xc8, 0x00},
	{0xc9, 0xd4},
	{0xca, 0x00},
	{0xcb, 0xa8},
	{0xcc, 0x00},
	{0xcd, 0x50},
	{0xce, 0x00},
	{0xcf, 0xa1},
////// ///   DARKSUN   ////////////////////
	{0xfe, 0x02},
	{0x54, 0xf7},
	{0x55, 0xf0},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x58, 0x00},
	{0x5a, 0x04},
////// //////   DD   //////////////////////
	{0xfe, 0x04},
	{0x81, 0x8a},
////// /////	 MIPI	/////////////////////
	{0xfe, 0x03},
	{0x01, 0x03},
	{0x02, 0x77},
	{0x03, 0x90},
	{0x10, 0x90},
	{0x11, 0x2b},
	{0x12, 0x90}, //lwc 1280*5/4
	{0x13, 0x01},
	{0x15, 0x00},
	{0x21, 0x05},
	{0x22, 0x01},
	{0x23, 0x06},  //0x10
	{0x24, 0x02},
	{0x25, 0x10},
	{0x26, 0x03},
	{0x29, 0x01},
	{0x2a, 0x05},
	{0x2b, 0x03},

	{0x42, 0x40},
	{0x43, 0x01},
	{0xfe, 0x00},

	{0xfe, 0x01},
	{0x80, 0x10},
	{0xfe, 0x04},
	{0x81, 0x00},

	{0xfe, 0x00},
};

static struct regval_list sensor_240p120fps_to_720p20fps[] = {
	//////////////////////   SYS   //////////////////////
	{0xfe, 0x03},
	{0x10, 0x00},
	{0xfe, 0x00},

	{0xfe, 0x20},

	{0xf8, 0x0c},

	{0x07, 0x01},//VB
	{0x08, 0x81},

	{0x0a, 0x04},//row start
	{0x0b, 0x00},
	{0x0c, 0x00},//col start
	{0x0d, 0x02},
	{0x0e, 0xd4},//height 724
	{0x0f, 0x05},
	{0x10, 0x08},//width 1288
////// /////   ISP   //////////////////////
	{0xfe, 0x01},
	{0x95, 0x02},//crop win height
	{0x96, 0xd0},
	{0x97, 0x05},//crop win width
	{0x98, 0x00},
////// /////	 MIPI	/////////////////////
	{0xfe, 0x03},
	{0x12, 0x40}, //lwc 1280*5/4
	{0x13, 0x06},
	{0x42, 0x00}, //lwc 1280*5/4
	{0x43, 0x05},
	{0x10, 0x90},
	{0xfe, 0x00},

	{0xfe, 0x01},
	{0x80, 0xc0},
	{0xfe, 0x04},
	{0x42, 0x09},
	{0x43, 0xc8},
	{0xfe, 0x00},

	////// /////   ISP   //////////////////////
	{0xfe, 0x01},
	{0x80, 0x50},
	{0x88, 0x73},
	{0x89, 0x03},
	// {0x90, 0x01},
	// {0x92, 0x03},//crop win 2<=y<=4
	// {0x94, 0x02},//crop win 2<=x<=5
	// {0x95, 0x02},//crop win height
	// {0x96, 0xd0},
	// // {0x96, 0xd8},
	// {0x97, 0x05},//crop win width
	// {0x98, 0x00},
};
#endif
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
#if 0
static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}
static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}
#endif

static int sensor_s_exp(int id, unsigned int exp_val)
{
	int tmp_exp_val = exp_val / 16;

	sensor_dbg("exp_val:%d\n", exp_val);
	sensor_write(id, 0x03, (tmp_exp_val >> 8) & 0xFF);
	sensor_write(id, 0x04, (tmp_exp_val & 0xFF));

	return 0;
}

static int sensor_s_gain(int id,int gain_val)
{
	unsigned char tmp;
	gain_val = gain_val * 4;

	sensor_write(id, 0xfe, 0x01);
	//sensor_write(id, 0xb1, 0x01);
	//sensor_write(id, 0xb2, 0x00);

	if (gain_val < 0x40) {
		gain_val = 0x40;
	}

	if ((ANALOG_GAIN_1 <= gain_val) && (gain_val < ANALOG_GAIN_2)) {

	sensor_write(id,0xb6, 0x00);
	tmp = gain_val;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_2 <= gain_val) && (gain_val < ANALOG_GAIN_3)) {

	sensor_write(id,0xb6, 0x01);
	tmp = 64 * gain_val / ANALOG_GAIN_2;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_3 <= gain_val) && (gain_val < ANALOG_GAIN_4)) {

	sensor_write(id,0xb6, 0x02);
	tmp = 64 * gain_val / ANALOG_GAIN_3;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_4 <= gain_val) && (gain_val < ANALOG_GAIN_5)) {

	sensor_write(id,0xb6, 0x03);
	tmp = 64 * gain_val / ANALOG_GAIN_4;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_5 <= gain_val) && (gain_val < ANALOG_GAIN_6)) {

	sensor_write(id,0xb6, 0x04);
	tmp = 64 * gain_val / ANALOG_GAIN_5;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_6 <= gain_val) && (gain_val < ANALOG_GAIN_7)){

	sensor_write(id,0xb6, 0x05);
	tmp = 64 * gain_val / ANALOG_GAIN_6;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp <<2) & 0xfc);
	} else if ((ANALOG_GAIN_7 <= gain_val) && (gain_val < ANALOG_GAIN_8)){

	sensor_write(id,0xb6, 0x06);
	tmp = 64 * gain_val / ANALOG_GAIN_7;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp <<2) & 0xfc);
	} else if ((ANALOG_GAIN_8 <= gain_val) && (gain_val < ANALOG_GAIN_9)){

	sensor_write(id,0xb6, 0x07);
	tmp = 64 * gain_val / ANALOG_GAIN_8;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp <<2) & 0xfc);
	} else if ((ANALOG_GAIN_9 <= gain_val) && (gain_val < ANALOG_GAIN_10)){

	sensor_write(id,0xb6, 0x08);
	tmp = 64 * gain_val / ANALOG_GAIN_9;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp <<2) & 0xfc);
	} else if ((ANALOG_GAIN_10 <= gain_val) && (gain_val < ANALOG_GAIN_11)){

	sensor_write(id,0xb6, 0x09);
	tmp = 64 * gain_val / ANALOG_GAIN_10;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp <<2) & 0xfc);
	} else if (ANALOG_GAIN_11 <= gain_val){

	sensor_write(id,0xb6, 0x0a);
	tmp = 64 * gain_val / ANALOG_GAIN_11;
	sensor_write(id,0xb1, tmp >> 6);
	sensor_write(id,0xb2, (tmp <<2) & 0xfc);
	}
	sensor_write(id, 0xfe, 0x00);
	sensor_dbg("gc1054 sensor_set_gain = %d, %d (1,2,4,8,15->0,1,2,3,4) Done!\n", gain_val, gain_val/16/6);

	return 0;
}

static int gc1054_sensor_vts;
static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	unsigned int shutter = 0, vertical_blanking = 0;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;

	shutter = exp_val >> 4;
	if(shutter < gc1054_sensor_vts - 8)
		vertical_blanking = gc1054_sensor_vts - 724 - 20;
	else
		vertical_blanking = shutter + 8 - 724 - 20;

	/* select page0 */
	sensor_write(id, 0xfe, 0x00);
	/* update VB for more exp_time */
	if (full_size) {
		sensor_write(id, VB_HIGH, (vertical_blanking & 0x1f00) >> 8);
		sensor_write(id, VB_LOW, (vertical_blanking & 0xff));
	}

	sensor_s_exp(id, exp_val);
	sensor_s_gain(id, gain_val);

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
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(1000);
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(1000);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!do nothing\n");
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		vin_set_mclk(id, 0);
		hal_usleep(1000);
		break;
	default:
		return -1;
	}

	return 0;
}

static int sensor_set_ir(int id, int status)
{
	// hal_usleep(1000*1000);
	// rt_kprintf("sensor_set_ir %d\n",status);
	// return 0;
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
#if 0
static int sensor_reset(int id, u32 val)
{

	sensor_dbg("%s: val=%d\n", __func__);
	switch (val) {
	case 0:
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(1000);
		break;
	case 1:
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(1000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif

static int sensor_detect(int id)
{
	data_type rdval;
	int eRet;
	int times_out = 3;
	do {
		eRet = sensor_read(id, ID_REG_HIGH, &rdval);
		sensor_dbg("eRet:%d, ID_VAL_HIGH:0x%x, times_out:%d\n", eRet, rdval, times_out);
		hal_usleep(200);
		times_out--;
	} while (eRet < 0  &&  times_out > 0);

	sensor_read(id, ID_REG_HIGH, &rdval);
	sensor_dbg("ID_VAL_HIGH = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_HIGH)
		return -ENODEV;

	sensor_read(id, ID_REG_LOW, &rdval);
	sensor_dbg("ID_VAL_LOW = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_LOW)
		return -ENODEV;

	sensor_dbg("Done!\n");
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
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC // FULL_SIZE
		{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3200,
		.vts        = 1125,
		.pclk       = 72*1000*1000,
		.mipi_bps   = 312 * 1000 * 1000,
		.fps_fixed  = 20,
		.bin_factor = 1,

		.intg_min   = 1 << 4,
		.intg_max   = (1125) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 32 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_720p20_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p20_regs),
	},
#else
{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 320,
		.height     = 240,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1250,
		.vts        = 288,
		.pclk       = 42*1000*1000,
		.mipi_bps   = 312 * 1000 * 1000,
		.fps_fixed  = 120,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (288 - 8) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 32 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_240p120_regs,
		.regs_size  = ARRAY_SIZE(sensor_240p120_regs),
	}
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
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3200,
		.vts        = 1125,
		.pclk       = 72*1000*1000,
		.mipi_bps   = 312 * 1000 * 1000,
		.fps_fixed  = 20,
		.bin_factor = 1,

		.intg_min   = 1 << 4,
		.intg_max   = (1125) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 32 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.switch_regs	   = sensor_240p120fps_to_720p20fps,
		.switch_regs_size  = ARRAY_SIZE(sensor_240p120fps_to_720p20fps),
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
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{
	int ret = 0;
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);
	struct sensor_exp_gain exp_gain;

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

	gc1054_sensor_vts = current_win[id]->vts;
//#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_FAST_CONVERGENCE
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, 1125 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 110 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, 1125 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 110 << 4);
	}
	gc1054_sensor_vts = current_win[id]->vts;

	sensor_s_exp_gain(id, &exp_gain);
	sensor_write(id, 0x3e, 0x91);
//#else  //CONFIG_ISP_HARD_LIGHTADC

//#endif
	//sensor_flip_status = 0x0;
	//sensor_dbg("gc2053_sensor_vts = %d\n", gc2053_sensor_vts);

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

static int sensor_s_switch(int id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	struct sensor_exp_gain exp_gain;
	int ret = -1;

	full_size = 1;
	gc1054_sensor_vts = current_switch_win[id]->vts;
	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_err("cannot find 240p120fps to 720p%dfps reg\n", current_switch_win[id]->fps_fixed);
	if (ret < 0)
		return ret;
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

struct sensor_fuc_core gc1054_core  = {
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
