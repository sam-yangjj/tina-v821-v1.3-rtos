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

#ifndef __WLAN_FW_TCP__
#define __WLAN_FW_TCP__

#include <unistd.h>
#include <hal_osal.h>
#include "public.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "net/wlan/wlan_ext_req.h"
#include "net_ctrl.h"
#include "cmd_defs.h"
#include "cmd_util.h"
#include "pm_mem.h"
#include "lwip/netdb.h"
#include "lwip/etharp.h"

#define FW_TCP_SERVER_IP       "rtpeer.allwinnertech.com" //"221.4.213.86" //"rtpeer.allwinnertech.com"
#define FW_TCP_SERVER_PORT     ( 3001U )
#define FW_TCP_HEART_INTERVAL  ( 60U )
#define FW_TCP_WAKEUP_MSG      "wakeup"

typedef struct {
	int tcp_keepalive_sock;
	volatile int fw_network_ok;
} wlan_fw_keep;

typedef struct p2p_keepalive_all{
	wlan_ext_p2p_svr_set_t svr;
	wlan_ext_p2p_wkp_param_set_t wkp_param;
	wlan_ext_p2p_kpalive_param_set_t param;
	wlan_ext_arp_kpalive_set_t arpkeepalive_param;
} p2p_keepalive_all_t;

int wlan_fw_low_power_keepalive_demo(void);
int wlan_fw_low_power_keepalive_demo_deinit(void);
int wlan_msg_packet_wakeup_callback(void *arg);
int wlan_msg_connect_lost_callback(void *arg);
int wlan_msg_ip_wakeup_callback(void *arg);
int wlan_fw_keepalive_network_deinit(wlan_fw_keep *tcp_kl);

#endif
