/*
 * vin_supply.c
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zequn Zheng <zequnzhengi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <sunxi_hal_twi.h>

#include "vin_supply.h"
#include "../platform/platform_cfg.h"
#include "../vin_video/vin_core.h"

extern struct vin_core *global_vinc[VIN_MAX_VIDEO];

int sunxi_vin_get_csi_top_irq(void)
{
	return SUNXI_IRQ_CSI_TOP_PKT;
}

int sunxi_vin_get_isp_irq(int id)
{
	return vin_isp_irq[id];
}

unsigned long sunxi_vin_get_top_base(void)
{
	return (unsigned long)CSI_TOP_REGS_BASE;
}

unsigned long sunxi_vin_get_ccu_base(void)
{
	return (unsigned long)CSI_CCU_REGS_BASE;
}

unsigned long sunxi_vin_get_mipi_base(void)
{
	return (unsigned long)vin_mipi_base[0];
}

unsigned long sunxi_vin_get_mipiphy_base(int id)
{
	return (unsigned long)vin_mipi_base[1 + id];
}

unsigned long sunxi_vin_get_mipiport_base(int id)
{
	return (unsigned long)vin_mipi_port_base[id];
}

unsigned long sunxi_vin_get_csi_base(int id)
{
	return (unsigned long)vin_csi_base[id];
}

unsigned long sunxi_vin_get_isp_base(int id)
{
	return (unsigned long)vin_isp_base[id/DEV_VIRT_NUM];
}

unsigned long sunxi_vin_get_scaler_base(int id)
{
	return (unsigned long)vin_scaler_base[id/DEV_VIRT_NUM];
}

int sunxi_vin_get_vipp_irq(int id)
{
	return vin_vipp_irq[id/DEV_VIRT_NUM];
}

int vin_vinc_parser(unsigned int id)
{
	struct vin_core *vinc = global_vinc[id];

	vinc->used = global_video[id].used;
	vinc->csi_sel = global_video[id].csi_sel;
	vinc->mipi_sel = global_video[id].mipi_sel;
	vinc->isp_sel = global_video[id].isp_sel;
	vinc->tdm_rx_sel = global_video[id].tdm_rx_sel;
	vinc->isp_tx_ch = global_video[id].isp_tx_ch;
	vinc->rear_sensor = global_video[id].rear_sensor;
	vinc->large_image = global_video[id].large_image;
	if (vinc->large_image) {
		vinc->o_width = global_video[id].merge_width;
		vinc->o_height = global_video[id].merge_height;
	} else {
		vinc->o_width = global_video[id].o_width;
		vinc->o_height = global_video[id].o_height;
	}

	return 0;
}

int sunxi_twi_init(int id)
{
	int slave_addr;
	twi_port_t port;
#if defined CONFIG_ARCH_SUN55IW3
	data_type value;
#endif
	slave_addr = global_sensors[id].sensor_twi_addr >> 1;
	port = global_sensors[id].sensor_twi_id;

	hal_twi_init(port);
#if !defined CONFIG_ARCH_SUN300IW1
#if defined CONFIG_ARCH_SUN55IW3
	value = hal_readb(0x02000380);
	/* workaround, need to config in config file */
	value |= (1 << 10);
	value &= ~(1 << 4);
	hal_writel(value, 0x02000380);
	value = hal_readb(0x02000384);
	value |= (1 << 4);
	hal_writel(value, 0x02000384);
#else
	hal_writel(0x135, 0x02000340);
#endif
#endif
	vin_log(VIN_LOG_POWER, "%s:id=%d, slave=0x%x\n", __func__, port, slave_addr);

	return 0;
}

int sunxi_twi_exit(int id)
{
	twi_port_t port;

	port = global_sensors[id].sensor_twi_id;
	hal_twi_uninit(port);

	return 0;
}

int vin_set_pmu_channel(int on)
{
	return 0;
}

int vin_gpio_set_status(int id, enum gpio_type gpio_id, unsigned int status)
{
	gpio_pin_t gpio;
	gpio_driving_level_t gpio_output_driving_level = GPIO_DRIVING_LEVEL1;

#if defined CONFIG_DRIVER_SYSCONFIG && defined CONFIG_VIN_USE_SYSCONFIG
	if (gpio_id == PWDN) {
		gpio = global_sensors[id].sensor_pin[PWDN];
		gpio_output_driving_level = global_sensors[id].sensor_pin_driving_level[PWDN];
	} else if (gpio_id == RESET) {
		gpio = global_sensors[id].sensor_pin[RESET];
		gpio_output_driving_level = global_sensors[id].sensor_pin_driving_level[RESET];
	} else if (gpio_id == IR_CUT0) {
		gpio = global_sensors[id].sensor_pin[IR_CUT0];
		gpio_output_driving_level = global_sensors[id].sensor_pin_driving_level[IR_CUT0];
	} else if (gpio_id == IR_CUT1) {
		gpio = global_sensors[id].sensor_pin[IR_CUT1];
		gpio_output_driving_level = global_sensors[id].sensor_pin_driving_level[IR_CUT1];
	} else if (gpio_id == IR_LED) {
		gpio = global_sensors[id].sensor_pin[IR_LED];
		gpio_output_driving_level = global_sensors[id].sensor_pin_driving_level[IR_LED];
	} else if (gpio_id == SM_VS) {
		gpio = global_sensors[id].sensor_pin[SM_VS];
		gpio_output_driving_level = global_sensors[id].sensor_pin_driving_level[SM_VS];
	} else
		return -1;
#else
	if (gpio_id == PWDN)
		gpio = global_sensors[id].pwdn_gpio;
	else if (gpio_id == RESET)
		gpio = global_sensors[id].reset_gpio;
	else if (gpio_id == IR_CUT0)
		gpio = global_sensors[id].ir_cut_gpio[0];
	else if (gpio_id == IR_CUT1)
		gpio = global_sensors[id].ir_cut_gpio[1];
	else if (gpio_id == IR_LED)
		gpio = global_sensors[id].ir_led_gpio;
	else
		return -1;
#endif

	if (gpio == 0xffff)
		return -1;

	if (status == 1) { //output
		hal_gpio_set_pull(gpio, GPIO_PULL_UP);
		hal_gpio_set_driving_level(gpio, gpio_output_driving_level);
		hal_gpio_set_direction(gpio, GPIO_DIRECTION_OUTPUT);
		hal_gpio_set_data(gpio, GPIO_DATA_LOW);
	} else if (status == 0) {
		hal_gpio_set_pull(gpio, GPIO_PULL_DOWN);
		hal_gpio_set_direction(gpio, GPIO_DIRECTION_INPUT);
		hal_gpio_set_data(gpio, GPIO_DATA_LOW);
	} else if (status == 2) { //smvs
		hal_gpio_pinmux_set_function(gpio, GPIO_MUXSEL_FUNCTION2);
	} else {
		hal_gpio_pinmux_set_function(gpio, GPIO_MUXSEL_DISABLED);
	}

	return 0;
}

int vin_gpio_write(int id, enum gpio_type gpio_id, unsigned int out_value)
{
	gpio_pin_t gpio;

#if defined CONFIG_DRIVER_SYSCONFIG && defined CONFIG_VIN_USE_SYSCONFIG
	if (gpio_id == PWDN)
		gpio = global_sensors[id].sensor_pin[PWDN];
	else if (gpio_id == RESET)
		gpio = global_sensors[id].sensor_pin[RESET];
	else if (gpio_id == IR_CUT0)
		gpio = global_sensors[id].sensor_pin[IR_CUT0];
	else if (gpio_id == IR_CUT1)
		gpio = global_sensors[id].sensor_pin[IR_CUT1];
	else if (gpio_id == IR_LED)
		gpio = global_sensors[id].sensor_pin[IR_LED];
	else
		return -1;
#else
	if (gpio_id == PWDN)
		gpio = global_sensors[id].pwdn_gpio;
	else if (gpio_id == RESET)
		gpio = global_sensors[id].reset_gpio;
	else if (gpio_id == IR_CUT0)
		gpio = global_sensors[id].ir_cut_gpio[0];
	else if (gpio_id == IR_CUT1)
		gpio = global_sensors[id].ir_cut_gpio[1];
	else if (gpio_id == IR_LED)
		gpio = global_sensors[id].ir_led_gpio;
	else
		return -1;
#endif

	if (gpio == 0xffff)
		return -1;

	hal_gpio_set_direction(gpio, GPIO_DIRECTION_OUTPUT);
	if (out_value) {
		hal_gpio_set_data(gpio, GPIO_DATA_HIGH);
	} else {
		hal_gpio_set_data(gpio, GPIO_DATA_LOW);
	}

	return 0;
}
#ifdef CONFIG_KERNEL_FREERTOS
int vin_set_mclk_freq(int id, unsigned long freq)
{
	hal_clk_t mclk_src = NULL;
	int mclk_id = global_sensors[id].mclk_id;

#if defined CONFIG_ARCH_SUN8IW20 || defined CONFIG_ARCH_SUN20IW3 || defined CONFIG_ARCH_SUN55IW3
	vind_default_mclk[mclk_id].mclk = hal_clock_get(HAL_SUNXI_CCU, vind_default_mclk[mclk_id].mclk_id);
#else
	vind_default_mclk[mclk_id].mclk = hal_clock_get(vind_default_mclk[mclk_id].mclk_type, vind_default_mclk[mclk_id].mclk_id);
#endif

	if (freq == 24000000 || freq == 12000000 || freq == 6000000) {
#if defined CONFIG_ARCH_SUN8IW20 || defined CONFIG_ARCH_SUN20IW3 || defined CONFIG_ARCH_SUN55IW3
		mclk_src = hal_clock_get(HAL_SUNXI_FIXED_CCU, vind_default_mclk[mclk_id].clk_24m_id);
#else
		mclk_src = hal_clock_get(vind_default_mclk[mclk_id].clk_24m_type, vind_default_mclk[mclk_id].clk_24m_id);
#endif
	} else {
#if defined CONFIG_ARCH_SUN8IW20 || defined CONFIG_ARCH_SUN20IW3 || defined CONFIG_ARCH_SUN55IW3
		mclk_src = hal_clock_get(HAL_SUNXI_CCU, vind_default_mclk[mclk_id].clk_pll_id);
#else
		mclk_src = hal_clock_get(vind_default_mclk[mclk_id].clk_pll_type, vind_default_mclk[mclk_id].clk_pll_id);
#endif
	}

	if (hal_clk_set_parent(vind_default_mclk[mclk_id].mclk, mclk_src)) {
		vin_err("set mclk%d source failed!\n", mclk_id);
		return -1;
	}
	if (hal_clk_set_rate(vind_default_mclk[mclk_id].mclk, freq)) {
		vin_err("set csi master%d clock error\n", mclk_id);
		return -1;
	}
	vin_log(VIN_LOG_POWER, "mclk%d set rate %ld, get rate %u\n", mclk_id,
		freq, hal_clk_get_rate(vind_default_mclk[mclk_id].mclk));
	hal_clock_put(mclk_src);

	return 0;
}
#else
int vin_set_mclk_freq(int id, unsigned long freq)
{
	hal_clk_t mclk_src = 0;
	int mclk_id = global_sensors[id].mclk_id;

	if (freq == 24000000 || freq == 12000000 || freq == 6000000) {
		mclk_src = vind_default_mclk[mclk_id].clk_24m;
	} else {
		mclk_src = vind_default_mclk[mclk_id].clk_pll;
	}

	if (hal_clk_set_parent(vind_default_mclk[mclk_id].mclk, mclk_src)) {
		vin_err("set mclk%d source failed!\n", mclk_id);
		return -1;
	}
	if (hal_clk_set_rate(vind_default_mclk[mclk_id].mclk, freq)) {
		vin_err("set csi master%d clock error\n", mclk_id);
		return -1;
	}
	vin_log(VIN_LOG_POWER, "mclk%d set rate %ld, get rate %ld\n", mclk_id,
		freq, hal_clk_get_rate(vind_default_mclk[mclk_id].mclk));

	return 0;
}
#endif

int vin_set_mclk(int id, unsigned int on_off)
{
	struct vin_mclk_info *mclk = NULL;
	int mclk_id = global_sensors[id].mclk_id;

	mclk = &vind_default_mclk[mclk_id];

	if (on_off && mclk->use_count++ > 0)
		return 0;
	else if (!on_off && (mclk->use_count == 0 || --mclk->use_count > 0))
		return 0;

	switch (on_off) {
	case 1:
		vin_log(VIN_LOG_POWER, "sensor mclk on, use_count %d!\n", mclk->use_count);
		hal_gpio_pinmux_set_function(mclk->pin, mclk->pin_func[0]);
		if (mclk->mclk) {
			if (hal_clock_enable(mclk->mclk)) {
				vin_err("csi master clock enable error\n");
				return -1;
			}
		} else {
			vin_err("csi master%d clock is null\n", mclk_id);
			return -1;
		}
		break;
	case 0:
		hal_gpio_pinmux_set_function(mclk->pin, mclk->pin_func[1]);
		vin_log(VIN_LOG_POWER, "sensor mclk off, use_count %d!\n", mclk->use_count);
		if (mclk->mclk) {
			hal_clock_disable(mclk->mclk);
			hal_clock_put(mclk->mclk);
		} else {
			vin_err("csi master%d clock is null\n", mclk_id);
			return -1;
		}
		break;
	default:
		return -1;
	}

	return 0;
}

#ifdef CONFIG_TDM_RX_BUF_NUM_WITH_TWORX
int vin_sync_ctrl(int id, struct csi_sync_ctrl *sync)
{
	if (!sync->type) {
		csic_prs_sync_en_cfg(global_video[id].csi_sel, sync);
		csic_prs_sync_cfg(global_video[id].csi_sel, sync);
		csic_prs_sync_wait_N(global_video[id].csi_sel, sync);
		csic_prs_sync_wait_M(global_video[id].csi_sel, sync);
		csic_frame_cnt_enable(global_video[id].vipp_sel);
		csic_dma_frm_cnt(global_video[id].vipp_sel, sync);
		csic_prs_sync_en(global_video[id].csi_sel, sync);
	} else {
		csic_prs_xs_en(id, sync);
		csic_prs_xs_period_len_register(id, sync);
	}
	return 0;
}
#endif

unsigned int vin_set_large_overlayer(unsigned int width)
{
	unsigned int max_overlayer = 0,  min_overlayer = 0;
	unsigned int overlayer = 0;
	unsigned align = 16;
	unsigned int max_width = ISP_MAX_WIDTH;

	min_overlayer = ALIGN(width / 61, align) * 3;
	if (min_overlayer / 2 + width / 2 > max_width) {
		vin_err("min overlayer/2 + width/2 is more than max_width, %d + %d > %d\n", min_overlayer / 2, width / 2, max_width);
		return overlayer;
	}

	max_overlayer = ALIGN(width / 57, align) * 7;
	overlayer = max_overlayer;
	while (overlayer / 2 + width / 2 > max_width)
		overlayer -= align / 2;
	vin_log(VIN_LOG_FMT, "max_overlayer/2 is %d, min_overlayer/2 is %d, overlayer/2 is %d\n", max_overlayer / 2, min_overlayer / 2, overlayer / 2);

	if (overlayer == 0)
		overlayer = 256;

	return overlayer / 2;
}

struct sensor_format_struct *sensor_find_format(int isp_id, int vinc_id,
	struct sensor_format_struct *sensor_formats, int format_size)
{
	int ispid = clamp(isp_id, 0, ISP_GET_CFG_NUM - 1);
	struct vin_core *vinc = &global_video[vinc_id];
	struct sensor_format_struct *sensor_format = NULL;
	int wdr_on = isp_get_cfg[ispid].sensor_wdr_on;
	int fps = isp_get_cfg[ispid].sensor_get_fps;
	int i, dist, best_dist = INT_MAX;

#if defined CONFIG_COMPONENTS_RPBUF_UART
	for (i = 0; i < format_size; i++) {
		if (sensor_formats[i].wdr_mode == wdr_on) {
			if (sensor_formats[i].fps_fixed == fps) {
				dist = abs(sensor_formats[i].width - vinc->o_width) +
					abs(sensor_formats[i].height - vinc->o_height);
				if ((dist < best_dist) &&
					(sensor_formats[i].width == vinc->o_width) &&
					(sensor_formats[i].height == vinc->o_height)) {
						best_dist = dist;
						sensor_format = &sensor_formats[i];
				}
			}
		}
	}

	best_dist = INT_MAX;
#endif
	if (sensor_format == NULL) {
		for (i = 0; i < format_size; i++) {
			if (sensor_formats[i].wdr_mode == wdr_on) {
				if (sensor_formats[i].fps_fixed == fps) {
					dist = abs(sensor_formats[i].width - vinc->o_width) +
						abs(sensor_formats[i].height - vinc->o_height);
					if ((dist < best_dist) &&
						(sensor_formats[i].width >= vinc->o_width) &&
						(sensor_formats[i].height >= vinc->o_height)) {
							best_dist = dist;
							sensor_format = &sensor_formats[i];
					}
				}
			}
		}
	}

	best_dist = INT_MAX;
	if (sensor_format == NULL) {
		for (i = 0; i < format_size; i++) {
			if (sensor_formats[i].wdr_mode == wdr_on) {
				dist = abs(sensor_formats[i].width - vinc->o_width) +
					abs(sensor_formats[i].height - vinc->o_height);
				if ((dist < best_dist) &&
					(sensor_formats[i].width >= vinc->o_width) &&
					(sensor_formats[i].height >= vinc->o_height)) {
						best_dist = dist;
						sensor_format = &sensor_formats[i];
						isp_get_cfg[ispid].sensor_get_fps = sensor_format->fps_fixed;
				}
			}
		}
	}

	best_dist = INT_MAX;
	if (sensor_format == NULL) {
		for (i = 0; i < format_size; i++) {
			if (sensor_formats[i].fps_fixed == wdr_on) {
				dist = abs(sensor_formats[i].width - vinc->o_width) +
					abs(sensor_formats[i].height - vinc->o_height);
				if ((dist < best_dist) &&
					(sensor_formats[i].width >= vinc->o_width) &&
					(sensor_formats[i].height >= vinc->o_height)) {
						best_dist = dist;
					sensor_format = &sensor_formats[i];
					isp_get_cfg[ispid].sensor_wdr_on = sensor_format->wdr_mode;
				}
			}
		}
	}

	best_dist = INT_MAX;
	if (sensor_format == NULL) {
		for (i = 0; i < format_size; i++) {
			dist = abs(sensor_formats[i].width - vinc->o_width) +
				abs(sensor_formats[i].height - vinc->o_height);
			if ((dist < best_dist) &&
				(sensor_formats[i].width >= vinc->o_width) &&
				(sensor_formats[i].height >= vinc->o_height)) {
					best_dist = dist;
					sensor_format = &sensor_formats[i];
				}
		}

		isp_get_cfg[ispid].sensor_wdr_on = sensor_format->wdr_mode;
		isp_get_cfg[ispid].sensor_get_fps = sensor_format->fps_fixed;
	}

	return sensor_format;
}
