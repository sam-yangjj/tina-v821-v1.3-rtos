
/*
 * A V4L2 driver for N5 YUV cameras.
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Hong Yi <hongyi@allwinnertech.com>
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
#define V4L2_IDENT_SENSOR 0xe0

static unsigned int CLK_POL  = V4L2_MBUS_PCLK_SAMPLE_FALLING;  //V4L2_MBUS_PCLK_SAMPLE_FALLING | V4L2_MBUS_PCLK_SAMPLE_RISING

#define BT656   0
#define BT1120  1

typedef enum _n5_outmode_sel {
	N5_OUTMODE_1MUX_SD = 0,
	N5_OUTMODE_1MUX_HD,
	N5_OUTMODE_1MUX_FHD,
	N5_OUTMODE_1MUX_FHD_HALF,
	N5_OUTMODE_2MUX_SD,
	N5_OUTMODE_2MUX_HD,
	N5_OUTMODE_2MUX_FHD,
	N5_OUTMODE_1MUX_BT1120S,
	N5_OUTMODE_2MUX_BT1120S_720P,
	N5_OUTMODE_2MUX_BT1120S_1080P,
	N5_OUTMODE_BUTT
} N5_OUTMODE_SEL;

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 25

/*
 * The N5 sits on i2c with ID 0x64 or 0x66
 * SAD-low:0x64 SAD-high:0x66
 */
#define I2C_ADDR 0x64
#define SENSOR_NAME "n5"

static int sensor_power_count[2];
static int sensor_stream_count[2];
static struct sensor_format_struct *current_win[2];
//static struct sensor_format_struct *current_switch_win[2];


void n5_regs_init_common(int id)
{
	sensor_write(id, 0xff, 0x00);
	sensor_write(id, 0x00, 0x10);
	sensor_write(id, 0x01, 0x10);
	sensor_write(id, 0x18, 0x3f);
	sensor_write(id, 0x19, 0x3f);
	sensor_write(id, 0x22, 0x0b);
	sensor_write(id, 0x23, 0x71);
	sensor_write(id, 0x26, 0x0b);
	sensor_write(id, 0x27, 0x71);
	sensor_write(id, 0x54, 0x00);
	sensor_write(id, 0xa0, 0x05);
	sensor_write(id, 0xa1, 0x05);
	sensor_write(id, 0xff, 0x05);
	sensor_write(id, 0x00, 0xf0);
	sensor_write(id, 0x01, 0x22);
	sensor_write(id, 0x05, 0x04);
	sensor_write(id, 0x08, 0x55);
	sensor_write(id, 0x1b, 0x08);
	sensor_write(id, 0x25, 0xdc);
	sensor_write(id, 0x28, 0x80);
	sensor_write(id, 0x2f, 0x00);
	sensor_write(id, 0x30, 0xe0);
	sensor_write(id, 0x31, 0x43);
	sensor_write(id, 0x32, 0xa2);
	sensor_write(id, 0x57, 0x00);
	sensor_write(id, 0x58, 0x77);
	sensor_write(id, 0x5b, 0x41);
	sensor_write(id, 0x5c, 0x78);
	sensor_write(id, 0x5f, 0x00);
	sensor_write(id, 0x7b, 0x11);
	sensor_write(id, 0x7c, 0x01);
	sensor_write(id, 0x7d, 0x80);
	sensor_write(id, 0x80, 0x00);
	sensor_write(id, 0x90, 0x01);
	sensor_write(id, 0xa9, 0x00);
	sensor_write(id, 0xb5, 0x00);
	sensor_write(id, 0xb9, 0x72);
	sensor_write(id, 0xd1, 0x00);
	sensor_write(id, 0xd5, 0x80);
	sensor_write(id, 0xff, 0x06);
	sensor_write(id, 0x00, 0xf0);
	sensor_write(id, 0x01, 0x22);
	sensor_write(id, 0x05, 0x04);
	sensor_write(id, 0x08, 0x55);
	sensor_write(id, 0x1b, 0x08);
	sensor_write(id, 0x25, 0xdc);
	sensor_write(id, 0x28, 0x80);
	sensor_write(id, 0x2f, 0x00);
	sensor_write(id, 0x30, 0xe0);
	sensor_write(id, 0x31, 0x43);
	sensor_write(id, 0x32, 0xa2);
	sensor_write(id, 0x57, 0x00);
	sensor_write(id, 0x58, 0x77);
	sensor_write(id, 0x5b, 0x41);
	sensor_write(id, 0x5c, 0x78);
	sensor_write(id, 0x5f, 0x00);
	sensor_write(id, 0x7b, 0x11);
	sensor_write(id, 0x7c, 0x01);
	sensor_write(id, 0x7d, 0x80);
	sensor_write(id, 0x80, 0x00);
	sensor_write(id, 0x90, 0x01);
	sensor_write(id, 0xa9, 0x00);
	sensor_write(id, 0xb5, 0x00);
	sensor_write(id, 0xb9, 0x72);
	sensor_write(id, 0xd1, 0x00);
	sensor_write(id, 0xd5, 0x80);
	sensor_write(id, 0xff, 0x09);
	sensor_write(id, 0x50, 0x30);
	sensor_write(id, 0x51, 0x6f);
	sensor_write(id, 0x52, 0x67);
	sensor_write(id, 0x53, 0x48);
	sensor_write(id, 0x54, 0x30);
	sensor_write(id, 0x55, 0x6f);
	sensor_write(id, 0x56, 0x67);
	sensor_write(id, 0x57, 0x48);
	sensor_write(id, 0x96, 0x00);
	sensor_write(id, 0x9e, 0x00);
	sensor_write(id, 0xb6, 0x00);
	sensor_write(id, 0xbe, 0x00);
	sensor_write(id, 0xff, 0x0a);
	sensor_write(id, 0x25, 0x10);
	sensor_write(id, 0x27, 0x1e);
	sensor_write(id, 0x30, 0xac);
	sensor_write(id, 0x31, 0x78);
	sensor_write(id, 0x32, 0x17);
	sensor_write(id, 0x33, 0xc1);
	sensor_write(id, 0x34, 0x40);
	sensor_write(id, 0x35, 0x00);
	sensor_write(id, 0x36, 0xc3);
	sensor_write(id, 0x37, 0x0a);
	sensor_write(id, 0x38, 0x00);
	sensor_write(id, 0x39, 0x02);
	sensor_write(id, 0x3a, 0x00);
	sensor_write(id, 0x3b, 0xb2);
	sensor_write(id, 0xa5, 0x10);
	sensor_write(id, 0xa7, 0x1e);
	sensor_write(id, 0xb0, 0xac);
	sensor_write(id, 0xb1, 0x78);
	sensor_write(id, 0xb2, 0x17);
	sensor_write(id, 0xb3, 0xc1);
	sensor_write(id, 0xb4, 0x40);
	sensor_write(id, 0xb5, 0x00);
	sensor_write(id, 0xb6, 0xc3);
	sensor_write(id, 0xb7, 0x0a);
	sensor_write(id, 0xb8, 0x00);
	sensor_write(id, 0xb9, 0x02);
	sensor_write(id, 0xba, 0x00);
	sensor_write(id, 0xbb, 0xb2);
	sensor_write(id, 0x77, 0x8F);
	sensor_write(id, 0xF7, 0x8F);
	sensor_write(id, 0xff, 0x13);
	sensor_write(id, 0x07, 0x47);
	sensor_write(id, 0x12, 0x04);
	sensor_write(id, 0x1e, 0x1f);
	sensor_write(id, 0x1f, 0x27);
	sensor_write(id, 0x2e, 0x10);
	sensor_write(id, 0x2f, 0xc8);
	sensor_write(id, 0x30, 0x00);
	sensor_write(id, 0x31, 0xff);
	sensor_write(id, 0x32, 0x00);
	sensor_write(id, 0x33, 0x00);
	sensor_write(id, 0x3a, 0xff);
	sensor_write(id, 0x3b, 0xff);
	sensor_write(id, 0x3c, 0xff);
	sensor_write(id, 0x3d, 0xff);
	sensor_write(id, 0x3e, 0xff);
	sensor_write(id, 0x3f, 0x0f);
	sensor_write(id, 0x70, 0x00);
	sensor_write(id, 0x72, 0x05);
	sensor_write(id, 0x7A, 0xf0);
	sensor_write(id, 0xff, 0x01);
	sensor_write(id, 0x97, 0x00);
	sensor_write(id, 0x97, 0x0f);
	sensor_write(id, 0x7A, 0x0f);
	sensor_write(id, 0xff, 0x00); //8x8 color block test pattern
	sensor_write(id, 0x78, 0x22); //0xba -> 0x22
	sensor_write(id, 0xff, 0x05);
	sensor_write(id, 0x2c, 0x08);
	sensor_write(id, 0x6a, 0x80);
	sensor_write(id, 0xff, 0x06);
	sensor_write(id, 0x2c, 0x08);
	sensor_write(id, 0x6a, 0x80);
}

void n5_set_chn_720p_25(int id, unsigned char chn)
{
	data_type val;

	sensor_print("%s chn=%d\n", __func__, chn);

	sensor_write(id, 0xff, 0x00);
	sensor_write(id, 0x08 + chn, 0x00);
	sensor_write(id, 0x34 + chn, 0x00);
	sensor_write(id, 0x81 + chn, 0x07);
	sensor_write(id, 0x85 + chn, 0x00);
	sensor_read(id, 0x54, &val);
	sensor_write(id, 0x54, val&(~(0x10<<chn)));
	sensor_write(id, 0x18 + chn, 0x3f);
	sensor_write(id, 0x58 + chn, 0x74);
	sensor_write(id, 0x5c + chn, 0x80);
	sensor_write(id, 0x64 + chn, 0x01);
	sensor_write(id, 0x89 + chn, 0x00);
	sensor_write(id, chn + 0x8e, 0x00);
	sensor_write(id, 0x30 + chn, 0x12);
	sensor_write(id, 0xa0 + chn, 0x05);

	sensor_write(id, 0xff, 0x01);
	sensor_write(id, 0x84 + chn, 0x08);
	sensor_write(id, 0x8c + chn, 0x08);
	sensor_read(id, 0xed, &val);
	sensor_write(id, 0xed, val & (~(0x01 << chn)));

	sensor_write(id, 0xff, 0x05+chn);
	sensor_write(id, 0x20, 0x84);
	sensor_write(id, 0x25, 0xdc);
	sensor_write(id, 0x28, 0x80);
	sensor_write(id, 0x2b, 0xa8);
	sensor_write(id, 0x47, 0x04);
	sensor_write(id, 0x50, 0x84);
	sensor_write(id, 0x58, 0x70);  // 0x70
	sensor_write(id, 0x69, 0x00);
	sensor_write(id, 0x7b, 0x11);
	sensor_write(id, 0xb8, 0xb9);

}

void n5_set_chn_1080p_25(int id, unsigned char chn)
{
	data_type val;

	sensor_dbg("%s chn=%d\n", __func__, chn);

	sensor_write(id, 0xff, 0x00);
	sensor_write(id, 0x08 + chn, 0x00);
	sensor_write(id, 0x34 + chn, 0x00);
	sensor_write(id, 0x81 + chn, 0x03);
	sensor_write(id, 0x85 + chn, 0x00);
	sensor_read(id, 0x54, &val);
	sensor_write(id, 0x54, val & (~(0x10 << chn)));
	sensor_write(id, 0x18 + chn, 0x3f);
	sensor_write(id, 0x58 + chn, 0x78);
	sensor_write(id, 0x5c + chn, 0x80);
	sensor_write(id, 0x64 + chn, 0x01);
	sensor_write(id, 0x89 + chn, 0x10);
	sensor_write(id, chn + 0x8e, 0x00);
	sensor_write(id, 0x30 + chn, 0x12);
	sensor_write(id, 0xa0 + chn, 0x05);

	sensor_write(id, 0xff, 0x01);
	sensor_write(id, 0x84 + chn, 0x00);
	sensor_write(id, 0x8c + chn, 0x40);
	sensor_read(id, 0xed, &val);
	sensor_write(id, 0xed, val & (~(0x01 << chn)));

	sensor_write(id, 0xff, 0x05 + chn);
	sensor_write(id, 0x20, 0x84);
	sensor_write(id, 0x25, 0xdc);
	sensor_write(id, 0x28, 0x80);
	sensor_write(id, 0x2b, 0xa8);
	sensor_write(id, 0x47, 0x04);
	sensor_write(id, 0x50, 0x84);
	sensor_write(id, 0x58, 0x77);
	sensor_write(id, 0x69, 0x00);
	sensor_write(id, 0x7b, 0x11);
	sensor_write(id, 0xb8, 0x39);
}

void n5_set_chn_1080p_30(int id, unsigned char chn)
{
	data_type val;

	sensor_dbg("%s chn=%d\n", __func__, chn);

	sensor_write(id, 0xff, 0x00);
	sensor_write(id, 0x08 + chn, 0x00);
	sensor_write(id, 0x34 + chn, 0x00);
	sensor_write(id, 0x81 + chn, 0x02);
	sensor_write(id, 0x85 + chn, 0x00);
	sensor_read(id, 0x54, &val);
	sensor_write(id, 0x54, val & (~(0x10 << chn)));
	sensor_write(id, 0x18 + chn, 0x3f);
	sensor_write(id, 0x58 + chn, 0x78);
	sensor_write(id, 0x5c + chn, 0x80);
	sensor_write(id, 0x64 + chn, 0x01);
	sensor_write(id, 0x89 + chn, 0x10);
	sensor_write(id, chn + 0x8e, 0x00);
	sensor_write(id, 0x30 + chn, 0x12);
	sensor_write(id, 0xa0 + chn, 0x05);

	sensor_write(id, 0xff, 0x01);
	sensor_write(id, 0x84 + chn, 0x00);
	sensor_write(id, 0x8c + chn, 0x40);
	sensor_read(id, 0xed, &val);
	sensor_write(id, 0xed, val & (~(0x10 << chn)));

	sensor_write(id, 0xff, 0x05 + chn);
	sensor_write(id, 0x20, 0x84);
	sensor_write(id, 0x25, 0xdc);
	sensor_write(id, 0x28, 0x80);
	sensor_write(id, 0x2b, 0xa8);
	sensor_write(id, 0x47, 0xee);
	sensor_write(id, 0x50, 0xc6);
	sensor_write(id, 0x58, 0x77);
	sensor_write(id, 0x69, 0x00);
	sensor_write(id, 0x7b, 0x11);
	sensor_write(id, 0xb8, 0x39);
}

void n5_set_chn_720h_pal(int id, unsigned char chn)
{
	data_type val;
	sensor_dbg("%s chn=%d\n", __func__, chn);
	sensor_write(id, 0xff, 0x00);
	sensor_write(id, 0x08 + chn, 0xdd);
	sensor_write(id, 0x34 + chn, 0x00);
	sensor_write(id, 0x81 + chn, 0x70);
	sensor_write(id, 0x85 + chn, 0x00);

	sensor_read(id, 0x54, &val);
	sensor_write(id, 0x54, val & (~(0x10 << chn)));
	sensor_write(id, 0x18 + chn, 0x08);
	sensor_write(id, 0x58 + chn, 0x80);
	sensor_write(id, 0x5c + chn, 0xbe);
	sensor_write(id, 0x64 + chn, 0xa0);
	sensor_write(id, 0x89 + chn, 0x10);
	sensor_write(id, chn + 0x8e, 0x2e);
	sensor_write(id, 0x30 + chn, 0x12);
	sensor_write(id, 0xa0 + chn, 0x05);

	sensor_write(id, 0xff, 0x01);
	sensor_write(id, 0x84 + chn, 0x06);
	sensor_write(id, 0x8c + chn, 0x86);
	sensor_read(id, 0xed, &val);
	sensor_write(id, 0xed, val & (~(0x01 << chn)));

	sensor_write(id, 0xff, 0x05 + chn);
	sensor_write(id, 0x20, 0x90);
	sensor_write(id, 0x25, 0xcc);
	sensor_write(id, 0x28, 0x80);
	sensor_write(id, 0x2b, 0x78);
	sensor_write(id, 0x47, 0x04);
	sensor_write(id, 0x50, 0x84);
	sensor_write(id, 0x58, 0x70);
	sensor_write(id, 0x69, 0x00);
	sensor_write(id, 0x7b, 0x00);
	sensor_write(id, 0xb8, 0xb9);
}

void n5_set_chn_720h_ntsc(int id, unsigned char chn)
{
	data_type val;

	sensor_write(id, 0xff, 0x00);
	sensor_write(id, 0x08 + chn, 0xa0);
	sensor_write(id, 0x34 + chn, 0x00);
	sensor_write(id, 0x81 + chn, 0x60);
	sensor_write(id, 0x85 + chn, 0x00);

	sensor_read(id, 0x54, &val);
	sensor_write(id, 0x54, val | (0x10 << chn));

	sensor_write(id, 0x18 + chn, 0x08);
	sensor_write(id, 0x58 + chn, 0x90);
	sensor_write(id, 0x5c + chn, 0xbc);
	sensor_write(id, 0x64 + chn, 0x81);
	sensor_write(id, 0x89 + chn, 0x10);
	sensor_write(id, chn + 0x8e, 0x2f);
	sensor_write(id, 0x30 + chn, 0x12);
	sensor_write(id, 0xa0 + chn, 0x05);
	sensor_write(id, 0xff, 0x01);
	sensor_write(id, 0x84 + chn, 0x06);
	sensor_write(id, 0x8c + chn, 0x86);
	sensor_read(id, 0xed, &val);
	sensor_write(id, 0xed, val & (~(0x01 << chn)));
	sensor_write(id, 0xff, 0x05 + chn);
	sensor_write(id, 0x20, 0x90);
	sensor_write(id, 0x25, 0xdc);
	sensor_write(id, 0x28, 0x80);
	sensor_write(id, 0x2b, 0x78);
	sensor_write(id, 0x47, 0x04);
	sensor_write(id, 0x50, 0x84);
	sensor_write(id, 0x58, 0x70);
	sensor_write(id, 0x69, 0x00);
	sensor_write(id, 0x7b, 0x00);
	sensor_write(id, 0xb8, 0xb9);
}

void n5_set_portmode(int id, unsigned char port, unsigned char muxmode, unsigned char is_bt601)
{
	data_type val_1xc8, val_1xca, val_0x54;
	unsigned char clk_freq_array[4] = {0x83, 0x03, 0x43, 0x63}; //clk_freq: 0~3:37.125M/74.25M/148.5M/297M
	sensor_write(id, 0xff, 0x00);
	sensor_read(id, 0x54, &val_0x54);
	if ((N5_OUTMODE_2MUX_SD == muxmode)  || (N5_OUTMODE_2MUX_HD == muxmode) ||
		(N5_OUTMODE_2MUX_FHD == muxmode) || (N5_OUTMODE_2MUX_BT1120S_720P == muxmode) ||
		(N5_OUTMODE_2MUX_BT1120S_1080P == muxmode))
		val_0x54 |= 0x01;
	else
		val_0x54 &= 0xFE;
	sensor_write(id, 0x54, val_0x54);

	sensor_write(id, 0xff, 0x01);
	sensor_read(id, 0xc8, &val_1xc8);
	switch (muxmode) {
	case N5_OUTMODE_1MUX_SD:
		sensor_write(id, 0xA0 + port, 0x00);
		sensor_write(id, 0xC0, 0x00);
		sensor_write(id, 0xC2, 0x11);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		sensor_write(id, 0xC8, val_1xc8);
		sensor_write(id, 0xCC + port, clk_freq_array[0]);
		break;
	case N5_OUTMODE_1MUX_HD:
		sensor_write(id, 0xA0 + port, 0x00);
		sensor_write(id, 0xC0, 0x00);
		sensor_write(id, 0xC2, 0x11);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		sensor_write(id, 0xC8, val_1xc8);
		sensor_write(id, 0xCC + port, clk_freq_array[1]);
		break;
	case N5_OUTMODE_1MUX_FHD:
		sensor_write(id, 0xA0 + port, 0x00);
		sensor_write(id, 0xC0, 0x00);
		sensor_write(id, 0xC2, 0x11);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		sensor_write(id, 0xC8, val_1xc8);
		sensor_write(id, 0xCC + port, clk_freq_array[2]);
		break;
	case N5_OUTMODE_1MUX_FHD_HALF:
		sensor_write(id, 0xA0 + port, 0x00);
		sensor_write(id, 0xC0, 0x88);
		sensor_write(id, 0xC2, 0x99);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		sensor_write(id, 0xC8, val_1xc8);
		sensor_write(id, 0xCC + port, clk_freq_array[1]);
		break;
	case N5_OUTMODE_2MUX_SD:
		sensor_write(id, 0xA0 + port, 0x20);
		sensor_write(id, 0xC0, 0x10);
		sensor_write(id, 0xC2, 0x10);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		val_1xc8 |= (1 == port ? 0x20 : 0x02);
		sensor_write(id, 0xC8, val_1xc8);
		sensor_write(id, 0xCC + port, clk_freq_array[1]);
		break;
	case N5_OUTMODE_2MUX_HD:
		sensor_write(id, 0xA0 + port, 0x20);
		sensor_write(id, 0xC0, 0x10);
		sensor_write(id, 0xC2, 0x10);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		val_1xc8 |= (1 == port ? 0x20 : 0x02);
		sensor_write(id, 0xC8, val_1xc8);
		sensor_write(id, 0xCC + port, clk_freq_array[2]);
		break;
	case N5_OUTMODE_2MUX_FHD:
		sensor_write(id, 0xA0 + port, 0x00);
		sensor_write(id, 0xC0, 0x10);
		sensor_write(id, 0xC2, 0x10);
		val_1xc8 &= (1 == port ? 0x0F : 0xF0);
		val_1xc8 |= (1 == port ? 0x20 : 0x02);
		sensor_write(id, 0xC8, val_1xc8);
		sensor_write(id, 0xCC + port, clk_freq_array[2]);
		break;
	case N5_OUTMODE_1MUX_BT1120S:
		sensor_write(id, 0xA0, 0x00);
		sensor_write(id, 0xA1, 0x00);
		sensor_write(id, 0xC0, 0xCC);
		sensor_write(id, 0xC1, 0xCC);
		sensor_write(id, 0xC2, 0x44);
		sensor_write(id, 0xC3, 0x44);
		sensor_write(id, 0xC8, 0x00);
		sensor_write(id, 0xCA, 0x33); //two ports are enabled
		sensor_write(id, 0xCC, clk_freq_array[2]);
		break;
	case N5_OUTMODE_2MUX_BT1120S_720P:
		sensor_write(id, 0xA0, 0x00);
		sensor_write(id, 0xA1, 0x00);
		sensor_write(id, 0xC0, 0xDC);  //C data
		sensor_write(id, 0xC1, 0xDC);
		sensor_write(id, 0xC2, 0x54); //Y data
		sensor_write(id, 0xC3, 0x54);
		sensor_write(id, 0xC8, 0x22);
		sensor_write(id, 0xCA, 0x33); //two ports are enabled
		sensor_write(id, 0xCC, clk_freq_array[1]);
		break;
	case N5_OUTMODE_2MUX_BT1120S_1080P:
		sensor_write(id, 0xA0, 0x20);
		sensor_write(id, 0xA1, 0x20);
		sensor_write(id, 0xC0, 0xDC);  //C data
		sensor_write(id, 0xC1, 0xDC);
		sensor_write(id, 0xC2, 0x54); //Y data
		sensor_write(id, 0xC3, 0x54);
		sensor_write(id, 0xC8, 0x22);
		sensor_write(id, 0xCA, 0x33); //two ports are enabled
		sensor_write(id, 0xCC, clk_freq_array[1]);
		break;
	}
	if (1 == is_bt601) {
		sensor_write(id, 0xA8 + port, 0x90 + (port * 0x10));	//h/v0 sync enabled
		//sensor_write(id, 0xA9, 0xA0); //h/v1 sync enabled
		//sensor_write(id, 0xBC, 0x10);	//h/v0 swap enabled
		//sensor_write(id, 0xBD, 0x10);
		//sensor_write(id, 0xBE, 0x10);	//h/v1 swap enabled
		//sensor_write(id, 0xBF, 0x10);
	} else {
		sensor_write(id, 0xA8, 0x00);
		//sensor_write(id, 0xA9, 0x00);  //h/v sync disable.
	}

	if (N5_OUTMODE_2MUX_BT1120S_720P == muxmode) {
		sensor_write(id, 0xE4, 0x11);
		sensor_write(id, 0xE5, 0x11);
	} else {
		sensor_write(id, 0xE4, 0x00);
		sensor_write(id, 0xE5, 0x00);
	}

	sensor_read(id, 0xca, &val_1xca);
	val_1xca |= (0x11 << port); //enable port
	sensor_write(id, 0xCA, val_1xca);
}

void n5_720pX2(int id)
{

	sensor_dbg("set %s\n", __func__);
	n5_regs_init_common(id);
	n5_set_chn_720p_25(id, 0);
	n5_set_chn_720p_25(id, 1);
	n5_set_portmode(id, 0, N5_OUTMODE_2MUX_HD, BT656);
	n5_set_portmode(id, 1, N5_OUTMODE_2MUX_HD, BT656);
}

void n5_1080pX1(int id)
{

	sensor_dbg("set %s\n", __func__);

	n5_regs_init_common(id);

	if (current_win[id]->fps_fixed == 30)
		n5_set_chn_1080p_30(id, 0);
	else if (current_win[id]->fps_fixed == 25)
		n5_set_chn_1080p_25(id, 0);
	else
		sensor_err("can not match %d fps for 1080p fmt\n", current_win[id]->fps_fixed);

	n5_set_portmode(id, 0, N5_OUTMODE_1MUX_FHD, BT656);
}

void n5_cvbs_palX1(int id)
{
	sensor_dbg("set %s\n", __func__);
	n5_regs_init_common(id);
	n5_set_chn_720h_pal(id, 0);
	n5_set_portmode(id, 0, N5_OUTMODE_1MUX_SD, BT656);
}

void  n5_cvbs_ntscX1(int id)
{
	sensor_dbg("set %s\n", __func__);
	n5_regs_init_common(id);
	n5_set_chn_720h_ntsc(id, 0);
	n5_set_portmode(id, 0, N5_OUTMODE_1MUX_SD, BT656);
}

static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
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
		sensor_dbg("CSI_SUBDEV_PWR_ON!\n");
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(1000);
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(10000);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(30000);
		break;
	case PWR_OFF:
		sensor_dbg("CSI_SUBDEV_PWR_OFF!\n");
		vin_set_mclk(id, 0);
		hal_usleep(100);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
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
	data_type id0;

	sensor_dbg("sensor_detect_1--!\n");

	sensor_write(id, 0xFF, 0x00);
	sensor_read(id, 0xF4, &id0);

	sensor_dbg("chip_id = 0x%x\n", id0);

	if (id0 != V4L2_IDENT_SENSOR) {
		sensor_err(KERN_DEBUG "sensor error,read id is 0x%x.\n", id0);
		return -1;
	} else {
		sensor_dbg(KERN_DEBUG "find n5 csi camera sensor now.\n");
		return 0;
	}
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

static struct sensor_format_struct sensor_formats[] = {
#if defined CONFIG_ISP_READ_THRESHOLD || defined CONFIG_ISP_ONLY_HARD_LIGHTADC
	{
		.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
		.width = HD720_WIDTH,
		.height = HD720_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 25,
		.switch_regs = NULL,
		.switch_regs_size = 0,
	},

	{
		.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
		.width = HD1080_WIDTH,
		.height = HD1080_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 30,
		.switch_regs = NULL,
		.switch_regs_size = 0,
	},

	{
		.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
		.width = HD1080_WIDTH,
		.height = HD1080_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 25,
		.switch_regs = NULL,
		.switch_regs_size = 0,
	},

	{
		.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
		.width = 720,
		.height = 576,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 25,
		.switch_regs = NULL,
		.switch_regs_size = 0,
	},

	{
		.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
		.width = 720,
		.height = 480,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 30,
		.switch_regs = NULL,
		.switch_regs_size = 0,
 	},
#endif
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
				sensor_print("fine wdr is %d, fine fps is %d\n", wdr_on, fps);
				goto done;
			}
		}
	}

	if (sensor_format == NULL) {
		for (i = 0; i < ARRAY_SIZE(sensor_formats); i++) {
			if (sensor_formats[i].wdr_mode != ISP_NORMAL_MODE && sensor_formats[i].wdr_mode == wdr_on) {
				sensor_format = &sensor_formats[i];
				isp_get_cfg[ispid].sensor_get_fps = sensor_format->fps_fixed;
				sensor_print("fine wdr is %d, use fps is %d\n", wdr_on, sensor_format->fps_fixed);
				goto done;
			}
		}
	}

	if (sensor_format == NULL && global_video[vinc_id].fps_fixed != 0) {
		for (i = 0; i < ARRAY_SIZE(sensor_formats); i++) {
			int dist = abs(sensor_formats[i].width - global_video[vinc_id].o_width)
				+ abs(sensor_formats[i].height - global_video[vinc_id].o_height);
				if ((dist < best_dist) &&
				(sensor_formats[i].width >= global_video[vinc_id].o_width) &&
				(sensor_formats[i].height >= global_video[vinc_id].o_height) &&
				(sensor_formats[i].wdr_mode == ISP_NORMAL_MODE) &&
				(sensor_formats[i].fps_fixed == global_video[vinc_id].fps_fixed)) {
					best_dist = dist;
					sensor_format = &sensor_formats[i];
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
	}

	sensor_print("use wdr is %d, use fps is %d, width:%d, height:%d\n", sensor_format->wdr_mode, sensor_format->fps_fixed, sensor_format->width, sensor_format->height);

done:
	current_win[id] = sensor_format;
	return sensor_format;
}
/*
static struct sensor_format_struct switch_sensor_formats[] = {
#if defined CONFIG_ISP_FAST_CONVERGENCE || defined CONFIG_ISP_HARD_LIGHTADC
	{
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
	 .width = HD720_WIDTH,
	 .height = HD720_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .fps_fixed = 25,
	 .regs = NULL,
	 .regs_size = 0,
	 .set_size = NULL,
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
	cfg->type = V4L2_MBUS_BT656;

	if (current_win[id]->width == HD1080_WIDTH) {
		CLK_POL = V4L2_MBUS_PCLK_SAMPLE_RISING;
		cfg->flags = CLK_POL | CSI_CH_0;
	} else if (current_win[id]->width == 720) {
		CLK_POL = V4L2_MBUS_PCLK_SAMPLE_FALLING;
		cfg->flags = CLK_POL | CSI_CH_0;
	} else {
		CLK_POL = V4L2_MBUS_PCLK_SAMPLE_FALLING;
		cfg->flags = CLK_POL | CSI_CH_0 | CSI_CH_1;
	}

	return 0;
}

static int sensor_reg_init(int id, int isp_id)
{

	sensor_dbg("sensor_reg_init\n");

	if (current_win[id]->width == 1920 && current_win[id]->height == 1080) {
		n5_1080pX1(id);
		sensor_print("sensor_reg_init 1920*1080\n");
	} else if (current_win[id]->width == 1280 && current_win[id]->height == 720) {
		n5_720pX2(id);
		sensor_print("sensor_reg_init 1280*720\n");
	} else if (current_win[id]->width == 720 && current_win[id]->height == 576) {
		n5_cvbs_palX1(id);
		sensor_print("sensor_reg_init 720*576\n");
	} else if (current_win[id]->width == 720 && current_win[id]->height == 480) {
		n5_cvbs_ntscX1(id);
		sensor_print("sensor_reg_init 720*480\n");
	}

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
		  current_win[id]->height, current_win[id]->fps);

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

struct sensor_fuc_core n5_core  = {
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