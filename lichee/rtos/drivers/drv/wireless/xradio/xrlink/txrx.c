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
#include <string.h>
#include <sys/endian.h>

#include "kernel/os/os.h"
#include "sys/atomic.h"
#include "FreeRTOS.h"
#include "wlan.h"
#include "queue.h"
#include "command.h"
#include "checksum.h"
#include "xradio_rpmsg.h"
#include "xradio_rpbuf.h"
#include "xrlink.h"
#include "txrx.h"
#include "net_utils.h"
#include "net/wlan/wlan.h"
#include "net/ethernetif/ethernetif.h"
#include "port/workqueue.h"
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
#include "pm_state.h"
#endif

#define XRLINK_THREAD_STACK_SIZE         (1024 * 3)

#ifdef RPBUF_NO_CHECK_RSP
#define XRLINK_DATA_TX_QUEUE_SIZE         256
#else
#define XRLINK_DATA_TX_QUEUE_SIZE         160 //64//256
#endif
#define XRLINK_DATA_RX_QUEUE_SIZE         128 //256
#define XRLINK_TX_COMMOM_QUEUE_SIZE       8
#define XRLINK_CMD_RSP_QUEUE_SIZE         2

#ifdef RPBUF_NO_CHECK_RSP
#define XRLINK_TXQ_PAUSE_THRESH           6
#define XRLINK_TXQ_RESUME_THRESH          3
#else
#define XRLINK_TXQ_PAUSE_THRESH           9
#define XRLINK_TXQ_RESUME_THRESH          5 // 160 * 0.5 = 80
#endif

#define TX_DISPATCH_RETRY_COUNT           10
#define TX_DISPATCH_TXQ_LOCK_TIMEOUT_MS   500

#define XRLINK_TX_DATA_RETRY_MAX_CNT      1000
#define XRLINK_CMD_EVTENT_TO_MS           (10 * 1000)

#define txrx_malloc                        malloc
#define txrx_free                          free

#define txrx_dump(str, data, len)          //data_hex_dump(str, 16, data, len)
#define txrx_err_dump(str, data, len)      data_hex_dump(str, 16, data, len)

#define XR_QUEUE_NAME_SIZE                32
typedef void (*xrlink_queue_free_t)(void *data, uint8_t type, uint8_t flag);
typedef struct {
	char name[XR_QUEUE_NAME_SIZE];
	uint16_t capacity;
	uint16_t num_queued;
	XR_OS_Queue_t queue;
	XR_OS_Mutex_t lock;
	uint8_t pause;
	xrlink_queue_free_t free_buf;
} xrlink_queue_t;

typedef struct {
	uint8_t type;
	uint8_t flag;
	uint16_t len;
	uint8_t *buf;
} xr_tbuf_t;
#define XR_BUF_FLAG_MBUF  (1 << 0)

/* XRlink Slave port */
typedef struct {
	XR_OS_Semaphore_t sem_tx_com;
	XR_OS_Semaphore_t sem_data_tx;
	XR_OS_Semaphore_t sem_cmd_rsp;
	XR_OS_Thread_t thread_data_tx; // Linux -> RTOS data pkt
	XR_OS_Thread_t thread_tx_com; // Linux -> RTOS CMD/Event pkt
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	XR_OS_Thread_t thread_data_rx; // Linux <- RTOS data pkt
	xrlink_queue_t queue_data_rx;
	XR_OS_Semaphore_t sem_data_rx;
	uint8_t thread_data_rx_en;
#endif
	xrlink_queue_t queue_data_tx;
	xrlink_queue_t queue_tx_com;
	xrlink_queue_t queue_cmd_rsp;

	XR_OS_Mutex_t lock_rx_com;
	XR_OS_Mutex_t lock_rx_data;
	atomic_t data_tx_thread_sleep_ms;

	uint8_t thread_data_tx_en;
	uint8_t thread_tx_com_en;
	uint8_t tx_stop;
	uint8_t rx_stop;
	uint8_t status_rx_com;
	uint8_t status_data_rx;
	uint8_t master_rx_ready;
	uint16_t seq_rx_com;
	uint16_t seq_data_rx;
} xrlink_tranc_t;

static xrlink_tranc_t *xrlink_tranc;

static inline void xrlink_txrx_lock(XR_OS_Mutex_t *mutex)
{
	XR_OS_Status ret;

	ret = XR_OS_MutexLock(mutex, XR_OS_WAIT_FOREVER);
	if (ret != XR_OS_OK) {
		TXRX_ERR("lock err\n");
	}
}

static inline void xrlink_txrx_unlock(XR_OS_Mutex_t *mutex)
{
	XR_OS_Status ret;

	ret = XR_OS_MutexUnlock(mutex);
	if (ret != XR_OS_OK) {
		TXRX_ERR("unlock err\n");
	}
}

//static
int xrlink_queue_is_empty(xrlink_queue_t *q)
{
	//return (!(XR_OS_QueueMessagesWaiting(&q->queue) > 0));
	return (!(q->num_queued > 0));
}

int xrlink_queue_is_full(xrlink_queue_t *q)
{
	return (q->num_queued == q->capacity);
}

static int xrlink_queue_push(xrlink_queue_t *q, xr_tbuf_t *buf)
{
	int ret = -1;

	if (XR_OS_QueueIsValid(&q->queue)) {
		xrlink_txrx_lock(&q->lock);
		if (XR_OS_QueueSend(&q->queue, buf, 0) != XR_OS_OK) {
			TXRX_DBG("xrlink queue %s push failed qd %d\n", q->name, q->num_queued);
		} else {
			++q->num_queued;
			ret = 0;
		}
		xrlink_txrx_unlock(&q->lock);
	} else {
		TXRX_WRN("queue is not inited\n");
		return -1;
	}

	return ret;
}

static int xrlink_queue_pop(xrlink_queue_t *q, xr_tbuf_t *buf)
{
	int ret = 0;

	if (!XR_OS_QueueIsValid(&q->queue)) {
		TXRX_WRN("queue is not inited\n");
		return -1;
	}

	xrlink_txrx_lock(&q->lock);
	ret = XR_OS_QueueMessagesWaiting(&q->queue);
	if (ret > 0) {
		if (XR_OS_QueueReceive(&q->queue, buf, 0) == XR_OS_OK) {
			--q->num_queued;
		} else {
			ret = -1;
			TXRX_DBG("xrlink queue %s pop failed\n", q->name);
		}

	}
	xrlink_txrx_unlock(&q->lock);

	return ret;
}

static int xrlink_queue_clear(xrlink_queue_t *q)
{
	xr_tbuf_t rbuf = {0};

	if (!XR_OS_QueueIsValid(&q->queue)) {
		TXRX_WRN("queue is not inited\n");
		return -1;
	}
	xrlink_txrx_lock(&q->lock);
	while (XR_OS_QueueReceive(&q->queue, &rbuf, 0) == XR_OS_OK) {
		if (rbuf.buf && q->free_buf)
			q->free_buf(rbuf.buf, rbuf.type, rbuf.flag);
		rbuf.buf = NULL;
	}
	xrlink_txrx_unlock(&q->lock);

	return 0;
}

static void xrlink_data_tx_pause_check(void)
{
	xrlink_tranc_t *tranc;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return;
	}
	tranc = xrlink_tranc;

	if (!tranc->queue_data_tx.pause &&
		(tranc->queue_data_tx.num_queued > ((uint32_t)tranc->queue_data_tx.capacity * XRLINK_TXQ_PAUSE_THRESH / 10))) {
		int ret = xradio_link_tx_data_pause(XRLINK_RX_DATA_PAUSE_FLAG_XRLINK_TXQ);

		if (!ret) {
			xrlink_txrx_lock(&tranc->queue_data_tx.lock);
			tranc->queue_data_tx.pause = 1;
			xrlink_txrx_unlock(&tranc->queue_data_tx.lock);
		}
	}
}

static void xrlink_data_tx_resume_check(void)
{
	xrlink_tranc_t *tranc;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return;
	}
	tranc = xrlink_tranc;

	if (tranc->queue_data_tx.pause &&
		(tranc->queue_data_tx.num_queued < ((uint32_t)tranc->queue_data_tx.capacity * XRLINK_TXQ_RESUME_THRESH / 10))) {
		int ret = xradio_link_tx_data_resume(XRLINK_RX_DATA_PAUSE_FLAG_XRLINK_TXQ);

		if (!ret) {
			xrlink_txrx_lock(&tranc->queue_data_tx.lock);
			tranc->queue_data_tx.pause = 0;
			xrlink_txrx_unlock(&tranc->queue_data_tx.lock);

		}
	}
}

int xrlink_get_data_tx_queued_num(void)
{
	xrlink_tranc_t *tranc;

	if (!xrlink_tranc) {
		TXRX_WRN("xrlink_tranc is null.\n");
		return 0;
	}
	tranc = xrlink_tranc;

	return tranc->queue_data_tx.num_queued;
}

#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
static __inline void data_rx_free(uint8_t *data, uint8_t type, uint8_t flag)
{
#ifdef XRLINK_RX_DATA_USE_MBUF
	if ((flag & XR_BUF_FLAG_MBUF) && ((type == XR_TYPE_DATA) || (type == XR_TYPE_DATA_AP)))
		mb_free((struct mbuf *)data);
	else
#endif
		txrx_free(data);
}
#endif

#ifdef XRADIO_HDR_CKSUM
static int xrlink_checksum_check(struct xradio_hdr *hdr)
{
#ifdef XRADIO_HDR_CKSUM
	uint16_t checksum, cal_checksum;

	checksum = le16_to_cpu(hdr->checksum);
	cal_checksum = le16_to_cpu(xradio_crc_16(hdr->payload, hdr->len));
	if (checksum != cal_checksum) {
		TXRX_ERR("data error!\n");
		TXRX_ERR("len:%d, checksum = 0x%x, cal_checksum = 0x%x\n",
				 hdr->len, checksum, cal_checksum);
		txrx_err_dump("src:", (void *)hdr, hdr->len + XR_HDR_SZ);
		return -1;
	}
#endif
	return 0;
}
#endif

int xrlink_low_decode_handler(uint8_t **data, uint16_t *len, xr_tbuf_t *int_buf)
{
	if ((int_buf->flag & XR_BUF_FLAG_MBUF) && ((int_buf->type == XR_TYPE_DATA) || (int_buf->type == XR_TYPE_DATA_AP))) {
		*data = int_buf->buf; //mbuf
		*len = int_buf->len;
	} else {
		struct xradio_hdr *hdr = NULL;

		hdr = (struct xradio_hdr *)int_buf->buf;
#ifdef XRADIO_HDR_CKSUM
		if(xrlink_checksum_check(hdr))
			return -1;
#endif
		if (hdr->len != (int_buf->len - XR_HDR_SZ)) {
			TXRX_ERR("data error! hdr len %d, buf len %d hdr sz:%d\n",
			         hdr->len, int_buf->len, XR_HDR_SZ);
			return -1;
		}

		*data = hdr->payload;
		*len = hdr->len;
	}

	return 0;
}

static __inline__ bool xrlink_check_rx_retry(int status)
{
	if ((status == XR_TXD_ST_NO_MEM) || (status == XR_TXD_ST_NO_QUEUE) ||
		(status == XR_TXD_ST_BUS_TX_FAIL))
		return true;
	else
		return false;
}

static __inline__ int xrlink_rx_data_send(xrlink_tranc_t *tranc, struct xradio_hdr hdr, void *data, enum xrlink_rpbuf_op op)
{
	int ret = 0;
	int max_retry = XRLINK_TX_DATA_RETRY_MAX_CNT;
	uint16_t sleep_time = 1;
	uint32_t sys_pm_status = 1;
	enum xrlink_rpbuf_op _op = op;

	while (max_retry-- && !tranc->rx_stop && tranc->master_rx_ready) {
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
		sys_pm_status = (pm_state_get() == PM_STATUS_RUNNING) ? 1 : 0;
#endif
		if (!sys_pm_status) {
			TXRX_WRN("host sleep, we can not send data to host\n");
			break;
		}
		ret = rpbuf_xradio_send(hdr, data, hdr.len, _op);
		if (ret == 0) {
			break;
		} else if (!xrlink_check_rx_retry(ret)) {
			break;
		} else if (max_retry) {
			_op = XR_TXBUS_OP_TX_RETRY;
			//TXRX_DBG
			TXRX_DBG("data rx sleep:%dms retryed:%d ret:%d\n", sleep_time, XRLINK_TX_DATA_RETRY_MAX_CNT - max_retry, ret);
			XR_OS_MSleep(sleep_time);
			//sleep_time += 1;
			//sleep_time = max(sleep_time, 5);
		}
	}
	if (ret) {
		max_retry++;
		TXRX_ERR("rxdata err:%d type:%d retryed:%d\n", ret, hdr.type, XRLINK_TX_DATA_RETRY_MAX_CNT - max_retry);
		op = XR_TXBUS_OP_RESET_BUF;
		rpbuf_xradio_send(hdr, NULL, 0, op);
	}

	return ret;
}

int xrlink_rx_data_process(void *data, uint16_t len, uint8_t type, uint8_t force_tx, uint8_t flag)
{
	int ret = 0;
	xrlink_tranc_t *tranc;
	enum xrlink_rpbuf_op op = XR_TXBUS_OP_AUTO;
	struct xradio_hdr hdr = {0};
	uint8_t data_is_xrhdr = 1;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	tranc = xrlink_tranc;
	if (tranc->rx_stop) {
		TXRX_WRN("rx stop!\n");
		return -1;
	}

	xrlink_txrx_lock(&tranc->lock_rx_data);
	tranc->status_data_rx = 1;
#ifdef XRLINK_RX_DATA_USE_MBUF
	if ((flag & XR_BUF_FLAG_MBUF) && ((type == XR_TYPE_DATA) || (type == XR_TYPE_DATA_AP)))
		data_is_xrhdr = 0;
	else
		data_is_xrhdr = 1;
#else
	data_is_xrhdr = 1;
#endif

	hdr.type = type;
	hdr.seq = tranc->seq_data_rx;
	if (data_is_xrhdr)
		hdr.len = len - XR_HDR_SZ;
	else
		hdr.len = len;
#ifdef XRADIO_HDR_CKSUM
	if (data_is_xrhdr)
		hdr.checksum = cpu_to_le16(xradio_crc_16(((struct xradio_hdr *)data)->payload, len));
	else
		hdr.checksum = cpu_to_le16(xradio_crc_16(mtod((struct mbuf *)data, uint8_t *), len));
	TXRX_DBG("checksum:%x\n", hdr.checksum);
#endif
	TXRX_DBG("data tx len:%d seq:%d type:%d\n", hdr.len, hdr.seq, hdr.type);
	if (hdr.type == XR_TYPE_FW_BIN) {
		op = XR_TXBUS_OP_FLUSH_BUF;
		if (data_is_xrhdr)
			xrlink_rx_data_send(tranc, hdr, ((struct xradio_hdr *)data)->payload, op);
		else
			xrlink_rx_data_send(tranc, hdr, mtod((struct mbuf *)data, uint8_t *), op);
	}
	if (force_tx)
		op = XR_TXBUS_OP_FORCE_TX;
	else
		op = XR_TXBUS_OP_AUTO;
	if (data_is_xrhdr)
		ret = xrlink_rx_data_send(tranc, hdr, ((struct xradio_hdr *)data)->payload, op);
	else
		ret = xrlink_rx_data_send(tranc, hdr, mtod((struct mbuf *)data, uint8_t *), op);
	tranc->seq_data_rx++;

	tranc->status_data_rx = 0;
	xrlink_txrx_unlock(&tranc->lock_rx_data);

	return ret;
}

// ret == 0: mbuf xrlink free
int xrlink_send_data2host(uint8_t type, void *data, uint16_t len, uint8_t force_tx, uint8_t mbuf)
{
	struct xradio_hdr *hdr = NULL;
	int ret = -1;
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	xrlink_tranc_t *tranc;
	xr_tbuf_t tranc_buf;
	int max_retry = XRLINK_TX_DATA_RETRY_MAX_CNT, retry = 0;
	uint16_t sleep_time = 1;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	tranc = xrlink_tranc;
#else
	uint8_t flag = 0;
#endif
	if ((type != XR_TYPE_DATA) && (type != XR_TYPE_DATA_AP)) {
		TXRX_ERR("type err %d.\n", type);
		return -1;
	}

	if (mbuf == 0) {
		hdr = txrx_malloc(XR_HDR_SZ + len);
		if (!hdr) {
			TXRX_WRN("malloc failed len:%d.\n", XR_HDR_SZ + len);
			return -1;
		}
		hdr->type = type;
		hdr->len = len;
		memcpy(hdr->payload, data, len);
	}

#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	if (mbuf) {
		tranc_buf.buf = data;
		tranc_buf.len = len;
		tranc_buf.type = type;
		tranc_buf.flag = XR_BUF_FLAG_MBUF;
	} else {
		tranc_buf.buf = (void *)hdr;
		tranc_buf.len = hdr->len + XR_HDR_SZ;
		tranc_buf.type = hdr->type;
		tranc_buf.flag = 0;
	}
	TXRX_INF("type:%d, buf:%p len:%d\n", tranc_buf.type, tranc_buf.buf, tranc_buf.len);

#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
	if (xrlink_queue_is_full(&tranc->queue_data_rx) && pm_state_get() != PM_STATUS_RUNNING) {
		TXRX_WRN("queue full in sleeping, discard\n");
		return -1;
	}
#endif

	while (max_retry-- && !tranc->rx_stop) {
		ret = xrlink_queue_push(&tranc->queue_data_rx, &tranc_buf);
		if (ret == 0) {
			break;
		} else if (max_retry) {
			retry++;
			if (retry > 10)
				sleep_time = 2;
			else if (retry > 20)
				sleep_time = 3;
			else if (retry > 100)
				sleep_time = 4;
			XR_OS_MSleep(sleep_time);
		}
	}
	if (ret) {
		TXRX_WRN("push Q fail %d, retryed:%d\n", ret, retry);
		if (xrlink_queue_is_full(&tranc->queue_data_rx))
			XR_OS_SemaphoreRelease(&tranc->sem_data_rx);
	} else {
		XR_OS_SemaphoreRelease(&tranc->sem_data_rx);
	}
#else // no XRLINK_SEND_DATA2HOST_USE_THREAD

	if (mbuf)
		flag = XR_BUF_FLAG_MBUF;

#ifdef XRLINK_RX_DATA_USE_MBUF
	ret = xrlink_rx_data_process(data, len, type, force_tx, flag);
	if (!ret)
		data_rx_free(data, type, flag);
#else
	ret = xrlink_rx_data_process(hdr, hdr->len + XR_HDR_SZ, hdr->type, force_tx, flag);
	txrx_free(hdr);
#endif

#endif // XRLINK_SEND_DATA2HOST_USE_THREAD
	return ret;
}

int xrlink_flush_send_data2host(void)
{
	xrlink_tranc_t *tranc;
	enum xrlink_rpbuf_op op;
	struct xradio_hdr xr_hdr = {0};
	int ret;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	tranc = xrlink_tranc;
	if (tranc->rx_stop) {
		TXRX_WRN("rx stop!\n");
		return -1;
	}
	xrlink_txrx_lock(&tranc->lock_rx_data);
	TXRX_DBG("rx2host flush buf\n");
	op = XR_TXBUS_OP_FLUSH_BUF;
	ret = xrlink_rx_data_send(tranc, xr_hdr, NULL, op);
	xrlink_txrx_unlock(&tranc->lock_rx_data);

	return ret;
}

int xrlink_send_cmd_ack(void *data, uint16_t data_len)
{
	struct xradio_hdr *hdr = NULL;
	int ret = 0;
	int max_retry = 10;
	xrlink_tranc_t *tranc;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	tranc = xrlink_tranc;

	hdr = (struct xradio_hdr *)txrx_malloc(XR_HDR_SZ + data_len);
	if (!hdr) {
		TXRX_ERR("no mem %d.\n", XR_HDR_SZ + data_len);
		return -1;
	}

	memset(hdr, 0, XR_HDR_SZ + data_len);
	if (data)
		memcpy(hdr->payload, data, data_len);

	hdr->type = XR_TYPE_CMD_RSP;
	hdr->len = data_len;
#ifdef XRADIO_HDR_CKSUM
	hdr->checksum = cpu_to_le16(xradio_crc_16(hdr->payload, hdr->len));
#endif
	while (max_retry-- && !tranc->rx_stop) {
		ret = rpmsg_xradio_transmit((void *)hdr, hdr->len + XR_HDR_SZ);
		if (ret < 0) {
			TXRX_ERR("msg send fail %d retry %d.\n", ret, max_retry);
			XR_OS_MSleep(1);
		} else {
			break;
		}
	}
	if (ret < 0) {
		TXRX_ERR("cmd ack send fail %d.\n", ret);
	} else {
		ret = 0;
		TXRX_DBG("cmd ack send sucess.\n");
	}

	txrx_free(hdr);

	return ret;
}

static int xrlink_rx_com_send(xrlink_tranc_t *tranc, struct xradio_hdr *hdr, void *cfm,
	                               uint16_t cfm_len, uint8_t data_buf, uint8_t force_tx)
{
	bool need_rsp;
	int ret = 0;
	int tmo;
	XR_OS_Status status;
	xr_tbuf_t rxbuf = {0};

	if (!tranc->master_rx_ready) {
		TXRX_ERR("master_rx_ready is 0.\n");
		return -1;
	}

	if ((hdr->type == XR_TYPE_CMD) || (hdr->type == XR_TYPE_FW_BIN))
		need_rsp = true;
	else
		need_rsp = false;

	if (data_buf)
		ret = xrlink_rx_data_process(hdr, hdr->len + XR_HDR_SZ, hdr->type, force_tx, 0);
	else
		ret = rpmsg_xradio_transmit((void *)hdr, hdr->len + XR_HDR_SZ);
	if (ret < 0) {
		TXRX_ERR("msg send fail %d data_buf:%d.\n", ret, data_buf);
		return ret;
	} else {
		ret = 0;
	}
	if (!need_rsp)
		return ret;
	if (tranc->rx_stop) {
		TXRX_WRN("rx stop!\n");
		return -1;
	}

	tmo = XRLINK_CMD_EVTENT_TO_MS;
	status = XR_OS_SemaphoreWait(&tranc->sem_cmd_rsp, tmo);
	if (status == XR_OS_E_TIMEOUT) {
		TXRX_ERR("cmd rsp timeout, timeout %d ms.\n", tmo);
		return -1;
	} else if (status) {
		TXRX_ERR("wait_event error %d.\n", status);
		return -1;
	}
	if (tranc->rx_stop) {
		TXRX_ERR("rx stop!\n");
		return -1;
	}
	ret = -1;
	ret = xrlink_queue_pop(&tranc->queue_cmd_rsp, &rxbuf);
	TXRX_DBG("xrlink cmd rsp type:%d qnum:%d\n", rxbuf.type, xrlink_queue_is_empty(&tranc->queue_cmd_rsp));
	if ((ret > 0) && rxbuf.buf) {
		struct xradio_hdr *rsp;
#ifdef XRADIO_HDR_CKSUM
		uint16_t checksum, cal_checksum;
#endif

		rsp = (void *)rxbuf.buf;
#ifdef XRADIO_HDR_CKSUM
		checksum = le16_to_cpu(hdr->checksum);
		cal_checksum = le16_to_cpu(xradio_crc_16(hdr->payload, hdr->len));
		if (checksum != cal_checksum) {
			TXRX_ERR("recv data error!\n");
			TXRX_ERR("len:%d, checksum = 0x%x, cal_checksum = 0x%x\n",
					hdr->len, checksum, cal_checksum);
			txrx_err_dump("src:", (void *)hdr, hdr->len + XR_HDR_SZ);
			return -1;
		}
#endif
		TXRX_DBG("cmd rsp type %d len %d buf:%p.\n", rsp->type, rsp->len, rxbuf.buf);
		//txrx_dump("rsp:", (void *)rsp, rsp->len > 64 ? 64 : rsp->len);

		if ((rsp->type == XR_TYPE_CMD_RSP) || (rsp->type == XR_TYPE_FW_BIN)) {
			if (cfm) {
				if (((rsp->type == XR_TYPE_CMD_RSP) && (cfm_len == rsp->len)) ||
					(rsp->type == XR_TYPE_FW_BIN)) {
					if (rsp->type == XR_TYPE_FW_BIN)
						memcpy(cfm, rsp->payload, min(cfm_len, rsp->len));
					else
						memcpy(cfm, rsp->payload, cfm_len);
					ret = 0;
					//txrx_dump("cfm:", (void *)cfm, min(cfm_len, rsp->len));
				} else {
					TXRX_ERR("rsp len %d error, req:%d.\n", rsp->len, cfm_len);
					txrx_err_dump("rsp:", (void *)rsp, rsp->len > 64 ? 64 : rsp->len);
				}
			} else {
				ret = 0;
			}
		} else {
			TXRX_ERR("rsp type err type:%d len:%d.\n", rsp->type, rsp->len);
		}

		txrx_free(rxbuf.buf);
	} else {
		TXRX_ERR("cmd rsp recive err from queue %d.\n", ret);
	}

	return ret;
}

int xrlink_rx_com_process(struct xradio_hdr *xr_hdr, void *cfm, uint16_t cfm_len, uint8_t data_buf, uint8_t force_tx)
{
	int ret = 0;
	xrlink_tranc_t *tranc;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	tranc = xrlink_tranc;

	xrlink_txrx_lock(&tranc->lock_rx_com);
	tranc->status_rx_com = 1;
	xr_hdr->seq = tranc->seq_rx_com;
#ifdef XRADIO_HDR_CKSUM
	xr_hdr->checksum = cpu_to_le16(xradio_crc_16(xr_hdr->payload, xr_hdr->len));
#endif
	ret = xrlink_rx_com_send(tranc, xr_hdr, cfm, cfm_len, data_buf, force_tx);
	if (ret) {
		TXRX_ERR("send com fail %d, type:%d len:%d seq:%d.\n",
			ret, xr_hdr->type,
			xr_hdr->len, tranc->seq_rx_com);
	}
	tranc->seq_rx_com++;
	tranc->status_rx_com = 0;
	xrlink_txrx_unlock(&tranc->lock_rx_com);

	return ret;
}

static int xrlink_tx_dispatch(xrlink_tranc_t *tranc, uint8_t type, uint8_t *data, uint16_t len)
{
	int retry = 1;
	int ret = -255;
#ifdef CONFIG_STA_SOFTAP_COEXIST
	enum wlan_mode mode = WLAN_MODE_STA;
#endif

	switch (type) {
	case XR_TYPE_DATA:
#ifdef CONFIG_STA_SOFTAP_COEXIST
	case XR_TYPE_DATA_AP:
		if (type == XR_TYPE_DATA_AP)
			mode = WLAN_MODE_HOSTAP;
#endif
		while (xradio_link_wifi_drv_txq_lock() && (retry++ < TX_DISPATCH_TXQ_LOCK_TIMEOUT_MS) &&
			tranc->thread_data_tx_en)
			XR_OS_MSleep(1);

		if (atomic_read(&tranc->data_tx_thread_sleep_ms) > 0) {
			atomic_dec(&tranc->data_tx_thread_sleep_ms);
			XR_OS_MSleep(1);
		}

		if (!tranc->thread_data_tx_en || xradio_link_wifi_drv_txq_lock()) {
			// free mbuf
			mb_free((struct mbuf *)data);
			TXRX_WRN("drop frame len:%d.\n", len);
			break;
		}
		retry = 0;

		/*
		ret:
		-2: linkoutput failed, mbuf already free;
		-1: netif not ready, uplayer need to free mbuf;
		0 : sucesss, mbuf already free.
		*/
#ifdef CONFIG_STA_SOFTAP_COEXIST
		ret = ethernetif_host_to_wlan(data, len, mode);
#else
		ret = ethernetif_host_to_wlan(data, len);
#endif
		// check need to free mbuf
		if (ret == -1) {
			// free mbuf
			mb_free((struct mbuf *)data);
		}
		break;
	case XR_TYPE_CMD:
	case XR_TYPE_EVENT:
	case XR_TYPE_CMD_RSP:
		ret = ptc_command_handler(type, data, len);
		break;
	case XR_TYPE_FW_BIN:
		TXRX_ERR("ERR, type=0x%x, len:%d\n", type, len);
		break;
	default:
		TXRX_ERR("not support type=0x%x, len:%d\n", type, len);
		break;
	}

	return ret;
}

static void xrlink_tx_com_task(void *arg)
{
	int ret = -1;
	xrlink_tranc_t *tranc = arg;
	xr_tbuf_t rxbuf;

	tranc->thread_tx_com_en = 1;
	while (1) {
		if (XR_OS_SemaphoreWait(&tranc->sem_tx_com, XR_OS_WAIT_FOREVER) != XR_OS_OK) {
			continue;
		}
		if (!tranc->thread_tx_com_en) {
			TXRX_INF("xrlink_tx_com_task should stop\n");
			break;
		}

		ret = xrlink_queue_pop(&tranc->queue_tx_com, &rxbuf);
		TXRX_INF("xrlink com type:%d\n", rxbuf.type);
		if (ret > 0) {
			if (rxbuf.buf) {
				uint8_t *data;
				uint16_t len;

				ret = xrlink_low_decode_handler(&data, &len, &rxbuf);
				if (!ret)
					xrlink_tx_dispatch(tranc, rxbuf.type, data, len);
				txrx_free(rxbuf.buf);
			} else {
				TXRX_ERR("payload err, type:%d buf:%p\n", rxbuf.type, rxbuf.buf);
			}
		}
		rxbuf.buf = NULL;
		rxbuf.type = 0xFF;
	}

	TXRX_INF("xrlink_tx_com_task task quit\n");
	XR_OS_ThreadDelete(&tranc->thread_tx_com);
}

static __inline void data_tx_free(uint8_t *data, uint8_t type)
{
	if ((type == XR_TYPE_DATA) || (type == XR_TYPE_DATA_AP))
		return;
	txrx_free(data);
}

static void xrlink_data_tx_task(void *arg)
{
	int ret = -1;
	xrlink_tranc_t *tranc = arg;
	xr_tbuf_t rxbuf;

	tranc->thread_data_tx_en = 1;
	while (1) {
		if (XR_OS_SemaphoreWait(&tranc->sem_data_tx, XR_OS_WAIT_FOREVER) != XR_OS_OK) {
			continue;
		}
		if (!tranc->thread_data_tx_en) {
			TXRX_INF("xrlink_data_tx_task should stop\n");
			break;
		}

		ret = xrlink_queue_pop(&tranc->queue_data_tx, &rxbuf);
		TXRX_INF("xrlink data tx type:%d\n", rxbuf.type);
		if (ret > 0) {
			if (rxbuf.buf) {
				uint8_t *data = NULL;
				uint16_t len = 0;

				ret = xrlink_low_decode_handler(&data, &len, &rxbuf);
				if (!ret)
					xrlink_tx_dispatch(tranc, rxbuf.type, data, len);
				data_tx_free(rxbuf.buf, rxbuf.type);
			} else {
				TXRX_ERR("payload err, type:%d buf:%p\n", rxbuf.type, rxbuf.buf);
			}
			xrlink_data_tx_resume_check();
		}
		rxbuf.buf = NULL;
		rxbuf.type = 0xFF;
	}

	TXRX_INF("xrlink_data_tx_task quit\n");
	XR_OS_ThreadDelete(&tranc->thread_data_tx);
}

#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
static void xrlink_data_rx_task(void *arg)
{
	int ret = -1;
	xrlink_tranc_t *tranc = arg;
	uint8_t force_tx = 0;
	xr_tbuf_t buf;

	tranc->rx_stop = 0;
	tranc->thread_data_rx_en = 1;
	while (1) {
		if (wait_event_interruptible(&tranc->sem_data_rx,
			                          (!xrlink_queue_is_empty(&tranc->queue_data_rx) &&
			                          tranc->master_rx_ready) || !tranc->thread_data_rx_en) < 0) {
			continue;
		}
		if (!tranc->thread_data_rx_en) {
			TXRX_INF("xrlink_data_rx_task should stop\n");
			break;
		}

		while (!xrlink_queue_is_empty(&tranc->queue_data_rx) && tranc->thread_data_rx_en && tranc->master_rx_ready) {
			buf.buf = NULL;
			buf.type = 0xFF;
			ret = xrlink_queue_pop(&tranc->queue_data_rx, &buf);
			TXRX_INF("xrlink data rx type:%d buf:%p len:%d\n", buf.type, buf.buf, buf.len);
			if (ret > 0) {
				if (buf.buf) {
					if (xrlink_queue_is_empty(&tranc->queue_data_rx))
						force_tx = 1;
					else
						force_tx = 0;
					xrlink_rx_data_process(buf.buf, buf.len, buf.type, force_tx, buf.flag);
					data_rx_free(buf.buf, buf.type, buf.flag);
				} else {
					TXRX_ERR("payload err, type:%d buf:%p len:%d\n", buf.type, buf.buf, buf.len);
				}
			} else {
				break;
			}
		}
	}

	TXRX_INF("xrlink Master data rx task quit\n");
	XR_OS_ThreadDelete(&tranc->thread_data_rx);
}
#endif

int xrlink_tx_com_cb(void *data, int data_len, int buf_sz)
{
	struct xradio_hdr *hdr;
	int ret = 0;
	u8 *buf;
	xr_tbuf_t tranc_buf;
	xrlink_tranc_t *tranc;
	char *q_name;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	tranc = xrlink_tranc;
	TXRX_INF("recv len:%d\n", data_len);
	buf = txrx_malloc(data_len);
	if (!buf) {
		TXRX_ERR("buf alloc failed len %d\n", data_len);
		return -1;
	}

	memcpy(buf, data, data_len);
	hdr = (void *)buf;
	if ((hdr->len + XR_HDR_SZ) != data_len) {
		TXRX_ERR("recv data len error! hdr len %d, msg len %d hdr sz:%d\n",
				hdr->len, data_len, XR_HDR_SZ);
		return -1;
	}

	tranc_buf.buf = buf;
	tranc_buf.len = hdr->len + XR_HDR_SZ;
	tranc_buf.type = hdr->type;

	if ((hdr->type == XR_TYPE_CMD_RSP) || (hdr->type == XR_TYPE_FW_BIN)) {
		ret = xrlink_queue_push(&tranc->queue_cmd_rsp, &tranc_buf);
		q_name = tranc->queue_cmd_rsp.name;
	} else {
		ret = xrlink_queue_push(&tranc->queue_tx_com, &tranc_buf);
		q_name = tranc->queue_tx_com.name;
	}
	if (ret) {
		txrx_free(buf);
		TXRX_ERR("push %s queue fail:%d, discard type %d!\n",
				q_name, ret, hdr->type);
		return ret;
	}
	TXRX_DBG("recv q %s type:%d buf:%p\n", q_name, hdr->type, buf);

	if ((hdr->type == XR_TYPE_CMD_RSP) || (hdr->type == XR_TYPE_FW_BIN))
		XR_OS_SemaphoreRelease(&tranc->sem_cmd_rsp);
	else
		XR_OS_SemaphoreRelease(&tranc->sem_tx_com);

	return ret;
}

// return: > 0: rsp data len; < 0 : err code
int xrlink_data_tx_callback(void *data, int data_len, int buf_sz)
{
	struct xradio_agg_hdr *agg_hdr;
	struct xradio_hdr *hdr;
	int i = 0, rx_len = 0, ret = 0;
	u8 agg_cnt, recived = 0;
	u8 *src, *buf;
	xr_tbuf_t tranc_buf;
	xrlink_tranc_t *tranc;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	tranc = xrlink_tranc;

	src = data;
	agg_hdr = (struct xradio_agg_hdr *)src;
	agg_cnt = agg_hdr->agg_cnt;
	src += XR_AGG_HDR_SZ;
	TXRX_DBG("%s received data (data: %p, len: %d), agg_cnt:%d\n",
			__func__, data, data_len, agg_cnt);

	for (i = 0; i < agg_cnt; i++) {
		hdr = (struct xradio_hdr *)src;
		TXRX_DBG("agg:%d/%d len=%d\n", i, agg_cnt, (XR_HDR_SZ + hdr->len));
		buf = NULL;
		rx_len += (XR_HDR_SZ + hdr->len);
		if ((rx_len > data_len) || (rx_len > buf_sz)) {
			int loop;
			u8 *loop_buf;
			struct xradio_hdr *loop_hdr;

			TXRX_ERR("agg:%d/%d len=%d\n", i, agg_cnt, (XR_HDR_SZ + hdr->len));
			TXRX_ERR("data err:rx_len:%d data_len:%d bufsz:%d now:%d\n",
				rx_len, data_len, buf_sz, (XR_HDR_SZ + hdr->len));

			loop_buf = data;
			loop_buf += XR_AGG_HDR_SZ;
			for (loop = 0; loop < agg_cnt; loop++) {
				loop_hdr = (struct xradio_hdr *)loop_buf;
				TXRX_ERR("[LOOP] agg:%d/%d len=%d, hdr:%x\n",
					loop + 1, agg_cnt, XR_HDR_SZ + loop_hdr->len, (u32)loop_hdr);
				txrx_err_dump("LOOP:", (void *)loop_hdr, (XR_HDR_SZ + loop_hdr->len) > 32 ? 32 : (XR_HDR_SZ + loop_hdr->len));
				loop_buf += (XR_HDR_SZ + loop_hdr->len);
			}
			txrx_err_dump("DATA:", data, data_len > 128 ? 128 : data_len);
			txrx_err_dump("HDR :", (void *)hdr, (XR_HDR_SZ + hdr->len) > 128 ? 128 : (XR_HDR_SZ + hdr->len));

			agg_hdr->ack_cnt = i;
			agg_hdr->rsp_code = XR_TXD_ST_DATA_ERR;
			goto end;
		}
		if (i >= agg_hdr->start_idx) {
			struct mbuf *mbuf_data = NULL;

			TXRX_DBG("=>agg:%d/%d len=%d\n", i, agg_cnt, XR_HDR_SZ + hdr->len);
			if ((hdr->type == XR_TYPE_DATA) || (hdr->type == XR_TYPE_DATA_AP)) {
#ifdef XRADIO_HDR_CKSUM
				xrlink_checksum_check(hdr);
#endif
				mbuf_data = eth_raw_to_mbuf(hdr->payload, hdr->len);
				if (!mbuf_data) {
					//TXRX_DBG
					TXRX_DBG("Malloc fail len:%d, discard data %d!\n", (XR_HDR_SZ + hdr->len), agg_cnt - i);
					agg_hdr->ack_cnt = i;
					agg_hdr->rsp_code = XR_TXD_ST_NO_MEM;
					goto end;
				}
				tranc_buf.buf = (void *)mbuf_data;
				tranc_buf.len = hdr->len;
				tranc_buf.type = hdr->type;
				tranc_buf.flag = XR_BUF_FLAG_MBUF;
			} else {
				buf = txrx_malloc(hdr->len + XR_HDR_SZ);
				if (!buf) {
					//TXRX_DBG
					TXRX_DBG("Malloc fail len:%d, discard data %d!\n", (XR_HDR_SZ + hdr->len), agg_cnt - i);
					agg_hdr->ack_cnt = i;
					agg_hdr->rsp_code = XR_TXD_ST_NO_MEM;
					goto end;
				}
				memcpy(buf, src, hdr->len + XR_HDR_SZ);
				tranc_buf.buf = buf;
				tranc_buf.len = hdr->len + XR_HDR_SZ;
				tranc_buf.type = hdr->type;
				tranc_buf.flag = 0;
			}
			ret = xrlink_queue_push(&tranc->queue_data_tx, &tranc_buf);
			if (ret) {
				txrx_free(buf);
				//TXRX_DBG
				TXRX_DBG("push data rx queue fail:%d, discard data %d!\n",
						ret, agg_cnt - i);
				agg_hdr->ack_cnt = i;
				agg_hdr->rsp_code = XR_TXD_ST_NO_QUEUE;
				goto end;
			}
			recived++;
		}
		src += (XR_HDR_SZ + hdr->len);
	}

	agg_hdr->ack_cnt = i;
	agg_hdr->rsp_code = XR_TXD_ST_SUCESS;
end:
	if (recived) {
		for (i = 0; i < recived; i++)
			XR_OS_SemaphoreRelease(&tranc->sem_data_tx);
	}
	ret = XR_AGG_HDR_SZ;
	xrlink_data_tx_pause_check();
	TXRX_DBG("==>agg:%d/%d ack:%d start:%d\n", i, agg_cnt, agg_hdr->ack_cnt, agg_hdr->start_idx);

	return ret;
}

int xrlink_master_rx_ready_set(uint8_t ready)
{
	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}

	xrlink_tranc->master_rx_ready = ready;
	return 0;
}

int xrlink_data_tx_th_sleeptime_set(uint16_t ms)
{
	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	//TXRX_DBG("s:%d\n", ms);
	atomic_set(&xrlink_tranc->data_tx_thread_sleep_ms, ms);

	return 0;
}

int xrlink_data_tx_th_sleeptime_get(void)
{
	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	return atomic_read(&xrlink_tranc->data_tx_thread_sleep_ms);
}

static void xrlink_queue_free_data_tx(void *data, uint8_t type, uint8_t flag)
{
	if ((type == XR_TYPE_DATA) || (type == XR_TYPE_DATA_AP))
		mb_free(data);
	else
		txrx_free(data);
}

static void xrlink_queue_free_tx_com(void *data, uint8_t type, uint8_t flag)
{
	txrx_free(data);
}

static void xrlink_queue_free_cmd_rsp(void *data, uint8_t type, uint8_t flag)
{
	txrx_free(data);
}

#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
static void xrlink_queue_free_data_rx(void *data, uint8_t type, uint8_t flag)
{
	data_rx_free(data, type, flag);
}
#endif

int xrlink_tranc_early_init(void)
{
	TXRX_INF("start\n");
	int ret = -1;
	xrlink_tranc_t *tranc;
	XR_OS_Status status;

	if (xrlink_tranc) {
		TXRX_WRN("maybe already init, xrlink_tranc %p\n", xrlink_tranc);
		return 0;
	}
	tranc = txrx_malloc(sizeof(xrlink_tranc_t));
	if (!tranc) {
		TXRX_ERR("no mem for tranc\n");
		return -1;
	}
	memset(tranc, 0, sizeof(xrlink_tranc_t));

	status = XR_OS_SemaphoreCreate(&tranc->sem_tx_com, 0, XR_OS_SEMAPHORE_MAX_COUNT);
	if (status != XR_OS_OK) {
		TXRX_ERR("sem_tx_com creat err %d\n", status);
		goto err0;
	}
	status = XR_OS_SemaphoreCreate(&tranc->sem_data_tx, 0, XR_OS_SEMAPHORE_MAX_COUNT);
	if (status != XR_OS_OK) {
		TXRX_ERR("sem_data_tx creat err %d\n", status);
		goto err1;
	}
	status = XR_OS_SemaphoreCreate(&tranc->sem_cmd_rsp, 0, XR_OS_SEMAPHORE_MAX_COUNT);
	if (status != XR_OS_OK) {
		TXRX_ERR("sem_cmd_rsp creat err %d\n", status);
		goto err1;
	}
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	status = XR_OS_SemaphoreCreate(&tranc->sem_data_rx, 0, XR_OS_SEMAPHORE_MAX_COUNT);
	if (status != XR_OS_OK) {
		TXRX_ERR("sem_data_rx creat err %d\n", status);
		goto err1;
	}
#endif

	status = XR_OS_MutexCreate(&tranc->queue_data_tx.lock);
	if (status != XR_OS_OK) {
		TXRX_ERR("mutex queue_data_tx creat err %d\n", status);
		goto err2;
	}
	status = XR_OS_MutexCreate(&tranc->queue_tx_com.lock);
	if (status != XR_OS_OK) {
		TXRX_ERR("mutex queue_tx_com creat err %d\n", status);
		goto err2;
	}
	status = XR_OS_MutexCreate(&tranc->queue_cmd_rsp.lock);
	if (status != XR_OS_OK) {
		TXRX_ERR("mutex queue_cmd_rsp creat err %d\n", status);
		goto err2;
	}
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	status = XR_OS_MutexCreate(&tranc->queue_data_rx.lock);
	if (status != XR_OS_OK) {
		TXRX_ERR("mutex queue_data_rx creat err %d\n", status);
		goto err2;
	}
#endif
	status = XR_OS_MutexCreate(&tranc->lock_rx_com);
	if (status != XR_OS_OK) {
		TXRX_ERR("mutex lock_rx_com creat err %d\n", status);
		goto err2;
	}
	status = XR_OS_MutexCreate(&tranc->lock_rx_data);
	if (status != XR_OS_OK) {
		TXRX_ERR("mutex lock_rx_data creat err %d\n", status);
		goto err2;
	}

	status = XR_OS_QueueCreate(&tranc->queue_data_tx.queue, XRLINK_DATA_TX_QUEUE_SIZE, sizeof(xr_tbuf_t));
	if (status != XR_OS_OK) {
		TXRX_ERR("queue_data_tx creat err %d\n", status);
		goto err3;
	}
	tranc->queue_data_tx.capacity = XRLINK_DATA_TX_QUEUE_SIZE;
	strncpy(tranc->queue_data_tx.name, "xr_data_tx", XR_QUEUE_NAME_SIZE);
	tranc->queue_data_tx.free_buf = xrlink_queue_free_data_tx;

	status = XR_OS_QueueCreate(&tranc->queue_tx_com.queue, XRLINK_TX_COMMOM_QUEUE_SIZE, sizeof(xr_tbuf_t));
	if (status != XR_OS_OK) {
		TXRX_ERR("queue_tx_com creat err %d\n", status);
		goto err3;
	}
	tranc->queue_tx_com.capacity = XRLINK_TX_COMMOM_QUEUE_SIZE;
	strncpy(tranc->queue_tx_com.name, "xr_tx_com", XR_QUEUE_NAME_SIZE);
	tranc->queue_tx_com.free_buf = xrlink_queue_free_tx_com;

	status = XR_OS_QueueCreate(&tranc->queue_cmd_rsp.queue, XRLINK_CMD_RSP_QUEUE_SIZE, sizeof(xr_tbuf_t));
	if (status != XR_OS_OK) {
		TXRX_ERR("queue_cmd_rsp creat err %d\n", status);
		goto err3;
	}
	tranc->queue_cmd_rsp.capacity = XRLINK_CMD_RSP_QUEUE_SIZE;
	strncpy(tranc->queue_cmd_rsp.name, "xr_cmd_rsp", XR_QUEUE_NAME_SIZE);
	tranc->queue_cmd_rsp.free_buf = xrlink_queue_free_cmd_rsp;

#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	status = XR_OS_QueueCreate(&tranc->queue_data_rx.queue, XRLINK_DATA_RX_QUEUE_SIZE, sizeof(xr_tbuf_t));
	if (status != XR_OS_OK) {
		TXRX_ERR("queue_data_rx creat err %d\n", status);
		goto err3;
	}
	tranc->queue_data_rx.capacity = XRLINK_DATA_RX_QUEUE_SIZE;
	strncpy(tranc->queue_data_rx.name, "xr_data_rx", XR_QUEUE_NAME_SIZE);
	tranc->queue_data_rx.free_buf = xrlink_queue_free_data_rx;
#endif

	tranc->tx_stop = 0;
	tranc->rx_stop = 0;
	tranc->master_rx_ready = 0;

	xrlink_tranc = tranc;
	TXRX_SYSLOG("%s init sucess.\n", __func__);
	return 0;
err3:
	if (XR_OS_QueueIsValid(&tranc->queue_data_tx.queue))
		XR_OS_QueueDelete(&tranc->queue_data_tx.queue);
	if (XR_OS_QueueIsValid(&tranc->queue_tx_com.queue))
		XR_OS_QueueDelete(&tranc->queue_tx_com.queue);
	if (XR_OS_QueueIsValid(&tranc->queue_cmd_rsp.queue))
		XR_OS_QueueDelete(&tranc->queue_cmd_rsp.queue);
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	if (XR_OS_QueueIsValid(&tranc->queue_data_rx.queue))
		XR_OS_QueueDelete(&tranc->queue_data_rx.queue);
#endif
err2:
	if (XR_OS_MutexIsValid(&tranc->lock_rx_data))
		XR_OS_MutexDelete(&tranc->lock_rx_data);
	if (XR_OS_MutexIsValid(&tranc->lock_rx_com))
		XR_OS_MutexDelete(&tranc->lock_rx_com);
	if (XR_OS_MutexIsValid(&tranc->queue_data_tx.lock))
		XR_OS_MutexDelete(&tranc->queue_data_tx.lock);
	if (XR_OS_MutexIsValid(&tranc->queue_tx_com.lock))
		XR_OS_MutexDelete(&tranc->queue_tx_com.lock);
	if (XR_OS_MutexIsValid(&tranc->queue_cmd_rsp.lock))
		XR_OS_MutexDelete(&tranc->queue_cmd_rsp.lock);
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	if (XR_OS_MutexIsValid(&tranc->queue_data_rx.lock))
		XR_OS_MutexDelete(&tranc->queue_data_rx.lock);
#endif
err1:
	if (XR_OS_SemaphoreIsValid(&tranc->sem_data_tx))
		XR_OS_SemaphoreDelete(&tranc->sem_data_tx);
	if (XR_OS_SemaphoreIsValid(&tranc->sem_cmd_rsp))
		XR_OS_SemaphoreDelete(&tranc->sem_cmd_rsp);
	if (XR_OS_SemaphoreIsValid(&tranc->sem_tx_com))
		XR_OS_SemaphoreDelete(&tranc->sem_tx_com);
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	if (XR_OS_SemaphoreIsValid(&tranc->sem_data_rx))
		XR_OS_SemaphoreDelete(&tranc->sem_data_rx);
#endif
err0:
	txrx_free(tranc);
	return ret;
}

void xrlink_tranc_early_deinit(void)
{
	TXRX_INF("start\n");
	xrlink_tranc_t *tranc;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return;
	}
	tranc = xrlink_tranc;
	wlan_sta_bss_flush(0);
	// stop tranc
	tranc->rx_stop = 1;
	tranc->tx_stop = 1;
	// flush tx cmd wait
	xrlink_tranc = NULL;
	XR_OS_SemaphoreRelease(&tranc->sem_cmd_rsp);
	xrlink_txrx_lock(&tranc->lock_rx_com);
	xrlink_txrx_unlock(&tranc->lock_rx_com);
	// flush all rx data
	xrlink_txrx_lock(&tranc->lock_rx_data);
	xrlink_txrx_unlock(&tranc->lock_rx_data);

	xrlink_queue_clear(&tranc->queue_data_tx);
	XR_OS_QueueDelete(&tranc->queue_data_tx.queue);

	xrlink_queue_clear(&tranc->queue_tx_com);
	XR_OS_QueueDelete(&tranc->queue_tx_com.queue);

	xrlink_queue_clear(&tranc->queue_cmd_rsp);
	XR_OS_QueueDelete(&tranc->queue_cmd_rsp.queue);
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	xrlink_queue_clear(&tranc->queue_data_rx);
	XR_OS_QueueDelete(&tranc->queue_data_rx.queue);
#endif

	XR_OS_MutexDelete(&tranc->lock_rx_data);
	XR_OS_MutexDelete(&tranc->lock_rx_com);
	XR_OS_MutexDelete(&tranc->queue_data_tx.lock);
	XR_OS_MutexDelete(&tranc->queue_tx_com.lock);
	XR_OS_MutexDelete(&tranc->queue_cmd_rsp.lock);
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	XR_OS_MutexDelete(&tranc->queue_data_rx.lock);
#endif

	XR_OS_SemaphoreDelete(&tranc->sem_data_tx);
	XR_OS_SemaphoreDelete(&tranc->sem_tx_com);
	XR_OS_SemaphoreDelete(&tranc->sem_cmd_rsp);
#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	XR_OS_SemaphoreDelete(&tranc->sem_data_rx);
#endif

	txrx_free(tranc);

}

int xrlink_tranc_init(void)
{
	xrlink_tranc_t *tranc;
	XR_OS_Status status;

	TXRX_SYSLOG("(%s)xrlink txrx init.\n", __func__);
	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return -1;
	}
	tranc = xrlink_tranc;

	XR_OS_ThreadSetInvalid(&tranc->thread_data_tx);

	status = XR_OS_ThreadCreate(&tranc->thread_data_tx,
					"xrlink_data_tx",
					xrlink_data_tx_task,
					(void *)tranc,
					XRLINK_THREAD_PRIO_DATA_TX,
					XRLINK_THREAD_STACK_SIZE);
	if (status != XR_OS_OK) {
		TXRX_ERR("xrlink_slave_rx task creat fail %d.\n", status);
		return -1;
	}

	XR_OS_ThreadSetInvalid(&tranc->thread_tx_com);
	status = XR_OS_ThreadCreate(&tranc->thread_tx_com,
					"xrlink_tx_com",
					xrlink_tx_com_task,
					(void *)tranc,
					XRLINK_THREAD_PRIO_COM,
					XRLINK_THREAD_STACK_SIZE);

	if (status != XR_OS_OK) {
		TXRX_ERR("xrlink_tx_com task creat fail %d.\n", status);
		tranc->thread_data_tx_en = 0;
		XR_OS_SemaphoreRelease(&tranc->sem_data_tx);
		while ((XR_OS_ThreadIsValid(&tranc->thread_data_tx))) {
			XR_OS_MSleep(1);
		}
		return -1;
	}

#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	XR_OS_ThreadSetInvalid(&tranc->thread_data_rx);
	status = XR_OS_ThreadCreate(&tranc->thread_data_rx,
					"xrlink_data_rx",
					xrlink_data_rx_task,
					(void *)tranc,
					XRLINK_THREAD_PRIO_DATA_RX,
					XRLINK_THREAD_STACK_SIZE);

	if (status != XR_OS_OK) {
		TXRX_ERR("xrlink_data_rx task creat fail %d.\n", status);
		tranc->thread_data_tx_en = 0;
		XR_OS_SemaphoreRelease(&tranc->sem_data_tx);
		while ((XR_OS_ThreadIsValid(&tranc->thread_data_tx))) {
			XR_OS_MSleep(1);
		}
		tranc->thread_tx_com_en = 0;
		XR_OS_SemaphoreRelease(&tranc->sem_tx_com);
		while ((XR_OS_ThreadIsValid(&tranc->thread_tx_com))) {
			XR_OS_MSleep(1);
		}
		return -1;
	}
#endif

	TXRX_SYSLOG("(%s) success.\n", __func__);
	return 0;
}

void xrlink_tranc_deinit(void)
{
	xrlink_tranc_t *tranc;

	if (!xrlink_tranc) {
		TXRX_ERR("xrlink_tranc is null.\n");
		return;
	}
	tranc = xrlink_tranc;

#ifdef XRLINK_SEND_DATA2HOST_USE_THREAD
	tranc->thread_data_rx_en = 0;
	XR_OS_SemaphoreRelease(&tranc->sem_data_rx);
	while ((XR_OS_ThreadIsValid(&tranc->thread_data_rx))) {
		XR_OS_MSleep(1);
	}
#endif
	tranc->thread_tx_com_en = 0;
	XR_OS_SemaphoreRelease(&tranc->sem_tx_com);
	while ((XR_OS_ThreadIsValid(&tranc->thread_tx_com))) {
		XR_OS_MSleep(1);
	}

	tranc->thread_data_tx_en = 0;
	XR_OS_SemaphoreRelease(&tranc->sem_data_tx);
	while ((XR_OS_ThreadIsValid(&tranc->thread_data_tx))) {
		XR_OS_MSleep(1);
	}

	TXRX_SYSLOG("xrlink txrx deinit finish.\n");
}
