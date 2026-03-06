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
#define V4L2_IDENT_SENSOR   0xcb3a
#define V4L2_IDENT_SENSOR_2 0x9b3a

#define SENSOR_FRAME_RATE   15

/*
 * The SC2337p i2c address
 */
#define I2C_ADDR 0x60

#define SENSOR_NUM 0x2
#define SENSOR_NAME "sc2337p_mipi"
#define SENSOR_NAME_2 "sc2337p_mipi_2"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_exp_gain glb_exp_gain[2];
static struct sensor_format_struct *current_win[2];
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
static struct sensor_format_struct *current_switch_win[2];
#endif
#define SENSOR_15FPS 1
// #define SENSOR_10FPS 1
/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#if defined CONFIG_SENSOR_SC2337P_MASTER_SLAVE_SYNCHR_OUTPUT
static int sc2337p_synchr_max_exp_line;
static struct regval_list sensor_slave_mode_smvs_regs[] = {
	{0x3222, 0x02},//Bit[1]:Slave mode en

#if SENSOR_15FPS
	{0x322e, 0x08},//vts-rbrow
	{0x322f, 0xc6},
#endif

#if SENSOR_10FPS
	{0x322e, 0x0d},//vts-rbrow
	{0x322f, 0x2b},
#endif

	{0x3224, 0x82},//efsync
	{0x300a, 0x24},
};

static struct regval_list sensor_master_mode_smvs_regs[] = {
	{0x300a, 0x24}, //bit[2]=1 fsync_oen
	{0x3032, 0xa0}, //bit[7]=1 vsync_tc_en
};
#endif

#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC
#if SENSOR_15FPS
static struct regval_list sensor_1080p_15fps_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x3018, 0x12},
	{0x3019, 0x0e},
	{0x301f, 0x10},
	{0x3106, 0x05},

	{0x320e, 0x08},//2250
	{0x320f, 0xCA},
	{0x3248, 0x04},
	{0x3249, 0x0b},
	{0x3253, 0x08},
	{0x3301, 0x09},
	{0x3302, 0xff},
	{0x3303, 0x10},
	{0x3306, 0x80},
	{0x3307, 0x02},
	{0x3309, 0xc8},
	{0x330a, 0x01},
	{0x330b, 0x30},
	{0x330c, 0x16},
	{0x330d, 0xff},
	{0x3318, 0x02},
	{0x331f, 0xb9},
	{0x3321, 0x0a},
	{0x3327, 0x0e},
	{0x332b, 0x12},
	{0x3333, 0x10},
	{0x3334, 0x40},
	{0x335e, 0x06},
	{0x335f, 0x0a},
	{0x3364, 0x1f},
	{0x337c, 0x02},
	{0x337d, 0x0e},
	{0x3390, 0x09},
	{0x3391, 0x0f},
	{0x3392, 0x1f},
	{0x3393, 0x20},
	{0x3394, 0x20},
	{0x3395, 0xe0},
	{0x33a2, 0x04},
	{0x33b1, 0x80},
	{0x33b2, 0x68},
	{0x33b3, 0x42},
	{0x33f9, 0x90},
	{0x33fb, 0xd0},
	{0x33fc, 0x0f},
	{0x33fd, 0x1f},
	{0x349f, 0x03},
	{0x34a6, 0x0f},
	{0x34a7, 0x1f},
	{0x34a8, 0x42},
	{0x34a9, 0x18},
	{0x34aa, 0x01},
	{0x34ab, 0x43},
	{0x34ac, 0x01},
	{0x34ad, 0x80},
	{0x3630, 0xf4},
	{0x3632, 0x44},
	{0x3633, 0x22},
	{0x3639, 0xf4},
	{0x363c, 0x47},

	{0x3650, 0x33},
	{0x3651, 0x7f},

	{0x3670, 0x09},
	{0x3674, 0xf4},
	{0x3675, 0xfb},
	{0x3676, 0xed},
	{0x367c, 0x09},
	{0x367d, 0x0f},
	{0x3690, 0x22},
	{0x3691, 0x22},
	{0x3692, 0x22},
	{0x3698, 0x89},
	{0x3699, 0x96},
	{0x369a, 0xd0},
	{0x369b, 0xd0},
	{0x369c, 0x09},
	{0x369d, 0x0f},
	{0x36a2, 0x09},
	{0x36a3, 0x0f},
	{0x36a4, 0x1f},
	{0x36d0, 0x01},
	{0x36ec, 0x0c},
	{0x3722, 0xc1},
	{0x3724, 0x41},
	{0x3725, 0xc1},
	{0x3728, 0x20},
	{0x3900, 0x0d},
	{0x3905, 0x98},
	{0x3919, 0x04},
	{0x391b, 0x81},
	{0x391c, 0x10},
	{0x3933, 0x81},
	{0x3934, 0xd0},
	{0x3940, 0x75},
	{0x3941, 0x00},
	{0x3942, 0x01},
	{0x3943, 0xd1},
	{0x3952, 0x02},
	{0x3953, 0x0f},
	{0x3e01, 0x45},
	{0x3e02, 0xf0},
	{0x3e08, 0x1f},
	{0x3e1b, 0x14},
	{0x4509, 0x38},

	{0x4819, 0x09},
	{0x481b, 0x05},
	{0x481d, 0x14},
	{0x481f, 0x04},
	{0x4821, 0x0a},
	{0x4823, 0x05},
	{0x4825, 0x04},
	{0x4827, 0x05},
	{0x4829, 0x08},

	{0x5799, 0x06},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x28},
	{0x5ae4, 0x20},
	{0x5ae5, 0x30},
	{0x5ae6, 0x28},
	{0x5ae7, 0x20},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x28},
	{0x5af6, 0x20},
	{0x5af7, 0x30},
	{0x5af8, 0x28},
	{0x5af9, 0x20},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
	{0x36e9, 0x20},
	{0x37f9, 0x27},
	{0x0100, 0x01},
};
#endif

#if SENSOR_10FPS
static struct regval_list sensor_1080p_10fps_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x3018, 0x12},
	{0x3019, 0x0e},
	{0x301f, 0x10},
	{0x3106, 0x05},

	{0x320e, 0x0d},//3375
	{0x320f, 0x2f},
	{0x3248, 0x04},
	{0x3249, 0x0b},
	{0x3253, 0x08},
	{0x3301, 0x09},
	{0x3302, 0xff},
	{0x3303, 0x10},
	{0x3306, 0x80},
	{0x3307, 0x02},
	{0x3309, 0xc8},
	{0x330a, 0x01},
	{0x330b, 0x30},
	{0x330c, 0x16},
	{0x330d, 0xff},
	{0x3318, 0x02},
	{0x331f, 0xb9},
	{0x3321, 0x0a},
	{0x3327, 0x0e},
	{0x332b, 0x12},
	{0x3333, 0x10},
	{0x3334, 0x40},
	{0x335e, 0x06},
	{0x335f, 0x0a},
	{0x3364, 0x1f},
	{0x337c, 0x02},
	{0x337d, 0x0e},
	{0x3390, 0x09},
	{0x3391, 0x0f},
	{0x3392, 0x1f},
	{0x3393, 0x20},
	{0x3394, 0x20},
	{0x3395, 0xe0},
	{0x33a2, 0x04},
	{0x33b1, 0x80},
	{0x33b2, 0x68},
	{0x33b3, 0x42},
	{0x33f9, 0x90},
	{0x33fb, 0xd0},
	{0x33fc, 0x0f},
	{0x33fd, 0x1f},
	{0x349f, 0x03},
	{0x34a6, 0x0f},
	{0x34a7, 0x1f},
	{0x34a8, 0x42},
	{0x34a9, 0x18},
	{0x34aa, 0x01},
	{0x34ab, 0x43},
	{0x34ac, 0x01},
	{0x34ad, 0x80},
	{0x3630, 0xf4},
	{0x3632, 0x44},
	{0x3633, 0x22},
	{0x3639, 0xf4},
	{0x363c, 0x47},

	{0x3650, 0x33},
	{0x3651, 0x7f},

	{0x3670, 0x09},
	{0x3674, 0xf4},
	{0x3675, 0xfb},
	{0x3676, 0xed},
	{0x367c, 0x09},
	{0x367d, 0x0f},
	{0x3690, 0x22},
	{0x3691, 0x22},
	{0x3692, 0x22},
	{0x3698, 0x89},
	{0x3699, 0x96},
	{0x369a, 0xd0},
	{0x369b, 0xd0},
	{0x369c, 0x09},
	{0x369d, 0x0f},
	{0x36a2, 0x09},
	{0x36a3, 0x0f},
	{0x36a4, 0x1f},
	{0x36d0, 0x01},
	{0x36ec, 0x0c},
	{0x3722, 0xc1},
	{0x3724, 0x41},
	{0x3725, 0xc1},
	{0x3728, 0x20},
	{0x3900, 0x0d},
	{0x3905, 0x98},
	{0x3919, 0x04},
	{0x391b, 0x81},
	{0x391c, 0x10},
	{0x3933, 0x81},
	{0x3934, 0xd0},
	{0x3940, 0x75},
	{0x3941, 0x00},
	{0x3942, 0x01},
	{0x3943, 0xd1},
	{0x3952, 0x02},
	{0x3953, 0x0f},
	{0x3e01, 0x45},
	{0x3e02, 0xf0},
	{0x3e08, 0x1f},
	{0x3e1b, 0x14},
	{0x4509, 0x38},

	{0x4819, 0x09},
	{0x481b, 0x05},
	{0x481d, 0x14},
	{0x481f, 0x04},
	{0x4821, 0x0a},
	{0x4823, 0x05},
	{0x4825, 0x04},
	{0x4827, 0x05},
	{0x4829, 0x08},

	{0x5799, 0x06},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x28},
	{0x5ae4, 0x20},
	{0x5ae5, 0x30},
	{0x5ae6, 0x28},
	{0x5ae7, 0x20},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x28},
	{0x5af6, 0x20},
	{0x5af7, 0x30},
	{0x5af8, 0x28},
	{0x5af9, 0x20},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
	{0x36e9, 0x20},
	{0x37f9, 0x27},
	{0x0100, 0x01},
};
#endif

#endif

#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
static struct regval_list sensor_480x270p120_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x3018, 0x12},
	{0x3019, 0x0e},
	{0x301f, 0x26},
	{0x3106, 0x05},
	{0x3200, 0x01},
	{0x3201, 0xd4},
	{0x3202, 0x01},
	{0x3203, 0x0e},
	{0x3204, 0x05},
	{0x3205, 0xb3},
	{0x3206, 0x03},
	{0x3207, 0x31},
	{0x3208, 0x01},
	{0x3209, 0xe0},
	{0x320a, 0x01},
	{0x320b, 0x0e},
	{0x320c, 0x07},
	{0x320d, 0xfa},
	{0x320e, 0x01},
	{0x320f, 0x2f},
	{0x3210, 0x00},
	{0x3211, 0x08},
	{0x3212, 0x00},
	{0x3213, 0x02},
	{0x3215, 0x31},
	{0x3220, 0x17},
	{0x3248, 0x04},
	{0x3249, 0x0b},
	{0x3253, 0x08},
	{0x3301, 0x09},
	{0x3302, 0xff},
	{0x3303, 0x10},
	{0x3306, 0x80},
	{0x3307, 0x02},
	{0x3309, 0xc8},
	{0x330a, 0x01},
	{0x330b, 0x30},
	{0x330c, 0x16},
	{0x330d, 0xff},
	{0x3318, 0x02},
	{0x331f, 0xb9},
	{0x3321, 0x0a},
	{0x3327, 0x0e},
	{0x332b, 0x12},
	{0x3333, 0x10},
	{0x3334, 0x40},
	{0x335e, 0x06},
	{0x335f, 0x0a},
	{0x3364, 0x1f},
	{0x337c, 0x02},
	{0x337d, 0x0e},
	{0x3390, 0x09},
	{0x3391, 0x0f},
	{0x3392, 0x1f},
	{0x3393, 0x20},
	{0x3394, 0x20},
	{0x3395, 0xe0},
	{0x33a2, 0x04},
	{0x33b1, 0x80},
	{0x33b2, 0x68},
	{0x33b3, 0x42},
	{0x33f9, 0x90},
	{0x33fb, 0xc8},
	{0x33fc, 0x0f},
	{0x33fd, 0x1f},
	{0x349f, 0x03},
	{0x34a6, 0x0f},
	{0x34a7, 0x1f},
	{0x34a8, 0x42},
	{0x34a9, 0x18},
	{0x34aa, 0x01},
	{0x34ab, 0x43},
	{0x34ac, 0x01},
	{0x34ad, 0x78},
	{0x3630, 0xf4},
	{0x3632, 0x44},
	{0x3633, 0x22},
	{0x3639, 0xf4},
	{0x363c, 0x47},
	{0x3670, 0x09},
	{0x3674, 0xf4},
	{0x3675, 0xfb},
	{0x3676, 0xed},
	{0x367c, 0x09},
	{0x367d, 0x0f},
	{0x3690, 0x22},
	{0x3691, 0x22},
	{0x3692, 0x22},
	{0x3698, 0x89},
	{0x3699, 0x96},
	{0x369a, 0xd0},
	{0x369b, 0xd0},
	{0x369c, 0x09},
	{0x369d, 0x0f},
	{0x36a2, 0x09},
	{0x36a3, 0x0f},
	{0x36a4, 0x1f},
	{0x36d0, 0x01},
	{0x36ec, 0x1c},
	{0x3722, 0xc1},
	{0x3724, 0x41},
	{0x3725, 0xc1},
	{0x3728, 0x20},
	{0x3900, 0x0d},
	{0x3905, 0x98},
	{0x3919, 0x04},
	{0x391b, 0x81},
	{0x391c, 0x10},
	{0x3933, 0x81},
	{0x3934, 0xd0},
	{0x3940, 0x75},
	{0x3941, 0x00},
	{0x3942, 0x01},
	{0x3943, 0xd1},
	{0x3952, 0x02},
	{0x3953, 0x0f},
	{0x3e01, 0x12},
	{0x3e02, 0x90},
	{0x3e08, 0x1f},
	{0x3e1b, 0x14},
	{0x4509, 0x38},
	{0x4819, 0x05},
	{0x481b, 0x03},
	{0x481d, 0x0a},
	{0x481f, 0x02},
	{0x4821, 0x08},
	{0x4823, 0x03},
	{0x4825, 0x02},
	{0x4827, 0x03},
	{0x4829, 0x04},
	{0x5000, 0x46},
	{0x5011, 0x40},
	{0x5799, 0x06},
	{0x5900, 0xf1},
	{0x5901, 0x04},
	{0x5988, 0x70},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x28},
	{0x5ae4, 0x20},
	{0x5ae5, 0x30},
	{0x5ae6, 0x28},
	{0x5ae7, 0x20},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x28},
	{0x5af6, 0x20},
	{0x5af7, 0x30},
	{0x5af8, 0x28},
	{0x5af9, 0x20},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
	{0x36e9, 0x20},
	{0x37f9, 0x27},

	{0x3021, 0x86},
	// {0x0100, 0x01},
};

#if SENSOR_15FPS
static struct regval_list sensor_270p_120fps_to_1080p_15fps_regs[] = {
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x3200, 0x00},
	{0x3201, 0x00},
	{0x3202, 0x00},
	{0x3203, 0x00},
	{0x3204, 0x07},
	{0x3205, 0x87},
	{0x3206, 0x04},
	{0x3207, 0x3f},
	{0x3208, 0x07},
	{0x3209, 0x80},
	{0x320a, 0x04},
	{0x320b, 0x38},
	{0x320c, 0x08},
	{0x320d, 0x98},
	{0x320e, 0x08},
	{0x320f, 0xca},
	{0x3210, 0x00},
	{0x3211, 0x04},
	{0x3212, 0x00},
	{0x3213, 0x04},
	{0x3215, 0x11},
	{0x3220, 0x13},
	{0x33fb, 0xd0},
	{0x34ad, 0x80},
	{0x36ec, 0x0c},
	{0x3e01, 0x8b},
	{0x3e02, 0xf0},
	{0x4819, 0x09},
	{0x481b, 0x05},
	{0x481d, 0x14},
	{0x481f, 0x04},
	{0x4821, 0x0a},
	{0x4823, 0x05},
	{0x4825, 0x04},
	{0x4827, 0x05},
	{0x4829, 0x08},
	{0x5000, 0x06},
	{0x5011, 0xc0},
	{0x5900, 0x01},
	{0x5901, 0x00},
	{0x5988, 0x80},
	{0x36e9, 0x20},
	{0x37f9, 0x27},

	{0x3021, 0x86},
	// {0x0100, 0x01},
};
#endif

#if SENSOR_10FPS
static struct regval_list sensor_270p_120fps_to_1080p_10fps_regs[] = {
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x3200, 0x00},
	{0x3201, 0x00},
	{0x3202, 0x00},
	{0x3203, 0x00},
	{0x3204, 0x07},
	{0x3205, 0x87},
	{0x3206, 0x04},
	{0x3207, 0x3f},
	{0x3208, 0x07},
	{0x3209, 0x80},
	{0x320a, 0x04},
	{0x320b, 0x38},
	{0x320c, 0x08},
	{0x320d, 0x98},
	{0x320e, 0x0d},
	{0x320f, 0x2f},
	{0x3210, 0x00},
	{0x3211, 0x04},
	{0x3212, 0x00},
	{0x3213, 0x04},
	{0x3215, 0x11},
	{0x3220, 0x13},
	{0x33fb, 0xd0},
	{0x34ad, 0x80},
	{0x36ec, 0x0c},
	{0x3e01, 0x8b},
	{0x3e02, 0xf0},
	{0x4819, 0x09},
	{0x481b, 0x05},
	{0x481d, 0x14},
	{0x481f, 0x04},
	{0x4821, 0x0a},
	{0x4823, 0x05},
	{0x4825, 0x04},
	{0x4827, 0x05},
	{0x4829, 0x08},
	{0x5000, 0x06},
	{0x5011, 0xc0},
	{0x5900, 0x01},
	{0x5901, 0x00},
	{0x5988, 0x80},
	{0x36e9, 0x20},
	{0x37f9, 0x27},
	// {0x0100, 0x01},
};
#endif

#endif

// #define PRINT_EXP
static int sensor_s_exp(int id, unsigned int exp_val)
{
	data_type explow, expmid, exphigh;

	exphigh = (unsigned char) (0x0f & (exp_val >> 16));
	expmid = (unsigned char) (0xff & (exp_val >> 8));
	explow = (unsigned char) (0xf0 & (exp_val));

	sensor_write(id, 0x3e02, explow);
	sensor_write(id, 0x3e01, expmid);
	sensor_write(id, 0x3e00, exphigh);

#ifdef PRINT_EXP
	data_type explow_read, expmid_read, exphigh_read;
	sensor_read(id, 0x3e00, &exphigh_read);
	sensor_read(id, 0x3e01, &expmid_read);
	sensor_read(id, 0x3e02, &explow_read);
	printf("sensor%d 0x3e00 = 0x%x, 0x3e01 = 0x%x, 0x3e02 = 0x%x\n", id, exphigh_read, expmid_read, explow_read);
#endif

	return 0;
}

static int sensor_s_gain(int id, int gain_val)
{
	data_type anagain = 0x00;
	data_type gaindiglow = 0x80;
	data_type gaindighigh = 0x00;
	int gain_tmp;

	gain_tmp = gain_val << 3;
	if (gain_val < 32) {// 16 * 2
		anagain = 0x00;
		gaindighigh = 0x00;
		gaindiglow = gain_tmp;
	} else if (gain_val < 64) {//16 * 4
		anagain = 0x08;
		gaindighigh = 0x00;
		gaindiglow = gain_tmp * 100 / 200 / 1;
	} else if (gain_val < 128) {//16 * 8
		anagain = 0x09;
		gaindighigh = 0x00;
		gaindiglow = gain_tmp * 100 / 200 / 2;
	} else if (gain_val < 256) {//16 * 16
		anagain = 0x0b;
		gaindighigh = 0x00;
		gaindiglow = gain_tmp * 100 / 200 / 4;
	} else if (gain_val < 512) {//16 * 32
		anagain = 0x0f;
		gaindighigh = 0x00;
		gaindiglow = gain_tmp * 100 / 200 / 8;
	} else if (gain_val < 1024) {//16 * 32 * 2
		anagain = 0x1f;
		gaindighigh = 0x00;
		gaindiglow = gain_tmp * 100 / 200 / 16;
	} else if (gain_val < 2048) {//16 * 32 * 4
		anagain = 0x1f;
		gaindighigh = 0x01;
		gaindiglow = gain_tmp * 100 / 200 / 32;
	} else {
		anagain = 0x1f;
		gaindighigh = 0x01;
		gaindiglow = 0xfc;
	}

	sensor_write(id, 0x3e09, (unsigned char)anagain);
	sensor_write(id, 0x3e07, (unsigned char)gaindiglow);
	sensor_write(id, 0x3e06, (unsigned char)gaindighigh);

	sensor_dbg("sensor_set_anagain = %d, 0x%x, 0x%x, 0x%x Done!\n", gain_val, anagain, gaindighigh, gaindiglow);

	return 0;

}

static int sc2337p_sensor_vts;
static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
#if !defined CONFIG_SENSOR_SC2337P_MASTER_SLAVE_SYNCHR_OUTPUT
	int shutter = 0, frame_length = 0;
#endif

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;
	if (gain_val < 1 * 16)
		gain_val = 16;
#if defined CONFIG_SENSOR_SC2337P_MASTER_SLAVE_SYNCHR_OUTPUT
	if (exp_val > sc2337p_synchr_max_exp_line)
		exp_val = sc2337p_synchr_max_exp_line;
#else
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	shutter = exp_val >> 4;
	if (shutter > sc2337p_sensor_vts - 8) {
		frame_length = shutter + 8;
	} else
		frame_length = sc2337p_sensor_vts;
	sensor_write(id, 0x320f, (frame_length & 0xff));
	sensor_write(id, 0x320e, (frame_length >> 8));
#endif

	//sensor_write(id, 0x3812, 0x00);//group_hold
	sensor_s_exp(id, exp_val);
	sensor_s_gain(id, gain_val);
	//sensor_write(id, 0x3812, 0x30);

	glb_exp_gain[id].exp_val = exp_val;
	glb_exp_gain[id].gain_val = gain_val;
	sensor_dbg("sensor%d exp_val = %d, gain_val = %d\n", id, exp_val, gain_val);

	return 0;
}

#if defined CONFIG_SENSOR_SC2337P_MULTIPLEXES_RESET
static int sensor_power_flag;
#endif
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
		sensor_dbg("id %d PWR_ON!\n", id);
#if defined CONFIG_SENSOR_SC2337P_MULTIPLEXES_RESET
		if (++sensor_power_flag > 1) {
			sensor_print("sensor_power_flag = %d, SC2337P multiplexes RESET, it needs to be operated once when power on\n", sensor_power_flag);
			break;
		}
#endif
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
#if defined CONFIG_SENSOR_SC2337P_MULTIPLEXES_RESET
		sensor_dbg("id %d PWR_OFF!do nothing\n", id);
		if (--sensor_power_flag >= 1) {
			sensor_print("sensor_power_flag = %d, SC2337P multiplexes RESET, it needs to be operated once when power off\n", sensor_power_flag);
			break;
		}
#endif
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
	// vin_gpio_set_status(id, IR_LED, 1);

	// hal_writel(0x00000003, 0x42000358);
	// sensor_print("id: %d, 0x42000358: 0x%x, 0x420000ac: 0x%x\n", id, hal_readl(0x42000358), hal_readl(0x420000ac));

	switch (status) {
	case IR_DAY:
		vin_gpio_write(id, IR_CUT0, CSI_GPIO_HIGH);
		vin_gpio_write(id, IR_CUT1, CSI_GPIO_LOW);
		// vin_gpio_write(id, IR_LED, CSI_GPIO_LOW);
		break;
	case IR_NIGHT:
		vin_gpio_write(id, IR_CUT0, CSI_GPIO_LOW);
		vin_gpio_write(id, IR_CUT1, CSI_GPIO_HIGH);
		// vin_gpio_write(id, IR_LED, CSI_GPIO_HIGH);
		break;
	default:
		return -1;
	}
	vin_gpio_set_status(0, IR_LED, 1);
	switch (status) {
	case IR_DAY:
		vin_gpio_write(0, IR_LED, CSI_GPIO_LOW);
		break;
	case IR_NIGHT:
		vin_gpio_write(0, IR_LED, CSI_GPIO_HIGH);
		break;
	default:
		return -1;
	}

	// sensor_print("id: %d, 0x42000098: 0x%x, 0x420000a0: 0x%x\n", id, hal_readl(0x42000098), hal_readl(0x420000a0));

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

	while ((SENSOR_ID != V4L2_IDENT_SENSOR && SENSOR_ID != V4L2_IDENT_SENSOR_2) && (cnt < 5)) {
		sensor_read(id, 0x3107, &rdval);
		SENSOR_ID |= (rdval << 8);
		sensor_read(id, 0x3108, &rdval);
		SENSOR_ID |= (rdval);
		sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n",
			cnt, SENSOR_ID);
		cnt++;
	}
	if (SENSOR_ID != V4L2_IDENT_SENSOR && SENSOR_ID != V4L2_IDENT_SENSOR_2)
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
#if SENSOR_15FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 2250,
		.pclk       = 74250000,
		.mipi_bps   = 742500000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
#if defined CONFIG_SENSOR_SC2337P_MASTER_SLAVE_SYNCHR_OUTPUT
		.intg_max   = (2250 - 4 - 6) << 4,
#else
		.intg_max   = (2250 - 6) << 4,
#endif
		.gain_min   = 1 << 4,
		.gain_max   = 128 << 4,
		.regs       = sensor_1080p_15fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p_15fps_regs),
	},
#endif

#if SENSOR_10FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 3375,
		.pclk       = 74250000,
		.mipi_bps   = 742500000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (3375 - 6) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 128 << 4,
		.regs       = sensor_1080p_10fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p_10fps_regs),
	},
#endif

#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 480,
		.height     = 270,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1021,
		.vts        = 303,
		.pclk       = 37125000,
		.mipi_bps   = 371250000,
		.fps_fixed  = 120,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (303 - 6) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 128 << 4,
		.regs       = sensor_480x270p120_regs,
		.regs_size  = ARRAY_SIZE(sensor_480x270p120_regs),
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
#if SENSOR_15FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 2250,
		.pclk       = 74250000,
		.mipi_bps   = 742500000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
#if defined CONFIG_SENSOR_SC2337P_MASTER_SLAVE_SYNCHR_OUTPUT
		.intg_max   = (2250 - 4 - 6) << 4,
#else
		.intg_max   = (2250 - 6) << 4,
#endif
		.gain_min   = 1 << 4,
		.gain_max   = 128 << 4,
		.switch_regs       = sensor_270p_120fps_to_1080p_15fps_regs,
		.switch_regs_size  = ARRAY_SIZE(sensor_270p_120fps_to_1080p_15fps_regs),
	},
#endif

#if SENSOR_10FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 3375,
		.pclk       = 74250000,
		.mipi_bps   = 742500000,
		.fps_fixed  = 10,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (3375 - 6) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 128 << 4,
		.switch_regs       = sensor_270p_120fps_to_1080p_10fps_regs,
		.switch_regs_size  = ARRAY_SIZE(sensor_270p_120fps_to_1080p_10fps_regs),
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
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	res->deskew = 0x05;

	return 0;
}

static void sensor_s_flip_before_streamon(int id)
{
	struct tagsensor_isp_config_s *sensor_isp_cfg = NULL;
	data_type flip_status;

	if (id == 0)
		sensor_isp_cfg = (struct tagsensor_isp_config_s *)VIN_SENSOR0_RESERVE_ADDR;
	else
		sensor_isp_cfg = (struct tagsensor_isp_config_s *)VIN_SENSOR1_RESERVE_ADDR;

	if ((id == 0 && sensor_isp_cfg->sign == 0xAA66AA66) ||
		(id != 0 && sensor_isp_cfg->sign == 0xBB66BB66)) {
		sensor_read(id, 0x3221, &flip_status);
		if (sensor_isp_cfg->filp) {
			flip_status = flip_status | 0x60;
		} else {
			flip_status = flip_status & 0x9f;
		}
		if (sensor_isp_cfg->mirror) {
			flip_status = flip_status | 0x06;
		} else {
			flip_status = flip_status & 0xf9;
		}
		sensor_write(id, 0x3221, flip_status);
		sensor_print("id: %d, flip = %d mirror = %d, set_to_status: 0x%x\n",
				id, sensor_isp_cfg->filp, sensor_isp_cfg->mirror, flip_status);
	} else {
		sensor_err("sensor_isp_cfg is NULL\n");
	}
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

	sc2337p_sensor_vts = current_win[id]->vts;
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, sc2337p_sensor_vts << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 128 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, sc2337p_sensor_vts << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 128 << 4);
	}
	sensor_dbg("%s, id:%d exp_val = %d, gain_val = %d\n", __func__, id, exp_gain.exp_val, exp_gain.gain_val);

#if defined CONFIG_SENSOR_SC2337P_MASTER_SLAVE_SYNCHR_OUTPUT
	sc2337p_synchr_max_exp_line = current_win[id]->intg_max;
	sensor_dbg("id:%d sc2337p_synchr_max_exp_line = %d\n", id, sc2337p_synchr_max_exp_line);
#endif

	sensor_s_flip_before_streamon(id);

	sensor_s_exp_gain(id, &exp_gain);

#if defined CONFIG_SENSOR_SC2337P_MASTER_SLAVE_SYNCHR_OUTPUT && defined CONFIG_ISP_READ_THRESHOLD
	if (id == 0) {
		sensor_print("sensor%d  sync  master  mode reg init. \n", id);
		sensor_write_array(id, sensor_master_mode_smvs_regs,  ARRAY_SIZE(sensor_master_mode_smvs_regs));
	}

	if (id == 1) {
		sensor_print("sensor%d  sync  slave  mode reg init. \n", id);
		sensor_write_array(id, sensor_slave_mode_smvs_regs,  ARRAY_SIZE(sensor_slave_mode_smvs_regs));
	}
#elif defined CONFIG_SENSOR_SC2337P_MASTER_SYNCHR_OUTPUT
	if (id == 1) {
		sensor_print("streamon_toether\n");
		sensor_write((id - 1), 0x0100, 0x01);
		sensor_write(id, 0x0100, 0x01);
	}
#else
	sensor_write(id, 0x0100, 0x01);
#endif

	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

	sensor_dbg("%s on = %d, 1920*1080 fps: 20\n", __func__, enable);

	if (!enable)
		return 0;

	return sensor_reg_init(id, isp_id);
}

static int sensor_s_switch(int id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	struct sensor_exp_gain exp_gain;
	int ret = -1;

	sc2337p_sensor_vts = current_switch_win[id]->vts;
	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_err("cannot find 1152x320p120fps to 1080p%dfps reg\n", current_switch_win[id]->fps_fixed);
	if (ret < 0)
		return ret;

	if (glb_exp_gain[id].exp_val && glb_exp_gain[id].gain_val) {
		exp_gain.exp_val = glb_exp_gain[id].exp_val;
		exp_gain.gain_val = glb_exp_gain[id].gain_val;
	} else {
		exp_gain.exp_val = 1000;
		exp_gain.gain_val = 16;
	}

	sensor_s_flip_before_streamon(id);

#if defined CONFIG_SENSOR_SC2337P_MASTER_SYNCHR_OUTPUT
	if (id == 1) {
		sensor_print("id: %d, exp: %d, gain: %d\n", id, exp_gain.exp_val, exp_gain.gain_val);
		/* set same exp */
		sensor_s_exp_gain((id - 1), &exp_gain);
		sensor_s_exp_gain(id, &exp_gain);
		sensor_write(id, 0x0100, 0x01);
		sensor_write((id - 1), 0x0100, 0x01);
	}
#elif defined CONFIG_SENSOR_SC2337P_MASTER_SLAVE_SYNCHR_OUTPUT
	if (id == 0) {
		sensor_print("sensor%d  sync  master  mode reg init. \n", id);
		sensor_write_array(id, sensor_master_mode_smvs_regs,  ARRAY_SIZE(sensor_master_mode_smvs_regs));
	}
	sc2337p_synchr_max_exp_line = current_switch_win[id]->intg_max;
	sensor_print("id:%d sc2337p_synchr_max_exp_line = %d after switch\n", id, sc2337p_synchr_max_exp_line);

	if (id == 1) {
		sensor_print("sensor%d  sync  slave  mode reg init. \n", id);
		sensor_write_array(id, sensor_slave_mode_smvs_regs,  ARRAY_SIZE(sensor_slave_mode_smvs_regs));
		 /* set streamon toether  */
		sensor_print("id: %d, exp: %d, gain: %d\n", id, exp_gain.exp_val, exp_gain.gain_val);
		sensor_s_exp_gain((id - 1), &exp_gain);
		sensor_s_exp_gain(id, &exp_gain);
#ifdef CONFIG_SENSOR_SC2337P_STREAMON_IN_KERNEL
		sensor_print("sc2337p streamon in kernel for synchr output stablely\n");
#else
		sensor_write((id - 1), 0x0100, 0x01);
		sensor_write(id, 0x0100, 0x01);
#endif
	}
#else
	sensor_s_exp_gain(id, &exp_gain); /* make switch_regs firstframe  */
	sensor_write(id, 0x0100, 0x01);
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
#if defined CONFIG_SENSOR_SC2337P_MASTER_SLAVE_SYNCHR_OUTPUT
	sensor_dbg("sc2337p is working by MASTER_SLAVE_SYNCHR_OUTPUT\n");
#endif

#if defined CONFIG_SENSOR_SC2337P_MASTER_SYNCHR_OUTPUT
	sensor_dbg("sc2337p is working by MASTER_SYNCHR_OUTPUT\n");
#endif

	sensor_power_count[0] = 0;
	sensor_power_count[1] = 0;
	sensor_stream_count[0] = 0;
	sensor_stream_count[1] = 0;
	memset(&glb_exp_gain, 0, 2 * sizeof(struct sensor_exp_gain));
	current_win[0] = NULL;
	current_win[1] = NULL;
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	current_switch_win[0] = NULL;
	current_switch_win[1] = NULL;
#endif
	vind_default_mclk[global_sensors[id].mclk_id].use_count = 0;

	return 0;
}

struct sensor_fuc_core sc2337p_core  = {
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
