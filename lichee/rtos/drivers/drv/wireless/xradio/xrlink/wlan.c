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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/os/os.h"
#include "command.h"
#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "adapter/net_ctrl/net_ctrl.h"
#include "sys_ctrl/sys_ctrl.h"
#include "wlan.h"
#include "net/wlan/wlan_ext_req.h"

static wifi_country_code_t g_country_code = WIFI_COUNTRY_CODE_NONE;
static int wlan_net_event_report(uint8_t event, enum wlan_mode mode)
{
	SLAVE_WLAN_DBG("wlan_net_event_report:%d, mode:%d\n", EVENT_SUBTYPE(event), mode);

	if (WLAN_MODE_STA == mode) {
#ifdef CONFIG_WLAN_STA
		return ptc_command_sta_connect_event(event);
#endif
	} else if (WLAN_MODE_HOSTAP == mode) {
#ifdef CONFIG_WLAN_AP
		return ptc_command_ap_connect_event(event);
#endif
	}
	return 0;
}

static void wlan_net_ctrl_msg_proc(uint32_t event, uint32_t data, void *arg)
{
#ifdef CONFIG_STA_SOFTAP_COEXIST
	struct netif *nif = NULL;
	if (data == WLAN_MODE_HOSTAP || event == NET_CTRL_MSG_WLAN_AP_START ||
		event == NET_CTRL_MSG_WLAN_AP_STOP)
		nif = ethernetif_get_netif(WLAN_MODE_HOSTAP);
	else
		nif = ethernetif_get_netif(WLAN_MODE_STA);
#else
	struct netif *nif = ethernetif_get_netif(WLAN_MODE_NONE);
#endif

	uint16_t type = EVENT_SUBTYPE(event);

	SLAVE_WLAN_DBG("net event %d\r\n", type);

	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_CONNECTED, WLAN_MODE_STA);
			if (NET_IS_IP4_VALID(nif))
				wlan_net_event_report(XR_WIFI_DHCP_SUCCESS, WLAN_MODE_STA);
		}
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_DISCONNECTED, WLAN_MODE_STA);
		}
		break;
	case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
		if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_SCAN_STARTED, WLAN_MODE_STA);
		}
		set_wlan_scan_state(1);
		break;
	case NET_CTRL_MSG_WLAN_SCAN_FAILED:
		if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_SCAN_FAILED, WLAN_MODE_STA);
		}
		break;
	case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
		if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_PASSWORD_INCORRECT, WLAN_MODE_STA);
		}
		break;
	case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
		if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_NETWORK_NOT_FOUND, WLAN_MODE_STA);
		}
		break;
	case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
	case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
		if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_DEAUTH, WLAN_MODE_STA);
		}
		break;
	case NET_CTRL_MSG_NETWORK_UP:
		if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_DHCP_SUCCESS, WLAN_MODE_STA);
		}
		set_wlan_scan_state(0);
		break;
	case NET_CTRL_MSG_NETWORK_DOWN:
		if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_NETWORK_DOWN, WLAN_MODE_STA);
		}
		break;
	case NET_CTRL_MSG_WLAN_AP_STA_CONNECTED:
		if (WLAN_MODE_HOSTAP == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_AP_STA_CONNECTED, WLAN_MODE_HOSTAP);
		}
		break;
	case NET_CTRL_MSG_WLAN_AP_STA_DISCONNECTED:
		if (WLAN_MODE_HOSTAP == ethernetif_get_mode(nif)) {
			wlan_net_event_report(XR_WIFI_AP_STA_DISCONNECTED, WLAN_MODE_HOSTAP);
		}
		break;
	default:
		SLAVE_WLAN_DBG("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

typedef struct {
	wlan_user_msg_cb init_cb;
	wlan_user_msg_cb deinit_cb;
}user_wlan_ctrl_msg_cb;

static user_wlan_ctrl_msg_cb user_wlan_ctrl_msg_cb_f;

static observer_base *ob = NULL;
int wlan_net_ctrl_init(void)
{
	if (!ob) {
		ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK,
		                                  NET_CTRL_MSG_ALL,
		                                  wlan_net_ctrl_msg_proc,
		                                  NULL);
		if (ob == NULL) {
			SLAVE_WLAN_DBG("%s:ob is NULL\n", __func__);
			return -1;
		}
		if (sys_ctrl_attach(ob) != 0) {
			SLAVE_WLAN_DBG("%s:sys_ctrl_attach error\n", __func__);
			return -1;
		}
		SLAVE_WLAN_DBG("%s:success\n", __func__);
	}
	SLAVE_WLAN_DBG("%s:already success\n", __func__);

#ifdef CONFIG_COMPONENTS_LOW_POWER_APP
	if (user_wlan_ctrl_msg_cb_f.init_cb != NULL) {
		user_wlan_ctrl_msg_cb_f.init_cb();
	}
#endif

	return 0;
}

int wlan_net_ctrl_deinit(void)
{
	SLAVE_WLAN_DBG("wlan net ctrl deinit\n");

	if (ob)
		sys_ctrl_detach(ob);
	if (ob)
		sys_callback_observer_destroy(ob);

	ob = NULL;

#ifdef CONFIG_COMPONENTS_LOW_POWER_APP
	if (user_wlan_ctrl_msg_cb_f.deinit_cb != NULL) {
		user_wlan_ctrl_msg_cb_f.deinit_cb();
	}
#endif

	return 0;
}

int wlan_net_ctrl_msg_register(wlan_user_msg_cb init_cb, wlan_user_msg_cb deinit_cb)
{
	user_wlan_ctrl_msg_cb_f.init_cb = init_cb;
	user_wlan_ctrl_msg_cb_f.deinit_cb = deinit_cb;
	return 0;
}

static int wifi_set_scan_freq(wlan_ext_scan_freq_t *scan_freq)
{
	int ret;
	struct netif *nif = ethernetif_get_netif(WLAN_MODE_NONE);
	ret = wlan_ext_request(nif, WLAN_EXT_CMD_SET_SCAN_FREQ, (int)(scan_freq));
	return ret;
}

int wifi_set_countrycode(wifi_country_code_t countrycode)
{
	int ret = 0;
	int us_scan_freq_list[11] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462};
	int jp_scan_freq_list[14] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484};
	int other_scan_freq_list[13] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};
	wlan_ext_scan_freq_t scan_freq;

	if ((countrycode < WIFI_COUNTRY_CODE_AU) || (countrycode >= WIFI_COUNTRY_CODE_NONE)) {
		SLAVE_WLAN_ERR("countrycode not support param!\n");
		return -1;
	}
	memset(&scan_freq, 0, sizeof(wlan_ext_scan_freq_t));
	switch (countrycode) {
		case WIFI_COUNTRY_CODE_CA:
		case WIFI_COUNTRY_CODE_US: {
			scan_freq.freq_num = 11;
			scan_freq.freq_list = us_scan_freq_list;
		}
		break;
		case WIFI_COUNTRY_CODE_JP: {
			scan_freq.freq_num = 14;
			scan_freq.freq_list = jp_scan_freq_list;
		}
		break;
		case WIFI_COUNTRY_CODE_AU:
		case WIFI_COUNTRY_CODE_CN:
		case WIFI_COUNTRY_CODE_DE:
		case WIFI_COUNTRY_CODE_EU:
		case WIFI_COUNTRY_CODE_FR:
		case WIFI_COUNTRY_CODE_RU:
		case WIFI_COUNTRY_CODE_SA: {
			scan_freq.freq_num = 13;
			scan_freq.freq_list = other_scan_freq_list;
		}
		break;
		default: {
			SLAVE_WLAN_ERR("not support param!\n");
			return -1;
		}
		break;
	}

	ret = wifi_set_scan_freq(&scan_freq);
	if (ret < 0) {
		SLAVE_WLAN_ERR("set scan freq error\r\n");
		return ret;
	}

	g_country_code = countrycode;

	SLAVE_WLAN_DBG("set country code success\n");
	return ret;
}

wifi_country_code_t wifi_get_countrycode(void)
{
	return g_country_code;
}
