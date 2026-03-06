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
#include <sunxi_hal_pwm.h>

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR  0x0469
/*
 * Our nominal (default) frame rate.
 */
#define ID_REG_HIGH		0xf0
#define ID_REG_LOW		0xf1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)
#define SENSOR_FRAME_RATE 30

/*
 * The GC0406 i2c address
 */
#define I2C_ADDR 0x42

#define SENSOR_NUM 0x2
#define SENSOR_NAME "gc0406_mipi"
#define SENSOR_NAME_2 "gc0406_mipi_2"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];
static struct sensor_format_struct *current_switch_win[2];
static int temperature_ctrl = 1;
static int gc0406_sensor_vts;
static struct pwm_config *config;
/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_800x480p30_regs[] = {
/*system*/
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xf7, 0x01},
	{0xf8, 0x05},
	{0xf9, 0x0f},//[0] not_use_pll
	{0xfa, 0x00},
	{0xfc, 0x0f},//[0] apwd
	{0xfe, 0x00},
/*analog & CISCTL*/		
	{0xfe, 0x00},
	{0x03, 0x01},
	{0x04, 0xda},
	{0x05, 0x02},
	{0x06, 0x97},//HB
	{0x07, 0x00},
	{0x08, 0x14},
	{0x0a, 0x00},//row_start
	{0x0c, 0x04},
	{0x0d, 0x01},
	{0x0e, 0xe8},
	{0x0f, 0x03},
	{0x10, 0x30},//win_width
	{0x17, 0x43},//Don't Change Here!!! 0x42
	{0x18, 0x12},
	{0x19, 0x0b},
	{0x1a, 0x1a},
	{0x1e, 0x50},
	{0x1f, 0x80},
	{0x24, 0xc8},//c8
	{0x25, 0xe2},
	{0x27, 0xaf},
	{0x28, 0x24},
	{0x29, 0x14},
	{0x2f, 0x14},
	{0x3f, 0x18},//1b tx en
	{0x40, 0x26},	
	{0x72, 0xab},//20151012 0x9b
	{0x73, 0x4a},//20151012 0xea
	{0x74, 0x40},
	{0x75, 0x10},
	{0x76, 0xa2},//20151012 0x92
	{0x7a, 0x4e},
	{0xc1, 0x12},
	{0xc2, 0x0c},
	{0xcf, 0x48},
	{0xdc, 0x75},
	{0xeb, 0x78},
/*ISP*/
	{0xfe, 0x00},
	{0x90, 0x01},
	{0x92, 0x00},//Don't Change Here!!!
	{0x94, 0x00},//Don't Change Here!!!
	{0x95, 0x01},
	{0x96, 0xe0},
	{0x97, 0x03},
	{0x98, 0x20},
/*gain*/
	{0xb0, 0x50},//global_gain	
	{0xb1, 0x01},
	{0xb2, 0x00},	
	{0xb3, 0x40},
	{0xb4, 0x40},
	{0xb5, 0x40},
	{0xb6, 0x00},
	{0x30, 0x00},	
	{0x31, 0x01},
	{0x32, 0x02},
	{0x33, 0x03},
	{0x34, 0x07},
	{0x35, 0x0b},
	{0x36, 0x0f},
/*BLK*/
	{0xfe, 0x00},
	{0x40, 0x26}, 
	{0x4f, 0xc3},
	{0x5e, 0x00},
	{0x5f, 0x80},
/*dark sun*/
	{0xfe, 0x00},
	{0xe0, 0x9e},  
	{0xe1, 0x80},
	{0xe4, 0x0f},
	{0xe5, 0xff},
/*mipi*/
	{0xfe, 0x03},
	{0x10, 0x00},	
	{0x01, 0x03},
	{0x02, 0x22},
	{0x03, 0x96},
	{0x04, 0x01},
	{0x05, 0x00},
	{0x06, 0x80},
	//{0x10, 0x90},
	{0x11, 0x2b},
	{0x12, 0xe8},
	{0x13, 0x03},
	{0x15, 0x00},
	{0x21, 0x10},
	{0x22, 0x00},
	{0x23, 0x30},
	{0x24, 0x02},
	{0x25, 0x12},
	{0x26, 0x03},
	{0x29, 0x01},
	{0x2a, 0x0a},
	{0x2b, 0x03},	
	{0xfe, 0x00},
	{0xf9, 0x0e},//[0] not_use_pll
	{0xfc, 0x0e},//[0] apwd
	{0xfe, 0x00},
};
//#ifdef CONFIG_ISP_FAST_CONVERGENCE
static struct regval_list sensor_320x240p90_regs[] = {
/*system*/
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xf7, 0x01},
	{0xf8, 0x05},
	{0xf9, 0x0f},//[0] not_use_pll
	{0xfa, 0x00},
	{0xfc, 0x0f},//[0] apwd
	{0xfe, 0x00},
/*analog & CISCTL*/		
	{0xfe, 0x00},
	{0x03, 0x01},
	{0x04, 0xda},
	{0x05, 0x02},
	{0x06, 0x97},//HB
	{0x07, 0x00},
	{0x08, 0x14},
	{0x0a, 0x00},//row_start
	{0x0c, 0x04},

    {0x0d, 0x00},
	{0x0e, 0xf8},
	
	{0x0f, 0x01},
	{0x10, 0x50},//win_width
	
	{0x17, 0x42},//Don't Change Here!!!
	{0x18, 0x12},
	{0x19, 0x0b},
	{0x1a, 0x1a},
	{0x1e, 0x50},
	{0x1f, 0x80},
	{0x24, 0xc8},//c8
	{0x25, 0xe2},
	{0x27, 0xaf},
	{0x28, 0x24},
	{0x29, 0x14},
	{0x2f, 0x14},
	{0x3f, 0x18},//1b tx en
	{0x40, 0x26},	
	{0x72, 0xab},//20151012 0x9b
	{0x73, 0x4a},//20151012 0xea
	{0x74, 0x40},
	{0x75, 0x10},
	{0x76, 0xa2},//20151012 0x92
	{0x7a, 0x4e},
	{0xc1, 0x12},
	{0xc2, 0x0c},
	{0xcf, 0x48},
	{0xdc, 0x75},
	{0xeb, 0x78},
/*ISP*/
	{0xfe, 0x00},
	{0x90, 0x01},
	{0x92, 0x00},//Don't Change Here!!!
	{0x94, 0x00},//Don't Change Here!!!
	
	{0x95, 0x00},
	{0x96, 0xf0},
	{0x97, 0x01},
	{0x98, 0x40},
/*gain*/
	{0xb0, 0x50},//global_gain	
	{0xb1, 0x01},
	{0xb2, 0x00},	
	{0xb3, 0x40},
	{0xb4, 0x40},
	{0xb5, 0x40},
	{0xb6, 0x00},
	{0x30, 0x00},	
	{0x31, 0x01},
	{0x32, 0x02},
	{0x33, 0x03},
	{0x34, 0x07},
	{0x35, 0x0b},
	{0x36, 0x0f},
/*BLK*/
	{0xfe, 0x00},
	{0x40, 0x26}, 
	{0x4f, 0xc3},
	{0x5e, 0x00},
	{0x5f, 0x80},
/*dark sun*/
	{0xfe, 0x00},
	{0xe0, 0x9e},  
	{0xe1, 0x80},
	{0xe4, 0x0f},
	{0xe5, 0xff},
/*mipi*/
	{0xfe, 0x03},
	{0x10, 0x00},	
	{0x01, 0x03},
	{0x02, 0x22},
	{0x03, 0x96},
	{0x04, 0x01},
	{0x05, 0x00},
	{0x06, 0x80},
	//{0x10, 0x90},
	{0x11, 0x2b},
	
	{0x12, 0x90},
	{0x13, 0x01},
	
	{0x15, 0x00},
	{0x21, 0x10},
	{0x22, 0x00},
	{0x23, 0x30},
	{0x24, 0x02},
	{0x25, 0x12},
	{0x26, 0x03},
	{0x29, 0x01},
	{0x2a, 0x0a},
	{0x2b, 0x03},	
	{0xfe, 0x00},
	{0xf9, 0x0e},//[0] not_use_pll
	{0xfc, 0x0e},//[0] apwd
	{0xfe, 0x00},
};
//#endif

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
	#if 1
	sensor_print("before:tmp_exp_val:%d, temperature_ctrl:%d, gc0406_sensor_vts:%d\n", tmp_exp_val, temperature_ctrl, gc0406_sensor_vts);	
	if (tmp_exp_val > gc0406_sensor_vts){
		tmp_exp_val = gc0406_sensor_vts;
	}	
	#endif
	
	sensor_print("after:tmp_exp_val:%d, temperature_ctrl:%d\n", tmp_exp_val, temperature_ctrl);
	sensor_write(id, 0x03, (tmp_exp_val >> 8) & 0xFF);
	sensor_write(id, 0x04, (tmp_exp_val & 0xFF));

	return 0;
}

static unsigned char regValTable[7][2] = {   //0x29  0xb6 
	                            {0x0b,0x00},
                                {0x0b,0x01},
                                {0x0b,0x02},
                                {0x0b,0x03},
                                {0x14,0x04},
                                {0x14,0x05},
                                {0x14,0x06},

   						};
static unsigned int gainLevelTable[7] = {
							64,
							88,
							122,
							168,
							240,
							336,
							479,
};

static int setSensorGain(int id, int gain)
{
	int i;
	data_type temperature_value;
	unsigned int tol_dig_gain = 0;
	int total;	

	total = sizeof(gainLevelTable) / sizeof(unsigned int);
	
	for(i = 0; i < total - 1; i++)
	{
		if((gainLevelTable[i] <= gain)&&(gain < gainLevelTable[i+1]))
			break;
	}
	
	tol_dig_gain = gain*64/gainLevelTable[i];		
	sensor_dbg("tol_dig_gain:%d\n", tol_dig_gain);		
	sensor_write(id, 0x29, regValTable[i][0]);
	sensor_write(id, 0xb6, regValTable[i][1]);
	sensor_write(id, 0xb1, (tol_dig_gain>>6));
	sensor_write(id, 0xb2, ((tol_dig_gain&0x3f)<<2));

	return 0;
}

static int sensor_s_gain(int id, int gain_val)
{
	sensor_dbg("gain_val:%d\n", gain_val);
	setSensorGain(id, gain_val * 4);

	return 0;
}

static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	int shutter = 0, frame_length = 0;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < (1 * 16)) {
		gain_val = 16;
	}

	if (exp_val < (1 * 16)) {
		exp_val = 16;
	}
	
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	shutter = exp_val >> 4;
	if (shutter > gc0406_sensor_vts - 16)
		frame_length = shutter + 16;
	else
		frame_length = gc0406_sensor_vts;

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
#if 0
	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(id, 0x17, &get_value);
	sensor_dbg("ready to vflip, regs_data = 0x%x\n", get_value);

	if (enable) {
		set_value = get_value | 0x02;
		sensor_flip_status |= 0x02;
	} else {
		set_value = get_value & 0xFD;
		sensor_flip_status &= 0xFD;
	}
	sensor_write(id, 0x17, set_value);
	usleep_range(80000, 100000);
	sensor_read(id, 0x17, &get_value);
	sensor_dbg("after vflip, regs_data = 0x%x, sensor_flip_status = %d\n",
				get_value, sensor_flip_status);
#endif
	return 0;
}

static int sensor_s_hflip(int id, int enable)
{
	data_type get_value;
	data_type set_value;

#if 0
	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(id, 0x17, &get_value);
	sensor_dbg("ready to hflip, regs_data = 0x%x\n", get_value);

	if (enable) {
		set_value = get_value | 0x01;
		sensor_flip_status |= 0x01;
	} else {
		set_value = get_value & 0xFE;
		sensor_flip_status &= 0xFE;
	}
	sensor_write(id, 0x17, set_value);
	usleep_range(80000, 100000);
	sensor_read(id, 0x17, &get_value);
	sensor_dbg("after hflip, regs_data = 0x%x, sensor_flip_status = %d\n",
				get_value, sensor_flip_status);
#endif
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
	//*code = MEDIA_BUS_FMT_SBGGR10_1X10; // gc0406 support change the rgb format by itself
	*code = MEDIA_BUS_FMT_SGBRG10_1X10; // gc0406 support change the rgb format by itself
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
		sensor_print("PWR_ON!\n");
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		//vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		hal_usleep(5000);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(5000);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		hal_usleep(5000);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(5000);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!do nothing\n");
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

	sensor_dbg("[%s]-[%s]-[%d]:id:%d, status:%d\n", __FILE__, __func__, __LINE__, id, status);	
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
#else
	switch(status){
	case IR_NIGHT:
		vin_err("[%s]-[%s]-[%d]:IR_NIGHT\n", __FILE__, __func__, __LINE__);		
		if(config == NULL){
			sensor_dbg("[%s]-[%s]-[%d]:\n", __FILE__, __func__, __LINE__);			
			config = (struct pwm_config *)malloc(sizeof(struct pwm_config));	
		    hal_pwm_init();			
		}

	    config->duty_ns   = 7000000;
	    config->period_ns = 10000000;
	    config->polarity  = PWM_POLARITY_NORMAL;

	    hal_pwm_control(6, config);	
		sensor_dbg("[%s]-[%s]-[%d]:\n", __FILE__, __func__, __LINE__);	
		break;
	case IR_DAY:
		vin_err("[%s]-[%s]-[%d]:IR_DAY\n", __FILE__, __func__, __LINE__);	
		if(config == NULL){
			sensor_dbg("[%s]-[%s]-[%d]:\n", __FILE__, __func__, __LINE__);			
			config = (struct pwm_config *)malloc(sizeof(struct pwm_config));	
		    hal_pwm_init();			
		}			
		//if(config){
			sensor_dbg("[%s]-[%s]-[%d]:\n", __FILE__, __func__, __LINE__);	
		    config->duty_ns   = 0;
		    config->period_ns = 10000000;
		    config->polarity  = PWM_POLARITY_NORMAL;

		    hal_pwm_control(6, config);						
		//}
		break;
	default:
		return -1;		
	}	
#endif	
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
	sensor_dbg("[%s]-[%s]-[%d]:\n", __FILE__, __func__, __LINE__);	
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
		.mbus_code = MEDIA_BUS_FMT_SGBRG10_1X10, /*.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
		.width      = 800,
		.height     = 480,
		.hts        = 1519,
		.vts        = 516,
		.pclk       = 24 * 1000 * 1000,
		.mipi_bps   = 144 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 516 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_800x480p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_800x480p30_regs),
		//.set_size   = NULL,
	},	

#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
#if 0
	{
		.mbus_code = MEDIA_BUS_FMT_SGBRG10_1X10, /*.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
		.width      = 800,
		.height     = 480,
		.hts        = 1519,
		.vts        = 516,
		.pclk       = 24 * 1000 * 1000,
		.mipi_bps   = 144 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 516 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_800x480p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_800x480p30_regs),
		//.set_size   = NULL,
	},	
#endif
#if 1
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, /*.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
		.width      = 320,
		.height     = 240,
		.hts        = 1075,//1519,
		.vts        = 248,//516,
		.pclk       = 24 * 1000 * 1000,
		.mipi_bps   = 144 * 1000 * 1000,
		.fps_fixed  = 90,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 248 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs	    = sensor_320x240p90_regs,
		.regs_size  = ARRAY_SIZE(sensor_320x240p90_regs),
	},
#endif	
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
#if 1
	{
		.mbus_code = MEDIA_BUS_FMT_SGBRG10_1X10, /*.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
		.width      = 800,
		.height     = 480,
		.hts        = 1519,
		.vts        = 516,
		.pclk       = 24 * 1000 * 1000,
		.mipi_bps   = 144 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 516 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.switch_regs	    = sensor_800x480p30_regs,
		.switch_regs_size  = ARRAY_SIZE(sensor_800x480p30_regs),
	},
#else
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, /*.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
		.width      = 320,
		.height     = 240,
		.hts        = 1075,//1519,
		.vts        = 248,//516,
		.pclk       = 24 * 1000 * 1000,
		.mipi_bps   = 144 * 1000 * 1000,
		.fps_fixed  = 90,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 248 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.switch_regs	    = sensor_320x240p90_regs,
		.switch_regs_size  = ARRAY_SIZE(sensor_320x240p90_regs),
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
	//struct sensor_info *info = to_state(sd);

	cfg->type  = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	//res->res_time_hs = 0x28;
	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{
	int ret = 0;
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);
	struct sensor_exp_gain exp_gain;
	sensor_print("[%s]-[%d]:ispid:%d\n", __func__, __LINE__, ispid);

	ret = sensor_write_array(id, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}
	
	sensor_print("[%s]-[%d]:ispid:%d--qwerty!\n", __func__, __LINE__, ispid);
	if (current_win[id]->regs){
		ret = sensor_write_array(id, current_win[id]->regs, current_win[id]->regs_size);
		sensor_print("[%s]-[%s]-[%d]:regs_size:%d, ret:%d\n", __FILE__, __func__, __LINE__, current_win[id]->regs_size, ret);
		
		if (ret < 0)
			return ret;		
	}
	sensor_print("[%s]-[%d]:ispid:%d\n", __func__, __LINE__, ispid);

	gc0406_sensor_vts = current_win[id]->vts;
//#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_FAST_CONVERGENCE
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, 1125 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 110 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, 1125 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 110 << 4);
	}
//#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
//	temperature_ctrl = 0;
//#endif
	exp_gain.exp_val = 3568;
	exp_gain.gain_val = 1760;
	sensor_s_exp_gain(id, &exp_gain);
	//set stream on
	sensor_write(id, 0xfe, 0x03);
	sensor_write(id, 0x10, 0x90);
	sensor_write(id, 0xfe, 0x00);

	hal_msleep(40);
	//sensor_flip_status = 0x0;
	//sensor_dbg("gc0406_sensor_vts = %d\n", gc0406_sensor_vts);

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
	//temperature_ctrl = 1;
	gc0406_sensor_vts = current_switch_win[id]->vts;
	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_err("cannot find 320x240p90fps to 1080p%dfps reg\n", current_switch_win[id]->fps_fixed);
	if (ret < 0)
		return ret;	
	//set stream on
	sensor_write(id, 0xfe, 0x03);
	sensor_write(id, 0x10, 0x90);
	sensor_write(id, 0xfe, 0x00);
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

struct sensor_fuc_core gc0406_core  = {
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