
/*
 * vin_video.c for video api
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *           Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../utility/vin_io.h"
#include "../vin_csi/sunxi_csi.h"
#include "../vin_mipi/sunxi_mipi.h"
#include "../vin.h"

extern struct vin_core *global_vinc[VIN_MAX_VIDEO];

int vin_set_addr(unsigned int id, unsigned long phy_addr)
{
	struct vin_core *vinc = global_vinc[id];
	struct vin_addr paddr;
	__maybe_unused int offset_width;

	paddr.y = phy_addr;
	paddr.cb = (unsigned long)(phy_addr + global_video[id].o_width * global_video[id].o_height);
	paddr.cr = 0;

	if (global_video[id].fourcc == V4L2_PIX_FMT_LBC_1_0X || global_video[id].fourcc == V4L2_PIX_FMT_LBC_1_5X ||
			global_video[id].fourcc == V4L2_PIX_FMT_LBC_2_0X || global_video[id].fourcc == V4L2_PIX_FMT_LBC_2_5X) {
		paddr.cb = 0;
		paddr.cr = 0;
	}

	if (vinc->large_image) {
		offset_width = global_video[id].o_width / 2;
		csic_dma_buffer_address(vinc->id - 1, CSI_BUF_0_A, paddr.y);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr.y + offset_width);
		if (paddr.cb) {
			csic_dma_buffer_address(vinc->id - 1, CSI_BUF_1_A, paddr.cb);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr.cb + (offset_width >> 1));
		} else {
			csic_dma_buffer_address(vinc->id - 1, CSI_BUF_1_A, paddr.cb);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr.cb);
		}

		csic_dma_buffer_address(vinc->id - 1, CSI_BUF_2_A, paddr.cr);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr.cr);
	} else {
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr.y);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr.cb);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr.cr);
	}

	return 0;
}

static void vin_detect_buffer_cfg(struct vin_core *vinc)
{
#if !defined NOT_USE_ISP
	int ispid = vinc->isp_sel;
	struct isp_autoflash_config_s *isp_autoflash_cfg = NULL;
	unsigned int sign;

	if (ispid == 0) {  /* isp0 */
		isp_autoflash_cfg = (struct isp_autoflash_config_s *)ISP0_NORFLASH_SAVE;
		sign = 0xAA11AA11;
	} else { /* isp1/isp2 */
		isp_autoflash_cfg = (struct isp_autoflash_config_s *)ISP1_NORFLASH_SAVE;
		sign = 0xBB11BB11;
	}

	if (vinc->get_yuv_en) {
		isp_autoflash_cfg->melisyuv_sign_id = sign;
		isp_autoflash_cfg->melisyuv_paddr = (unsigned int)vinc->buff[0].phy_addr;
		isp_autoflash_cfg->melisyuv_size = vinc->buffer_size;
#ifdef CONFIG_NOT_FRAME_LOSS
		isp_autoflash_cfg->not_frame_loss_flag = 1;
#endif
	} else {
		isp_autoflash_cfg->melisyuv_sign_id = 0xFFFFFFFF;
	}
#endif
}

int buffer_queue(unsigned int id)
{
	unsigned int size;
	unsigned int i;
	unsigned int buffer_num;

	size = global_video[id].o_width * global_video[id].o_height * 3 / 2;

#ifdef DMA_USE_IRQ_BUFFER_QUEUE
#ifdef CONFIG_NOT_FRAME_LOSS
	buffer_num = 1;
#else
	if (global_vinc[id]->get_yuv_en)
		buffer_num = 2;
	else
		buffer_num = 1;
#endif
	for (i = 0; i < buffer_num; i++) {
#if defined CONFIG_KERNEL_FREERTOS
#ifdef CONFIG_NOT_FRAME_LOSS
		global_vinc[id]->buff[i].phy_addr = (void *)YUV_MEMRESERVE;
#else
		global_vinc[id]->buff[i].phy_addr = memheap_alloc_align(&isp_mempool, size, 0x1000);
#endif
#else
		global_vinc[id]->buff[i].phy_addr = rt_memheap_alloc_align(&isp_mempool, size, 0x1000);
#endif
		global_vinc[id]->buffer_size = size;
		if (global_vinc[id]->buff[i].phy_addr == NULL) {
			vin_err("%s:video%d:alloc bk buffer%d error\n", __func__, id, i);
			return -1;
		}
		vin_log(VIN_LOG_MD, "video%d: buffer[%d] phy_addr is 0x%p\n", id, i, global_vinc[id]->buff[i].phy_addr);
		if (global_vinc[id]->get_yuv_en)
			vin_print("video%d: buffer[%d] phy_addr is 0x%p\n", id, i, global_vinc[id]->buff[i].phy_addr);
	}
	vin_detect_buffer_cfg(global_vinc[id]);
#else
#if defined CONFIG_KERNEL_FREERTOS
	global_vinc[id]->buff[0].phy_addr = memheap_alloc_align(&isp_mempool, size, 0x1000);
#else
	global_vinc[id]->buff[0].phy_addr = rt_memheap_alloc_align(&isp_mempool, size, 0x1000);
#endif
	if (global_vinc[id]->buff[0].phy_addr == NULL) {
		vin_err("%s:video%d:alloc bk buffer error\n", __func__, id);
		return -1;
	}
#endif
	return 0;
}
#if defined CONFIG_KERNEL_FREERTOS
int buffer_free(unsigned int id)
{
#ifdef DMA_USE_IRQ_BUFFER_QUEUE
	if (global_vinc[id]->get_yuv_en)
		memheap_free_align(global_vinc[id]->buff[1].phy_addr);
	else
		memheap_free_align(global_vinc[id]->buff[0].phy_addr);
#else
	memheap_free_align(global_vinc[id]->buff[0].phy_addr);
#endif
	vin_log(VIN_LOG_VIDEO, "buf free!\n");
	return 0;
}
#else
int buffer_free(unsigned int id)
{
#ifdef DMA_USE_IRQ_BUFFER_QUEUE
	if (global_vinc[id]->get_yuv_en)
		rt_memheap_free_align(global_vinc[id]->buff[1].phy_addr);
	else
		rt_memheap_free_align(global_vinc[id]->buff[0].phy_addr);
#else
	rt_memheap_free_align(global_vinc[id]->buff[0].phy_addr);
#endif
	vin_log(VIN_LOG_VIDEO, "buf free!\n");
	return 0;
}
#endif
#if 0
/* The color format (colplanes, memplanes) must be already configured. */
int vin_set_addr(struct vin_core *vinc, struct vb2_buffer *vb,
		      struct vin_frame *frame, struct vin_addr *paddr)
{
	u32 pix_size, depth, y_stride, u_stride, v_stride;

	if (vb == NULL || frame == NULL)
		return -EINVAL;

	pix_size = ALIGN(frame->o_width, VIN_ALIGN_WIDTH) * frame->o_height;

	depth = frame->fmt.depth[0] + frame->fmt.depth[1] + frame->fmt.depth[2];

	paddr->y = vb2_dma_contig_plane_dma_addr(vb, 0);

	if (frame->fmt.memplanes == 1) {
		switch (frame->fmt.colplanes) {
		case 1:
			paddr->cb = 0;
			paddr->cr = 0;
			break;
		case 2:
			/* decompose Y into Y/Cb */

			if (frame->fmt.fourcc == V4L2_PIX_FMT_FBC) {
				paddr->cb = (u32)(paddr->y + CEIL_EXP(frame->o_width, 7) * CEIL_EXP(frame->o_height, 5) * 96);
				paddr->cr = 0;

			} else {
				paddr->cb = (u32)(paddr->y + pix_size);
				paddr->cr = 0;
			}
			break;
		case 3:
			paddr->cb = (u32)(paddr->y + pix_size);
			/* 420 */
			if (12 == frame->fmt.depth[0])
				paddr->cr = (u32)(paddr->cb + (pix_size >> 2));
			/* 422 */
			else
				paddr->cr = (u32)(paddr->cb + (pix_size >> 1));
			break;
		default:
			return -EINVAL;
		}
	} else if (!frame->fmt.mdataplanes) {
		if (frame->fmt.memplanes >= 2)
			paddr->cb = vb2_dma_contig_plane_dma_addr(vb, 1);

		if (frame->fmt.memplanes == 3)
			paddr->cr = vb2_dma_contig_plane_dma_addr(vb, 2);
	}

	if (vinc->vid_cap.frame.fmt.fourcc == V4L2_PIX_FMT_YVU420) {
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cb);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cr);
	} else {
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cb);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cr);
	}
	return 0;
}
#endif

static int lbc_mode_select(struct dma_lbc_cmp *lbc_cmp, unsigned int fourcc)
{
	switch (fourcc) {
	case V4L2_PIX_FMT_LBC_2_0X: /* 2x */
		lbc_cmp->is_lossy = 1;
		lbc_cmp->bit_depth = 8;
		lbc_cmp->glb_enable = 1;
		lbc_cmp->dts_enable = 1;
		lbc_cmp->ots_enable = 1;
		lbc_cmp->msq_enable = 1;
		lbc_cmp->cmp_ratio_even = 600;
		lbc_cmp->cmp_ratio_odd  = 450;
		lbc_cmp->mb_mi_bits[0]  = 55;
		lbc_cmp->mb_mi_bits[1]  = 110;
		lbc_cmp->rc_adv[0] = 60;
		lbc_cmp->rc_adv[1] = 30;
		lbc_cmp->rc_adv[2] = 15;
		lbc_cmp->rc_adv[3] = 8;
		lbc_cmp->lmtqp_en  = 1;
		lbc_cmp->lmtqp_min = 1;
		lbc_cmp->updata_adv_en = 1;
		lbc_cmp->updata_adv_ratio = 2;
		break;
	case V4L2_PIX_FMT_LBC_1_5X: /* 1.5x */
		lbc_cmp->is_lossy = 1;
		lbc_cmp->bit_depth = 8;
		lbc_cmp->glb_enable = 1;
		lbc_cmp->dts_enable = 1;
		lbc_cmp->ots_enable = 1;
		lbc_cmp->msq_enable = 1;
		lbc_cmp->cmp_ratio_even = 670;
		lbc_cmp->cmp_ratio_odd	= 658;
		lbc_cmp->mb_mi_bits[0]	= 87;
		lbc_cmp->mb_mi_bits[1]	= 167;
		lbc_cmp->rc_adv[0] = 60;
		lbc_cmp->rc_adv[1] = 30;
		lbc_cmp->rc_adv[2] = 15;
		lbc_cmp->rc_adv[3] = 8;
		lbc_cmp->lmtqp_en  = 1;
		lbc_cmp->lmtqp_min = 1;
		lbc_cmp->updata_adv_en = 1;
		lbc_cmp->updata_adv_ratio = 2;
		break;
	case V4L2_PIX_FMT_LBC_2_5X: /* 2.5x */
		lbc_cmp->is_lossy = 1;
		lbc_cmp->bit_depth = 8;
		lbc_cmp->glb_enable = 1;
		lbc_cmp->dts_enable = 1;
		lbc_cmp->ots_enable = 1;
		lbc_cmp->msq_enable = 1;
		lbc_cmp->cmp_ratio_even = 440;
		lbc_cmp->cmp_ratio_odd  = 380;
		lbc_cmp->mb_mi_bits[0]  = 55;
		lbc_cmp->mb_mi_bits[1]  = 94;
		lbc_cmp->rc_adv[0] = 60;
		lbc_cmp->rc_adv[1] = 30;
		lbc_cmp->rc_adv[2] = 15;
		lbc_cmp->rc_adv[3] = 8;
		lbc_cmp->lmtqp_en  = 1;
		lbc_cmp->lmtqp_min = 1;
		lbc_cmp->updata_adv_en = 1;
		lbc_cmp->updata_adv_ratio = 2;
		break;
	case V4L2_PIX_FMT_LBC_1_0X: /* lossless */
		lbc_cmp->is_lossy = 0;
		lbc_cmp->bit_depth = 8;
		lbc_cmp->glb_enable = 1;
		lbc_cmp->dts_enable = 1;
		lbc_cmp->ots_enable = 1;
		lbc_cmp->msq_enable = 1;
		lbc_cmp->cmp_ratio_even = 1000;
		lbc_cmp->cmp_ratio_odd  = 1000;
		lbc_cmp->mb_mi_bits[0]  = 55;
		lbc_cmp->mb_mi_bits[1]  = 94;
		lbc_cmp->rc_adv[0] = 60;
		lbc_cmp->rc_adv[1] = 30;
		lbc_cmp->rc_adv[2] = 15;
		lbc_cmp->rc_adv[3] = 8;
		lbc_cmp->lmtqp_en  = 1;
		lbc_cmp->lmtqp_min = 1;
		lbc_cmp->updata_adv_en = 1;
		lbc_cmp->updata_adv_ratio = 2;
		break;
	default:
		return -1;
	}
	return 0;
}

static int vin_subdev_logic_s_stream(unsigned char virtual_id, int on)
{
#if defined CSIC_DMA_VER_140_000
	unsigned char logic_id = dma_virtual_find_logic[virtual_id];
	struct vin_core *logic_vinc = global_vinc[logic_id];

	if (logic_vinc->work_mode == BK_ONLINE && virtual_id != logic_id) {
		vin_err("video%d work on online mode, video%d cannot to work!!\n", logic_id, virtual_id);
		return -1;
	}

	if (on && (logic_vinc->logic_top_stream_count)++ > 0)
		return 0;
	else if (!on && (logic_vinc->logic_top_stream_count == 0 || --(logic_vinc->logic_top_stream_count) > 0))
		return 0;

	if (on) {
		csic_dma_top_enable(logic_id);
		csic_dma_mul_ch_enable(logic_id, logic_vinc->work_mode);
		//if (logic_vinc->id == CSI_VE_ONLINE_VIDEO && logic_vinc->ve_online_cfg.ve_online_en) {
		//	csic_ve_online_hs_enable(logic_id);
		//	logic_vinc->ve_ol_ch = CSI_VE_ONLINE_VIDEO;
		//	csic_ve_online_ch_sel(logic_id, logic_vinc->ve_ol_ch);
		//}
		csic_dma_buf_length_software_enable(logic_vinc->vipp_sel, 0);
		csi_dam_flip_software_enable(logic_vinc->vipp_sel, 0);
		//csic_dma_top_interrupt_en(logic_id, VIDEO_INPUT_TO_INT | CLR_FS_FRM_CNT_INT | FS_PUL_INT);//for debug
		hal_enable_irq(logic_vinc->irq);
	} else {
		//csic_dma_top_interrupt_disable(logic_id, DMA_TOP_INT_ALL);
		hal_disable_irq(logic_vinc->irq);
		csic_ve_online_hs_disable(logic_id);
		csic_dma_top_disable(logic_id);
	}
	vin_log(VIN_LOG_FMT, "dma%d top init by video%d, %s\n", logic_id, virtual_id, on ? "steram on" : "steam off");
#endif
	return 0;
}

int vin_subdev_s_stream(unsigned int id, int enable)
{
	struct vin_core *vinc = global_vinc[id];
	struct csic_dma_cfg cfg;
	struct csic_dma_flip flip;
	struct dma_output_size size;
	struct dma_buf_len buf_len;
	struct dma_flip_size flip_size;
	struct dma_lbc_cmp lbc_cmp;
	int flag = 0;
	int flip_mul = 2;
	int wth;
	int *stream_count;
#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
	struct vin_buffer *buf = NULL;
#endif
	int sensor_id = vinc->rear_sensor;
	struct sensor_format_struct *sensor_format;
	if (global_sensors[sensor_id].sensor_core->sensor_g_format)
		sensor_format = global_sensors[sensor_id].sensor_core->sensor_g_format(sensor_id, vinc->isp_sel, id);
	else
		return -1;

	stream_count = &vinc->stream_count;
	if (enable && (*stream_count)++ > 0)
		return 0;
	else if (!enable && (*stream_count == 0 || --(*stream_count) > 0))
		return 0;

	vin_log(VIN_LOG_FMT, "csic_dma%d %s, %d*%d hoff: %d voff: %d\n",
		vinc->id, enable ? "stream on" : "stream off",
		global_video[id].o_width, global_video[id].o_height,
		sensor_format->offs_h, sensor_format->offs_v);

	if (enable) {
		memset(&cfg, 0, sizeof(cfg));
		memset(&size, 0, sizeof(size));
		memset(&buf_len, 0, sizeof(buf_len));

		if (global_video[id].fourcc == V4L2_PIX_FMT_LBC_1_0X || global_video[id].fourcc == V4L2_PIX_FMT_LBC_2_0X
			|| global_video[id].fourcc == V4L2_PIX_FMT_LBC_2_5X) {
			lbc_mode_select(&lbc_cmp, global_video[id].fourcc);
			wth = roundup(global_video[id].o_width, 32);
			if (lbc_cmp.is_lossy) {
				lbc_cmp.line_tar_bits[0] = roundup(lbc_cmp.cmp_ratio_even * wth * lbc_cmp.bit_depth/1000, 512);
				lbc_cmp.line_tar_bits[1] = roundup(lbc_cmp.cmp_ratio_odd * wth * lbc_cmp.bit_depth/500, 512);
			} else {
				lbc_cmp.line_tar_bits[0] = roundup(wth * lbc_cmp.bit_depth * 1 + (wth * 1 / 16 * 2), 512);
				lbc_cmp.line_tar_bits[1] = roundup(wth * lbc_cmp.bit_depth * 2 + (wth * 2 / 16 * 2), 512);
			}
		}

		switch (sensor_format->field) {
		case V4L2_FIELD_ANY:
		case V4L2_FIELD_NONE:
			cfg.field = FIELD_EITHER;
			break;
		case V4L2_FIELD_TOP:
			cfg.field = FIELD_1;
			flag = 1;
			break;
		case V4L2_FIELD_BOTTOM:
			cfg.field = FIELD_2;
			flag = 1;
			break;
		case V4L2_FIELD_INTERLACED:
			cfg.field = FIELD_EITHER;
			flag = 1;
			break;
		default:
			cfg.field = FIELD_EITHER;
			break;
		}

		switch (global_video[id].fourcc) {
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_FBC:
			cfg.fmt = flag ? FRAME_UV_CB_YUV420 : FIELD_UV_CB_YUV420;
			buf_len.buf_len_y = global_video[id].o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_LBC_2_0X:
		case V4L2_PIX_FMT_LBC_2_5X:
		case V4L2_PIX_FMT_LBC_1_5X:
		case V4L2_PIX_FMT_LBC_1_0X:
			cfg.fmt = LBC_MODE_OUTPUT;
			buf_len.buf_len_y = lbc_cmp.line_tar_bits[1] >> 3;
			buf_len.buf_len_c = lbc_cmp.line_tar_bits[0] >> 3;
			break;
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV21M:
			cfg.fmt = flag ? FRAME_VU_CB_YUV420 : FIELD_VU_CB_YUV420;
			buf_len.buf_len_y = global_video[id].o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YUV420M:
			cfg.fmt = flag ? FRAME_PLANAR_YUV420 : FIELD_PLANAR_YUV420;
			buf_len.buf_len_y = global_video[id].o_width;
			buf_len.buf_len_c = buf_len.buf_len_y >> 1;
			break;
		case V4L2_PIX_FMT_GREY:
			cfg.fmt = flag ? FRAME_CB_YUV400 : FIELD_CB_YUV400;
			buf_len.buf_len_y = global_video[id].o_width;
			break;
		case V4L2_PIX_FMT_YUV422P:
			cfg.fmt = flag ? FRAME_PLANAR_YUV422 : FIELD_PLANAR_YUV422;
			buf_len.buf_len_y = global_video[id].o_width;
			buf_len.buf_len_c = buf_len.buf_len_y >> 1;
			break;
		case V4L2_PIX_FMT_NV61:
		case V4L2_PIX_FMT_NV61M:
			cfg.fmt = flag ? FRAME_VU_CB_YUV422 : FIELD_VU_CB_YUV422;
			buf_len.buf_len_y = global_video[id].o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV16M:
			cfg.fmt = flag ? FRAME_UV_CB_YUV422 : FIELD_UV_CB_YUV422;
			buf_len.buf_len_y = global_video[id].o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR8:
		case V4L2_PIX_FMT_SGBRG8:
		case V4L2_PIX_FMT_SGRBG8:
		case V4L2_PIX_FMT_SRGGB8:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_8 : FIELD_RAW_8;
			buf_len.buf_len_y = global_video[id].o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR10:
		case V4L2_PIX_FMT_SGBRG10:
		case V4L2_PIX_FMT_SGRBG10:
		case V4L2_PIX_FMT_SRGGB10:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_10 : FIELD_RAW_10;
			buf_len.buf_len_y = global_video[id].o_width * 2;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_12 : FIELD_RAW_12;
			buf_len.buf_len_y = global_video[id].o_width * 2;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		default:
			cfg.fmt = flag ? FRAME_UV_CB_YUV420 : FIELD_UV_CB_YUV420;
			buf_len.buf_len_y = global_video[id].o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		}

		if (vinc->isp_dbg.debug_en) {
			buf_len.buf_len_y = 0;
			buf_len.buf_len_c = 0;
		}

		cfg.ds = vinc->fps_ds;
		vinc->frame_cnt = 0;

		if (vin_subdev_logic_s_stream(vinc->id, enable))
			return -1;

		csic_dma_config(vinc->vipp_sel, &cfg);
#if !defined CSIC_DMA_VER_140_000
		csic_dma_buf_length_software_enable(vinc->vipp_sel, 0);
		csi_dam_flip_software_enable(vinc->vipp_sel, 0);
		hal_enable_irq(vinc->irq);
#endif
		size.hor_len = vinc->isp_dbg.debug_en ? 0 : global_video[id].o_width;
		size.ver_len = vinc->isp_dbg.debug_en ? 0 : global_video[id].o_height;
		size.hor_start = vinc->isp_dbg.debug_en ? 0 : sensor_format->offs_h;
		size.ver_start = vinc->isp_dbg.debug_en ? 0 : sensor_format->offs_v;
		flip_size.hor_len = vinc->isp_dbg.debug_en ? 0 : global_video[id].o_width * flip_mul;
		flip_size.ver_len = vinc->isp_dbg.debug_en ? 0 : global_video[id].o_height;
		flip.hflip_en = vinc->hflip;
		flip.vflip_en = vinc->vflip;

		csic_dma_output_size_cfg(vinc->vipp_sel, &size);

		csic_dma_buffer_length(vinc->vipp_sel, &buf_len);
		csic_dma_flip_size(vinc->vipp_sel, &flip_size);

#ifndef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
		if ((vinc->large_image && vinc->id % 2 == 1) || (!vinc->large_image)) {
			buffer_queue(id);
			vin_set_addr(id, (unsigned long)vinc->buff[0].phy_addr);
		}
#else
		if (!list_empty(&vinc->vin_active)) {
			buf = list_entry(vinc->vin_active.next, struct vin_buffer, list);
		} else {
			vin_warn("stream on, but no buffer now.\n");
			return -1;;
		}
		vin_set_addr(id, (unsigned long)buf->phy_addr);
#endif

#ifdef DMA_USE_IRQ_BUFFER_QUEUE
		if ((vinc->large_image && vinc->id % 2 == 1) || (!vinc->large_image)) {
			csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
			csic_dma_int_enable(vinc->vipp_sel, DMA_INT_BUF_0_OVERFLOW | DMA_INT_HBLANK_OVERFLOW |
				DMA_INT_VSYNC_TRIG | DMA_INT_CAPTURE_DONE | DMA_INT_FRAME_DONE | DMA_INT_LBC_HB);
		//csic_dma_int_enable(vinc->vipp_sel, DMA_INT_ADDR_NO_READY | DMA_INT_ADDR_OVERFLOW);
		}
#endif
#if !defined CSIC_DMA_VER_140_000
		csic_dma_top_enable(vinc->vipp_sel);
#endif
		switch (global_video[id].fourcc) {
		case V4L2_PIX_FMT_LBC_2_0X:
		case V4L2_PIX_FMT_LBC_2_5X:
		case V4L2_PIX_FMT_LBC_1_5X:
		case V4L2_PIX_FMT_LBC_1_0X:
			csic_lbc_enable(vinc->vipp_sel);
			csic_lbc_cmp_ratio(vinc->vipp_sel, &lbc_cmp);
			break;
		default:
			csic_dma_flip_en(vinc->vipp_sel, &flip);
			csic_dma_enable(vinc->vipp_sel);
			break;
		}
	} else {
#ifdef DMA_USE_IRQ_BUFFER_QUEUE
		csic_dma_int_disable(vinc->vipp_sel, DMA_INT_ALL);
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
#endif
		switch (global_video[id].fourcc) {
		case V4L2_PIX_FMT_FBC:
			csic_fbc_disable(vinc->vipp_sel);
			break;
		case V4L2_PIX_FMT_LBC_2_0X:
		case V4L2_PIX_FMT_LBC_2_5X:
		case V4L2_PIX_FMT_LBC_1_5X:
		case V4L2_PIX_FMT_LBC_1_0X:
			csic_lbc_disable(vinc->vipp_sel);
			break;
		default:
			csic_dma_disable(vinc->vipp_sel);
			break;
		}
		vin_subdev_logic_s_stream(vinc->id, enable);

		//buffer_free(id);
	}

	return 0;
}

#if defined CONFIG_VIDEO_SUNXI_VIN_SPECIAL
#define VIN_VIDEO_SOURCE_WIDTH_DEFAULT  1920
#define VIN_VIDEO_SOURCE_HEIGHT_DEFAULT 1080
int vin_s_input_special(int id, int val)
{
	struct vin_core *vinc = global_vinc[id];

	if (val == 0)
		vinc->sensor_sel = vinc->rear_sensor;
	else
		vinc->sensor_sel = vinc->front_sensor;

	return 0;
}

int vin_g_fmt_special(int id, struct vin_format *f)
{
	struct vin_core *vinc = global_vinc[id];
//	struct sensor_format_struct *sensor_format = NULL;

	f->win.width = vinc->o_width;
	f->win.height = vinc->o_height;
	f->pixelformat = vinc->fourcc;
	f->win.fps = vinc->fps_fixed;

	return 0;
}

int vin_g_fmt_special_ext(int id, struct vin_format *f)
{
	struct vin_core *vinc = global_vinc[id];
	struct sensor_format_struct *sensor_format = NULL;

	f->win.width = VIN_VIDEO_SOURCE_WIDTH_DEFAULT;
	f->win.height = VIN_VIDEO_SOURCE_HEIGHT_DEFAULT;
	if (global_sensors[vinc->rear_sensor].sensor_core->sensor_g_format)
		sensor_format = global_sensors[vinc->rear_sensor].sensor_core->sensor_g_format(vinc->sensor_sel, vinc->isp_sel, id);
	if (sensor_format == NULL) {
		vin_err("vinc get sensor format error!\n");
		return -1;
	}
	f->pixelformat = sensor_format->mbus_code;
	f->win.fps = sensor_format->fps_fixed;

	return 0;
}

int vin_s_fmt_special(int id, struct vin_format *f)
{
	struct vin_core *vinc = global_vinc[id];

	vinc->o_width = f->win.width;
	vinc->o_height = f->win.height;
	vinc->fourcc = f->pixelformat;
	vinc->fps_fixed = f->win.fps;

	return 0;
}

int vin_streamon_special(int id)
{
	struct vin_core *vinc = global_vinc[id];
	int sensor_id;
	int ret = 0;
	int j;

	sensor_id = vinc->rear_sensor;
	if (global_sensors[sensor_id].sensor_core->sensor_power)
		global_sensors[sensor_id].sensor_core->sensor_power(sensor_id, PWR_ON);
	if (global_sensors[sensor_id].sensor_core->sensor_g_format)
		global_sensors[sensor_id].sensor_core->sensor_g_format(sensor_id, vinc->isp_sel, id);

	vin_pipeline_set_mbus_config(id);
#if defined CONFIG_ARCH_SUN55IW3 || defined CONFIG_ARCH_SUN20IW3
	for (j = 0; j < vinc->total_rx_ch; j++) {
		csic_isp_input_select(isp_virtual_find_sel[vinc->isp_sel], isp_ch_find[vinc->isp_sel] + j, vinc->csi_sel, j);
	}
	csic_vipp_input_select(vipp_virtual_find_sel[vinc->vipp_sel], isp_virtual_find_sel[vinc->isp_sel], vinc->isp_tx_ch);
#else
	for (j = 0; j < vinc->total_rx_ch; j++)
		csic_isp_input_select(vinc->isp_sel, j, vinc->csi_sel, j);
	csic_vipp_input_select(vinc->vipp_sel, vinc->isp_sel, vinc->isp_tx_ch);
#endif

	ret = vin_s_stream(id, PWR_ON);
	if (ret)
		vin_err("find not sensor\n");

	return ret;
}

/*
debug use
void *vin_rebuf_special(int id)
{
	unsigned int size;
	unsigned int i;
	struct vin_core *vinc = global_vinc[id];
	struct vin_buffer * = NULL;

	size = global_video[id].o_width * global_video[id].o_height * 3 / 2;

	hal_mutex_lock(vinc->lock);
	buffer->phy_addr = memheap_alloc_align(&isp_mempool, size, 0x1000);
	if (buffer->phy_addr == NULL) {
		vin_err("%s:alloc bk buffer error\n", __func__);
		return -1;
	}
	hal_mutex_unlock(vinc->lock);

	return buffer->phy_addr;
}
*/

int vin_reqbuf_special(int id, int buf_num)
{
	unsigned int size;
	unsigned int i;
	struct vin_core *vinc = global_vinc[id];

	if (buf_num > BUF_NUM) {
		buf_num = BUF_NUM;
		vin_warn("buffer moar than %d, set to %d\n", BUF_NUM, buf_num);
	}

	size = global_video[id].o_width * global_video[id].o_height * 3 / 2;

	hal_mutex_lock(vinc->lock);
	for (i = 0; i < buf_num; i++) {
		vinc->buff[i].phy_addr = memheap_alloc_align(&isp_mempool, size, 0x1000);
		vinc->buff[i].index = i;
		vinc->buffer_count = buf_num;
		vinc->buff[i].size = size;
		vinc->buffer_size = size;
		if (vinc->buff[i].phy_addr == NULL) {
			vin_err("%s:video%d:alloc bk buffer%d error\n", __func__, id, i);
			return -1;
		}
		vin_log(VIN_LOG_MD, "video%d: buffer[%d] phy_addr is 0x%p\n", id, i, vinc->buff[i].phy_addr);
		list_add_tail(&vinc->buff[i].list, &vinc->vin_active);
	}
	hal_mutex_unlock(vinc->lock);

	return 0;
}

int vin_dqbuffer_special(int id, struct vin_buffer **buf)
{
	int ret;
	struct vin_core *vinc = global_vinc[id];
	unsigned int timeout_msec = TIME_OUT_MAX;

	ret = hal_wait_event_timeout(vinc->vin_waitqueue, list_empty(&vinc->vin_done) == 0, timeout_msec);
	if (!ret) {
		vin_err("vin wait buffer timeout %d msec!\n", timeout_msec);
		return -1;
	}

	if (list_empty(&vinc->vin_done)) {
		vin_err("vinc done queue is empty\n");
		return -1;
	}

	hal_mutex_lock(vinc->lock);
	*buf = list_first_entry(&vinc->vin_done, struct vin_buffer, list);
	list_del(&((*buf)->list));
	hal_mutex_unlock(vinc->lock);

	return ret;
}

int vin_qbuffer_special(int id, struct vin_buffer *buf)
{
	struct vin_core *vinc = global_vinc[id];

	if (buf == NULL) {
		vin_err("buf is NULL, cannot qbuf\n");
		return -1;
	}

	hal_mutex_lock(vinc->lock);
	list_add_tail(&buf->list, &vinc->vin_active);
	hal_mutex_unlock(vinc->lock);

	return 0;
}

int vin_buffer_free(int id)
{
	struct vin_core *vinc = global_vinc[id];
	struct vin_buffer *buf;
	int i;

	if (vinc->stream_count) {
		vin_err("please stream off vin frist\n");
		return -1;
	}

	hal_mutex_lock(vinc->lock);
	while (!list_empty(&vinc->vin_done)) {
		buf = list_entry(vinc->vin_done.next, struct vin_buffer, list);
		list_del(&buf->list);
		vin_log(VIN_LOG_MD, "vin buf%d delect from done queue\n", buf->index);
	}
	while (!list_empty(&vinc->vin_active)) {
		buf = list_entry(vinc->vin_active.next, struct vin_buffer, list);
		list_del(&buf->list);
		vin_log(VIN_LOG_MD, "vin buf%d delect from active queue\n", buf->index);
	}

	for (i = 0; i < vinc->buffer_count; i++) {
		memheap_free_align(vinc->buff[i].phy_addr);
	}
	hal_mutex_unlock(vinc->lock);

	return 0;
}

int vin_streamoff_special(int id)
{
	struct vin_core *vinc = global_vinc[id];
	int sensor_id = vinc->rear_sensor;

	vin_print("close video%d\n", vinc->id);

	vin_s_stream(id, PWR_OFF);
	/* if use vin_reqbuf_special, please open here */
	//vin_buffer_free(id);

#ifdef CONFIG_KERNEL_FREERTOS
	memheap_detach(&isp_mempool);
#else
	rt_memheap_detach(&isp_mempool);
#endif

	global_sensors[sensor_id].sensor_core->sensor_power(sensor_id, PWR_OFF);

	vin_log(VIN_LOG_MD, "GoodBye CSI!\n");

	return 0;
}

int vin_force_reset_buffer(int id)
{
	struct vin_core *vinc = global_vinc[id];
	struct vin_buffer *buf;

	hal_mutex_lock(vinc->lock);
	while (!list_empty(&vinc->vin_active)) {
	buf = list_first_entry(&vinc->vin_active, struct vin_buffer, list);
	list_del(&buf->list);
	list_add(&buf->list, &vinc->vin_done);
	}
	hal_mutex_unlock(vinc->lock);

	return 0;
}

void vin_register_buffer_done_callback(int id, void *func)
{
	struct vin_core *vinc = global_vinc[id];

	vinc->vin_buffer_process = func;
}
#endif
