/*
 * vin.h
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zequn Zheng <zequnzhengi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __VIN_H__
#define __VIN_H__

#include <hal_log.h>
#include <hal_reset.h>
#include "utility/config.h"
#include "utility/vin_supply.h"
#include "utility/vin_common.h"
#include "utility/media-bus-format.h"
#include "platform/platform_cfg.h"
#include "vin_cci/cci_helper.h"
#include "vin_mipi/sunxi_mipi.h"
#include "vin_csi/sunxi_csi.h"
#include "vin_video/vin_core.h"
#include "vin_video/vin_video.h"
#if !defined CONFIG_ARCH_SUN8IW20
#include "vin_isp/sunxi_isp.h"
#endif
#include "vin_vipp/sunxi_scaler.h"
#include "vin_tdm/vin_tdm.h"
#include "modules/sensor/camera.h"
#include "modules/sensor/sensor_register.h"
#include "top_reg.h"
#include "vin_rpmsg/vin_rpmsg.h"

#if !defined CONFIG_RTTKERNEL && !defined CONFIG_KERNEL_FREERTOS
#error "vin driver only support rtt platform"
#endif

#define VIN_PLL_CSI_RATE (2376UL*1000*1000)

#ifdef CONFIG_KERNEL_FREERTOS
extern struct memheap isp_mempool;
#else
extern struct rt_memheap isp_mempool;
#endif

/*
 * RTC_NUM:
 * bit0: 1 mean boot0 done, sram give back to isp
 * bit1: 1 mean csi_init is init once, when melis crash and then restart, isp server is reinit
 * bit2~31: reserve
 */
#define RTC_NUM 1

#if defined CONFIG_COMPONENTS_RPBUF_UART
#define TAKE_PHOTO_MODE 0
#define AI_MODE 4
#define TAKE_VIDEO_VERTICAL_MODE 10
extern int aglink_get_mode(void);
#endif
extern unsigned int vin_log_mask;

#define VIN_LOG_MD				(1 << 0) 	/*0x1 */
#define VIN_LOG_FLASH				(1 << 1) 	/*0x2 */
#define VIN_LOG_CCI				(1 << 2) 	/*0x4 */
#define VIN_LOG_CSI				(1 << 3) 	/*0x8 */
#define VIN_LOG_MIPI				(1 << 4) 	/*0x10*/
#define VIN_LOG_ISP				(1 << 5) 	/*0x20*/
#define VIN_LOG_STAT				(1 << 6) 	/*0x40*/
#define VIN_LOG_SCALER				(1 << 7) 	/*0x80*/
#define VIN_LOG_POWER				(1 << 8) 	/*0x100*/
#define VIN_LOG_CONFIG				(1 << 9) 	/*0x200*/
#define VIN_LOG_VIDEO				(1 << 10)	/*0x400*/
#define VIN_LOG_FMT				(1 << 11)	/*0x800*/
#define VIN_LOG_TDM				(1 << 12)	/*0x1000*/
#define VIN_LOG_STAT1				(1 << 13) 	/*0x2000*/

#define vin_log(flag, arg...) do { \
	if (flag & vin_log_mask) { \
		switch (flag) { \
		case VIN_LOG_MD: \
			printk(KERN_DEBUG"[VIN_LOG_MD]" arg); \
			break; \
		case VIN_LOG_FLASH: \
			printk(KERN_DEBUG"[VIN_LOG_FLASH]" arg); \
			break; \
		case VIN_LOG_CCI: \
			printk(KERN_DEBUG"[VIN_LOG_CCI]" arg); \
			break; \
		case VIN_LOG_CSI: \
			printk(KERN_DEBUG"[VIN_LOG_CSI]" arg); \
			break; \
		case VIN_LOG_MIPI: \
			printk(KERN_DEBUG"[VIN_LOG_MIPI]" arg); \
			break; \
		case VIN_LOG_ISP: \
			printk(KERN_DEBUG"[VIN_LOG_ISP]" arg); \
			break; \
		case VIN_LOG_STAT: \
			printk(KERN_DEBUG"[VIN_LOG_STAT]" arg); \
			break; \
		case VIN_LOG_SCALER: \
			printk(KERN_DEBUG"[VIN_LOG_SCALER]" arg); \
			break; \
		case VIN_LOG_POWER: \
			printk(KERN_DEBUG"[VIN_LOG_POWER]" arg); \
			break; \
		case VIN_LOG_CONFIG: \
			printk(KERN_DEBUG"[VIN_LOG_CONFIG]" arg); \
			break; \
		case VIN_LOG_VIDEO: \
			printk(KERN_DEBUG"[VIN_LOG_VIDEO]" arg); \
			break; \
		case VIN_LOG_FMT: \
			printk(KERN_DEBUG"[VIN_LOG_FMT]" arg); \
			break; \
		default: \
			printk(KERN_DEBUG"[VIN_LOG]" arg); \
			break; \
		} \
	} \
} while (0)

#if defined CONFIG_KERNEL_FREERTOS
#define vin_print(x, arg...) hal_log_info("[VIN]" x, ##arg)
#define vin_warn(x, arg...) hal_log_warn("[VIN_WRN]" x, ##arg)
#define vin_err(x, arg...) hal_log_err("[VIN_ERR]" x, ##arg)
#else
#define vin_err(x, arg...) printk(KERN_ERR"[VIN_ERR]%s, line: %d," x, __FUNCTION__, __LINE__, ##arg)
#define vin_warn(x, arg...) printk(KERN_WARNING"[VIN_WRN]" x, ##arg)
#define vin_print(x, arg...) printk(KERN_DEBUG"[VIN]" x, ##arg)
#endif

int vin_s_stream(unsigned int id, int enable);
int vin_pipeline_set_mbus_config(unsigned int id);

#endif
