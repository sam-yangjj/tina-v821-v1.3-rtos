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

#include "wlan_fw_keepalive_client.h"
#include "lp_rpmsg.h"
#include "lp_ctrl_msg.h"
#include "stdlib.h"

static wlan_fw_keep tcp_client[1] = { 0 };
__standby_unsaved_data const char wakeup_msg[] = FW_TCP_WAKEUP_MSG;

struct wakelock fw_keepalive_wakelock = {
	.name = "fw_keepalive",
	.ref = 0,
};

static int domain_get_host(char *name, char *res)
{
	struct hostent *hptr = gethostbyname(name);
	if (hptr == NULL) {
		LP_LOG_INFO("gethostbyname error for host: %s: %d\n", name, h_errno);
		return -1;
	}
	LP_LOG_INFO("hostname: %s\n", hptr->h_name);
	memcpy(res, hptr->h_addr_list[0], INET_ADDRSTRLEN);
	LP_LOG_INFO("ipaddress: %s\n", inet_ntop(hptr->h_addrtype, hptr->h_addr_list[0], res, INET_ADDRSTRLEN));
	return 0;
}

static void lp_close_socket(int sockfd)
{
	if (!close(sockfd)) {
		LP_LOG_INFO("Socket closed successfully\n");
	} else {
		LP_LOG_INFO("Error closing socket: %s\n", strerror(errno));
	}
}

static int lp_set_socket_o_onoblock(int sockfd, int block)
{
	LP_LOG_INFO("lp_set_socket_o_onoblock set mode = %s\n", block ? "nonblock" : "block");
	if (!block) {
		if (lwip_fcntl(sockfd, F_SETFL, lwip_fcntl(sockfd, F_GETFL, 0) & ~O_NONBLOCK) == -1) {
			LP_LOG_INFO("Error setting socket block mode: %s\n", strerror(errno));
			return -1;
		}
	} else {
		if (lwip_fcntl(sockfd, F_SETFL, lwip_fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
			LP_LOG_INFO("Error setting socket block mode: %s\n", strerror(errno));
			return -1;
		}
	}
	return 0;
}

/* tcp connect */
static int wlan_keepalive_connect(void)
{
	struct sockaddr_in server;
	char s_ip[INET_ADDRSTRLEN] = { 0 };
	int sockfd = -1;
retry:
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		LP_LOG_INFO("socket failure\n");
		return -1;
	}

	if (lp_set_socket_o_onoblock(sockfd, 1) == -1) {
		lp_close_socket(sockfd);
		return -1;
	}

	/* get domain ip */
	if (!domain_get_host(FW_TCP_SERVER_IP, s_ip)) {
		LP_LOG_INFO("domain_get_host success\n");
	} else {
		LP_LOG_INFO("domain_get_host error\n");
	}

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(FW_TCP_SERVER_PORT);
	server.sin_addr.s_addr = inet_addr(s_ip);

	if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		if (errno != EINPROGRESS) {
			LP_LOG_INFO("Error connecting to server: %s\n", strerror(errno));
			lp_close_socket(sockfd);
			hal_sleep(1);
			goto retry;
		}
		fd_set write_fds;
		FD_ZERO(&write_fds);
		FD_SET(sockfd, &write_fds);
		struct timeval timeout;
		timeout.tv_sec = 3; //timeout time
		timeout.tv_usec = 0;
		int result = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);
		if (result == -1) {
			LP_LOG_INFO("Error select in connect: %s\n", strerror(errno));
			lp_close_socket(sockfd);
			return -1;
		} else if (result == 0) {
			LP_LOG_INFO("Connection timed out\n");
			lp_close_socket(sockfd);
			hal_sleep(1);
			goto retry;
		} else {
			int error = 0;
			socklen_t len = sizeof(error);
			if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
				LP_LOG_INFO("Error getting socket option: %s\n", strerror(errno));
				lp_close_socket(sockfd);
				hal_sleep(1);
				goto retry;
			}
			if (error != 0) {
				LP_LOG_INFO("Socket error: %s\n", strerror(errno));
				lp_close_socket(sockfd);
				hal_sleep(1);
				goto retry;
			}
		}
	}
	if (lp_set_socket_o_onoblock(sockfd, 0) == -1) {
		lp_close_socket(sockfd);
		return -1;
	}
	return sockfd;
}

static int wlan_fw_keepalive_network_init(wlan_fw_keep *tcp_kl)
{
	char server_mac[6] = { 0 };
	char str[32] = { 0 };
	int i = 0;
	int ret = -1;
	int ser_num = 0;
	struct sockaddr_in server;
	struct sockaddr_in client;
	socklen_t addr_len = sizeof(struct sockaddr);
	wlan_fw_keep *ptcp_keepalive = tcp_kl;
	p2p_keepalive_all_t *pparam;

	/*1. creat a network connection */
	ptcp_keepalive->tcp_keepalive_sock = wlan_keepalive_connect();
	if (ptcp_keepalive->tcp_keepalive_sock != -1) {
		LP_LOG_INFO("wlan_keepalive_connect success\n");
	} else {
		LP_LOG_INFO("wlan_keepalive_connect error\n");
		return -1;
	}

	/* simple authentication demo */
	ret = sprintf(str, "REGISTER:%02X:%02X:%02X:%02X:%02X:%02X",
		g_wlan_netif->hwaddr[0],
		g_wlan_netif->hwaddr[1],
		g_wlan_netif->hwaddr[2],
		g_wlan_netif->hwaddr[3],
		g_wlan_netif->hwaddr[4],
		g_wlan_netif->hwaddr[5]); //g_wlan_netif decleared in net_ctrl.h
	ret = send(ptcp_keepalive->tcp_keepalive_sock, str, ret, 0);
	if (ret < 0) {
		LP_LOG_INFO("send auth_info failed %d\n", ret);
		close(ptcp_keepalive->tcp_keepalive_sock);
		return -1;
	} else {
		LP_LOG_INFO("send auth_info = [%s], success!\n", str);
	}

	LP_LOG_INFO("waiting recv\n");
	memset(str, 0, sizeof(str));
	ret = recv(ptcp_keepalive->tcp_keepalive_sock, str, sizeof(str), 0);
	if (ret <= 0) {
		LP_LOG_INFO("tcp recv ok failed %d\n", ret);
		close(ptcp_keepalive->tcp_keepalive_sock);
		return -1;
	} else {
		LP_LOG_INFO("tcp recv [%s][len:%d] success, start set fw keepalive\n", str, ret);
	}

	/* 2. set fw keepalive parameters */
	/* 2.1 set keepalive server information */
	pparam = malloc(sizeof(p2p_keepalive_all_t));
	if (pparam == NULL) {
		LP_LOG_INFO("malloc fail\n");
		goto ERR1;
	}
	memset(pparam, 0, sizeof(p2p_keepalive_all_t));

	getsockname(ptcp_keepalive->tcp_keepalive_sock, (struct sockaddr *)&client, &addr_len);
	memcpy(pparam->svr.SrcIPv4Addr, &(g_wlan_netif->ip_addr.addr), 4);
	pparam->svr.P2PServerCfgs[ser_num].DstPort = FW_TCP_SERVER_PORT;
	pparam->svr.P2PServerCfgs[ser_num].SrcPort = ntohs(client.sin_port);

	getpeername(ptcp_keepalive->tcp_keepalive_sock, (struct sockaddr *)&server, &addr_len);
	memcpy(pparam->svr.P2PServerCfgs[ser_num].DstIPv4Addr, &server.sin_addr.s_addr, 4);
	etharp_get_mac_from_ip((ip4_addr_t *)&server.sin_addr.s_addr, (struct eth_addr *)&server_mac);

	for (i = 0;i < 6;i++) {
		pparam->svr.P2PServerCfgs[ser_num].DstMacAddr[i] = server_mac[i];
	}

	pparam->svr.P2PServerCfgs[ser_num].TcpSeqInit = getsockack(ptcp_keepalive->tcp_keepalive_sock);
	pparam->svr.P2PServerCfgs[ser_num].TcpAckInit = getsockseq(ptcp_keepalive->tcp_keepalive_sock);
	pparam->svr.P2PServerCfgs[ser_num].IPIdInit = 1;
	pparam->svr.P2PServerCfgs[ser_num].Enable = 1;
	pparam->svr.P2PServerCfgs[ser_num].TcpOrUdp = 1;  //tcp 1;udp 2

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_SVR, (uint32_t)(uintptr_t)(&pparam->svr));
	if (ret == -2) {
		LP_LOG_INFO("%s: command 'p2p_server' invalid arg\n", __func__);
		goto ERR2;
	} else if (ret == -1) {
		LP_LOG_INFO("%s: command 'p2p_server' exec failed\n", __func__);
		goto ERR2;
	}
	LP_LOG_INFO("server mac is %02x:%02x:%02x:%02x:%02x:%02x\n", server_mac[0], server_mac[1], server_mac[2], server_mac[3], server_mac[4], server_mac[5]);

	/* 2.2 set keepalive packet parameters */
	pparam->param.KeepAlivePeriod_s = FW_TCP_HEART_INTERVAL;
	pparam->param.TxTimeOut_s = 5;
	pparam->param.TxRetryLimit = 10;
	pparam->param.P2PKeepAliveParamCfgs[ser_num].Enable = 1;
	/* example: keepalive packet content is the last character of device eth addr */
	sprintf((char *)pparam->param.P2PKeepAliveParamCfgs[ser_num].Payload, "%02X:%02X:%02X:%02X:%02X:%02X",
		(int)g_wlan_netif->hwaddr[0],
		(int)g_wlan_netif->hwaddr[1],
		(int)g_wlan_netif->hwaddr[2],
		(int)g_wlan_netif->hwaddr[3],
		(int)g_wlan_netif->hwaddr[4],
		(int)g_wlan_netif->hwaddr[5]);//g_wlan_netif decleared in net_ctrl.h
	pparam->param.P2PKeepAliveParamCfgs[ser_num].PayloadLen = strlen((char *)pparam->param.P2PKeepAliveParamCfgs[ser_num].Payload);

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_KPALIVE_CFG, (uint32_t)(uintptr_t)(&pparam->param));
	if (ret == -2) {
		LP_LOG_INFO("%s: command 'p2p_keepalive' invalid arg\n", __func__);
		goto ERR3;
	} else if (ret == -1) {
		LP_LOG_INFO("%s: command 'p2p_keepalive' exec failed\n", __func__);
		goto ERR3;
	}
	LP_LOG_INFO("set fw keeplaive interval is %ds, timeout %ds, retry limit %d, keepalive content %s\n",
		pparam->param.KeepAlivePeriod_s, pparam->param.TxTimeOut_s, pparam->param.TxRetryLimit, pparam->param.P2PKeepAliveParamCfgs[ser_num].Payload);

	/* 2.3 set wakeup packet parameters */
	pparam->wkp_param.P2PWkpParamCfgs[ser_num].Enable = 1;
	pparam->wkp_param.P2PWkpParamCfgs[ser_num].PayloadLen = strlen(wakeup_msg);
	for (i = 0; i < strlen(wakeup_msg); i++) {
		pparam->wkp_param.P2PWkpParamCfgs[ser_num].Payload[i] = wakeup_msg[i];
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_WKP_CFG, (uint32_t)(uintptr_t)(&pparam->wkp_param));
	if (ret == -2) {
		LP_LOG_INFO("%s: command 'p2p_wakeup_set' invalid arg\n", __func__);
		goto ERR4;
	} else if (ret == -1) {
		LP_LOG_INFO("%s: command 'p2p_wakeup_set' exec failed\n", __func__);
		goto ERR4;
	}
	LP_LOG_INFO("set fw keepalive wakeup payload is %s, payload len %d\n",
		pparam->wkp_param.P2PWkpParamCfgs[ser_num].Payload, pparam->wkp_param.P2PWkpParamCfgs[ser_num].PayloadLen);

	/* 2.4 set ARP Response replace NULL to keepalive */
	pparam->arpkeepalive_param.ArpKeepAlivePeriod = 48;
	memcpy(pparam->arpkeepalive_param.SenderIpv4IpAddress, &(g_wlan_netif->ip_addr.addr), 4);
	memcpy(pparam->arpkeepalive_param.TargetIpv4IpAddress, &server.sin_addr.s_addr, 4);
	for (int i = 0; i < 6; i++) {
		pparam->arpkeepalive_param.TargetMacAddress[i] = (uint8_t)server_mac[i];
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_ARP_KPALIVE, (uint32_t)(uintptr_t)(&pparam->arpkeepalive_param));
	if (ret == -2) {
		LP_LOG_INFO("%s: command set_arp_keepalive invalid arg\n", __func__);
		goto ERR5;
	} else if (ret == -1) {
		LP_LOG_INFO("%s: command set_arp_keepalive exec failed\n", __func__);
		goto ERR5;
	}
	LP_LOG_INFO("set arp_keepalive\n");

	/* 2.5 set wlan host sleep */
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_HOST_SLEEP, 1);
	if (ret == -2) {
		LP_LOG_INFO("%s: command host_sleep invalid arg\n", __func__);
		goto ERR6;
	} else if (ret == -1) {
		LP_LOG_INFO("%s: command host_sleep exec failed\n", __func__);
		goto ERR6;
	}
	LP_LOG_INFO("set fw keepalive hostsleep 1\n");

	free(pparam);
	return 0;

ERR6:
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_HOST_SLEEP, 0);
ERR5:
	memset(&pparam->arpkeepalive_param, 0, sizeof(wlan_ext_arp_kpalive_set_t));
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_ARP_KPALIVE, (uint32_t)(uintptr_t)(&pparam->arpkeepalive_param));
ERR4:
	memset(&pparam->wkp_param, 0, sizeof(wlan_ext_p2p_wkp_param_set_t));
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_WKP_CFG, (uint32_t)(uintptr_t)(&pparam->wkp_param));
ERR3:
	memset(&pparam->param, 0, sizeof(wlan_ext_p2p_kpalive_param_set_t));
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_KPALIVE_CFG, (uint32_t)(uintptr_t)(&pparam->param));
ERR2:
	memset(&pparam->svr, 0, sizeof(wlan_ext_p2p_svr_set_t));
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_SVR, (uint32_t)(uintptr_t)(&pparam->svr));
	free(pparam);
ERR1:
	close(ptcp_keepalive->tcp_keepalive_sock);
	return ret;
}

/* deinit wlan fw keepalive */
int wlan_fw_keepalive_network_deinit(wlan_fw_keep *tcp_kl)
{
	int sockfd = tcp_client->tcp_keepalive_sock;
	p2p_keepalive_all_t param;

	memset(&param, 0, sizeof(p2p_keepalive_all_t));
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_HOST_SLEEP, 0);
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_ARP_KPALIVE, (uint32_t)(uintptr_t)(&param.arpkeepalive_param));
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_WKP_CFG, (uint32_t)(uintptr_t)(&param.wkp_param));
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_KPALIVE_CFG, (uint32_t)(uintptr_t)(&param.param));
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_SVR, (uint32_t)(uintptr_t)(&param.svr));
	close(sockfd);

	return 0;
}

int wlan_msg_packet_wakeup_callback(void *arg)
{
	/* TODO: wakeup by packet process */
	LP_LOG_INFO("device recv wakeup packet\n");
	if (pm_wakelocks_acquire(&fw_keepalive_wakelock,
		PM_WL_TYPE_WAIT_ONCE,
		OS_WAIT_FOREVER)) {
		LP_LOG_ERR("pm_wakelocks_acquire error\n");
	}
	pm_wakesrc_relax(fw_keepalive_wakelock.ws, PM_RELAX_WAKEUP);  /* wakeup cpux */
	if (fw_keepalive_wakelock.ref) {
		LP_LOG_INFO("%s:pm_wakelocks_release in %d\n", __FUNCTION__, __LINE__);
		pm_wakelocks_release(&fw_keepalive_wakelock);
	}
	return 0;
}

int wlan_msg_connect_lost_callback(void *arg)
{
	/* TODO: keepalive ip process */
	LP_LOG_INFO("conncet lost\n");
	wlan_fw_keepalive_network_deinit(arg);
	lp_rpmsg_set_keepalive_state(0);
	return 0;
}

int wlan_msg_ip_wakeup_callback(void *arg)
{
	/* TODO: wakeup by packet process */
	LP_LOG_INFO("device recv wakeup ip\n");
	return 0;
}

void wlan_fw_low_power_msg_proc(uint32_t event, uint32_t data, void *arg)
{
	int ret = -1;
	uint16_t type = EVENT_SUBTYPE(event);
	struct netif *nif = g_wlan_netif;
	LP_LOG_INFO("Low Power net event: %s\r\n", net_ctrl_msg_type_to_str(type));
	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA && NET_IS_IP4_VALID(nif)) {
			/* set wlan ditm 10 when wifi connect */
			ret = wlan_ext_low_power_param_set_default(10);
			if (ret == -2) {
				LP_LOG_ERR("Set Dtim 10 invalid arg\n");
			} else if (ret == -1) {
				LP_LOG_ERR("Set Dtim 10 exec failed\n");
			}
		}
		break;
	case NET_CTRL_MSG_NETWORK_UP:
		LP_LOG_INFO("### Network up\n");
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA) {
			tcp_client->fw_network_ok = 1;
			/* set wlan ditm 10 when wifi connect */
			ret = wlan_ext_low_power_param_set_default(10);
			if (ret == -2) {
				LP_LOG_ERR("Set Dtim 10 invalid arg\n");
			} else if (ret == -1) {
				LP_LOG_ERR("Set Dtim 10 exec failed\n");
			}
		}
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		LP_LOG_INFO("### Wlan disconnect\n");
		break;
	case NET_CTRL_MSG_WLAN_P2P_WAKE_UP:
		wlan_msg_packet_wakeup_callback(arg);
		break;
	case NET_CTRL_MSG_WLAN_P2P_KEEPALIVE_CONNECT_LOST:
		wlan_msg_connect_lost_callback(arg);
		break;
	case NET_CTRL_MSG_WLAN_P2P_IP_WAKE_UP:
		wlan_msg_ip_wakeup_callback(arg);
		break;
	case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
		LP_LOG_INFO("### Wlan ssid not found\n");
		break;
	case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
		LP_LOG_INFO("### Wlan connect loss(BSS LOST)\n");
		break;
	default:
		LP_LOG_WARN("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

int wlan_fw_low_power_keepalive_demo(void)
{
	int ret = -1;

	if (lp_rpmsg_init()) {
		LP_LOG_INFO("%s: lp_rpmsg_init failed\n", __func__);
		return -1;
	}

	lp_ctrl_msg_register(wlan_fw_low_power_msg_proc);

	while (!tcp_client->fw_network_ok) {
		hal_msleep(100);
	}
	LP_LOG_INFO("Network is OK, go on now\n");

	hal_msleep(3000);  //wait cpux mqtt connected

	ret = wlan_fw_keepalive_network_init(tcp_client);
	if (ret) {
		LP_LOG_INFO("network init error %d\n", ret);
		return ret;
	}

	/* set wifi fw wait tcp ack timeout to 15 ms */
	//low_power_wlan_set_fw_try_wait_tcp_pkt_tmo(15);

	lp_rpmsg_set_keepalive_state(1);

	LP_LOG_INFO("keepalive has build\n");

	return ret;
}

int wlan_fw_low_power_keepalive_demo_deinit(void)
{
	wlan_fw_keepalive_network_deinit(NULL);
	lp_rpmsg_set_keepalive_state(0);
	lp_rpmsg_deinit();
	memset(tcp_client, 0, sizeof(*tcp_client));
	LP_LOG_INFO("wlan_fw_low_power_keepalive_demo_deinit complete\n");
	return 0;
}
