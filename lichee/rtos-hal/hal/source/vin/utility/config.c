/*
 * config.c for device tree and sensor list parser.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 * Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "config.h"
#include "../platform/platform_cfg.h"
#include "../vin.h"

#if defined CONFIG_DRIVER_SYSCONFIG && defined CONFIG_VIN_USE_SYSCONFIG
static int get_mname(char *main_name, char *sub_name,
		     int sneor_id)
{
	char str[16] = {0};

	if(hal_cfg_get_keyvalue(main_name, sub_name, (void *)str, sizeof(str))) {
		vin_err("%s error!\n", __func__);
		return -1;
	} else {
		memcpy((void *)&global_sensors[sneor_id].sensor_name, (void *)str, strlen(str) + 1);
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %s\n", main_name, sub_name, global_sensors[sneor_id].sensor_name);
	return 0;

}

static int get_twi_addr(char *main_name, char *sub_name,
		     int sneor_id)
{
	if(hal_cfg_get_keyvalue(main_name, sub_name, &global_sensors[sneor_id].sensor_twi_addr, sizeof(int))) {
		vin_err("%s error!\n", __func__);
		return -1;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_sensors[sneor_id].sensor_twi_addr);
	return 0;
}

static int get_twi_id(char *main_name, char *sub_name,
		     int sneor_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_sensors[sneor_id].sensor_twi_id, 1)) {
		vin_err("%s error!\n", __func__);
		return -1;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_sensors[sneor_id].sensor_twi_id);
	return 0;
}

static int get_mclk_id(char *main_name, char *sub_name,
		     int sneor_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_sensors[sneor_id].mclk_id, 1)) {
		vin_err("%s error!\n", __func__);
		return -1;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_sensors[sneor_id].mclk_id);
	return 0;
}

static int get_isp_used(char *main_name, char *sub_name,
		     int sneor_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_sensors[sneor_id].use_isp, 1)) {
		vin_err("%s error!\n", __func__);
		return -1;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_sensors[sneor_id].use_isp);
	return 0;
}

static int get_gpio(char *main_name, char *sub_name,
		     int sneor_id)
{
	int count, i;
	user_gpio_set_t gpio_cfg[MAX_GPIO_NUM];

	count = hal_cfg_get_gpiosec_keycount(main_name);
	if (!count)	{
		vin_warn("%s not config gpio", main_name);
		return 0;
	}

	hal_cfg_get_gpiosec_data(main_name, gpio_cfg, count);

	for (i = 0; i < count; i++) {
		global_sensors[sneor_id].sensor_pin[i] = (gpio_cfg[i].port - 1) * 32 + gpio_cfg[i].port_num;
		global_sensors[sneor_id].sensor_pin_muxsel[i] = gpio_cfg[i].mul_sel;
		global_sensors[sneor_id].sensor_pin_driving_level[i] = gpio_cfg[i].drv_level;
		if (global_sensors[sneor_id].sensor_pin_muxsel[i] == -1)
			global_sensors[sneor_id].sensor_pin[i] = 0xffff;

		vin_log(VIN_LOG_CONFIG, "%s, %s, %d, %d\n", main_name, sub_name, global_sensors[sneor_id].sensor_pin[i], global_sensors[sneor_id].sensor_pin_muxsel[i]);
	}

	return 0;
}

static int get_csi_sel(char *main_name, char *sub_name,
		     int video_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_video[video_id].csi_sel, 1)) {
		global_video[video_id].csi_sel = 0;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].csi_sel);
	return 0;
}

static int get_mipi_sel(char *main_name, char *sub_name,
		     int video_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_video[video_id].mipi_sel, 1)) {
		global_video[video_id].mipi_sel = 0;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].mipi_sel);
	return 0;
}

static int get_isp_sel(char *main_name, char *sub_name,
		     int video_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_video[video_id].isp_sel, 1)) {
		global_video[video_id].isp_sel = 0;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].isp_sel);
	return 0;
}

static int get_isp_tx_ch(char *main_name, char *sub_name,
		     int video_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_video[video_id].isp_tx_ch, 1)) {
		global_video[video_id].isp_tx_ch = 0;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].isp_tx_ch);
	return 0;
}

static int get_tdm_rx_sel(char *main_name, char *sub_name,
		     int video_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_video[video_id].tdm_rx_sel, 1)) {
		global_video[video_id].tdm_rx_sel = 0;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].tdm_rx_sel);
	return 0;
}

static int get_rear_sensor_sel(char *main_name, char *sub_name,
		     int video_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_video[video_id].rear_sensor, 1)) {
		global_video[video_id].rear_sensor = 0;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].rear_sensor);
	return 0;
}

static int get_front_sensor_sel(char *main_name, char *sub_name,
		     int video_id)
{
	if (hal_cfg_get_keyvalue(main_name, sub_name, &global_video[video_id].front_sensor, 1)) {
		global_video[video_id].front_sensor = 1;
	}

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].front_sensor);
	return 0;
}

static int get_width(char *main_name, char *sub_name,
		     int video_id)
{
	int value;

	if (hal_cfg_get_keyvalue(main_name, sub_name, &value, 1)) {
		global_video[video_id].o_width = 640;
		return 0;
	}
	global_video[video_id].o_width = value;

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].o_width);
	return 0;
}

static int get_height(char *main_name, char *sub_name,
		     int video_id)
{
	int value;

	if (hal_cfg_get_keyvalue(main_name, sub_name, &value, 1)) {
		global_video[video_id].o_height = 480;
		return 0;
	}
	global_video[video_id].o_height = value;

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].o_height);
	return 0;
}

static int get_merge_width(char *main_name, char *sub_name,
		     int video_id)
{
	int value;

	if (hal_cfg_get_keyvalue(main_name, sub_name, &value, 1)) {
		global_video[video_id].merge_width = 640;
		return 0;
	}
	global_video[video_id].merge_width = value;

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].merge_width);
	return 0;
}

static int get_merge_height(char *main_name, char *sub_name,
		     int video_id)
{
	int value;

	if (hal_cfg_get_keyvalue(main_name, sub_name, &value, 1)) {
		global_video[video_id].merge_height = 480;
		return 0;
	}
	global_video[video_id].merge_height = value;

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].merge_height);
	return 0;
}

static int get_use_sensor_list(char *main_name, char *sub_name,
		     int video_id)
{
	int value;

	if (hal_cfg_get_keyvalue(main_name, sub_name, &value, 1)) {
		global_video[video_id].use_sensor_list = 0;
		return 0;
	}
	global_video[video_id].use_sensor_list = 0;

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].use_sensor_list);
	return 0;
}

static int get_mipi_num(char *main_name, char *sub_name,
		     int video_id)
{
	int value;

	if (hal_cfg_get_keyvalue(main_name, sub_name, &value, 1)) {
		global_video[video_id].mipi_num = 1;
		return 0;
	}
	global_video[video_id].mipi_num = value;

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].mipi_num);
	return 0;
}

static int get_work_mode(char *main_name, char *sub_name,
		     int video_id)
{
	int value;


	if (video_id % 4 != 0)
		return 0;

	if (hal_cfg_get_keyvalue(main_name, sub_name, &value, 1)) {
		vin_work_mode = 0;
		return 0;
	}
	vin_work_mode = value;

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, vin_work_mode);
	return 0;
}

static int get_large_image(char *main_name, char *sub_name,
		     int video_id)
{
	int value;

	if (hal_cfg_get_keyvalue(main_name, sub_name, &value, 1)) {
		global_video[video_id].large_image = 0;
		return 0;
	}
	global_video[video_id].large_image = value;

	vin_log(VIN_LOG_CONFIG, "%s, %s, %d\n", main_name, sub_name, global_video[video_id].large_image);
	return 0;
}

struct fetchfunarr fetch_sensor[] = {
	{"mname", get_mname,},
	{"twi_addr", get_twi_addr,},
	{"twi_cci_id", get_twi_id,},
	{"mclk_id", get_mclk_id,},
	{"isp_used", get_isp_used,},
	{"gpio_get", get_gpio,},
};

struct fetchfunarr fetch_video[] = {
	{"csi_sel", get_csi_sel,},
	{"mipi_sel", get_mipi_sel,},
	{"isp_sel", get_isp_sel,},
	{"isp_tx_ch", get_isp_tx_ch,},
	{"tdm_rx_sel", get_tdm_rx_sel,},
	{"rear_sensor_sel", get_rear_sensor_sel,},
	{"front_sensor_sel", get_front_sensor_sel,},
	{"width", get_width,},
	{"height", get_height,},
	{"merge_width", get_merge_width,},
	{"merge_height", get_merge_height,},
	{"use_sensor_list", get_use_sensor_list,},
	{"mipi_num", get_mipi_num,},
	{"work_mode", get_work_mode,},
	{"large_image", get_large_image,},
};

static int get_clk_from_sys_config(void)
{
	int value;

	if (hal_cfg_get_keyvalue("vind", "csi_top", &value, 1)) {
		vin_err("get csi clk freq error!\n");
		return -1;
	}
	vind_default_clk[VIN_TOP_CLK].frequency = value;

	if (hal_cfg_get_keyvalue("vind", "csi_top_parent", &value, 1)) {
		vin_err("get csi parent clk freq error!\n");
		return -1;
	}
	vind_csi_isp_parent_clk[VIN_CSI_PARENT].frequency = value;

	if (hal_cfg_get_keyvalue("vind", "csi_isp", &value, 1)) {
		vin_warn("get isp clk freq fail!\n");
		vind_default_isp_clk[VIN_ISP_CLK].frequency = 0xffffffff;
	} else
		vind_default_isp_clk[VIN_ISP_CLK].frequency = value;

	if (vind_default_isp_clk[VIN_ISP_CLK].frequency != 0xffffffff) {
		if (hal_cfg_get_keyvalue("vind", "csi_isp_parent", &value, 1)) {
			vin_warn("get isp clk freq fail!\n");
			return -1;
		}
		vind_csi_isp_parent_clk[VIN_ISP_PARENT].frequency = value;
	}

	return 0;
}

int parse_modules_from_sys_config(void)
{
	int i, value, j, ret;
	char main_name[16];
	char sub_name[16];

	for (i = 0; i < VIN_MAX_CSI; i++) {
		sprintf(main_name, "vind/sensor%d", i);
		sprintf(sub_name, "sensor%d_used", i);
		value = 0;
		hal_cfg_get_keyvalue(main_name, sub_name, &value, 1);
		if (!value)
			continue;
		for(j = 0; j < ARRAY_SIZE(fetch_sensor); j++) {
			sprintf(sub_name, "sensor%d_%s", i, fetch_sensor[j].sub);
			fetch_sensor[j].fun(main_name, sub_name, i);
		}
	}

	for (i = 0; i < VIN_MAX_VIDEO; i++) {
		sprintf(main_name, "vind/vinc%d", i);
		sprintf(sub_name, "vinc%d_used", i);
		value = 0;
		hal_cfg_get_keyvalue(main_name, sub_name, &value, 1);
		global_video[i].used = value;
		if (!value)
			continue;
		global_video[i].id = i;
		for(j = 0; j < ARRAY_SIZE(fetch_video); j++) {
			sprintf(sub_name, "vinc%d_%s", i, fetch_video[j].sub);
			fetch_video[j].fun(main_name, sub_name, i);
		}
	}

	ret = get_clk_from_sys_config();

	return ret;
}
#else
int parse_modules_from_sys_config(void)
{
	return 0;
}
#endif
