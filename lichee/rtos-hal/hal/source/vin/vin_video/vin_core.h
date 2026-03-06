/*
 * vin_core.h for video manage
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *	Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _VIN_CORE_H_
#define _VIN_CORE_H_

#include <hal_atomic.h>
#ifdef CONFIG_KERNEL_FREERTOS
#include <hal_waitqueue.h>
#include "hal_mutex.h"
#endif

#include "../platform/platform_cfg.h"
#include "../vin_mipi/bsp_mipi_csi.h"
#include "../utility/vin_supply.h"
#include "vin_video.h"
#if defined CSIC_DMA_VER_140_000
#include "dma140/dma140_reg.h"
#else
#include "dma130/dma_reg.h"
#endif

#define MAX_FRAME_MEM	(500*1024*1024)
#define MIN_WIDTH	(32)
#define MIN_HEIGHT	(32)
#define MAX_WIDTH	(4800)
#define MAX_HEIGHT	(4800)

#define DMA_USE_IRQ_BUFFER_QUEUE

struct vin_core {
	unsigned char total_rx_ch;
	bool use_sensor_list;
	unsigned int is_empty;
	int id;
	int used;
	/* about some global info */
	int rear_sensor;
	int front_sensor;
	int sensor_sel;
	int csi_sel;
	int mipi_sel;
	int isp_sel;
	int vipp_sel;
	int tdm_rx_sel;
	int isp_tx_ch;
	int hflip;
	int vflip;
	int fps_ds;
	int irq;
	int first_frame;
	int frame_cnt;
	int stream_count;
	int large_image;
	unsigned long base;
	struct isp_debug_mode isp_dbg;
	hal_spinlock_t slock;
#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
	hal_mutex_t lock;
	hal_waitqueue_head_t vin_waitqueue;
	struct list_head vin_active;
	struct list_head vin_done;
	void (*vin_buffer_process)(int id);
#endif

	unsigned int o_width;
	unsigned int o_height;
	unsigned int merge_width;
	unsigned int merge_height;
	unsigned int fps_fixed;
	u32 fourcc;

	unsigned int work_mode;
	unsigned char vir_prosess_ch;
	unsigned char is_irq_empty;
	unsigned char noneed_register;
	unsigned char logic_top_stream_count;
	unsigned char ve_ol_ch;
	unsigned char get_yuv_en;

	struct vin_buffer buff[BUF_NUM];
	unsigned int buffer_size;
	unsigned int buffer_count;
	unsigned int sensor_lane;
	unsigned int mipi_num;
};

int vin_core_probe(unsigned int id);
int vin_core_remove(unsigned int id);

#endif /*_VIN_CORE_H_*/

