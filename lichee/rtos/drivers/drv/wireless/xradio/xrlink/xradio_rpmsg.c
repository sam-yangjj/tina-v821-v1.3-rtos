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
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <aw_list.h>
#include <hal_mutex.h>
#include <hal_mem.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/rpmsg_master.h>

#include "wlan.h"
#include "txrx.h"
#include "xrlink.h"
#include "xradio_rpmsg.h"
#include "command.h"

#define XRLINK_DEBUG           0
#define XRADIO_RPMSG_NAME      "xrlink_rpmsg"
#define XRADIO_RPMSG_ID        0
#define XRADIO_RPMSG_DATA_LEN  496

#define rpmsg_dump(str, data, len)       //data_hex_dump(str, 16, data, len)
#define rpmsg_err_dump(str, data, len)   data_hex_dump(str, 16, data, len)

struct rpmsg_xradio_private {
	uint8_t rpmsg_id;
	uint8_t is_unbound;
	struct rpmsg_endpoint *ept;
};

static struct rpmsg_xradio_private rpmsg_xr = {
	.rpmsg_id = XRADIO_RPMSG_ID,
	.ept = NULL,
	.is_unbound = 0,
};

int rpmsg_xradio_transmit(void *data, uint32_t len)
{
	int ret;
	struct xradio_hdr *hdr;
	ptc_cmdpl_t* pro;

	if (!rpmsg_xr.ept) {
		SLAVE_WLAN_ERR("rpmsg not ready\n");
		return -1;
	}

	if (len > XRADIO_RPMSG_DATA_LEN) {
		SLAVE_WLAN_ERR("rpmsg_send too long: %d\n", len);
		return -2;
	}

	hdr = data;
	pro = (void *)hdr->payload;
	SLAVE_WLAN_DBG("xr type %d cmd type %d\n", hdr->type, pro->type);
	rpmsg_dump("txmsg: ", data, (XR_HDR_SZ + hdr->len) > 32 ? 32 : (XR_HDR_SZ + hdr->len));

	ret = openamp_rpmsg_send(rpmsg_xr.ept, data, len);
	if (ret < 0) {
		SLAVE_WLAN_ERR("Failed to send data %d\n", ret);
	}
	return ret;
}

static int rpmsg_xradio_ept_callback(struct rpmsg_endpoint *ept, void *data, size_t len,
                                             uint32_t src, void *priv)
{
	xrlink_tx_com_cb(data, len, XRADIO_RPMSG_DATA_LEN);

	return 0;
}

static void rpmsg_xradio_unbind_callback(struct rpmsg_endpoint *ept)
{
	rpmsg_xr.is_unbound = 1;

	SLAVE_WLAN_SYSLOG("Remote endpoint is destroyed\n");
}

int rpmsg_xradio_create()
{
	uint32_t src_addr = RPMSG_ADDR_ANY;
	uint32_t dst_addr = RPMSG_ADDR_ANY;

	SLAVE_WLAN_SYSLOG("create %s rpmsg endpoint\r\n", XRADIO_RPMSG_NAME);

	rpmsg_xr.ept = openamp_ept_open(XRADIO_RPMSG_NAME, rpmsg_xr.rpmsg_id, src_addr, dst_addr,
					&rpmsg_xr, rpmsg_xradio_ept_callback, rpmsg_xradio_unbind_callback);
	if (!rpmsg_xr.ept) {
		SLAVE_WLAN_ERR("Failed to Create Endpoint\r\n");
		return -1;
	}
	rpmsg_xr.is_unbound = 0;

	SLAVE_WLAN_SYSLOG("creat rpmsg succeed\n");

 	return 0;
}

int rpmsg_xradio_init(void)
{
	int ret = 0;

	ret = rpmsg_xradio_create();
	if (ret < 0) {
		SLAVE_WLAN_ERR("rpmsg_xradio_create failed\n");
		return ret;
	}

	SLAVE_WLAN_SYSLOG("rpmsg_xradio_create success\n");

	return ret;
}

void rpmsg_xradio_deinit(void)
{
	SLAVE_WLAN_SYSLOG("rpmsg_xradio_deinit\n");
	if (rpmsg_xr.ept) {
		openamp_ept_close(rpmsg_xr.ept);
		rpmsg_xr.ept = NULL;
	}
}
