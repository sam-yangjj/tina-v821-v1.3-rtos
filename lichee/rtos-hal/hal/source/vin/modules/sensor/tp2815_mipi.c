/*
 * A V4L2 driver for tp2815 cameras and TVI Coax protocol.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Hongyi <hongyi@allwinnertech.com>
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



/*define module timing*/
#define MCLK              (27*1000*1000)
#define V4L2_IDENT_SENSOR  0x40

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE	25

#define Test_flag 0   // add adaptive function
#define Print_reg 0	  // print reg
/*
 * The tp2815 i2c address
 */

#define I2C_ADDR  0x88

//TP28xx audio
//both record and playback are master, 20ch,I2S mode, backend can use 16ch mode to capture.
#define I2S  0
#define DSP  1
#define AUDIO_FORMAT   I2S

#define SAMPLE_8K    0
#define SAMPLE_16K   1
#define SAMPLE_RATE  SAMPLE_16K

#define DATA_16BIT  0
#define DATA_8BIT   1
#define DATA_BIT    DATA_16BIT

#define AUDIO_CHN   4

enum{
	CH_1 = 0,   //
	CH_2 = 1,   //
	CH_3 = 2,   //
	CH_4 = 3,   //
	CH_ALL = 4,   //
	MIPI_PAGE = 8,
	APAGE = 0x40,
};
enum{
	STD_TVI, //TVI
	STD_HDA, //AHD
};
enum{
	PAL,
	NTSC,
	HD25,  //720p25
	HD30,  //720p30
	FHD25, //1080p25
	FHD30, //1080p30
	FHD50, //1080p50
	FHD60, //1080p60
	QHD25, //2560x1440p25
	QHD30, //2560x1440p30
	UVGA25,  //1280x960p25, must use with MIPI_4CH4LANE_445M
	UVGA30,  //1280x960p30, must use with MIPI_4CH4LANE_445M
	HD30HDR, //special 720p30 with ISX019, must use with MIPI_4CH4LANE_396M
	HD50,    //720p50
	HD60,    //720p60
	A_UVGA30,  //HDA 1280x960p30, must use with MIPI_4CH4LANE_378M
	F_UVGA30,  //FH 1280x960p30, must use with MIPI_4CH4LANE_432M
	UVGA30_945, //TVI 1280x960p30, must use with MIPI_4CH4LANE_378M
	FHD275,   //1080p27.5
	FMT5M20,
	FMT5M12,
	FMT8M15,
	FMT8M12,
	FMT8M7,
	HD30864, //total 1600x900 86.4M
};
enum{
	MIPI_4CH4LANE_297M, //up to 4x720p25/30
	MIPI_4CH4LANE_594M, //up to 4x1080p25/30
	MIPI_4CH2LANE_594M, //up to 4x720pp25/30
	MIPI_4CH4LANE_445M, //only for 4x960p25/30
	MIPI_2CH4LANE_297M, //up to 2x1080p25/30
	MIPI_2CH4LANE_594M, //up to 2xQHDp25/30 or 2x1080p50/60
	MIPI_4CH4LANE_396M, //only for 4xHD30HDR
	MIPI_4CH4LANE_378M, //only for 4xA_UVGA30
	MIPI_4CH4LANE_432M, //only for 4xF_UVGA30
	MIPI_1CH2LANE_594M,
	MIPI_3CH4LANE_594M,
	MIPI_4CH4LANE_345M, //only for HD30864
};

/*static struct delayed_work sensor_s_ae_ratio_work;*/
#define SENSOR_NAME "tp2815_mipi"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];
//static struct sensor_format_struct *current_switch_win[2];

void tp2815_decoder_init(int id, unsigned char ch, unsigned char fmt, unsigned char std);
int tp2815_hardware_init(int id, unsigned char fmt);
void tp2815_mipi_out(int id, unsigned char output);

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};

/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {
};

static struct regval_list sensor_1080p_25fps_regs[] = {
};

int sensor_s_exp(int id, unsigned int exp_val)
{
	return 0;
}

int sensor_g_gain(int id, __s32 *value)
{
	return 0;
}

int sensor_s_gain(int id, int gain_val)
{
	return 0;
}

int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	return 0;
}

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
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);/*CSI_GPIO_HIGH*/
		hal_usleep(20000);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(10000);  //delay 10ms
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF\n");
		vin_set_mclk(id, 0);
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_set_status(id, RESET, CSI_GPIO_LOW);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_set_ir(int id, int status)
{
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
#endif
	return 0;
}

static int sensor_detect(int id)
{
	int i = 0;
	data_type rdval = 0;

	sensor_write(id, 0x40, 0x00);
	sensor_read(id, 0xff, &rdval);
	sensor_print("reg 0xff = 0x%x --N\n", rdval);
	sensor_read(id, 0xfe, &rdval);
	sensor_print("reg 0xfe = 0x%x --N\n", rdval);

	while ((rdval != 0x28) && (i < 5)) {
		sensor_read(id, 0x1E, &rdval);
		sensor_dbg("reg 0x%x = 0x%x\n", 0x1e, rdval);
		i++;
	}
	if (rdval != 0x28) {
		sensor_err("No tp2815 address 0x88 is connectted\n");
		//return -ENXIO;
	}

	return 0;
}


/////////////////////////////////
//ch: video channel
//fmt: PAL/NTSC/HD25/HD30...
//std: STD_TVI/STD_HDA
////////////////////////////////
void tp2815_decoder_init(int id, unsigned char ch, unsigned char fmt, unsigned char std)
{
	data_type tmp;
	const unsigned char SYS_MODE[5] = {0x01, 0x02, 0x04, 0x08, 0x0f};

	sensor_dbg("tp2815_decoder_init\n")

	sensor_write(id, 0x40, ch);
	sensor_write(id, 0x45, 0x01);
	sensor_write(id, 0x06, 0x12); //default value
	sensor_write(id, 0x27, 0x2d); //default value

	if (PAL == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp |= SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x06, 0x32);
		sensor_write(id, 0x02, 0x47);
		sensor_write(id, 0x07, 0x80);
		sensor_write(id, 0x0b, 0x80);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x51);
		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0xf0);
		sensor_write(id, 0x17, 0xa0);
		sensor_write(id, 0x18, 0x17);
		sensor_write(id, 0x19, 0x20);
		sensor_write(id, 0x1a, 0x15);
		sensor_write(id, 0x1c, 0x06);
		sensor_write(id, 0x1d, 0xc0);
		sensor_write(id, 0x20, 0x48);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x37);
		sensor_write(id, 0x23, 0x3f);
		sensor_write(id, 0x2a, 0x34); // add pattern mode
		sensor_write(id, 0x2b, 0x70);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x4b);
		sensor_write(id, 0x2e, 0x56);
		sensor_write(id, 0x30, 0x7a);
		sensor_write(id, 0x31, 0x4a);
		sensor_write(id, 0x32, 0x4d);
		sensor_write(id, 0x33, 0xfb);
		sensor_write(id, 0x35, 0x65);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x04);
	} else if (NTSC == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp |= SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x47);
		sensor_write(id, 0x07, 0x80);
		sensor_write(id, 0x0b, 0x80);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x50);
		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0xd6);
		sensor_write(id, 0x17, 0xa0);
		sensor_write(id, 0x18, 0x12);
		sensor_write(id, 0x19, 0xf0);
		sensor_write(id, 0x1a, 0x05);
		sensor_write(id, 0x1c, 0x06);
		sensor_write(id, 0x1d, 0xb4);
		sensor_write(id, 0x20, 0x40);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);
		sensor_write(id, 0x2a, 0x34); // add pattern mode
		sensor_write(id, 0x2b, 0x70);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x4b);
		sensor_write(id, 0x2e, 0x57);

		sensor_write(id, 0x30, 0x62);
		sensor_write(id, 0x31, 0xbb);
		sensor_write(id, 0x32, 0x96);
		sensor_write(id, 0x33, 0xcb);
		sensor_write(id, 0x35, 0x65);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x04);
	} else if (HD25 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp |= SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x42);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x50);
		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x15);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x19);
		sensor_write(id, 0x19, 0xd0);
		sensor_write(id, 0x1a, 0x25);
		sensor_write(id, 0x1c, 0x07);  //1280*720, 25fps
		sensor_write(id, 0x1d, 0xbc);  //1280*720, 25fps
		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x0a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xbb);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);
		sensor_write(id, 0x35, 0x25);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x18);

		if (STD_HDA == std) {
			sensor_write(id, 0x02, 0x46);

			sensor_write(id, 0x0d, 0x71);

			sensor_write(id, 0x18, 0x1b);

			sensor_write(id, 0x20, 0x40);
			sensor_write(id, 0x21, 0x46);

			sensor_write(id, 0x25, 0xfe);
			sensor_write(id, 0x26, 0x01);

			sensor_write(id, 0x2a, 0x34); // add pattern mode
			sensor_write(id, 0x2c, 0x3a);
			sensor_write(id, 0x2d, 0x5a);
			sensor_write(id, 0x2e, 0x40);

			sensor_write(id, 0x30, 0x9e);
			sensor_write(id, 0x31, 0x20);
			sensor_write(id, 0x32, 0x10);
			sensor_write(id, 0x33, 0x90);
		}
	} else if (HD30 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp |= SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x42);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x50);
		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x15);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x19);
		sensor_write(id, 0x19, 0xd0);
		sensor_write(id, 0x1a, 0x25);
		sensor_write(id, 0x1c, 0x06);  //1280*720, 30fps
		sensor_write(id, 0x1d, 0x72);  //1280*720, 30fps
		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xbb);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);
		sensor_write(id, 0x35, 0x25);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x18);

		if (STD_HDA == std) {
			sensor_write(id, 0x02, 0x46);

			sensor_write(id, 0x0d, 0x70);

			sensor_write(id, 0x18, 0x1b);

			sensor_write(id, 0x20, 0x40);
			sensor_write(id, 0x21, 0x46);

			sensor_write(id, 0x25, 0xfe);
			sensor_write(id, 0x26, 0x01);

			sensor_write(id, 0x2c, 0x3a);
			sensor_write(id, 0x2d, 0x5a);
			sensor_write(id, 0x2e, 0x40);

			sensor_write(id, 0x30, 0x9d);
			sensor_write(id, 0x31, 0xca);
			sensor_write(id, 0x32, 0x01);
			sensor_write(id, 0x33, 0xd0);
		}
	} else if (FHD30 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x40);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);
		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0xd2);
		sensor_write(id, 0x17, 0x80);
		sensor_write(id, 0x18, 0x29);
		sensor_write(id, 0x19, 0x38);
		sensor_write(id, 0x1a, 0x47);
		sensor_write(id, 0x1c, 0x08);  //1920*1080, 30fps
		sensor_write(id, 0x1d, 0x98);  //
		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xbb);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);
		sensor_write(id, 0x35, 0x05);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x1C);

		if (STD_HDA == std) {
			sensor_write(id, 0x02, 0x44);

			sensor_write(id, 0x0d, 0x72);

			sensor_write(id, 0x15, 0x01);
			sensor_write(id, 0x16, 0xf0);
			sensor_write(id, 0x18, 0x2a);

			sensor_write(id, 0x20, 0x38);
			sensor_write(id, 0x21, 0x46);

			sensor_write(id, 0x25, 0xfe);
			sensor_write(id, 0x26, 0x0d);

			sensor_write(id, 0x2c, 0x3a);
			sensor_write(id, 0x2d, 0x54);
			sensor_write(id, 0x2e, 0x40);

			sensor_write(id, 0x30, 0xa5);
			sensor_write(id, 0x31, 0x95);
			sensor_write(id, 0x32, 0xe0);
			sensor_write(id, 0x33, 0x60);
		}
	} else if (FHD25 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x40);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);
		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0xd2);
		sensor_write(id, 0x17, 0x80);
		sensor_write(id, 0x18, 0x29);
		sensor_write(id, 0x19, 0x38);
		sensor_write(id, 0x1a, 0x47);
		sensor_write(id, 0x1c, 0x0a);  //1920*1080, 25fps
		sensor_write(id, 0x1d, 0x50);  //
		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xbb);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);
		sensor_write(id, 0x35, 0x05);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x1C);

		if (STD_HDA == std) {
			sensor_write(id, 0x02, 0x44);
			sensor_write(id, 0x0d, 0x73);
			sensor_write(id, 0x15, 0x01);
			sensor_write(id, 0x16, 0xf0);
			sensor_write(id, 0x18, 0x2a);

			sensor_write(id, 0x20, 0x3c);
			sensor_write(id, 0x21, 0x46);
			sensor_write(id, 0x25, 0xfe);
			sensor_write(id, 0x26, 0x0d);
			sensor_write(id, 0x2a, 0x34); // add pattern mode
			sensor_write(id, 0x2c, 0x3a);
			sensor_write(id, 0x2d, 0x54);
			sensor_write(id, 0x2e, 0x40);
			sensor_write(id, 0x30, 0xa5);
			sensor_write(id, 0x31, 0x86);
			sensor_write(id, 0x32, 0xfb);
			sensor_write(id, 0x33, 0x60);
		}
	} else if (FHD275 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x40);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x88);
		sensor_write(id, 0x17, 0x80);
		sensor_write(id, 0x18, 0x29);
		sensor_write(id, 0x19, 0x38);
		sensor_write(id, 0x1a, 0x47);
		sensor_write(id, 0x1c, 0x09);  //1920*1080, 30fps
		sensor_write(id, 0x1d, 0x60);  //
		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xbb);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);
		sensor_write(id, 0x35, 0x05);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x1C);
	} else if (FHD60 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x40);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0xf0);
		sensor_write(id, 0x17, 0x80);
		sensor_write(id, 0x18, 0x12);
		sensor_write(id, 0x19, 0x38);
		sensor_write(id, 0x1a, 0x47);
		sensor_write(id, 0x1c, 0x08);  //
		sensor_write(id, 0x1d, 0x96);  //
		sensor_write(id, 0x20, 0x38);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);
		sensor_write(id, 0x27, 0xad);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x40);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x74);
		sensor_write(id, 0x31, 0x9b);
		sensor_write(id, 0x32, 0xa5);
		sensor_write(id, 0x33, 0xe0);
		sensor_write(id, 0x35, 0x05);
		sensor_write(id, 0x38, 0x40);
		sensor_write(id, 0x39, 0x68);
	} else if (FHD50 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x40);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);
		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0xe2);
		sensor_write(id, 0x17, 0x80);
		sensor_write(id, 0x18, 0x27);
		sensor_write(id, 0x19, 0x38);
		sensor_write(id, 0x1a, 0x47);
		sensor_write(id, 0x1c, 0x0a);  //
		sensor_write(id, 0x1d, 0x4e);  //
		sensor_write(id, 0x20, 0x38);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x27, 0xad);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x40);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x74);
		sensor_write(id, 0x31, 0x9b);
		sensor_write(id, 0x32, 0xa5);
		sensor_write(id, 0x33, 0xe0);
		sensor_write(id, 0x35, 0x05);
		sensor_write(id, 0x38, 0x40);
		sensor_write(id, 0x39, 0x68);
	} else if (QHD30 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x50);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x23);
		sensor_write(id, 0x16, 0x1b);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x38);
		sensor_write(id, 0x19, 0xa0);
		sensor_write(id, 0x1a, 0x5a);
		sensor_write(id, 0x1c, 0x0c);  //2560*1440, 30fps
		sensor_write(id, 0x1d, 0xe2);  //

		sensor_write(id, 0x20, 0x50);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x27, 0xad);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x58);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x74);
		sensor_write(id, 0x31, 0x58);
		sensor_write(id, 0x32, 0x9f);
		sensor_write(id, 0x33, 0x60);
		sensor_write(id, 0x35, 0x15);
		sensor_write(id, 0x36, 0xdc);
		sensor_write(id, 0x38, 0x40);
		sensor_write(id, 0x39, 0x48);

	} else if (QHD25 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x50);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x23);
		sensor_write(id, 0x16, 0x1b);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x38);
		sensor_write(id, 0x19, 0xa0);
		sensor_write(id, 0x1a, 0x5a);
		sensor_write(id, 0x1c, 0x0f);  //2560*1440, 25fps
		sensor_write(id, 0x1d, 0x76);  //

		sensor_write(id, 0x20, 0x50);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x27, 0xad);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x58);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x74);
		sensor_write(id, 0x31, 0x58);
		sensor_write(id, 0x32, 0x9f);
		sensor_write(id, 0x33, 0x60);

		sensor_write(id, 0x35, 0x15);
		sensor_write(id, 0x36, 0xdc);
		sensor_write(id, 0x38, 0x40);
		sensor_write(id, 0x39, 0x48);
	} else if (UVGA25 == fmt) { //960P25
		sensor_write(id, 0xf5, 0xf0);
		sensor_write(id, 0x02, 0x42);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x16);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0xa0);
		sensor_write(id, 0x19, 0xc0);
		sensor_write(id, 0x1a, 0x35);
		sensor_write(id, 0x1c, 0x07);  //
		sensor_write(id, 0x1d, 0xbc);  //

		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x26, 0x01);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xba);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);
		sensor_write(id, 0x35, 0x14);
		sensor_write(id, 0x36, 0x65);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x18);
	} else if (UVGA30 == fmt) { //960P30
		sensor_write(id, 0xf5, 0xf0);
		sensor_write(id, 0x02, 0x42);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x16);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0xa0);
		sensor_write(id, 0x19, 0xc0);
		sensor_write(id, 0x1a, 0x35);
		sensor_write(id, 0x1c, 0x06);  //
		sensor_write(id, 0x1d, 0x72);  //

		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x26, 0x01);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x43);
		sensor_write(id, 0x31, 0x3b);
		sensor_write(id, 0x32, 0x79);
		sensor_write(id, 0x33, 0x90);

		sensor_write(id, 0x35, 0x14);
		sensor_write(id, 0x36, 0x65);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x18);
	} else if (HD30HDR == fmt) {
		sensor_write(id, 0xf5, 0xf0);
		sensor_write(id, 0x02, 0x42);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x15);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x90);
		sensor_write(id, 0x19, 0xd0);
		sensor_write(id, 0x1a, 0x25);
		sensor_write(id, 0x1c, 0x06);
		sensor_write(id, 0x1d, 0x72);

		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xba);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);

		sensor_write(id, 0x35, 0x13);
		sensor_write(id, 0x36, 0xe8);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x18);
	} else if (HD50 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);

		sensor_write(id, 0x02, 0x42);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x15);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x19);
		sensor_write(id, 0x19, 0xd0);
		sensor_write(id, 0x1a, 0x25);
		sensor_write(id, 0x1c, 0x07);  //1280*720,
		sensor_write(id, 0x1d, 0xbc);  //1280*720, 50fps

		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xbb);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);

		sensor_write(id, 0x35, 0x05);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x1c);

		if (STD_HDA == std) { //subcarrier=24M
			sensor_write(id, 0x02, 0x46);
			sensor_write(id, 0x05, 0x01);
			sensor_write(id, 0x0d, 0x76);
			sensor_write(id, 0x0e, 0x0a);
			sensor_write(id, 0x14, 0x00);
			sensor_write(id, 0x15, 0x13);
			sensor_write(id, 0x16, 0x1a);
			sensor_write(id, 0x18, 0x1b);

			sensor_write(id, 0x20, 0x40);

			sensor_write(id, 0x26, 0x01);

			sensor_write(id, 0x2c, 0x3a);
			sensor_write(id, 0x2d, 0x54);
			sensor_write(id, 0x2e, 0x50);

			sensor_write(id, 0x30, 0xa5);
			sensor_write(id, 0x31, 0x9f);
			sensor_write(id, 0x32, 0xce);
			sensor_write(id, 0x33, 0x60);
		}
	} else if (HD60 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);

		sensor_write(id, 0x02, 0x42);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x15);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x19);
		sensor_write(id, 0x19, 0xd0);
		sensor_write(id, 0x1a, 0x25);
		sensor_write(id, 0x1c, 0x06);  //1280*720,
		sensor_write(id, 0x1d, 0x72);  //1280*720, 60fps

		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xbb);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);

		sensor_write(id, 0x35, 0x05);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x1c);

		if (STD_HDA == std) { //subcarrier=11M
			sensor_write(id, 0x02, 0x46);
			sensor_write(id, 0x05, 0xf9);
			sensor_write(id, 0x0d, 0x76);
			sensor_write(id, 0x0e, 0x03);
			sensor_write(id, 0x14, 0x00);
			sensor_write(id, 0x15, 0x13);
			sensor_write(id, 0x16, 0x41);
			sensor_write(id, 0x18, 0x1b);
			sensor_write(id, 0x20, 0x50);
			sensor_write(id, 0x21, 0x84);

			sensor_write(id, 0x25, 0xff);
			sensor_write(id, 0x26, 0x0d);

			sensor_write(id, 0x2c, 0x3a);
			sensor_write(id, 0x2d, 0x68);
			sensor_write(id, 0x2e, 0x60);

			sensor_write(id, 0x30, 0x4e);
			sensor_write(id, 0x31, 0xf8);
			sensor_write(id, 0x32, 0xdc);
			sensor_write(id, 0x33, 0xf0);
		}
	} else if (A_UVGA30 == fmt) { //HDA 960P30
		sensor_write(id, 0xf5, 0xf0);
		sensor_write(id, 0x02, 0x40);
		sensor_write(id, 0x05, 0x01);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x76);
		sensor_write(id, 0x0e, 0x12);

		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0x5f);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x9c);
		sensor_write(id, 0x19, 0xc0);
		sensor_write(id, 0x1a, 0x35);
		sensor_write(id, 0x1c, 0x85);  //
		sensor_write(id, 0x1d, 0x78);  //

		sensor_write(id, 0x20, 0x14);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x26, 0x0d);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x1e);
		sensor_write(id, 0x2e, 0x50);

		sensor_write(id, 0x30, 0x29);
		sensor_write(id, 0x31, 0x01);
		sensor_write(id, 0x32, 0x76);
		sensor_write(id, 0x33, 0x80);

		sensor_write(id, 0x35, 0x14);
		sensor_write(id, 0x36, 0x65);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x88);
	} else if (F_UVGA30 == fmt) { //FH 960P30
		sensor_write(id, 0xf5, 0xf0);
		sensor_write(id, 0x02, 0x4c);
		sensor_write(id, 0x05, 0xfd);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x76);
		sensor_write(id, 0x0e, 0x16);
		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x8f);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x23);
		sensor_write(id, 0x19, 0xc0);
		sensor_write(id, 0x1a, 0x35);
		sensor_write(id, 0x1c, 0x07);  //
		sensor_write(id, 0x1d, 0x08);  //

		sensor_write(id, 0x20, 0x60);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x26, 0x05);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x70);
		sensor_write(id, 0x2e, 0x50);

		sensor_write(id, 0x30, 0x7f);
		sensor_write(id, 0x31, 0x49);
		sensor_write(id, 0x32, 0xf4);
		sensor_write(id, 0x33, 0x90);
		sensor_write(id, 0x35, 0x13);
		sensor_write(id, 0x36, 0xe8);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x88);
	} else if (UVGA30_945 == fmt) { //1400x1125x30,94.5M
		sensor_write(id, 0xf5, 0xf0);
		sensor_write(id, 0x02, 0x40);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);
		sensor_write(id, 0x14, 0x00);
		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0x60);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0xa0);
		sensor_write(id, 0x19, 0xc0);
		sensor_write(id, 0x1a, 0x35);
		sensor_write(id, 0x1c, 0x05);  //1920*1080, 30fps
		sensor_write(id, 0x1d, 0x78);  //
		sensor_write(id, 0x20, 0x18);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);
		sensor_write(id, 0x26, 0x06);
		sensor_write(id, 0x27, 0x2d);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x1a);
		sensor_write(id, 0x2d, 0x1a);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xba);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);
		sensor_write(id, 0x35, 0x14);
		sensor_write(id, 0x36, 0x65);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x88);
	} else if (FMT5M20 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);

		sensor_write(id, 0x02, 0x50);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x23);
		sensor_write(id, 0x16, 0x36);
		sensor_write(id, 0x17, 0x20);
		sensor_write(id, 0x18, 0x1a);
		sensor_write(id, 0x19, 0x98);
		sensor_write(id, 0x1a, 0x7a);
		sensor_write(id, 0x1c, 0x0e);  //
		sensor_write(id, 0x1d, 0xa4);  //
		sensor_write(id, 0x20, 0x50);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x27, 0xad);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x54);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x74);
		sensor_write(id, 0x31, 0xa7);
		sensor_write(id, 0x32, 0x18);
		sensor_write(id, 0x33, 0x50);
		sensor_write(id, 0x35, 0x17);
		sensor_write(id, 0x36, 0xbc);
		sensor_write(id, 0x38, 0x40);
		sensor_write(id, 0x39, 0x48);
		if (STD_HDA == std) {
			sensor_write(id, 0x0d, 0x70);
			sensor_write(id, 0x0e, 0x0b);
			sensor_write(id, 0x1c, 0x8e);
			sensor_write(id, 0x20, 0x80);
			sensor_write(id, 0x21, 0x86);
			sensor_write(id, 0x2d, 0xa0);
			sensor_write(id, 0x2e, 0x40);
			sensor_write(id, 0x30, 0x48);
			sensor_write(id, 0x31, 0x77);
			sensor_write(id, 0x32, 0x0e);
			sensor_write(id, 0x33, 0xa0);
			sensor_write(id, 0x39, 0x68);
		}
	} else if (FMT5M12 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x40);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0x1f);
		sensor_write(id, 0x17, 0x20);
		sensor_write(id, 0x18, 0x34);
		sensor_write(id, 0x19, 0x98);
		sensor_write(id, 0x1a, 0x7a);
		sensor_write(id, 0x1c, 0x0b);  //
		sensor_write(id, 0x1d, 0x9a);  //
		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xbb);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);
		sensor_write(id, 0x35, 0x17);
		sensor_write(id, 0x36, 0xd0);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x1C);

		if (STD_HDA == std) {
			sensor_write(id, 0x02, 0x44);
			sensor_write(id, 0x05, 0x29);
			sensor_write(id, 0x0d, 0x76);
			sensor_write(id, 0x0e, 0x0f);
			sensor_write(id, 0x15, 0x73);
			sensor_write(id, 0x16, 0x5a);
			sensor_write(id, 0x17, 0x20);
			sensor_write(id, 0x18, 0x1a);
			sensor_write(id, 0x19, 0x98);
			sensor_write(id, 0x1a, 0x7a);
			sensor_write(id, 0x1c, 0x0b);
			sensor_write(id, 0x1d, 0xb8);
			sensor_write(id, 0x20, 0x38);
			sensor_write(id, 0x21, 0x46);
			sensor_write(id, 0x25, 0xfe);
			sensor_write(id, 0x26, 0x01);
			sensor_write(id, 0x2c, 0x2a);
			sensor_write(id, 0x2d, 0x62);
			sensor_write(id, 0x2e, 0x40);
			sensor_write(id, 0x30, 0xa5);
			sensor_write(id, 0x31, 0xa1);
			sensor_write(id, 0x32, 0xca);
			sensor_write(id, 0x33, 0xc0);
			sensor_write(id, 0x35, 0x17);
			sensor_write(id, 0x36, 0xbc);
		}
	} else if (FMT8M15 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);
		sensor_write(id, 0x02, 0x50);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);
		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0xbd);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x50);
		sensor_write(id, 0x19, 0x70);
		sensor_write(id, 0x1a, 0x8f);
		sensor_write(id, 0x1c, 0x11);  //3840*2160, 15fps
		sensor_write(id, 0x1d, 0x2e);  //
		sensor_write(id, 0x20, 0x60);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);
		sensor_write(id, 0x27, 0xad);
		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x54);
		sensor_write(id, 0x2e, 0x70);
		sensor_write(id, 0x30, 0x74);
		sensor_write(id, 0x31, 0x59);
		sensor_write(id, 0x32, 0xbd);
		sensor_write(id, 0x33, 0x60);
		sensor_write(id, 0x35, 0x18);
		sensor_write(id, 0x36, 0xca);
		sensor_write(id, 0x38, 0x40);
		sensor_write(id, 0x39, 0x68);

		if (STD_HDA == std) {
			sensor_write(id, 0x14, 0x40);
			sensor_write(id, 0x15, 0x13);
			sensor_write(id, 0x16, 0x74);
			sensor_write(id, 0x20, 0x80);
			sensor_write(id, 0x21, 0x86);
			sensor_write(id, 0x2d, 0x58);
			sensor_write(id, 0x2e, 0x40);
			sensor_write(id, 0x30, 0x48);
			sensor_write(id, 0x31, 0x68);
			sensor_write(id, 0x32, 0x43);
			sensor_write(id, 0x33, 0x00);
		}
	} else if (FMT8M12 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);

		sensor_write(id, 0x02, 0x50);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x13);
		sensor_write(id, 0x16, 0xbd);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0x50);
		sensor_write(id, 0x19, 0x70);
		sensor_write(id, 0x1a, 0x8f);
		sensor_write(id, 0x1c, 0x14);  //3840*2160, 12fps
		sensor_write(id, 0x1d, 0x9e);  //

		sensor_write(id, 0x20, 0x60);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x27, 0xad);

		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x02a);
		sensor_write(id, 0x2d, 0x54);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x74);
		sensor_write(id, 0x31, 0x59);
		sensor_write(id, 0x32, 0xbd);
		sensor_write(id, 0x33, 0x60);

		sensor_write(id, 0x35, 0x18);
		sensor_write(id, 0x36, 0xca);
		sensor_write(id, 0x38, 0x40);
		sensor_write(id, 0x39, 0x68);
	} else if (FMT8M7 == fmt) {
		sensor_read(id, 0xf5, &tmp);
		tmp &= ~SYS_MODE[ch];
		sensor_write(id, 0xf5, tmp);

		sensor_write(id, 0x02, 0x40);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x03);
		sensor_write(id, 0x0d, 0x50);
		sensor_write(id, 0x14, 0x40);
		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0xc0);
		sensor_write(id, 0x17, 0x80);
		sensor_write(id, 0x18, 0x50);
		sensor_write(id, 0x19, 0x70);
		sensor_write(id, 0x1a, 0x8f);
		sensor_write(id, 0x1c, 0x0f);  //3840*2160, 7fps
		sensor_write(id, 0x1d, 0xa0);  //
		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);

		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x38);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x24);
		sensor_write(id, 0x31, 0x34);
		sensor_write(id, 0x32, 0x21);
		sensor_write(id, 0x33, 0x80);
		sensor_write(id, 0x35, 0x19);
		sensor_write(id, 0x36, 0xab);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x1C);
	} else if (HD30864 == fmt) {
		sensor_write(id, 0xf5, 0xf0);
		sensor_write(id, 0x02, 0x42);
		sensor_write(id, 0x07, 0xc0);
		sensor_write(id, 0x0b, 0xc0);
		sensor_write(id, 0x0c, 0x13);
		sensor_write(id, 0x0d, 0x50);

		sensor_write(id, 0x15, 0x03);
		sensor_write(id, 0x16, 0xec);
		sensor_write(id, 0x17, 0x00);
		sensor_write(id, 0x18, 0xae);
		sensor_write(id, 0x19, 0xd0);
		sensor_write(id, 0x1a, 0x25);
		sensor_write(id, 0x1c, 0x06);
		sensor_write(id, 0x1d, 0x40);

		sensor_write(id, 0x20, 0x30);
		sensor_write(id, 0x21, 0x84);
		sensor_write(id, 0x22, 0x36);
		sensor_write(id, 0x23, 0x3c);
		sensor_write(id, 0x26, 0x05);
		sensor_write(id, 0x27, 0x2d);

		sensor_write(id, 0x2b, 0x60);
		sensor_write(id, 0x2c, 0x2a);
		sensor_write(id, 0x2d, 0x30);
		sensor_write(id, 0x2e, 0x70);

		sensor_write(id, 0x30, 0x48);
		sensor_write(id, 0x31, 0xba);
		sensor_write(id, 0x32, 0x2e);
		sensor_write(id, 0x33, 0x90);

		sensor_write(id, 0x35, 0x13);
		sensor_write(id, 0x36, 0x84);
		sensor_write(id, 0x38, 0x00);
		sensor_write(id, 0x39, 0x18);
	}
}

void tp2815_mipi_out(int id, unsigned char output)
{
	//mipi setting
	sensor_write(id, 0x40, MIPI_PAGE); //MIPI page
	sensor_write(id, 0x01, 0xf0);
	sensor_write(id, 0x02, 0x01);
	sensor_write(id, 0x08, 0x0f);
	sensor_dbg("tp2815_mipi_out\n");
	if (MIPI_4CH4LANE_594M == output || MIPI_2CH4LANE_594M == output) {
		sensor_write(id, 0x20, 0x44);
		if (MIPI_2CH4LANE_594M == output)
			sensor_write(id, 0x20, 0x24);
		sensor_write(id, 0x34, 0xe4); //
		sensor_write(id, 0x15, 0x0C);
		sensor_write(id, 0x25, 0x08);
		sensor_write(id, 0x26, 0x06);
		sensor_write(id, 0x27, 0x11);
		sensor_write(id, 0x29, 0x0a);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0x33);
		sensor_write(id, 0x14, 0xb3);
		sensor_write(id, 0x14, 0x33);
	}
	if (MIPI_3CH4LANE_594M == output) {
		sensor_write(id, 0x20, 0x34);
		sensor_write(id, 0x18, 0x8d); //VC0+VC1+VC
		sensor_write(id, 0x34, 0x8d); //VIN1+VIN2+VIN3
		sensor_write(id, 0x15, 0x0C);
		sensor_write(id, 0x25, 0x08);
		sensor_write(id, 0x26, 0x06);
		sensor_write(id, 0x27, 0x11);
		sensor_write(id, 0x29, 0x0a);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0x33);
		sensor_write(id, 0x14, 0xb3);
		sensor_write(id, 0x14, 0x33);
	} else if (MIPI_4CH4LANE_297M == output || MIPI_2CH4LANE_297M == output) {
		sensor_write(id, 0x20, 0x44);
		sensor_write(id, 0x20, 0x44);
		if (MIPI_2CH4LANE_297M == output)
			sensor_write(id, 0x20, 0x24);
		sensor_write(id, 0x34, 0xe4); //
		sensor_write(id, 0x14, 0x44);
		sensor_write(id, 0x15, 0x0d);
		sensor_write(id, 0x25, 0x04);
		sensor_write(id, 0x26, 0x03);
		sensor_write(id, 0x27, 0x09);
		sensor_write(id, 0x29, 0x02);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0xc4);
		sensor_write(id, 0x14, 0x44);
	} else if (MIPI_4CH2LANE_594M == output) {
		sensor_write(id, 0x20, 0x42);
		sensor_write(id, 0x34, 0xe4); //output vin1&vin2
		sensor_write(id, 0x15, 0x0c);
		sensor_write(id, 0x25, 0x08);
		sensor_write(id, 0x26, 0x06);
		sensor_write(id, 0x27, 0x11);
		sensor_write(id, 0x29, 0x0a);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0x43);
		sensor_write(id, 0x14, 0xc3);
		sensor_write(id, 0x14, 0x43);
	} else if (MIPI_4CH4LANE_445M == output) {//only for 4x960p25/30
		sensor_write(id, 0x20, 0x44);
		sensor_write(id, 0x34, 0xe4); //
		sensor_write(id, 0x12, 0x5f);
		sensor_write(id, 0x13, 0x07);
		sensor_write(id, 0x15, 0x0C);
		sensor_write(id, 0x25, 0x06);
		sensor_write(id, 0x26, 0x05);
		sensor_write(id, 0x27, 0x0d);
		sensor_write(id, 0x29, 0x0a);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0x33);
		sensor_write(id, 0x14, 0xb3);
		sensor_write(id, 0x14, 0x33);
	} else if (MIPI_4CH4LANE_396M == output) {//only for Sony ISX019
		sensor_write(id, 0x20, 0x44);
		sensor_write(id, 0x34, 0xe4); //
		sensor_write(id, 0x12, 0x6a);
		sensor_write(id, 0x13, 0x27);
		sensor_write(id, 0x15, 0x0C);
		sensor_write(id, 0x25, 0x06);
		sensor_write(id, 0x26, 0x04);
		sensor_write(id, 0x27, 0x0c);
		sensor_write(id, 0x29, 0x0a);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0x33);
		sensor_write(id, 0x14, 0xb3);
		sensor_write(id, 0x14, 0x33);
	} else if (MIPI_4CH4LANE_378M == output) {//only for 4xA_UVGA30
		sensor_write(id, 0x20, 0x44);
		sensor_write(id, 0x34, 0xe4); //
		sensor_write(id, 0x12, 0x5a);
		sensor_write(id, 0x13, 0x07);
		sensor_write(id, 0x15, 0x0C);
		sensor_write(id, 0x25, 0x06);
		sensor_write(id, 0x26, 0x04);
		sensor_write(id, 0x27, 0x0c);
		sensor_write(id, 0x29, 0x0a);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0x33);
		sensor_write(id, 0x14, 0xb3);
		sensor_write(id, 0x14, 0x33);
	} else if (MIPI_4CH4LANE_432M == output) {//only for 4xF_UVGA30
		sensor_write(id, 0x20, 0x44);
		sensor_write(id, 0x34, 0xe4); //
		sensor_write(id, 0x12, 0x5e);
		sensor_write(id, 0x13, 0x07);
		sensor_write(id, 0x15, 0x0C);
		sensor_write(id, 0x25, 0x06);
		sensor_write(id, 0x26, 0x05);
		sensor_write(id, 0x27, 0x0d);
		sensor_write(id, 0x29, 0x0a);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0x33);
		sensor_write(id, 0x14, 0xb3);
		sensor_write(id, 0x14, 0x33);
	} else if (MIPI_1CH2LANE_594M == output) {
		sensor_write(id, 0x20, 0x12);
		sensor_write(id, 0x34, 0x10); //output vin1&vin2
		sensor_write(id, 0x15, 0x0c);
		sensor_write(id, 0x25, 0x08);
		sensor_write(id, 0x26, 0x06);
		sensor_write(id, 0x27, 0x11);
		sensor_write(id, 0x29, 0x0a);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0x43);
		sensor_write(id, 0x14, 0xc3);
		sensor_write(id, 0x14, 0x43);
	}

	if (MIPI_4CH4LANE_345M == output) {
		sensor_write(id, 0x20, 0x44);
		sensor_write(id, 0x34, 0xe4);
		sensor_write(id, 0x12, 0x7e);
		sensor_write(id, 0x13, 0x67);
		sensor_write(id, 0x15, 0x0C);
		sensor_write(id, 0x25, 0x06);
		sensor_write(id, 0x26, 0x04);
		sensor_write(id, 0x27, 0x11);
		sensor_write(id, 0x29, 0x0e);
		sensor_write(id, 0x33, 0x07);
		sensor_write(id, 0x33, 0x00);
		sensor_write(id, 0x14, 0x33);
		sensor_write(id, 0x14, 0xb3);
		sensor_write(id, 0x14, 0x33);
	}
	/* Enable MIPI CSI2 output */
	sensor_write(id, 0x23, 0x02);
	sensor_write(id, 0x23, 0x00);
	sensor_write(id, 0x40, 0x04); //Decoder page
}

static int tp2815_audio_config_rmpos(int id, unsigned chip, unsigned format, unsigned chn_num)
{
	int i = 0;

	//clear first
	for (i = 0; i < 20; i++) {
		sensor_write(id, i, 0);
	}

	switch (chn_num) {
	case 2:
		if (format) {
			sensor_write(id, 0x0, 1);
			sensor_write(id, 0x1, 2);
		} else {
			sensor_write(id, 0x0, 1);
			sensor_write(id, 0x8, 2);
		}

		break;
	case 4:
		if (format) {
			sensor_write(id, 0x0, 1);
			sensor_write(id, 0x1, 2);
			sensor_write(id, 0x2, 3);
			sensor_write(id, 0x3, 4);/**/
		} else {
			sensor_write(id, 0x0, 1);
			sensor_write(id, 0x1, 2);
			sensor_write(id, 0x8, 3);
			sensor_write(id, 0x9, 4);/**/
		}
		break;
	case 8:
		if (0 == chip % 4) {
			if (format) {
				sensor_write(id, 0x0, 1);
				sensor_write(id, 0x1, 2);
				sensor_write(id, 0x2, 3);
				sensor_write(id, 0x3, 4);/**/
				sensor_write(id, 0x4, 5);
				sensor_write(id, 0x5, 6);
				sensor_write(id, 0x6, 7);
				sensor_write(id, 0x7, 8);/**/
			} else {
				sensor_write(id, 0x0, 1);
				sensor_write(id, 0x1, 2);
				sensor_write(id, 0x2, 3);
				sensor_write(id, 0x3, 4);/**/
				sensor_write(id, 0x8, 5);
				sensor_write(id, 0x9, 6);
				sensor_write(id, 0xa, 7);
				sensor_write(id, 0xb, 8);/**/
			}
		} else if (1 == chip % 4) {
			if (format) {
				sensor_write(id, 0x0, 0);
				sensor_write(id, 0x1, 0);
				sensor_write(id, 0x2, 0);
				sensor_write(id, 0x3, 0);
				sensor_write(id, 0x4, 1);
				sensor_write(id, 0x5, 2);
				sensor_write(id, 0x6, 3);
				sensor_write(id, 0x7, 4);/**/
			} else {
				sensor_write(id, 0x0, 0);
				sensor_write(id, 0x1, 0);
				sensor_write(id, 0x2, 1);
				sensor_write(id, 0x3, 2);
				sensor_write(id, 0x8, 0);
				sensor_write(id, 0x9, 0);
				sensor_write(id, 0xa, 3);
				sensor_write(id, 0xb, 4);/**/
			}
		}
		break;

	case 16:
		if (0 == chip % 4) {
			for (i = 0; i < 16; i++) {
				sensor_write(id, i, i + 1);
			}
		} else if (1 == chip % 4) {
			for (i = 4; i < 16; i++) {
				sensor_write(id, i, i + 1 -4);
			}
		} else if (2 == chip % 4) {
			for (i = 8; i < 16; i++) {
				sensor_write(id, i, i + 1 - 8);
			}
		} else {
			for (i = 12; i < 16; i++) {
				sensor_write(id, i, i + 1 - 12);
			}
		}
		break;

	case 20:
		for (i = 0; i < 20; i++) {
			sensor_write(id, i, i + 1);
		}
		break;

	default:
		for (i = 0; i < 20; i++) {
			sensor_write(id, i, i + 1);
		}
		break;
	}

	hal_usleep(10 * 1000);
	return 0;
}

void tp2815_audio_dataSet(int id, unsigned char chip)
{
	data_type tmp;

	sensor_read(id, 0x40, &tmp);
	sensor_write(id, 0x40, 0x40);

	tp2815_audio_config_rmpos(id, chip, AUDIO_FORMAT, AUDIO_CHN);

	sensor_write(id, 0x17, 0x00|(DATA_BIT<<2));
	sensor_write(id, 0x1B, 0x01|(DATA_BIT<<6));

#if (AUDIO_CHN == 20)
	sensor_write(id, 0x18, 0xd0|(SAMPLE_RATE));
#else
	sensor_write(id, 0x18, 0xc0|(SAMPLE_RATE));
#endif

#if (AUDIO_CHN >= 8)
	sensor_write(id, 0x19, 0x1F);
#else
	sensor_write(id, 0x19, 0x0F);
#endif
	sensor_write(id, 0x1A, 0x15);
	sensor_write(id, 0x37, 0x20);
	sensor_write(id, 0x38, 0x38);
	sensor_write(id, 0x3E, 0x00);
	sensor_write(id, 0x7a, 0x25);
	sensor_write(id, 0x3d, 0x01);//audio reset
	sensor_write(id, 0x40, tmp);
}

int  tp2815_hardware_init(int id, unsigned char fmt)
{
	int ret = -1;
	sensor_dbg("tp2815_hardware_init.\n");

	/* Disable MIPI CSI2 output */
	ret = sensor_write(id, 0x40, MIPI_PAGE);
	if (ret != 0) {
		sensor_err("Can't access tp2854.\n");
		return -ENODEV;
	}
	sensor_write(id, 0x23, 0x02);

	tp2815_decoder_init(id, CH_1, fmt, STD_HDA);
	tp2815_decoder_init(id, CH_2, fmt, STD_HDA);
	tp2815_decoder_init(id, CH_3, fmt, STD_HDA);
	tp2815_decoder_init(id, CH_4, fmt, STD_HDA);

	tp2815_mipi_out(id, MIPI_4CH4LANE_594M);

	sensor_write(id, 0x40, 0x0);
#if Print_reg
	int k;
	data_type tmp;
	for (k = 0; k < 255; ++k) {
		sensor_read(id, k, &tmp);
		sensor_print("rx_page 0x%x val=0x%x\n", k, tmp);
	}
#endif
	sensor_write(id, 0x40, MIPI_PAGE);

	sensor_write(id, 0x23, 0x00);

#if Print_reg
	for (k = 0; k < 255; ++k) {

		sensor_read(id, k, &tmp);
		sensor_print("mipi_page 0x%x val=0x%x\n", k, tmp);
	}
#endif
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
#if Test_flag

#define TVD_CH_MAX 4

int tp2815_init_ch_hardware(int id, struct tvin_init_info *info)
{
	int fmt;
	int ch;

	ch = info->ch_id;
	fmt = info->input_fmt[ch];

	if (ch >= TVD_CH_MAX)
		return -1;

	switch (fmt) {
	case CVBS_NTSC:
		tp2815_decoder_init(id, ch, NTSC, STD_HDA);
		break;
	case CVBS_PAL:
		tp2815_decoder_init(id, ch, PAL, STD_HDA);
		break;
	case AHD720P:
		tp2815_decoder_init(id, ch, HD25, STD_HDA);
		break;
	case AHD1080P:
		tp2815_decoder_init(id, ch, FHD25, STD_HDA);
		break;
	default:
		return -1;
	}

	sensor_print("Init ch successful!\r\n");
	return 0;
}

static int tp2815_get_output_fmt(int id, struct sensor_output_fmt *fmt)
{
	struct sensor_info *info = to_state(sd);
	__u32 *sensor_fmt = info->tvin.tvin_info.input_fmt;
	__u32 *out_feld = &fmt->field;
	__u32 ch_id = fmt->ch_id;

	if (ch_id >= TVD_CH_MAX)
		return -EINVAL;

	switch (sensor_fmt[ch_id]) {
	case CVBS_PAL:
	case CVBS_NTSC:
		/*Interlace ouput set out_feld as 1*/
		*out_feld = 1;
		break;
	case AHD720P:
	case AHD1080P:
		/*Progressive ouput set out_feld as 0*/
		*out_feld = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void tp2815_set_input_size(struct sensor_info *info,
		struct v4l2_subdev_format *fmt, int ch_id)
{
	struct tvin_init_info *tvin_info = &info->tvin.tvin_info;

	switch (tvin_info->input_fmt[ch_id]) {
	case AHD720P:
		fmt->format.width = 1280;
		fmt->format.height = 720;
		break;
	case AHD1080P:
		fmt->format.width = 1920;
		fmt->format.height = 1080;
		break;
	case CVBS_PAL:
		fmt->format.width = 720;
		fmt->format.height = 576;
		break;
	case CVBS_NTSC:
		fmt->format.width = 720;
		fmt->format.height = 480;
		break;
	default:
		break;
	}
}

int tp2815_sensor_set_fmt(int id, struct v4l2_subdev_state *state, struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
	int ret = 0;

	if (fmt->format.width == 1440 || fmt->format.width == 720)  // NTSC/PAL
		info->sensor_field = V4L2_FIELD_INTERLACED;
	else
		info->sensor_field = V4L2_FIELD_NONE;

	if (!info->tvin.flag)
		return sensor_set_fmt(sd, state, fmt);

	if (sd->entity.stream_count == 0) {
		tp2815_set_input_size(info, fmt, fmt->reserved[0]);
		ret = sensor_set_fmt(sd, state, fmt);
		sensor_print("%s befor ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	} else {
		ret = sensor_set_fmt(sd, state, fmt);
		tp2815_set_input_size(info, fmt, fmt->reserved[0]);
		sensor_print("%s after ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	}

	return ret;
}

static int sensor_tvin_init(int id, struct tvin_init_info *tvin_info)
{
	struct sensor_info *info = to_state(sd);
	__u32 *sensor_fmt = info->tvin.tvin_info.input_fmt;
	__u32 ch_id = tvin_info->ch_id;

	sensor_print("set ch%d fmt as %d\n", ch_id, tvin_info->input_fmt[ch_id]);
	sensor_fmt[ch_id] = tvin_info->input_fmt[ch_id];
	info->tvin.tvin_info.ch_id = ch_id;

	if (sd->entity.stream_count != 0) {
		tp2815_init_ch_hardware(sd, &info->tvin.tvin_info);
		sensor_print("sensor_tvin_init tp2815_init_ch_hardware\n");
	}

	info->tvin.flag = true;

	return 0;
}
#endif

static struct sensor_format_struct sensor_formats[] = {
	{
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
	.width = 720,
	.height = 480,
	.hoffset = 0,
	.voffset = 0,
	.pclk = 150*1000*1000,
	.mipi_bps = 600*1000*1000,
	.fps_fixed = 25,
	.switch_regs = sensor_default_regs,
	.switch_regs_size = ARRAY_SIZE(sensor_default_regs),
	},
	{
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
	.width = 720,
	.height = 576,
	.hoffset = 0,
	.voffset = 0,
	.pclk = 150*1000*1000,
	.mipi_bps = 600*1000*1000,
	.fps_fixed = 25,
	.switch_regs = sensor_default_regs,
	.switch_regs_size = ARRAY_SIZE(sensor_default_regs),
	},
	{
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
	.width = 1280,
	.height = 720,
	.hoffset = 0,
	.voffset = 0,
	.pclk = 150*1000*1000,
	.mipi_bps = 600*1000*1000,
	.fps_fixed = 25,
	.switch_regs = sensor_default_regs,
	.switch_regs_size = ARRAY_SIZE(sensor_default_regs),
	},
	{
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
	.width = 1920,
	.height = 1080,
	.hoffset = 0,
	.voffset = 0,
	.pclk = 594 * 1000 * 1000,
	.mipi_bps = 1188 * 1000 * 1000,
	.fps_fixed = 25,
	.switch_regs = sensor_1080p_25fps_regs,
	.switch_regs_size = ARRAY_SIZE(sensor_1080p_25fps_regs),
	},
};

static struct sensor_format_struct *sensor_get_format(int id, int isp_id, int vinc_id)
{
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);
	struct sensor_format_struct *sensor_format = NULL;
	int wdr_on = isp_get_cfg[ispid].sensor_wdr_on;
	int fps = isp_get_cfg[ispid].sensor_get_fps;
	int i;
	int best_dist = INT_MAX;

	if (current_win[id])
		return current_win[id];

	for (i = 0; i < ARRAY_SIZE(sensor_formats); i++) {
		if (sensor_formats[i].wdr_mode == wdr_on) {
			if (sensor_formats[i].fps_fixed == fps) {
				sensor_format = &sensor_formats[i];
				sensor_dbg("fine wdr is %d, fine fps is %d\n", wdr_on, fps);
				goto done;
			}
		}
	}

	if (sensor_format == NULL) {
		for (i = 0; i < ARRAY_SIZE(sensor_formats); i++) {
			if (sensor_formats[i].wdr_mode != ISP_NORMAL_MODE && sensor_formats[i].wdr_mode == wdr_on) {
				sensor_format = &sensor_formats[i];
				isp_get_cfg[ispid].sensor_get_fps = sensor_format->fps_fixed;
				sensor_dbg("fine wdr is %d, use fps is %d\n", wdr_on, sensor_format->fps_fixed);
				goto done;
			}
		}
	}

	if (sensor_format == NULL) {
		for (i = 0; i < ARRAY_SIZE(sensor_formats); i++) {
			int dist = abs(sensor_formats[i].width - global_video[vinc_id].o_width)
				+ abs(sensor_formats[i].height - global_video[vinc_id].o_height);
				if ((dist < best_dist) &&
				    (sensor_formats[i].width >= global_video[vinc_id].o_width) &&
				    (sensor_formats[i].height >= global_video[vinc_id].o_height) &&
				    (sensor_formats[i].wdr_mode == ISP_NORMAL_MODE)) {
					best_dist = dist;
					sensor_format = &sensor_formats[i];
				}
		}

		sensor_dbg("use wdr is %d, use fps is %d, width:%d, height:%d\n", sensor_format->wdr_mode, sensor_format->fps_fixed, sensor_format->width, sensor_format->height);
	}

done:
	current_win[id] = sensor_format;
	return sensor_format;
}

/*
static struct sensor_format_struct switch_sensor_formats[] = {
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	{
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
	.width = 720,
	.height = 480,
	.hoffset = 0,
	.voffset = 0,
	.pclk = 150*1000*1000,
	.mipi_bps = 600*1000*1000,
	.fps_fixed = 25,
	.switch_regs = sensor_default_regs,
	.switch_regs_size = ARRAY_SIZE(sensor_default_regs),
	},
	{
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
	.width = 720,
	.height = 576,
	.hoffset = 0,
	.voffset = 0,
	.pclk = 150*1000*1000,
	.mipi_bps = 600*1000*1000,
	.fps_fixed = 25,
	.regs = sensor_default_regs,
	.switch_regs = ARRAY_SIZE(sensor_default_regs),
	.switch_regs_size = NULL,
	},
	{
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
	.width = 1280,
	.height = 720,
	.hoffset = 0,
	.voffset = 0,
	.pclk = 150*1000*1000,
	.mipi_bps = 600*1000*1000,
	.fps_fixed = 25,
	.switch_regs = sensor_default_regs,
	.switch_regs_size = ARRAY_SIZE(sensor_default_regs),
	},
	{
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
	.width = 1920,
	.height = 1080,
	.hoffset = 0,
	.voffset = 0,
	.pclk = 594 * 1000 * 1000,
	.mipi_bps = 1188 * 1000 * 1000,
	.fps_fixed = 25,
	.switch_regs = sensor_1080p_25fps_regs,
	.switch_regs_size = ARRAY_SIZE(sensor_1080p_25fps_regs),
	},
#endif
};
*/

static struct sensor_format_struct *sensor_get_switch_format(int id, int isp_id, int vinc_id)
{
	return 0;
/*
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
	sensor_wdr_mode[id] = sensor_format->wdr_mode;
	current_switch_win[id] = sensor_format;
	return sensor_format;
#else
	return NULL;
#endif
*/
}

static int sensor_g_mbus_config(int id, struct v4l2_mbus_config *cfg, struct mbus_framefmt_res *res)
{
	cfg->type = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1 | V4L2_MBUS_CSI2_CHANNEL_2 | V4L2_MBUS_CSI2_CHANNEL_3;

	res->res_time_hs = 0x10;
	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{
	int ret = -1;
	//struct sensor_exp_gain exp_gain;

	sensor_dbg("ARRAY_SIZE(sensor_default_regs)=%d\n",
			(unsigned int)ARRAY_SIZE(sensor_default_regs));

	ret = sensor_write_array(id, sensor_default_regs,
					ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	if (current_win[id]->switch_regs) {
		sensor_dbg("%s, line:%d\n", __func__, __LINE__);
		if (current_win[id]->width == 1920 && current_win[id]->height == 1080) {
			ret = tp2815_hardware_init(id, FHD25);
		} else if (current_win[id]->width == 1280 && current_win[id]->height == 720) {
			ret = tp2815_hardware_init(id, HD25);
		} else if (current_win[id]->width == 720 && current_win[id]->height == 576) {
			ret = tp2815_hardware_init(id, PAL);
		} else {
			ret = tp2815_hardware_init(id, NTSC);
		}
		sensor_dbg("%s, line:%d\n", __func__, __LINE__);
	}
	if (ret < 0)
		return ret;


	return 0;
}

static int sensor_s_stream(int id, int isp_id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

	sensor_dbg("%s on = %d, %d*%d %d\n", __func__, enable,
		  current_win[id]->width,
		  current_win[id]->height, current_win[id]->fps_fixed);

	if (!enable)
		return 0;

	return sensor_reg_init(id, isp_id);
}

static int sensor_s_switch(int id)
{
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	// struct sensor_exp_gain exp_gain;
	// int ret = -1;

	// full_size = 1;
	// gc4663_sensor_vts = current_switch_win[id]->vts;

	// sensor_write(id, 0x0100, 0x00);
	// if (glb_exp_gain.exp_val && glb_exp_gain.gain_val) {
	// 	exp_gain.exp_val = glb_exp_gain.exp_val;
	// 	exp_gain.exp_mid_val = glb_exp_gain.exp_mid_val;
	// 	exp_gain.gain_val = glb_exp_gain.gain_val;
	// } else {
	// 	exp_gain.exp_val = 15408;
	// 	exp_gain.exp_mid_val = 16;
	// 	exp_gain.gain_val = 32;
	// }
	// //sensor_s_exp_gain(id, &exp_gain); /* make switch_regs exp&gain normal */

	// if (current_switch_win[id]->switch_regs)
	// 	ret = sensor_write_array(id, current_switch_win[id]->switch_regs, current_switch_win[id]->switch_regs_size);
	// else
	// 	sensor_err("cannot find 720p120fps to 2k%dfps reg\n", current_switch_win[id]->fps_fixed);
	// if (ret < 0)
	// 	return ret;
	// sensor_write(id, 0x0451, 0x00);/* RGAIN */
	// sensor_write(id, 0x0452, 0x00);/* BGAIN */
	// sensor_s_exp_gain(id, &exp_gain); /* make switch_regs firstframe  */
	// sensor_write(id, 0x03fe, 0x10);
	// sensor_write(id, 0x03fe, 0x00);
	// sensor_write(id, 0x0100, 0x09); /* stream_on */
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

struct sensor_fuc_core tp2815_core  = {
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
