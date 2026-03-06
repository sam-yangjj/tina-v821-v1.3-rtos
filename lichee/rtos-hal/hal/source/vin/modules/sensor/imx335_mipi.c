/*
 * A V4L2 driver for Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Chen weihong <chenweihong@allwinnertech.com>
 *
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
#define V4L2_IDENT_SENSOR  0x0335

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The IMX335 i2c address
 */
#define I2C_ADDR 0x52

#define SENSOR_NUM 0x2
#define SENSOR_NAME "imx335_mipi"
#define SENSOR_NAME_2 "imx335_mipi_2"

static int DOL_RHS1 = 354;

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

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC//FULL_SIZE
static struct regval_list sensor_10b_2592x1944_p25_regs[] = {
	// 2592x1944-10bit-25fps
	{0x3000, 0x01}, //stanby
	{0x3002, 0x00}, //master mode
	{0x3004, 0x04},
	{0x3004, 0x00},
	//All pixel AD Conversion 10bit / Output 10 bit / 891 Mbps /25fps
	{0x3018, 0x00}, //All-pixel scan mode
	{0x3030, 0x94}, //VMAX 4500
	{0x3031, 0x11}, //VMAX
	{0x3032, 0x00}, //VMAX
	{0x3034, 0x94}, //HMAX 660
	{0x3035, 0x02}, //HMAX
	{0x304c, 0x14}, //OPB_SIZE_V
	{0x304e, 0x00}, //HREVERSE
	{0x304f, 0x00}, //VREVERSE
	{0x3050, 0x00}, //AD10bit
	{0x3056, 0xac},
	{0x3057, 0x07},
	{0x3072, 0x28},
	{0x3073, 0x00},
	{0x3074, 0xb0},
	{0x3075, 0x00},
	{0x3076, 0x58},
	{0x3077, 0x0f},

	{0x3199, 0x00},
	{0x319d, 0x00}, //10bit AD
	{0x3300, 0x00},
	{0x341c, 0xff}, //10bit AD
	{0x341d, 0x01},
	{0x3a01, 0x03},
	{0x3a18, 0x8f},
	{0x3a19, 0x00},
	{0x3a1a, 0x4f},
	{0x3a1b, 0x00},
	{0x3a1c, 0x47},
	{0x3a1d, 0x00},
	{0x3a1e, 0x37},
	{0x3a1f, 0x01},

	//1188Mbps
	//{0x3a20, 0x4f},
	//{0x3a21, 0x00},
	//{0x3a22, 0x87},
	//{0x3a23, 0x00},
	//{0x3a24, 0x4f},
	//{0x3a25, 0x00},
	//{0x3a26, 0x7f},
	//{0x3a27, 0x00},
	//{0x3a28, 0x3f},
	//{0x3a29, 0x00},
	{0x3a20, 0x3f},
	{0x3a21, 0x00},
	{0x3a22, 0x6f},
	{0x3a23, 0x00},
	{0x3a24, 0x3f},
	{0x3a25, 0x00},
	{0x3a26, 0x5f},
	{0x3a27, 0x00},
	{0x3a28, 0x2f},
	{0x3a29, 0x00},

	//----1188Mbps/lane 24MHz
	//{0x300c, 0x3b},
	//{0x300d, 0x2a},
	//{0x314c, 0xc6},
	//{0x314d, 0x00},
	//{0x315a, 0x02},
	//{0x3168, 0xa0},
	//{0x316a, 0x7e},
	//{0x319e, 0x01},
	{0x300c, 0x3b}, //891Mbps/lane 24MHz
	{0x300d, 0x2a},
	{0x314c, 0x29},
	{0x314d, 0x01},
	{0x315a, 0x06},
	{0x3168, 0xa0},
	{0x316a, 0x7e},
	{0x319e, 0x02},

	{0x3000, 0x00}, //operation
};

static struct regval_list sensor_10b_2592x1944_p25wdr_regs[] = {
	// 2592x1944-10bit-30fps
	{0x3000, 0x01}, //stanby
	{0x3002, 0x00}, //master mode
	{0x3004, 0x04},
	{0x3004, 0x00},
	{0x30e8, 0x8c}, //gain

	//All pixel AD Conversion 10bit / Output 10 bit / 1188 Mbps /30fps
	{0x3018, 0x00}, //All-pixel scan mode
	{0x3030, 0x94}, //VMAX 4500
	{0x3031, 0x11}, //VMAX
	{0x3032, 0x00}, //VMAX
	//{0x3034, 0x13}, //HMAX 275 --- 30fps
	//{0x3035, 0x01}, //HMAX
	{0x3034, 0x4a}, //HMAX 330 --- 25fps
	{0x3035, 0x01}, //HMAX
	{0x304c, 0x14}, //OPB_SIZE_V
	{0x304e, 0x00}, //HREVERSE
	{0x304f, 0x00}, //VREVERSE
	{0x3050, 0x00}, //AD10bit
	{0x3056, 0xac},
	{0x3057, 0x07},
	{0x3072, 0x28},
	{0x3073, 0x00},
	{0x3074, 0xb0},
	{0x3075, 0x00},
	{0x3076, 0x58},
	{0x3077, 0x0f},

	{0x3199, 0x00},
	{0x319d, 0x00}, //10bit AD
	{0x3300, 0x00},
	{0x341c, 0xff}, //10bit AD
	{0x341d, 0x01},
	{0x3a01, 0x03},
	{0x3a18, 0x8f},
	{0x3a19, 0x00},
	{0x3a1a, 0x4f},
	{0x3a1b, 0x00},
	{0x3a1c, 0x47},
	{0x3a1d, 0x00},
	{0x3a1e, 0x37},
	{0x3a1f, 0x01},

	{0x3a20, 0x4f},
	{0x3a21, 0x00},
	{0x3a22, 0x87},
	{0x3a23, 0x00},
	{0x3a24, 0x4f},
	{0x3a25, 0x00},
	{0x3a26, 0x7f},
	{0x3a27, 0x00},
	{0x3a28, 0x3f},
	{0x3a29, 0x00},

	//----1188Mbps/lane 24MHz
	{0x300c, 0x3b},
	{0x300d, 0x2a},
	{0x314c, 0xc6},
	{0x314d, 0x00},
	{0x315a, 0x02},
	{0x3168, 0xa0},
	{0x316a, 0x7e},
	{0x319e, 0x01},

	//wdr
	{0x319f, 0x01}, //VCEN - LI (virtual channel mode)
	{0x3048, 0x01}, //WDMODE - DOL mode
	{0x3049, 0x01}, //WDSEL - DOL 2 frame
	{0x304a, 0x04}, //WD_SET1 - DOL 2 frame
	{0x304b, 0x03}, //WD_SET2 - DOL
	//{0x3050, 0x00}, //ADBIT - 10bit
	{0x3058, 0xe8}, //SHR0
	{0x3059, 0x21},
	{0x305a, 0x00},
	{0x305c, 0x8e}, //SHR1
	{0x305d, 0x00},
	{0x305e, 0x00},

	//{0x3068, (DOL_RHS1 & 0xff)},	//RHS1
	//{0x3069, ((DOL_RHS1 >> 8) & 0xff)},
	//{0x306a, ((DOL_RHS1 >> 16) & 0x0f)},
	{0x3068, (354 & 0xff)},	//RHS1
	{0x3069, ((354 >> 8) & 0xff)},
	{0x306a, ((354 >> 16) & 0x0f)},
	//{0x319d, 0x00},	//MDBIT - 10bit
	//{0x341c, 0xff},	//ADBIT1 - 10bit
	//{0x341d, 0x01},
	{0x304c, 0x13},	//OPB_SIZE_V - LI mode
	{0x3056, 0xac}, //Y_OUT_SIZE  LI
	{0x3057, 0x07},
	{0x31d7, 0x01}, //XVSMSKCNT

	{0x3000, 0x00}, //operation
};

#else  //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
static struct regval_list sensor_720p120_regs[] = {

};
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

static int imx335_sensor_vts;
static int sensor_s_exp(int id, unsigned int exp_val, unsigned int exp_val_short)
{
	data_type explow, expmid, exphigh;
	int exptime, exp_val_m;
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
		// RHS1
		exp_val_m = exp_val_short / 16;
		if (exp_val_m < 4)
			exp_val_m = 4;
		DOL_RHS1 = 18 + exp_val_m + 8;
		DOL_RHS1 = (DOL_RHS1 / 8) * 8 + 2;

		sensor_write(id, 0x3068, (DOL_RHS1 & 0xff));
		sensor_write(id, 0x3069, ((DOL_RHS1 >> 8) & 0xff));
		sensor_write(id, 0x306a, ((DOL_RHS1 >> 16) & 0x0f));
		
		// LEF
		exptime = (imx335_sensor_vts<<1) - (exp_val>>4);
		if (exptime < DOL_RHS1 + 18) {
			exptime = DOL_RHS1 + 18;
			exp_val = ((imx335_sensor_vts << 1) - exptime) << 4;
		}
		sensor_dbg("long exp_val: %d, exptime: %d\n", exp_val, exptime);

		exphigh	= (unsigned char) ((0x00f0000 & exptime) >> 16);
		expmid	= (unsigned char) ((0x000ff00 & exptime) >> 8);
		explow	= (unsigned char) ((0x00000ff & exptime));

		sensor_write(id, 0x3058, explow);
		sensor_write(id, 0x3059, expmid);
		sensor_write(id, 0x305a, exphigh);
		
		// SEF
		//exp_val_m = exp_val / DOL_RATIO;
		exp_val_m = exp_val_short;
		if (exp_val_m < 4 * 16)
			exp_val_m = 4 * 16;
		exptime = DOL_RHS1 - (exp_val_m >> 4);
		if (exptime < 18)
			exptime = 18;

		sensor_dbg("short exp_val: %d, exptime: %d\n", exp_val_m, exptime);

		exphigh	= (unsigned char) ((0x00f0000 & exptime) >> 16);
		expmid	= (unsigned char) ((0x000ff00 & exptime) >> 8);
		explow	= (unsigned char) ((0x00000ff & exptime));

		sensor_write(id, 0x305c, explow);
		sensor_write(id, 0x305d, expmid);
		sensor_write(id, 0x305e, exphigh);
	} else {
		exptime = imx335_sensor_vts - (exp_val >> 4) - 1;
		exphigh = (unsigned char)((0x00f0000 & exptime) >> 16);
		expmid =  (unsigned char)((0x000ff00 & exptime) >> 8);
		explow =  (unsigned char)((0x00000ff & exptime));
		sensor_write(id, 0x3058, explow);
		sensor_write(id, 0x3059, expmid);
		sensor_write(id, 0x305a, exphigh);
		sensor_dbg("sensor_set_exp = %d %d line Done!\n", exp_val, exptime);
	}

	return 0;
}

static unsigned char gain2db[497] = {
	0,   2,   3,	 5,   6,   8,	9,  11,  12,  13,  14,	15,  16,  17,
	18,  19,  20,	21,  22,  23,  23,  24,  25,  26,  27,	27,  28,  29,
	29,  30,  31,	31,  32,  32,  33,  34,  34,  35,  35,	36,  36,  37,
	37,  38,  38,	39,  39,  40,  40,  41,  41,  41,  42,	42,  43,  43,
	44,  44,  44,	45,  45,  45,  46,  46,  47,  47,  47,	48,  48,  48,
	49,  49,  49,	50,  50,  50,  51,  51,  51,  52,  52,	52,  52,  53,
	53,  53,  54,	54,  54,  54,  55,  55,  55,  56,  56,	56,  56,  57,
	57,  57,  57,	58,  58,  58,  58,  59,  59,  59,  59,	60,  60,  60,
	60,  60,  61,	61,  61,  61,  62,  62,  62,  62,  62,	63,  63,  63,
	63,  63,  64,	64,  64,  64,  64,  65,  65,  65,  65,	65,  66,  66,
	66,  66,  66,	66,  67,  67,  67,  67,  67,  68,  68,	68,  68,  68,
	68,  69,  69,	69,  69,  69,  69,  70,  70,  70,  70,	70,  70,  71,
	71,  71,  71,	71,  71,  71,  72,  72,  72,  72,  72,	72,  73,  73,
	73,  73,  73,	73,  73,  74,  74,  74,  74,  74,  74,	74,  75,  75,
	75,  75,  75,	75,  75,  75,  76,  76,  76,  76,  76,	76,  76,  77,
	77,  77,  77,	77,  77,  77,  77,  78,  78,  78,  78,	78,  78,  78,
	78,  79,  79,	79,  79,  79,  79,  79,  79,  79,  80,	80,  80,  80,
	80,  80,  80,	80,  80,  81,  81,  81,  81,  81,  81,	81,  81,  81,
	82,  82,  82,	82,  82,  82,  82,  82,  82,  83,  83,	83,  83,  83,
	83,  83,  83,	83,  83,  84,  84,  84,  84,  84,  84,	84,  84,  84,
	84,  85,  85,	85,  85,  85,  85,  85,  85,  85,  85,	86,  86,  86,
	86,  86,  86,	86,  86,  86,  86,  86,  87,  87,  87,	87,  87,  87,
	87,  87,  87,	87,  87,  88,  88,  88,  88,  88,  88,	88,  88,  88,
	88,  88,  88,	89,  89,  89,  89,  89,  89,  89,  89,	89,  89,  89,
	89,  90,  90,	90,  90,  90,  90,  90,  90,  90,  90,	90,  90,  91,
	91,  91,  91,	91,  91,  91,  91,  91,  91,  91,  91,	91,  92,  92,
	92,  92,  92,	92,  92,  92,  92,  92,  92,  92,  92,	93,  93,  93,
	93,  93,  93,	93,  93,  93,  93,  93,  93,  93,  93,	94,  94,  94,
	94,  94,  94,	94,  94,  94,  94,  94,  94,  94,  94,	95,  95,  95,
	95,  95,  95,	95,  95,  95,  95,  95,  95,  95,  95,	95,  96,  96,
	96,  96,  96,	96,  96,  96,  96,  96,  96,  96,  96,	96,  96,  97,
	97,  97,  97,	97,  97,  97,  97,  97,  97,  97,  97,	97,  97,  97,
	97,  98,  98,	98,  98,  98,  98,  98,  98,  98,  98,	98,  98,  98,
	98,  98,  98,	99,  99,  99,  99,  99,  99,  99,  99,	99,  99,  99,
	99,  99,  99,	99,  99,  99, 100, 100, 100, 100, 100, 100, 100, 100,
	100, 100, 100, 100, 100, 100, 100,
};

static int sensor_s_gain(int id, int gain_val)
{
	if (gain_val < 1 * 16)
		gain_val = 16;
	
	if (gain_val < 32 * 16) {
		sensor_write(id, 0x30e8, gain2db[gain_val - 16]);
	} else {
		sensor_write(id, 0x30e8, gain2db[(gain_val>>5) - 16] + 100);
	}
	sensor_dbg("sensor_set_gain = %d, Done!\n", gain_val);
	return 0;
}

static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, exp_mid_val, gain_val;
	unsigned int temp, gain_compen;
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
	exp_mid_val = exp_gain->exp_mid_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < (1 * 16)) {
		gain_val = 16;
	}
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	if (isp_wdr_mode == ISP_DOL_WDR_MODE) {
		if (exp_mid_val % 64) {
			temp = (unsigned int)exp_mid_val / 64;
			exp_mid_val = temp * 64;
			gain_compen = (exp_gain->exp_mid_val % 64) * 10000 / exp_mid_val;
			gain_val = (gain_compen * gain_val + 5000) / 10000 + gain_val;
			exp_val = exp_gain->exp_val * exp_mid_val / exp_gain->exp_mid_val;
		}
	}

	sensor_write(id, 0x3001, 0x01);
	sensor_s_exp(id, exp_val, exp_mid_val);
	sensor_s_gain(id, gain_val);
	sensor_write(id, 0x3001, 0x00);

	glb_exp_gain.exp_val = exp_val;
	glb_exp_gain.exp_mid_val = exp_mid_val;
	glb_exp_gain.gain_val = gain_val;

	sensor_print("gain_val:%d, exp_val:%d, exp_mid_val:%d\n", gain_val, exp_val, exp_mid_val);

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
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		//vin_set_pmu_channel(1);
		hal_usleep(1000);
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		hal_usleep(1000);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!do nothing\n");
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		vin_set_mclk(id, 0);
		//vin_set_pmu_channel(0);
		vin_gpio_set_status(id, RESET, 0);
		vin_gpio_set_status(id, PWDN, 0);
		break;
	default:
		return -1;
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
static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{

	sensor_dbg("%s: val=%d\n", __func__);
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
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
	sensor_read(id, 0x3008, &rdval);
	sensor_print("%s imx335 read value is 0x%x\n", __func__, rdval);
	sensor_read(id, 0x3034, &rdval);
	sensor_print("0x3034 is 0x%x\n", rdval);
	sensor_read(id, 0x3035, &rdval);
	sensor_print("0x3035 is 0x%x\n", rdval);
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
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC// FULL_SIZE
	{
		.mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,
		.width      = 2592,
		.height     = 1944,
		.hts        = 660,
		.vts        = 4500,
		.pclk       = 74250000,
		.mipi_bps   = 891 * 1000 * 1000,
		.fps_fixed  = 25,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (4500 - 10) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 2000 << 4,
		.hoffset    = 0,
		.voffset    = 0,
		.offs_h     = 0,
		.offs_v     = 0,
		.field      = V4L2_FIELD_NONE,
		.regs	    = sensor_10b_2592x1944_p25_regs,
		.regs_size  = ARRAY_SIZE(sensor_10b_2592x1944_p25_regs),
	},

	{
		.mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,
		.width      = 2592,
		.height     = 1944,
		.hts        = 330,
		.vts        = 4500,
		.pclk       = 74250000,
		.mipi_bps   = 1188 * 1000 * 1000,
		.fps_fixed  = 25,
		.bin_factor = 1,
		.wdr_mode   = ISP_DOL_WDR_MODE,
		.intg_min   = 1 << 4,
		.intg_max   = (4500 - 10) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 2000 << 4,
		.hoffset    = 0,
		.voffset    = 0,
		.offs_h     = 0,
		.offs_v     = 0,
		.field      = V4L2_FIELD_NONE,
		.regs       = sensor_10b_2592x1944_p25wdr_regs,
		.regs_size  = ARRAY_SIZE(sensor_10b_2592x1944_p25wdr_regs),
	},

#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
	{
		.mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,
		.width      = 1280,
		.height     = 720,
		.hts        = 705,
		.vts        = 1600,
		.pclk       = 135 * 1000 * 1000,
		.mipi_bps   = 677 * 1000 * 1000,
		.fps_fixed  = 120,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1600 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.hoffset    = 0,
		.voffset    = 0,
		.offs_h     = 0,
		.offs_v     = 0,
		.field      = V4L2_FIELD_NONE,
		.regs       = sensor_720p120_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p120_regs),
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
	sensor_wdr_mode[id] = sensor_format->wdr_mode;
	current_win[id] = sensor_format;
	return sensor_format;
#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC
	if (current_win[id])
		return current_win[id];

	sensor_wdr_mode[id] = sensor_formats[0].wdr_mode;
	current_win[id] = &sensor_formats[0];
	sensor_print("fine wdr is %d, fps is %d\n", sensor_formats[0].wdr_mode, sensor_formats[0].fps_fixed);
	return &sensor_formats[0];
#endif
}

static struct sensor_format_struct switch_sensor_formats[] = {
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC

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
	int ret = -1;
	struct sensor_exp_gain exp_gain;
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);

	//unsigned int timestamp1, timestamp2;
	//timestamp1 = get_sys_ticks();

	sensor_dbg("ARRAY_SIZE(sensor_default_regs)=%d\n",
			(unsigned int)ARRAY_SIZE(sensor_default_regs));

	ret = sensor_write_array(id, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	imx335_sensor_vts = current_win[id]->vts;
	if (current_win[id]->regs)
		ret = sensor_write_array(id, current_win[id]->regs, current_win[id]->regs_size);
	if (ret < 0)
		return ret;
//#if !defined CONFIG_ISP_ONLY_HARD_LIGHTADC
	if (ispid == 0) {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, 4500 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 2000 << 4);
		exp_gain.exp_mid_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 4), 16, 4500 << 4);
		exp_gain.gain_mid_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 3), 16, 2000 << 4);
	} else {
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, 4500 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 2000 << 4);
		exp_gain.exp_mid_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 4), 16, 4500 << 4);
		exp_gain.gain_mid_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 3), 16, 2000 << 4);
	}
	sensor_s_exp_gain(id, &exp_gain);
//#endif
	sensor_write(id, 0x3000, 0x00);

	//sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
	//		 wsize->height);

	//timestamp2 = get_sys_ticks();
	//sensor_print("sensor write reg time is %d(ms)\n", timestamp2 - timestamp1);

	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

	sensor_dbg("%s on = %d, 2592*1944 fps: 25\n", __func__, enable);

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
	imx335_sensor_vts = current_switch_win[id]->vts;

	sensor_write(id, 0x3000, 0x01);
	if (glb_exp_gain.exp_val && glb_exp_gain.gain_val) {
		exp_gain.exp_val = glb_exp_gain.exp_val;
		exp_gain.exp_mid_val = glb_exp_gain.exp_mid_val;
		exp_gain.gain_val = glb_exp_gain.gain_val;
	} else {
		exp_gain.exp_val = 20480;
		exp_gain.exp_mid_val = 1280;
		exp_gain.gain_val = 32;
	}

	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_err("cannot find 720p120fps to 2k%dfps reg\n", current_switch_win[id]->fps_fixed);
	if (ret < 0)
		return ret;
	sensor_s_exp_gain(id, &exp_gain); /* make switch_regs firstframe  */
	sensor_write(id, 0x3000, 0x00); /* stream_on */
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

struct sensor_fuc_core imx335_core  = {
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

