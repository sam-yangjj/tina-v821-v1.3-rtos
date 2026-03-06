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

#ifndef __XRADIO_TXRX_H__
#define __XRADIO_TXRX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "xrlink.h"
#include "xrlink_debug.h"

int xrlink_get_data_tx_queued_num(void);
int xrlink_send_data2host(uint8_t type, void *data, uint16_t len, uint8_t force_tx, uint8_t mbuf);
int xrlink_flush_send_data2host(void);
int xrlink_send_cmd_ack(void *data, uint16_t data_len);
int xrlink_rx_com_process(struct xradio_hdr *xr_hdr, void *cfm, uint16_t cfm_len, uint8_t data_buf, uint8_t force_tx);
int xrlink_rx_data_process(void *data, uint16_t len, uint8_t type, uint8_t force_tx, uint8_t flag);
int xrlink_tx_com_cb(void *data, int data_len, int buf_sz);
int xrlink_data_tx_callback(void *data, int data_len, int buf_sz);
int xrlink_master_rx_ready_set(uint8_t ready);

int xrlink_tranc_early_init(void);
void xrlink_tranc_early_deinit(void);
int xrlink_tranc_init(void);
void xrlink_tranc_deinit(void);

#ifdef __cplusplus
}
#endif
#endif
