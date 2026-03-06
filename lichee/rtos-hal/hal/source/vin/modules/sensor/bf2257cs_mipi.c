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
#define V4L2_IDENT_SENSOR  0x2253

/*
 * Our nominal (default) frame rate.
 */
#define ID_REG_HIGH		0xfc
#define ID_REG_LOW		0xfd
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)
#define SENSOR_FRAME_RATE 30

/*
 * The bf2257cs i2c address
 */
#define I2C_ADDR 0xdc//0xdc

#define SENSOR_NUM 0x2
#define SENSOR_NAME "bf2257cs_mipi"
#define SENSOR_NAME_2 "bf2257cs_mipi_2"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];
static struct sensor_format_struct *current_switch_win[2];

#define SENSOR_30FPS 1
#define SENSOR_25FPS 0
#define SENSOR_20FPS 0
#define SENSOR_15FPS 0

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

	

};

#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC//FULL_SIZE
#if SENSOR_30FPS

static struct regval_list sensor_1600x1200_30_regs[] = {
	//BF2257_RAW10_MIPI_2M_XCLK24M_PCLK67.2M_max30fps_V3.1.1_20221018_phone
	//XCLK:24M;  MIPICLK:672M  PCLK(RAW10):67.2M
	//行长：1792  帧长：1250
	//Max fps:30fps
	//1600*1200
	
	{0xf2,0x01},
	{0x00,0x01},
	{0x02,0xb7},
	{0x1e,0x04},
	{0x24,0x60},
	{0xe0,0x00},
	{0xe1,0x01},//延时2ns
	{0xe2,0x08},
	{0xe5,0xe3},
	{0xe6,0x60},//ADC range
	{0xe7,0x33},
	{0xe8,0x12},
	{0xe9,0x89},
	{0xea,0x87},
	{0xeb,0x80},//高频写0x80，低频写0x84
	{0xec,0x91},
	{0xed,0x60},
	
	//MIPICLK:672M
	{0xe3,0x78},
	{0xe4,0xe0},
	
	//行长，帧长1792*1250
	{0x06,0x10},
	{0x07,0x00},
	{0x0b,0x80},
	{0x0c,0x03},
	
	//Black target
	{0x59,0x40},
	{0x5a,0x40},
	{0x5b,0x40},
	{0x5c,0x40},
	
	//MIPI Setting
	{0x70,0x08},
	{0x71,0x07},
	{0x72,0x12},
	{0x73,0x09},
	{0x74,0x08},
	{0x75,0x06},
	{0x76,0x20},
	{0x77,0x02},
	{0x78,0x10},
	{0x79,0x09},
	{0x7a,0x00},
	{0x7b,0x00},
	{0x7c,0x00},
	{0x7d,0x0f},
	
	//Window 1600*1200：从中心取
	{0xca,0x60},
	{0xcb,0x40},
	{0xcc,0x04},
	{0xcd,0x44},
	{0xce,0x04},
	{0xcf,0xb4},

	{0x6a,0x0f},//Global gain
	{0x6b,0x04},
	{0x6c,0xe1},//{0x6b,0x6c}:int_time
	{0x6d,0x0f},
	
	//Stream on 
	{0x01,0x4b},
	{0xf3,0x00},
	{0xd0,0x00},
	{0x01,0x43},
};
#endif

#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
static struct regval_list sensor_480p120_regs[] = {
	
};
#if SENSOR_30FPS
static struct regval_list sensor_480p120fps_to_1600x1200_30fps[] = {
	
};
#endif

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
	sensor_write(id, 0x6b, (tmp_exp_val >> 8) & 0xFF);
	sensor_write(id, 0x6c, (tmp_exp_val & 0xFF));

	return 0;
}

static int setSensorGain(int id, int gain)
{
#define ISP_BASE_GAIN		16
#define SENSOR_BASE_GAIN	0X400
#define SENSOR_MAX_GAIN		10856
	int i;
	data_type temperature_value;
	uint32_t temp_gain = 0;
	int32_t gain_index;
	int isp_gain = gain;
	int sensor_gain = 0;
	
	sensor_gain = isp_gain < ISP_BASE_GAIN ? ISP_BASE_GAIN : isp_gain;
	sensor_gain = sensor_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;
	
	if (SENSOR_MAX_GAIN < sensor_gain)
		sensor_gain = SENSOR_MAX_GAIN;
	
	uint16_t BF2257CS_AGC_Param[][2] = {
		 {1024  ,15},
		 {1088  ,16},
		 {1150  ,17},
		 {1214  ,18},
		 {1277  ,19},
		 {1342  ,20},
		 {1403  ,21},
		 {1468  ,22},
		 {1527  ,23},
		 {1594  ,24},
		 {1652  ,25},
		 {1716  ,26},
		 {1775  ,27},
		 {1839  ,28},
		 {1897  ,29},
		 {1960  ,30},
		 {2023  ,31},
		 {2145  ,32},
		 {2273  ,33},
		 {2395  ,34},
		 {2519  ,35},
		 {2641  ,36},
		 {2765  ,37},
		 {2885  ,38},
		 {3006  ,39},
		 {3131  ,40},
		 {3249  ,41},
		 {3373  ,42},
		 {3493  ,43},
		 {3610  ,44},
		 {3730  ,45},
		 {3846  ,46},
		 {3962  ,47},
		 {4206  ,48},
		 {4441  ,49},
		 {4678  ,50},
		 {4906  ,51},
		 {5146  ,52},
		 {5371  ,53},
		 {5601  ,54},
		 {5831  ,55},
		 {6046  ,56},
		 {6259  ,57},
		 {6493  ,58},
		 {6713  ,59},
		 {6928  ,60},
		 {7149  ,61},
		 {7359  ,62},
		 {7564  ,63},
		 {7789  ,64},
		 {8000  ,65},
		 {8216  ,66},
		 {8439  ,67},
		 {8648  ,68},
		 {8859  ,69},
		 {9065  ,70},
		 {9280  ,71},
		 {9482  ,72},
		 {9665  ,73},
		 {9891  ,74},
		 {10073 ,75},
		 {10273 ,76},
		 {10474 ,77},
		 {10663 ,78},
		 {10856 ,79}
	};

	if (sensor_gain < SENSOR_BASE_GAIN)
		sensor_gain = SENSOR_BASE_GAIN;
	if (sensor_gain > SENSOR_MAX_GAIN)
		sensor_gain = SENSOR_MAX_GAIN;		
	
	uint32_t total_cnt = (sizeof(BF2257CS_AGC_Param)/sizeof(BF2257CS_AGC_Param[0]));
	for (gain_index = 0; gain_index < total_cnt -1; gain_index++)
	{
		if((sensor_gain >= BF2257CS_AGC_Param[gain_index][0])&&(sensor_gain <= BF2257CS_AGC_Param[gain_index+1][0]))
		{
			if((sensor_gain-BF2257CS_AGC_Param[gain_index][0]) <= (BF2257CS_AGC_Param[gain_index+1][0]-sensor_gain))
				temp_gain = BF2257CS_AGC_Param[gain_index][1];
			else
				temp_gain = BF2257CS_AGC_Param[gain_index+1][1];
		 break;
		}
	}

	sensor_dbg("BF2257CS_AGC_Param[gain_index][1] = 0x%x, temp_gain = 0x%x, sensor_gain = 0x%x, total_cnt = %d\n",
		BF2257CS_AGC_Param[gain_index][1], temp_gain, sensor_gain, total_cnt);
		
	sensor_write(id, 0x6a, temp_gain);

	return 0;
}

static int sensor_s_gain(int id, int gain_val)
{
	sensor_dbg("gain_val:%d\n", gain_val);
	setSensorGain(id, gain_val);

	return 0;
}

static int bf2257cs_sensor_vts;
static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	int shutter = 0, frame_length = 0;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < (1 * 16)) {
		gain_val = 16;
	}

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	shutter = exp_val >> 4;
	if (shutter > bf2257cs_sensor_vts - 16)
		frame_length = shutter + 16;
	else
		frame_length = bf2257cs_sensor_vts;
	sensor_dbg("frame_length = %d\n", frame_length);
	//sensor_write(sd, 0x07, (frame_length-1236) & 0xff);
	//sensor_write(sd, 0x08, (frame_length-1236) >> 8);

	sensor_s_exp(id, exp_val);
	sensor_s_gain(id, gain_val);

	sensor_print("gain_val:%d, exp_val:%d\n", gain_val, exp_val);

	return 0;
}

#if 0
static int sensor_flip_status;
static int sensor_s_vflip(int id, int enable)
{
	data_type get_value;
	data_type set_value;

	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(id, 0x00, &get_value);
	sensor_dbg("ready to vflip, regs_data = 0x%x\n", get_value);

	if (enable) {
		set_value = get_value | 0x04;
		sensor_flip_status |= 0x04;
	} else {
		set_value = get_value & 0xFB;
		sensor_flip_status &= 0xFB;
	}
	sensor_write(id, 0x00, set_value);
	usleep_range(80000, 100000);
	sensor_read(id, 0x00, &get_value);
	sensor_dbg("after vflip, regs_data = 0x%x, sensor_flip_status = %d\n",
				get_value, sensor_flip_status);

	return 0;
}

static int sensor_s_hflip(int id, int enable)
{
	data_type get_value;
	data_type set_value;

	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(id, 0x00, &get_value);
	sensor_dbg("ready to hflip, regs_data = 0x%x\n", get_value);

	if (enable) {
		set_value = get_value | 0x08;
		sensor_flip_status |= 0x08;
	} else {
		set_value = get_value & 0xF7;
		sensor_flip_status &= 0xF7;
	}
	sensor_write(id, 0x00, set_value);
	usleep_range(80000, 100000);
	sensor_read(id, 0x00, &get_value);
	sensor_dbg("after hflip, regs_data = 0x%x, sensor_flip_status = %d\n",
				get_value, sensor_flip_status);

	return 0;
}

static int sensor_get_fmt_mbus_core(struct v4l2_subdev *sd, int *code)
{
//	struct sensor_info *info = to_state(sd);
//	data_type get_value = 0, check_value = 0;

//	sensor_read(sd, 0x17, &get_value);
//	check_value = get_value & 0x03;
//	check_value = sensor_flip_status & 0x3;
//	sensor_dbg("0x17 = 0x%x, check_value = 0x%x\n", get_value, check_value);

//	switch (check_value) {
//	case 0x00:
//		sensor_dbg("RGGB\n");
//		*code = MEDIA_BUS_FMT_SRGGB10_1X10;
//		break;
//	case 0x01:
//		sensor_dbg("GRBG\n");
//		*code = MEDIA_BUS_FMT_SGRBG10_1X10;
//		break;
//	case 0x02:
//		sensor_dbg("GBRG\n");
//		*code = MEDIA_BUS_FMT_SGBRG10_1X10;
//		break;
//	case 0x03:
//		sensor_dbg("BGGR\n");
//		*code = MEDIA_BUS_FMT_SBGGR10_1X10;
//		break;
//	default:
//		 *code = info->fmt->mbus_code;
//	}
	*code = MEDIA_BUS_FMT_SRGGB10_1X10; // bf2257cs support change the rgb format by itself

	return 0;
}
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
		sensor_dbg("PWR_ON!\n");
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_set_status(id, PWDN, 1);
		//vin_gpio_set_status(id, RESET, 1);
		//vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		//vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(1000);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		hal_usleep(1000);
		//vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		//hal_usleep(1000);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!do nothing\n");
		vin_set_mclk(id, 0);
		hal_usleep(1000);
		vin_gpio_set_status(id, PWDN, 1);
		//vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		//vin_gpio_write(id, RESET, CSI_GPIO_LOW);
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
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, /*.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
		.width		= 1600,
		.height 	= 1200,//1080,
		.hoffset	= 0,
		.voffset	= 0,
		.hts		= 1792,
		.vts		= 1250,
		.pclk       = 67.2 * 1000 * 1000,
		.mipi_bps   = 672 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1250 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_1600x1200_30_regs,
		.regs_size  = ARRAY_SIZE(sensor_1600x1200_30_regs),
	}
#endif
#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 640,
		.height     = 480,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1696,
		.vts        = 525,
		.pclk       = 106848000,
		.mipi_bps   = 648 * 1000 * 1000,
		.fps_fixed  = 120,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (525 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_480p120_regs,
		.regs_size  = ARRAY_SIZE(sensor_480p120_regs),
	}
#endif
};

static struct sensor_format_struct *sensor_get_format(int id, int isp_id)
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
		.width		= 1600,
		.height 	= 1200,//1080,
		.hoffset	= 0,
		.voffset	= 0,
		.hts		= 1792,
		.vts		= 1250,
		.pclk       = 67.2 * 1000 * 1000,
		.mipi_bps   = 672 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1250 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.switch_regs	   = sensor_480p120fps_to_1600x1200_30fps,
		.switch_regs_size  = ARRAY_SIZE(sensor_480p120fps_to_1600x1200_30fps),
	}
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
	//struct sensor_info *info = to_state(sd);

	cfg->type  = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	res->res_time_hs = 0x28;

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

	bf2257cs_sensor_vts = current_win[id]->vts;
//#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_FAST_CONVERGENCE
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, 1125 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 110 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, 1125 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 110 << 4);
	}
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	temperature_ctrl = 0;
#endif
	sensor_s_exp_gain(id, &exp_gain);
	sensor_write(id, 0x3e, 0x91);
//#else  //CONFIG_ISP_HARD_LIGHTADC

//#endif
	//sensor_flip_status = 0x0;
	//sensor_dbg("bf2257cs_sensor_vts = %d\n", bf2257cs_sensor_vts);

	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

	sensor_dbg("%s on = %d, 2560*1440 fps: 15\n", __func__, enable);

	if (!enable)
		return 0;

	return sensor_reg_init(id, isp_id);
}

static int sensor_s_switch(int id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	struct sensor_exp_gain exp_gain;
	int ret = -1;
	temperature_ctrl = 1;
	bf2257cs_sensor_vts = current_switch_win[id]->vts;
	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_err("cannot find 480p120fps to 1600x1200_%dfps reg\n", current_switch_win[id]->fps_fixed);
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

struct sensor_fuc_core bf2257cs_core  = {
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
