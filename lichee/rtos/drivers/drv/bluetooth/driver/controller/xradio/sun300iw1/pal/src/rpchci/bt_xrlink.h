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

#include "bt_command.h"

#define XRLINK_TX_QUEUE_SIZE                    512
#define XRLINK_RX_QUEUE_SIZE                    256

#define XRLINK_RX_DATA_PAUSE_FLAG_XRLINK_RXQ    (1 << 0)
#define XRLINK_RX_DATA_PAUSE_FLAG_MBUF_LIMIT    (1 << 1)
#define XRLINK_RX_DATA_PAUSE_FLAG_WIFI_DRV_TXQ  (1 << 2)

typedef void(*xrlink_recv_cb)(uint8_t *data, uint32_t len);

int bt_xradio_link_send_raw_data(uint8_t *data, uint32_t len, void *info);

int bt_xradio_link_reg_recv_cb(xrlink_recv_cb cb);

int bt_xradio_link_init(int is_ultra_standby);

void bt_xradio_link_deinit(int is_ultra_standby);

int bt_xrlink_net_to_host(uint8_t *buff, uint16_t len);

int bt_xradio_link_rx_data_pause(uint32_t flag);
int bt_xradio_link_rx_data_resume(uint32_t flag);
int bt_xradio_link_wifi_drv_txq_lock(void);

void bt_xradio_link_rx_buf_status_ind(uint8_t req,
		uint32_t mem_sum, uint32_t mem_max);

int bt_xradio_link_request_firmware(struct param_wlan_bin *param, void *data, protocol_cmd_dev_op_t type);
void bt_xradio_link_release_firmware(uint8_t type);

#endif
