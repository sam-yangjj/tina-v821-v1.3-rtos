/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Filename: tcpip_adapter.c
 * description: tcp ip stack init
 * Created: 2019.07.22
 * Author:laumy
 */

#include <stdio.h>
#include <stdbool.h>
#include <lwip/tcpip.h>
#include <lwip/netif.h>
#include <stdlib.h>
#include <string.h>

#include "tcpip_adapter.h"
#include "console.h"

#define DEBUG(fmt,args...) do { printf("tcpip_adapter[%s,%d]"fmt,__func__,__LINE__,##args); }while(0)
#define INFO(fmt,args...) do { printf("tcpip_adapter[%s,%d]"fmt,__func__,__LINE__,##args); }while(0)
#define ERROR(fmt,args...) do { printf("tcpip_adapter[%s,%d]"fmt,__func__,__LINE__,##args); }while(0)
#if 0
static struct netif net_if;
#endif
struct netif *aw_netif[IF_MAX + 1];

int8_t netif_ifconfig(void)
{
	int i;
	for(i=0; i<IF_MAX; i++) {
		if(aw_netif[i]) {
			printf("%c%c%u\t%s\n \
\t%s%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F"\n \
\t%s%"U16_F".%"U16_F".%"U16_F".%"U16_F"\n \
\t%s%"U16_F".%"U16_F".%"U16_F".%"U16_F"\n \
\t%s%"U16_F".%"U16_F".%"U16_F".%"U16_F"\n \
\t%s %s %s %s%d\n",
				aw_netif[i]->name[0],aw_netif[i]->name[1],aw_netif[i]->num,
				"Link encap:Ethernet",
				"HWaddr ",
				aw_netif[i]->hwaddr[0], aw_netif[i]->hwaddr[1], aw_netif[i]->hwaddr[2],
				aw_netif[i]->hwaddr[3], aw_netif[i]->hwaddr[4], aw_netif[i]->hwaddr[5],
				"inet addr ",
				ip4_addr1_16(netif_ip4_addr(aw_netif[i])),
				ip4_addr2_16(netif_ip4_addr(aw_netif[i])),
				ip4_addr3_16(netif_ip4_addr(aw_netif[i])),
				ip4_addr4_16(netif_ip4_addr(aw_netif[i])),
				"gw addr ",
			ip4_addr1_16(netif_ip4_gw(aw_netif[i])),
			ip4_addr2_16(netif_ip4_gw(aw_netif[i])),
			ip4_addr3_16(netif_ip4_gw(aw_netif[i])),
			ip4_addr4_16(netif_ip4_gw(aw_netif[i])),
				"netmask ",
				ip4_addr1_16(netif_ip4_netmask(aw_netif[i])),
				ip4_addr2_16(netif_ip4_netmask(aw_netif[i])),
				ip4_addr3_16(netif_ip4_netmask(aw_netif[i])),
				ip4_addr4_16(netif_ip4_netmask(aw_netif[i])),
				((aw_netif[i]->flags & NETIF_FLAG_UP) == NETIF_FLAG_UP) ? "UP" : "DOWN",
				((aw_netif[i]->flags & NETIF_FLAG_BROADCAST) == NETIF_FLAG_BROADCAST) ? "BROADCAST" : " ",
				((aw_netif[i]->flags & NETIF_FLAG_LINK_UP) == NETIF_FLAG_LINK_UP) ? "LINK_UP" : "LINK_DOWN",
				" mtu: ",aw_netif[i]->mtu);
		}
	}

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(netif_ifconfig,ifconfig, Console network config Command);

struct netif* get_netif(if_type_t mode)
{
	if(aw_netif[mode] != NULL) {
		return aw_netif[mode];
	}
	ERROR("net interface not init.\n");
	return NULL;
}

void set_netif(if_type_t mode, struct netif *netif)
{
    if (mode >= IF_MAX) {
        printf("unsupported interface mode: %d\n", mode);
        return;
    }

    aw_netif[mode] = netif;
}

static bool tcpip_enable = false;

int8_t tcpip_stack_init(void)
{
	if (tcpip_enable == false) {
		tcpip_init(NULL,NULL);
		tcpip_enable = true;
	}
	return 0;
}

int get_mac_hwaddr(unsigned char *addr, int len)
{
    struct netif* nif = get_netif(MODE_AP);

    if (!nif) {
        return -1;
    }

#ifndef MIN
#define MIN(x, y)   ((x) < (y) ? (x) : (y))
#endif
    memcpy(addr, nif->hwaddr, MIN(sizeof(nif->hwaddr), len));
    return 1;
}

void cmd_tcpip_init(void)
{
	tcpip_stack_init();
	printf("======%s->%d tcpip init successful!======\n", __func__, __LINE__);
}

FINSH_FUNCTION_EXPORT_CMD(cmd_tcpip_init,tcpip, Console tcpip init test Command);
