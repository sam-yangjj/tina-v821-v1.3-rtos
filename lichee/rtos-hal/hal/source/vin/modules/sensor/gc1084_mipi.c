/*
 * A V4L2 driver for Raw cameras.
 *
 * Copyright (c) 2023 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Liang WeiJie <liangweijie@allwinnertech.com>
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
#define V4L2_IDENT_SENSOR  0x1084

#define EXP_MID			0x0d03
#define EXP_LOW			0x0d04

#define ID_REG_HIGH		0x03f0
#define ID_REG_LOW		0x03f1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 20

#ifdef CONFIG_SENSOR_GC1084_8BIT_MIPI
#define RAW8 1 //raw8 select
#else
#define RAW8 0 //raw10 select
#endif

/*
 * The gc1084 i2c address
 */
#define I2C_ADDR 0x6e
#ifdef CONFIG_SENSOR_GC1084_MIPI_USE_SAME_TWI
#define TARGET_I2C_ADDR 0x7e
#endif

#define SENSOR_NUM 0x2
#define SENSOR_NAME "gc1084_mipi"
#define SENSOR_NAME_2 "gc1084_mipi_2"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_exp_gain glb_exp_gain;
static struct sensor_format_struct *current_win[2];
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
static struct sensor_format_struct *current_switch_win[2];
#endif

#if defined CONFIG_ISP_FAST_CONVERGENCE  || defined CONFIG_ISP_HARD_LIGHTADC
static int full_size = 0;
#else // CONFIG_ISP_ONLY_HARD_LIGHTADC & CONFIG_ISP_READ_THRESHOLD
static int full_size = 1;
#endif

#define SENSOR_30FPS 1
#define SENSOR_20FPS 1
#define SENSOR_15FPS 1
#define SENSOR_10FPS 1

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC//FULL_SIZE

#if SENSOR_30FPS
static struct regval_list sensor_720p30_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x10},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x08},
	{0x0d06, 0x98},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x02},
	{0x0d42, 0xee},
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},

	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xd0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},
	{0x0201, 0x23},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0208, 0x39},
#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif
	//20230308
#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	//{0x023e, 0x9c},
	{0x031e, 0x3e},
#else
	{0x0215, 0x10},
	{0x0229, 0x05},
	//{0x023e, 0x98},
	{0x031e, 0x3e},
#endif
	{0x03fe, 0x10},
	{0x0187, 0x51},
	{0x03fe, 0x00},
	{0x0040, 0x02},
};
#endif

#if SENSOR_20FPS
static struct regval_list sensor_720p20_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x08},
	{0x0d06, 0x98},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x04},
	{0x0d42, 0x65},
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},

	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xd0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},
	{0x0201, 0x23},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0208, 0x39},
#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif
	//20230308
#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	//{0x023e, 0x9c},
	{0x031e, 0x3e},
#else
	{0x0215, 0x10},
	{0x0229, 0x05},
	//{0x023e, 0x98},
	{0x031e, 0x3e},
#endif

	{0x0040, 0x02},
};
#endif

#if SENSOR_15FPS
static struct regval_list sensor_720p15_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x10},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x08},
	{0x0d06, 0x98},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x05},
	{0x0d42, 0xdc}, //vts 1500
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},

	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xd0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},
	{0x0201, 0x23},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0208, 0x39},
#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif
	//20230308
#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	//{0x023e, 0x9c},
	{0x031e, 0x3e},
#else
	{0x0215, 0x10},
	{0x0229, 0x05},
	//{0x023e, 0x98},
	{0x031e, 0x3e},
#endif
	{0x03fe, 0x10},
	{0x0187, 0x51},
	{0x03fe, 0x00},
	{0x0040, 0x02},
};
#endif

#if SENSOR_10FPS
static struct regval_list sensor_720p10_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x08},
	{0x0d06, 0x98},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x08},
	{0x0d42, 0xca}, //vts 2250
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},

	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xd0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},
	{0x0201, 0x23},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0208, 0x39},
#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif
	//20230308
#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	//{0x023e, 0x9c},
	{0x031e, 0x3e},
#else
	{0x0215, 0x10},
	{0x0229, 0x05},
	//{0x023e, 0x98},
	{0x031e, 0x3e},
#endif

	{0x0040, 0x02},
};
#endif

#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC

static struct regval_list sensor_360p120_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x48},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x07},
	{0x0d06, 0x08},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd0},
	{0x000f, 0x05},
	{0x0010, 0x00},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x02},
	{0x0d42, 0xee},
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},
	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},
	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xe0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	/* out */
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x01},
	{0x0196, 0x68},
	{0x0197, 0x02},
	{0x0198, 0x80},
	/* mipi */
	{0x0201, 0x23},
	{0x0202, 0x5f},
	{0x0203, 0xce},
	{0x0208, 0x39},
	{0x0212, 0x03},
	{0x0213, 0x20},
	{0x0215, 0x10},
	{0x0229, 0x05},
	//{0x023e, 0x98},
	{0x031e, 0x3e},
	/* binning */
	// {0x0040, 0x0a},
	// {0x0015, 0x04},
	// {0x0d15, 0x04},
	// {0x03fc, 0x8e},
};

#if SENSOR_30FPS
static struct regval_list sensor_360p120fps_to_720p30fps[] = {
//	{0x023e, 0x00},

	{0x03f8, 0x2c},

	{0x0d05, 0x08},
	{0x0d06, 0x98},

	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},

	{0x0d04, 0xd0},

	//vts
	{0x0d41, 0x02},
	{0x0d42, 0xee},

	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},

#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif

	{0x0040, 0x02},
	{0x0015, 0x00},
	{0x0d15, 0x00},
	{0x03fc, 0xae},

#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	{0x031e, 0x3e},
#else
	{0x0229, 0x05},
#endif

//	{0x03fe, 0x10},
//	{0x03fe, 0x00},

//#if RAW8
//	{0x023e, 0x9c},
//#else
//	{0x023e, 0x98},
//#endif
};
#endif

#if SENSOR_20FPS
static struct regval_list sensor_360p120fps_to_720p20fps[] = {
//	{0x023e, 0x00},

	{0x03f8, 0x2c},

	{0x0d05, 0x08},
	{0x0d06, 0x98},

	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},

	{0x0d04, 0xd0},

	//vts
	{0x0d41, 0x04},
	{0x0d42, 0x65},

	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},

#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif

	{0x0040, 0x02},
	{0x0015, 0x00},
	{0x0d15, 0x00},
	{0x03fc, 0xae},

#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	{0x031e, 0x3e},
#else
	{0x0229, 0x05},
#endif

//	{0x03fe, 0x10},
//	{0x03fe, 0x00},

//#if RAW8
//	{0x023e, 0x9c},
//#else
//	{0x023e, 0x98},
//#endif
};
#endif

#if SENSOR_15FPS
static struct regval_list sensor_360p120fps_to_720p15fps[] = {
//	{0x023e, 0x00},

	{0x03f8, 0x2c},

	{0x0d05, 0x08},
	{0x0d06, 0x98},

	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},

	{0x0d04, 0xd0},

	//vts
	{0x0d41, 0x05},
	{0x0d42, 0xdc},

	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},

#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif

	{0x0040, 0x02},
	{0x0015, 0x00},
	{0x0d15, 0x00},
	{0x03fc, 0xae},

#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	{0x031e, 0x3e},
#else
	{0x0229, 0x05},
#endif

//	{0x03fe, 0x10},
//	{0x03fe, 0x00},

//#if RAW8
//	{0x023e, 0x9c},
//#else
//	{0x023e, 0x98},
//#endif
};
#endif

#if SENSOR_10FPS
static struct regval_list sensor_360p120fps_to_720p10fps[] = {
//	{0x023e, 0x00},

	{0x03f8, 0x2c},

	{0x0d05, 0x08},
	{0x0d06, 0x98},

	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},

	{0x0d04, 0xd0},

	//vts
	{0x0d41, 0x08},
	{0x0d42, 0xca},

	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},

#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif

	{0x0040, 0x02},
	{0x0015, 0x00},
	{0x0d15, 0x00},
	{0x03fc, 0xae},

#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	{0x031e, 0x3e},
#else
	{0x0229, 0x05},
#endif

//	{0x03fe, 0x10},
//	{0x03fe, 0x00},

//#if RAW8
//	{0x023e, 0x9c},
//#else
//	{0x023e, 0x98},
//#endif
};
#endif

#endif

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
	sensor_write(id, EXP_MID, (tmp_exp_val >> 8) & 0xFF);
	sensor_write(id, EXP_LOW, (tmp_exp_val & 0xFF));

	return 0;
}

static unsigned char regValTable[25][6] = {
	{0x00, 0x00, 0x00, 0x80, 0x01, 0x00},
	{0x0a, 0x00, 0x00, 0x80, 0x01, 0x0b},
	{0x00, 0x01, 0x00, 0x80, 0x01, 0x19},
	{0x0a, 0x01, 0x00, 0x80, 0x01, 0x2a},
	{0x00, 0x02, 0x00, 0x80, 0x02, 0x00},
	{0x0a, 0x02, 0x00, 0x80, 0x02, 0x17},
	{0x00, 0x03, 0x00, 0x80, 0x02, 0x33},
	{0x0a, 0x03, 0x00, 0x80, 0x03, 0x14},
	{0x00, 0x04, 0x00, 0x90, 0x04, 0x00},
	{0x0a, 0x04, 0x00, 0x90, 0x04, 0x2f},
	{0x00, 0x05, 0x00, 0x90, 0x05, 0x26},
	{0x0a, 0x05, 0x00, 0x90, 0x06, 0x28},
	{0x00, 0x06, 0x00, 0xa0, 0x08, 0x00},
	{0x0a, 0x06, 0x00, 0xa0, 0x09, 0x1e},
	{0x12, 0x46, 0x00, 0xa0, 0x0b, 0x0c},
	{0x19, 0x66, 0x00, 0xa0, 0x0d, 0x10},
	{0x00, 0x04, 0x01, 0xa0, 0x10, 0x00},
	{0x0a, 0x04, 0x01, 0xa0, 0x12, 0x3d},
	{0x00, 0x05, 0x01, 0xb0, 0x16, 0x19},
	{0x0a, 0x05, 0x01, 0xc0, 0x1a, 0x23},
	{0x00, 0x06, 0x01, 0xc0, 0x20, 0x00},
	{0x0a, 0x06, 0x01, 0xc0, 0x25, 0x3b},
	{0x12, 0x46, 0x01, 0xc0, 0x2c, 0x30},
	{0x19, 0x66, 0x01, 0xd0, 0x35, 0x01},
	{0x20, 0x06, 0x01, 0xe0, 0x3f, 0x3f},
};

static unsigned int gainLevelTable[25] = {
	64,  76,  90, 106, 128, 152, 179, 212, 256, 303,
	358, 425, 512, 607, 716, 848, 1024, 1214, 1434, 1699,
	2048, 2427,2865, 3393, 4096
};

static int total = sizeof(gainLevelTable) / sizeof(unsigned int);
static int setSensorGain(int id, int gain)
{
	int i;
	unsigned int tol_dig_gain = 0;

	for (i = 0; (total > 1) && (i < (total - 1)); i++) {
		if ((gainLevelTable[i] <= gain) && (gain < gainLevelTable[i+1]))
			break;
	}

	tol_dig_gain = gain * 64 / gainLevelTable[i];
	sensor_write(id, 0x00d1, regValTable[i][0]);
	sensor_write(id, 0x00d0, regValTable[i][1]);
	sensor_write(id, 0x031d, 0x2e);
	sensor_write(id, 0x0dc1, regValTable[i][2]);
	sensor_write(id, 0x031d, 0x28);
	sensor_write(id, 0x0155, regValTable[i][3]);
	sensor_write(id, 0x00b8, regValTable[i][4]);
	sensor_write(id, 0x00b9, regValTable[i][5]);
	sensor_write(id, 0x00b1, (tol_dig_gain>>6));
	sensor_write(id, 0x00b2, ((tol_dig_gain&0x3f)<<2));

	return 0;
}

static int sensor_s_gain(int id,int gain_val)
{
	setSensorGain(id, gain_val * 4);

	return 0;
}

static int gc1084_sensor_vts = 1125;
static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	int shutter = 0, frame_length = 0;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;

	glb_exp_gain.exp_val = exp_val;
	glb_exp_gain.gain_val = gain_val;

	if (full_size) {//720p
		shutter = exp_val >> 4;
		if (shutter > gc1084_sensor_vts - 16)
			frame_length = shutter + 16;
		else
			frame_length = gc1084_sensor_vts;
		sensor_dbg("frame_length = %d\n", frame_length);
		sensor_write(id, 0x0d42, frame_length & 0xff);
		sensor_write(id, 0x0d41, frame_length >> 8);
	} else {//360p
		if (exp_val > ((gc1084_sensor_vts - 16) * 16))
			exp_val = (gc1084_sensor_vts - 16) * 16;
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
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(1000);
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(1000);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!do nothing\n");
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
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

#if SENSOR_30FPS
	{
#if RAW8
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
#else
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
#endif
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 750,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (750 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_720p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p30_regs),
	},
#endif

#if SENSOR_20FPS
	{
#if RAW8
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
#else
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
#endif
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 1125,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1125 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_720p20_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p20_regs),
	},
#endif

#if SENSOR_15FPS
	{
#if RAW8
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
#else
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
#endif
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 1500,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1500 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_720p15_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p15_regs),
	},
#endif

#if SENSOR_10FPS
	{
#if RAW8
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
#else
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
#endif
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 2250,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 10,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (2250 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_720p10_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p10_regs),
	},
#endif

#else
	{
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.width      = 640,
		.height     = 360,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1800,
		.vts        = 750,
		.pclk       = 162000000,
		.mipi_bps   = 648 * 1000 * 1000,
		.fps_fixed  = 120,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (750 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_360p120_regs,
		.regs_size  = ARRAY_SIZE(sensor_360p120_regs),
	}
#endif
};

static struct sensor_format_struct *sensor_get_format(int id, int isp_id, int vinc_id)
{
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC
	struct sensor_format_struct *sensor_format = NULL;

	if (current_win[id])
		return current_win[id];

	sensor_format = sensor_find_format(isp_id, vinc_id, sensor_formats, ARRAY_SIZE(sensor_formats));
	current_win[id] = sensor_format;
	sensor_print("fine wdr is %d, use fps is %d, width:%d, height:%d\n", sensor_format->wdr_mode, sensor_format->fps_fixed, sensor_format->width, sensor_format->height);

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
#if SENSOR_30FPS
	{
#if RAW8
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
#else
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
#endif
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 750,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (750 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.switch_regs	   = sensor_360p120fps_to_720p30fps,
		.switch_regs_size  = ARRAY_SIZE(sensor_360p120fps_to_720p30fps),
	},
#endif

#if SENSOR_20FPS
	{
#if RAW8
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
#else
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
#endif
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 1125,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1125 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.switch_regs	   = sensor_360p120fps_to_720p20fps,
		.switch_regs_size  = ARRAY_SIZE(sensor_360p120fps_to_720p20fps),
	},
#endif

#if SENSOR_15FPS
	{
#if RAW8
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
#else
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
#endif
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 1500,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1500 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.switch_regs	   = sensor_360p120fps_to_720p15fps,
		.switch_regs_size  = ARRAY_SIZE(sensor_360p120fps_to_720p15fps),
	},
#endif

#if SENSOR_10FPS
	{
#if RAW8
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
#else
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
#endif
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 2250,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 10,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (2250 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.switch_regs	   = sensor_360p120fps_to_720p10fps,
		.switch_regs_size  = ARRAY_SIZE(sensor_360p120fps_to_720p10fps),
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
	cfg->type = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#if CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
	res->res_time_hs = 0x16;
	res->deskew = 0x7;
#endif

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

	gc1084_sensor_vts = current_win[id]->vts;
//#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_FAST_CONVERGENCE
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, gc1084_sensor_vts << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 64 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, gc1084_sensor_vts << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 64 << 4);
	}

	sensor_s_exp_gain(id, &exp_gain);
#if RAW8
	if (full_size)
		sensor_write(id, 0x023e, 0x9c);
	else
		sensor_write(id, 0x023e, 0x98);
#else
	sensor_write(id, 0x023e, 0x98);
#endif
//#else  //CONFIG_ISP_HARD_LIGHTADC

//#endif

	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

#ifdef CONFIG_SENSOR_GC1084_MIPI_USE_SAME_TWI

	switch (id) {
	case 0 :
		if (sensor_stream_count[1] == 0) {
			sensor_write(id, 0x03fb, (TARGET_I2C_ADDR & 0xFF));
			sensor_set_twi_addr(id, TARGET_I2C_ADDR);
		}
		break;
	case 1 :
		if (sensor_stream_count[0] == 0) {
			sensor_write(id, 0x03fb, (TARGET_I2C_ADDR & 0xFF));
			sensor_set_twi_addr(id, TARGET_I2C_ADDR);
		}
		break;
	}
#endif

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
	gc1084_sensor_vts = current_switch_win[id]->vts;

	sensor_write(id, 0x023e, 0x00);
	if (glb_exp_gain.exp_val && glb_exp_gain.gain_val) {
		exp_gain.exp_val = glb_exp_gain.exp_val;
		exp_gain.gain_val = glb_exp_gain.gain_val;
	} else {
		exp_gain.exp_val = 15408;
		exp_gain.gain_val = 32;
	}

	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_err("cannot find 360p120fps to 720p%dfps reg\n", current_switch_win[id]->fps_fixed);
	if (ret < 0)
		return ret;
	sensor_s_exp_gain(id, &exp_gain); /* make switch_regs firstframe  */
	sensor_write(id, 0x03fe, 0x10);
	sensor_write(id, 0x03fe, 0x00);
#if RAW8
	sensor_write(id, 0x023e, 0x9c); /* stream_on */
#else
	sensor_write(id, 0x023e, 0x98); /* stream_on */
#endif
#endif
	return 0;
}

static struct sensor_driver sensor_drv = {
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_8,
};

static int sensor_test_i2c(int id)
{
	int ret;
	sensor_power(id, PWR_ON);
	ret = sensor_init(id);
	sensor_power(id, PWR_OFF);

	return ret;

}

static int sensor_probe(int id)
{
	sensor_info_set(id, &sensor_drv);
#if defined CONFIG_SENSOR_INIT_BEFORE_VIN && defined CONFIG_SENSOR_GC1084_MIPI_USE_SAME_TWI
	if (id == 0)
		sensor_set_twi_addr(id, TARGET_I2C_ADDR);
#endif
	return 0;
}

struct sensor_fuc_core gc1084_core  = {
	.g_mbus_config = sensor_g_mbus_config,
	.sensor_test_i2c = sensor_test_i2c,
	.sensor_power = sensor_power,
	.s_ir_status = sensor_set_ir,
	.s_stream = sensor_s_stream,
	.s_switch = sensor_s_switch,
	.s_exp_gain = sensor_s_exp_gain,
	.sensor_g_format = sensor_get_format,
	.sensor_g_switch_format = sensor_get_switch_format,
	.probe = sensor_probe,
};
