/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __XR_LINK_H__
#define __XR_LINK_H__

#include "xrlink_debug.h"
#include "command.h"
#include "kernel/os/os.h"

#if (defined CONFIG_ARCH_SUN300IW1)
#include "kernel/os/os_stby.h"
#endif

#define XRLINK_SEND_DATA2HOST_USE_THREAD
#define XRLINK_RX_DATA_USE_MBUF // need MBUF_FREE_BY_ETHERNETIF

//#define XRADIO_HDR_CKSUM
struct xradio_hdr {
	uint8_t  type;
	uint8_t  rsv;
	uint16_t seq;
	uint16_t len; /* payload len */
	uint16_t rsv1;
#ifdef XRADIO_HDR_CKSUM
	uint16_t checksum;
	uint16_t checksum_pad;
#endif
	uint8_t payload[];
} __packed;
#define XR_HDR_SZ sizeof(struct xradio_hdr)

struct xradio_agg_hdr {
	uint8_t agg_cnt; /* Input */
	uint8_t start_idx; /* Input */
	uint8_t ack_cnt;  /* Output, rx port rxed cnt of this agg data */
	uint8_t rsp_code; /* Output */
} __packed;
#define XR_AGG_HDR_SZ sizeof(struct xradio_agg_hdr)

/* Xradio xrlink xradio_hdr's type */
#define XR_TYPE_DATA           0x00
#define XR_TYPE_DATA_AP        0x01
#define XR_TYPE_CMD            0x02 // need respond !
#define XR_TYPE_EVENT          0x03
#define XR_TYPE_CMD_RSP        0x04
#define XR_TYPE_FW_BIN         0x05 // need respond !
#define XR_TYPE_RAW_CMD_DATA   0x06 // only use for rtos send raw data

#define XRLINK_THREAD_PRIO_DATA_TX     XR_OS_THREAD_PRIO_LINK_DATA_TX
#define XRLINK_THREAD_PRIO_DATA_RX     XR_OS_THREAD_PRIO_LINK_DATA_RX
#define XRLINK_THREAD_PRIO_COM         XR_OS_THREAD_PRIO_LINK_COM

#define XRLINK_RX_DATA_PAUSE_FLAG_XRLINK_TXQ    (1 << 0)
#define XRLINK_RX_DATA_PAUSE_FLAG_MBUF_LIMIT    (1 << 1)
#define XRLINK_RX_DATA_PAUSE_FLAG_WIFI_DRV_TXQ  (1 << 2)
#define XRLINK_RX_DATA_PAUSE_FLAG_XRLINK_TX     (1 << 3)

typedef void(*xrlink_recv_cb)(uint8_t *data, uint32_t len);
#if (defined CONFIG_WLAN_RAW_PACKET_FEATURE) || (defined CONFIG_WLAN_MONITOR)
int xrlink_send_raw_data(uint8_t *data, uint32_t len, void *info, xrlink_raw_type_t type);
int xradio_link_reg_raw_data_recv_cb(xrlink_recv_cb cb);
#endif
#ifdef CONFIG_STA_SOFTAP_COEXIST
int xrlink_net_to_host(uint8_t *buff, uint16_t len, int rxq_remain, void *nif, uint8_t mbuf);
#else
int xrlink_net_to_host(uint8_t *buff, uint16_t len, int rxq_remain, uint8_t mbuf);
#endif
int xradio_link_tx_data_pause(uint32_t flag);
int xradio_link_tx_data_resume(uint32_t flag);
int xradio_link_wifi_drv_txq_lock(void);
int xrlink_wifi_drv_tx_check_wakeup_bh(size_t queued);
void xradio_link_rx_buf_status_ind(uint8_t req, uint32_t mem_sum, uint32_t mem_max);
int xradio_link_request_firmware(struct param_wlan_bin *param, void *data, ptc_cmd_dev_op_t type);
void xradio_link_release_firmware(uint8_t type);
int xrlink_data_tx_th_sleeptime_set(uint16_t ms);
int xrlink_data_tx_th_sleeptime_get(void);

int wlan_fmac_xrlink_init(int is_ultra_standby);
void wlan_fmac_xrlink_deinit(int is_ultra_standby);

#endif
