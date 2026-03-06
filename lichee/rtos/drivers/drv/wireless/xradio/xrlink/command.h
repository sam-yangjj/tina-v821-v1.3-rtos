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

#ifndef __PROTOCOL_COMMAND_H__
#define __PROTOCOL_COMMAND_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "xrlink_debug.h"

/* opcode[1 Byte] + len[2 Bytes] + Param */
#define PTC_CMD_OPCODE_SIZE				2
#define PTC_CMD_LEN_SIZE				2
#define PTC_CMD_HEAD_SIZE				(PTC_CMD_OPCODE_SIZE + PTC_CMD_LEN_SIZE)

#define SSID_MAX_LEN					32
#define STA_MAX_NUM						32
#define PSK_MAX_LEN						64

#define BSSID_MAX_LEN					6
#define IFNAME_MAX_LEN					5
#define MAC_MAX_LEN						6
#define IP_MAX_LEN						4
#define NETMASK_MAX_LEN					4
#define GW_MAX_LEN						4
#define DNS_MAX_LEN						4

#define STORE_LIST_NUM					3

/* firmware define */
#define WLAN_BIN_TYPE_BL	(0)
#define WLAN_BIN_TYPE_FW	(1)
#define WLAN_BIN_TYPE_SDD	(2)

#define XR_FW_SET_TYPE_OFFSET(op, type, offset) ((op) << 24 | (type) << 20 | (offset))
#define XR_FW_GET_OPERATE(v) ((v) >> 24)
#define XR_FW_GET_TYPE(v) (((v) >> 20) & 0xF)
#define XR_FW_GET_OFFSET(v) ((v) & 0xFFFFF)

struct param_wlan_bin {
    uint32_t type;
    uint32_t offset;
    uint32_t len;
};

struct cmd_para_block_rw {
    uint32_t memaddr;
    uint32_t memsize;
    uint32_t memdata[];
};

struct cmd_fw_receive {
    uint32_t fwsize;
    uint32_t *fwdata;
};

/* protocol cmd paylod in param */
/* XR_WIFI_HOST_HAND_WAY */
struct cmd_para_hand_way {
    uint8_t id;
};

/* XR_WIFI_DEV_HAND_WAY */
struct cmd_para_hand_way_res {
    uint8_t id;
    uint8_t mac[6];
    uint8_t ip_addr[4];
    uint8_t netmask[4];
    uint8_t gw[4];
};

/* XR_WIFI_HOST-DEV_KEEP_AVLIE*/
struct cmd_para_keep_alive {
    uint8_t data;
};

/* XR_WIFI_HOST_DATA_TEST */
struct cmd_para_data_test {
    uint16_t len;
    uint8_t data[0];
};

/* XR_WIFI_DEV_DATA_TEST */
struct cmd_para_data_test_res {
    uint16_t len;
    uint8_t data[0];
};

struct cmd_para_mac_info {
    uint8_t ifname[5];
    uint8_t mac[6];
};

struct cmd_wlan_power_data {
    uint8_t enable;
    uint8_t mode;
    uint8_t runing;
};

/* XR_WIFI_HOST_SET_EFUSE */
struct cmd_para_efuse {
    uint32_t addr;
};

/* XR_WIFI_HOST_ON */
typedef enum {
    XR_WIFI_STATION = 0,
    XR_WIFI_AP,
    XR_WIFI_MONITOR,
    XR_WIFI_AP_STATION,
    XR_WIFI_AP_MONITOR,
    XR_WIFI_MODE_UNKNOWN,
} xr_wifi_mode_t;

/* XR_WIFI_DEV_DEV_STATUS */
typedef enum {
    XR_WLAN_STATUS_DOWN = 0,
    XR_WLAN_STATUS_UP,
} xr_wifi_dev_status_t;

typedef struct {
    uint8_t mode;
} xr_wifi_on_t;

typedef enum {
    XR_WIFI_SEC_NONE = 0b0,
    XR_WIFI_SEC_WEP = 0b1,
    XR_WIFI_SEC_WPA_PSK = 0b10,
    XR_WIFI_SEC_WPA2_PSK = 0b100,
    XR_WIFI_SEC_WPA2_PSK_SHA256 = 0b1000,
    XR_WIFI_SEC_WPA3_PSK = 0b10000,
    XR_WIFI_SEC_EAP = 0b100000,
} xr_wifi_secure_t;

//Op: XR_WIFI_HOST_STA_CONNECT
typedef struct {
	char ssid[SSID_MAX_LEN];
	uint8_t ssid_len;
	char pwd[PSK_MAX_LEN];
	uint8_t pwd_len;
	uint8_t sec;
	uint8_t fast_connect;
	int auth_alg;
	uint8_t bssid[BSSID_MAX_LEN];
} xr_wifi_sta_cn_t;

//Op: XR_WIFI_HOST_AP_ENABLE
typedef struct {
    char ssid[SSID_MAX_LEN + 1];
    char pwd[PSK_MAX_LEN];
    uint8_t sec;
    uint8_t channel;
    uint8_t ip_addr[IP_MAX_LEN];
    uint8_t netmask[NETMASK_MAX_LEN];
    uint8_t gw[GW_MAX_LEN];
    uint8_t max_num_sta;
} xr_wifi_ap_config_t;

//Op: XR_WIFI_HOST_MONITOR_ENABLE
typedef struct {
    uint8_t channel;
} xr_wifi_monitor_enable_t;

//Op: XR_WIFI_HOST_SET/GET_MAC
typedef struct {
    uint8_t ifname[IFNAME_MAX_LEN];
    uint8_t mac[MAC_MAX_LEN];
} xr_wifi_mac_info_t;

//Op: XR_WIFI_DEV_STA_INFO
typedef struct {
    int id;
    int freq;
    int rssi;
    uint8_t bssid[BSSID_MAX_LEN];
    char ssid[SSID_MAX_LEN + 1];
    uint8_t mac_addr[MAC_MAX_LEN];
    uint8_t ip_addr[IP_MAX_LEN];
    uint8_t netmask[NETMASK_MAX_LEN];
    uint8_t gw[GW_MAX_LEN];
    uint8_t sec;
} xr_wifi_sta_info_t;

//Op: XR_WIFI_DEV_AP_STATE,
typedef enum {
    XR_WIFI_AP_ON = 0,
    XR_WIFI_AP_OFF,
    XR_WIFI_AP_STA_DISCONNECTED,
    XR_WIFI_AP_STA_CONNECTED,
    XR_WIFI_AP_UNKNOWN,
} xr_wifi_ap_state_t;

typedef struct {
    xr_wifi_ap_state_t state;
    uint8_t ip_addr[IP_MAX_LEN];
    uint8_t netmask[NETMASK_MAX_LEN];
    uint8_t gw[GW_MAX_LEN];
} xr_wifi_ap_network_card_info_t;

//Op: XR_WIFI_DEV_SCAN_RES
typedef struct {
    uint8_t bssid[BSSID_MAX_LEN];
    uint8_t ssid[SSID_MAX_LEN];
	uint8_t ssid_len;
    uint16_t freq;
    int rssi;
    uint8_t key_mgmt;
} xr_wifi_scan_info_t;

typedef struct {
    uint16_t num;
    uint8_t ap_info[0];
} xr_wifi_scan_result_t;

typedef struct {
	uint8_t addr[MAC_MAX_LEN];
} xr_wifi_ap_sta_info_t;

typedef struct {
	uint8_t enable;
	uint8_t ssid[SSID_MAX_LEN];
	uint8_t ssid_len;
} xr_wifi_scan_param_t;

//Op: XR_WIFI_DEV_AP_INF
typedef struct {
    char ssid[SSID_MAX_LEN + 1];
    char pwd[PSK_MAX_LEN];
    uint8_t sec;
    uint8_t channel;
    uint8_t sta_num;
	uint8_t dev_list[0];
} xr_wifi_ap_info_t;

//Op: XR_WIFI_DEV_MONITOR_STATE,
typedef enum {
    XR_WIFI_MONITOR_ON,
    XR_WIFI_MONITOR_OFF,
} xr_wifi_monitor_state_t;

//Op: XR_WIFI_DEV_MONITOR_DATA,
typedef struct {
    uint32_t len;
    uint8_t data[0];
} xr_wifi_monitor_data_t;

//Op: XR_WIFI_HOST_SEND_RAW,
typedef struct {
    uint32_t len;
    uint8_t *data;
} xr_wifi_send_raw_data_t;

//Op: XR_WIFI_DEV_NETWORKS,
typedef struct {
    int id;
    char ssid[SSID_MAX_LEN];
    char bssid[BSSID_MAX_LEN];
    char flags[16];
} xr_wifi_sta_list_nod_t;

typedef struct {
    int list_num;
    int sys_list_num;
    uint8_t list_nod[0];
} xr_wifi_sta_list_t;

typedef enum{
    WIFI_STA_AUTO_RECONNCECT_DISABLE,
    WIFI_STA_AUTO_RECONNCECT_ENABLE
} wifi_sta_auto_reconnect_t;

//Op: XR_WIFI_DEV_STA_CN_EV,
enum XR_STA_CONNECT_EVET {
    XR_WIFI_DISCONNECTED = 0,
    XR_WIFI_SCAN_STARTED,
    XR_WIFI_SCAN_FAILED,
    XR_WIFI_SCAN_RESULTS,
    XR_WIFI_NETWORK_NOT_FOUND,
    XR_WIFI_NETWORK_DOWN,
    XR_WIFI_PASSWORD_INCORRECT,
    XR_WIFI_AUTHENTIACATION,
    XR_WIFI_AUTH_REJECT,
    XR_WIFI_ASSOCIATING,
    XR_WIFI_ASSOC_REJECT,
    XR_WIFI_ASSOCIATED,
    XR_WIFI_4WAY_HANDSHAKE,
    XR_WIFI_GROUNP_HANDSHAKE,
    XR_WIFI_GROUNP_HANDSHAKE_DONE,
    XR_WIFI_CONNECTED,
    XR_WIFI_CONNECT_TIMEOUT,
    XR_WIFI_DEAUTH,
    XR_WIFI_DHCP_START,
    XR_WIFI_DHCP_TIMEOUT,
    XR_WIFI_DHCP_SUCCESS,
    XR_WIFI_TERMINATING,
    XR_WIFI_UNKNOWN,
};

typedef struct {
    uint8_t event;
} xr_wifi_sta_cn_event_t;

//Op: XR_WIFI_DEV_IND_IP_INFO,
typedef struct {
    uint8_t ip_addr[IP_MAX_LEN];
    uint8_t netmask[NETMASK_MAX_LEN];
    uint8_t gw[GW_MAX_LEN];
	uint8_t dns[GW_MAX_LEN];
} xr_wifi_sta_ip_info_t;

//Op: XR_WIFI_DEV_ETH_STATE
enum XR_ETH_STATE {
    XR_ETH0_STOP = 0,
    XR_ETH0_START,
    XR_ETH1_STOP,
    XR_ETH1_START,
};

typedef struct {
    uint8_t event;
} xr_wifi_dev_eth_state_t;

// XR_WIFI_HOST_SEND_RAW
typedef struct {
    uint32_t len;
    uint8_t data[0];
} xr_wifi_raw_data_t;

#define PRO_HOST_LOW_ID_NUM 50
/* opcode */
typedef enum {
    XR_WIFI_HOST_HAND_WAY = 0,
    XR_WIFI_HOST_WLAN_POWER,
    XR_WIFI_HOST_KEEP_AVLIE,
    XR_WIFI_HOST_BLOCK_RW_REQ,
    XR_WIFI_HOST_SET_DEVICE_MAC_REQ,
    XR_WIFI_HOST_GET_DEVICE_MAC_DRV_REQ,
    XR_WIFI_HOST_SET_EFUSE,
    XR_WIFI_HOST_ON = PRO_HOST_LOW_ID_NUM, //HOST UP LAYER START
    XR_WIFI_HOST_OFF,
    XR_WIFI_HOST_STA_CONNECT,
    XR_WIFI_HOST_STA_DISCONNECT,
    XR_WIFI_HOST_STA_GET_INFO,
    XR_WIFI_HOST_AP_ENABLE,
    XR_WIFI_HOST_AP_DISABLE,
    XR_WIFI_HOST_AP_GET_INF,
    XR_WIFI_HOST_MONITOR_ENABLE,
    XR_WIFI_HOST_MONITOR_DISABLE,
    XR_WIFI_HOST_GET_SCAN_RES,
    XR_WIFI_HOST_SET_MAC,
    XR_WIFI_HOST_GET_MAC,
    XR_WIFI_HOST_SEND_RAW,
    XR_WIFI_HOST_STA_AUTO_RECONNECT,
    XR_WIFI_HOST_STA_LIST_NETWORKS,
    XR_WIFI_HOST_STA_REMOVE_NETWORKS,
    XR_WIFI_HOST_SET_COUNTRY_CODE,
    XR_WIFI_HOST_GET_COUNTRY_CODE,
    XR_WIFI_HOST_EXPEND_CMD,
    XR_WIFI_HOST_SET_STA_IP,
    XR_WIFI_HOST_GET_RTOS_VERSION,
    XR_WIFI_HOST_ID_MAX,
} ptc_cmd_host_op_t;

typedef enum {
    XR_WIFI_DEV_HAND_WAY = 0,
    XR_WIFI_DEV_WLAN_POWER_STATE,
    XR_WIFI_DEV_RX_PAUSE,
    XR_WIFI_DEV_RX_RESUME,
    XR_WIFI_DEV_KEEP_AVLIE,
    XR_WIFI_DEV_BLOCK_RW_REQ,
    XR_WIFI_DEV_REQUEST_FW_REQ,
    XR_WIFI_DEV_UPDATE_FW_REQ,
    XR_WIFI_DEV_RELEASE_FW_REQ,
    XR_WIFI_DEV_SET_DEVICE_MAC_RES,
    XR_WIFI_DEV_ETH_STATE,
    XR_WIFI_DEV_DEV_STATUS = PRO_HOST_LOW_ID_NUM, //HOST UP Layer START
    XR_WIFI_DEV_STA_CN_EV,
    XR_WIFI_DEV_STA_INFO,
    XR_WIFI_DEV_AP_STATE,
    XR_WIFI_DEV_AP_INF,
    XR_WIFI_DEV_SCAN_RES,
    XR_WIFI_DEV_MAC,
    XR_WIFI_DEV_MONITOR_STATE,
    XR_WIFI_DEV_MONITOR_DATA,
    XR_WIFI_DEV_NETWORKS,
    XR_WIFI_DEV_COUNTRY_CODE,
    XR_WIFI_DEV_IND_IP_INFO,
    XR_WIFI_DEV_IND_RAW_DATA,
    XR_WIFI_DEV_EXPEND_CMD,
    XR_WIFI_DEV_RTOS_VERSION,
    XR_WIFI_DEV_ID_MAX,
} ptc_cmd_dev_op_t;

/* event */
typedef enum {
    XR_WIFI_ID_DEV_STATUS,
    XR_WIFI_ID_STA_CN_EV,
    XR_WIFI_ID_STA_STATE_CHANGE,
    XR_WIFI_ID_STA_AP_CN_EVENT,
    XR_WIFI_ID_MONITOR,
    XR_WIFI_ID_ID_MAX,
} ptc_cmd_event_t;

typedef struct {
    uint16_t len;
    char ext_cmd_info[0];
} xr_wifi_ext_cmd_info_t;

typedef struct {
    uint32_t type;
    uint32_t len;
    uint8_t data[0];
} xr_wifi_expand_cmd_data_t;

struct cmd_para_fw {
	uint32_t fw_type;
	uint32_t fw_len;
	uint32_t memsize;
	uint32_t memdata[];
};

typedef enum {
	XRLINK_RAW_DATA_MONITOR,
	XRLINK_RAW_DATA_STA_AP,
	XRLINK_RAW_DATA_STA_AP_EXP,
} xrlink_raw_type_t;

/* event */
typedef enum {
    XR_WIFI_ID_EXPAND_CMD_STATUS,//cmd status
    XR_WIFI_ID_EXPAND_CMD_RAW_DATA_RCV,//raw data
    XR_WIFI_ID_EXPAND_CMD_PRINTF,// printf rx data
    XR_WIFI_ID_EXPAND_CMD_MAX,
} ptc_expand_cmd_event_t;

/* protocol cmd payload */
typedef struct {
	uint16_t type;
	uint16_t len; /* param len */
	uint32_t param[0];
} ptc_cmdpl_t;

typedef struct ptc_cmd_handle{
	ptc_cmd_host_op_t opcode;
	int (*hanler)(ptc_cmdpl_t *data, uint8_t **rsp, uint16_t *rsp_len);
} ptc_cmd_handle_t;

int ptc_command_handler(uint8_t type, uint8_t *pdata, uint16_t datalen);

typedef void(*pro_raw_recv_cb)(uint8_t *data, uint32_t len);

int ptc_command_data_rx_pause_event(void);

int ptc_command_data_rx_resume_event(void);
#ifdef CONFIG_WLAN_STA
int ptc_command_sta_connect_event(uint8_t event);
#endif
#ifdef CONFIG_WLAN_AP
int ptc_command_ap_connect_event(uint8_t event);
#endif
int ptc_command_request_firmware(struct param_wlan_bin *param, void *data, ptc_cmd_dev_op_t type);

int ptc_command_release_firmware(uint8_t type);

void set_wlan_scan_state(int on);

int get_wlan_scan_state(void);

#ifdef CONFIG_WLAN_SUPPORT_EXPAND_CMD
void xrlink_cmd_expand_rsp_data_set(uint32_t type, uint8_t *data, uint32_t len);
#endif
#if (defined CONFIG_WLAN_RAW_PACKET_FEATURE) || (defined CONFIG_WLAN_MONITOR)
void ptc_command_reg_raw_data_recv_cb(pro_raw_recv_cb cb);
int ptc_command_raw_data_send(uint8_t *data, uint16_t len, uint8_t force_tx, xrlink_raw_type_t type);
int ptc_command_expand_raw_data_upload(uint8_t *data, uint32_t len, uint8_t force_tx);
int ptc_command_send_expand_cmd(void *exp_data, uint16_t len, uint32_t type);
#endif
#ifdef __cplusplus
}
#endif

#endif
