
/*
 * vin_video.h for video api
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

#ifndef _VIN_VIDEO_H_
#define _VIN_VIDEO_H_

#include "../utility/vin_common.h"
#include "../utility/sunxi_camera_v2.h"
#if defined CSIC_DMA_VER_140_000
#include "dma140/dma140_reg.h"
#else
#include "dma130/dma_reg.h"
#endif
#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
#include <aw_list.h>
#endif

#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
// #define DISPLAY_FRAM
#define TIME_OUT_MAX	(CONFIG_VIDEO_SUNXI_VIN_SPECIAL_EVENT_TIMEOUT)
#endif

/* buffer for one video frame */
struct vin_buffer {
	void *phy_addr;
#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
	struct list_head list;
	unsigned char index;
	unsigned int size;
#endif
};

#define BUF_NUM 3

enum bk_work_mode {
	BK_ONLINE = 0,
	BK_OFFLINE = 1,
};

struct vin_addr {
	u32	y;
	u32	cb;
	u32	cr;
};

int vin_subdev_s_stream(unsigned int id, int enable);
int vin_set_addr(unsigned int id, unsigned long phy_addr);
int buffer_queue(unsigned int id);
int buffer_free(unsigned int id);
#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
int vin_g_fmt_special(int id, struct vin_format *f);
int vin_g_fmt_special_ext(int id, struct vin_format *f);
int vin_s_fmt_special(int id, struct vin_format *f);
int vin_streamon_special(int id);
int vin_reqbuf_special(int id, int buf_num);
int vin_dqbuffer_special(int id, struct vin_buffer **buf);
int vin_qbuffer_special(int id, struct vin_buffer *buf);
int vin_streamoff_special(int id);
#endif
#endif /*_VIN_VIDEO_H_*/
