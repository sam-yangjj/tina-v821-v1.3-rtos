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

#include "cmd_util.h"
#include "cmd_dhcps.h"
#include "lwip/dhcps.h"
#include "lwip/inet.h"
#include "lwip/netifapi.h"
#include "console.h"

#ifdef CONFIG_STA_SOFTAP_COEXIST
#define CMD_DHCPS_ADDR_START "192.168.31.100"
#else
#define CMD_DHCPS_ADDR_START "192.168.51.100"
#endif
#define CMD_DHCPS_ADDR_NUM   5
#define CMD_DHCPS_LEASE_TIME (60 * 60 * 12)
#define CMD_DHCPS_OFFER_TIME (5 * 60)
#define CMD_DHCPS_DECLINE_TIME (10 * 60)

static dhcp_server_config_t dhcps_info;
#ifdef CONFIG_STA_SOFTAP_COEXIST
extern struct netif *g_wlan_ap_netif;
#else
extern struct netif *g_wlan_netif;
#endif

static enum cmd_status dhcps_start_exec(char *cmd)
{
#ifdef CONFIG_STA_SOFTAP_COEXIST
	struct netif *nif = g_wlan_ap_netif;
#else
	struct netif *nif = g_wlan_netif;
#endif

	if (dhcps_info.start_ip == NULL)
		dhcps_info.start_ip = CMD_DHCPS_ADDR_START;
	if (dhcps_info.ip_num == 0)
		dhcps_info.ip_num = CMD_DHCPS_ADDR_NUM;
	if (dhcps_info.lease_time == 0)
		dhcps_info.lease_time = CMD_DHCPS_LEASE_TIME;
	if (dhcps_info.offer_time == 0)
		dhcps_info.offer_time = CMD_DHCPS_OFFER_TIME;
	if (dhcps_info.decline_time == 0)
		dhcps_info.decline_time = CMD_DHCPS_DECLINE_TIME;

	dhcps_config(&dhcps_info);
	netifapi_dhcps_start(nif);
	return CMD_STATUS_OK;
}

static enum cmd_status dhcps_stop_exec(char *cmd)
{
#ifdef CONFIG_STA_SOFTAP_COEXIST
	struct netif *nif = g_wlan_ap_netif;
#else
	struct netif *nif = g_wlan_netif;
#endif

	netifapi_dhcps_stop(nif);
	return CMD_STATUS_OK;
}

static enum cmd_status dhcps_set_ippool_exec(char *cmd)
{
	int argc;
	char *argv[2];
	ip_addr_t ip_addr_start;
	uint16_t ip_num;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc != 2) {
		CMD_ERR("invalid dhcp cmd, argc %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	if (inet_aton(argv[0], &ip_addr_start) == 0 || \
			(ip_num = cmd_atoi(argv[1])) == 0) {
		CMD_ERR("invalid dhcp cmd <%s %s>\n", argv[0], argv[1]);
		return CMD_STATUS_INVALID_ARG;
	}

#ifdef CONFIG_LWIP_V1
	dhcps_info.start_ip = inet_ntoa(ip_addr_start);
	dhcps_info.ip_num   = ip_num;
#elif LWIP_IPV4 /* now only for IPv4 */
	dhcps_info.start_ip = inet_ntoa(ip_addr_start);
	dhcps_info.ip_num   = ip_num;
#else
	#error "IPv4 not support!"
#endif

	return CMD_STATUS_OK;
}

static enum cmd_status dhcps_set_lease_time_exec(char *cmd)
{
	int argc;
	char *argv[1];
	int lease_time;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc != 1) {
		CMD_ERR("invalid dhcp cmd, argc %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	lease_time = cmd_atoi(argv[0]);
	if (lease_time < 60) {
		CMD_ERR("leasetime must greater than 60\n");
		return CMD_STATUS_INVALID_ARG;
	}

	dhcps_info.lease_time = lease_time;
	return CMD_STATUS_OK;
}

static enum cmd_status dhcps_set_offer_time_exec(char *cmd)
{
	int argc;
	char *argv[1];
	int offer_time;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc != 1) {
		CMD_ERR("invalid dhcp cmd, argc %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	offer_time = cmd_atoi(argv[0]);
	if (offer_time < 60) {
		CMD_ERR("offertime must greater than 60\n");
		return CMD_STATUS_INVALID_ARG;
	}

	dhcps_info.offer_time = offer_time;
	return CMD_STATUS_OK;
}

static enum cmd_status dhcps_set_decline_time_exec(char *cmd)
{
	int argc;
	char *argv[1];
	int decline_time;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc != 1) {
		CMD_ERR("invalid dhcp cmd, argc %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	decline_time = cmd_atoi(argv[0]);
	if (decline_time < 60) {
		CMD_ERR("declinetime must greater than 60\n");
		return CMD_STATUS_INVALID_ARG;
	}

	dhcps_info.decline_time = decline_time;
	return CMD_STATUS_OK;
}

#if CMD_DESCRIBE
#define dhcps_start_help_info "start the dhcps server."
#define dhcps_stop_help_info  "stop the dhcps server."
#define dhcps_ippool_help_info "set the dhcps server ippool, ippool <ip-addr-start> <ip-addr-num>."
#define dhcps_set_lease_time_help_info "set the set leases time of dhcps server, leasetime <sec>."
#define dhcps_set_offer_time_help_info "set the set offer time of dhcps server, offertime <sec>."
#define dhcps_set_decline_time_help_info "set the set decline time of dhcps server, declinetime <sec>."
#endif

/*
 * dhcp commands
 */
static enum cmd_status cmd_dhcps_help_exec(char *cmd);

static const struct cmd_data g_dhcps_cmds[] = {
	{ "start",      dhcps_start_exec,          CMD_DESC(dhcps_start_help_info) },
	{ "stop",       dhcps_stop_exec,           CMD_DESC(dhcps_stop_help_info) },
	{ "ippool",     dhcps_set_ippool_exec,     CMD_DESC(dhcps_ippool_help_info) },
	{ "leasetime",  dhcps_set_lease_time_exec, CMD_DESC(dhcps_set_lease_time_help_info) },
	{ "offertime",  dhcps_set_offer_time_exec, CMD_DESC(dhcps_set_offer_time_help_info) },
	{ "declinetime",  dhcps_set_decline_time_exec, CMD_DESC(dhcps_set_decline_time_help_info) },
	{ "help",       cmd_dhcps_help_exec,       CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_dhcps_help_exec(char *cmd)
{
	return cmd_help_exec(g_dhcps_cmds, cmd_nitems(g_dhcps_cmds), 8);
}

enum cmd_status cmd_dhcps_exec(char *cmd)
{
	return cmd_exec(cmd, g_dhcps_cmds, cmd_nitems(g_dhcps_cmds));
}
static void dhcps_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_dhcps_exec);
}

FINSH_FUNCTION_EXPORT_CMD(dhcps_exec, dhcps, dhcps testcmd);
