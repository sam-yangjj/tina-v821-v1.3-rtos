#include <hal_timer.h>

#include <hal_timer.h>
#include "../../vin_mipi/combo_common.h"
#include "camera.h"
#include "../../utility/sunxi_camera_v2.h"
#include "../../utility/media-bus-format.h"
#include "../../utility/vin_supply.h"

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR  0x0841

/*
 * Our nominal (default) frame rate.
 */
#define ID_REG_HIGH		0x0A
#define ID_REG_LOW		0x0B
#define ID_VAL_HIGH		0x08
#define ID_VAL_LOW		0x41

/*
 * The i2c address
 */
#define I2C_ADDR 0x80

#define SENSOR_NUM 0x1
#define SENSOR_NAME "f37p_dvp"
#define SENSOR_NAME_2 "f37p_dvp_2"
#define CLK_POH           V4L2_MBUS_PCLK_SAMPLE_RISING
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_FALLING
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH

#define F37P_1920X1080_15FPS
#define SENSOR_FRAME_RATE 15

static int sensor_power_count[3];
static int sensor_stream_count[3];
static struct sensor_format_struct *current_win[3];
static struct sensor_format_struct *current_switch_win[3];

#define FLIP_BEFORE_STREAM_ON

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC
#ifdef F37P_1920X1080_15FPS
static struct regval_list sensor_1080p15_regs[] = {
	{0x12, 0x40},
	{0x48, 0x85},
	{0x48, 0x05},
	{0x0E, 0x1D},
	{0x0F, 0x04},
	{0x10, 0x24},
	{0x11, 0x80},
	{0x46, 0x01},
	{0x47, 0x62},
	{0x0D, 0xF2},
	{0x57, 0x6A},
	{0x58, 0x22},
	{0x5F, 0x41},
	{0x60, 0x28},
	{0xA5, 0xC0},
	{0x20, 0x00},
	{0x21, 0x05},
	{0x22, 0x65},
	{0x23, 0x04},
	{0x24, 0xC0},
	{0x25, 0x38},
	{0x26, 0x43},
	{0x27, 0xA0},
	{0x28, 0x15},
	{0x29, 0x03},
	{0x2A, 0x95},
	{0x2B, 0x13},
	{0x2C, 0x02},
	{0x2D, 0x00},
	{0x2E, 0x14},
	{0x2F, 0x04},
	{0x41, 0xC5},
	{0x42, 0x33},
	{0x47, 0x42},
	{0x76, 0x60},
	{0x77, 0x09},
	{0x80, 0x01},
	{0xAF, 0x22},
	{0xAB, 0x00},
	{0x1D, 0xFF},
	{0x1E, 0xBF},
	{0x6C, 0xC0},
	{0x9E, 0xF8},
	{0x30, 0x88},
	{0x31, 0x08},
	{0x32, 0x0C},
	{0x33, 0xD4},
	{0x34, 0x2F},
	{0x35, 0x2F},
	{0x3A, 0xAF},
	{0x3B, 0x00},
	{0x3C, 0xFF},
	{0x3D, 0xFF},
	{0x3E, 0x60},
	{0x3F, 0x00},
	{0x40, 0x00},
	{0x56, 0x92},
	{0x59, 0x4A},
	{0x5A, 0x47},
	{0x61, 0x18},
	{0x6F, 0x04},
	{0x85, 0x24},
	{0x8A, 0x44},
	{0x91, 0x13},
	{0x94, 0xA0},
	{0x9B, 0x83},
	{0x9C, 0xE1},
	{0xA4, 0x80},
	{0xA6, 0x22},
	{0xA9, 0x1C},
	{0x5B, 0xE7},
	{0x5C, 0x28},
	{0x5D, 0x67},
	{0x5E, 0x11},
	{0x62, 0x21},
	{0x63, 0x0F},
	{0x64, 0xD0},
	{0x65, 0x02},
	{0x67, 0x49},
	{0x66, 0x00},
	{0x68, 0x04},
	{0x69, 0x72},
	{0x6A, 0x12},
	{0x7A, 0x00},
	{0x82, 0x20},
	{0x8D, 0x47},
	{0x8F, 0x90},
	{0x45, 0x01},
	{0x97, 0x20},
	{0x13, 0x81},
	{0x96, 0x84},
	{0x4A, 0x01},
	{0xB1, 0x00},
	{0xA1, 0x0F},
	{0xBE, 0x00},
	{0x7E, 0x48},
	{0xB5, 0xC0},
	{0x50, 0x02},
	{0x49, 0x10},
	{0x7F, 0x57},
	{0x90, 0x00},
	{0x7B, 0x4A},
	{0x7C, 0x0C},
	{0x8C, 0xFF},
	{0x8E, 0x00},
	{0x8B, 0x01},
	{0x0C, 0x00},
	{0xBC, 0x11},
	{0x19, 0x20},
	{0x1B, 0x4F},
	{0x12, 0x00},
#ifndef FLIP_BEFORE_STREAM_ON
	{0x00, 0x10},
#endif
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

static int sensor_s_exp(int id, unsigned int exp_val)
{
	int tmp_exp_val = exp_val / 16;

	sensor_dbg("exp_val:%d\n", exp_val);
	sensor_write( id, 0x02, (tmp_exp_val >> 8) & 0xFF );
	sensor_write( id, 0x01, (tmp_exp_val     ) & 0xFF );

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

static int f37p_sensor_vts;

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
	if (shutter > f37p_sensor_vts - 16)
		frame_length = shutter + 16;
	else
		frame_length = f37p_sensor_vts;
	sensor_dbg("frame_length = %d\n", frame_length);
	sensor_write(id, 0x22, frame_length & 0xff);
	sensor_write(id, 0x23, frame_length >> 8);

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
		sensor_print("PWR_ON!\n");
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(1000);
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(10000);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(1000);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!do nothing\n");
		vin_set_mclk(id, 0);
		hal_usleep(1000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_set_ir(int id, int status)
{
//	vin_gpio_set_status(id, IR_CUT0, 1);
//	vin_gpio_set_status(id, IR_CUT1, 1);
//	vin_gpio_set_status(id, IR_LED, 1);
//	switch (status) {
//	case IR_DAY:
//		vin_gpio_write(id, IR_CUT0, CSI_GPIO_HIGH);
//		vin_gpio_write(id, IR_CUT1, CSI_GPIO_LOW);
//		vin_gpio_write(id, IR_LED, CSI_GPIO_LOW);
//		break;
//	case IR_NIGHT:
//		vin_gpio_write(id, IR_CUT0, CSI_GPIO_LOW);
//		vin_gpio_write(id, IR_CUT1, CSI_GPIO_HIGH);
//		vin_gpio_write(id, IR_LED, CSI_GPIO_HIGH);
//		break;
//	default:
//		return -1;
//	}
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
//	printk("**f37p sensor init(2023-10-20)**\r\n");
//	printk("**********************************\r\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(id);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	return 0;
}

static struct sensor_format_struct sensor_formats[] = {
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC
#ifdef F37P_1920X1080_15FPS
	{
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width      = 1920,
		.height     = 1080,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3200,
		.vts        = 1350,
		.pclk       = 64.8*1000*1000,
		.mipi_bps   = 324*1000*1000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1350 - 4) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.offs_h     = 0,
		.offs_v     = 0,
		.regs       = sensor_1080p15_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p15_regs),
	},
#endif

#else //CONFIG_ISP_FAST_CONVERGENCE || CONFIG_ISP_HARD_LIGHTADC

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
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | CLK_POL | HREF_POL | VREF_POL;

	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{
	int ret = 0;
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);
	struct sensor_exp_gain exp_gain;
	struct tagsensor_isp_config_s *sensor_isp_cfg = NULL;
	data_type flip_status;

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

	f37p_sensor_vts = current_win[id]->vts;

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

	switch (ispid) {
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
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

    sensor_print("%s on = %d\n", __func__, enable);

	if (!enable)
		return 0;

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
		sensor_err("cannot find 1920x360p120fps to 1080p%dfps reg\n", current_switch_win[id]->fps_fixed);
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

struct sensor_fuc_core f37p_dvp_core  = {
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
