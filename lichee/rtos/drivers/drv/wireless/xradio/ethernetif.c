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

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#ifdef CONFIG_LWIP_V1
#include "netif/etharp.h"
#else
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#endif
#include "lwip/netifapi.h"
#include "net/ethernetif/ethernetif.h"
#include <string.h>
#include "sys/mbuf.h"
#include "sys/xr_util.h"
#include "net/wlan/wlan.h"

#include "tcpip_adapter.h"

#include <stdlib.h>

#ifdef CONFIG_MUTIL_NET_STACK
#include "xrlink/xrlink.h"
#define ICMP_POTOCOL_TYPE	1
#endif
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
#include "pm_state.h"
#endif

#define ETH_DBG_ON      0
#define ETH_WRN_ON      1
#define ETH_ERR_ON      1
#define ETH_ABORT_ON    0

#define ETH_SYSLOG      printf
#define ETH_ABORT()     sys_abort()

#define ETH_LOG(flags, fmt, arg...)     \
    do {                                \
        if (flags)                      \
            ETH_SYSLOG(fmt, ##arg);     \
    } while (0)

#define ETH_DBG(fmt, arg...)   ETH_LOG(ETH_DBG_ON, "[eth] "fmt, ##arg)
#define ETH_WRN(fmt, arg...)   ETH_LOG(ETH_WRN_ON, "[eth W] "fmt, ##arg)
#define ETH_ERR(fmt, arg...)                            \
    do {                                                \
        ETH_LOG(ETH_ERR_ON, "[eth E] %s():%d, "fmt,     \
               __func__, __LINE__, ##arg);              \
        if (ETH_ABORT_ON)                               \
            ETH_ABORT();                                \
    } while (0)

#define ETHER_MTU_MAX           1500
#define NETIF_LINK_SPEED_BPS    (100 * 1000 * 1000)
#define NETIF_ATTACH_BASE_FLAGS (NETIF_FLAG_BROADCAST   | \
                                 NETIF_FLAG_ETHARP      | \
                                 NETIF_FLAG_ETHERNET    | \
                                 NETIF_FLAG_IGMP)
#if (!defined(CONFIG_LWIP_V1) && LWIP_IPV6)
#define NETIF_ATTACH_FLAGS      (NETIF_ATTACH_BASE_FLAGS | NETIF_FLAG_MLD6)
#else
#define NETIF_ATTACH_FLAGS      (NETIF_ATTACH_BASE_FLAGS)
#endif

#if ((defined(CONFIG_LWIP_V1) && LWIP_SNMP) || \
	 (!defined(CONFIG_LWIP_V1) && MIB2_STATS))
#define ETH_SNMP_STATS	1
#else
#define ETH_SNMP_STATS	0
#endif

struct ethernetif {
	struct netif nif;
	enum wlan_mode mode;
};

#define ethernetif2netif(eth)	((struct netif *)(eth))
#define netif2ethernetif(nif)	((struct ethernetif *)(nif))

#ifdef CONFIG_STA_SOFTAP_COEXIST
static struct ethernetif *g_eth_netif;
static struct ethernetif *g_eth_ap_netif;
#else
static struct ethernetif g_eth_netif;
#endif


#if LWIP_NETIF_HOSTNAME
#define NETIF_HOSTNAME_MAX_LEN		32
static char g_netif_hostname[NETIF_HOSTNAME_MAX_LEN];

void ethernetif_set_hostname(char *hostname)
{
	if (hostname != NULL) {
		strlcpy(g_netif_hostname, hostname, NETIF_HOSTNAME_MAX_LEN);
	}
}
#endif /* LWIP_NETIF_HOSTNAME */

static err_t tcpip_null_input(struct pbuf *p, struct netif *nif)
{
	ETH_WRN("%s() called\n", __func__);
	pbuf_free(p);
	return ERR_OK;
}

#ifdef CONFIG_LWIP_V1

static err_t ethernetif_null_output(struct netif *nif, struct pbuf *p, ip_addr_t *ipaddr)
{
	ETH_WRN("%s() called\n", __func__);
	return ERR_IF;
}

#else /* CONFIG_LWIP_V1 */

#if LWIP_IPV4
static err_t ethernetif_null_ip4output(struct netif *nif, struct pbuf *p, const ip4_addr_t *ip4addr)
{
	ETH_WRN("%s() called\n", __func__);
	return ERR_IF;
}
#endif

#if LWIP_IPV6
static err_t ethernetif_null_ip6output(struct netif *nif, struct pbuf *p, const ip6_addr_t *ip6addr)
{
	ETH_WRN("%s() called\n", __func__);
	return ERR_IF;
}
#endif

#endif /* CONFIG_LWIP_V1 */

static err_t ethernetif_null_linkoutput(struct netif *nif, struct pbuf *p)
{
	ETH_WRN("%s() called\n", __func__);
	return ERR_IF;
}

#if (LWIP_MBUF_SUPPORT == 0)
static __inline struct mbuf *eth_pbuf2mbuf(struct pbuf *p)
{
	struct mbuf *m;
	struct pbuf *q;
	uint8_t *data;
	int32_t left;

	/* get a mbuf */
	m = mb_get(p->tot_len, 1 | MBUF_GET_FLAG_LIMIT_TX);
	if (m == NULL) {
		return NULL;
	}

	/* copy all data to mbuf */
	data = mtod(m, uint8_t *);
	left = m->m_len;
	for (q = p; q != NULL; q = q->next) {
		if (left >= q->len) {
			memcpy(data, q->payload, q->len);
			data += q->len;
			left -= q->len;
		} else {
			break;
		}
	}
	if (left != 0) {
		ETH_ERR("left %d, total %u\n", left, p->tot_len);
		mb_free(m);
		return NULL;
	}
	return m;
}
#endif /* (LWIP_MBUF_SUPPORT == 0) */

/* NB: @p is freed by Lwip. */
static err_t ethernetif_linkoutput(struct netif *nif, struct pbuf *p)
{
	struct mbuf *m;
	int ret;

#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

#if ETH_SNMP_STATS
	snmp_add_ifoutoctets(nif, p->tot_len);
	if (((u8_t *)p->payload)[0] & 1) {
		snmp_inc_ifoutnucastpkts(nif); /* broadcast or multicast packet*/
	} else {
		snmp_inc_ifoutucastpkts(nif); /* unicast packet */
	}
#endif

#if (LWIP_MBUF_SUPPORT == 0)
	m = eth_pbuf2mbuf(p);
#elif (LWIP_MBUF_SUPPORT == 1)
	m = mb_pbuf2mbuf(p);
#endif /* LWIP_MBUF_SUPPORT */

#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

	if (m == NULL) {
		ETH_DBG("pbuf2mbuf() failed\n");
		LINK_STATS_INC(link.memerr);
		snmp_inc_ifoutdiscards(nif);
		return ERR_MEM;
	}

	ret = wlan_linkoutput(nif, m);
	if (ret != 0) {
		ETH_WRN("linkoutput failed (%d)\n", ret);
		LINK_STATS_INC(link.err);
#ifndef CONFIG_LWIP_V1
		snmp_inc_ifouterrors(nif);
#endif
	} else {
		LINK_STATS_INC(link.xmit);
	}
	return ret;
}

#ifdef CONFIG_LWIP_V1

static err_t ethernetif_output(struct netif *nif, struct pbuf *p, ip_addr_t *ipaddr)
{
	if (!netif_is_link_up(nif)) {
		ETH_DBG("netif %p is link down\n", nif);
		return ERR_IF;
	}

	return etharp_output(nif, p, ipaddr);
}

#else /* CONFIG_LWIP_V1 */

#if LWIP_IPV4
static err_t ethernetif_ip4output(struct netif *nif, struct pbuf *p, const ip4_addr_t *ip4addr)
{
	if (!netif_is_link_up(nif)) {
		ETH_DBG("netif %p is link down\n", nif);
		return ERR_IF;
	}

	return etharp_output(nif, p, ip4addr);
}
#endif

#if LWIP_IPV6
static err_t ethernetif_ip6output(struct netif *nif, struct pbuf *p, const ip6_addr_t *ip6addr)
{
	if (!netif_is_link_up(nif)) {
		ETH_DBG("netif %p is link down\n", nif);
		return ERR_IF;
	}

	return ethip6_output(nif, p, ip6addr);
}
#endif

#endif /* CONFIG_LWIP_V1 */

/* NB: call by RX task to process received data */
err_t ethernetif_input(struct netif *nif, struct pbuf *p)
{
	err_t err = ERR_MEM;

	do {
		if (p == NULL) {
			ETH_DBG("pbuf is NULL\n");
			LINK_STATS_INC(link.memerr);
			break;
		}
#if ETH_SNMP_STATS
		snmp_add_ifinoctets(nif, p->tot_len);
		if (((u8_t *)p->payload)[0] & 1) {
			snmp_inc_ifinnucastpkts(nif); /* broadcast or multicast packet*/
		} else {
			snmp_inc_ifinucastpkts(nif); /* unicast packet*/
		}
#endif
#if ETH_PAD_SIZE
		if (pbuf_header(p, ETH_PAD_SIZE) != 0) {
			/* add padding word for LwIP */
			ETH_WRN("pbuf_header(%d) failed!\n", ETH_PAD_SIZE);
			LINK_STATS_INC(link.memerr);
			break;
		}
#endif /* ETH_PAD_SIZE */

		/* send data to LwIP, nif->input() == tcpip_input() */
		err = nif->input(p, nif);
		if (err != ERR_OK) {
			ETH_WRN("lwip process data failed, err %d!\n", err);
//			LINK_STATS_INC(link.err);
		} else {
			p = NULL; /* pbuf will be freed by LwIP */
		}
	} while (0);

	if (p) {
		pbuf_free(p);
	}

	if (err == ERR_OK) {
		LINK_STATS_INC(link.recv);
	} else {
		LINK_STATS_INC(link.drop);
		snmp_inc_ifindiscards(nif);
	}

	return err;
}

#ifdef CONFIG_MUTIL_NET_STACK
#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
int xr_rxq_queued_num(void);
#endif
uint8_t g_dbg_disable_tx2host;
#endif

#if (LWIP_MBUF_SUPPORT == 0)
#if MBUF_FREE_BY_ETHERNETIF
err_t ethernetif_raw_input(struct netif *nif, struct mbuf *m, u16_t len)
#else
err_t ethernetif_raw_input(struct netif *nif, uint8_t *data, u16_t len)
#endif /* MBUF_FREE_BY_ETHERNETIF */
{
	struct pbuf *p, *q;
#if MBUF_FREE_BY_ETHERNETIF
	uint8_t *data = mtod(m, uint8_t *);
#endif

#ifdef CONFIG_MUTIL_NET_STACK
	int send2xrlink_err = -1;
	struct eth_hdr *eth_hdr = (struct eth_hdr *)data;
	uint16_t type = lwip_ntohs(eth_hdr->type);
#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
	int rxq_remain = xr_rxq_queued_num();
#else
	int rxq_remain = 0;
#endif
	uint8_t mbuf_data = 0;
	uint8_t *xrlink_data = data;
	uint32_t sys_pm_status = 1;

	if (!g_dbg_disable_tx2host) {
#ifdef XRLINK_RX_DATA_USE_MBUF
		mbuf_data = 1;
		xrlink_data = (void *)m;
#endif
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
		sys_pm_status = (pm_state_get() == PM_STATUS_RUNNING) ? 1 : 0;
#endif
		if (type == ETHTYPE_ARP) {
			struct etharp_hdr *arp_hdr;

			arp_hdr = (struct etharp_hdr *)((char *)data + sizeof(struct eth_hdr));

			uint16_t arp_opcode = lwip_ntohs(arp_hdr->opcode);

			if (arp_opcode == ARP_REPLY && sys_pm_status) {
#ifdef CONFIG_STA_SOFTAP_COEXIST
				send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, nif, mbuf_data);
#else
				send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, mbuf_data);
#endif
			}
		} else if (type == ETHTYPE_IP) {
			struct ip_hdr *ip4_hdr;
			ip4_hdr = (struct ip_hdr *)((char *)data + sizeof(struct eth_hdr));

			uint8_t ip_header_len = IPH_HL(ip4_hdr) * 4;
			uint8_t *ip_payload = ((uint8_t *)data + sizeof(struct eth_hdr) + ip_header_len);
			uint8_t *ip_potocol = ((uint8_t *)data + sizeof(struct eth_hdr) + 9);//icmp=1,igmp=2,ip=4,tcp=6,egp=8,igp=9,udp=17
			uint16_t dst_port = (*(ip_payload + 2) << 8) + *(ip_payload + 3);
			uint16_t src_port = ((*ip_payload) << 8) + (*(ip_payload + 1));

			if (*ip_potocol == ICMP_POTOCOL_TYPE ) {//icmp
				if (sys_pm_status) {
#ifdef CONFIG_STA_SOFTAP_COEXIST
					send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, nif, mbuf_data);
#else
					send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, mbuf_data);
#endif
#if MBUF_FREE_BY_ETHERNETIF
					if (!mbuf_data || (mbuf_data && send2xrlink_err))
						mb_free(m);
#endif
					return (send2xrlink_err ? ERR_MEM : ERR_OK);
				}
			} else if (src_port == DNS_PORT || src_port == SNTP_PORT) {//dns & sntp packet type
				if (sys_pm_status) {
#ifdef CONFIG_STA_SOFTAP_COEXIST
					send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, nif, mbuf_data);
#else
					send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, mbuf_data);
#endif
				}
			} else if ((dst_port != DHCP_PORT_CLIENT) && (dst_port != DHCP_PORT_SERVER)
					&& (!(dst_port >= CONFIG_HOST_PORT_RANGE_START && dst_port
							<= CONFIG_HOST_PORT_RANGE_END))) {
				if (sys_pm_status) {
#ifdef CONFIG_STA_SOFTAP_COEXIST
					send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, nif, mbuf_data);
#else
					send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, mbuf_data);
#endif
				}
#if MBUF_FREE_BY_ETHERNETIF
				if (!mbuf_data || (mbuf_data && send2xrlink_err))
					mb_free(m);
#endif
				return (send2xrlink_err ? ERR_MEM : ERR_OK);
			}
		}
#if LWIP_IPV6
		else if (type == ETHTYPE_IPV6) {
			uint8_t *next_header = ((uint8_t *)data + sizeof(struct eth_hdr) + 6);
			uint8_t *ip_payload = ((uint8_t *)data + sizeof(struct eth_hdr) + 40);
			uint16_t dst_port = (*(ip_payload + 2) << 8) + *(ip_payload + 3);

			if (*next_header == 58 && sys_pm_status) {
				//for icmpv6
#ifdef CONFIG_STA_SOFTAP_COEXIST
				send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, nif, mbuf_data);
#else
				send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, mbuf_data);
#endif
				if (send2xrlink_err) {
#if MBUF_FREE_BY_ETHERNETIF
					if (!mbuf_data || (mbuf_data && send2xrlink_err))
						mb_free(m);
#endif
					return (send2xrlink_err ? ERR_MEM : ERR_OK);
				}
			} else if ((*next_header == 6 || *next_header == 17) && sys_pm_status) {
				//for 6=tcp 17=udp
				if (!(dst_port != DHCP_PORT_CLIENT) && (dst_port != DHCP_PORT_SERVER)
						&& (!(dst_port >= CONFIG_HOST_PORT_RANGE_START && dst_port
								<= CONFIG_HOST_PORT_RANGE_END))) {
#ifdef CONFIG_STA_SOFTAP_COEXIST
					send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, nif, mbuf_data);
#else
					send2xrlink_err = xrlink_net_to_host(xrlink_data, len, rxq_remain, mbuf_data);
#endif
#if MBUF_FREE_BY_ETHERNETIF
					if (!mbuf_data || (mbuf_data && send2xrlink_err))
						mb_free(m);
#endif
					return (send2xrlink_err ? ERR_MEM : ERR_OK);
				}
			}
		}
#endif
#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
		if (rxq_remain == 0) {
			// flush buffer
#ifdef CONFIG_STA_SOFTAP_COEXIST
			xrlink_net_to_host(NULL, 0, rxq_remain, nif, 0);
#else
			xrlink_net_to_host(NULL, 0, rxq_remain, 0);
#endif
		}
#endif
	}

#endif /* CONFIG_MUTIL_NET_STACK */

#if ETH_PAD_SIZE
	len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif
	/* We allocate a pbuf chain of pbufs from the pool. */
	p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
	if (p != NULL) {
#if ETH_PAD_SIZE
		pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif
		/* We iterate over the pbuf chain until we have read the entire packet into the pbuf. */
		for (q = p; q != NULL; q = q->next) {
			/* Read enough bytes to fill this pbuf in the chain. The available data
			 * in the pbuf is given by the q->len variable.
			 * This does not necessarily have to be a memcpy, you can also preallocate
			 * pbufs for a DMA-enabled MAC and after receiving truncate it to the
			 * actually received size. In this case, ensure the tot_len member of the
			 * pbuf is the sum of the chained pbuf len members.
			 */
			memcpy(q->payload, data, q->len);
			data += q->len;
		}
#if MBUF_FREE_BY_ETHERNETIF
#ifdef CONFIG_MUTIL_NET_STACK
	if (!mbuf_data || (mbuf_data && send2xrlink_err))
#endif
		mb_free(m);
#endif
	}
	return ethernetif_input(nif, p);
}
#endif /* (LWIP_MBUF_SUPPORT == 0) */

static err_t ethernetif_hw_init(struct netif *nif, enum wlan_mode mode)
{
	char name[4];
	name[0] = nif->name[0];
	name[1] = nif->name[1];
	name[2] = nif->num + '0';
	name[3] = '\0';

	nif->state = wlan_if_create(mode, nif, name);
	if (nif->state == NULL) {
		ETH_ERR("wlan interface create failed\n");
		return ERR_IF;
	}

	if (wlan_get_mac_addr(nif, nif->hwaddr, ETHARP_HWADDR_LEN) != ETHARP_HWADDR_LEN) {
		ETH_DBG("get mac addr failed\n");
		wlan_if_delete(nif->state);
		nif->state = NULL;
		return ERR_IF;
	}
	return ERR_OK;
}

static void ethernetif_hw_deinit(struct netif *nif)
{
	wlan_if_delete(nif->state);
}

static err_t ethernetif_init(struct netif *nif, enum wlan_mode mode)
{
	if (mode == WLAN_MODE_STA || mode == WLAN_MODE_HOSTAP) {
//		nif->input = tcpip_input;
#ifdef CONFIG_LWIP_V1
		nif->output = ethernetif_output;
#else
  #if LWIP_IPV4
		nif->output = ethernetif_ip4output;
  #endif
  #if LWIP_IPV6
		nif->output_ip6 = ethernetif_ip6output;
  #endif
#endif
		nif->linkoutput = ethernetif_linkoutput;
	} else if (mode == WLAN_MODE_MONITOR) {
//		nif->input = tcpip_null_input;
#ifdef CONFIG_LWIP_V1
		nif->output = ethernetif_null_output;
#else
  #if LWIP_IPV4
	      	nif->output = ethernetif_null_ip4output;
  #endif
  #if LWIP_IPV6
		nif->output_ip6 = ethernetif_null_ip6output;
  #endif
#endif
		nif->linkoutput = ethernetif_null_linkoutput;
	} else {
		ETH_ERR("mode %d\n", mode);
		return ERR_ARG;
	}

#if LWIP_NETIF_HOSTNAME
	/* Initialize interface hostname */
	nif->hostname = g_netif_hostname;
#endif /* LWIP_NETIF_HOSTNAME */

	/*
	 * Initialize the snmp variables and counters inside the struct netif.
	 * The last argument should be replaced with your link speed, in units
	 * of bits per second.
	 */
	NETIF_INIT_SNMP(nif, snmp_ifType_ethernet_csmacd, NETIF_LINK_SPEED_BPS);

	nif->name[0] = 'e';
	nif->name[1] = 'n';
	nif->num = mode + 1; // modify netif number: en1 for sta, en2 for hostap
	nif->mtu = ETHER_MTU_MAX;
	nif->hwaddr_len = ETHARP_HWADDR_LEN;
	nif->flags |= NETIF_ATTACH_FLAGS;

#if (!defined(CONFIG_LWIP_V1) && LWIP_IPV6 && LWIP_IPV6_MLD)
	/*
	 * For hardware/netifs that implement MAC filtering.
	 * All-nodes link-local is handled by default, so we must let
	 * the hardware know to allow multicast packets in.
	 * Should set mld_mac_filter previously.
	 */
	if (nif->mld_mac_filter != NULL) {
		ip6_addr_t ip6_allnodes_ll;
		ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
		nif->mld_mac_filter(nif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
	}
#endif /* (!defined(CONFIG_LWIP_V1) && LWIP_IPV6 && LWIP_IPV6_MLD) */

	/* initialize the hardware */
	return ethernetif_hw_init(nif, mode);
}

static err_t ethernetif_sta_init(struct netif *nif)
{
	return ethernetif_init(nif, WLAN_MODE_STA);
}

static err_t ethernetif_hostap_init(struct netif *nif)
{
	return ethernetif_init(nif, WLAN_MODE_HOSTAP);
}

static err_t ethernetif_monitor_init(struct netif *nif)
{
	return ethernetif_init(nif, WLAN_MODE_MONITOR);
}

struct netif *ethernetif_create(enum wlan_mode mode)
{
	netif_init_fn init_fn;
	netif_input_fn input_fn;
	struct netif *nif;

	if (mode == WLAN_MODE_STA) {
		init_fn = ethernetif_sta_init;
		input_fn = tcpip_input;
	} else if (mode == WLAN_MODE_HOSTAP) {
		init_fn = ethernetif_hostap_init;
		input_fn = tcpip_input;
	} else if (mode == WLAN_MODE_MONITOR) {
		init_fn = ethernetif_monitor_init;
		input_fn = tcpip_null_input;
	} else {
		ETH_ERR("mode %d\n", mode);
		return NULL;
	}

#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (mode == WLAN_MODE_HOSTAP) {
		if (g_eth_ap_netif != NULL) {
			ETH_ERR("g_eth_ap_netif is not empty!\n");
			return NULL;
		}
		g_eth_ap_netif = (struct ethernetif *)malloc(sizeof(struct ethernetif));
		if (!g_eth_ap_netif) {
			ETH_ERR("fail to malloc g_eth_ap_netif!\n");
			return NULL;
		}
		nif = ethernetif2netif(g_eth_ap_netif);
		memset(nif, 0, sizeof(*nif));
		g_eth_ap_netif->mode = mode;
	} else {
		if (g_eth_netif != NULL) {
			ETH_ERR("g_eth_netif is not empty!\n");
			return NULL;
		}
		g_eth_netif = (struct ethernetif *)malloc(sizeof(struct ethernetif));
		if (!g_eth_netif) {
			ETH_ERR("fail to malloc g_eth_netif!\n");
			return NULL;
		}
		nif = ethernetif2netif(g_eth_netif);
		memset(nif, 0, sizeof(*nif));
		g_eth_netif->mode = mode;
	}
#else
	nif = ethernetif2netif(&g_eth_netif);
	memset(nif, 0, sizeof(*nif));
	g_eth_netif.mode = mode;
#endif
	set_netif(mode, nif);

	/* add netif */
#if (defined(CONFIG_LWIP_V1) || LWIP_IPV4)
	netifapi_netif_add(nif, NULL, NULL, NULL, NULL, init_fn, input_fn);
#else
	netifapi_netif_add(nif, NULL, init_fn, input_fn);
#endif
#ifdef CONFIG_STA_SOFTAP_COEXIST
	/* need ensure? */
#else
	netifapi_netif_set_default(nif);
#endif

#ifndef CONFIG_DRIVER_V821
	netif_set_up(nif);
	netif_set_link_up(nif);
#endif
	return nif;

}

void ethernetif_delete(struct netif *nif)
{
	/* remove netif from LwIP stack */
	netifapi_dhcp_stop(nif);
	netifapi_netif_common(nif, dhcp_cleanup, NULL);
	netifapi_netif_remove(nif);
	nif->flags &= ~NETIF_ATTACH_FLAGS;
	ethernetif_hw_deinit(nif);
#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (nif == ethernetif2netif(g_eth_netif)) {
		set_netif(g_eth_netif->mode, NULL);
		free(g_eth_netif);
		g_eth_netif = NULL;
	} else if (nif == ethernetif2netif(g_eth_ap_netif)) {
		set_netif(g_eth_ap_netif->mode, NULL);
		free(g_eth_ap_netif);
		g_eth_ap_netif = NULL;
	}
#else
	if (nif == ethernetif2netif(&g_eth_netif)) {
		set_netif(g_eth_netif.mode, NULL);
		g_eth_netif.mode = WLAN_MODE_INVALID;
	}
#endif
}

enum wlan_mode ethernetif_get_mode(struct netif *nif)
{
#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (nif == ethernetif2netif(g_eth_netif)) {
		return g_eth_netif->mode;
	} else if (nif == ethernetif2netif(g_eth_ap_netif)) {
		return g_eth_ap_netif->mode;
#else
	if (nif == ethernetif2netif(&g_eth_netif)) {
		return g_eth_netif.mode;
#endif
	} else {
		return WLAN_MODE_INVALID;
	}
}

void *ethernetif_get_state(struct netif *nif)
{
	return (nif ? nif->state : NULL);
}

struct netif *ethernetif_get_netif(enum wlan_mode mode)
{
#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (mode == WLAN_MODE_HOSTAP) {
		return ethernetif2netif(g_eth_ap_netif);
	} else if (mode == WLAN_MODE_STA || mode == WLAN_MODE_MONITOR) {
		return ethernetif2netif(g_eth_netif);
	}
	return NULL;
#else
	if ((mode == WLAN_MODE_NONE && g_eth_netif.mode < WLAN_MODE_NUM) ||
	    (mode != WLAN_MODE_NONE && mode == g_eth_netif.mode)) {
		return ethernetif2netif(&g_eth_netif);
	}
	return NULL;
#endif
}

#ifdef CONFIG_STA_SOFTAP_COEXIST
int ethernetif_set_state(struct netif *nif, void *ifp)
{
	if (!nif)
		return -1;
	nif->state = ifp;
	return 0;
}
#endif

#ifdef CONFIG_MUTIL_NET_STACK
struct mbuf *eth_raw_to_mbuf(uint8_t *buff, uint16_t len)
{
	struct mbuf *m;
	uint8_t *data;
	int32_t left;

	/* get a mbuf */
	m = mb_get(len, 1 | MBUF_GET_FLAG_LIMIT_TX);
	if (m == NULL) {
		return NULL;
	}

	/* copy all data to mbuf */
	data = mtod(m, uint8_t *);
	left = m->m_len;
	if (left >= len) {
		memcpy(data, buff, len);
	} else {
		ETH_ERR("left %d, total %u\n", left, len);
		mb_free(m);
		return NULL;
	}
	return m;
}

#ifdef CONFIG_XRADIO_RPBUF_PERF_TEST
int xradio_rpbuf_rx_perf_test_callback(struct mbuf *m, uint16_t len);
#endif
// ret: -2:linkoutput failed; -1:netif not ready, uplayer need to free mbuf.
int ethernetif_host_to_wlan(uint8_t *buff, uint16_t len
#ifdef CONFIG_STA_SOFTAP_COEXIST
	                                , enum wlan_mode mode
#endif
	                               )
{
	struct mbuf *m = (void *)buff;
	int ret;
	struct netif *nif;

	ETH_DBG("ethernetif_host_to_wlan:len=%d\n", len);
#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (WLAN_MODE_HOSTAP == mode)
		nif = &g_eth_ap_netif->nif;
	else
		nif = &g_eth_netif->nif;
#else
	nif = &g_eth_netif.nif;
#endif

#ifdef CONFIG_XRADIO_RPBUF_PERF_TEST
#else
	if ((ethernetif_get_state(nif) == NULL) || (!netif_is_link_up(nif))) {
		ETH_WRN("e-h2w: eth state null or down.\n");
		return -1;
	}
#endif

#ifdef CONFIG_XRADIO_RPBUF_PERF_TEST
	if (xradio_rpbuf_rx_perf_test_callback(m, len))
		return 0;
#endif
	ret = wlan_linkoutput(nif, m);
	if (ret != 0) {
		ETH_WRN("e-h2w:linkoutput failed (%d)\n", ret);
		return -2;
	}

	return 0;
}
#endif /* CONFIG_MUTIL_NET_STACK */
