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

#include "prj_config.h"

#include <string.h>
#include <stdlib.h>
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"
#include "net/wlan/wlan.h"
#include "net/wlan/wlan_ext_req.h"

#if PRJCONF_NET_DHCPS_FROM_LWIP
#include "lwip/dhcps.h"
#endif

#include "sys_ctrl/sys_ctrl.h"
#include "sysinfo/sysinfo.h"
#include "net_ctrl.h"
#include "net_ctrl_debug.h"
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
#include "pm_wakelock.h"
#endif

#ifdef CONFIG_BT_CLOSE_WLAN_FREQ_AUTO_ALIGN
#include "drv/xr_coex/xr_coex.h"
#endif

struct netif_conf {
	uint8_t     bring_up;   // bring up or down
	uint8_t     use_dhcp;   // use DHCP or not
#ifdef CONFIG_LWIP_V1
	ip_addr_t   ipaddr;
	ip_addr_t   netmask;
	ip_addr_t   gw;
#elif LWIP_IPV4 /* now only for IPv4 */
	ip4_addr_t  ipaddr;
	ip4_addr_t  netmask;
	ip4_addr_t  gw;
#else
	#error "IPv4 not support!"
#endif
};

#if (defined(CONFIG_LWIP_V1) || LWIP_IPV4)
#ifdef CONFIG_WLAN_STA_AUTO_PS_MODE
#if defined(CONFIG_ARCH_SUN300IW1) && defined(CONFIG_DRIVERS_EFUSE)
#include "efuse/platform_efuse.h"
#include "hal_efuse.h"

static int netif_sta_is_enable_ps(void)
{
	uint8_t buffer = 1;

	if (!hal_efuse_get_pmc_enable(&buffer) && (buffer == PMC_ENABLE_BIT_PWR_AON)) {
		NET_INF("STA Active mode\n");
		return 0;
	}
	NET_INF("STA PS mode\n");
	return 1;
}
#else
static __inline int netif_sta_is_enable_ps(void)
{
	return 1;
}
#endif
#endif

static void netif_up_handler(struct netif *nif)
{
	if (nif == NULL) {
		NET_DBG("netif is NULL\n");
		return;
	}
#ifdef CONFIG_WLAN_AP
#if DHCP_SERVER_FROM_SYSINFO
	struct sysinfo *sysinfo = sysinfo_get();
#endif
#endif
	enum wlan_mode mode = wlan_if_get_mode(nif);
	switch (mode) {
#ifdef CONFIG_WLAN_STA
	case WLAN_MODE_STA:
#ifdef CONFIG_WLAN_STA_AUTO_PS_MODE
		if (netif_sta_is_enable_ps())
			wlan_set_ps_mode(nif, 1);
		else
			wlan_set_ps_mode(nif, 0);
#elif defined(CONFIG_WLAN_STA_FORCE_ACTIVE_MODE)
		NET_INF("STA Active mode\n");
		wlan_set_ps_mode(nif, 0);
#else
		wlan_set_ps_mode(nif, 1);
#endif
#ifdef CONFIG_ARCH_SUN300IW1
		wlan_set_router_addr(nif->gw.addr);
#endif
  #if LWIP_AUTOIP
		wlan_set_ip_addr(nif, NULL, 0);
  #else
		uint32_t addr = NET_IP_ADDR_GET_IP4U32(&nif->ip_addr);
		wlan_set_ip_addr(nif, (uint8_t *)&addr, sizeof(addr));
  #endif
		break;
#endif
#ifdef CONFIG_WLAN_AP
	case WLAN_MODE_HOSTAP:
#if PRJCONF_NET_DHCPS_FROM_LWIP
#if DHCP_SERVER_FROM_SYSINFO
		if (sysinfo == NULL) {
			NET_ERR("sysinfo is NULL\n");
		} else {
			dhcps_config(&sysinfo->dhcp_server_param);
		}
#endif
		netifapi_dhcps_start(nif);
		NET_DBG("dhcp server start success\n");
#else
		NET_WRN("dhcps_start not support\n");
#endif
		break;
#endif
#ifdef CONFIG_WLAN_MONITOR
	case WLAN_MODE_MONITOR:
		break;
#endif
	default:
		NET_ERR("Invalid wlan mode %d\n", mode);
		break;
	}
}

static void netif_down_handler(struct netif *nif)
{
#if (!LWIP_AUTOIP)
	if (nif == NULL) {
		NET_DBG("netif is NULL\n");
		return;
	}

	enum wlan_mode mode = wlan_if_get_mode(nif);
	if (mode == WLAN_MODE_STA) {
		wlan_set_ip_addr(nif, NULL, 0);
	}
#endif
}

#else /* (defined(CONFIG_LWIP_V1) || LWIP_IPV4) */

#define netif_up_handler(nif)   do { } while (0)
#define netif_down_handler(nif) do { } while (0)

#endif /* (defined(CONFIG_LWIP_V1) || LWIP_IPV4) */

#if LWIP_NETIF_LINK_CALLBACK
static void netif_link_callback(struct netif *netif)
{
	if (netif_is_link_up(netif)) {
		NET_INF("netif is link up\n");
	} else {
		NET_INF("netif is link down\n");
	}
}
#endif /* LWIP_NETIF_LINK_CALLBACK */

#ifdef CONFIG_LWIP_V1

#if LWIP_NETIF_STATUS_CALLBACK
static void netif_status_callback(struct netif *netif)
{
	if (NET_IS_IP4_VALID(netif)) {
		NET_INF("netif is up\n");
		NET_INF("address: %s\n", inet_ntoa(netif->ip_addr));
		NET_INF("gateway: %s\n", inet_ntoa(netif->gw));
		NET_INF("netmask: %s\n", inet_ntoa(netif->netmask));
#ifdef CONFIG_STA_SOFTAP_COEXIST
		if (wlan_if_get_mode(netif) == WLAN_MODE_HOSTAP)
			net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_UP, WLAN_MODE_HOSTAP);
		else
			net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_UP, WLAN_MODE_STA);
#else
		net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_UP, 0);
#endif
	} else {
		NET_INF("netif is down\n");
		netif_down_handler(netif);
#ifdef CONFIG_STA_SOFTAP_COEXIST
		if (wlan_if_get_mode(netif) == WLAN_MODE_HOSTAP)
			net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_DOWN, WLAN_MODE_HOSTAP);
		else
			net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_DOWN, WLAN_MODE_STA);
#else
		net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_DOWN, 0);
#endif
	}
}
#endif /* LWIP_NETIF_STATUS_CALLBACK */

#else /* CONFIG_LWIP_V1 */

#if LWIP_NETIF_STATUS_CALLBACK
static struct in_addr m_ipv4_addr;
#if LWIP_IPV6
static uint8_t m_ipv6_addr_state;
#if (LWIP_IPV6_NUM_ADDRESSES > 8)
#error "MUST enlarge sizeof(m_ipv6_addr_state)!"
#endif
#endif

static void netif_status_callback(struct netif *netif)
{
	struct in_addr ipv4_addr;

	/* netif is always up, check IPv4 addr status */
	inet_addr_from_ip4addr(&ipv4_addr, ip_2_ip4(&netif->ip_addr));
	if (m_ipv4_addr.s_addr != ipv4_addr.s_addr) {
		/* only send message when IP changed, different with lwip-1.4.1 */
		m_ipv4_addr.s_addr = ipv4_addr.s_addr;
		if (NET_IS_IP4_VALID(netif)) {
			NET_INF("netif (IPv4) is up\n");
			NET_INF("address: %s\n", inet_ntoa(netif->ip_addr));
			NET_INF("gateway: %s\n", inet_ntoa(netif->gw));
			NET_INF("netmask: %s\n", inet_ntoa(netif->netmask));
#ifdef CONFIG_STA_SOFTAP_COEXIST
			if (wlan_if_get_mode(netif) == WLAN_MODE_HOSTAP)
				net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_UP, WLAN_MODE_HOSTAP);
			else
				net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_UP, WLAN_MODE_STA);
#else
			net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_UP, 0);
#endif
		} else {
			NET_INF("netif (IPv4) is down\n");
			netif_down_handler(netif);
#ifdef CONFIG_STA_SOFTAP_COEXIST
			if (wlan_if_get_mode(netif) == WLAN_MODE_HOSTAP)
				net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_DOWN, WLAN_MODE_HOSTAP);
			else
				net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_DOWN, WLAN_MODE_STA);
#else
			net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_DOWN, 0);
#endif
		}
	}

#if LWIP_IPV6
	int i;
	uint8_t ipv6_addr_state = 0;

	for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
		if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
			ipv6_addr_state |= (1 << i);
		}
	}

	if (m_ipv6_addr_state != ipv6_addr_state) {
		NET_INF("IPv6 addr state change: 0x%x --> 0x%x\n",
		        m_ipv6_addr_state, ipv6_addr_state);
		m_ipv6_addr_state = ipv6_addr_state;
		net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_IPV6_STATE, ipv6_addr_state);
	}
#endif /* LWIP_IPV6 */
}
#endif /* LWIP_NETIF_STATUS_CALLBACK */

#endif /* CONFIG_LWIP_V1 */

#if LWIP_NETIF_REMOVE_CALLBACK
static void netif_remove_callback(struct netif *netif)
{
	NET_DBG("%s()\n", __func__);
}
#endif /* LWIP_NETIF_REMOVE_CALLBACK */

#ifdef CONFIG_LWIP_V1
#define NET_DHCP_DATA(nif)      (nif->dhcp)
#define NET_DHCP_STATE(nif)     (nif->dhcp->state)
#define NET_DHCP_STATE_OFF      DHCP_OFF
#define NET_DHCP_STATE_BOUND    DHCP_BOUND
//#define NET_IP4_ADDR_ANY        IP_ADDR_ANY
#elif LWIP_IPV4 /* now only for IPv4 */
#include "lwip/prot/dhcp.h"
#define NET_DHCP_DATA(nif)      netif_dhcp_data(nif)
#define NET_DHCP_STATE(nif)     (netif_dhcp_data(nif)->state)
#define NET_DHCP_STATE_OFF      DHCP_STATE_OFF
#define NET_DHCP_STATE_BOUND    DHCP_STATE_BOUND
//#define NET_IP4_ADDR_ANY        IP4_ADDR_ANY4
#else
#error "IPv4 not support!"
#endif

static void netif_config(struct netif *nif, struct netif_conf *conf)
{
	if (conf->bring_up) {
		if (NET_IS_IP4_VALID(nif)) {
			NET_INF("netif is already up\n");
#ifdef CONFIG_LWIP_V1
	#if LWIP_NETIF_STATUS_CALLBACK
			if (!conf->use_dhcp) {
				netif_status_callback(nif);
			}
	#endif
#else /* CONFIG_LWIP_V1 */
			/* Maybe no new NET_CTRL_MSG_NETWORK_UP message due to the IP
			 * address is the same as before, do netif_up_handler() first.
			 */
			netif_up_handler(nif);
#endif /* CONFIG_LWIP_V1 */
			return;
		}
		if (conf->use_dhcp) {
			if (NET_DHCP_DATA(nif) &&
			    NET_DHCP_STATE(nif) != NET_DHCP_STATE_OFF) {
				NET_INF("DHCP is already started\n");
				return;
			}

			NET_INF("start DHCP...\n");
			if (netifapi_dhcp_start(nif) != ERR_OK) {
				NET_ERR("DHCP start failed!\n");
				return;
			}
		} else {
			NET_INF("bring up netif\n");
			netifapi_netif_set_addr(nif, &conf->ipaddr, &conf->netmask, &conf->gw);
#ifdef CONFIG_LWIP_V1
			netifapi_netif_set_up(nif);
#endif
//			netif_up_handler(nif);
		}
	} else {
		if (conf->use_dhcp) {
			if (NET_DHCP_DATA(nif) == NULL) {
				NET_INF("DHCP is not started\n");
				return;
			}
			if (netif_is_link_up(nif)) {
				NET_INF("release DHCP\n");
				netifapi_dhcp_release(nif);
			} else {
				NET_INF("bring down netif\n");
#ifdef CONFIG_LWIP_V1
				netifapi_netif_set_down(nif);
#endif
				netifapi_netif_set_addr(nif, NULL, NULL, NULL);
			}

			NET_INF("stop DHCP\n");
			netifapi_dhcp_stop(nif);
		} else {
			if (!NET_IS_IP4_VALID(nif)) {
				NET_INF("netif is already down\n");
				return;
			}
			NET_INF("bring down netif\n");
#ifdef CONFIG_LWIP_V1
			netifapi_netif_set_down(nif);
#endif
			netifapi_netif_set_addr(nif, NULL, NULL, NULL);
		}
	}
}

/* bring up/down network */
void net_config(struct netif *nif, uint8_t bring_up)
{
	struct sysinfo *sysinfo = sysinfo_get();
#ifdef CONFIG_STA_SOFTAP_COEXIST
	int mode = ethernetif_get_mode(nif);
#endif
	if (sysinfo == NULL) {
		NET_ERR("failed to get sysinfo %p\n", sysinfo);
		return;
	}

	struct netif_conf net_conf;
	memset(&net_conf, 0, sizeof(struct netif_conf));
	net_conf.bring_up = bring_up;

#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (mode == WLAN_MODE_STA) {
#else
	if (sysinfo->wlan_mode == WLAN_MODE_STA) {
#endif
		if (sysinfo->sta_use_dhcp) {
			net_conf.use_dhcp = 1;
		} else {
			net_conf.use_dhcp = 0;
			memcpy(&net_conf.ipaddr, &sysinfo->netif_sta_param.ip_addr, sizeof(net_conf.ipaddr));
			memcpy(&net_conf.netmask, &sysinfo->netif_sta_param.net_mask, sizeof(net_conf.netmask));
			memcpy(&net_conf.gw, &sysinfo->netif_sta_param.gateway, sizeof(net_conf.gw));
		}
#ifdef CONFIG_STA_SOFTAP_COEXIST
	} else if (mode == WLAN_MODE_HOSTAP) {
#else
	} else if (sysinfo->wlan_mode == WLAN_MODE_HOSTAP) {
#endif
		net_conf.use_dhcp = 0;
		memcpy(&net_conf.ipaddr, &sysinfo->netif_ap_param.ip_addr, sizeof(net_conf.ipaddr));
		memcpy(&net_conf.netmask, &sysinfo->netif_ap_param.net_mask, sizeof(net_conf.netmask));
		memcpy(&net_conf.gw, &sysinfo->netif_ap_param.gateway, sizeof(net_conf.gw));
	} else {
#ifdef CONFIG_STA_SOFTAP_COEXIST
		NET_ERR("invalid wlan mode %d\n", mode);
#else
		NET_ERR("invalid wlan mode %d\n", sysinfo->wlan_mode);
#endif
		return;
	}

	netif_config(nif, &net_conf);
}

#ifdef CONFIG_STA_SOFTAP_COEXIST
void net_check_sta_channel(void)
{
	if ((g_wlan_netif != NULL) &&
		(netif_is_link_up(g_wlan_netif))) {
		wlan_ap_default_conf_t *conf;
		NET_WRN("set soft AP default channel to STA channel\n");
		wlan_sta_ap_t *ap = malloc(sizeof(wlan_sta_ap_t));
		if (ap == NULL) {
			NET_WRN("%s(), malloc failed\n", __func__);
			return;
		}
		wlan_sta_ap_info(ap);
		conf = (wlan_ap_default_conf_t *)wlan_ap_get_default_conf();
		conf->channel = ap->channel;
		free(ap);
	}
}
#endif

struct netif *net_open(enum wlan_mode mode)
{
	struct netif *nif;

	NET_DBG("%s(), mode %d\n", __func__, mode);

	nif = wlan_netif_create(mode);
	if (nif == NULL) {
		return NULL;
	}

#if LWIP_NETIF_LINK_CALLBACK
	netif_set_link_callback(nif, netif_link_callback);
#endif
#if LWIP_NETIF_STATUS_CALLBACK
	netif_set_status_callback(nif, netif_status_callback);
#endif
#if LWIP_NETIF_REMOVE_CALLBACK
	netif_set_remove_callback(nif, netif_remove_callback);
#endif

#ifndef CONFIG_LWIP_V1
#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (mode == WLAN_MODE_STA) {
#endif
		m_ipv4_addr.s_addr = 0;
#if LWIP_IPV6
		m_ipv6_addr_state = 0;
	/* enable IPv6 for station only */
#ifndef CONFIG_STA_SOFTAP_COEXIST
	if (mode == WLAN_MODE_STA) {
#endif
		netif_create_ip6_linklocal_address(nif, 1);
#if LWIP_IPV6_AUTOCONFIG
		netif_set_ip6_autoconfig_enabled(nif, 1);
#endif
	}
#endif /* LWIP_IPV6 */
#ifdef CONFIG_STA_SOFTAP_COEXIST
	}
#endif
	/* set netif up, but no valid ip address, required by lwip-2.x.x */
	netifapi_netif_set_up(nif);
#endif /* CONFIG_LWIP_V1 */

#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (mode == WLAN_MODE_HOSTAP) {
		net_check_sta_channel();
	}
#endif

	wlan_start(nif);

#ifndef CONFIG_STA_SOFTAP_COEXIST
	/* need ensure */
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo) {
		sysinfo->wlan_mode = mode;
	}
#endif
#if PRJCONF_TEMP_MONITOR_EN
	temp_monitor_init();
#endif

	return nif;
}

void net_close(struct netif *nif)
{
	if (!nif) {
		NET_ERR("%s() has closed!\n", __func__);
		return ;
	}

	NET_DBG("%s(), mode %d\n", __func__, wlan_if_get_mode(nif));

#ifdef CONFIG_WLAN_AP
	if (WLAN_MODE_HOSTAP == wlan_if_get_mode(nif)) {
#if PRJCONF_NET_DHCPS_FROM_LWIP
		netifapi_dhcps_stop(nif);
		NET_DBG("stop dhcp server\n");
#endif
	}
#endif
	wlan_stop(nif);
#if 0
#if LWIP_NETIF_REMOVE_CALLBACK
	netif_set_remove_callback(nif, NULL);
#endif
#if LWIP_NETIF_STATUS_CALLBACK
	netif_set_status_callback(nif, NULL);
#endif
#if LWIP_NETIF_LINK_CALLBACK
	netif_set_link_callback(nif, NULL);
#endif
#endif
	wlan_netif_delete(nif);
}

int net_switch_mode(enum wlan_mode mode)
{
	int ret = -1;
	enum wlan_mode cur_mode;
	struct netif *nif = NULL;

	if (net_sys_status_get() != NET_SYS_STATUS_START) {
		NET_WRN("net system has stopped!\n");
		return -1;
	}

	if (mode >= WLAN_MODE_NUM) {
		NET_WRN("invalid wlan mode %d\n", mode);
		return ret;
	}

#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (mode == WLAN_MODE_HOSTAP)
		nif = g_wlan_ap_netif;
	else
		nif = g_wlan_netif;

	if (nif == NULL) {
		return net_sys_start(mode); /* start net sys only */
	}
#else
	nif = g_wlan_netif;
	if (nif == NULL) {
		g_wlan_netif = net_open(mode);
		return (g_wlan_netif ? 0 : -1);
	}
#endif

	NET_DBG("%s(), mode %d --> %d\n", __func__, wlan_if_get_mode(nif), mode);

	cur_mode = wlan_if_get_mode(nif);
	if (mode == cur_mode) {
		NET_INF("no need to switch wlan mode %d\n", cur_mode);
		return 0;
	}

	if (NET_IS_IP4_VALID(nif)) {
		net_config(nif, 0); /* bring down netif */
	}

	if (cur_mode == WLAN_MODE_STA && netif_is_link_up(nif)) {
		wlan_sta_disable(); /* disconnect wlan */
	}

	switch (mode) {
	case WLAN_MODE_HOSTAP:
#ifdef CONFIG_WLAN_AP
#ifdef CONFIG_STA_SOFTAP_COEXIST
		net_close(nif);
		nif = net_open(mode);
		g_wlan_ap_netif = nif;
		if (nif) {
			ret = 0;
		}
		break;
#endif
#endif
	case WLAN_MODE_STA:
	case WLAN_MODE_MONITOR:
		net_close(nif);
		nif = net_open(mode);
		g_wlan_netif = nif;
		if (nif) {
			ret = 0;
		}
		break;
	default:
		break;
	}

	return ret;
}

void net_ctrl_msg_send(uint32_t type, uint32_t data)
{
	sys_event_send(CTRL_MSG_TYPE_NETWORK, (uint16_t)type, data, (uint32_t)XR_OS_WAIT_FOREVER);
}

int net_ctrl_msg_send_with_free(uint16_t type, void *data)
{
	return sys_event_send_with_free(CTRL_MSG_TYPE_NETWORK, type, (void *)data, (uint32_t)XR_OS_WAIT_FOREVER);
}

#if NET_INF_ON
static const char * const net_ctrl_msg_str[] = {
	"wlan connected",
	"wlan disconnected",
	"wlan scan success",
	"wlan scan failed",
	"wlan 4way handshake failed",
	"wlan ssid not found",
	"wlan auth timeout",
	"wlan associate timeout",
	"wlan connect failed",
	"wlan connect loss",
	"wlan associate failed",
	"wlan SAE auth-commit failed",
	"wlan SAE auth-confirm failed",
	"wlan p2p wake up",
	"wlan p2p keepalive connect lost",
	"wlan p2p ip wake up",
#ifdef CONFIG_STA_SOFTAP_COEXIST
	"wlan ap start",
	"wlan ap stop",
	"wlan select bss",
#endif
	"wlan dev hang",
	"wlan ap-sta connected",
	"wlan ap-sta disconnected",
	"network up",
	"network down",
#if (!defined(CONFIG_LWIP_V1) && LWIP_IPV6)
	"network IPv6 state",
#endif
};
#endif

int net_ctrl_connect_ap(void)
{
	return 0;
}

int net_ctrl_disconnect_ap(void)
{
	return 0;
}

#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
struct wakelock pm_connect_lock = {
	.name = "xradio_connect",
	.type = PM_WL_TYPE_WAIT_TIMEOUT,
};

#define STADNBY_RECONNECT_AP_HOLD_WAKELOCK_TIME     20000
#endif
static uint8_t g_record_bssid[6];
static __inline void net_ctrl_record_bssid(uint8_t *record, uint8_t *bssid)
{
	for (int i = 0; i < 6; i++)
		record[i] = bssid[i];
}

__weak void net_ctrl_msg_process(uint32_t event, uint32_t data, void *arg)
{
	int need_config = 0;
	struct netif *nif = g_wlan_netif;
	uint16_t type = EVENT_SUBTYPE(event);
	int ret = -1;

#ifdef CONFIG_STA_SOFTAP_COEXIST
	struct netif *wlan_netif = NULL;
	if (type == NET_CTRL_MSG_WLAN_CONNECTED ||
		type == NET_CTRL_MSG_WLAN_DISCONNECTED ||
		type == NET_CTRL_MSG_WLAN_SCAN_SUCCESS ||
		type == NET_CTRL_MSG_WLAN_SCAN_FAILED ||
		type == NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED ||
		type == NET_CTRL_MSG_WLAN_CONNECT_FAILED ||
		type == NET_CTRL_MSG_WLAN_CONNECTION_LOSS ||
		type == NET_CTRL_MSG_WLAN_SELECT_BSS) {
		wlan_netif = g_wlan_netif;
	} else if (type == NET_CTRL_MSG_WLAN_AP_START ||
		type == NET_CTRL_MSG_WLAN_AP_STOP) {
		wlan_netif = g_wlan_ap_netif;
	} else if (type == NET_CTRL_MSG_NETWORK_UP ||
		type == NET_CTRL_MSG_NETWORK_DOWN) {
		if (data == WLAN_MODE_HOSTAP)
			wlan_netif = g_wlan_ap_netif;
		else
			wlan_netif = g_wlan_netif;
	} else {
		NET_INF("msg <%s>\n", net_ctrl_msg_str[type]);
		return;
	}
#endif

	NET_INF("msg <%s>\n", net_ctrl_msg_str[type]);

	wlan_ext_stats_code_get_t param;
	NET_DBG("func : %s event : %d \n", __func__, type);

	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
#ifdef CONFIG_STA_SOFTAP_COEXIST
		nif = wlan_netif;
#endif
		if (nif && !netif_is_link_up(nif)) {
			netifapi_netif_set_link_up(nif); /* set link up */
			need_config = 1;
		}
#if (defined(CONFIG_LWIP_V1) || LWIP_IPV4)
		if ((nif && !NET_IS_IP4_VALID(nif)) || need_config) {
			/* if the ap router is changed, restart dhcp */
			if (wlan_if_get_mode(nif) == WLAN_MODE_STA) {
				wlan_sta_ap_t ap_info;
				wlan_sta_ap_info(&ap_info);
				if (memcmp(g_record_bssid, ap_info.bssid, 6)) {
					net_ctrl_record_bssid(g_record_bssid, ap_info.bssid);
					if (NET_DHCP_DATA(nif)) {
						NET_INF("ap router changed\n");
						net_config(nif, 0);
					}
				}
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
				else if (NET_IS_IP4_VALID(nif)) {
					pm_wakelocks_release(&pm_connect_lock);
					wlan_ext_request(nif, WLAN_EXT_CMD_SET_STAY_AWAKE_TMO, 0);
				}
#endif
			}
			net_config(nif, 1); /* bring up network */
		}
#endif
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA) {
#ifdef CONFIG_WLAN_STA_FREQ_AUTO_ALIGN
#ifdef CONFIG_BT_CLOSE_WLAN_FREQ_AUTO_ALIGN
			ret = xr_coex_set_freq_auto_align(1, 1, 0);
#else
			wlan_ext_freq_auto_align_cfg_t cfg = {0};

			cfg.mode = FFA_EXT_MODE_OPEN_USE_DEFAULT;
			ret = wlan_ext_request(nif, WLAN_EXT_CMD_SET_FREQ_AUTO_ALIGN, (uint32_t)(uintptr_t)(&cfg));
#endif
			if (ret)
				NET_WRN("Open freq auto align fail:%d\n", ret);
#endif
			wlan_set_scan_additional_time(0);
		}
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
#ifdef CONFIG_STA_SOFTAP_COEXIST
		nif = wlan_netif;
#endif
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
		if (pm_state_get() != PM_STATUS_RUNNING)
			pm_wakelocks_acquire(&pm_connect_lock, PM_WL_TYPE_WAIT_TIMEOUT, STADNBY_RECONNECT_AP_HOLD_WAKELOCK_TIME);
#endif
		ret = wlan_ext_request(nif, WLAN_EXT_CMD_GET_STATS_CODE, (uint32_t)(uintptr_t)(&param));
		if (!ret) {
			NET_DBG("reason code:%d status code:%d\n", param.reason_code, param.status_code);
		}
		if (nif) {
			if (netif_is_link_up(nif)) {
				netifapi_netif_set_link_down(nif); /* set link down */
			}
#if (defined(CONFIG_LWIP_V1) || LWIP_IPV4)
			/* if dhcp is started and not bound, stop it */
			if (NET_DHCP_DATA(nif) &&
				NET_DHCP_STATE(nif) != NET_DHCP_STATE_OFF &&
				NET_DHCP_STATE(nif) != NET_DHCP_STATE_BOUND) {
				net_config(nif, 0);
			}
#endif
		}
#ifdef CONFIG_WLAN_STA_FREQ_AUTO_ALIGN
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA) {
#ifdef CONFIG_BT_CLOSE_WLAN_FREQ_AUTO_ALIGN
			ret = xr_coex_set_freq_auto_align(1, 0, 0);
#else
			wlan_ext_freq_auto_align_cfg_t cfg = {0};

			cfg.mode = FFA_EXT_MODE_CLOSE_RECOVER_TRIM;
			ret = wlan_ext_request(nif, WLAN_EXT_CMD_SET_FREQ_AUTO_ALIGN, (uint32_t)(uintptr_t)(&cfg));
#endif
			if (ret)
				NET_WRN("Close freq auto align fail:%d\n", ret);
		}
#endif
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
		wlan_ext_request(nif, WLAN_EXT_CMD_SET_STAY_AWAKE_TMO, 0); //release wakelock
#endif
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA) {
			wlan_set_scan_additional_time(0);
		}
		break;
	case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
		wlan_ext_request(nif, WLAN_EXT_CMD_SET_STAY_AWAKE_TMO, 0); //release wakelock
#endif
		break;
	case NET_CTRL_MSG_WLAN_SCAN_FAILED:
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
		wlan_ext_request(nif, WLAN_EXT_CMD_SET_STAY_AWAKE_TMO, 0); //release wakelock
#endif
		break;
	case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
		wlan_set_scan_additional_time(1);
		break;
	case NET_CTRL_MSG_WLAN_AUTH_TIMEOUT:
		break;
	case NET_CTRL_MSG_WLAN_ASSOC_TIMEOUT:
		break;
	case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_DEV_HANG:
		break;
	case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
	case NET_CTRL_MSG_WLAN_ASSOC_FAILED:
	case NET_CTRL_MSG_WLAN_SAE_COMMIT_FAILED:
	case NET_CTRL_MSG_WLAN_SAE_CONFIRM_FAILED:
#ifdef CONFIG_STA_SOFTAP_COEXIST
	case NET_CTRL_MSG_WLAN_SELECT_BSS:
	{
		int sta_ch, ap_ch;
		int freq = (int)data;
		if (g_wlan_ap_netif != NULL) {
			if (freq >= 2412 && freq <= 2472) {
				sta_ch = (freq - 2407) / 5;
			} else if (freq == 2484) {
				sta_ch = 14;
			} else {
				sta_ch = 0;
			}
			wlan_ap_config_t config;
			memset(&config, 0, sizeof(config));
			config.field = WLAN_AP_FIELD_CHANNEL;
			wlan_ap_get_config(&config);
			ap_ch = config.u.channel;
			if (sta_ch != ap_ch) {
				NET_WRN("Soft AP need to switch to channel %d!!\n", sta_ch);
				wlan_ap_disable();
				config.u.channel = sta_ch;
				wlan_ap_set_config(&config);
				wlan_ap_enable();
			}
		}
		wlan_sta_select_network_to_connect();
	}
		break;
	case NET_CTRL_MSG_WLAN_AP_START:
		if (wlan_netif && !netif_is_link_up(wlan_netif)) {
			netifapi_netif_set_link_up(wlan_netif); /* set link up */
#if (defined(CONFIG_LWIP_V1) || LWIP_IPV4)
#if (!DHCP_SERVER_FROM_SYSINFO)
			net_config(wlan_netif, 1); /* bring up network */
#endif
#endif
		}
		//set sta netif default, only set ap netif when sta is not up
		if (!netif_is_link_up(g_wlan_netif)) {
			netifapi_netif_set_default(wlan_netif);
		}
		break;
	case NET_CTRL_MSG_WLAN_AP_STOP:
		if (wlan_netif) {
			if (netif_is_link_up(wlan_netif)) {
				netifapi_netif_set_link_down(wlan_netif); /* set link down */
			}
#if (defined(CONFIG_LWIP_V1) || LWIP_IPV4)
			/* if dhcp is started and not bound, stop it */
			if (NET_DHCP_DATA(wlan_netif) &&
				NET_DHCP_STATE(wlan_netif) != NET_DHCP_STATE_OFF &&
				NET_DHCP_STATE(wlan_netif) != NET_DHCP_STATE_BOUND) {
				net_config(wlan_netif, 0);
			}
#endif
		}
		break;
#endif /* CONFIG_STA_SOFTAP_COEXIST */

	case NET_CTRL_MSG_WLAN_AP_STA_DISCONNECTED:
#ifdef CONFIG_STA_SOFTAP_COEXIST
		nif = wlan_netif;
#endif
		ret = wlan_ext_request(nif, WLAN_EXT_CMD_GET_STATS_CODE, (uint32_t)(uintptr_t)(&param));
		if (!ret) {
			NET_DBG("reason code:%d status code:%d\n", param.reason_code, param.status_code);
		}
		break;
	case NET_CTRL_MSG_WLAN_AP_STA_CONNECTED:
		break;
	case NET_CTRL_MSG_WLAN_P2P_WAKE_UP:
		break;
	case NET_CTRL_MSG_WLAN_P2P_KEEPALIVE_CONNECT_LOST:
		break;
	case NET_CTRL_MSG_WLAN_P2P_IP_WAKE_UP:
		break;
	case NET_CTRL_MSG_NETWORK_UP:
#ifdef CONFIG_STA_SOFTAP_COEXIST
		nif = wlan_netif;
#endif
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
		pm_wakelocks_release(&pm_connect_lock);
		wlan_ext_request(nif, WLAN_EXT_CMD_SET_STAY_AWAKE_TMO, 0); //release wakelock
#endif
		netif_up_handler(nif);
		break;
	case NET_CTRL_MSG_NETWORK_DOWN:
		break;
#if (!defined(CONFIG_LWIP_V1) && LWIP_IPV6)
	case NET_CTRL_MSG_NETWORK_IPV6_STATE:
		break;
#endif
	default:
		NET_WRN("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

#if CONFIG_COMPONENTS_TUYA_LOW_POWER_APP || CONFIG_COMPONENTS_LOW_POWER_APP
typedef struct {
	net_ctrl_user_msg_cb init_cb;
} lp_net_ctrl_user_msg_cb;
static lp_net_ctrl_user_msg_cb lp_net_ctrl_user_msg_cb_f;
#endif

int net_ctrl_init(void)
{
	observer_base *ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK,
	                                                 NET_CTRL_MSG_ALL,
	                                                 net_ctrl_msg_process,
	                                                 NULL);
	if (ob == NULL)
		return -1;
	if (sys_ctrl_attach(ob) != 0)
		return -1;

#if CONFIG_COMPONENTS_TUYA_LOW_POWER_APP || CONFIG_COMPONENTS_LOW_POWER_APP
	if (lp_net_ctrl_user_msg_cb_f.init_cb != NULL) {
		lp_net_ctrl_user_msg_cb_f.init_cb();
	}
#endif
	return 0;
}

#if CONFIG_COMPONENTS_TUYA_LOW_POWER_APP || CONFIG_COMPONENTS_LOW_POWER_APP
int net_ctrl_msg_register(net_ctrl_user_msg_cb init_cb)
{
	lp_net_ctrl_user_msg_cb_f.init_cb = init_cb;
	return 0;
}
#endif

char *net_ctrl_msg_type_to_str(uint32_t type)
{
    switch (type) {
        case NET_CTRL_MSG_WLAN_CONNECTED:
            return "WLAN_CONNECTED";
        case NET_CTRL_MSG_WLAN_DISCONNECTED:
            return "WLAN_DISCONNECTED";
        case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
            return "WLAN_SCAN_SUCCESS";
        case NET_CTRL_MSG_WLAN_SCAN_FAILED:
            return "WLAN_SCAN_FAILED";
        case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
            return "WLAN_4WAY_HANDSHAKE_FAILED";
        case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
            return "WLAN_SSID_NOT_FOUND";
        case NET_CTRL_MSG_WLAN_AUTH_TIMEOUT:
            return "WLAN_AUTH_TIMEOUT";
        case NET_CTRL_MSG_WLAN_ASSOC_TIMEOUT:
            return "WLAN_ASSOC_TIMEOUT";
        case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
            return "WLAN_CONNECT_FAILED";
        case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
            return "WLAN_CONNECTION_LOSS";
        case NET_CTRL_MSG_WLAN_ASSOC_FAILED:
            return "WLAN_ASSOC_FAILED";
        case NET_CTRL_MSG_WLAN_SAE_COMMIT_FAILED:
            return "WLAN_SAE_COMMIT_FAILED";
        case NET_CTRL_MSG_WLAN_SAE_CONFIRM_FAILED:
            return "WLAN_SAE_CONFIRM_FAILED";
        case NET_CTRL_MSG_WLAN_P2P_WAKE_UP:
            return "WLAN_P2P_WAKE_UP";
        case NET_CTRL_MSG_WLAN_P2P_KEEPALIVE_CONNECT_LOST:
            return "WLAN_P2P_KEEPALIVE_CONNECT_LOST";
        case NET_CTRL_MSG_WLAN_P2P_IP_WAKE_UP:
            return "WLAN_P2P_IP_WAKE_UP";
#ifdef CONFIG_STA_SOFTAP_COEXIST
        case NET_CTRL_MSG_WLAN_AP_START:
            return "WLAN_AP_START";
        case NET_CTRL_MSG_WLAN_AP_STOP:
            return "WLAN_AP_STOP";
        case NET_CTRL_MSG_WLAN_SELECT_BSS:
            return "WLAN_SELECT_BSS";
#endif
        case NET_CTRL_MSG_WLAN_DEV_HANG:
            return "WLAN_DEV_HANG";
        case NET_CTRL_MSG_WLAN_AP_STA_CONNECTED:
            return "WLAN_AP_STA_CONNECTED";
        case NET_CTRL_MSG_WLAN_AP_STA_DISCONNECTED:
            return "WLAN_AP_STA_DISCONNECTED";
        case NET_CTRL_MSG_NETWORK_UP:
            return "NETWORK_UP";
        case NET_CTRL_MSG_NETWORK_DOWN:
            return "NETWORK_DOWN";
#if (!defined(CONFIG_LWIP_V1) && LWIP_IPV6)
        case NET_CTRL_MSG_NETWORK_IPV6_STATE:
            return "NETWORK_IPV6_STATE";
#endif
        case NET_CTRL_MSG_ALL:
            return "NET_CTRL_MSG_ALL";
        default:
            return "UNKNOWN";
    }
}
