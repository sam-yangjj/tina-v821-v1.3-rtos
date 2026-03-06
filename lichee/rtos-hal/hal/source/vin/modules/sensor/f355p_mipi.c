#include <hal_timer.h>

#include <hal_timer.h>
#include "../../vin_mipi/combo_common.h"
#include "camera.h"
#include "../../utility/sunxi_camera_v2.h"
#include "../../utility/media-bus-format.h"
#include "../../utility/vin_supply.h"

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR  0x0846

/*
 * Our nominal (default) frame rate.
 */
#define ID_REG_HIGH		0x0A
#define ID_REG_LOW		0x0B
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xFF)

/*
 * The i2c address
 */
#define I2C_ADDR 0x80

#define SENSOR_NUM 0x2
#define SENSOR_NAME "f355p_mipi"
#define SENSOR_NAME_2 "f355p_mipi_2"

#define F355P_1920X1080_15FPS
#define SENSOR_FRAME_RATE 15

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];
static struct sensor_format_struct *current_switch_win[2];

#define FLIP_BEFORE_STREAM_ON

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#ifdef F355P_1920X1080_30FPS
static struct regval_list sensor_1080p30_regs[] = {
	{0x12, 0x40},
	{0x48, 0x8F},
	{0x48, 0x0F},
	{0x0E, 0x11},
	{0x0F, 0x04},
	{0x10, 0x24},
	{0x11, 0x80},
	{0x46, 0x18},
	{0x0D, 0xA0},
	{0x57, 0x60},
	{0x58, 0x18},
	{0x5F, 0x41},
	{0x60, 0x20},
	{0x20, 0x00},
	{0x21, 0x05},
	{0x22, 0x65},
	{0x23, 0x04},
	{0x24, 0xC0},
	{0x25, 0x38},
	{0x26, 0x43},
	{0x27, 0xFD},
	{0x28, 0x19},
	{0x29, 0x04},
	{0x2A, 0xF0},
	{0x2B, 0x14},
	{0x2C, 0x00},
	{0x2D, 0x00},
	{0x2E, 0x15},
	{0x2F, 0x44},
	{0x41, 0xC4},
	{0x42, 0x33},
	{0x47, 0x52},
	{0x80, 0x22},
	{0xAF, 0x22},
	{0xBD, 0x80},
	{0xBE, 0x07},
	{0x9B, 0x83},
	{0xAB, 0x00},
	{0x82, 0x30},
	{0x1D, 0x00},
	{0x1E, 0x04},
	{0x6C, 0x40},
	{0x68, 0x10},
	{0x70, 0x8D},
	{0x71, 0x6E},
	{0x72, 0x2A},
	{0x73, 0x36},
	{0x75, 0x94},
	{0x74, 0x12},
	{0x89, 0x09},
	{0x0C, 0xD0},
	{0x6B, 0x20},
	{0x86, 0x00},
	{0x6E, 0x4C},
	{0x78, 0x44},
	{0xA1, 0xBF},
	{0x32, 0x3F},
	{0x33, 0x64},
	{0x34, 0x4F},
	{0x35, 0x4F},
	{0x3A, 0xAF},
	{0x56, 0x0A},
	{0x59, 0x87},
	{0x61, 0x28},
	{0x85, 0x58},
	{0x8E, 0xA0},
	{0x91, 0x14},
	{0x94, 0x48},
	{0x9F, 0x41},
	{0xBA, 0x04},
	{0xBF, 0x01},
	{0x57, 0x24},
	{0xBF, 0x00},
	{0xAA, 0x40},
	{0x5A, 0xC1},
	{0x5B, 0xE0},
	{0x5C, 0x58},
	{0x5D, 0x6F},
	{0x5E, 0x85},
	{0x63, 0x0F},
	{0x64, 0xEC},
	{0x65, 0x82},
	{0x66, 0x00},
	{0x67, 0x79},
	{0x69, 0xF0},
	{0x7A, 0x88},
	{0x8C, 0x0A},
	{0x84, 0x00},
	{0x8F, 0x90},
	{0x9D, 0x10},
	{0xBF, 0x01},
	{0x55, 0x10},
	{0x56, 0xE1},
	{0xBF, 0x00},
	{0x97, 0x7A},
	{0x13, 0x01},
	{0x96, 0x04},
	{0x4A, 0x01},
	{0x50, 0x02},
	{0x49, 0x10},
	{0xBF, 0x01},
	{0x4E, 0x11},
	{0x50, 0x00},
	{0x55, 0x10},
	{0x51, 0x8F},
	{0x64, 0xE0},
	{0x6A, 0x15},
	{0x6B, 0x80},
	{0x6C, 0x32},
	{0xBF, 0x00},
	{0x7E, 0xCC},
	{0x7F, 0x5F},
	{0xA7, 0x00},
	{0x6A, 0x4A},
	{0x19, 0x20},
	{0x12, 0x00}
};
#endif

#ifdef F355P_1920X1080_20FPS
static struct regval_list sensor_1080p20_regs[] = {
	{0x12, 0x40},
	{0x48, 0x8F},
	{0x48, 0x0F},
	{0x0E, 0x11},
	{0x0F, 0x04},
	{0x10, 0x24},
	{0x11, 0x80},
	{0x46, 0x18},
	{0x0D, 0xA0},
	{0x57, 0x60},
	{0x58, 0x18},
	{0x5F, 0x41},
	{0x60, 0x20},
	{0x20, 0x00},
	{0x21, 0x05},
	{0x22, 0x98},
	{0x23, 0x06},
	{0x24, 0xC0},
	{0x25, 0x38},
	{0x26, 0x43},
	{0x27, 0xFD},
	{0x28, 0x19},
	{0x29, 0x04},
	{0x2A, 0xF0},
	{0x2B, 0x14},
	{0x2C, 0x00},
	{0x2D, 0x00},
	{0x2E, 0x15},
	{0x2F, 0x44},
	{0x41, 0xC4},
	{0x42, 0x33},
	{0x47, 0x52},
	{0x80, 0x22},
	{0xAF, 0x22},
	{0xBD, 0x80},
	{0xBE, 0x07},
	{0x9B, 0x83},
	{0xAB, 0x00},
	{0x82, 0x30},
	{0x1D, 0x00},
	{0x1E, 0x04},
	{0x6C, 0x40},
	{0x68, 0x10},
	{0x70, 0x8D},
	{0x71, 0x6E},
	{0x72, 0x2A},
	{0x73, 0x36},
	{0x75, 0x94},
	{0x74, 0x12},
	{0x89, 0x09},
	{0x0C, 0xD0},
	{0x6B, 0x20},
	{0x86, 0x00},
	{0x6E, 0x4C},
	{0x78, 0x44},
	{0xA1, 0xBF},
	{0x32, 0x3F},
	{0x33, 0x64},
	{0x34, 0x4F},
	{0x35, 0x4F},
	{0x3A, 0xAF},
	{0x56, 0x0A},
	{0x59, 0x87},
	{0x61, 0x28},
	{0x85, 0x58},
	{0x8E, 0xA0},
	{0x91, 0x14},
	{0x94, 0x48},
	{0x9F, 0x41},
	{0xBA, 0x04},
	{0xBF, 0x01},
	{0x57, 0x24},
	{0xBF, 0x00},
	{0xAA, 0x40},
	{0x5A, 0xC1},
	{0x5B, 0xE0},
	{0x5C, 0x58},
	{0x5D, 0x6F},
	{0x5E, 0x85},
	{0x63, 0x0F},
	{0x64, 0xEC},
	{0x65, 0x82},
	{0x66, 0x00},
	{0x67, 0x79},
	{0x69, 0xF0},
	{0x7A, 0x88},
	{0x8C, 0x0A},
	{0x84, 0x00},
	{0x8F, 0x90},
	{0x9D, 0x10},
	{0xBF, 0x01},
	{0x55, 0x10},
	{0x56, 0xE1},
	{0xBF, 0x00},
	{0x97, 0x7A},
	{0x13, 0x01},
	{0x96, 0x04},
	{0x4A, 0x01},
	{0x50, 0x02},
	{0x49, 0x10},
	{0xBF, 0x01},
	{0x4E, 0x11},
	{0x50, 0x00},
	{0x55, 0x10},
	{0x51, 0x8F},
	{0x64, 0xE0},
	{0x6A, 0x15},
	{0x6B, 0x80},
	{0x6C, 0x32},
	{0xBF, 0x00},
	{0x7E, 0xCC},
	{0x7F, 0x5F},
	{0xA7, 0x00},
	{0x6A, 0x4A},
	{0x19, 0x20},
	{0x12, 0x00}
};
#endif

#ifdef F355P_1920X1080_15FPS
static struct regval_list sensor_1080p15_regs[] = {
	{0x12, 0x40},
	{0x48, 0x8F},
	{0x48, 0x0F},
	{0x0E, 0x11},
	{0x0F, 0x04},
	{0x10, 0x24},
	{0x11, 0x80},
	{0x46, 0x18},
	{0x0D, 0xA0},
	{0x57, 0x60},
	{0x58, 0x18},
	{0x5F, 0x41},
	{0x60, 0x20},
	{0x20, 0x00},
	{0x21, 0x05},
	{0x22, 0xCA},
	{0x23, 0x08},
	{0x24, 0xC0},
	{0x25, 0x38},
	{0x26, 0x43},
	{0x27, 0xFD},
	{0x28, 0x19},
	{0x29, 0x04},
	{0x2A, 0xF0},
	{0x2B, 0x14},
	{0x2C, 0x00},
	{0x2D, 0x00},
	{0x2E, 0x15},
	{0x2F, 0x44},
	{0x41, 0xC4},
	{0x42, 0x33},
	{0x47, 0x52},
	{0x80, 0x22},
	{0xAF, 0x22},
	{0xBD, 0x80},
	{0xBE, 0x07},
	{0x9B, 0x83},
	{0xAB, 0x00},
	{0x82, 0x30},
	{0x1D, 0x00},
	{0x1E, 0x04},
	{0x6C, 0x40},
	{0x68, 0x10},
	{0x70, 0x8D},
	{0x71, 0x6E},
	{0x72, 0x2A},
	{0x73, 0x36},
	{0x75, 0x94},
	{0x74, 0x12},
	{0x89, 0x09},
	{0x0C, 0xD0},
	{0x6B, 0x20},
	{0x86, 0x00},
	{0x6E, 0x4C},
	{0x78, 0x44},
	{0xA1, 0xBF},
	{0x32, 0x3F},
	{0x33, 0x64},
	{0x34, 0x4F},
	{0x35, 0x4F},
	{0x3A, 0xAF},
	{0x56, 0x0A},
	{0x59, 0x87},
	{0x61, 0x28},
	{0x85, 0x58},
	{0x8E, 0xA0},
	{0x91, 0x14},
	{0x94, 0x48},
	{0x9F, 0x41},
	{0xBA, 0x04},
	{0xBF, 0x01},
	{0x57, 0x24},
	{0xBF, 0x00},
	{0xAA, 0x40},
	{0x5A, 0xC1},
	{0x5B, 0xE0},
	{0x5C, 0x58},
	{0x5D, 0x6F},
	{0x5E, 0x85},
	{0x63, 0x0F},
	{0x64, 0xEC},
	{0x65, 0x82},
	{0x66, 0x00},
	{0x67, 0x79},
	{0x69, 0xF0},
	{0x7A, 0x88},
	{0x8C, 0x0A},
	{0x84, 0x00},
	{0x8F, 0x90},
	{0x9D, 0x10},
	{0xBF, 0x01},
	{0x55, 0x10},
	{0x56, 0xE1},
	{0xBF, 0x00},
	{0x97, 0x7A},
	{0x13, 0x01},
	{0x96, 0x04},
	{0x4A, 0x01},
	{0x50, 0x02},
	{0x49, 0x10},
	{0xBF, 0x01},
	{0x4E, 0x11},
	{0x50, 0x00},
	{0x55, 0x10},
	{0x51, 0x8F},
	{0x64, 0xE0},
	{0x6A, 0x15},
	{0x6B, 0x80},
	{0x6C, 0x32},
	{0xBF, 0x00},
	{0x7E, 0xCC},
	{0x7F, 0x5F},
	{0xA7, 0x00},
	{0x6A, 0x4A},
	{0x19, 0x20},
	{0x12, 0x00},
#ifndef FLIP_BEFORE_STREAM_ON
	{0x00, 0x10},
#endif
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
 * if not support the follow function, retrun -EINVAL
 */


static int sensor_s_exp(int id, unsigned int exp_val)
{
	sensor_write(id, 0x02, (exp_val >> 12) & 0xFF);
	sensor_write(id, 0x01, (exp_val >> 4) & 0xFF);

	return 0;
}

static int setSensorGain(int id, int gain)
{
	int	again = 0;
	int	tmp = 0;
	unsigned char	regdata = 0;

	/// gain: 16=1X, 32=2X, 48=3X, 64=4X, ......, 240=15X, 256=16X, ......
	again = gain;
	while (again > 31) {
		again = again >> 1;
		tmp++;
	}
	if (again > 15)
		again = again - 16;
	regdata = (unsigned char)((tmp<<4) | again);

	sensor_write(id, 0x00, regdata & 0xFF);

	return 0;
}

static int sensor_s_gain(int id, int gain_val)
{
	sensor_dbg("gain_val:%d\n", gain_val);
	setSensorGain(id, gain_val);

	return 0;
}

static int f355p_sensor_vts;

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
	if (shutter > f355p_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = f355p_sensor_vts;

	//sensor_print("frame_length = %d\n", frame_length);
	/* write vts */
	sensor_write(id, 0x22, frame_length & 0xff);
	sensor_write(id, 0x23, frame_length >> 8);

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
		sensor_print("PWR_ON!\n");
		vin_gpio_set_status(id, RESET, 1);
		hal_usleep(1000);
		sensor_print("Ready to set RESEY PWD_ON!\n");
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);        /// Pull down RESET# pin initially.
		hal_usleep(1000);
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(100);

		hal_usleep(100);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(10000);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		sensor_print("SET  RESEY PWD_ON success!\n");

		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!do nothing\n");
		sensor_print("Ready to set RESEY PWD_OFF!\n");
		hal_usleep(10000);			/// > 512 MCLK cycles
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(100);						/// Add delay.
		vin_set_mclk(id, 0);						/// Disable MCLK.
		vin_gpio_set_status(id, RESET, 0);
		sensor_print("SET RESEY PWD_OFF success!\n");
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_set_ir(int id, int status)
{
	// vin_gpio_set_status(id, IR_CUT0, 1);
	// vin_gpio_set_status(id, IR_CUT1, 1);
	// vin_gpio_set_status(id, IR_LED, 1);
	// switch (status) {
	// case IR_DAY:
	//     vin_gpio_write(id, IR_CUT0, CSI_GPIO_HIGH);
	//     vin_gpio_write(id, IR_CUT1, CSI_GPIO_LOW);
	//     vin_gpio_write(id, IR_LED, CSI_GPIO_LOW);
	//     break;
	// case IR_NIGHT:
	//     vin_gpio_write(id, IR_CUT0, CSI_GPIO_LOW);
	//     vin_gpio_write(id, IR_CUT1, CSI_GPIO_HIGH);
	//     vin_gpio_write(id, IR_LED, CSI_GPIO_HIGH);
	//     break;
	// default:
	//     return -1;
	// }
	return 0;
}

static int sensor_detect(int id)
{
	data_type rdval;

	sensor_read(id, ID_REG_HIGH, &rdval);
	sensor_print("ID_VAL_HIGH = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_HIGH)
		return -ENODEV;

	sensor_read(id, ID_REG_LOW, &rdval);
	sensor_print("ID_VAL_LOW = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_LOW)
		return -ENODEV;

	sensor_print("Sensor detect Done!\n");
	return 0;
}

static int sensor_init(int id)
{
	int ret;

//	printk("**********************************\r\n");
//	printk("**f355p sensor init(2023-10-20)**\r\n");
//	printk("**********************************\r\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(id);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	return 0;
}

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_format_struct sensor_formats[] = {
#ifdef F355P_1920X1080_30FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2560,
		.vts        = 1125,
		.pclk       = 86.4*1000*1000,
		.mipi_bps	= 432*1000*1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1125 << 4,
		.gain_min   = 1 << 4,
		.gain_max	= 15 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs       = sensor_1080p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p30_regs),
	},
#endif

#ifdef F355P_1920X1080_20FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2560,
		.vts        = 1688,
		.pclk       = 86.4*1000*1000,
		.mipi_bps	= 432*1000*1000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1688 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 15 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs       = sensor_1080p20_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p20_regs),
	},
#endif

#ifdef F355P_1920X1080_15FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2560,
		.vts        = 2250,
		.pclk       = 86.4*1000*1000,
		.mipi_bps   = 432*1000*1000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 2250 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 15 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs       = sensor_1080p15_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p15_regs),
	},
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
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	res->res_time_hs = 0x28;

	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{
	int ret;
	data_type rdval_l, rdval_h;
	struct tagsensor_isp_config_s *sensor_isp_cfg = NULL;
	data_type flip_status;
	struct sensor_exp_gain exp_gain;

	ret = sensor_write_array(id, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	if (current_win[id]->regs)
		ret = sensor_write_array(id, current_win[id]->regs, current_win[id]->regs_size);
	f355p_sensor_vts = current_win[id]->vts;

	switch (id) {
	case 0:
		sensor_isp_cfg = (struct tagsensor_isp_config_s *)VIN_SENSOR0_RESERVE_ADDR;
		break;
	case 1:
		sensor_isp_cfg = (struct tagsensor_isp_config_s *)VIN_SENSOR1_RESERVE_ADDR;
		break;
#ifdef CONFIG_SUPPORT_THREE_CAMERA
	case 2:
		sensor_isp_cfg = (struct tagsensor_isp_config_s *)VIN_SENSOR2_RESERVE_ADDR;
		break;
#endif
	default:
		sensor_isp_cfg = (struct tagsensor_isp_config_s *)VIN_SENSOR0_RESERVE_ADDR;
		break;
	}

#ifdef FLIP_BEFORE_STREAM_ON
	if ((id == 0 && sensor_isp_cfg->sign == 0xAA66AA66) ||
#ifdef CONFIG_SUPPORT_THREE_CAMERA
		(id != 0 && sensor_isp_cfg->sign == 0xBB66BB66) ||
		(id != 0 && sensor_isp_cfg->sign == 0xCC66CC66)) {
#else
		(id != 0 && sensor_isp_cfg->sign == 0xBB66BB66)) {
#endif
		sensor_read(id, 0x12, &flip_status);
		if (sensor_isp_cfg->filp) {
			flip_status = flip_status | 0x10;
			sensor_print("melis to do flip\n");
		} else {
			flip_status = flip_status & 0xef;
		}
		if (sensor_isp_cfg->mirror) {
			flip_status = flip_status | 0x20;
			sensor_print("melis to do mirror\n");
		} else {
			flip_status = flip_status & 0xdf;
		}
		sensor_write(id, 0x12, flip_status);
	}
#endif

	switch (isp_id) {
	case 0:
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, 1350 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 110 << 4);
		break;
	case 1:
		exp_gain.exp_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 2), 16, 1350 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP1_NORFLASH_SAVE + 1), 16, 110 << 4);
		break;
#ifdef CONFIG_SUPPORT_THREE_CAMERA
	case 2:
		exp_gain.exp_val = clamp(*((unsigned int *)ISP2_NORFLASH_SAVE + 2), 16, 1350 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP2_NORFLASH_SAVE + 1), 16, 110 << 4);
		break;
#endif
	default:
		exp_gain.exp_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 2), 16, 1350 << 4);
		exp_gain.gain_val = clamp(*((unsigned int *)ISP0_NORFLASH_SAVE + 1), 16, 110 << 4);
		break;
	}
	sensor_s_exp_gain(id, &exp_gain);
#ifdef FLIP_BEFORE_STREAM_ON
	sensor_write(id, 0x00, 0x10);
#endif

	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	int ret;
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

    sensor_print("%s on = %d\n", __func__, enable);

	if (!enable)
		return 0;
	
	ret = sensor_detect(id);
	if (ret) {
		sensor_print("sensor id : %d ,f355p i2c detect error ,  chip found is not an target chip.\n", id);
		return ret;
	} else {
		sensor_print("sensor id : %d f355p i2c detect success!!!.\n", id);
	}

	return sensor_reg_init(id, isp_id);
}

static int sensor_s_switch(int id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	struct sensor_exp_gain exp_gain;
	int ret = -1;
	f37p_sensor_vts = current_switch_win[id]->vts;
	if (current_switch_win[id]->switch_regs)
		ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	else
		sensor_print("[sensor_s_switch]:cannot find 480p120fps to 1080p%dfps reg\n", current_switch_win[id]->fps_fixed);
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

struct sensor_fuc_core f355p_core  = {
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
