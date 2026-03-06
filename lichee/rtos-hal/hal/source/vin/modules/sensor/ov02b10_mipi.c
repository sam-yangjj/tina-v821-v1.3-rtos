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
#define V4L2_IDENT_SENSOR  0x2B

//define the registers
#define EXP_HIGH		0xff
#define EXP_MID			0x03
#define EXP_LOW			0x04
#define GAIN_HIGH		0xff
#define GAIN_LOW		0x24

/*
 * The  i2c address
 */
#define OV02B1B_I2C_ADDR  (0x78>>1)
#define OV02B10_I2C_ADDR  (0x7A>>1)

#define SENSOR_NUM		2
#define SENSOR_NAME		"ov02b10_mipi"
#define SENSOR_NAME_2	"ov02b1b_mipi"

#define SENSOR_1600x1200_30FPS  1
#define SENSOR_1280x720_30FPS	0
#define SENSOR_1280x720_15FPS	0
#define SENSOR_800x600_15FPS	0
#define SENSOR_640x480_15FPS	0

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
static struct sensor_format_struct *current_switch_win[2];
#endif

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#if SENSOR_1600x1200_30FPS
#if 0
static struct regval_list sensor_1600x1200_30fps_regs[] = {
	{0xfc, 0x01},//   ;soft reset
//;delay 5ms
//sl 5 5
	{0xfd, 0x00},
	{0x24, 0x02},   //;pll_mc
	{0x25, 0x06},   //;pll_nc,dpll clk 72M
	{0x29, 0x01},
	{0x2a, 0xb4},   //;mpll_nc, mpll clk 660M
	{0x2b, 0x00},
	{0x1e, 0x17},   //;vlow 0.53v
	{0x33, 0x07},   //;ipx 2.84u
	{0x35, 0x07},
	{0x4a, 0x0c},   //;ncp -1.4v
	{0x3a, 0x05},   //;icomp1 4.25u
	{0x3b, 0x02},   //;icomp2 1.18u
	{0x3e, 0x00},
	{0x46, 0x01},
	{0x6d, 0x03},
	{0xfd, 0x01},
	{0x0e, 0x02},
	{0x0f, 0x1a},   //;exp
	{0x18, 0x00},   //;un fixed-fps
	{0x22, 0xff},   //;analog gain
	{0x23, 0x02},   //;adc_range 0.595v
	{0x17, 0x2c},   //;pd reset row address time
	{0x19, 0x20},   //;dac_d0 1024
	{0x1b, 0x06},   //;rst_num1 96
	{0x1c, 0x04},   //;rst_num2 64
	{0x20, 0x03},
	{0x30, 0x01},   //;p0
	{0x33, 0x01},   //;p3
	{0x31, 0x0a},   //;p1
	{0x32, 0x09},   //;p2
	{0x38, 0x01},
	{0x39, 0x01},   //;p9
	{0x3a, 0x01},   //;p10
	{0x3b, 0x01},
	{0x4f, 0x04},   //;p24
	{0x4e, 0x05},   //;p23
	{0x50, 0x01},   //;p25
	{0x35, 0x0c},   //;p5
	{0x45, 0x2a},   //;sc1,p20_1
	{0x46, 0x2a},   //;p20_2
	{0x47, 0x2a},   //;p20_3
	{0x48, 0x2a},   //;p20_4
	{0x4a, 0x2c},   //;sc2,p22_1
	{0x4b, 0x2c},   //;p22_2
	{0x4c, 0x2c},   //;p22_3
	{0x4d, 0x2c},   //;p22_4
	{0x56, 0x3a},   //;p31, 1st d0
	{0x57, 0x0a},   //;p32, 1st d1
	{0x58, 0x24},   //;col_en1
	{0x59, 0x20},   //;p34 2nd d0
	{0x5a, 0x0a},   //;p34 2nd d1
	{0x5b, 0xff},   //;col_en2
	{0x37, 0x0a},   //;p7, tx
	{0x42, 0x0e},   //;p17, psw
	{0x68, 0x90},
	{0x69, 0xcd},   //;blk en, no sig_clamp
	{0x6a, 0x8f},
	{0x7c, 0x0a},
	{0x7d, 0x09},	//;0a
	{0x7e, 0x09},	//;0a
	{0x7f, 0x08},
	{0x83, 0x14},
	{0x84, 0x14},
	{0x86, 0x14},
	{0x87, 0x07},   //;vbl2_4
	{0x88, 0x0f},
	{0x94, 0x02},   //;evsync del frame
	{0x98, 0xd1},   //;del bad frame
	{0xfe, 0x02},
	{0xfd, 0x03},   //;RegPage
	{0x97, 0x78},
	{0x98, 0x78},
	{0x99, 0x78},
	{0x9a, 0x78},
	{0xa1, 0x40},
	{0xb1, 0x30},
	{0xae, 0x0d},   //;bit0=1,high 8bit
	{0x88, 0x5b},   //;BLC_ABL
	{0x89, 0x7c},   //;bit6=1 trigger en
	{0xb4, 0x05},   //;mean trigger 5
	{0x8c, 0x40},   //;BLC_BLUE_SUBOFFSET_8lsb
	{0x8e, 0x40},   //;BLC_RED_SUBOFFSET_8lsb
	{0x90, 0x40},   //;BLC_GR_SUBOFFSET_8lsb
	{0x92, 0x40},   //; BLC_GB_SUBOFFSET_8lsb
	{0x9b, 0x46},   //;digtal gain
	{0xac, 0x40},   //;blc random noise rpc_th 4x
	{0xfd, 0x00},
	{0x5a, 0x15},
	{0x74, 0x01},   //; //PD_MIPI:turn on mipi phy

	{0xfd, 0x00},  //;crop to 1600x1200
	{0x50, 0x40},  //;mipi hszie low 8bit
	{0x52, 0xb0},  //;mipi vsize low 8bit
	{0xfd, 0x01},
	{0x03, 0x70},  //;window hstart low 8bit
	{0x05, 0x10},  //;window vstart low 8bit
	{0x07, 0x20},  //;window hsize low 8bit
	{0x09, 0xb0},  //;window vsize low 8bit


	{0xfb, 0x01},

	//;stream on
	{0xfd, 0x03},
	{0xc2, 0x01},  //;MIPI_EN
	{0xfd, 0x01},
};
#else
static struct regval_list sensor_1600x1200_30fps_regs[] = {
	{0xfc, 0x01},
	{0xfd, 0x00},
	{0xfd, 0x00},
	{0x24, 0x02},
	{0x25, 0x06},
	{0x29, 0x03},
	{0x2a, 0x34},
	{0x1e, 0x17},
	{0x33, 0x07},
	{0x35, 0x07},
	{0x4a, 0x0c},
	{0x3a, 0x05},
	{0x3b, 0x02},
	{0x3e, 0x00},
	{0x46, 0x01},
	{0x6d, 0x03},
	{0xfd, 0x01},
	{0x0e, 0x02},
	{0x0f, 0x1a},
	{0x18, 0x00},
	{0x22, 0xff},
	{0x23, 0x02},
	{0x17, 0x2c},
	{0x19, 0x20},
	{0x1b, 0x06},
	{0x1c, 0x04},
	{0x20, 0x03},
	{0x30, 0x01},
	{0x33, 0x01},
	{0x31, 0x0a},
	{0x32, 0x09},
	{0x38, 0x01},
	{0x39, 0x01},
	{0x3a, 0x01},
	{0x3b, 0x01},
	{0x4f, 0x04},
	{0x4e, 0x05},
	{0x50, 0x01},
	{0x35, 0x0c},
	{0x45, 0x2a},
	{0x46, 0x2a},
	{0x47, 0x2a},
	{0x48, 0x2a},
	{0x4a, 0x2c},
	{0x4b, 0x2c},
	{0x4c, 0x2c},
	{0x4d, 0x2c},
	{0x56, 0x3a},
	{0x57, 0x0a},
	{0x58, 0x24},
	{0x59, 0x20},
	{0x5a, 0x0a},
	{0x5b, 0xff},
	{0x37, 0x0a},
	{0x42, 0x0e},
	{0x68, 0x90},
	{0x69, 0xcd},
	{0x6a, 0x8f},
	{0x7c, 0x0a},
	{0x7d, 0x0a},
	{0x7e, 0x0a},
	{0x7f, 0x08},
	{0x83, 0x14},
	{0x84, 0x14},
	{0x86, 0x14},
	{0x87, 0x07},
	{0x88, 0x0f},
	{0x94, 0x02},
	{0x98, 0xd1},
	{0xfe, 0x02},
	{0xfd, 0x03},
	{0x97, 0x6c},	//;B
	{0x98, 0x60},	//;R
	{0x99, 0x60},	//;Gr
	{0x9a, 0x6c},	//;Gb
	{0xa1, 0x40},
	{0xb1, 0x40},
	{0xaf, 0x04},	//;blc max update frame
	{0xae, 0x0d},
	{0x88, 0x5b},
	{0x89, 0x7c},
	{0xb4, 0x05},
	{0x8c, 0x40},
	{0x8e, 0x40},
	{0x90, 0x40},
	{0x92, 0x40},
	{0x9b, 0x46},
	{0xac, 0x40},
	{0xfd, 0x00},
	{0x5a, 0x15},
	{0x74, 0x01},

	{0xfd, 0x00},  //;crop to 1600 1200
	{0x50, 0x40},  //;mipi hszie low 8bit
	{0x52, 0xb0},  //;mipi vsize low 8bit
	{0xfd, 0x01},
	{0x03, 0x70},  //;window hstart low 8bit
	{0x05, 0x10},  //;window vstart low 8bit
	{0x07, 0x20},  //;window hsize low 8bit
	{0x09, 0xb0},  //;window vsize low 8bit

	{0xfd, 0x03},  //;
	{0xc2, 0x01},  //;MIPI_EN
	{0xfb, 0x01},
	{0xfd, 0x01},
};
#endif
#endif

#if SENSOR_1280x720_30FPS
static struct regval_list sensor_720p_30fps_regs[] = {
	/* 1280x720 30fps 1 lane MIPI 600Mbps/lane */
	{0xfc, 0x01},  //soft reset
	{0xfd, 0x00},
	{0xfd, 0x00},
	{0x24, 0x02},  //pll_mc
	{0x25, 0x06},  //pll_nc,dpll clk 72M
	{0x29, 0x01},
	{0x2a, 0xAF},  //mpll_nc, mpll clk 600M
	{0x2b, 0x00},
	{0x1e, 0x17},  //vlow 0.53v
	{0x33, 0x07},  //ipx 2.84u
	{0x35, 0x07},
	{0x4a, 0x0c},  //ncp -1.4v
	{0x3a, 0x05},  //icomp1 4.25u
	{0x3b, 0x02},  //icomp2 1.18u
	{0x3e, 0x00},
	{0x46, 0x01},
	{0x6d, 0x03},
	{0xfd, 0x01},
	{0x0e, 0x04},
	{0x0f, 0xc5},  //exp
	{0x12, 0x03},  //mirror and flip
	{0x14, 0x01},
	{0x15, 0xe8},
	{0x18, 0x00},  //un fixed-fps
	{0x22, 0x40},  //analog gain
	{0x23, 0x02},  //adc_range 0.595v
	{0x17, 0x2c},  //pd reset row address time
	{0x19, 0x20},  //dac_d0 1024
	{0x1b, 0x06},  //rst_num1 96
	{0x1c, 0x04},  //rst_num2 64
	{0x20, 0x03},
	{0x30, 0x01},  //p0
	{0x33, 0x01},  //p3
	{0x31, 0x0a},  //p1
	{0x32, 0x09},  //p2
	{0x38, 0x01},
	{0x39, 0x01},  //p9
	{0x3a, 0x01},  //p10
	{0x3b, 0x01},
	{0x4f, 0x04},  //p24
	{0x4e, 0x05},  //p23
	{0x50, 0x01},  //p25
	{0x35, 0x0c},  //p5
	{0x45, 0x2a},  //sc1,p20_1
	{0x46, 0x2a},  //p20_2
	{0x47, 0x2a},  //p20_3
	{0x48, 0x2a},  //p20_4
	{0x4a, 0x2c},  //sc2,p22_1
	{0x4b, 0x2c},  //p22_2
	{0x4c, 0x2c},  //p22_3
	{0x4d, 0x2c},  //p22_4
	{0x56, 0x3a},  //p31, 1st d0
	{0x57, 0x0a},  //p32, 1st d1
	{0x58, 0x24},  //col_en1
	{0x59, 0x20},  //p34 2nd d0
	{0x5a, 0x0a},  //p34 2nd d1
	{0x5b, 0xff},  //col_en2
	{0x37, 0x0a},  //p7, tx
	{0x42, 0x0e},  //p17, psw
	{0x68, 0x90},
	{0x69, 0xcd},  //blk en, no sig_clamp
	{0x6a, 0x8f},
	{0x7c, 0x0a},
	{0x7d, 0x09},  //0a
	{0x7e, 0x09},  //0a
	{0x7f, 0x08},
	{0x83, 0x14},
	{0x84, 0x14},
	{0x86, 0x14},
	{0x87, 0x07},  //vbl2_4
	{0x88, 0x0f},
	{0x94, 0x02},  //evsync del frame
	{0x98, 0xd1},  //del bad frame
	{0xfe, 0x02},
	{0xfd, 0x03},  //RegPage
	{0x97, 0x78},
	{0x98, 0x78},
	{0x99, 0x78},
	{0x9a, 0x78},
	{0xa1, 0x40},
	{0xb1, 0x30},
	{0xae, 0x0d},  //bit0=1,high 8bit
	{0x88, 0x5b},  //BLC_ABL
	{0x89, 0x7c},  //bit6=1 trigger en
	{0xb4, 0x05},  //mean trigger 5
	{0x8c, 0x40},  //BLC_BLUE_SUBOFFSET_8lsb
	{0x8e, 0x40},  //BLC_RED_SUBOFFSET_8lsb
	{0x90, 0x40},  //BLC_GR_SUBOFFSET_8lsb
	{0x92, 0x40},  //BLC_GB_SUBOFFSET_8lsb
	{0x9b, 0x46},  //digtal gain
	{0xac, 0x40},  //blc random noise rpc_th 4x
	{0xfd, 0x00},
	{0x5a, 0x15},
	{0x74, 0x01},  //PD_MIPI:turn on mipi phy

	{0xfd, 0x00},  //crop to 1280x720
	{0x4f, 0x05},
	{0x50, 0x00},  //mipi hszie low 8bit
	{0x51, 0x02},
	{0x52, 0xd0},  //mipi vsize low 8bit
	{0x55, 0x2b},  //raw10 output

	{0xfd, 0x01},
	{0x03, 0xc0},  //window hstart low 8bit
	{0x04, 0x01},
	{0x05, 0x00},  //window vstart low 8bit
	{0x06, 0x02},
	{0x07, 0x80},  //window hsize low 8bit
	{0x08, 0x02},
	{0x09, 0xd0},  //window vsize low 8bit
	{0xfe, 0x02},
	{0xfb, 0x01},

	{0xfd, 0x03},
//	{0x81, 0x01},  //Color Bar
	{0xc2, 0x01},  //MIPI_EN
	{0xfd, 0x01},
};
#endif

#if SENSOR_1280x720_15FPS
static struct regval_list sensor_720p_15fps_regs[] = {
	/* 1280x720 15fps 1 lane MIPI 600Mbps/lane */
	{0xfc, 0x01},  //soft reset
	{0xfd, 0x00},
	{0xfd, 0x00},
	{0x24, 0x02},  //pll_mc
	{0x25, 0x06},  //pll_nc,dpll clk 72M
	{0x29, 0x01},
	{0x2a, 0xAF},  //mpll_nc, mpll clk 600M
	{0x2b, 0x00},
	{0x1e, 0x17},  //vlow 0.53v
	{0x33, 0x07},  //ipx 2.84u
	{0x35, 0x07},
	{0x4a, 0x0c},  //ncp -1.4v
	{0x3a, 0x05},  //icomp1 4.25u
	{0x3b, 0x02},  //icomp2 1.18u
	{0x3e, 0x00},
	{0x46, 0x01},
	{0x6d, 0x03},
	{0xfd, 0x01},
	{0x0e, 0x04},
	{0x0f, 0xb0},  //exp
	{0x12, 0x03},  //mirror and flip
	{0x14, 0x05},
	{0x15, 0xd4},
	{0x18, 0x00},  //un fixed-fps
	{0x22, 0x20},  //analog gain
	{0x23, 0x02},  //adc_range 0.595v
	{0x17, 0x2c},  //pd reset row address time
	{0x19, 0x20},  //dac_d0 1024
	{0x1b, 0x06},  //rst_num1 96
	{0x1c, 0x04},  //rst_num2 64
	{0x20, 0x03},
	{0x30, 0x01},  //p0
	{0x33, 0x01},  //p3
	{0x31, 0x0a},  //p1
	{0x32, 0x09},  //p2
	{0x38, 0x01},
	{0x39, 0x01},  //p9
	{0x3a, 0x01},  //p10
	{0x3b, 0x01},
	{0x4f, 0x04},  //p24
	{0x4e, 0x05},  //p23
	{0x50, 0x01},  //p25
	{0x35, 0x0c},  //p5
	{0x45, 0x2a},  //sc1,p20_1
	{0x46, 0x2a},  //p20_2
	{0x47, 0x2a},  //p20_3
	{0x48, 0x2a},  //p20_4
	{0x4a, 0x2c},  //sc2,p22_1
	{0x4b, 0x2c},  //p22_2
	{0x4c, 0x2c},  //p22_3
	{0x4d, 0x2c},  //p22_4
	{0x56, 0x3a},  //p31, 1st d0
	{0x57, 0x0a},  //p32, 1st d1
	{0x58, 0x24},  //col_en1
	{0x59, 0x20},  //p34 2nd d0
	{0x5a, 0x0a},  //p34 2nd d1
	{0x5b, 0xff},  //col_en2
	{0x37, 0x0a},  //p7, tx
	{0x42, 0x0e},  //p17, psw
	{0x68, 0x90},
	{0x69, 0xcd},  //blk en, no sig_clamp
	{0x6a, 0x8f},
	{0x7c, 0x0a},
	{0x7d, 0x09},  //0a
	{0x7e, 0x09},  //0a
	{0x7f, 0x08},
	{0x83, 0x14},
	{0x84, 0x14},
	{0x86, 0x14},
	{0x87, 0x07},  //vbl2_4
	{0x88, 0x0f},
	{0x94, 0x02},  //evsync del frame
	{0x98, 0xd1},  //del bad frame
	{0xfe, 0x02},
	{0xfd, 0x03},  //RegPage
	{0x97, 0x78},
	{0x98, 0x78},
	{0x99, 0x78},
	{0x9a, 0x78},
	{0xa1, 0x40},
	{0xb1, 0x30},
	{0xae, 0x0d},  //bit0=1,high 8bit
	{0x88, 0x5b},  //BLC_ABL
	{0x89, 0x7c},  //bit6=1 trigger en
	{0xb4, 0x05},  //mean trigger 5
	{0x8c, 0x40},  //BLC_BLUE_SUBOFFSET_8lsb
	{0x8e, 0x40},  //BLC_RED_SUBOFFSET_8lsb
	{0x90, 0x40},  //BLC_GR_SUBOFFSET_8lsb
	{0x92, 0x40},  //BLC_GB_SUBOFFSET_8lsb
	{0x9b, 0x46},  //digtal gain
	{0xac, 0x40},  //blc random noise rpc_th 4x
	{0xfd, 0x00},
	{0x5a, 0x15},
	{0x74, 0x01},  //PD_MIPI:turn on mipi phy

	{0xfd, 0x00},  //crop to 1280x720
	{0x4f, 0x05},
	{0x50, 0x00},  //mipi hszie low 8bit
	{0x51, 0x02},
	{0x52, 0xd0},  //mipi vsize low 8bit
	{0x55, 0x2b},  //raw10 output

	{0xfd, 0x01},
	{0x03, 0xc0},  //window hstart low 8bit
	{0x04, 0x01},
	{0x05, 0x00},  //window vstart low 8bit
	{0x06, 0x02},
	{0x07, 0x80},  //window hsize low 8bit
	{0x08, 0x02},
	{0x09, 0xd0},  //window vsize low 8bit
	{0xfe, 0x02},
	{0xfb, 0x01},

	{0xfd, 0x03},
//	{0x81, 0x01},  //Color Bar
	{0xc2, 0x01},  //MIPI_EN
	{0xfd, 0x01},
};
#endif

#if SENSOR_640x480_15FPS
static struct regval_list sensor_vga_30fps_regs[] = {
	/* 640x480 30fps 1 lane MIPI 420Mbps/lane */
	{0xfc, 0x01},  //soft reset
	{0xfd, 0x00},
	{0xfd, 0x00},
	{0x24, 0x02},  //pll_mc
	{0x25, 0x06},  //pll_nc,dpll clk 72M
	{0x28, 0x00},
	{0x29, 0x03},
	{0x2a, 0xb4},  //mpll_nc, mpll clk 330M
	{0x1e, 0x17},  //vlow 0.53v
	{0x33, 0x07},  //ipx 2.84u
	{0x35, 0x07},  //pcp off
	{0x4a, 0x0c},  //ncp -1.4v
	{0x3a, 0x05},  //icomp1 4.25u
	{0x3b, 0x02},  //icomp2 1.18u
	{0x3e, 0x00},
	{0x46, 0x01},
	{0xfd, 0x01},
	{0x0e, 0x04},
	{0x0f, 0xb0},  //exp
	{0x12, 0x03},  //mirror and flip
	{0x14, 0x0a},
	{0x15, 0x8e},
	{0x18, 0x00},  //un fixed-fps
	{0x22, 0x10},  //analog gain
	{0x23, 0x02},  //adc_range 0.595v
	{0x17, 0x2c},  //pd reset row address time
	{0x19, 0x20},  //dac_d0 1024
	{0x1b, 0x06},  //rst_num1 96
	{0x1c, 0x04},  //rst_num2 64
	{0x20, 0x03},
	{0x30, 0x01},  //p0
	{0x33, 0x01},  //p3
	{0x31, 0x0a},  //p1
	{0x32, 0x09},  //p2
	{0x38, 0x01},
	{0x39, 0x01},  //p9
	{0x3a, 0x01},  //p10
	{0x3b, 0x01},
	{0x4f, 0x04},  //p24
	{0x4e, 0x05},  //p23
	{0x50, 0x01},  //p25
	{0x35, 0x0c},  //p5
	{0x45, 0x2a},  //sc1,p20_1
	{0x46, 0x2a},  //p20_2
	{0x47, 0x2a},  //p20_3
	{0x48, 0x2a},  //p20_4
	{0x4a, 0x2c},  //sc2,p22_1
	{0x4b, 0x2c},  //p22_2
	{0x4c, 0x2c},  //p22_3
	{0x4d, 0x2c},  //p22_4
	{0x56, 0x3a},  //p31, 1st d0
	{0x57, 0x0a},  //p32, 1st d1
	{0x58, 0x24},  //col_en1
	{0x59, 0x20},  //p34 2nd d0
	{0x5a, 0x0a},  //p34 2nd d1
	{0x5b, 0xff},  //col_en2
	{0x37, 0x0a},  //p7, tx
	{0x42, 0x0e},  //p17, psw
	{0x68, 0x90},
	{0x69, 0xcd},  //blk en, no sig_clamp
	{0x7c, 0x08},
	{0x7d, 0x08},
	{0x7e, 0x08},
	{0x7f, 0x08},  //vbl1_4
	{0x83, 0x14},
	{0x84, 0x14},
	{0x86, 0x14},
	{0x87, 0x07},  //vbl2_4
	{0x88, 0x0f},
	{0x94, 0x02},  //evsync del frame
	{0x98, 0xd1},  //del bad frame
	{0xfe, 0x02},
	{0xfd, 0x03},  //RegPage
	{0x97, 0x6c},
	{0x98, 0x60},
	{0x99, 0x60},
	{0x9a, 0x6c},
	{0xae, 0x0d},  //bit0=1,high 8bit
	{0x88, 0x49},  //BLC_ABL
	{0x89, 0x7c},  //bit6=1 trigger en
	{0xb4, 0x05},  //mean trigger 5
	{0xbd, 0x0d},  //blc_rpc_coe
	{0x8c, 0x40},  //BLC_BLUE_SUBOFFSET_8lsb
	{0x8e, 0x40},  //BLC_RED_SUBOFFSET_8lsb
	{0x90, 0x40},  //BLC_GR_SUBOFFSET_8lsb
	{0x92, 0x40},  // BLC_GB_SUBOFFSET_8lsb
	{0x9b, 0x49},  //digtal gain
	{0xac, 0x40},  //blc random noise rpc_th 4x
	{0xfd, 0x00},
	{0x5a, 0x15},
	{0x74, 0x01},  //PD_MIPI:turn on mipi phy

	{0xfd, 0x00},  //binning 640x480
	{0x4f, 0x02},  //mipi size
	{0x50, 0x80},
	{0x51, 0x01},
	{0x52, 0xe0},
	{0x55, 0x2b},  //raw10 output

	{0xfd, 0x01},
	{0x03, 0xc0},  //window hstart low 8bit
	{0x04, 0x01},
	{0x05, 0x88},  //window vstart low 8bit
	{0x06, 0x02},
	{0x06, 0x02},
	{0x07, 0x80},  //window hsize low 8bit

	{0x08, 0x01},
	{0x09, 0xe0},  //window vsize low 8bit

//	{0x08, 0x03},
//	{0x09, 0xc0},  //window vsize low 8bit
//	{0x6c, 0x09},  //binning22 en

	{0xfe, 0x02},
	{0xfb, 0x01},

	{0xfd, 0x03},
//	{0x81, 0x01},  //Color Bar
	{0xc2, 0x01},  //MIPI_EN
	{0xfd, 0x01},
};
#endif



static int ov02b10_sensor_vts;
static int sensor_s_exp(int id, unsigned int exp_val)
{
	if (exp_val < 16) {
		exp_val = 16;
	}
	else if (exp_val > (ov02b10_sensor_vts-7)) {
		exp_val = (ov02b10_sensor_vts-7);
	}

	sensor_write(id, 0xfd, 0x01);
	sensor_write(id, 0x0f, (exp_val & 0xFF));
	sensor_write(id, 0x0e, (exp_val >> 8) & 0xFF);
	sensor_write(id, 0xfe, 0x02);
	sensor_dbg("sensor_s_exp:0x%04x\n", exp_val);

	return 0;
}

static int sensor_s_gain(int id, int gain_val)
{
    if (gain_val < 16) {
        gain_val = 16;
    } else if (gain_val > 0xff) {
        gain_val = 0xff;
    }

	sensor_write(id, 0xfd, 0x01);
	sensor_write(id, 0x22, gain_val);
	sensor_write(id, 0xfe, 0x02);
	sensor_dbg("sensor_s_exp:0x%02x\n", gain_val);

	return 0;
}

static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	sensor_s_exp(id, exp_val);
	sensor_s_gain(id, gain_val);

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
		hal_usleep(5000);

		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);

		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		hal_usleep(4000);

		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(5000);

		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");

		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		vin_set_mclk(id, 0);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);

		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_set_ir(int id, int status)
{
#if 0
	vin_gpio_set_status(id, IR_LED, 1);
	switch (status) {
	case IR_DAY:
		vin_gpio_write(id, IR_LED, CSI_GPIO_LOW);
		break;
	case IR_NIGHT:
		vin_gpio_write(id, IR_LED, CSI_GPIO_HIGH);
		break;
	default:
		return -1;
	}
#endif	
	return 0;
}

static int sensor_detect(int id)
{
	data_type rdval;
	unsigned int sensor_id;

	sensor_read(id, 0x03, &rdval);
	sensor_id = rdval;
	sensor_dbg("Sensor%d ID = 0x%X\n", id, sensor_id);
	if (sensor_id != V4L2_IDENT_SENSOR) {
		sensor_err("OV02B10 %s error", __func__);
		return -ENODEV;
	}

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

#if SENSOR_1600x1200_30FPS
	/* 1600x1200 30fps */
	{
		//.mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,//MEDIA_BUS_FMT_SBGGR10_1X10,
		.mbus_code  = MEDIA_BUS_FMT_SBGGR10_1X10,//MEDIA_BUS_FMT_SBGGR10_1X10,		
		.width      = 1600,
		.height     = 1200,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 448,//zcy md 463 to 448,2688
		.vts        = 1221,
		//.pclk       = 66*1000*1000,//md 72*1000*1000 to 16501320=16*1000*1000
		//.mipi_bps	= 660*1000*1000,
		.pclk       = 33*1000*1000,
		.mipi_bps	= 330*1000*1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 16,
		.intg_max   = (1228-7),
		.gain_min   = 16,
		.gain_max   = 16<<4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs		= sensor_1600x1200_30fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_1600x1200_30fps_regs),
	},
#endif

#if SENSOR_1280x720_30FPS
	/* 720p 30fps */
	{
		.mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,//MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 448,//zcy md 463 to 448,2688
		.vts        = 1228,
		.pclk       = 16*1000*1000,//md 72*1000*1000 to 16501320=16*1000*1000
		.mipi_bps	= 600*1000*1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 16,
		.intg_max   = (1228-7),
		.gain_min   = 16,
		.gain_max   = 16<<4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs		= sensor_720p_30fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p_30fps_regs),
	},
#endif
#if SENSOR_1280x720_15FPS
	/* 720p 15fps */
	{
		.mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,//MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 463,
		.vts        = 2232,
		.pclk       = 72*1000*1000,
		.mipi_bps	= 600*1000*1000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 16,
		.intg_max   = (2232-7),
		.gain_min   = 16,
		.gain_max   = 16<<4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs		= sensor_720p_15fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p_15fps_regs),
	},
#endif
#if SENSOR_640x480_15FPS
	/* VGA 30fps */
	{
		.mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,//MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 640,
		.height     = 480,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 448,
		.vts        = 1569,
		.pclk       = 72*1000*1000,
		.mipi_bps	= 600*1000*1000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.intg_min   = 16,
		.intg_max   = (1569-7),
		.gain_min   = 16,
		.gain_max   = 16<<4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs		= sensor_vga_30fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_vga_30fps_regs),
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
#if SENSOR_1280x720_30FPS
	/* 720p 30fps */
	{
		.mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,//MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 463,
		.vts        = 1228,
		.pclk       = 72*1000*1000,
		.mipi_bps	= 600*1000*1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 16,
		.intg_max   = (1228-7),
		.gain_min   = 16,
		.gain_max   = 16<<4,
		.switch_regs		= sensor_720p_30fps_regs,
		.switch_regs_size	= ARRAY_SIZE(sensor_720p_30fps_regs),
	},
#endif
};
#endif

static struct sensor_format_struct *sensor_get_switch_format(int id, int isp_id, int vinc_id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);
	struct sensor_format_struct *sensor_format = NULL;
	int wdr_on = isp_get_cfg[ispid].sensor_wdr_on;
	int fps = isp_get_cfg[ispid].sensor_get_fps;
	int i;

	if (current_switch_win[id])
		return current_switch_win[id];

	for (i = 0; i < ARRAY_SIZE(switch_sensor_formats); i++) {
		if (switch_sensor_formats[i].wdr_mode == wdr_on) {
			if (switch_sensor_formats[i].fps_fixed == fps) {
				sensor_format = &switch_sensor_formats[i];
				sensor_print("switch fine wdr is %d, fine fps is %d\n", wdr_on, fps);
				goto done;
			}
		}
	}

	if (sensor_format == NULL) {
		for (i = 0; i < ARRAY_SIZE(switch_sensor_formats); i++) {
			if (switch_sensor_formats[i].wdr_mode == wdr_on) {
				sensor_format = &switch_sensor_formats[i];
				isp_get_cfg[ispid].sensor_get_fps = sensor_format->fps_fixed;
				sensor_print("switch fine wdr is %d, use fps is %d\n", wdr_on, sensor_format->fps_fixed);
				goto done;
			}
		}
	}

	if (sensor_format == NULL) {
		sensor_format = &switch_sensor_formats[0];
		isp_get_cfg[ispid].sensor_wdr_on = sensor_format->wdr_mode;
		isp_get_cfg[ispid].sensor_get_fps = sensor_format->fps_fixed;
		sensor_print("switch use wdr is %d, use fps is %d\n", sensor_format->wdr_mode, sensor_format->fps_fixed);
	}

done:
	current_switch_win[id] = sensor_format;
	return sensor_format;
#else
	return NULL;
#endif
}

static int sensor_g_mbus_config(int id, struct v4l2_mbus_config *cfg, struct mbus_framefmt_res *res)
{

	cfg->type  = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	res->res_time_hs = 0x28;//zcy md 0x28 to 0x18

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

	if (current_win[id]->regs)
		ret = sensor_write_array(id, current_win[id]->regs, current_win[id]->regs_size);
	if (ret < 0)
		return ret;

	ov02b10_sensor_vts = current_win[id]->vts;
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_FAST_CONVERGENCE
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, 0x4c5);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 0x20);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, 0x4c5); //0x4b0
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 0x40); //0x20
	}
	sensor_s_exp_gain(id, &exp_gain);
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC

#endif

#else  //CONFIG_ISP_HARD_LIGHTADC

#endif
	sensor_dbg("ov02b10_sensor_vts = %d\n", ov02b10_sensor_vts);

	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

//	sensor_dbg("%s id=%d, on=%d \n", __func__, id, enable);

	if (!enable)
		return 0;

	return sensor_reg_init(id, isp_id);
}

static int sensor_s_switch(int id)
{
	sensor_dbg("sensor_s_switch \n");

#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	struct sensor_exp_gain exp_gain;
	int ret = -1;

	ov02b10_sensor_vts = current_switch_win[id]->vts;
	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_err("cannot find 480p120fps to 720p%dfps reg\n", current_switch_win[id]->fps_fixed);
	if (ret < 0)
		return ret;
#endif
	return 0;
}

static struct sensor_driver sensor_drv = {
	.addr_width = CCI_BITS_8,
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

	return 0;
}

struct sensor_fuc_core ov02b10_core  = {
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
