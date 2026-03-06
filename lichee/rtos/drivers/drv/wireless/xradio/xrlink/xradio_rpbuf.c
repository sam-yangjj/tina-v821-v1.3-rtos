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
#include <rpbuf.h>
#include <hal_cache.h>

#include "kernel/os/os.h"
#include "sys/param.h"
#include "wlan.h"
#include "txrx.h"
#include "xrlink.h"
#include "xradio_rpbuf.h"
#include "command.h"

#define XRLINK_DEBUG                    0
#define XR_RPBUF_CACHE_ABLED            1//1

#define rp_malloc                        malloc
#define rp_free                          free

#define RPBUF_FLUSH_DATA_LEN_ALIGN       32
#define RPBUF_FLUSH_DATA_LEN(l)          roundup(l, RPBUF_FLUSH_DATA_LEN_ALIGN)
#define rpbuf_dump(str, data, len)       //data_hex_dump(str, 16, data, len)
#define rpbuf_err_dump(str, data, len)   data_hex_dump(str, 16, data, len)

#define RPBUF_LOG(controller_id, buf_name, buf_len, fmt, args...) \
	printf("[%d|%s|%d] " fmt, controller_id, buf_name, buf_len, ##args)

struct xrlink_rpbuf_dev {
	/* master rx, RTOS -> Linux */
	struct rpbuf_controller *ctrl_mrx;
	int len_mrx;
	struct rpbuf_buffer *buffer_mrx;
	u8 *buffer_mrx_tail;
	int buffer_mrx_used_len;
	int buffer_mrx_free_len;
	u16 buffer_mrx_agg_max;
	u16 buffer_mrx_agg_count;
	XR_OS_Mutex_t mutex_mrx;
	uint32_t mem_attr_mrx;

	/* master tx, RTOS <- Linux */
	struct rpbuf_controller *ctrl_mtx;
	int len_mtx;
	struct rpbuf_buffer *buffer_mtx;
	uint32_t mem_attr_mtx;
};

static struct xrlink_rpbuf_dev *xr_rpbuf;

static void rpbuf_xradio_buffer_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
	SLAVE_WLAN_SYSLOG("buffer \"%s\" is available\n", rpbuf_buffer_name(buffer));
}

static int rpbuf_xradio_buffer_rx_cb(struct rpbuf_buffer *buffer, void *data, int data_len, void *priv)
{
	int ret;

	ret = xrlink_data_tx_callback(data, data_len, rpbuf_buffer_len(buffer));
	if (ret < 0) {
		SLAVE_WLAN_ERR("xrlink_rx_ind failed\n");
		ret = min(RPBUF_FLUSH_DATA_LEN(XR_AGG_HDR_SZ), rpbuf_buffer_len(buffer));
	} else if (ret) {
		ret = RPBUF_FLUSH_DATA_LEN(ret);
		ret = min(RPBUF_FLUSH_DATA_LEN(ret), rpbuf_buffer_len(buffer));
	} else {
		ret = min(RPBUF_FLUSH_DATA_LEN(XR_AGG_HDR_SZ), rpbuf_buffer_len(buffer));
	}
#if XR_RPBUF_CACHE_ABLED
	hal_dcache_clean((unsigned long)data, (unsigned long)ret);
#endif
	return 0;
}

static int rpbuf_xradio_buffer_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	SLAVE_WLAN_SYSLOG("buffer \"%s\": remote buffer destroyed\n", rpbuf_buffer_name(buffer));

	return 0;
}

static const struct rpbuf_buffer_cbs rpbuf_xradio_mtx_cbs = {
	.available_cb = rpbuf_xradio_buffer_available_cb,
	.rx_cb = rpbuf_xradio_buffer_rx_cb,
	.destroyed_cb = rpbuf_xradio_buffer_destroyed_cb,
};
static const struct rpbuf_buffer_cbs rpbuf_xradio_mrx_cbs = {
	.available_cb = rpbuf_xradio_buffer_available_cb,
	.rx_cb = NULL,
	.destroyed_cb = rpbuf_xradio_buffer_destroyed_cb,
};

static int rpbuf_xradio_create(struct xrlink_rpbuf_dev *rpbuf, u8 mtx, int controller_id,
	                                 const char *name, int len)
{
	int ret;
	struct rpbuf_controller *controller = NULL;
	struct rpbuf_buffer *buffer = NULL;

	controller = rpbuf_get_controller_by_id(controller_id);
	if (!controller) {
		RPBUF_LOG(controller_id, name, len,
				"Failed to get controller%d, controller_id\n", controller_id);
		ret = -ENOENT;
		return ret;
	}

	if (mtx)
		buffer = rpbuf_alloc_buffer(controller, name, len, NULL, &rpbuf_xradio_mtx_cbs, NULL);
	else
		buffer = rpbuf_alloc_buffer(controller, name, len, NULL, &rpbuf_xradio_mrx_cbs, NULL);
	if (!buffer) {
		RPBUF_LOG(controller_id, name, len, "rpbuf_alloc_buffer failed\n");
		ret = -ENOENT;
		return ret;
	}
	rpbuf_buffer_set_sync(buffer, true);

	if (mtx) {
		rpbuf->ctrl_mtx = controller;
		rpbuf->buffer_mtx = buffer;
		rpbuf->len_mtx = rpbuf_buffer_len(buffer);
		rpbuf->buffer_mtx = buffer;
	} else {
		rpbuf->ctrl_mrx = controller;
		rpbuf->buffer_mrx = buffer;
		rpbuf->buffer_mrx_tail = rpbuf_buffer_va(buffer);
		rpbuf->buffer_mrx_used_len = 0;
		rpbuf->buffer_mrx_agg_count = 0;
		rpbuf->buffer_mrx_agg_max = RPBUF_BUFFER_MRX_AGG_MAX;
		rpbuf->buffer_mrx_free_len = rpbuf_buffer_len(buffer);
		rpbuf->len_mrx = rpbuf_buffer_len(buffer);
		SLAVE_WLAN_DBG("buf:%p len:%d rpbuf:%p\n", rpbuf->buffer_mrx_tail, rpbuf->len_mrx, rpbuf->buffer_mrx);
	}

	return 0;
}

static int rpbuf_xradio_destroy(struct xrlink_rpbuf_dev *rpbuf, u8 mtx)
{
	int ret;
	struct rpbuf_buffer *buffer = NULL;

	if (mtx)
		buffer = rpbuf->buffer_mtx;
	else
		buffer = rpbuf->buffer_mrx;

	ret = rpbuf_free_buffer(buffer);
	if (ret) {
		RPBUF_LOG(rpbuf_buffer_id(buffer),
				rpbuf_buffer_name(buffer),
				rpbuf_buffer_len(buffer),
				"ERROR: rpbuf_free_buffer failed %d\n", ret);
	}

	if (mtx)
		rpbuf->buffer_mtx = NULL;
	else
		rpbuf->buffer_mrx = NULL;

	return ret;
}

#ifdef CONFIG_XRADIO_RPBUF_PERF_TEST
#define RPBUF_AGG_PERF_DBG
#endif
#ifdef RPBUF_AGG_PERF_DBG
static int g_rpbuf_tx_cnt, g_rpbuf_agg_cnt, g_rpbuf_agg_min, g_rpbuf_agg_max;
#define RPBUF_AGG_PERF_DBG_THRESH    5000
#endif

static void rpbuf_txbuf_reset(struct xrlink_rpbuf_dev *rpbuf)
{
	struct rpbuf_buffer *buffer = rpbuf->buffer_mrx;
	struct xradio_agg_hdr *agg_hdr;

	rpbuf->buffer_mrx_tail = rpbuf_buffer_va(buffer);
	rpbuf->buffer_mrx_used_len = 0;
	rpbuf->buffer_mrx_agg_count = 0;
	rpbuf->buffer_mrx_free_len = rpbuf_buffer_len(buffer);

	// first pkt
	agg_hdr = (void *)rpbuf->buffer_mrx_tail;
	memset(agg_hdr, 0, XR_AGG_HDR_SZ);
	agg_hdr->rsp_code = XR_TXD_ST_NO_RSP;
	rpbuf->buffer_mrx_tail += XR_AGG_HDR_SZ;
	rpbuf->buffer_mrx_used_len += XR_AGG_HDR_SZ;
	SLAVE_WLAN_DBG("start:%d agg:%d, used len:%d bufsz:%d\n", agg_hdr->start_idx,
		agg_hdr->agg_cnt, rpbuf->buffer_mrx_used_len, rpbuf_buffer_len(buffer));
}

static __inline__ int rpbuf_send(struct xrlink_rpbuf_dev *rpbuf)
{
	int ret;
	struct rpbuf_buffer *buffer = rpbuf->buffer_mrx;
	struct xradio_agg_hdr *agg_hdr;
	u32 total_len = 0;
	u8 need_rsp = 0;
#if XRLINK_DEBUG
	u8 *data;
	int i;
#endif
	struct xradio_hdr *hdr;
	ptc_cmdpl_t *ptc;

#ifdef RPBUF_AGG_PERF_DBG
	g_rpbuf_tx_cnt++;
	g_rpbuf_agg_cnt += rpbuf->buffer_mrx_agg_count;
	if (rpbuf->buffer_mrx_agg_count < g_rpbuf_agg_min)
		g_rpbuf_agg_min = rpbuf->buffer_mrx_agg_count;
	else if (g_rpbuf_agg_min == 0)
		g_rpbuf_agg_min = rpbuf->buffer_mrx_agg_count;
	if (rpbuf->buffer_mrx_agg_count > g_rpbuf_agg_max)
		g_rpbuf_agg_max = rpbuf->buffer_mrx_agg_count;
	else if (g_rpbuf_agg_max == 0)
		g_rpbuf_agg_max = rpbuf->buffer_mrx_agg_count;

	if (g_rpbuf_tx_cnt >= RPBUF_AGG_PERF_DBG_THRESH) {
		int avg = g_rpbuf_agg_cnt / g_rpbuf_tx_cnt;

		SLAVE_WLAN_SYSLOG("cnt:%d agged:%d %d-%d-%d l:%d b:%d\n", g_rpbuf_tx_cnt, g_rpbuf_agg_cnt,
				g_rpbuf_agg_min, avg, g_rpbuf_agg_max, rpbuf->buffer_mrx_used_len, rpbuf_buffer_len(buffer));
		g_rpbuf_tx_cnt = g_rpbuf_agg_cnt = g_rpbuf_agg_min = g_rpbuf_agg_max = 0;
	}
#endif

	agg_hdr = (void *)rpbuf_buffer_va(buffer);
	if ((agg_hdr->agg_cnt != rpbuf->buffer_mrx_agg_count) ||
		(agg_hdr->agg_cnt > rpbuf->buffer_mrx_agg_max)) {
		SLAVE_WLAN_ERR("data err! hdr aggcnt:%d, agg cnt:%d max:%d\n",
			agg_hdr->agg_cnt, rpbuf->buffer_mrx_agg_count, rpbuf->buffer_mrx_agg_max);
		goto data_err;
	}
	if (rpbuf->buffer_mrx_used_len <= XR_AGG_HDR_SZ) {
		SLAVE_WLAN_ERR("data err! used_len too small:%d agg hdr sz:%d\n",
			rpbuf->buffer_mrx_used_len, XR_AGG_HDR_SZ);
		goto data_err;
	}
#if XRLINK_DEBUG
	data = ((u8 *)agg_hdr) + XR_AGG_HDR_SZ;
	total_len += XR_AGG_HDR_SZ;
	SLAVE_WLAN_DBG("start:%d agg:%d, used len:%d bufsz:%d\n", agg_hdr->start_idx,
		agg_hdr->agg_cnt, rpbuf->buffer_mrx_used_len, rpbuf_buffer_len(buffer));
	for (i = 0; i < agg_hdr->agg_cnt; i++) {
		hdr = (void *)data;

		SLAVE_WLAN_DBG("agg:%d/%d, xr type %d len %d\n",
			i + 1, agg_hdr->agg_cnt, hdr->type, XR_HDR_SZ + hdr->len);
		rpbuf_dump("txdata: ", data, (XR_HDR_SZ + hdr->len) > 24 ? 24 : (XR_HDR_SZ + hdr->len));

		total_len += XR_HDR_SZ + hdr->len;
		if (total_len > rpbuf_buffer_len(buffer)) {
			SLAVE_WLAN_ERR("data err! agg:%d/%d, len:%d total-len:%d bufsz:%d\n",
				i + 1, agg_hdr->agg_cnt, XR_HDR_SZ + hdr->len, total_len, rpbuf_buffer_len(buffer));

			data = (u8 *)agg_hdr + XR_AGG_HDR_SZ;
			for (i = 0; i < agg_hdr->agg_cnt; i++) {
				hdr = (void *)data;
				SLAVE_WLAN_ERR("dump: agg:%d/%d, len:%d\n",
							i + 1, agg_hdr->agg_cnt, XR_HDR_SZ + hdr->len);
				rpbuf_err_dump("data: ", data, (XR_HDR_SZ + hdr->len) > 64 ? 64 : (XR_HDR_SZ + hdr->len));
				data += XR_HDR_SZ + hdr->len;
			}
			goto data_err;
		}
		data += XR_HDR_SZ + hdr->len;
	}
	if (total_len != rpbuf->buffer_mrx_used_len) {
		SLAVE_WLAN_ERR("data err! len err, used_len:%d total_len:%d\n",
					rpbuf->buffer_mrx_used_len, total_len);
		goto data_err;
	}
#endif

	hdr = (void *)((u8 *)agg_hdr + XR_AGG_HDR_SZ);
	if (hdr->type == XR_TYPE_FW_BIN) {
		ptc = (void *)hdr->payload;
		if (ptc->type == XR_WIFI_DEV_UPDATE_FW_REQ) {
			need_rsp = 1;
		}
	}

	ret = rpbuf_transmit_buffer(buffer, 0, rpbuf->buffer_mrx_used_len);
	if (ret) {
		SLAVE_WLAN_ERR("rpbuf_transmit_buffer failed, len:%d, agg:%d, ret:%d\n",
			rpbuf->buffer_mrx_used_len, agg_hdr->agg_cnt, ret);
		ret = XR_TXD_ST_BUS_TX_FAIL;
	} else {
#ifdef RPBUF_NO_CHECK_RSP
		if ((hdr->type == XR_TYPE_DATA) || (hdr->type == XR_TYPE_DATA_AP)) {
			rpbuf_txbuf_reset(rpbuf);
			return 0;
		}
#endif
		if (need_rsp)
			total_len = RPBUF_FLUSH_DATA_LEN(rpbuf->buffer_mrx_used_len);
		else
			total_len = RPBUF_FLUSH_DATA_LEN(XR_AGG_HDR_SZ);
		total_len = min(total_len, rpbuf_buffer_len(buffer));

#if XR_RPBUF_CACHE_ABLED
		hal_dcache_invalidate((unsigned long)agg_hdr, (unsigned long)total_len);
#endif
		SLAVE_WLAN_DBG("rsp:%d flush a:%p len:%d ack:%d/%d code:%d\n", need_rsp, agg_hdr, total_len,
					agg_hdr->ack_cnt, agg_hdr->agg_cnt, agg_hdr->rsp_code);
		if (agg_hdr->rsp_code == XR_TXD_ST_SUCESS) {
			if (need_rsp) {
				xrlink_tx_com_cb(hdr, hdr->len + XR_HDR_SZ, hdr->len + XR_HDR_SZ);
			}
			rpbuf_txbuf_reset(rpbuf);
			ret = XR_TXD_ST_SUCESS;
		} else {
			ret = agg_hdr->rsp_code;
		}
	}

	return ret;
data_err:
	ret = XR_TXD_ST_DATA_ERR;
	return ret;
}

static int rpbuf_xradio_transmit(struct xrlink_rpbuf_dev *rpbuf, struct xradio_hdr hdr, void *data,
	                                   int data_len, enum xrlink_rpbuf_op op_code)
{
	int ret = 0;
	struct rpbuf_buffer *buffer;
	const char *buf_name;
	int buf_len;
	struct xradio_agg_hdr *agg_hdr;
	struct xradio_hdr *xr_hdr;

	buffer = rpbuf->buffer_mrx;
	buf_name = rpbuf_buffer_name(buffer);
	buf_len = rpbuf_buffer_len(buffer);

	if (!rpbuf_buffer_is_available(buffer)) {
		SLAVE_WLAN_ERR("buffer %s not available\n", buf_name);
		ret = -EACCES;
		return ret;
	}

	agg_hdr = rpbuf_buffer_va(buffer);
	SLAVE_WLAN_DBG("rpbuf_xradio_transmit: op %d, buf %p agg_cnt:%d %d\n", op_code, agg_hdr,
				agg_hdr->agg_cnt, rpbuf->buffer_mrx_agg_count);

	if (op_code == XR_TXBUS_OP_RESET_BUF) {
		rpbuf_txbuf_reset(rpbuf);
		return 0;
	} else if (op_code == XR_TXBUS_OP_TX_RETRY) {
		agg_hdr->start_idx = agg_hdr->ack_cnt;
		agg_hdr->rsp_code = XR_TXD_ST_NO_RSP;
		ret = rpbuf_send(rpbuf);
		if (ret)
			return ret;
	} else if (op_code == XR_TXBUS_OP_FLUSH_BUF) {
		if (rpbuf->buffer_mrx_used_len > XR_AGG_HDR_SZ) {
			SLAVE_WLAN_DBG("flush s2m buf\n");
			return rpbuf_send(rpbuf);
		} else {
			return 0;
		}
	}

	if ((rpbuf->buffer_mrx_used_len + data_len + XR_HDR_SZ) > buf_len) {
		if (rpbuf->buffer_mrx_used_len > XR_AGG_HDR_SZ) {
			SLAVE_WLAN_DBG("%s data big, send last, agg:%d, used len:%d bufsz:%d now_len:%d\n", __func__,
						rpbuf->buffer_mrx_agg_count, rpbuf->buffer_mrx_used_len, buf_len, data_len + XR_HDR_SZ);
			ret = rpbuf_send(rpbuf);
			if (ret)
				return ret;
		} else {
			SLAVE_WLAN_ERR("data to big, len:%d bufsz:%d\n", data_len + XR_HDR_SZ, rpbuf_buffer_len(buffer));
			return -EINVAL;
		}
	}
	if (!data || !data_len) {
		SLAVE_WLAN_ERR("no data, len:%d pkt:%p\n", data_len, data);
		return -EINVAL;
	}
	if ((data_len + XR_HDR_SZ) > buf_len) {
		SLAVE_WLAN_ERR("data to big, len:%d bufsz:%d\n", data_len + XR_HDR_SZ, rpbuf_buffer_len(buffer));
		return -EINVAL;
	}

	// agg data
	if ((rpbuf->buffer_mrx_used_len == 0) || (!rpbuf->buffer_mrx_tail)){
		rpbuf_txbuf_reset(rpbuf);
	}
	memcpy(rpbuf->buffer_mrx_tail, &hdr, XR_HDR_SZ);
	xr_hdr = (void *)rpbuf->buffer_mrx_tail;
	rpbuf->buffer_mrx_tail += XR_HDR_SZ;
	memcpy(rpbuf->buffer_mrx_tail, data, data_len);
	rpbuf->buffer_mrx_tail += data_len;
	rpbuf->buffer_mrx_used_len += data_len + XR_HDR_SZ;
	rpbuf->buffer_mrx_agg_count += 1;
	rpbuf->buffer_mrx_free_len = buf_len - rpbuf->buffer_mrx_used_len;
	agg_hdr->agg_cnt += 1;
	SLAVE_WLAN_DBG("agg data %d len:%d used:%d free:%d xr_type %d\n", agg_hdr->agg_cnt, data_len + XR_HDR_SZ,
				rpbuf->buffer_mrx_used_len, rpbuf->buffer_mrx_free_len, xr_hdr->type);
	rpbuf_dump("data: ", (void *)xr_hdr, (XR_HDR_SZ + xr_hdr->len) > 32 ? 32 : (XR_HDR_SZ + xr_hdr->len));

	if ((op_code == XR_TXBUS_OP_FORCE_TX) || (rpbuf->buffer_mrx_agg_count >=
		rpbuf->buffer_mrx_agg_max)) {
		ret = rpbuf_send(rpbuf);
		if (ret)
			return ret;
	}

	return ret;
}

int rpbuf_xradio_send(struct xradio_hdr hdr, void *data, int len, enum xrlink_rpbuf_op op)
{
	int ret = -1;

	if (xr_rpbuf && xr_rpbuf->buffer_mrx) {
		ret = rpbuf_xradio_transmit(xr_rpbuf, hdr, data, len, op);
		if (ret < 0) {
			SLAVE_WLAN_ERR("rpbuf_xradio_transmit (with data) failed %d\n", ret);
		}
	} else {
		SLAVE_WLAN_ERR("rpbuf not ready\n");
	}

	return ret;
}

int rpbuf_xradio_init(void)
{
	int ret = 0;
	struct xrlink_rpbuf_dev *rpbuf;

	rpbuf = rp_malloc(sizeof(struct xrlink_rpbuf_dev));
	if (!rpbuf) {
		SLAVE_WLAN_ERR("malloc fail, len: %d\n", sizeof(struct xrlink_rpbuf_dev));
		return -1;
	}
	ret = rpbuf_xradio_create(rpbuf, 1, 0, RPBUF_BUFFER_NAME_XRADIO_MTX, RPBUF_BUFFER_LENGTH_XRADIO_MTX);
	if (ret < 0) {
		SLAVE_WLAN_ERR("mtx rpbuf_xradio_create failed %d\n", ret);
		goto err0;
	}
	ret = rpbuf_xradio_create(rpbuf, 0, 0, RPBUF_BUFFER_NAME_XRADIO_MRX, RPBUF_BUFFER_LENGTH_XRADIO_MRX);
	if (ret < 0) {
		SLAVE_WLAN_ERR("mrx rpbuf_xradio_create failed %d\n", ret);
		goto err1;
	}

	xr_rpbuf = rpbuf;
	SLAVE_WLAN_SYSLOG("rpbuf_xradio_create success\n");
	return 0;
err1:
	rpbuf_xradio_destroy(rpbuf, 1);
err0:
	rp_free(rpbuf);
	return ret;
}

int rpbuf_xradio_deinit(void)
{
	int ret = 0;

	if (!xr_rpbuf) {
		SLAVE_WLAN_ERR("xr_rpbuf is null !\n");
		return -1;
	}

	ret = rpbuf_xradio_destroy(xr_rpbuf, 1);
	if (ret < 0) {
		SLAVE_WLAN_ERR("rpbuf_xradio_destroy mtx failed %d\n", ret);
	}
	ret = rpbuf_xradio_destroy(xr_rpbuf, 0);
	if (ret < 0) {
		SLAVE_WLAN_ERR("rpbuf_xradio_destroy mrx failed %d\n", ret);
	}
	rp_free(xr_rpbuf);

	return ret;
}
