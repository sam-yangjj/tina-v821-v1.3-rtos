/*
 * sunxi_isp.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <hal_mem.h>
#include <hal_interrupt.h>
#include <hal_timer.h>

#include "../vin.h"
#include "../platform/platform_cfg.h"
#include "sunxi_isp.h"
#include "../vin_csi/sunxi_csi.h"
#include "../vin_vipp/sunxi_scaler.h"
#include "../vin_video/vin_core.h"
#include "../utility/vin_io.h"
#include "isp_default_tbl.h"

#define MIN_IN_WIDTH			192
#define MIN_IN_HEIGHT			128
#define MAX_IN_WIDTH			4224
#define MAX_IN_HEIGHT			4224

struct isp_dev *global_isp[VIN_MAX_ISP];
extern struct vin_core *global_vinc[VIN_MAX_VIDEO];
extern struct csi_dev *global_csi[VIN_MAX_CSI];
bool isp_probe_flag[VIN_MAX_ISP];

#define ISP_USE_IRQ
#define ISP_SERVER_USE
#ifndef CONFIG_ARCH_SUN55IW3
#define ISP_WAIT_SRAM
#endif

static struct isp_pix_fmt sunxi_isp_formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_SBGGR8,
		.mbus_code = MEDIA_BUS_FMT_SBGGR8_1X8,
		.infmt = ISP_BGGR,
		.input_bit = RAW_8,
	}, {
		.fourcc = V4L2_PIX_FMT_SGBRG8,
		.mbus_code = MEDIA_BUS_FMT_SGBRG8_1X8,
		.infmt = ISP_GBRG,
		.input_bit = RAW_8,
	}, {
		.fourcc = V4L2_PIX_FMT_SGRBG8,
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
		.infmt = ISP_GRBG,
		.input_bit = RAW_8,
	}, {
		.fourcc = V4L2_PIX_FMT_SRGGB8,
		.mbus_code = MEDIA_BUS_FMT_SRGGB8_1X8,
		.infmt = ISP_RGGB,
		.input_bit = RAW_8,
	}, {
		.fourcc = V4L2_PIX_FMT_SBGGR10,
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.infmt = ISP_BGGR,
		.input_bit = RAW_10,
	}, {
		.fourcc = V4L2_PIX_FMT_SGBRG8,
		.mbus_code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.infmt = ISP_GBRG,
		.input_bit = RAW_10,
	}, {
		.fourcc = V4L2_PIX_FMT_SGRBG10,
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.infmt = ISP_GRBG,
		.input_bit = RAW_10,
	}, {
		.fourcc = V4L2_PIX_FMT_SRGGB10,
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
		.infmt = ISP_RGGB,
		.input_bit = RAW_10,
	}, {
		.fourcc = V4L2_PIX_FMT_SBGGR12,
		.mbus_code = MEDIA_BUS_FMT_SBGGR12_1X12,
		.infmt = ISP_BGGR,
		.input_bit = RAW_12,
	}, {
		.fourcc = V4L2_PIX_FMT_SGBRG12,
		.mbus_code = MEDIA_BUS_FMT_SGBRG12_1X12,
		.infmt = ISP_GBRG,
		.input_bit = RAW_12,
	}, {
		.fourcc = V4L2_PIX_FMT_SGRBG12,
		.mbus_code = MEDIA_BUS_FMT_SGRBG12_1X12,
		.infmt = ISP_GRBG,
		.input_bit = RAW_12,
	}, {
		.fourcc = V4L2_PIX_FMT_SRGGB12,
		.mbus_code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.infmt = ISP_RGGB,
		.input_bit = RAW_12,
	},
};

static int isp_3d_pingpong_alloc(struct isp_dev *isp)
{
	return 0;
}
static void isp_3d_pingpong_free(struct isp_dev *isp)
{

}
static int isp_3d_pingpong_update(struct isp_dev *isp)
{
	return 0;
}

static struct isp_pix_fmt *__isp_try_format(unsigned int id, unsigned int vinc_id)
{
	struct isp_dev *isp = global_isp[id];
	struct isp_pix_fmt *isp_fmt = NULL;
	struct isp_size_settings *ob = &isp->isp_ob;
	unsigned int i;
	struct sensor_format_struct *sensor_format;
	int sensor_id = global_video[vinc_id].rear_sensor;

	if (global_sensors[sensor_id].sensor_core->sensor_g_format)
		sensor_format = global_sensors[sensor_id].sensor_core->sensor_g_format(sensor_id, id, vinc_id);
	else
		return NULL;

	for (i = 0; i < ARRAY_SIZE(sunxi_isp_formats); ++i)
		if (sensor_format->mbus_code == sunxi_isp_formats[i].mbus_code)
			isp_fmt = &sunxi_isp_formats[i];

	if (isp_fmt == NULL)
		isp_fmt = &sunxi_isp_formats[0];

	if (isp->large_image)
		ob->ob_black.width = sensor_format->width / 2 + vin_set_large_overlayer(sensor_format->width);
	else
		ob->ob_black.width = sensor_format->width;
	ob->ob_black.height = sensor_format->height;

	/*if (isp->id == 1) {
		sensor_format->width = clamp(sensor_format->width, MIN_IN_WIDTH, 3264);
		sensor_format->height = clamp(sensor_format->height, MIN_IN_HEIGHT, 3264);
	} else {
		sensor_format->width = clamp(sensor_format->width, MIN_IN_WIDTH, 4224);
		sensor_format->height = clamp(sensor_format->height, MIN_IN_HEIGHT, 4224);
	}*/

	if (isp->large_image)
		ob->ob_valid.width = sensor_format->width / 2 + vin_set_large_overlayer(sensor_format->width);
	else
		ob->ob_valid.width = sensor_format->width;
	ob->ob_valid.height = sensor_format->height;
	ob->ob_start.hor = (ob->ob_black.width - ob->ob_valid.width) / 2;
	ob->ob_start.ver = (ob->ob_black.height - ob->ob_valid.height) / 2;

	return isp_fmt;
}

static int sunxi_isp_logic_s_stream(unsigned int virtual_id, int on)
{
	unsigned char logic_id = isp_virtual_find_logic[virtual_id];
	struct isp_dev *logic_isp = global_isp[logic_id];

	if (logic_isp->work_mode == ISP_ONLINE && virtual_id != logic_id) {
		vin_err("isp%d work on online mode, isp%d cannot to work!!\n", logic_id, virtual_id);
		return -1;
	}

	if (on && (logic_isp->logic_top_stream_count)++ > 0)
		return 0;
	else if (!on && (logic_isp->logic_top_stream_count == 0 || --(logic_isp->logic_top_stream_count) > 0))
		return 0;

	if (on) {
#ifndef CONFIG_ARCH_SUN55IW3
#ifdef ISP_WAIT_SRAM
		int i = 0, timeout_cnt = 2000;
		for (i = 0; i < timeout_cnt; i++) { //wait (timeout_cnt/1000)s
#if defined CONFIG_ARCH_SUN20IW3
			if (hal_readl(rtc_base + 0x100 + 0x4 * RTC_NUM) & 0x1) /* wait for boot0 sign out*/
#else
			if (hal_readl(rtc_base) & 0x1) /* wait for boot0 sign out*/
#endif
				break;
			hal_msleep(1);
		}
		/*after receiving the flag, it is necessary to delay 100 us to recycle the sram,
		 * because at this time boot0 will still run a few beats of assembly code
		 */
		hal_usleep(100);
		if (i >= timeout_cnt)
#if defined CONFIG_ARCH_SUN20IW3
			vin_err("isp wait 2s but rtc%d bit 0 not set to 1, sram cannot give back isp, read rtc%d val is 0x%x\n", RTC_NUM, RTC_NUM, hal_readl(rtc_base + 0x100 + 0x4 * RTC_NUM));
#else
			vin_err("isp wait 2s but rtc bit 0 not set to 1, sram cannot give back isp, read rtc val is 0x%x\n", hal_readl(rtc_base) & 0x1);
#endif
		else
			bsp_isp_sram_boot_mode_ctrl(logic_isp->id, SRAM_NORMAL_MODE); /* boot0 sram give back isp*/
#else
		bsp_isp_sram_boot_mode_ctrl(logic_isp->id, SRAM_NORMAL_MODE); /* boot0 sram give back isp*/
#endif
#endif
		bsp_isp_enable(logic_isp->id, on);
		bsp_isp_mode(logic_isp->id, logic_isp->work_mode);
		//bsp_isp_set_clk_back_door(logic_isp->id, on);
		bsp_isp_top_capture_start(logic_isp->id);
	} else {
		bsp_isp_top_capture_stop(logic_isp->id);
		bsp_isp_enable(logic_isp->id, on);
#ifdef ISP_WAIT_SRAM
		bsp_isp_sram_boot_mode_ctrl(logic_isp->id, SRAM_BOOT_MODE);
#endif
		/* in offline mode, top tdm must close after top isp, otherwise isp int will occur err*/
		if (logic_isp->work_mode == ISP_OFFLINE) {
			csic_tdm_set_speed_dn(logic_id, 0);
			csic_tdm_tx_cap_disable(logic_id);
			csic_tdm_top_disable(logic_id);
			csic_tdm_tx_disable(logic_id);
			csic_tdm_disable(logic_id);
		}
	}
	return 0;
}

static void sunxi_isp_set_load_ddr(struct isp_dev *isp)
{
#if 0
	struct v4l2_mbus_framefmt *mf = &isp->mf;

	bsp_isp_set_line_int_num(isp->id, mf->height - 1);
#endif
	bsp_isp_set_input_fmt(isp->id, isp->isp_fmt->infmt);
	bsp_isp_set_size(isp->id, &isp->isp_ob);
	bsp_isp_set_last_blank_cycle(isp->id, 5);
	bsp_isp_set_speed_mode(isp->id, 3);
#if defined (CONFIG_ARCH_SUN55IW3) || defined (CONFIG_ARCH_SUN20IW3)
	bsp_isp_set_save_load_addr(isp->id, (unsigned long)isp->isp_save_load.phy_addr);
#endif
	if (isp->wdr_mode == ISP_NORMAL_MODE)
		bsp_isp_set_ch_input_bit(isp->id, 0, isp->isp_fmt->input_bit);
	else if (isp->wdr_mode == ISP_DOL_WDR_MODE) {
		bsp_isp_set_ch_input_bit(isp->id, 0, isp->isp_fmt->input_bit);
		bsp_isp_set_ch_input_bit(isp->id, 1, isp->isp_fmt->input_bit);
	}
}

int sunxi_isp_subdev_s_stream(unsigned int id, unsigned int vinc_id, int enable)
{
	struct isp_dev *isp = global_isp[id];
	struct mbus_framefmt_res *res = isp->res;
	unsigned int load_val;
	int *stream_count;
	struct isp_part_para isp_part;

	stream_count = &isp->stream_count;
	if (enable && (*stream_count)++ > 0)
		return 0;
	else if (!enable && (*stream_count == 0 || --(*stream_count) > 0))
		return 0;

	if (!isp->use_isp)
		return 0;

	switch (global_video[vinc_id].fourcc) {
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
		vin_log(VIN_LOG_FMT, "%s output fmt is raw, return directly\n", __func__);
		if (isp->isp_dbg.debug_en) {
			bsp_isp_debug_output_cfg(isp->id, 1, isp->isp_dbg.debug_sel);
			break;
		} else {
			return 0;
		}
	default:
		break;
	}

	if (enable) {
		isp->large_image = global_video[vinc_id].large_image;
#ifdef ISP_SERVER_USE
		if (!isp->isp_server_init) {
			isp_part.flicker_mode = FREQUENCY_AUTO;
			isp_part.ir_mode = 0;
			if (!vin_server_init(isp, isp_part))
				isp->isp_server_init = 1;
		}
#endif
		if (isp->wdr_mode != res->res_wdr_mode) {
			isp->wdr_mode = res->res_wdr_mode;
			memcpy(isp->isp_load.phy_addr, &isp_default_reg[0], ISP_LOAD_REG_SIZE);
		}

		isp->init_done_flag = 0;

		if (isp->load_flag)
			memcpy(isp->isp_load.phy_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);

		isp->isp_fmt = __isp_try_format(isp->id, vinc_id);

		if (isp_3d_pingpong_alloc(isp))
			return -1;
		isp_3d_pingpong_update(isp);

		if (sunxi_isp_logic_s_stream(isp->id, enable))
			return -1;

#ifdef ISP_USE_IRQ
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_irq_enable(isp->id, PARA_LOAD_INT_EN | PARA_SAVE_INT_EN);
		bsp_isp_irq_enable(isp->id, DDR_RW_ERROR_INT_EN | HB_SHORT_INT_EN | CFG_ERROR_INT_EN | FRAME_LOST_INT_EN |
						INTER_FIFO_FULL_INT_EN | WDMA_FIFO_FULL_INT_EN | WDMA_OVER_BND_INT_EN |
						RDMA_FIFO_FULL_INT_EN | LBC_ERROR_INT_EN | AHB_MBUS_W_INT_EN);
		hal_enable_irq(isp->irq);
#endif

		load_val = bsp_isp_load_update_flag(isp->id);
#if !defined CONFIG_D3D
		bsp_isp_module_disable(isp->id, D3D_EN);
		load_val = load_val & ~D3D_UPDATE;
#endif
		load_val = load_val | (GAMMA_UPDATE | CEM_UPDATE);
		bsp_isp_update_table(isp->id, load_val);

		/* load addr*/
		sunxi_isp_set_load_ddr(isp);

		memcpy(isp->load_para[0].phy_addr, (unsigned long *)isp->isp_load.phy_addr, ISP_LOAD_DRAM_SIZE);
		hal_dcache_clean((unsigned long)isp->load_para[0].phy_addr, ISP_LOAD_DRAM_SIZE);

		bsp_isp_set_para_ready(isp->id, PARA_READY);
		bsp_isp_capture_start(isp->id);

	} else {
		bsp_isp_capture_stop(isp->id);
#ifdef ISP_USE_IRQ
		hal_disable_irq(isp->irq);
		bsp_isp_irq_disable(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
#endif
		hal_workqueue_cancel_work_sync(isp->isp_server_run_task, &isp->start_isp_server_work);
		sunxi_isp_logic_s_stream(isp->id, enable);

		isp_3d_pingpong_free(isp);
#ifdef ISP_SERVER_USE
#ifndef ISP_SERVER_FASTINIT
		if (isp->isp_server_init) {
			//isp_sensor_otp_exit(isp);
			isp_server_exit(&isp->hw_isp, isp->id);
			isp_free_reg_tbl(isp);
			isp->isp_server_init = 0;
		}
#endif
#endif
	}

	vin_log(VIN_LOG_FMT, "isp%d %s, %d*%d hoff: %u voff: %u\n",
		isp->id, enable ? "stream on" : "stream off",
		isp->isp_ob.ob_valid.width, isp->isp_ob.ob_valid.height,
		isp->isp_ob.ob_start.hor, isp->isp_ob.ob_start.ver);

	return 0;
}

int sunxi_isp_subdev_init(unsigned int id)
{
	struct isp_dev *isp = global_isp[id];

	if (!isp->use_isp)
		return 0;

	if (!isp->have_init) {
		memset(isp->isp_load.phy_addr, 0, ISP_LOAD_REG_SIZE);
		memcpy(isp->isp_load.phy_addr, &isp_default_reg[0], ISP_LOAD_REG_SIZE);
		hal_dcache_clean((unsigned long)isp->isp_load.phy_addr, ISP_LOAD_REG_SIZE);
		memset(&isp->load_shadow[0], 0, ISP_LOAD_DRAM_SIZE);
		isp->load_flag = 0;
		isp->have_init = 1;
	}
	isp->isp_frame_number = 0;
	isp->h3a_stat.buf[0].empty = 1;
	isp->h3a_stat.buf[0].state = ISPSTAT_IDLE; //ISPSTAT_LOAD_SET
	isp->h3a_stat.buf[0].phy_addr = isp->isp_stat.phy_addr;
	isp->h3a_stat.buf[0].id = 0;
	bsp_isp_set_statistics_addr(isp->id, (unsigned long)isp->isp_stat.phy_addr);
	bsp_isp_set_saved_addr(isp->id, (unsigned long)isp->isp_save.phy_addr);
	isp->load_select = false;
	bsp_isp_set_load_addr(isp->id, (unsigned long)isp->load_para[0].phy_addr);
	bsp_isp_set_save_load_addr(isp->id, (unsigned long)isp->isp_save_load.phy_addr);
	bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);

	return 0;
}

#ifdef ISP_USE_IRQ
/*
 * must reset all the pipeline through isp.
 */
__attribute__((__unused__)) static void __sunxi_isp_reset(struct isp_dev *isp)
{
	struct prs_cap_mode mode = {.mode = VCAP};
	struct vin_core *vinc = NULL;
	struct csi_dev *csi = NULL;
	int i;

	if (!isp->use_isp)
		return;

	if (!isp->stream_count) {
		vin_err("isp%d is not used, cannot be resetted!!!\n", isp->id);
		return;
	}

	if (isp->work_mode == ISP_OFFLINE)
		return;

	for (i = 0; i < VIN_MAX_VIDEO; i++) {
		if (global_vinc[i]->used == 0)
			continue;
		if (global_vinc[i]->stream_count == 0)
			continue;
		if (global_vinc[i]->isp_sel == isp->id) {
			vinc = global_vinc[i];
			break;
		}
	}
	vin_print("isp:%d reset , frame number %ld!!\n", isp->id, isp->isp_frame_number);
	bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);

	/*****************stop*******************/
	csic_prs_capture_stop(vinc->csi_sel);
	csic_prs_disable(vinc->csi_sel);
	csic_isp_bridge_disable();
	bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
	bsp_isp_capture_stop(isp->id);
	bsp_isp_top_capture_stop(isp->id);
	bsp_isp_enable(isp->id, 0);
	vipp_chn_cap_disable(vinc->vipp_sel);
	vipp_cap_disable(vinc->vipp_sel);
	vipp_clear_status(vinc->vipp_sel, VIPP_STATUS_ALL);
	vipp_top_clk_en(vinc->vipp_sel, 0);
	csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
	csic_dma_top_disable(vinc->vipp_sel);

	/*****************start*******************/
	csic_dma_top_enable(vinc->vipp_sel);
	vipp_clear_status(vinc->vipp_sel, VIPP_STATUS_ALL);
	vipp_cap_enable(vinc->vipp_sel);
	vipp_top_clk_en(vinc->vipp_sel, 1);
	vipp_chn_cap_enable(vinc->vipp_sel);
	bsp_isp_enable(isp->id, 1);
	bsp_isp_top_capture_start(isp->id);
	bsp_isp_set_input_fmt(isp->id, isp->isp_fmt->infmt); //update load&save_load input_fmt when flip
	memcpy(isp->load_para[0].phy_addr, isp->isp_load.phy_addr, ISP_LOAD_DRAM_SIZE);
	memcpy(isp->load_para[1].phy_addr, isp->isp_load.phy_addr, ISP_LOAD_DRAM_SIZE);
	bsp_isp_set_para_ready(isp->id, PARA_READY);
	bsp_isp_capture_start(isp->id);
	isp->isp_frame_number = 0;
	csic_isp_bridge_enable();
	csic_prs_enable(vinc->csi_sel);
	csi = global_csi[vinc->csi_sel];
	csic_prs_capture_start(vinc->csi_sel, csi->bus_info.ch_total_num, &mode);
}
#endif

static int isp_resource_alloc(unsigned int id)
{
	struct isp_dev *isp = global_isp[id];

	isp->isp_stat.size = ISP_SAVE_DRAM_SIZE + ISP_SAVE_LOAD_DRAM_SIZE + 3 * ISP_LOAD_DRAM_SIZE;
#ifdef CONFIG_KERNEL_FREERTOS
	isp->isp_stat.phy_addr = memheap_alloc_align(&isp_mempool, isp->isp_stat.size, 0x1000);
#else
	isp->isp_stat.phy_addr = rt_memheap_alloc_align(&isp_mempool, isp->isp_stat.size, 0x1000);
#endif
	if (isp->isp_stat.phy_addr == NULL) {
		vin_err("%s:isp%d:alloc isp buffer error\n", __func__, id);
		return -1;
	}

	isp->isp_save.phy_addr = isp->isp_stat.phy_addr;
	isp->isp_save_load.phy_addr = isp->isp_stat.phy_addr + ISP_STAT_TOTAL_SIZE;
	isp->isp_load.phy_addr = isp->isp_stat.phy_addr + ISP_STAT_TOTAL_SIZE + ISP_SAVE_LOAD_DRAM_SIZE;
	isp->load_para[0].phy_addr = isp->isp_load.phy_addr + ISP_LOAD_DRAM_SIZE;
	isp->load_para[1].phy_addr = isp->isp_load.phy_addr + ISP_LOAD_DRAM_SIZE * 2;

	memset(isp->isp_stat.phy_addr, 0, isp->isp_stat.size);

	return 0;
}

#ifdef ISP_USE_IRQ
static void __isp_server_run_handle(hal_work *work, void* work_data)
{
	struct isp_dev *isp = (struct isp_dev *)work_data;
	static unsigned char ir_delay_cnt = 0;

	if (!isp->stream_count) {
		vin_err("isp%d is not used, cannot handle run server!!!\n", isp->id);
		return;
	}

	//if (isp->isp_frame_number < 10)
	//	return;

	if (!isp->isp_server_init)
		return;

	if (isp_stat_request_statistics(&isp->h3a_stat)) {
		return;
	}
	if (ir_delay_cnt) {
		ir_delay_cnt--;
		hal_interrupt_enable_irqrestore(hal_interrupt_disable_irqsave());
	} else {
		isp_stats_process(isp->hw_isp, (void *)isp->isp_stat_buf.phy_addr);
	}

	isp_sensor_set_exp_gain(isp);
	isp_set_load_reg(isp);
	if (!ir_delay_cnt)
		ir_delay_cnt = isp_sensor_set_ir_status(isp);
}

void sunxi_isp_frame_sync_isr(struct isp_dev *isp)
{
	static short isp_log_param;
	unsigned int data[4];
	bool send_event = 0;

	if (!isp->isp_server_init)
		return;

	data[0] = 1;/*load type (0: load separate; 1: load together)*/
	switch (isp->colorspace) {
	case V4L2_COLORSPACE_REC709:
		data[1] = 1;
		break;
	case V4L2_COLORSPACE_BT2020:
		data[1] = 2;
		break;
	case V4L2_COLORSPACE_REC709_PART_RANGE:
		data[1] = 4;
		break;
	default:
		data[1] = 0;
		break;
	}
	if (isp_log_param != (vin_log_mask >> 16)) {
		isp_log_param = vin_log_mask >> 16;
		send_event = 1;
	} else {
		send_event = 0;
	}
	data[2] = isp_log_param;
	data[3] = isp_log_param >> 8;
	if ((isp->h3a_stat.frame_number < 2) || send_event)
		isp_fsync_process(isp->hw_isp, data);

	isp_stat_isr(&isp->h3a_stat);

	hal_workqueue_dowork(isp->isp_server_run_task, &isp->start_isp_server_work);
}

static hal_irqreturn_t isp_isr(void *priv)
{
	struct isp_dev *isp = (struct isp_dev *)priv;
	unsigned int load_val;
	uint32_t __cpsr;

	if (!isp->use_isp)
		return 0;

	//vin_log(VIN_LOG_ISP, "isp%d interrupt, status is 0x%x!!!\n", isp->id,
	//	bsp_isp_get_irq_status(isp->id, ISP_IRQ_STATUS_ALL));

	__cpsr = hal_spin_lock_irqsave(&isp->slock);

	if (bsp_isp_get_irq_status(isp->id, DDR_RW_ERROR_PD)) {
		vin_err("isp%d ddr read and write error!\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, DDR_RW_ERROR_PD);
	}
	if (bsp_isp_get_irq_status(isp->id, LBC_ERROR_PD)) {
		vin_err("isp%d lbc decompress error\n", isp->id);
		if (bsp_isp_get_internal_status1(isp->id, LBC_DEC_LONG_ERROR))
			vin_err("isp%d the data for LBC decode is too long\n", isp->id);
		if (bsp_isp_get_internal_status1(isp->id, LBC_DEC_SHORT_ERROR))
			vin_err("isp%d the data for LBC decode is too short\n", isp->id);
		if (bsp_isp_get_internal_status1(isp->id, LBC_DEC_ERROR))
			vin_err("isp%d the data for LBC decode is error\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, LBC_ERROR_PD);
		__sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, AHB_MBUS_W_PD)) {
		vin_err("isp%d AHB and MBUS write conflict\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, AHB_MBUS_W_PD);
	}
	if (bsp_isp_get_irq_status(isp->id, HB_SHORT_PD)) {
		vin_err("isp%d hblank short, hblank need morn than 128 cycles!\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, HB_SHORT_PD);
		__sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, FRAME_LOST_PD)) {
		vin_err("isp%d frame lost!\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, FRAME_LOST_PD);
		__sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, CFG_ERROR_PD)) {
		vin_err("isp%d configuration error\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, CFG_ERROR_PD);
		if (bsp_isp_get_internal_status0(isp->id, TWO_BYTE_ERROR)) {
			vin_err("isp%d two byte input mode encountered\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, TWO_BYTE_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, FMT_CHG_ERROR)) {
			vin_err("isp%d the bayer format is changed when d3d is open\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, FMT_CHG_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, CH0_BTYPE_ERROR)) {
			vin_err("isp%d the bytpe signal of channel 0 is changed\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, CH0_BTYPE_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, CH1_BTYPE_ERROR)) {
			vin_err("isp%d the bytpe signal of channel 1 is changed\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, CH1_BTYPE_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, CH2_BTYPE_ERROR)) {
			vin_err("isp%d the bytpe signal of channel 2 is changed\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, CH2_BTYPE_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, DVLD_ERROR)) {
			vin_err("isp%d the dvld signal is not aligned among different input channels\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, DVLD_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, HSYNC_ERROR)) {
			vin_err("isp%d the input hsync signal is not matched with the input channel cfg\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, HSYNC_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, VSYNC_ERROR)) {
			vin_err("isp%d the input vsync signal is not matched with the input channel cfg\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, VSYNC_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, WIDTH_ERROR)) {
			vin_err("isp%d width error\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WIDTH_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, HEIGHT_ERROR)) {
			vin_err("isp%d height error\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, HEIGHT_ERROR);
		}
		__sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, INTER_FIFO_FULL_PD)) {
		vin_err("isp%d internal fifo full\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, INTER_FIFO_FULL_PD);
		if (bsp_isp_get_internal_status2(isp->id, LCA_BYR_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of LCA_BYR_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, LCA_BYR_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, LCA_BYR_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of LCA_BYR_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, LCA_BYR_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, LCA_RGB0_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of LCA_RGB0_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, LCA_RGB0_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, LCA_RGB0_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of LCA_RGB0_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, LCA_RGB0_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_AVG_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of DMSC_AVG_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_AVG_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_AVG_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of DMSC_AVG_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_AVG_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_RGB_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of DMSC_RGB_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_RGB_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_RGB_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of DMSC_RGB_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_RGB_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_RATIO_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of DMSC_RATIO_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_RATIO_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_RATIO_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of DMSC_RATIO_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_RATIO_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_DIFF_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_DIFF_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_DIFF_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_LP0_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_LP0_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_LP0_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_LP1_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_LP1_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_LP1_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_PCNT_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_PCNT_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_PCNT_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_KPACK_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_KPACK_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_KPACK_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_CRKB_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_CRKB_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_CRKB_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_LBC_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_LBC_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_LBC_FIFO_OV);
		}
		__sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, WDMA_FIFO_FULL_PD)) {
		vin_err("isp%d wdma fifo full\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, WDMA_FIFO_FULL_PD);
		if (bsp_isp_get_internal_status0(isp->id, WDMA_PLTM_LUM_FIFO_OV)) {
			vin_err("isp%d wdma fifo of pltm lum fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_PLTM_LUM_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_PLTM_PKX_FIFO_OV)) {
			vin_err("isp%d wdma fifo of pltm ptk fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_PLTM_PKX_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_D3D_REC_FIFO_OV)) {
			vin_err("isp%d wdma fifo of d3d rec fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_D3D_REC_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_D3D_KSTB_FIFO_OV)) {
			vin_err("isp%d wdma fifo of d3d ktsb fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_D3D_KSTB_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_D3D_CNTR_FIFO_OV)) {
			vin_err("isp%d wdma fifo of d3d cntr fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_D3D_CNTR_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_D3D_CURK_FIFO_OV)) {
			vin_err("isp%d wdma fifo of d3d curk fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_D3D_CURK_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_LBC_H4DDR_FIFO_OV)) {
			vin_err("isp%d wdma fifo of lbc head for ddr output fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_LBC_H4DDR_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_LBC_H4DT_FIFO_OV)) {
			vin_err("isp%d wdma fifo of lbc head for data fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_LBC_H4DT_FIFO_OV);
		}
		__sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, WDMA_OVER_BND_PD)) {
		vin_err("isp%d wdma over border\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, WDMA_OVER_BND_PD);
		__sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, RDMA_FIFO_FULL_PD)) {
		vin_err("isp%d rdma fifo full\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, RDMA_FIFO_FULL_PD);
		if (bsp_isp_get_internal_status0(isp->id, RDMA_PLTM_PKX_FIFO_UV)) {
			vin_err("isp%d rdma fifo of pltm pkx fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_PLTM_PKX_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_REC_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d rec fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_REC_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_KSTB_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d kstb fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_KSTB_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_CNTR_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d cntr fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_CNTR_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_CURK_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d curk fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_CURK_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_PREK_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d prek fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_PREK_FIFO_UV);
		}
		__sunxi_isp_reset(isp);
	}

	if (bsp_isp_get_irq_status(isp->id, PARA_SAVE_PD)) {
		bsp_isp_clr_irq_status(isp->id, PARA_SAVE_PD);
		isp->isp_frame_number++;

		if (!isp->f1_after_librun) {
			sunxi_isp_frame_sync_isr(isp);
			if (isp->h3a_stat.stat_en_flag)
				isp->f1_after_librun = 1;
		} else {
			if (isp->save_get_flag || isp->load_save || (isp->event_lost_cnt == 10)) {
				sunxi_isp_frame_sync_isr(isp);
				isp->event_lost_cnt = 0;
			} else {
				isp->event_lost_cnt++;
			}
		}
		isp->load_save = 0;
	}

	if (bsp_isp_get_irq_status(isp->id, PARA_LOAD_PD)) {
		bsp_isp_clr_irq_status(isp->id, PARA_LOAD_PD);

		isp_stat_load_set(&isp->h3a_stat);

		if (isp->large_image && global_isp[1]) {
			if (isp->id == 0 && isp->load_flag && !global_isp[1]->load_buf_busy) {
				memcpy(global_isp[1]->isp_load.phy_addr, &global_isp[1]->load_shadow[0], ISP_LOAD_DRAM_SIZE);
				memcpy(isp->isp_load.phy_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
				isp->load_flag = 0;
			}
		} else {
			if (isp->load_flag) {
				memcpy(isp->isp_load.phy_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
				isp->load_flag = 0;
			}
		}

		load_val = bsp_isp_load_update_flag(isp->id);
#if !defined CONFIG_D3D
		bsp_isp_module_disable(isp->id, D3D_EN);
		load_val = load_val & ~D3D_UPDATE;
#endif
		load_val = load_val | (GAMMA_UPDATE | CEM_UPDATE);
		bsp_isp_update_table(isp->id, load_val);

		/*change load ddr data*/
		sunxi_isp_set_load_ddr(isp);

		isp_3d_pingpong_update(isp);

		if (isp->load_select) {
			memcpy(isp->load_para[1].phy_addr, isp->isp_load.phy_addr, ISP_LOAD_DRAM_SIZE);
			hal_dcache_clean((unsigned long)isp->load_para[1].phy_addr, ISP_LOAD_DRAM_SIZE);
			bsp_isp_set_load_addr(isp->id, (unsigned long)isp->load_para[1].phy_addr);
			isp->load_select = false;
		} else {
			memcpy(isp->load_para[0].phy_addr, isp->isp_load.phy_addr, ISP_LOAD_DRAM_SIZE);
			hal_dcache_clean((unsigned long)isp->load_para[0].phy_addr, ISP_LOAD_DRAM_SIZE);
			bsp_isp_set_load_addr(isp->id, (unsigned long)isp->load_para[0].phy_addr);
			isp->load_select = true;
		}

		bsp_isp_set_para_ready(isp->id, PARA_READY);
	}
	hal_spin_unlock_irqrestore(&isp->slock, __cpsr);
	return 0;
}
#endif

int sunxi_isp_s_mbus_config(unsigned int id, const struct mbus_framefmt_res *res)
{
	struct isp_dev *isp = global_isp[id];

	memcpy(isp->res, res, sizeof(struct mbus_framefmt_res));
	return 0;
}

int sunxi_isp_ae_done(unsigned int id, int drap_frame_en)
{
	struct isp_dev *isp = global_isp[id];

	return vin_isp_ae_done(isp, drap_frame_en);
}

void sunxi_isp_set_ae_idx(unsigned int id, unsigned short ev_idx)
{
	struct isp_dev *isp = global_isp[id];

	vin_isp_set_ae_idx(isp, ev_idx);
}

void sunxi_isp_set_lightadc_debug_en(unsigned int id, unsigned char lightadc_debug_en)
{
	struct isp_dev *isp = global_isp[id];

	vin_isp_set_lightadc_debug(isp, lightadc_debug_en);
}

void sunxi_isp_reset_server(unsigned int id)
{
	struct isp_dev *isp = global_isp[id];

	isp_reset(isp->hw_isp);
}

void sunxi_isp_update_server(unsigned int id)
{
	struct isp_dev *isp = global_isp[id];
	struct isp_part_para isp_part;
	struct isp_lib_context *ctx = isp_dev_get_ctx(isp->hw_isp);

	if (ctx == NULL) {
		ISP_ERR("ctx is NULL\n");
		return;
	}

	isp_part.flicker_mode = FREQUENCY_50HZ;
	if (ctx->isp_ir_flag == 2)
		isp_part.ir_mode = 0x6;
	else if (ctx->isp_ir_flag == 1)
		isp_part.ir_mode = 0x2;
	else
		isp_part.ir_mode = 0;
	vin_server_update(isp, isp_part);
}

void sunxi_isp_send_save_load(unsigned int id, void *destin_addr)
{
	struct isp_dev *isp = global_isp[id];

	memcpy(destin_addr, isp->isp_save_load.phy_addr + ISP_SAVE_LOAD_REG_SIZE, ISP_SAVE_LOAD_STATISTIC_SIZE);
}

int isp_probe(unsigned int id, unsigned int sensor_id)
{
	struct isp_dev *isp = NULL;
	struct isp_dev *logic_isp = NULL;
	char task_name[6] = {0};

	int ret = 0;

	if (isp_probe_flag[id])
		return 0;
	else
		isp_probe_flag[id] = true;

	if (id > VIN_MAX_ISP) {
		vin_err("isp%d is not existing, max is %d\n", id, VIN_MAX_ISP);
		return -1;
	}

	isp = hal_malloc(sizeof(struct isp_dev));
	if (!isp) {
		ret = -1;
		goto ekzalloc;
	}
	memset(isp, 0, sizeof(struct isp_dev));

	isp->res = hal_malloc(sizeof(struct mbus_framefmt_res));
	if (!isp->res) {
		ret = -1;
		goto ekzalloc;
	}
	memset(isp->res, 0, sizeof(struct mbus_framefmt_res));

	isp->id = id;
	isp->use_isp = global_sensors[sensor_id].use_isp;
	global_isp[id] = isp;

	if (isp->id == isp_virtual_find_logic[isp->id]) {
		isp->work_mode = vin_work_mode;//clamp(isp->work_mode, ISP_ONLINE, ISP_OFFLINE);
	} else {
		isp->work_mode = 0xff;
		if (isp->id != 4) {
			logic_isp = global_isp[isp_virtual_find_logic[isp->id]];
			if (logic_isp->work_mode == ISP_ONLINE) { /*logic isp work in online*/
				vin_log(VIN_LOG_VIDEO, "isp%d work in online mode, isp%d cannot to work!\n", logic_isp->id, isp->id);
				isp->is_empty = 1;
				goto freedev;
			}
		}
	}

	isp->base = sunxi_vin_get_isp_base(id);
	if (!isp->base) {
		vin_err("Fail toget the ISP base addr\n");
		isp->is_empty = 1;
		ret = -1;
		goto freedev;
	} else {
		isp->is_empty = 0;
#ifdef ISP_USE_IRQ
		isp->irq = sunxi_vin_get_isp_irq(id);
		if (isp->irq <= 0) {
			vin_err("isp%d: failed to get ISP IRQ resource\n", isp->id);
			goto isp_empty;
//			return -1;
		}
		vin_log(VIN_LOG_MD, "isp%d irq is %d\n", isp->id, isp->irq);
		ret = hal_request_irq(isp->irq, isp_isr, "sunxi_isp", isp);
#if defined CONFIG_ARCH_SUN55IW3
		if (ret != isp->irq / 100) {
#elif defined CONFIG_ARCH_SUN20IW3
		if (ret != isp->irq) {
#else
		if (ret != 0) {
#endif
			vin_err("isp%d request irq failed, ret:%d\n", isp->id, ret);
			goto freedev;
		}
#endif
		isp_resource_alloc(id);

		bsp_isp_map_reg_addr(isp->id, (unsigned long)isp->base);
		bsp_isp_map_load_dram_addr(isp->id, (unsigned long)isp->isp_load.phy_addr);
		bsp_isp_map_save_load_dram_addr(isp->id, (unsigned long)isp->isp_save_load.phy_addr);
#ifndef CONFIG_ARCH_SUN55IW3
		isp->syscfg_base = syscfg_base;
		bsp_isp_map_syscfg_addr(isp->id, (unsigned long)isp->syscfg_base);
#endif
	}
isp_empty:
	vin_log(VIN_LOG_MD, "isp%d reg is 0x%lx\n", isp->id, isp->base);

	ret = vin_isp_h3a_init(isp);
	if (ret < 0) {
		vin_err("VIN H3A initialization failed\n");
			goto freedev;
	}
	snprintf(task_name, sizeof(task_name), "isp_%d", id);
	isp->isp_server_run_task = hal_workqueue_create(task_name, 16*1024, HAL_THREAD_PRIORITY_SYS);
	if (!isp->isp_server_run_task) {
		vin_err("Create isp server work queue fail!\n");
		goto freedev;
	}
	hal_work_init(&isp->start_isp_server_work, __isp_server_run_handle, isp);

	sunxi_isp_subdev_init(id);

	vin_log(VIN_LOG_ISP, "isp%d probe end!\n", isp->id);
	return 0;
freedev:
	hal_free(isp->res);
	hal_free(isp);
ekzalloc:
	vin_err("isp probe err!\n");
	return ret;
}

int isp_remove(unsigned int id)
{
	struct isp_dev *isp = global_isp[id];

	if (!isp_probe_flag[id])
		return 0;
	else
		isp_probe_flag[id] = false;

	hal_workqueue_destroy(isp->isp_server_run_task);
	//hal_work_cancel(&isp->start_isp_server_work);
	vin_isp_h3a_cleanup(isp);

	if (!isp->is_empty) {
		hal_free_irq(isp->irq);

		hal_free(isp->res);
#if !defined ISP_SERVER_FASTINIT || defined CONFIG_VIDEO_SUNXI_VIN_SPECIAL
		hal_free(isp);
		isp = NULL;
#endif
	}
	return 0;
}
