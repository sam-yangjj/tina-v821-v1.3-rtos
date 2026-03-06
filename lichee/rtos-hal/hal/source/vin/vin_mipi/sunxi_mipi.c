/*
 * mipi subdev driver module
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <hal_mem.h>

#include "../vin.h"
#include "sunxi_mipi.h"

struct mipi_dev *global_mipi[VIN_MAX_MIPI];
bool mipi_probe_flag[VIN_MAX_MIPI];

#define IS_FLAG(x, y) (((x)&(y)) == y)

static struct combo_format sunxi_mipi_formats[] = {
	{
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR8_1X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG8_1X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG8_1X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB8_1X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.bit_width = RAW10,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.bit_width = RAW10,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.bit_width = RAW10,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB10_1X10,
		.bit_width = RAW10,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR12_1X12,
		.bit_width = RAW12,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG12_1X12,
		.bit_width = RAW12,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG12_1X12,
		.bit_width = RAW12,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.bit_width = RAW12,
	}, {
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.bit_width = RAW8,
		.yuv_seq = UYVY,
	}, {
		.code = MEDIA_BUS_FMT_VYUY8_2X8,
		.bit_width = RAW8,
		.yuv_seq = VYUY,
	}, {
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
		.bit_width = RAW8,
		.yuv_seq = YUYV,
	}, {
		.code = MEDIA_BUS_FMT_YVYU8_2X8,
		.bit_width = RAW8,
		.yuv_seq = YVYU,
	}, {
		.code = MEDIA_BUS_FMT_UYVY10_2X10,
		.bit_width = RAW10,
		.yuv_seq = UYVY,
	}, {
		.code = MEDIA_BUS_FMT_VYUY10_2X10,
		.bit_width = RAW10,
		.yuv_seq = VYUY,
	}, {
		.code = MEDIA_BUS_FMT_YUYV10_2X10,
		.bit_width = RAW10,
		.yuv_seq = YUYV,
	}, {
		.code = MEDIA_BUS_FMT_YVYU10_2X10,
		.bit_width = RAW10,
		.yuv_seq = YVYU,
	}
};

#if 0
static unsigned int data_formats_type(u32 code)
{
	switch (code) {
	case MIPI_RAW8:
	case MIPI_RAW12:
		return RAW;
	case MIPI_YUV422:
	case MIPI_YUV422_10:
		return YUV;
	case MIPI_RGB565:
		return RGB;
	default:
		return RAW;
	}
}
#endif

static enum pkt_fmt get_pkt_fmt(u32 code)
{
	switch (code) {
	case MEDIA_BUS_FMT_RGB565_2X8_BE:
		return MIPI_RGB565;
	case MEDIA_BUS_FMT_UYVY8_1X16:
	case MEDIA_BUS_FMT_UYVY8_2X8:
	case MEDIA_BUS_FMT_VYUY8_2X8:
	case MEDIA_BUS_FMT_YUYV8_2X8:
	case MEDIA_BUS_FMT_YVYU8_2X8:
		return MIPI_YUV422;
	case MEDIA_BUS_FMT_UYVY10_2X10:
	case MEDIA_BUS_FMT_VYUY10_2X10:
	case MEDIA_BUS_FMT_YUYV10_2X10:
	case MEDIA_BUS_FMT_YVYU10_2X10:
		return MIPI_YUV422_10;
	case MEDIA_BUS_FMT_SBGGR8_1X8:
	case MEDIA_BUS_FMT_SGBRG8_1X8:
	case MEDIA_BUS_FMT_SGRBG8_1X8:
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		return MIPI_RAW8;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
	case MEDIA_BUS_FMT_SGBRG10_1X10:
	case MEDIA_BUS_FMT_SGRBG10_1X10:
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		return MIPI_RAW10;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
	case MEDIA_BUS_FMT_SGBRG12_1X12:
	case MEDIA_BUS_FMT_SGRBG12_1X12:
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		return MIPI_RAW12;
	default:
		return MIPI_RAW8;
	}
}

static int __mcsi_pin_config(int id, int enable)
{
	struct vin_mipi_gpio_info *mipi_gpio = &vin_mipi_gpio[id];
	int i;

	if (enable) {
		for (i = 0; i < 6; i++) {
			if (mipi_gpio->pin[i] < 0)
				continue;
			hal_gpio_pinmux_set_function(mipi_gpio->pin[i], mipi_gpio->pin_func[0]);
		}
	} else {
		for (i = 0; i < 6; i++) {
			hal_gpio_pinmux_set_function(mipi_gpio->pin[i], mipi_gpio->pin_func[1]);
		}
	}

	return 0;
}

static void cmb_phy_init(struct mipi_dev *mipi)
{
	cmb_phy_lane_num_en(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_phy0_work_mode(mipi->id + mipi->phy_offset, 0);
	cmb_phy0_ofscal_cfg(mipi->id + mipi->phy_offset);
	//cmb_phy_deskew_en(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_term_ctl(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_hs_ctl(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_s2p_ctl(mipi->id + mipi->phy_offset, mipi->time_hs, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_mipirx_ctl(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_phy0_en(mipi->id + mipi->phy_offset, 1);
	cmb_phy_deskew1_cfg(mipi->id + mipi->phy_offset, mipi->deskew, mipi->cmb_mode == MIPI_VC_WDR_MODE ? true : false);
}

static struct combo_format *__mipi_try_format(struct sensor_format_struct *mf)
{
	struct combo_format *mipi_fmt = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_mipi_formats); i++)
		if (mf->mbus_code == sunxi_mipi_formats[i].code)
			mipi_fmt = &sunxi_mipi_formats[i];

	if (mipi_fmt == NULL)
		mipi_fmt = &sunxi_mipi_formats[0];

	return mipi_fmt;
}

static void combo_csi_mipi_init(struct mipi_dev *mipi, unsigned int vinc_id)
{
	int i;
	int isp_sel = global_video[vinc_id].isp_sel;
	struct sensor_format_struct *sensor_format = NULL;
#if defined CONFIG_ARCH_SUN55IW3
	int link_mode_tmp;
#endif
	int sensor_id = global_video[vinc_id].rear_sensor;

	mipi->cmb_csi_cfg.phy_lane_cfg.phy_laneck_en = CK_1LANE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpck_en = LPCK_1LANE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_termck_en = TERMCK_CLOSE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_termdt_en = TERMDT_CLOSE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_s2p_en = S2PDT_CLOSE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_hsck_en = HSCK_CLOSE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_hsdt_en = HSDT_CLOSE;

	cmb_phy_init(mipi);
#if defined CONFIG_ARCH_SUN55IW3
	/* A523 4lane needs cfg link mode */
	if (mipi->cmb_csi_cfg.phy_link_mode == LANE_4) {
		cmb_phy_link_mode_get(&link_mode_tmp);
		mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpck_en = LPCK_CLOSE;
		mipi->phy_offset = 1;
		if (mipi->set_lane_choice[0] == 4 || mipi->set_lane_choice[2] == 4) {
			if (mipi->set_lane_choice[0] == 4 &&  mipi->set_lane_choice[1] == 0 && link_mode_tmp == FOUR_2LANE)
				cmb_phy_link_mode_set(ONE_4LANE_PHYA);
			else if ((mipi->set_lane_choice[2] == 4 &&  mipi->set_lane_choice[3] == 0 && link_mode_tmp == FOUR_2LANE))
				cmb_phy_link_mode_set(ONE_4LANE_PHYC);
			else if ((mipi->set_lane_choice[0] == 4 &&  mipi->set_lane_choice[1] == 0 && link_mode_tmp == ONE_4LANE_PHYC)
			|| (mipi->set_lane_choice[2] == 4 &&  mipi->set_lane_choice[3] == 0 && link_mode_tmp == ONE_4LANE_PHYA))
				cmb_phy_link_mode_set(TWO_4LANE);
			else if ((mipi->set_lane_choice[0] == 4 &&  mipi->set_lane_choice[1] == 0 && link_mode_tmp == ONE_4LANE_PHYA)
			|| (mipi->set_lane_choice[0] == 4 &&  mipi->set_lane_choice[1] == 0 && link_mode_tmp == TWO_4LANE)
			|| (mipi->set_lane_choice[2] == 4 &&  mipi->set_lane_choice[3] == 0 && link_mode_tmp == ONE_4LANE_PHYC)
			|| (mipi->set_lane_choice[2] == 4 &&  mipi->set_lane_choice[3] == 0 && link_mode_tmp == TWO_4LANE))
				vin_log(VIN_LOG_MIPI, "mipi link mode already set!\n");
			else
				vin_err("phy link mode set error!\n");
		} else {
			vin_err("phy link mode set error, mipi sel set error!\n");
		}
		cmb_phy_init(mipi);
		mipi->phy_offset = 0;
	}
#elif defined CONFIG_ARCH_SUN300IW1
	/* V821 2lane needs cfg link mode */
	if (mipi->cmb_csi_cfg.phy_link_mode == ONE_2LANE) {
		mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpck_en = LPCK_CLOSE;
		mipi->phy_offset = 1;
		cmb_phy_link_mode_set(ONE_2LANE);
		cmb_phy_init(mipi);
		mipi->phy_offset = 0;
	}
#else
	/* V853 4lane needs cfg link mode */
	if (mipi->cmb_csi_cfg.phy_link_mode == ONE_4LANE) {
		mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpck_en = LPCK_CLOSE;
		mipi->phy_offset = 1;
		cmb_phy_link_mode_set(ONE_4LANE);
		cmb_phy_init(mipi);
		mipi->phy_offset = 0;
	}
#endif
	for (i = 0; i < mipi->cmb_csi_cfg.lane_num; i++) {
		mipi->cmb_csi_cfg.mipi_lane[i] = cmb_port_set_lane_map(mipi->id, i);
	}

	if (global_sensors[sensor_id].sensor_core->sensor_g_format) {
		sensor_format = global_sensors[sensor_id].sensor_core->sensor_g_format(sensor_id, isp_sel, vinc_id);
		if (sensor_format == NULL) {
			vin_err("mipi get sensor_format error!\n");
			return;
		}
	}

	for (i = 0; i < mipi->cmb_csi_cfg.total_rx_ch; i++) {
		mipi->cmb_csi_cfg.mipi_datatype[i] = get_pkt_fmt(sensor_format->mbus_code);
		mipi->cmb_csi_cfg.vc[i] = i;
		cmb_port_mipi_ch_trigger_en(mipi->id, i, 1);
	}

	cmb_port_lane_num(mipi->id, mipi->cmb_csi_cfg.lane_num);
	cmb_port_out_num(mipi->id, mipi->cmb_csi_cfg.pix_num);
	if (mipi->cmb_mode == MIPI_DOL_WDR_MODE)
		cmb_port_out_chnum(mipi->id, 0);
	else
		cmb_port_out_chnum(mipi->id, mipi->cmb_csi_cfg.total_rx_ch - 1);
	cmb_port_lane_map(mipi->id, mipi->cmb_csi_cfg.mipi_lane);
	mipi->cmb_fmt = __mipi_try_format(sensor_format);
	cmb_port_mipi_cfg(mipi->id, mipi->cmb_fmt->yuv_seq);
	cmb_port_set_mipi_datatype(mipi->id, &mipi->cmb_csi_cfg);
	if (mipi->cmb_mode == MIPI_DOL_WDR_MODE)
		cmb_port_set_mipi_wdr(mipi->id, 0, 2);
	cmb_port_enable(mipi->id);
}

void combo_csi_init(struct mipi_dev *mipi, unsigned int vinc_id)
{
	switch (mipi->if_type) {
	case V4L2_MBUS_PARALLEL:
	case V4L2_MBUS_BT656:
		break;
	case V4L2_MBUS_CSI2:
		combo_csi_mipi_init(mipi, vinc_id);
		break;
	case V4L2_MBUS_SUBLVDS:
		break;
	case V4L2_MBUS_HISPI:
		break;
	default:
		break;
	}
}

/* PHY_S2P_DLY = 145 * CSICLK(MHZ) / 1000 + PHY_FREQ_CNT / 800 -10 */
static int sunxi_mipi_cal_time_hs(unsigned int id)
{
	unsigned int csi_clk;
	unsigned int phy_freq_cnt;
	unsigned int phy_s2p_dly;

	csi_clk = vind_default_clk[VIN_TOP_CLK].frequency / 1000000;
	phy_freq_cnt = cmb_phy_freq_cnt_get(id);
	phy_s2p_dly = 145 * csi_clk / 1000 + phy_freq_cnt / 800 - 10;

	return phy_s2p_dly;
}

static int sunxi_mipi_top_s_stream(int id, int on)
{
	struct mipi_dev *mipi = global_mipi[id];

	if (on && (mipi->top_stream_count)++ > 0)
		return 0;
	else if (!on && (mipi->top_stream_count == 0 || --(mipi->stream_count) > 0))
		return 0;

	if (on) {
		cmb_phy_top_enable();
	} else {
		cmb_phy_top_disable();
	}

	return 0;
}


int sunxi_mipi_subdev_s_stream(unsigned int id, unsigned int vinc_id, int enable)
{
	struct mipi_dev *mipi = global_mipi[id];
	int *stream_count;

	stream_count = &mipi->stream_count;
	if (enable && (*stream_count)++ > 0)
		return 0;
	else if (!enable && (*stream_count == 0 || --(*stream_count) > 0))
		return 0;

	if (mipi->res->res_time_hs)
		mipi->time_hs = mipi->res->res_time_hs;
	else
		mipi->time_hs = sunxi_mipi_cal_time_hs(id);

	if (mipi->res->deskew)
		mipi->deskew = mipi->res->deskew;

	mipi->cmb_mode = mipi->res->res_combo_mode;
#if defined CONFIG_ARCH_SUN55IW3
	if (mipi->cmb_csi_cfg.phy_link_mode == LANE_4) {
		__mcsi_pin_config(mipi->id, enable);
		__mcsi_pin_config(mipi->id + 1, enable);
	} else
		__mcsi_pin_config(mipi->id, enable);
#elif defined CONFIG_ARCH_SUN300IW1
	if (mipi->cmb_csi_cfg.phy_link_mode == ONE_2LANE) {
		__mcsi_pin_config(mipi->id, enable);
		__mcsi_pin_config(mipi->id + 1, enable);
	} else
		__mcsi_pin_config(mipi->id, enable);
#else
	if (mipi->cmb_csi_cfg.phy_link_mode == ONE_4LANE) {
		__mcsi_pin_config(mipi->id, enable);
		__mcsi_pin_config(mipi->id + 1, enable);
	} else
		__mcsi_pin_config(mipi->id, enable);
#endif
	sunxi_mipi_top_s_stream(mipi->id, enable);
	if (enable) {
		combo_csi_init(mipi, vinc_id);
	} else {
/*
#if defined CONFIG_ARCH_SUN55IW3
		mipi->cmb_csi_cfg.phy_link_mode = FOUR_2LANE;
#else
		mipi->cmb_csi_cfg.phy_link_mode = TWO_2LANE;
#endif
		mipi->cmb_csi_cfg.pix_num = ONE_DATA;
*/
		cmb_port_disable(mipi->id);
		cmb_phy0_en(mipi->id, 0);
	}

	vin_log(VIN_LOG_FMT, "%s%d %s, lane_num %d\n",
		mipi->if_name, mipi->id, enable ? "stream on" : "stream off",
		mipi->cmb_csi_cfg.lane_num);

	return 0;
}

int sunxi_mipi_s_mbus_config(unsigned int id, const struct v4l2_mbus_config *cfg, const struct mbus_framefmt_res *res)
{
	struct mipi_dev *mipi = global_mipi[id];

#if 0
	if (cfg->type == V4L2_MBUS_CSI2) {
		mipi->if_type = V4L2_MBUS_CSI2;
		memcpy(mipi->if_name, "mipi", sizeof("mipi"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_4_LANE)) {
			mipi->csi2_cfg.lane_num = 4;
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.mipi_ln = MIPI_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_3_LANE)) {
			mipi->csi2_cfg.lane_num = 3;
			mipi->cmb_cfg.lane_num = 3;
			mipi->cmb_cfg.mipi_ln = MIPI_3LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_2_LANE)) {
			mipi->csi2_cfg.lane_num = 2;
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.mipi_ln = MIPI_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 1;
			mipi->csi2_cfg.lane_num = 1;
			mipi->cmb_cfg.mipi_ln = MIPI_1LANE;
		}
	} else if (cfg->type == V4L2_MBUS_SUBLVDS) {
		mipi->if_type = V4L2_MBUS_SUBLVDS;
		memcpy(mipi->if_name, "sublvds", sizeof("sublvds"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		}
	} else if (cfg->type == V4L2_MBUS_HISPI) {
		mipi->if_type = V4L2_MBUS_HISPI;
		memcpy(mipi->if_name, "hispi", sizeof("hispi"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		}
	} else {
		memcpy(mipi->if_name, "combo_parallel", sizeof("combo_parallel"));
		mipi->if_type = V4L2_MBUS_PARALLEL;
	}

	mipi->csi2_cfg.total_rx_ch = 0;
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0)) {
		mipi->csi2_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1)) {
		mipi->csi2_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2)) {
		mipi->csi2_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3)) {
		mipi->csi2_cfg.total_rx_ch++;
	}
#else
#ifdef CONFIG_ARCH_SUN55IW3
	if (cfg->type == V4L2_MBUS_CSI2) {
		mipi->if_type = V4L2_MBUS_CSI2;
		memcpy(mipi->if_name, "mipi", sizeof("mipi"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_4_LANE)) {
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_4LANE;
			mipi->cmb_csi_cfg.lane_num = 4;
			mipi->cmb_csi_cfg.phy_link_mode = LANE_4;
			mipi->cmb_csi_cfg.pix_num = TWO_DATA;
			if (mipi->id == 1 || mipi->id == 3)
				vin_err("PORT%d supports a maximum of 2lane!\n", mipi->id);
			if (mipi->id < 3 && global_mipi[mipi->id + 1]->stream_count)
				vin_err("PORT%d in using, PORT%d cannot 4lane!\n", mipi->id + 1, mipi->id);
			mipi->set_lane_choice[mipi->id] = 4;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_3_LANE)) {
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_3LANE;
			mipi->cmb_csi_cfg.lane_num = 3;
			mipi->cmb_csi_cfg.phy_link_mode = LANE_4;
			mipi->cmb_csi_cfg.pix_num = TWO_DATA;
			if (mipi->id == 1 || mipi->id == 3)
				vin_err("PORT%d supports a maximum of 2lane!\n", mipi->id);
			if (mipi->id < 3 && global_mipi[mipi->id + 1]->stream_count)
				vin_err("PORT%d in using, PORT%d cannot 3lane!\n", mipi->id + 1, mipi->id);
			mipi->set_lane_choice[mipi->id] = 4;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_2_LANE)) {
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_2LANE;
			mipi->cmb_csi_cfg.lane_num = 2;
			mipi->cmb_csi_cfg.phy_link_mode = LANE_2;
			mipi->cmb_csi_cfg.pix_num = ONE_DATA;
			mipi->set_lane_choice[mipi->id] = 2;
		} else {
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_1LANE;
			mipi->cmb_csi_cfg.lane_num = 1;
			mipi->cmb_csi_cfg.phy_link_mode = LANE_2;
			mipi->cmb_csi_cfg.pix_num = ONE_DATA;
			mipi->set_lane_choice[mipi->id] = 2;
		}
	}

#else
	if (cfg->type == V4L2_MBUS_CSI2) {
		mipi->if_type = V4L2_MBUS_CSI2;
		memcpy(mipi->if_name, "mipi", sizeof("mipi"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_4_LANE)) {
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_4LANE;
			mipi->cmb_csi_cfg.lane_num = 4;
#if defined CONFIG_ARCH_SUN20IW3
			mipi->cmb_csi_cfg.phy_link_mode = ONE_4LANE;
			if (mipi->id)
				vin_err("PORT1 supports a maximum of 2lane!\n");
#endif
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_3_LANE)) {
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_3LANE;
			mipi->cmb_csi_cfg.lane_num = 3;
#if defined CONFIG_ARCH_SUN20IW3
			mipi->cmb_csi_cfg.phy_link_mode = ONE_4LANE;
			if (mipi->id)
				vin_err("PORT1 supports a maximum of 2lane!\n");
#endif
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_2_LANE)) {
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_2LANE;
			mipi->cmb_csi_cfg.lane_num = 2;
#if defined CONFIG_ARCH_SUN300IW1
			mipi->cmb_csi_cfg.phy_link_mode = ONE_2LANE;
			if (mipi->id)
				vin_err("PORT1 supports a maximum of 1lane!\n");
#endif
		} else {
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_1LANE;
			mipi->cmb_csi_cfg.lane_num = 1;
		}
	}
#endif
	mipi->cmb_csi_cfg.total_rx_ch = 0;
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0)) {
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1)) {
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2)) {
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3)) {
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
#endif
	memcpy(mipi->res, res, sizeof(struct mbus_framefmt_res));

	return 0;
}

int mipi_probe(unsigned int id)
{
	struct mipi_dev *mipi = NULL;
	int ret = 0;

	if (mipi_probe_flag[id])
		return 0;
	else
		mipi_probe_flag[id] = true;

	if (id > VIN_MAX_MIPI) {
		vin_err("mipi%d is not existing, max is %d\n", id, VIN_MAX_MIPI);
		return -1;
	}

	mipi = hal_malloc(sizeof(struct mipi_dev));
	if (!mipi) {
		vin_err("Fail to init MIPI dev!\n");
		ret = -1;
		goto ekzalloc;
	}
	memset(mipi, 0, sizeof(struct mipi_dev));
	mipi->id = id;

	mipi->res = hal_malloc(sizeof(struct mbus_framefmt_res));
	if (!mipi->res) {
		vin_err("Fail to init MIPI res!\n");
		ret = -1;
		goto freedev;
	}
	memset(mipi->res, 0, sizeof(struct mbus_framefmt_res));

	mipi->base = sunxi_vin_get_mipi_base();
	cmb_csi_set_top_base_addr((unsigned long)mipi->base);
	vin_log(VIN_LOG_MD, "mipi%d reg is 0x%lx\n", mipi->id, mipi->base);
	mipi->phy_base = sunxi_vin_get_mipiphy_base(mipi->id);
	cmb_csi_set_phy_base_addr(mipi->id, (unsigned long)mipi->phy_base);
	mipi->port_base = sunxi_vin_get_mipiport_base(mipi->id);
	cmb_csi_set_port_base_addr(mipi->id, (unsigned long)mipi->port_base);

	global_mipi[id] = mipi;
	vin_log(VIN_LOG_MIPI, "mipi%d probe end!\n", mipi->id);
	return 0;

freedev:
	hal_free(mipi);
ekzalloc:
	vin_err("mipi probe err!\n");
	return ret;
}

int mipi_remove(unsigned int id)
{
	struct mipi_dev *mipi = global_mipi[id];

	if (!mipi_probe_flag[id])
		return 0;
	else
		mipi_probe_flag[id] = false;

	hal_free(mipi->res);
	hal_free(mipi);
	return 0;
}


