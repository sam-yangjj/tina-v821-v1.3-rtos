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
#include "xrlink.h"
#include "txrx.h"
#include "command.h"
#include "lwip/dns.h"
#include "sysinfo.h"
#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "sysinfo/sysinfo.h"
#include "adapter/net_ctrl/net_ctrl.h"
#include "adapter/net_ctrl/net_init.h"
#include "cmd_util.h"
#include "adapter/cmd_wlan/cmd_wlancmd.h"
#ifdef CONFIG_DRIVERS_XRADIO_CMD
#include "adapter/cmd_wlan/cmd_wlan.h"
#endif
#include "wlan.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "net/wlan/wlan_ext_req.h"

#define XRLINK_COMMAND_THREAD_SIZE    2048
#define WAIT_FW_SEM_MS                5000
#define XRLINK_CMD_BUF_LEN            496

#define PTC_CMD_TABLE_SIZE(a)         (sizeof(a)/sizeof(a[0]))
#define xrlink_cmd_malloc              malloc
#define xrlink_cmd_free                free

#define xrlink_cmd_dump(str, data, len)       //data_hex_dump(str, 16, data, len)
#define xrlink_cmd_err_dump(str, data, len)   data_hex_dump(str, 16, data, len)

#ifdef CONFIG_WLAN_STA
static xr_wifi_sta_list_t *sta_networklist = NULL;
#endif

static char ifname[IFNAME_MAX_LEN];
#ifdef CONFIG_ARCH_SUN300IW1
xr_unsaved_bss
#endif
static uint8_t wifi_scan_state = 0;

#define XRLINK_CMD_EXP_RSP_DATA_SIZE    256
static uint8_t record_mac[6] = {0};
#ifdef CONFIG_ARCH_SUN300IW1
xr_unsaved_bss
#endif

#ifdef CONFIG_WLAN_SUPPORT_EXPAND_CMD
static xr_wifi_expand_cmd_data_t *xrlink_cmd_expand_rsp_data;
#endif

#if (defined CONFIG_WLAN_RAW_PACKET_FEATURE) || (defined CONFIG_WLAN_MONITOR)
static pro_raw_recv_cb raw_recv_cb = NULL;
#endif

enum cmd_status cmd_rf_exec(char *cmd);

static ptc_cmdpl_t *ptc_low_cmd_payload_zalloc(uint16_t type, void *param, uint16_t param_len)
{
	ptc_cmdpl_t *payload = NULL;
	struct xradio_hdr *xr_hdr = NULL;

	xr_hdr = (struct xradio_hdr *)xrlink_cmd_malloc(XR_HDR_SZ + sizeof(ptc_cmdpl_t) + param_len);
	if (!xr_hdr)
		return NULL;

	memset(xr_hdr, 0, XR_HDR_SZ + sizeof(ptc_cmdpl_t) + param_len);
	payload = (void *)xr_hdr->payload;
	payload->type = type;
	payload->len = param_len;
	if (param)
		memcpy(payload->param, param, param_len);

	return payload;
}

static void ptc_low_cmd_payload_free(ptc_cmdpl_t *cmd_payload)
{
	if (cmd_payload) {
		struct xradio_hdr *xr_hdr = NULL;

		xr_hdr = container_of((void *)cmd_payload, struct xradio_hdr, payload);
		xrlink_cmd_free(xr_hdr);
	}
}

static int ptc_low_cmd_send_large_data_by_databuf(uint8_t type, void *cmd_payload, uint16_t len,
	                                              void *cfm, uint16_t cfm_len, uint8_t force_tx)
{
	int ret = -1;
	struct xradio_hdr *xr_hdr = NULL;

	xr_hdr = container_of(cmd_payload, struct xradio_hdr, payload);
	xr_hdr->type = type;
	xr_hdr->len = len;
	ret = xrlink_rx_com_process(xr_hdr, cfm, cfm_len, 1, force_tx);

	return ret;
}

static int ptc_low_cmd_send_cmd_ack(void *data, uint16_t len)
{
	return xrlink_send_cmd_ack(data, len);
}

static int ptc_low_cmd_send_cmd(void *cmd_payload, uint16_t len, void *cfm, uint16_t cfm_len)
{
	int ret = -1;
	struct xradio_hdr *xr_hdr = NULL;

	xrlink_cmd_dump("cmdpl:", cmd_payload, len > 64 ? 64 : len);

	xr_hdr = container_of(cmd_payload, struct xradio_hdr, payload);
	xr_hdr->type = XR_TYPE_CMD;
	xr_hdr->len = len;
	// Generally, the cmd size cannot exceed the cmd buf size
	if ((xr_hdr->len + XR_HDR_SZ) > XRLINK_CMD_BUF_LEN)
		ret = xrlink_rx_com_process(xr_hdr, cfm, cfm_len, 1, 1);
	else
		ret = xrlink_rx_com_process(xr_hdr, cfm, cfm_len, 0, 1);

	return ret;
}

static int ptc_low_cmd_send_event(void *cmd_payload, uint16_t len)
{
	int ret = -1;
	struct xradio_hdr *xr_hdr = NULL;

	xr_hdr = container_of(cmd_payload, struct xradio_hdr, payload);
	xr_hdr->type = XR_TYPE_EVENT;
	xr_hdr->len = len;
	ret = xrlink_rx_com_process(xr_hdr, NULL, 0, 0, 1);

	return ret;
}

int get_wlan_scan_state(void)
{
	return wifi_scan_state;
}

void set_wlan_scan_state(int on)
{
	wifi_scan_state = on;
}

int ptc_command_notify_eth_state(uint8_t event)
{
	int ret = XR_OS_FAIL;
	ptc_cmdpl_t* pro = NULL;
	xr_wifi_dev_eth_state_t state;

	state.event = event;
	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_ETH_STATE, &state, sizeof(xr_wifi_dev_eth_state_t));

	if (pro) {
		ret = ptc_low_cmd_send_event((uint8_t *)pro, sizeof(ptc_cmdpl_t) + pro->len);
		ptc_low_cmd_payload_free(pro);
	} else {
		PTC_CMD_ERR("alloc err\n");
	}

	return ret;
}

/* XR_WIFI_HOST_HAND_WAY */
static int cmd_handway_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret = 0;
	struct cmd_para_hand_way *cmd_payload = (struct cmd_para_hand_way *)data->param;
	struct cmd_para_hand_way_res res = {0};

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	if (!data) {
		return -1;
	}

	ret = sysinfo_init();
	if (ret) {
		PTC_CMD_ERR("sys info init failed %d.\n", ret);
	}

	PTC_CMD_DBG("handway id = %d\r\n", cmd_payload->id);

	res.id = cmd_payload->id + 1;

	PTC_CMD_DBG("handway res status = %d, size = %d\r\n",
				res.id, sizeof(struct cmd_para_hand_way_res));

	*rsp = xrlink_cmd_malloc(sizeof(struct cmd_para_hand_way_res));
	if (!rsp) {
		PTC_CMD_ERR("rsp alloc err\n");
		return -1;
	}
	memcpy(*rsp, &res, sizeof(struct cmd_para_hand_way_res));
	*rsp_len = sizeof(struct cmd_para_hand_way_res);

	xrlink_master_rx_ready_set(1);

	return ret;
}

/* XR_WIFI_HOST-DEV_KEEP_AVLIE*/
static int cmd_keep_alive_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret = 0;
	struct cmd_para_keep_alive *cmd_payload = (struct cmd_para_keep_alive *)data->param;
	struct cmd_para_keep_alive res;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	if (!data) {
		return 0;
	}
	PTC_CMD_DBG("keep alive id = %d\r\n", cmd_payload->data);

	res.data = cmd_payload->data + 1;
	*rsp = xrlink_cmd_malloc(sizeof(struct cmd_para_keep_alive));
	if (!rsp) {
		PTC_CMD_ERR("rsp alloc err\n");
		return -1;
	}
	memcpy(*rsp, &res, sizeof(struct cmd_para_keep_alive));
	*rsp_len = sizeof(struct cmd_para_keep_alive);

	return ret;
}

/* XR_WIFI_HOST_DATA_TEST */
static int cmd_data_test_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret = 0;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	*rsp = xrlink_cmd_malloc(data->len);
	if (!rsp) {
		PTC_CMD_ERR("rsp alloc err\n");
		return -1;
	}
	memcpy(*rsp, data->param, data->len);
	*rsp_len = data->len;

	return ret;
}

static int cmd_wifi_open_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	enum wlan_mode wlan_set_mode;
	xr_wifi_on_t wlan_pattern;
	int ret = 0;
	xr_wifi_mode_t mode = *((xr_wifi_mode_t *)data->param);
	ptc_cmdpl_t *cmd;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	if (mode == XR_WIFI_STATION) {
		wlan_set_mode = WLAN_MODE_STA;
		PTC_CMD_DBG("wlan set sta mode\r\n");
	} else if (mode == XR_WIFI_AP) {
		wlan_set_mode = WLAN_MODE_HOSTAP;
		PTC_CMD_DBG("wlan set ap mode\r\n");
	} else if (mode == XR_WIFI_MONITOR) {
		wlan_set_mode = WLAN_MODE_MONITOR;
		PTC_CMD_DBG("wlan set monitor mode\r\n");
	} else {
		PTC_CMD_ERR("wlan mode not support\r\n");
		wlan_pattern.mode = XR_WLAN_STATUS_DOWN;
		goto end;
	}

	if (ethernetif_get_netif(wlan_set_mode)) {
		PTC_CMD_WRN("%s:mode %d is already opened!\n", __func__, wlan_set_mode);
		wlan_pattern.mode = XR_WLAN_STATUS_UP;
		goto end;
	}

#ifdef CONFIG_STA_SOFTAP_COEXIST
	ret = net_sys_start(wlan_set_mode);
	if (ret) {
		wlan_pattern.mode = XR_WLAN_STATUS_DOWN;
	} else {
		wlan_pattern.mode = XR_WLAN_STATUS_UP;
	}
#else
	if (g_wlan_netif) {
		ret = net_switch_mode(wlan_set_mode);
	} else {
		g_wlan_netif = net_open(wlan_set_mode);
	}
	if (ret || g_wlan_netif == NULL) {
		wlan_pattern.mode = XR_WLAN_STATUS_DOWN;
	} else {
		wlan_pattern.mode = XR_WLAN_STATUS_UP;
	}
#endif
end:
	cmd = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_DEV_STATUS, &wlan_pattern, sizeof(xr_wifi_on_t));
	if (cmd) {
		ret = ptc_low_cmd_send_cmd(cmd, sizeof(ptc_cmdpl_t) + cmd->len, NULL, 0);
		ptc_low_cmd_payload_free(cmd);
		if (ret)
			PTC_CMD_ERR("cmd send err:%d\n", ret);
	} else {
		PTC_CMD_ERR("cmd alloc err\n");
	}

	return ret;
}


/* XR_WIFI_HOST_OFF */
static int cmd_wifi_close_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	enum wlan_mode wlan_set_mode;
	xr_wifi_on_t wlan_pattern;
	xr_wifi_mode_t mode = *((xr_wifi_mode_t *)data->param);
	struct netif *nif = NULL;
	ptc_cmdpl_t *cmd;
	int ret = -1;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	if (mode == XR_WIFI_STATION) {
		wlan_set_mode = WLAN_MODE_STA;
		PTC_CMD_DBG("wlan set sta mode\r\n");
	} else if (mode == XR_WIFI_AP) {
		wlan_set_mode = WLAN_MODE_HOSTAP;
		PTC_CMD_DBG("wlan set ap mode\r\n");
	} else if (mode == XR_WIFI_MONITOR) {
		wlan_set_mode = WLAN_MODE_MONITOR;
		PTC_CMD_DBG("wlan set monitor mode\r\n");
	} else {
		PTC_CMD_ERR("wlan mode not support\r\n");
		wlan_pattern.mode = XR_WLAN_STATUS_UP;
		goto end;
	}

	nif = ethernetif_get_netif(wlan_set_mode);
	if (nif == NULL) {
		if (wlan_set_mode == WLAN_MODE_MONITOR) {
			// monitor disable already switch to sta mode, so nif is sta mode
			wlan_pattern.mode = XR_WLAN_STATUS_DOWN;
			PTC_CMD_WRN("monitor mode already down\r\n");
		} else {
			PTC_CMD_ERR("netif is NULL, wlan_set_mode:%d\r\n", wlan_set_mode);
			wlan_pattern.mode = XR_WLAN_STATUS_UP;
		}
		goto end;
	}

	if (NET_IS_IP4_VALID(nif)) {
		net_config(nif, 0); /* bring down netif */
	}

	if (wlan_set_mode == WLAN_MODE_STA && netif_is_link_up(nif)) {
		wlan_sta_disable(); /* disconnect wlan */
	}

#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (net_sys_stop(wlan_set_mode)) {
		wlan_pattern.mode = XR_WLAN_STATUS_UP;
		PTC_CMD_ERR("mode %d has been closed\n", wlan_set_mode);
	} else {
		wlan_pattern.mode = XR_WLAN_STATUS_DOWN;
	}
#else
	net_close(nif);
	g_wlan_netif = NULL;
	wlan_pattern.mode = XR_WLAN_STATUS_DOWN;
#endif

end:
	cmd = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_DEV_STATUS, &wlan_pattern, sizeof(xr_wifi_on_t));
	if (cmd) {
		ret = ptc_low_cmd_send_cmd(cmd, sizeof(ptc_cmdpl_t) + cmd->len, NULL, 0);
		ptc_low_cmd_payload_free(cmd);
		if (ret)
			PTC_CMD_ERR("cmd send err:%d\n", ret);
	} else {
		PTC_CMD_ERR("cmd alloc err\n");
	}

	return ret;
}

/* XR_WIFI_GET_SCAN_RES */
static int cmd_get_scan_res_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	xr_wifi_scan_param_t *scan_param = (xr_wifi_scan_param_t *)data->param;
	ptc_cmdpl_t *command;
	xr_wifi_scan_result_t *host_scan;
	xr_wifi_scan_info_t *host_scan_ap_info;
	wlan_sta_scan_results_t results = {0};
	uint8_t scan_size = 50;
	int i = 0;
	int sleep_times = 80;
	uint16_t alloc_size = 0;

#ifdef CONFIG_STA_SOFTAP_COEXIST
	struct netif *nif = ethernetif_get_netif(WLAN_MODE_STA);
#else
	struct netif *nif = ethernetif_get_netif(WLAN_MODE_NONE);
#endif

	if (nif == NULL) {
#ifdef CONFIG_STA_SOFTAP_COEXIST
		PTC_CMD_ERR("sta-ap coex is not support scan in ap mode.\n");
#else
		PTC_CMD_ERR("nif is null!\n");
#endif
		return -1;
	}

	wifi_scan_state = 0;
	PTC_CMD_DBG("current mode is %d\n", ethernetif_get_mode(nif));
	if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
		if (scan_param->enable) {
			/* scan hidden ssid */
			wlan_sta_config_t config;
			memset(&config, 0, sizeof(config));
			config.field = WLAN_STA_FIELD_SSID;
			memcpy(config.u.ssid.ssid, scan_param->ssid, scan_param->ssid_len);
			config.u.ssid.ssid_len = scan_param->ssid_len;
			if (wlan_sta_set_config(&config) != 0) {
				PTC_CMD_ERR("set ssid fail\n");
				return -1;
			}

			config.field = WLAN_STA_FIELD_SCAN_SSID;
			config.u.scan_ssid = 1;
			if (wlan_sta_set_config(&config) != 0) {
				PTC_CMD_ERR("set scan ssid fail\n");
				return -1;
			}
			PTC_CMD_INF("sta scan target ssid:%s\n", scan_param->ssid);
		}
		wlan_sta_scan_once();
		PTC_CMD_DBG("current mode is sta\r\n");
	} else if(WLAN_MODE_HOSTAP == ethernetif_get_mode(nif)) {
		wlan_ap_scan_once();
		PTC_CMD_DBG("current mode is ap\r\n");
	} else {
		PTC_CMD_ERR("mode %d error !\n", ethernetif_get_mode(nif));
		return -1;
	}

	while (!wifi_scan_state) {
		if (sleep_times <= 0)
			break;
		sleep_times--;
		XR_OS_MSleep(100);
	}

	alloc_size = scan_size * sizeof(wlan_sta_ap_t);
	results.ap = xrlink_cmd_malloc(alloc_size);
	if (!results.ap) {
		PTC_CMD_ERR("Scan results, malloc failed.\n");
		return -1;
	}
	memset(results.ap, 0, alloc_size);

	results.size = scan_size;
	if (WLAN_MODE_STA == ethernetif_get_mode(nif)) {
		wlan_sta_scan_result(&results);
		PTC_CMD_INF("current mode is sta\r\n");
	} else if(WLAN_MODE_HOSTAP == ethernetif_get_mode(nif)) {
		wlan_ap_scan_result(&results);
		PTC_CMD_INF("current mode is ap\r\n");
	}

	if (results.num == 0) {
		free(results.ap);
		PTC_CMD_ERR("Scan results is empty, results num:%d\n", results.num);
		return -1;
	}

	alloc_size = sizeof(xr_wifi_scan_result_t) + results.num * sizeof(xr_wifi_scan_info_t);
	command = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_SCAN_RES, NULL, alloc_size);
	if (command == NULL) {
		xrlink_cmd_free(results.ap);
		return -1;
	}

	host_scan = (xr_wifi_scan_result_t *)command->param;
	host_scan->num = results.num;
	host_scan_ap_info = (xr_wifi_scan_info_t *)host_scan->ap_info;

	PTC_CMD_DBG("scan number %d\n", host_scan->num);

	for (i = 0; i < host_scan->num; i++) {
		memcpy(host_scan_ap_info[i].bssid, results.ap[i].bssid, BSSID_MAX_LEN);

		PTC_CMD_DBG("ssid:%s freq: %d rssi:%d wpa_flags:%d\n", results.ap[i].ssid.ssid,
				results.ap[i].freq, results.ap[i].rssi, results.ap[i].wpa_flags);

		if (results.ap[i].ssid.ssid_len <= SSID_MAX_LEN) {
			memcpy(host_scan_ap_info[i].ssid, results.ap[i].ssid.ssid, results.ap[i].ssid.ssid_len);
		} else {
			PTC_CMD_ERR("Scan results ssid over flow:%s\n", results.ap[i].ssid.ssid);
			memcpy(host_scan_ap_info[i].ssid, results.ap[i].ssid.ssid, SSID_MAX_LEN);
		}
		host_scan_ap_info[i].ssid_len = results.ap[i].ssid.ssid_len;
		host_scan_ap_info[i].freq = results.ap[i].freq;
		host_scan_ap_info[i].rssi = results.ap[i].level;
		host_scan_ap_info[i].key_mgmt = XR_WIFI_SEC_NONE;
		if(results.ap[i].wpa_flags & WPA_FLAGS_WPA) {
			host_scan_ap_info[i].key_mgmt |= XR_WIFI_SEC_WPA_PSK;
		}
		if ((results.ap[i].wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == WPA_FLAGS_RSN) {
			host_scan_ap_info[i].key_mgmt |= XR_WIFI_SEC_WPA2_PSK;
		}
		if ((results.ap[i].wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) {
			host_scan_ap_info[i].key_mgmt |= XR_WIFI_SEC_WPA3_PSK;
		}
		if (results.ap[i].wpa_flags & WPA_FLAGS_WEP) {
			host_scan_ap_info[i].key_mgmt |= XR_WIFI_SEC_WEP;
		}
	}

	xrlink_cmd_free(results.ap);
	i = ptc_low_cmd_send_cmd(command, sizeof(ptc_cmdpl_t) + command->len, NULL, 0);
	ptc_low_cmd_payload_free(command);
	PTC_CMD_DBG("scan result type:%d ret:%d\n", XR_WIFI_DEV_SCAN_RES, i);

	return i;
}

/* XR_WIFI_SET_MAC */
static int cmd_set_mac_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	xr_wifi_mac_info_t *info = (xr_wifi_mac_info_t *)data->param;
	struct sysinfo *sysinfo;
	int ret = -1;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	sysinfo = sysinfo_get();
	if (sysinfo) {
		memcpy(sysinfo->mac_addr, info->mac, MAC_MAX_LEN);
		PTC_CMD_DBG("mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
				sysinfo->mac_addr[0], sysinfo->mac_addr[1], sysinfo->mac_addr[2],
				sysinfo->mac_addr[3], sysinfo->mac_addr[4], sysinfo->mac_addr[5]);
		memcpy(ifname, info->ifname, IFNAME_MAX_LEN);
		ret = 0;
	}

	return ret;
}

/* XR_WIFI_GET_MAC */
static int cmd_get_mac_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret = 0;
	ptc_cmdpl_t *pro = NULL;
	xr_wifi_mac_info_t info;
	struct sysinfo *sysinfo = sysinfo_get();

	if (sysinfo) {
		PTC_CMD_DBG("cmd handle %s\r\n", __func__);
		memcpy(info.mac, sysinfo->mac_addr, MAC_MAX_LEN);
		memcpy(info.ifname, ifname, IFNAME_MAX_LEN);
		PTC_CMD_DBG("mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
				info.mac[0], info.mac[1], info.mac[2],
				info.mac[3], info.mac[4], info.mac[5]);
	} else {
		return -1;
	}

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_MAC, &info, sizeof(xr_wifi_mac_info_t));
	if (pro) {
		ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}
	return ret;
}

/* XR_WIFI_HOST_GET_DEVICE_MAC_DRV_REQ */
static int cmd_get_mac_drv_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret = 0;
	xr_wifi_mac_info_t info = {0};
	struct sysinfo *sysinfo = sysinfo_get();
	static uint8_t cnt = 0;

	if (sysinfo) {
		PTC_CMD_DBG("cmd handle %s\r\n", __func__);
		memcpy(info.mac, record_mac, MAC_MAX_LEN);
		memcpy(info.ifname, ifname, IFNAME_MAX_LEN);
		PTC_CMD_DBG("mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
				info.mac[0], info.mac[1], info.mac[2],
				info.mac[3], info.mac[4], info.mac[5]);
	} else {
		return -1;
	}

	*rsp = xrlink_cmd_malloc(sizeof(xr_wifi_mac_info_t));
	if (!rsp) {
		PTC_CMD_ERR("rsp alloc err\n");
		return -1;
	}

	if (!cnt) {
		memset(*rsp, 0, sizeof(xr_wifi_mac_info_t));
		cnt = 1;
	} else {
		memcpy(*rsp, &info, sizeof(xr_wifi_mac_info_t));
	}
	*rsp_len = sizeof(xr_wifi_mac_info_t);

	return ret;
}

static int cmd_set_device_mac_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret = 0;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	struct cmd_para_mac_info *info = (struct cmd_para_mac_info *)data->param;
	struct sysinfo *sysinfo = sysinfo_get();

	if (sysinfo) {
		memcpy(sysinfo->mac_addr, info->mac, MAC_MAX_LEN);
		memcpy(record_mac, info->mac, MAC_MAX_LEN);
		PTC_CMD_DBG("mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
				sysinfo->mac_addr[0], sysinfo->mac_addr[1], sysinfo->mac_addr[2],
				sysinfo->mac_addr[3], sysinfo->mac_addr[4], sysinfo->mac_addr[5]);

		memcpy(ifname, info->ifname, IFNAME_MAX_LEN);
	} else {
		ret = -1;
		PTC_CMD_ERR("sysinfo is null\n");
	}

	return ret;
}

/* XR_WIFI_HOST_SET_EFUSE */
static int cmd_set_efuse_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	return 0;
}

/* XR_WIFI_POWER */
static int cmd_wlan_power_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	enum wlan_mode wlan_set_mode;
	struct cmd_wlan_power_data *wlan_date = (struct cmd_wlan_power_data *)data->param;
	int ret = -1;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	if (wlan_date->mode == XR_WIFI_STATION) {
		wlan_set_mode = WLAN_MODE_STA;
		PTC_CMD_DBG("wlan set sta mode\r\n");
	} else if (wlan_date->mode == XR_WIFI_AP) {
		wlan_set_mode = WLAN_MODE_HOSTAP;
		PTC_CMD_DBG("wlan set ap mode\r\n");
	} else if (wlan_date->mode == XR_WIFI_MONITOR) {
		wlan_set_mode = WLAN_MODE_MONITOR;
		PTC_CMD_DBG("wlan set monitor mode\r\n");
	} else {
		PTC_CMD_DBG("wlan mode not support\r\n");
		return -1;
	}

	if (wlan_date->enable) {
		if (0 == xr_wlan_on(wlan_set_mode)) {
			wlan_net_ctrl_init();
			ret = 0;
		} else {
			ret = -1;
			PTC_CMD_ERR("wlan on err\n");
		}
	} else {
		wlan_net_ctrl_deinit();
		if (0 == xr_wlan_off()) {
			ret = 0;
		} else {
			ret = -1;
			PTC_CMD_ERR("wlan off err\n");
		}
	}
	PTC_CMD_DBG("wlan pwr on:%d ret:%d\n", wlan_date->enable, ret);

	return ret;
}

/* XR_WIFI_HOST_SET_COUNTRY_CODE */
static int cmd_set_country_code_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	int ret = -1;
	wifi_country_code_info_t *country_code = (wifi_country_code_info_t *)data->param;
	PTC_CMD_DBG("set country code = %d\r\n", country_code->code);
	ret = wifi_set_countrycode(country_code->code);
	if (ret) {
		PTC_CMD_ERR("set country code failed %d.\n", ret);
	}

	return ret;
}

/* XR_WIFI_HOST_GET_COUNTRY_CODE */
static int cmd_get_country_code_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	int ret;
	ptc_cmdpl_t *pro = NULL;
	wifi_country_code_info_t country_code;
	country_code.code = wifi_get_countrycode();
	PTC_CMD_DBG("get country code = %d\r\n", country_code.code);

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_COUNTRY_CODE, &country_code, sizeof(wifi_country_code_info_t));
	if (pro) {
		ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}
	return ret;
}

/* XR_WIFI_HOST_GET_RTOS_VERSION */
static char rtos_version[] = "1.00.00";
static int cmd_get_rtos_version_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	int ret = 0;
	ptc_cmdpl_t *pro = NULL;
	PTC_CMD_DBG("rtos_version = %s\n", rtos_version);
	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_RTOS_VERSION, rtos_version, strlen(rtos_version));
	if (pro) {
		ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}
	return ret;
}

#ifdef CONFIG_WLAN_STA
static int ptc_report_ip_config(void)
{
	int ret = -1;
	ptc_cmdpl_t* pro = NULL;

	xr_wifi_sta_ip_info_t ip_info;
	const ip_addr_t *getdnsip;
	struct netif * ethernetif = ethernetif_get_netif(WLAN_MODE_STA);

	if (ethernetif) {
		memcpy(ip_info.ip_addr, (char *)&ethernetif->ip_addr, IP_MAX_LEN);
		memcpy(ip_info.netmask, (char *)&ethernetif->netmask, NETMASK_MAX_LEN);
		memcpy(ip_info.gw, (char *)&ethernetif->gw, GW_MAX_LEN);
		getdnsip = dns_getserver(0);
		memcpy(ip_info.dns, (char *)getdnsip, DNS_MAX_LEN);
	}

	PTC_CMD_DBG("address: %s\n", inet_ntoa(ip_info.ip_addr));
	PTC_CMD_DBG("gateway: %s\n", inet_ntoa(ip_info.gw));
	PTC_CMD_DBG("netmask: %s\n", inet_ntoa(ip_info.netmask));
	PTC_CMD_DBG("dns: %s\n", inet_ntoa(ip_info.dns));

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_IND_IP_INFO, &ip_info, sizeof(xr_wifi_sta_ip_info_t));

#if LWIP_IPV4
	if (NET_IS_IP4_VALID(ethernetif) && netif_is_link_up(ethernetif))
#endif
		ptc_command_notify_eth_state(XR_ETH0_START);
#if LWIP_IPV4
	else
		PTC_CMD_WRN("Do not start eth0 !\n");
#endif

	if (pro) {
		ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	}

	return ret;
}

static int wlan_sta_set_wep(uint8_t *ssid, uint8_t ssid_len, uint8_t *psk, int auth_alg, uint8_t use_bssid)
{
	wlan_sta_config_t config;

	if (((ssid == NULL) || (ssid_len == 0) || (ssid_len > WLAN_SSID_MAX_LEN)) && (use_bssid == 0)) {
		PTC_CMD_ERR("invalid ssid (%p, %u)\n", ssid, ssid_len);
		return -1;
	} else if ((auth_alg != WPA_AUTH_ALG_OPEN) && (auth_alg != WPA_AUTH_ALG_SHARED)) {
		PTC_CMD_ERR("invalid auth_alg 0x%x\n", auth_alg);
		return -1;
	}

	memset(&config, 0, sizeof(config));

	if (!use_bssid) {
		config.field = WLAN_STA_FIELD_SSID;
		memcpy(config.u.ssid.ssid, ssid, ssid_len);
		config.u.ssid.ssid_len = ssid_len;
		if (wlan_sta_set_config(&config) != 0)
			return -1;
	}

	/* WEP key0 */
	config.field = WLAN_STA_FIELD_WEP_KEY0;
	strlcpy((char *)config.u.wep_key, (char *)psk, sizeof(config.u.wep_key));
	if (wlan_sta_set_config(&config) != 0)
		return -1;

	/* WEP key index */
	config.field = WLAN_STA_FIELD_WEP_KEY_INDEX;
	config.u.wep_tx_keyidx = 0;
	if (wlan_sta_set_config(&config) != 0)
		return -1;

	/* auth_alg: OPEN or SHARED */
	config.field = WLAN_STA_FIELD_AUTH_ALG;
	config.u.auth_alg = auth_alg;
	if (wlan_sta_set_config(&config) != 0)
		return -1;

	/* key_mgmt: NONE */
	config.field = WLAN_STA_FIELD_KEY_MGMT;
	config.u.key_mgmt = WPA_KEY_MGMT_NONE;
	if (wlan_sta_set_config(&config) != 0)
		return -1;

	return 0;
}

///////////////////////////////////////////////////////////
/* XR_WIFI_STA_CONNECT */
static int cmd_sta_connect_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret;
	wlan_sta_states_t state;
	xr_wifi_sta_cn_t *connect_param = (xr_wifi_sta_cn_t *)data->param;
	int ssid_len = connect_param->ssid_len;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	ret = wlan_sta_state(&state);
	if (ret == 0 && state == WLAN_STA_STATE_CONNECTED) {
		wlan_sta_ap_t ap;

		if (wlan_sta_ap_info(&ap) == 0 && (!strncmp(connect_param->ssid, (char *)ap.ssid.ssid, ap.ssid.ssid_len)
			|| !memcmp(connect_param->bssid, ap.bssid, BSSID_MAX_LEN))) {
			ptc_cmdpl_t *pro = NULL;
			xr_wifi_sta_cn_event_t connect;
			ptc_report_ip_config();
			PTC_CMD_INF("wifi is already connect.\n");
			connect.event = XR_WIFI_DHCP_SUCCESS;
			pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_STA_CN_EV,
			                                 &connect, sizeof(xr_wifi_sta_cn_event_t));
			if (pro) {
				ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
				ptc_low_cmd_payload_free(pro);
			} else {
				PTC_CMD_ERR("alloc fail.\n");
			}

			return ret;
		}
	}

	PTC_CMD_DBG("sta connect sec: 0x%x\r\n", connect_param->sec);
	if (ssid_len) {
		PTC_CMD_DBG("sta connect ssid: %s, len = %d.\r\n", connect_param->ssid, ssid_len);
	} else {
		PTC_CMD_DBG("sta connect bssid: %02x:%02x:%02x:%02x:%02x:%02x\r\n", connect_param->bssid[0],
		            connect_param->bssid[1],connect_param->bssid[2], connect_param->bssid[3],
		            connect_param->bssid[4], connect_param->bssid[5]);
	}
	PTC_CMD_DBG("sta connect psk : %s\r\n", connect_param->pwd);
	PTC_CMD_DBG("sta connect auth_alg : %x\r\n", connect_param->auth_alg);

	if (ssid_len) {
		if (connect_param->sec == XR_WIFI_SEC_NONE) {
			ret = wlan_sta_set((uint8_t *)connect_param->ssid, ssid_len, NULL);
		} else {
			if (connect_param->sec == XR_WIFI_SEC_WEP) {
				wlan_sta_set_wep((uint8_t *)connect_param->ssid, ssid_len,
					(uint8_t *)connect_param->pwd, connect_param->auth_alg, 0);
			} else {
				wlan_sta_set((uint8_t *)connect_param->ssid, ssid_len,
					(uint8_t *)connect_param->pwd);
			}
		}
	} else {
		wlan_sta_config_t config;
		memset(&config, 0, sizeof(config));
		config.field = WLAN_STA_FIELD_BSSID;
		memcpy(config.u.bssid.bssid, connect_param->bssid, BSSID_MAX_LEN);
		config.u.bssid.bssid_set = 1;
		if (wlan_sta_set_config(&config) != 0) {
			PTC_CMD_ERR("set bssid fail\n");
			return -1;
		}

		switch (connect_param->sec) {
			case XR_WIFI_SEC_NONE: {
				config.field = WLAN_STA_FIELD_KEY_MGMT;
				config.u.key_mgmt = WPA_KEY_MGMT_NONE;
				if (wlan_sta_set_config(&config) != 0) {
					PTC_CMD_ERR("set key_mgmt fail\n");
					return -1;
				}
				break;
			}
			case XR_WIFI_SEC_WEP: {
				if (wlan_sta_set_wep(NULL, 0, (uint8_t *)connect_param->pwd,
					connect_param->auth_alg, 1)) {
					PTC_CMD_ERR("set wep fail\n");
					return -1;
				}
				break;
			}
			default:{
				config.field = WLAN_STA_FIELD_PSK;
				strlcpy((char *)config.u.psk, connect_param->pwd, sizeof(config.u.psk));
				if (wlan_sta_set_config(&config) != 0) {
					PTC_CMD_ERR("set psk fail\n");
					return -1;
				}

				/* key_mgmt: PSK, PSK_SHA256, SAE */
				config.field = WLAN_STA_FIELD_KEY_MGMT;
				config.u.key_mgmt = (WPA_KEY_MGMT_PSK | WPA_KEY_MGMT_PSK_SHA256 | WPA_KEY_MGMT_SAE);
				if (wlan_sta_set_config(&config) != 0) {
					PTC_CMD_ERR("set key_mgmt fail\n");
					return -1;
				}

				/* auth_alg: AUTO, wpa_supplicant will do automatic selection */
				config.field = WLAN_STA_FIELD_AUTH_ALG;
				config.u.auth_alg = WPA_AUTH_ALG_AUTO;
				if (wlan_sta_set_config(&config) != 0) {
					PTC_CMD_ERR("set auth_alg fail\n");
					return -1;
				}
				break;
			}
		}
	}

	if (wlan_sta_disable() != 0) {
		PTC_CMD_ERR("wlan_sta_disable fail\n");
		return -1;
	}

	if (wlan_sta_connect() != 0) {
		PTC_CMD_ERR("wlan_sta_connect fail\n");
		return -1;
	}

	if (wlan_sta_enable() != 0) {
		PTC_CMD_ERR("wlan_sta_enable fail\n");
		return -1;
	}

	return ret;
}

/* XR_WIFI_STA_DISCONNECT */
static int cmd_sta_disconnect_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	ret = wlan_sta_disable();

	return ret;
}

/* XR_WIFI_STA_AUTO_RECONNECT */
static int cmd_sta_auto_reconnect_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret;
	wlan_sta_states_t state;
	wifi_sta_auto_reconnect_t en = *((wifi_sta_auto_reconnect_t *)data->param);

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	PTC_CMD_DBG("sta auto reconnect en = %d\r\n", en);

	ret = wlan_sta_state(&state);
	if (en && ret == 0 && state == WLAN_STA_STATE_CONNECTED) {
		xr_wifi_sta_cn_event_t connect;
		ptc_cmdpl_t *pro = NULL;

		ptc_report_ip_config();
		connect.event = XR_WIFI_DHCP_SUCCESS;

		pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_STA_CN_EV,
		                                 &connect, sizeof(xr_wifi_sta_cn_event_t));
		if (pro) {
			ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
			ptc_low_cmd_payload_free(pro);
		} else {
			PTC_CMD_ERR("alloc fail.\n");
		}
		return ret;
	}
	wlan_sta_set_autoconnect(en);
	return ret;
}

static int wlan_get_cur_signal(int *rssi)
{
	int ret;
	wlan_ext_signal_t signal;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_SIGNAL, (uint32_t)(uintptr_t)(&signal));
	if (ret) {
		PTC_CMD_ERR("%s: fail ret:%d\n", __func__, ret);
		return ret;
	}
	PTC_CMD_DBG("current rssi:%d(snr, 0.5db), noise:%d(dbm), level:%d(dbm)\n", signal.rssi,
	            signal.noise, signal.rssi / 2 + signal.noise);

	*rssi = signal.rssi / 2 + signal.noise;

	return 0;
}

/* XR_WIFI_STA_GET_STATUS */
static int cmd_sta_get_info_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	xr_wifi_sta_info_t info;
	wlan_sta_ap_t ap = {0};
	int ret = 0;
	ptc_cmdpl_t *pro = NULL;

	struct sysinfo *sysinfo = sysinfo_get();
	memset(&info, 0, sizeof(xr_wifi_sta_info_t));
	memcpy(info.mac_addr, sysinfo->mac_addr, MAC_MAX_LEN);

	if (!wlan_sta_ap_info(&ap)) {
		int rssi;

		info.id = -1; /* network ID, useless on xrlink */
		info.freq = ap.freq;
		info.rssi = ap.level;
		if (!wlan_get_cur_signal(&rssi))
			info.rssi = rssi;
		memcpy(info.bssid, ap.bssid, BSSID_MAX_LEN);
		memcpy(info.ssid, ap.ssid.ssid, ap.ssid.ssid_len);

		if(ap.wpa_flags & WPA_FLAGS_WPA) {
			info.sec = XR_WIFI_SEC_WPA_PSK;
		} else if ((ap.wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == WPA_FLAGS_RSN) {
			info.sec = XR_WIFI_SEC_WPA2_PSK;
		} else if ((ap.wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) {
			info.sec = XR_WIFI_SEC_WPA3_PSK;
		} else if (ap.wpa_flags & WPA_FLAGS_WEP) {
			info.sec = XR_WIFI_SEC_WEP;
		} else {
			info.sec = XR_WIFI_SEC_NONE;
		}
	}

	struct netif * ethernetif = ethernetif_get_netif(WLAN_MODE_STA);
	if (ethernetif) {
		memcpy(info.ip_addr, (char *)&ethernetif->ip_addr, IP_MAX_LEN);
		memcpy(info.netmask, (char *)&ethernetif->netmask, NETMASK_MAX_LEN);
		memcpy(info.gw, (char *)&ethernetif->gw, GW_MAX_LEN);
	}
	PTC_CMD_DBG("sta info ip %s\r\n", inet_ntoa(info.ip_addr));
	PTC_CMD_DBG("sta info netmask %s\r\n", inet_ntoa(info.netmask));
	PTC_CMD_DBG("sta info gw %s\r\n", inet_ntoa(info.gw));

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_STA_INFO, &info, sizeof(xr_wifi_sta_info_t));
	if (pro) {
		ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}

	return ret;
}

static void cmd_sta_list_init(void)
{
	sta_networklist = (xr_wifi_sta_list_t *)malloc(sizeof(xr_wifi_sta_list_t)  +
		sizeof(xr_wifi_sta_list_nod_t) * STORE_LIST_NUM);
	memset(sta_networklist, 0, sizeof(xr_wifi_sta_list_t)  +
		sizeof(xr_wifi_sta_list_nod_t) * STORE_LIST_NUM);
	sta_networklist->sys_list_num = 0;
	sta_networklist->list_num = STORE_LIST_NUM;
}

int cmd_sta_list_add(void)
{
	int i;
	xr_wifi_sta_list_nod_t *node;

	uint32_t bss_size;
	wlan_sta_bss_info_t bss_info;
	wlan_ssid_psk_t ssid_psk;

	if (!sta_networklist) {
		cmd_sta_list_init();
	}

	wlan_sta_get_ap_ssid_psk(&ssid_psk);
	//PTC_CMD_DBG("ssid_psk ssid=%s\r\n", ssid_psk->ssid);
	wlan_sta_get_bss_size(&bss_size);
	//PTC_CMD_DBG("bss_size size = %d\r\n", bss_size);
	bss_info.size = bss_size;
	bss_info.bss = xrlink_cmd_malloc(bss_size);
	if (!bss_info.bss) {
		return 0;
	}
	wlan_sta_get_bss(&bss_info);

	if (sta_networklist->sys_list_num == (sta_networklist->list_num - 1)) {
		PTC_CMD_WRN("list is ful, sys_list_num = %d\r\n", sta_networklist->list_num);

		xrlink_cmd_free(bss_info.bss);
		return 0;
	}

	PTC_CMD_DBG("list add sys_list_num = %d\r\n", sta_networklist->sys_list_num);
	if (!sta_networklist->sys_list_num) {
		node = (xr_wifi_sta_list_nod_t*)(sta_networklist->list_nod +
			sizeof(xr_wifi_sta_list_nod_t) * sta_networklist->sys_list_num);
		memcpy(node->ssid, ssid_psk.ssid, ssid_psk.ssid_len);
		memcpy(node->bssid, bss_info.bss, BSSID_MAX_LEN);

		sta_networklist->sys_list_num = 1;
		node->id = sta_networklist->sys_list_num;
		xrlink_cmd_free(bss_info.bss);
		return 0;
	}

	for (i = 0; i < sta_networklist->sys_list_num; i++) {
		node = (xr_wifi_sta_list_nod_t*)(sta_networklist->list_nod + sizeof(xr_wifi_sta_list_nod_t) * i);
		if (!memcmp(ssid_psk.ssid, node->ssid, ssid_psk.ssid_len)) {
			xrlink_cmd_free(bss_info.bss);
			return -1;
		}
	}

	if (sta_networklist->sys_list_num < sta_networklist->list_num) {
		node = (xr_wifi_sta_list_nod_t*)(sta_networklist->list_nod +
			sizeof(xr_wifi_sta_list_nod_t) * sta_networklist->sys_list_num);
		memcpy(node->ssid, ssid_psk.ssid, ssid_psk.ssid_len);
		memcpy(node->bssid, bss_info.bss, BSSID_MAX_LEN);

		sta_networklist->sys_list_num++;
		node->id = sta_networklist->sys_list_num;
	}

	xrlink_cmd_free(bss_info.bss);
	return 0;
}

static int cmd_sta_list_remove(char *ssid, int ssid_len)
{
	int i;
	int index = 0;

	xr_wifi_sta_list_nod_t *node;

	if (!sta_networklist) {
		return -1;
	}
	if (!sta_networklist->sys_list_num) {
		return -1;
	}

	PTC_CMD_DBG("remove_list sys_list_num=%d\r\n", sta_networklist->sys_list_num);

	for (i = 0; i <= (sta_networklist->sys_list_num-1); i++) {

		node = (xr_wifi_sta_list_nod_t*)(sta_networklist->list_nod +
		        sizeof(xr_wifi_sta_list_nod_t) * i);

		if (!memcmp(ssid, node->ssid, ssid_len)) {
			PTC_CMD_DBG("remove found ssid, index=%d\r\n", index);
			break;
		}
	}
	index = i;
	if (index >= sta_networklist->sys_list_num) {
		PTC_CMD_DBG("remove not found ssid\r\n");

	} else if (index == (sta_networklist->sys_list_num-1)) {
		PTC_CMD_DBG("remove found ssid, the last ssid, index=%d\r\n", index);

		sta_networklist->sys_list_num--;

		node = (xr_wifi_sta_list_nod_t*)(sta_networklist->list_nod +
		        sizeof(xr_wifi_sta_list_nod_t) * index);

		memset(node, 0, sizeof(xr_wifi_sta_list_nod_t));

	} else {

		PTC_CMD_DBG("remove found ssid, ssid index=%d\r\n", index);

		for (i = index; i < (sta_networklist->sys_list_num-1); i++) {
			node = (xr_wifi_sta_list_nod_t*)(sta_networklist->list_nod +
					sizeof(xr_wifi_sta_list_nod_t) * index);
			memcpy(node, node+1, sizeof(xr_wifi_sta_list_nod_t));
			memset(node+1, 0, sizeof(xr_wifi_sta_list_nod_t));
		}
		sta_networklist->sys_list_num--;
	}

	return 0;
}

/* XR_WIFI_HOST_STA_LIST_NETWORKS */
static int cmd_sta_list_networks_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int i, ret;
	xr_wifi_sta_list_t *list;
	xr_wifi_sta_list_nod_t *node;
	ptc_cmdpl_t *command;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	if (!sta_networklist) {
		cmd_sta_list_init();
	}

	PTC_CMD_DBG("networks sys_list_num =%d, list_num = %d\r\n",
		sta_networklist->sys_list_num, sta_networklist->list_num);

	for (i = 0; i < sta_networklist->sys_list_num; i++) {
		node = (xr_wifi_sta_list_nod_t*)(sta_networklist->list_nod +
				sizeof(xr_wifi_sta_list_nod_t) * i);

		PTC_CMD_DBG("node id=%d\r\n", node->id);
		PTC_CMD_DBG("node ssid %s\r\n", node->ssid);
		PTC_CMD_DBG("node bssid %s\r\n", node->bssid);
	}

	command = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_NETWORKS, NULL,
		sizeof(xr_wifi_sta_list_t) + sizeof(xr_wifi_sta_list_nod_t) * sta_networklist->sys_list_num);
	if (!command) {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
		return ret;
	}

	list = (xr_wifi_sta_list_t *)command->param;
	node = (xr_wifi_sta_list_nod_t *)list->list_nod;

	list->list_num = sta_networklist->list_num;
	list->sys_list_num = sta_networklist->sys_list_num;
	for (i = 0; i < list->sys_list_num; i++) {
		memcpy(node+i, sta_networklist->list_nod + sizeof(xr_wifi_sta_list_nod_t) * i,
				sizeof(xr_wifi_sta_list_nod_t));
	}
	ret = ptc_low_cmd_send_cmd(command, sizeof(ptc_cmdpl_t) + command->len, NULL, 0);
	ptc_low_cmd_payload_free(command);

	return ret;
}

/* XR_WIFI_HOST_STA_REMOVE_NETWORKS */
static int cmd_sta_remove_networks_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	char *ssid = (char *)data->param;
	cmd_sta_list_remove(ssid, strlen(ssid));

	return -1;
}

/* XR_WIFI_HOST_SET_STA_IP */
static int cmd_sta_set_ipaddr_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	int ret = 0;
	ip4_addr_t dns;
	xr_wifi_sta_ip_info_t *sta_ip_addr = (xr_wifi_sta_ip_info_t *)data->param;

	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo) {
		sysinfo->sta_use_dhcp = 0;
		IP4_ADDR(&(sysinfo->netif_sta_param.ip_addr), sta_ip_addr->ip_addr[0], sta_ip_addr->ip_addr[1],\
				sta_ip_addr->ip_addr[2], sta_ip_addr->ip_addr[3]);
		IP4_ADDR(&(sysinfo->netif_sta_param.net_mask), sta_ip_addr->netmask[0], sta_ip_addr->netmask[1],\
				sta_ip_addr->netmask[2], sta_ip_addr->netmask[3]);
		IP4_ADDR(&(sysinfo->netif_sta_param.gateway), sta_ip_addr->gw[0], sta_ip_addr->gw[1], sta_ip_addr->gw[2],\
				sta_ip_addr->gw[3]);
		IP4_ADDR(&dns, sta_ip_addr->dns[0], sta_ip_addr->dns[1], sta_ip_addr->dns[2], sta_ip_addr->dns[3]);
		dns_setserver(0, &dns);
		PTC_CMD_INF("address: %s\n", inet_ntoa(sysinfo->netif_sta_param.ip_addr));
		PTC_CMD_INF("gateway: %s\n", inet_ntoa(sysinfo->netif_sta_param.gateway));
		PTC_CMD_INF("netmask: %s\n", inet_ntoa(sysinfo->netif_sta_param.net_mask));
		PTC_CMD_INF("dns: %s\n", inet_ntoa(dns));
	} else {
		ret = -1;
		PTC_CMD_ERR("sysinfo is null\n");
	}

	return ret;
}

int ptc_command_sta_connect_event(uint8_t event)
{
	int ret = XR_OS_FAIL;

	ptc_cmdpl_t* pro = NULL;

	xr_wifi_sta_cn_event_t connect;

	if (event == XR_WIFI_DHCP_SUCCESS) {
		ptc_command_notify_eth_state(XR_ETH0_START);
		ptc_report_ip_config();
	} else if (event == XR_WIFI_DISCONNECTED || event == XR_WIFI_NETWORK_DOWN) {
		ptc_command_notify_eth_state(XR_ETH0_STOP);
	}

	connect.event = event;
	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_STA_CN_EV, &connect, sizeof(xr_wifi_sta_cn_event_t));
	if (pro) {
		ret = ptc_low_cmd_send_event(pro, sizeof(ptc_cmdpl_t) + pro->len);
		ptc_low_cmd_payload_free(pro);
	}

	return ret;
}
#endif

#ifdef CONFIG_WLAN_AP
static int wlan_ap_info_set(uint8_t *ssid, uint8_t ssid_len, uint8_t *psk, uint16_t sec, uint8_t ch, uint8_t sta_num)
{
	if ((ssid == NULL) || (ssid_len == 0) || (ssid_len > WLAN_SSID_MAX_LEN)) {
		PTC_CMD_ERR("invalid ssid (%p, %u)\n", ssid, ssid_len);
		return __LINE__;
	}

	wlan_ap_config_t config;
	memset(&config, 0, sizeof(config));

	/* ssid */
	config.field = WLAN_AP_FIELD_SSID;
	memcpy(config.u.ssid.ssid, ssid, ssid_len);
	config.u.ssid.ssid_len = ssid_len;
	if (wlan_ap_set_config(&config) != 0)
		return __LINE__;

	/* auth_alg: OPEN */
	config.field = WLAN_AP_FIELD_AUTH_ALG;
	config.u.auth_alg = WPA_AUTH_ALG_OPEN;
	if (wlan_ap_set_config(&config) != 0)
		return __LINE__;

	if ((psk == NULL) || (psk[0] == '\0')) {
		/* proto: 0 */
		config.field = WLAN_AP_FIELD_PROTO;
		config.u.proto = 0;
		if (wlan_ap_set_config(&config) != 0)
			return __LINE__;

		/* key_mgmt: NONE */
		config.field = WLAN_AP_FIELD_KEY_MGMT;
		config.u.key_mgmt = WPA_KEY_MGMT_NONE;
		if (wlan_ap_set_config(&config) != 0)
			return __LINE__;
	} else {
		/* psk */
		config.field = WLAN_AP_FIELD_PSK;
		strlcpy((char *)config.u.psk, (char *)psk, sizeof(config.u.psk));
		if (wlan_ap_set_config(&config) != 0)
			return __LINE__;
		/* key_mgmt: PSK */
		config.field = WLAN_AP_FIELD_KEY_MGMT;
		config.u.key_mgmt = WPA_KEY_MGMT_PSK;
		if (wlan_ap_set_config(&config) != 0)
			return __LINE__;

		if (sec == XR_WIFI_SEC_WPA_PSK) {
			/* proto: WPA */
			config.field = WLAN_AP_FIELD_PROTO;
			config.u.proto = WPA_PROTO_WPA;
			if (wlan_ap_set_config(&config) != 0)
				return __LINE__;
			/* wpa_cipher: TKIP */
			config.field = WLAN_AP_FIELD_WPA_PAIRWISE_CIPHER;
			config.u.wpa_cipher = WPA_CIPHER_TKIP;
			if (wlan_ap_set_config(&config) != 0)
				return __LINE__;
		} else if (sec == XR_WIFI_SEC_WPA2_PSK) {
			/* proto: RSN */
			config.field = WLAN_AP_FIELD_PROTO;
			config.u.proto = WPA_PROTO_RSN;
			if (wlan_ap_set_config(&config) != 0)
				return __LINE__;
			/* rsn_cipher: CCMP */
			config.field = WLAN_AP_FIELD_RSN_PAIRWISE_CIPHER;
			config.u.rsn_cipher = WPA_CIPHER_CCMP;
			if (wlan_ap_set_config(&config) != 0)
				return __LINE__;
		} else if (sec == (XR_WIFI_SEC_WPA_PSK | XR_WIFI_SEC_WPA2_PSK)) {
			/* proto: WPA | RSN */
			config.field = WLAN_AP_FIELD_PROTO;
			config.u.proto = WPA_PROTO_WPA | WPA_PROTO_RSN;
			if (wlan_ap_set_config(&config) != 0)
				return __LINE__;
			/* wpa_cipher: TKIP */
			config.field = WLAN_AP_FIELD_WPA_PAIRWISE_CIPHER;
			config.u.wpa_cipher = WPA_CIPHER_TKIP;
			if (wlan_ap_set_config(&config) != 0)
				return __LINE__;
			/* rsn_cipher: CCMP */
			config.field = WLAN_AP_FIELD_RSN_PAIRWISE_CIPHER;
			config.u.rsn_cipher = WPA_CIPHER_CCMP;
			if (wlan_ap_set_config(&config) != 0)
				return __LINE__;
		}
	}
	config.field = WLAN_AP_FIELD_CHANNEL;
	config.u.channel = ch;
	if (wlan_ap_set_config(&config) != 0)
		return __LINE__;

	/* set max_num_sta */
	config.field = WLAN_AP_FIELD_MAX_NUM_STA;
	config.u.max_num_sta = sta_num;
	if (wlan_ap_set_config(&config) != 0)
		return __LINE__;

	return 0;
}

/* XR_WIFI_AP_ENABLE */
static int cmd_ap_enable_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret;
	xr_wifi_ap_state_t state;
	xr_wifi_ap_network_card_info_t network_card_info;
	struct netif *nif = ethernetif_get_netif(WLAN_MODE_HOSTAP);
	xr_wifi_ap_config_t *config = (xr_wifi_ap_config_t *)data->param;
	ptc_cmdpl_t *cmd = NULL;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
#if DHCP_SERVER_FROM_SYSINFO
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo) {
		IP4_ADDR(&(sysinfo->netif_ap_param.ip_addr), config->ip_addr[0], config->ip_addr[1],\
		         config->ip_addr[2], config->ip_addr[3]);
		IP4_ADDR(&(sysinfo->netif_ap_param.net_mask), config->netmask[0], config->netmask[1],\
		         config->netmask[2], config->netmask[3]);
		IP4_ADDR(&(sysinfo->netif_ap_param.gateway), config->gw[0], config->gw[1], config->gw[2],\
		         config->gw[3]);
		memset(&sysinfo->dhcp_server_param, 0, sizeof(struct dhcp_server_config));
		ip4_addr_t start_ip_addr;
		IP4_ADDR(&start_ip_addr, config->ip_addr[0], config->ip_addr[1], config->ip_addr[2], 100);
		sysinfo->dhcp_server_param.start_ip = cmd_strdup(inet_ntoa(start_ip_addr));
		sysinfo->dhcp_server_param.ip_num = config->max_num_sta;
		net_config(nif, 0);
		netifapi_dhcps_stop(nif);
		net_config(nif, 1);
	} else {
		PTC_CMD_WRN("sysinfo is null\n");
	}
#endif
	if (WLAN_MODE_HOSTAP != ethernetif_get_mode(nif)) {
		net_switch_mode(WLAN_MODE_HOSTAP);
	}

	wlan_ap_disable();
	if(config != NULL) {
		PTC_CMD_INF("wlan ap sec 0x%x\n", config->sec);
		switch (config->sec)
		{
			case XR_WIFI_SEC_NONE:
				ret = wlan_ap_info_set((uint8_t *)config->ssid, strlen(config->ssid),
							NULL, config->sec, config->channel, config->max_num_sta);
				if (ret) {
					PTC_CMD_ERR("wlan ap set failed %d\n", ret);
					goto ap_stat;
				}
				break;
			case XR_WIFI_SEC_WPA_PSK:
			case XR_WIFI_SEC_WPA2_PSK:
			case (XR_WIFI_SEC_WPA_PSK | XR_WIFI_SEC_WPA2_PSK):
				ret = wlan_ap_info_set((uint8_t *)config->ssid, strlen(config->ssid),
							(uint8_t *)config->pwd, config->sec, config->channel, config->max_num_sta);
				if (ret) {
					PTC_CMD_ERR("wlan ap set failed %d\n", ret);
					goto ap_stat;
				}
				break;
			case XR_WIFI_SEC_WEP:
			default:
				ret = -1;
				PTC_CMD_ERR("unsupport sec type\n");
				goto ap_stat;
		}
	}

	ret = wlan_ap_enable();
ap_stat:
	if (ret) {
		state = XR_WIFI_AP_OFF;
		cmd = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_AP_STATE, &state, sizeof(xr_wifi_ap_state_t));
		PTC_CMD_ERR("ap enable fail:%d state:%d\n", ret, state);
	} else {
		network_card_info.state = XR_WIFI_AP_ON;
		memcpy(network_card_info.ip_addr, (char *)&nif->ip_addr, IP_MAX_LEN);
		memcpy(network_card_info.netmask, (char *)&nif->netmask, NETMASK_MAX_LEN);
		memcpy(network_card_info.gw, (char *)&nif->gw, GW_MAX_LEN);
		PTC_CMD_DBG("address: %s\n", inet_ntoa(network_card_info.ip_addr));
		PTC_CMD_DBG("gateway: %s\n", inet_ntoa(network_card_info.gw));
		PTC_CMD_DBG("netmask: %s\n", inet_ntoa(network_card_info.netmask));
		cmd = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_AP_STATE, &network_card_info,
		                                 sizeof(xr_wifi_ap_network_card_info_t));

#ifdef CONFIG_STA_SOFTAP_COEXIST
		ptc_command_notify_eth_state(XR_ETH1_START);
#else
		ptc_command_notify_eth_state(XR_ETH0_START);
#endif
	}

	if (cmd) {
		ret = ptc_low_cmd_send_cmd(cmd, sizeof(ptc_cmdpl_t) + cmd->len, NULL, 0);
		if (ret)
			PTC_CMD_ERR("AP state send err:%d\n", ret);
		ptc_low_cmd_payload_free(cmd);
	} else {
		PTC_CMD_ERR("alloc err\n");
	}

	PTC_CMD_DBG("%s,%d end\n", __func__, __LINE__);

	return ret;
}

/* XR_WIFI_AP_DISABLE */
static int cmd_ap_disable_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret;
	xr_wifi_ap_state_t state;
	ptc_cmdpl_t *pro = NULL;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

#if DHCP_SERVER_FROM_SYSINFO
	struct sysinfo *sysinfo = sysinfo_get();
	cmd_free(sysinfo->dhcp_server_param.start_ip);
#endif

	ret = wlan_ap_disable();
	if (ret) {
		state = XR_WIFI_AP_ON;
	} else {
		state = XR_WIFI_AP_OFF;
	}

#ifdef CONFIG_STA_SOFTAP_COEXIST
	ptc_command_notify_eth_state(XR_ETH1_STOP);
#else
	ptc_command_notify_eth_state(XR_ETH0_STOP);
#endif
	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_AP_STATE, &state, sizeof(xr_wifi_ap_state_t));
	if (pro) {
		ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}
	return ret;
}

/* XR_WIFI_AP_GET_CFG */
static int cmd_ap_get_cfg_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int i, ret;
	ptc_cmdpl_t *command;
	xr_wifi_ap_info_t *info;
	xr_wifi_ap_sta_info_t *sta_dev;

	wlan_ap_config_t ap_config = {0};
	wlan_ap_stas_t stations;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	/*get sta info*/
	stations.size = STA_MAX_NUM;
	stations.sta = xrlink_cmd_malloc(sizeof(wlan_ap_sta_t) * STA_MAX_NUM);
	if (!stations.sta) {
		PTC_CMD_ERR("alloc fail.\n");
		return -1;
	}
	wlan_ap_sta_info(&stations);

	/*get cmd payload*/
	command = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_AP_INF, &info,
	                                     sizeof(xr_wifi_ap_info_t) + stations.num * sizeof(xr_wifi_ap_sta_info_t));
	if (!command) {
		xrlink_cmd_free(stations.sta);
		return -1;
	}
	info = (xr_wifi_ap_info_t *)command->param;
	sta_dev = (xr_wifi_ap_sta_info_t *)info->dev_list;

	/*set devlist for address*/
	info->sta_num = stations.num;
	for (i = 0; i < info->sta_num; i++) {
		memcpy(sta_dev[i].addr, stations.sta[i].addr, MAC_MAX_LEN);
	}

	/*get ssid*/
	memset(&ap_config, 0, sizeof(ap_config));
	ap_config.field = WLAN_AP_FIELD_SSID;
	wlan_ap_get_config(&ap_config);
	memcpy(info->ssid, ap_config.u.ssid.ssid, min(SSID_MAX_LEN, ap_config.u.ssid.ssid_len));

	info->sec = 0;
	/*get proto */
	memset(&ap_config, 0, sizeof(ap_config));
	ap_config.field = WLAN_AP_FIELD_PROTO;
	wlan_ap_get_config(&ap_config);
	if (ap_config.u.proto == 0) {
		info->sec = XR_WIFI_SEC_NONE;
	} else {
		if (ap_config.u.proto & WPA_PROTO_WPA)
			info->sec |= XR_WIFI_SEC_WPA_PSK;
		if (ap_config.u.proto & WPA_PROTO_RSN)
			info->sec |= XR_WIFI_SEC_WPA2_PSK;

		/*get pwd*/
		memset(&ap_config, 0, sizeof(ap_config));
		ap_config.field = WLAN_AP_FIELD_PSK;
		wlan_ap_get_config(&ap_config);
		memcpy(info->pwd, ap_config.u.psk, PSK_MAX_LEN);
	}

	/*get channel*/
	memset(&ap_config, 0, sizeof(ap_config));
	ap_config.field = WLAN_AP_FIELD_CHANNEL;
	wlan_ap_get_config(&ap_config);
	info->channel = ap_config.u.channel;

	/*debug info*/
	PTC_CMD_DBG("ap config ssid %s\r\n", info->ssid);
	PTC_CMD_DBG("ap config psk %s\r\n", info->pwd);
	PTC_CMD_DBG("ap config channel %d\r\n", info->channel);
	PTC_CMD_DBG("ap config station num %d\r\n", info->sta_num);
	PTC_CMD_DBG("ap config sec 0x%x\r\n", info->sec);
	for (i = 0; i < info->sta_num; i++) {
		PTC_CMD_DBG("ap config mac:%02X:%02X:%02X:%02X:%02X:%02X\r\n",
				sta_dev[i].addr[0], sta_dev[i].addr[1],
				sta_dev[i].addr[2], sta_dev[i].addr[3],
				sta_dev[i].addr[4], sta_dev[i].addr[5]);
	}
	xrlink_cmd_free(stations.sta);

	ret = ptc_low_cmd_send_cmd(command, sizeof(ptc_cmdpl_t) + command->len, NULL, 0);
	ptc_low_cmd_payload_free(command);

	return ret;
}

int ptc_command_ap_connect_event(uint8_t event)
{
	int ret = XR_OS_FAIL;

	ptc_cmdpl_t* pro = NULL;
	xr_wifi_ap_state_t ap_state;
	ap_state = event;
	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_AP_STATE, &ap_state, sizeof(xr_wifi_ap_state_t));
	if (pro) {
		ret = ptc_low_cmd_send_event(pro, sizeof(ptc_cmdpl_t) + pro->len);
		ptc_low_cmd_payload_free(pro);
	}

	return ret;
}
#endif

#ifdef CONFIG_WLAN_MONITOR
/* XR_WIFI_MONITOR_ENABLE */
static void wlan_monitor_rx_cb_handle(uint8_t *data, uint32_t len, void *info)
{
	struct frame_info *p = info;

	xrlink_send_raw_data(data, len, NULL, XRLINK_RAW_DATA_MONITOR);

	PTC_CMD_DBG("rx data %p, len %u, ", data, len);
	if (p) {
		PTC_CMD_DBG("type 0x%02x, rx ch %d, ap ch %d, rssi %u, alg %d\n",
		            p->type, p->recv_channel, p->ap_channel, p->rssi, p->alg);
	} else {
		PTC_CMD_DBG("info NULL\n");
	}

	return;
}

static int cmd_monitor_enable_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret;
	xr_wifi_monitor_enable_t *ch = (xr_wifi_monitor_enable_t *)data->param;
	xr_wifi_monitor_state_t state;
	ptc_cmdpl_t* pro = NULL;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	ret = net_switch_mode(WLAN_MODE_MONITOR);
	if (ret) {
		state = XR_WIFI_MONITOR_OFF;
	} else {
		state = XR_WIFI_MONITOR_ON;
	}

	ret = wlan_monitor_set_channel(ethernetif_get_netif(WLAN_MODE_MONITOR), ch->channel);
	if (ret) {
		state = XR_WIFI_MONITOR_OFF;
	}

	if (state == XR_WIFI_MONITOR_ON)
		wlan_monitor_set_rx_cb(ethernetif_get_netif(WLAN_MODE_MONITOR), wlan_monitor_rx_cb_handle);
	else
		wlan_monitor_set_rx_cb(ethernetif_get_netif(WLAN_MODE_MONITOR), NULL);

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_MONITOR_STATE, &state, sizeof(xr_wifi_monitor_state_t));
	if (pro) {
		ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}

	return ret;
}

/* XR_WIFI_MONITOR_DISABLE */
static int cmd_monitor_disable_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	int ret;
	xr_wifi_monitor_state_t state;
	ptc_cmdpl_t *pro = NULL;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	ret = xr_wlan_on(WLAN_MODE_STA);
	if (ret) {
		state = XR_WIFI_MONITOR_ON;
		PTC_CMD_ERR("%s,error!\n", __func__);
	} else {
		state = XR_WIFI_MONITOR_OFF;
	}

	wlan_net_ctrl_init();
	wlan_monitor_set_rx_cb(ethernetif_get_netif(WLAN_MODE_MONITOR), NULL);

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_MONITOR_STATE, &state,
	                                 sizeof(xr_wifi_monitor_state_t));
	if (pro) {
		ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}
	return ret;
}
#endif

#ifdef CONFIG_SUPPORT_EXP_LMAC_CMD
extern void xradio_drv_cmd(int argc, const char **arg);
static enum cmd_status cmd_lmac_exec(char *cmd)
{
	int argc;
	char *argv[8];

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc > 0) {
		xradio_drv_cmd(argc, (const char **)argv);
		return CMD_STATUS_ACKED;
	} else {
		return CMD_STATUS_INVALID_ARG;
	}
}
#endif

int ptc_command_request_firmware(struct param_wlan_bin *param, void *data, ptc_cmd_dev_op_t type)
{
	int ret = -1;
	ptc_cmdpl_t* pro = NULL;
	int fw_size = 0;
	struct param_wlan_bin *bin;

	pro = ptc_low_cmd_payload_zalloc(type, NULL, sizeof(struct param_wlan_bin) + param->len);
	if (pro) {
		memcpy(pro->param, param, sizeof(struct param_wlan_bin));
		PTC_CMD_DBG("req fw type:%d, cmd type:%d\n", param->type, pro->type);
		bin = (void *)pro->param;
		PTC_CMD_DBG("req bin type:%d len:%d %d\n", bin->type, bin->len, param->len);

		if (type == XR_WIFI_DEV_REQUEST_FW_REQ) {
			ret = ptc_low_cmd_send_large_data_by_databuf(XR_TYPE_CMD, pro,
			                                             sizeof(ptc_cmdpl_t) + pro->len,
			                                             &fw_size, sizeof(int), 1);
			PTC_CMD_DBG("ret %d fw_size %d\n", ret, fw_size);
			if (fw_size && !ret) {
				pro->type = XR_WIFI_DEV_UPDATE_FW_REQ;
				PTC_CMD_DBG("get fw %d\n", pro->len);
				ret = ptc_low_cmd_send_large_data_by_databuf(XR_TYPE_FW_BIN, pro,
				                                             sizeof(ptc_cmdpl_t) + pro->len,
				                                             pro, sizeof(ptc_cmdpl_t) + pro->len, 1);
				if (!ret) {
					struct cmd_para_fw *cmd_fw = (void *)pro->param;

					memcpy(data, cmd_fw->memdata, cmd_fw->memsize);
					PTC_CMD_DBG("ret %d memsize %d\n", ret, cmd_fw->memsize);
					xrlink_cmd_dump("CFM:", (void *)cmd_fw, 32);
				}
				PTC_CMD_DBG("ret %d fw_size %d\n", ret, fw_size);
			}
		} else {
			PTC_CMD_DBG("get fw %d\n", pro->len);
			ret = ptc_low_cmd_send_large_data_by_databuf(XR_TYPE_FW_BIN, pro,
			                                             sizeof(ptc_cmdpl_t) + pro->len,
			                                             pro, sizeof(ptc_cmdpl_t) + pro->len, 1);
			if (!ret) {
				struct cmd_para_fw *cmd_fw = (void *)pro->param;

				memcpy(data, cmd_fw->memdata, cmd_fw->memsize);
				fw_size = cmd_fw->memsize;
			}
			PTC_CMD_DBG("ret %d fw_size %d\n", ret, fw_size);
		}
		ptc_low_cmd_payload_free(pro);
	} else {
		PTC_CMD_ERR("no mem %d\n", sizeof(struct param_wlan_bin) + param->len);
	}

	return fw_size;
}

int ptc_command_release_firmware(uint8_t type)
{
	ptc_cmdpl_t* pro = NULL;
	uint32_t release_type = type;
	int ret = -1;

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_RELEASE_FW_REQ, &release_type, sizeof(uint32_t));

	if (pro) {
		ret = ptc_low_cmd_send_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, &ret, sizeof(int));
		ptc_low_cmd_payload_free(pro);
		if (ret)
			PTC_CMD_ERR("release_firmware err %d.\n", ret);
		else
			PTC_CMD_DBG("release_firmware ok.\n");
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}

	return ret;
}

int ptc_command_data_rx_pause_event(void)
{
	ptc_cmdpl_t* pro = NULL;
	int ret = -1;

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_RX_PAUSE, NULL, 0);
	if (pro) {
		ret = ptc_low_cmd_send_event(pro, sizeof(ptc_cmdpl_t) + pro->len);
		ptc_low_cmd_payload_free(pro);
	}

	return ret;
}

int ptc_command_data_rx_resume_event(void)
{
	ptc_cmdpl_t* pro = NULL;
	int ret = -1;

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_RX_RESUME, NULL, 0);
	if (pro) {
		ret = ptc_low_cmd_send_event(pro, sizeof(ptc_cmdpl_t) + pro->len);
		ptc_low_cmd_payload_free(pro);
	}

	return ret;
}

#ifdef CONFIG_WLAN_SUPPORT_EXPAND_CMD
void xrlink_cmd_expand_rsp_data_set(uint32_t type, uint8_t *data, uint32_t len)
{
	if (xrlink_cmd_expand_rsp_data) {
		if (len > XRLINK_CMD_EXP_RSP_DATA_SIZE) {
			PTC_CMD_ERR("rsp data len overflower, len:%d max:%d\n", len, XRLINK_CMD_EXP_RSP_DATA_SIZE);
			return;
		}
		memcpy(xrlink_cmd_expand_rsp_data->data, data, len);
		xrlink_cmd_expand_rsp_data->len = len;
		xrlink_cmd_expand_rsp_data->type = type;
	}
}

static int ptc_low_cmd_send_raw_cmd(void *cmd_payload, uint16_t len, void *cfm, uint16_t cfm_len)
{
	int ret = -1;
	struct xradio_hdr *xr_hdr = NULL;

	xr_hdr = container_of(cmd_payload, struct xradio_hdr, payload);
	xr_hdr->type = XR_TYPE_RAW_CMD_DATA;
	xr_hdr->len = len;
	ret = xrlink_rx_com_process(xr_hdr, cfm, cfm_len, 0, 1);

	return ret;
}

static int cmd_expand_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	enum cmd_status ret = CMD_STATUS_UNKNOWN_CMD;
	xr_wifi_ext_cmd_info_t *e_cmd = (xr_wifi_ext_cmd_info_t *)data->param;
	xr_wifi_expand_cmd_data_t *expand_cmd_data = NULL;
	ptc_cmdpl_t* pro = NULL;
	uint32_t status = 0;
	char *str;

	PTC_CMD_DBG("cmd handle %s\r\n", __func__);
	PTC_CMD_SYSLOG("xrlink exp cmd: %s\n", e_cmd->ext_cmd_info);

	expand_cmd_data = (xr_wifi_expand_cmd_data_t *)xrlink_cmd_malloc(sizeof(xr_wifi_expand_cmd_data_t) + XRLINK_CMD_EXP_RSP_DATA_SIZE);
	if (!expand_cmd_data) {
		PTC_CMD_ERR("wlan wifi expand cmd malloc ERR\r\n");
		return -1;
	}
	expand_cmd_data->len = sizeof(status);
	expand_cmd_data->type = XR_WIFI_ID_EXPAND_CMD_STATUS;
	xrlink_cmd_expand_rsp_data = expand_cmd_data;
	str = e_cmd->ext_cmd_info;

#ifdef CONFIG_ARCH_SUN300IW1
#ifdef CONFIG_SUPPORT_EXP_RF_CMD
	if (!strncmp(e_cmd->ext_cmd_info, "rf ", strlen("rf "))) {
		str += strlen("rf ");
		ret = cmd_rf_exec(str);
	} else
#endif
#ifdef CONFIG_SUPPORT_EXP_LMAC_CMD
	if (!strncmp(e_cmd->ext_cmd_info, "lmac ", strlen("lmac "))) {
		str += strlen("lmac ");
		ret = cmd_lmac_exec(str);
	} else
#endif
#endif
#ifdef CONFIG_DRIVERS_XRADIO_CMD
#ifdef CONFIG_WLAN_STA
	if (!strncmp(e_cmd->ext_cmd_info, "sta ", strlen("sta "))) {
		str += strlen("sta ");
		ret = cmd_wlan_sta_exec(str);
	} else
#endif
#ifdef CONFIG_WLAN_AP
	if (!strncmp(e_cmd->ext_cmd_info, "ap ", strlen("ap "))) {
		str += strlen("ap ");
		ret = cmd_wlan_ap_exec(str);
	} else
#endif
#ifdef CONFIG_WLAN_MONITOR
	if (!strncmp(e_cmd->ext_cmd_info, "monitor ", strlen("monitor "))) {
		str += strlen("monitor ");
		ret = cmd_wlan_mon_exec(str);
	} else
#endif
#endif
	if (!strncmp(e_cmd->ext_cmd_info, "wlan ", strlen("wlan "))) {
		str += strlen("wlan ");
		ret = cmd_wlan_exec(str);
	}

	if (ret != CMD_STATUS_OK) {
		status = ret;
		PTC_CMD_WRN("cmd run warn:%d\r\n", ret);
	} else {
		status = 0;
	}

	if (expand_cmd_data->type == XR_WIFI_ID_EXPAND_CMD_STATUS)
		memcpy(expand_cmd_data->data, &status, sizeof(status));

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_EXPEND_CMD, expand_cmd_data,
	                                 sizeof(xr_wifi_expand_cmd_data_t) + expand_cmd_data->len);
	if (pro) {
		ret = ptc_low_cmd_send_raw_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}
	xrlink_cmd_free(expand_cmd_data);
	xrlink_cmd_expand_rsp_data = NULL;

	return ret;
}
#endif

#if (defined CONFIG_WLAN_RAW_PACKET_FEATURE) || (defined CONFIG_WLAN_MONITOR)
void ptc_command_reg_raw_data_recv_cb(pro_raw_recv_cb cb)
{
	raw_recv_cb = cb;
}

/* XR_WIFI_HOST_SEND_RAW */
static int cmd_send_raw_handle(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len)
{
	PTC_CMD_DBG("cmd handle %s\r\n", __func__);

	xr_wifi_raw_data_t *raw_data = (xr_wifi_raw_data_t *)data->param;

	if (raw_recv_cb) {
		raw_recv_cb(raw_data->data, raw_data->len);
	}
	return -1;
}

/* XR_WIFI_HOST_EXPEND_CMD */
int ptc_command_expand_raw_data_upload(uint8_t *data, uint32_t len, uint8_t force_tx)
{
	xr_wifi_expand_cmd_data_t *expand_cmd_data = NULL;
	ptc_cmdpl_t* pro = NULL;
	int ret = -1;

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_EXPEND_CMD, NULL,
	                                 sizeof(xr_wifi_expand_cmd_data_t) + len);
	if (pro) {
		expand_cmd_data = (xr_wifi_expand_cmd_data_t *)pro->param;
		expand_cmd_data->len = len;
		expand_cmd_data->type = XR_WIFI_ID_EXPAND_CMD_RAW_DATA_RCV;
		memcpy(expand_cmd_data->data, data, len);
		ret = ptc_low_cmd_send_large_data_by_databuf(XR_TYPE_RAW_CMD_DATA, pro,
		                                       sizeof(ptc_cmdpl_t) + pro->len, NULL, 0, force_tx);
		ptc_low_cmd_payload_free(pro);
	} else {
		PTC_CMD_ERR("alloc fail.\n");
	}

	return ret;
}

int ptc_command_raw_data_send(uint8_t *data, uint16_t len, uint8_t force_tx, xrlink_raw_type_t type)
{
	xr_wifi_raw_data_t *raw_data = NULL;
	ptc_cmdpl_t* pro = NULL;
	int ret = -1;
	uint16_t ptc_type;

	if (type == XRLINK_RAW_DATA_MONITOR)
		ptc_type = XR_WIFI_DEV_MONITOR_DATA;
	else
		ptc_type = XR_WIFI_DEV_IND_RAW_DATA;

	pro = ptc_low_cmd_payload_zalloc(ptc_type, NULL, sizeof(xr_wifi_raw_data_t) + len);
	if (pro) {
		raw_data = (void *)pro->param;
		memcpy(raw_data->data, data, len);
		raw_data->len = len;
		ret = ptc_low_cmd_send_large_data_by_databuf(XR_TYPE_RAW_CMD_DATA, pro, sizeof(ptc_cmdpl_t) +
		                                             sizeof(xr_wifi_raw_data_t) + len, NULL, 0, force_tx);
		ptc_low_cmd_payload_free(pro);
	} else {
		PTC_CMD_ERR("alloc fail.\n");
	}

	return ret;
}

int ptc_command_send_expand_cmd(void *exp_data, uint16_t len, uint32_t type)
{
	int ret = XR_OS_FAIL;
	ptc_cmdpl_t* pro = NULL;
	xr_wifi_expand_cmd_data_t *expand_cmd_data = NULL;

	expand_cmd_data = (xr_wifi_expand_cmd_data_t *)xrlink_cmd_malloc(sizeof(xr_wifi_expand_cmd_data_t) + len);
	if (!expand_cmd_data) {
		PTC_CMD_ERR("wlan wifi expand cmd malloc ERR\r\n");
		return -1;
	}
	expand_cmd_data->len = len;
	expand_cmd_data->type = type;
	memcpy(expand_cmd_data->data, exp_data, len);

	pro = ptc_low_cmd_payload_zalloc(XR_WIFI_DEV_EXPEND_CMD, expand_cmd_data,
	                                 sizeof(xr_wifi_expand_cmd_data_t) + expand_cmd_data->len);
	if (pro) {
		ret = ptc_low_cmd_send_raw_cmd(pro, sizeof(ptc_cmdpl_t) + pro->len, NULL, 0);
		ptc_low_cmd_payload_free(pro);
	} else {
		ret = -1;
		PTC_CMD_ERR("alloc fail.\n");
	}
	xrlink_cmd_free(expand_cmd_data);

	return ret;
}
#endif

static ptc_cmd_handle_t cmd_table[] = {
	{XR_WIFI_HOST_HAND_WAY,				cmd_handway_handle },
	{XR_WIFI_HOST_WLAN_POWER,			cmd_wlan_power_handle },
	{XR_WIFI_HOST_KEEP_AVLIE,			cmd_keep_alive_handle },
	{XR_WIFI_HOST_BLOCK_RW_REQ,			cmd_data_test_handle },
	{XR_WIFI_HOST_SET_DEVICE_MAC_REQ,	cmd_set_device_mac_handle },
	{XR_WIFI_HOST_SET_EFUSE,			cmd_set_efuse_handle },
	{XR_WIFI_HOST_ON,					cmd_wifi_open_handle },
	{XR_WIFI_HOST_OFF,					cmd_wifi_close_handle },
	{XR_WIFI_HOST_GET_SCAN_RES,			cmd_get_scan_res_handle },
	{XR_WIFI_HOST_SET_MAC,				cmd_set_mac_handle },
	{XR_WIFI_HOST_GET_MAC,				cmd_get_mac_handle },
	{XR_WIFI_HOST_GET_DEVICE_MAC_DRV_REQ,	cmd_get_mac_drv_handle },
	{XR_WIFI_HOST_SET_COUNTRY_CODE,		cmd_set_country_code_handle },
	{XR_WIFI_HOST_GET_COUNTRY_CODE,		cmd_get_country_code_handle },
	{XR_WIFI_HOST_GET_RTOS_VERSION,		cmd_get_rtos_version_handle },
#ifdef CONFIG_WLAN_STA
	{XR_WIFI_HOST_STA_CONNECT,			cmd_sta_connect_handle },
	{XR_WIFI_HOST_STA_DISCONNECT,		cmd_sta_disconnect_handle },
	{XR_WIFI_HOST_STA_GET_INFO,			cmd_sta_get_info_handle },
	{XR_WIFI_HOST_STA_AUTO_RECONNECT,	cmd_sta_auto_reconnect_handle },
	{XR_WIFI_HOST_STA_LIST_NETWORKS, 	cmd_sta_list_networks_handle },
	{XR_WIFI_HOST_STA_REMOVE_NETWORKS,	cmd_sta_remove_networks_handle },
	{XR_WIFI_HOST_SET_STA_IP,			cmd_sta_set_ipaddr_handle },
#endif
#ifdef CONFIG_WLAN_AP
	{XR_WIFI_HOST_AP_ENABLE,			cmd_ap_enable_handle },
	{XR_WIFI_HOST_AP_DISABLE,			cmd_ap_disable_handle },
	{XR_WIFI_HOST_AP_GET_INF,			cmd_ap_get_cfg_handle },
#endif
#ifdef CONFIG_WLAN_MONITOR
	{XR_WIFI_HOST_MONITOR_ENABLE,		cmd_monitor_enable_handle },
	{XR_WIFI_HOST_MONITOR_DISABLE,		cmd_monitor_disable_handle },
#endif
#if (defined CONFIG_WLAN_RAW_PACKET_FEATURE) || (defined CONFIG_WLAN_MONITOR)
	{XR_WIFI_HOST_SEND_RAW,				cmd_send_raw_handle },
#endif
#ifdef CONFIG_WLAN_SUPPORT_EXPAND_CMD
	{XR_WIFI_HOST_EXPEND_CMD,			cmd_expand_handle },
#endif
};

int ptc_command_handler(uint8_t type, uint8_t *pdata, uint16_t datalen)
{
	uint16_t i = 0;
	ptc_cmdpl_t *command = NULL;
	uint8_t cmd_table_len;
	int ret;
	uint8_t *rsp = NULL;
	uint16_t rsp_len = 0;

	cmd_table_len = sizeof(cmd_table)/sizeof(cmd_table[0]);
	command = (ptc_cmdpl_t *)pdata;
	if ((pdata == NULL) || (datalen < PTC_CMD_HEAD_SIZE)) {
		PTC_CMD_ERR("[ptc_command_hander] pdata error\r\n");
		return -1;
	}
	PTC_CMD_DBG("[ptc_command_hander] type = 0x%04x, len = %d, cmd_table_len = %d\n",
			command->type, command->len, cmd_table_len);

	for (i = 0; i < cmd_table_len; i++) {
		if (command->type == cmd_table[i].opcode) {
			if (cmd_table[i].hanler) {
				ret = cmd_table[i].hanler(command, &rsp, &rsp_len);
			}
			break;
		}
	}
	if (i == cmd_table_len) {
		PTC_CMD_ERR("[ptc_command_hander] cmd %d not found\n", command->type);
	}
	PTC_CMD_DBG("[ptc_command_hander] handler ret %d\n", ret);

	if (rsp) {
		ret = ptc_low_cmd_send_cmd_ack(rsp, rsp_len);
		xrlink_cmd_free(rsp);
	} else {
		ret = ptc_low_cmd_send_cmd_ack(&ret, sizeof(int));
	}

	return ret;
}
