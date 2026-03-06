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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <console.h>
#include "lwip/netdb.h"
#include "ping.h"

int ping_get_host_by_name(char *name, unsigned int *address)
{
	struct hostent *host_entry;
	host_entry = gethostbyname(name);
	if(host_entry) {
		*(address) = *((u_long*)host_entry->h_addr_list[0]);
		return 0; // OK
	} else {
		return 1; // Error
	}
}

int cmd_ping(int argc, char ** argv)
{
	int ch;
	int duration = 0;
	int pkt_len = 0;
	struct ping_data pdata;
	unsigned int address = 0;

	if (argc < 2) {
		printf("please input peer ip address!\n");
		goto help;
	}

	memset((void*) &pdata, 0, sizeof(pdata));
	pdata.count = 10;
	pdata.data_long = 64;

	if (ping_get_host_by_name(argv[1], &address) != 0) {
		printf("invalid ping host.\n");
		goto help;
	}

#ifdef CONFIG_LWIP_V1
	ip4_addr_set_u32(&pdata.sin_addr, address);
#elif LWIP_IPV4 /* now only for IPv4 */
	ip_addr_set_ip4_u32(&pdata.sin_addr, address);
#else
	#error "IPv4 not support!"
#endif

	while((ch = getopt(argc, argv, "t:s:h")) != -1) {
		switch(ch) {
		case 't':
			if (argc < 4) {
				printf("missing parameters!\n");
				goto help;
			}

			duration = atoi(optarg);
			if (duration < 10)
				pdata.count = 10;
			else
				pdata.count = duration;

			break;
		case 's':
			pkt_len = atoi(optarg);
			if (pkt_len < 64)
				pdata.data_long = 64;
			else if (pkt_len > 0xffff)
				pdata.data_long = 0xffff;
			else
				pdata.data_long = pkt_len;

			break;
		case 'h':
			goto help;
			break;
		default:
			printf("invalid parameters!\n");
			goto help;
		}
	}

	if (ping(&pdata) == 0)
		return 0;
	else
		return -1;

help:
	printf("Format:\n");
	printf("\tping [addr] [-t <duration>] [-s <packet size>]\n\n");
	printf("Options:\n");
	printf("\taddr: specify the peer ip address\n");
	printf("\t-t: set the duration of ping execute\n");
	printf("\t-s: set the size of packet\n\n");

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_ping, ping, network ping test);
