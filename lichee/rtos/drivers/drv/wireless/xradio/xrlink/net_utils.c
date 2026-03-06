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
#include <stdbool.h>
#include <sys/endian.h>

#define ETHTYPE_ARP     0x0806U
#define ETHTYPE_IP      0x0800U
#define IPPROTO_IP      0
#define IPPROTO_ICMP    1      /* Internet Control Message Protocol */
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

#define IPPROTO_IGMP    2		/* Internet Group Management Protocol	*/
/*for ip data*/
#define PF_TCP          0x0010
#define PF_UDP          0x0020
#define PF_DHCP         0x0040
#define PF_ICMP         0x0080

#define PF_IPADDR       0x2000    /*ip address of ip packets.*/
#define PF_UNKNWN       0x4000    /*print frames of unknown flag.*/
#define PF_RX           0x8000    /*0:TX, 1:RX. So, need to add PF_RX in Rx path.*/

#define PT_MSG_PUT(f, ...) do { \
	if (flags&f)	\
		proto_msg += sprintf(proto_msg, __VA_ARGS__); \
	} while (0)

#define IS_PROTO_PRINT (proto_msg != (char *)&protobuf[0])

#define LLC_LEN       14
#define LLC_TYPE_OFF  12	/*Ether type offset*/
#define IP_PROTO_OFF  9		/*protocol offset*/
#define IP_S_ADD_OFF  12
#define IP_D_ADD_OFF  16
#define UDP_LEN       8
/*DHCP*/
#define DHCP_BOOTP_C  68
#define DHCP_BOOTP_S  67
#define UDP_BOOTP_LEN 236	/*exclude "Options:64"*/
#define BOOTP_OPS_LEN 64
#define DHCP_MAGIC    0x63825363
#define DHCP_DISCOVER 0x01
#define DHCP_OFFER    0x02
#define DHCP_REQUEST  0x03
#define DHCP_DECLINE  0x04
#define DHCP_ACK      0x05
#define DHCP_NACK     0x06
#define DHCP_RELEASE  0x07
/*ARP*/
#define ARP_REQUEST     0x0001
#define ARP_RESPONSE    0x0002
#define ARP_TYPE_OFFSET 6


/*IP/IPV6/ARP layer...*/
bool is_ip(uint8_t *eth_data)
{
	/* 0x0800 */
	return (bool)(*(uint16_t *)(eth_data+LLC_TYPE_OFF) == cpu_to_be16(ETHTYPE_IP));
}

bool is_arp(uint8_t *eth_data)
{
	/* 0x0806 */
	return (bool)(*(uint16_t *)(eth_data+LLC_TYPE_OFF) == cpu_to_be16(ETHTYPE_ARP));
}

bool is_tcp(uint8_t *eth_data)
{
	return (bool) (eth_data[LLC_LEN + IP_PROTO_OFF] == IPPROTO_TCP);
}

bool is_udp(uint8_t *eth_data)
{
	return (bool)(eth_data[LLC_LEN+IP_PROTO_OFF] == IPPROTO_UDP);
}

bool is_icmp(uint8_t *eth_data)
{
	return (bool) (eth_data[LLC_LEN + IP_PROTO_OFF] == IPPROTO_ICMP);
}

bool is_igmp(uint8_t *eth_data)
{
	return (bool) (eth_data[LLC_LEN + IP_PROTO_OFF] == IPPROTO_IGMP);
}

bool is_dhcp(uint8_t *eth_data)
{
	uint8_t *ip_hdr = eth_data + LLC_LEN;
	if (!is_ip(eth_data))
		return (bool) 0;
	if (ip_hdr[IP_PROTO_OFF] == IPPROTO_UDP) {
		uint8_t *udp_hdr = ip_hdr + ((ip_hdr[0] & 0xf) << 2);	/*ihl:words*/
		/* DHCP client or DHCP server*/
		return (bool)((((udp_hdr[0]<<8)|udp_hdr[1]) == DHCP_BOOTP_C) ||
			      (((udp_hdr[0]<<8)|udp_hdr[1]) == DHCP_BOOTP_S));
	}
	return (bool)0;
}

void net_parse_frame(const char *func, uint8_t *eth_data, uint16_t len,
		uint8_t tx, uint32_t flags)
{
	char protobuf[512] = { 0 };
	char *proto_msg = &protobuf[0];
	if (len < LLC_TYPE_OFF)
		return;

	if (is_ip(eth_data)) {
		uint8_t *ip_hdr = eth_data + LLC_LEN;
		uint8_t *ipaddr_s = ip_hdr + IP_S_ADD_OFF;
		uint8_t *ipaddr_d = ip_hdr + IP_D_ADD_OFF;
		uint8_t *proto_hdr = ip_hdr + ((ip_hdr[0] & 0xf) << 2);	/*ihl:words*/

		if (is_tcp(eth_data)) {
			PT_MSG_PUT(PF_TCP,
				   "TCP%s%s, src=%d, dest=%d, seq=0x%08x, ack=0x%08x",
			    (proto_hdr[13]&0x01) ? "(S)" : "",
			    (proto_hdr[13]&0x02) ? "(F)" : "",
			    (proto_hdr[0]<<8)  | proto_hdr[1],
			    (proto_hdr[2]<<8)  | proto_hdr[3],
			    (proto_hdr[4]<<24) | (proto_hdr[5]<<16) |
			    (proto_hdr[6]<<8)  | proto_hdr[7],
			    (proto_hdr[8]<<24) | (proto_hdr[9]<<16) |
			    (proto_hdr[10]<<8) | proto_hdr[11]);

		} else if (is_udp(eth_data)) {
			if (is_dhcp(eth_data)) {
				uint8_t Options_len = BOOTP_OPS_LEN;
				uint32_t dhcp_magic  = cpu_to_be32(DHCP_MAGIC);
				uint8_t *dhcphdr = proto_hdr + UDP_LEN+UDP_BOOTP_LEN;
				while (Options_len) {
					if (*(uint32_t *)dhcphdr == dhcp_magic)
						break;
					dhcphdr++;
					Options_len--;
				}
				PT_MSG_PUT(PF_DHCP, "DHCP, Opt=%d, MsgType=%d",
					   *(dhcphdr+4), *(dhcphdr+6));
			} else {
				PT_MSG_PUT(PF_UDP, "UDP, source=%d, dest=%d",
					  (proto_hdr[0]<<8) | proto_hdr[1],
					  (proto_hdr[2]<<8) | proto_hdr[3]);
			}
		} else if (is_icmp(eth_data)) {
			PT_MSG_PUT(PF_ICMP, "ICMP%s%s, Seq=%d",
				   (8 == proto_hdr[0]) ? "(ping)"  : "",
				   (0 == proto_hdr[0]) ? "(reply)" : "",
					(proto_hdr[6]<<8) | proto_hdr[7]);
		} else if (is_igmp(eth_data)) {
			PT_MSG_PUT(PF_UNKNWN, "IGMP, type=0x%x", proto_hdr[0]);
		} else {
			PT_MSG_PUT(PF_UNKNWN, "unknown IP type=%d",
				   *(ip_hdr + IP_PROTO_OFF));
		}
		if (IS_PROTO_PRINT) {
			PT_MSG_PUT(PF_IPADDR, "-%d.%d.%d.%d(s)", \
				   ipaddr_s[0], ipaddr_s[1],
				   ipaddr_s[2], ipaddr_s[3]);
			PT_MSG_PUT(PF_IPADDR, "-%d.%d.%d.%d(d)", \
				   ipaddr_d[0], ipaddr_d[1],
				   ipaddr_d[2], ipaddr_d[3]);
		}
		/* printf("[%s] %s--%s\n", func, tx ? "TX-" : "RX-", protobuf); */
	}
}
