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
#include "bt_xradio_rpbuf.h"
#include "xrbtc.h"
#include "pal_os.h"

#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/rpmsg_master.h>

#define XRLINK_THREAD  1
#define XRLINK_DEBUG   0

#define BT_RPBUF_BUFFER_NAME_XRADIO_TX "xradio_0" // RTOS -> Linux
#define BT_RPBUF_BUFFER_NAME_XRADIO_RX "xradio_1" // RTOS <- Linux
#define BT_RPBUF_BUFFER_TX_AGG_MAX     24

#define RPBUF_LOG(controller_id, buf_name, buf_len, fmt, args...) \
	printf("[%d|%s|%d] " fmt, controller_id, buf_name, buf_len, ##args)

#define BT_XRADIO_RPMSG_NAME "bt_xrlink_rpmsg"
#define BT_XRADIO_RPMSG_ID 0
#define BT_XRADIO_RPMSG_DATA_LEN 496

struct rpmsg_xradio_private {
	int rpmsg_id;
	struct rpmsg_endpoint *ept;
	int is_unbound;
};

typedef struct {
	uint8_t type;
	uint8_t flag;
	uint16_t len;
	uint8_t *buf;
} bt_rpmsg_buf_t;

static struct rpmsg_xradio_private rpmsg_xr = {
	.rpmsg_id = BT_XRADIO_RPMSG_ID,
	.ept = NULL,
	.is_unbound = 0,
};

static PalOsSemaphore_t bt_xrlink_tx_sem;
static PalOsSemaphore_t bt_xrlink_rx_sem;
static PalOsSemaphore_t bt_rpmsg_rx_semid;
static PalOsQueue_t     bt_rpmsg_rx_queueid;
static PalOsThread_t    bt_xlink_rx_thread; // Linux -> RTOS
static PalOsThread_t    bt_xlink_ept_thread;

/*! \brief convert uint32_t to little endian byte stream, incrementing four bytes. */
#define UINT32_TO_BSTREAM_BT(p, n)   {*(p) = (uint8_t)(n); *(p+1) = (uint8_t)((n) >> 8); \
                                  *(p+2) = (uint8_t)((n) >> 16); *(p+3) = (uint8_t)((n) >> 24);}
#define BT_BYTES_TO_UINT32(n, p)	 {n = ((uint32_t)(p)[0] + ((uint32_t)(p)[1] << 8) + \
									 ((uint32_t)(p)[2] << 16) + ((uint32_t)(p)[3] << 24));}

int bt_rpmsg_xradio_transmit(void *data, uint32_t len)
{
	int ret;
	uint8_t *ptk;

#if XRLINK_DEBUG
	printf("rpmsg tx: ");
	for (int i = 0; i <len; ++i)
		printf(" 0x%x,", *((uint8_t *)(data) + i));
	printf("\n");
#endif

	if (len > BT_XRADIO_RPMSG_DATA_LEN) {
		printf("rpmsg_send too long: %d\n", len);
		return -1;
	}
	ptk = malloc(len + 5);
	ptk[0] = 7;
	UINT32_TO_BSTREAM_BT(ptk+1, len);
	memcpy(ptk+5, data, len);
	ret = openamp_rpmsg_send(rpmsg_xr.ept, ptk, len+5);
	if (ret < 0) {
		printf("rpmsg_xradio_transmit:Failed to send data\r\n");
	}
	free(ptk);
	return ret;
}
#if XRLINK_DEBUG
static void bt_rpmsg_print_u8_array(uint8_t *buf, int32_t size)
{
	for (int i = 0; i < size; i++) {
		if (i % 8 == 0) {
			printf("  ");
		}
		if (i % 32 == 0) {
			printf("\n");
		}
		printf("%02x ", buf[i]);
	}
	printf("\n");
}
#endif

static int xrlink_hci_c2h(uint8_t hciType, const uint8_t *pBuffStart, uint32_t buffOffset, uint32_t buffLen)
{
	//bt_print_u8_array(pBuffStart+buffOffset-1, buffLen+1);
	xrbtc_hci_c2h_cb(0, pBuffStart, buffOffset, buffLen);
	bt_rpmsg_xradio_transmit((uint8_t *)pBuffStart+buffOffset-1, buffLen+1);
	return 0;
}

static int xrlink_hci_h2c_cb(uint8_t status, const uint8_t *pBuffStart, uint32_t buffOffset, uint32_t buffLen)
{
	PalOsSemaphoreRelease(&bt_xrlink_tx_sem);
	return 0;
}

static int xrlink_drv_init()
{
	int ret = 0;
	if (!PalOsSemaphoreIsValid(&bt_xrlink_tx_sem))
		ret = PalOsSemaphoreCreate(&bt_xrlink_tx_sem, 1, 1);
	if (ret) {
		printf("tx sem create failed\n");
		return -1;
	}

	if (!PalOsSemaphoreIsValid(&bt_xrlink_rx_sem))
		ret = PalOsSemaphoreCreate(&bt_xrlink_rx_sem, 0, 1);
	if (ret) {
		printf("rx sem create failed\n");
		goto fail_xrlink_rx_sem;
	}
	return 0;

fail_xrlink_rx_sem:
	PalOsSemaphoreDelete(&bt_xrlink_tx_sem);
	PalOsSemaphoreSetInvalid(&bt_xrlink_tx_sem);

	return -1;
}

static int xrlink_drv_deinit()
{
	int ret = 0;
	ret = PalOsSemaphoreDelete(&bt_xrlink_rx_sem);;
	if (ret) {
		printf("rx  sem delete failed\n");
	}
	PalOsSemaphoreSetInvalid(&bt_xrlink_rx_sem);
	ret = PalOsSemaphoreDelete(&bt_xrlink_tx_sem);
	if (ret) {
		printf("rx sem delete failed\n");
	}
	PalOsSemaphoreSetInvalid(&bt_xrlink_tx_sem);

	return 0;
}

static int bt_rpmsg_xradio_ept_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	uint8_t *tmp;
	bt_rpmsg_buf_t bt_rpmsg_rx_buf;
	memset(&bt_rpmsg_rx_buf, 0, sizeof(bt_rpmsg_rx_buf));
	tmp = (uint8_t *)malloc(len);
	memcpy(tmp, data, len);
	bt_rpmsg_rx_buf.buf = tmp;
	bt_rpmsg_rx_buf.len = len;
#if XRLINK_DEBUG
	printf("rpmsg rx: ");
	bt_rpmsg_print_u8_array(data, len);
#endif
	PalOsQueueSend(&bt_rpmsg_rx_queueid, &bt_rpmsg_rx_buf, 1000);
	return 0;
}

static void bt_rpmsg_xradio_unbind_callback(struct rpmsg_endpoint *ept)
{
	rpmsg_xr.is_unbound = 1;

	printf("Remote endpoint is destroyed\n");
}

int bt_rpmsg_xradio_create()
{
	uint32_t src_addr = RPMSG_ADDR_ANY;
	uint32_t dst_addr = RPMSG_ADDR_ANY;

	printf("create %s rpmsg endpoint\r\n", BT_XRADIO_RPMSG_NAME);

	rpmsg_xr.ept = openamp_ept_open_async(BT_XRADIO_RPMSG_NAME, rpmsg_xr.rpmsg_id, src_addr, dst_addr,
					&rpmsg_xr, bt_rpmsg_xradio_ept_callback, bt_rpmsg_xradio_unbind_callback);
	if (!rpmsg_xr.ept) {
		printf("Failed to Create Endpoint\r\n");
		return -1;
	}
	openamp_ept_wait_async(rpmsg_xr.ept);
	printf("creat rpmsg succeed\n");

 	return 0;
}

static int  bt_rpmsg_handler(void *data, size_t len)
{
	uint8_t *ptr = (uint8_t *)data;
	switch(ptr[0])
	{
		case 1:
		{
			uint32_t size;
			BT_BYTES_TO_UINT32(size, ptr+1);
			xrbtc_sdd_init(size);
			break;
		}
		case 2:
		{
			uint32_t size;
			BT_BYTES_TO_UINT32(size, ptr+1);
			xrbtc_sdd_write(ptr+5, size);
			break;
		}
		case 3:
		{
			xrbtc_init();
			xrbtc_enable();
			xrlink_drv_init();
			xrbtc_hci_init((xrbtc_hci_c2h)xrlink_hci_c2h, (xrbtc_hci_h2c_cb)xrlink_hci_h2c_cb);
			break;
		}
		case 4:
		{
			xrbtc_hci_deinit();
			xrlink_drv_deinit();
			xrbtc_disable();
			xrbtc_deinit();
			break;
		}
		case 5:
		{
			break;
		}
		case 6:
		{
			if (PalOsSemaphoreWait(&bt_xrlink_tx_sem, PAL_OS_WAIT_FOREVER) == 0) {
				xrbtc_hci_h2c(ptr[5], ptr+6, 0, *((uint32_t*)(ptr+1)));//rpcID(1B)  + hcidatalen(4B) + hcitype(1B) + hcidata
			}
			break;
		}
		case 7:
		{
			break;
		}
		case 8:
		{
			//mem read;
			int ret;
			uint8_t *rpy_pkt;
			uint32_t addr;
			uint32_t size;

			if (len > BT_XRADIO_RPMSG_DATA_LEN) {
				printf("rpmsg_send too long: %d\n", len);
				len = BT_XRADIO_RPMSG_DATA_LEN;
			}
			BT_BYTES_TO_UINT32(addr, ptr+1);
			BT_BYTES_TO_UINT32(size, ptr+5);
			rpy_pkt = (uint8_t *)malloc(size + 9);
			rpy_pkt[0] = 9;
			UINT32_TO_BSTREAM_BT(rpy_pkt+1, addr);
			UINT32_TO_BSTREAM_BT(rpy_pkt+5, size);
			memcpy(rpy_pkt+9, (uint8_t *)addr, size);
			ret = openamp_rpmsg_send(rpmsg_xr.ept, rpy_pkt, size + 9);
			if (ret < 0) {
				printf("rpmsg_xradio_transmit:Failed to send data\r\n");
			}
			free(rpy_pkt);
			break;
		}
		case 10:
		{
			//mem write;
			uint8_t *data_pkt;
			uint32_t addr;
			uint32_t size;
			if (len > BT_XRADIO_RPMSG_DATA_LEN) {
				printf("rpmsg_send too long: %d\n", len);
				len = BT_XRADIO_RPMSG_DATA_LEN;
			}
			BT_BYTES_TO_UINT32(addr, ptr+1);
			BT_BYTES_TO_UINT32(size, ptr+5);
			data_pkt = (uint8_t *)addr;
			memcpy(data_pkt, ptr+9, size);
			break;
		}
		default :
		{
			printf("[Function:%s, Line:%d] ERRO %d\n", __func__, __LINE__, ptr[0]);
			break;
		}
	}

	return 0;
}

static void bt_xrlink_data_rx_task(void *arg)
{
	bt_rpmsg_buf_t bt_rpmsg_rx_buf;
	while (1) {
		memset(&bt_rpmsg_rx_buf, 0, sizeof(bt_rpmsg_rx_buf));
		PalOsQueueReceive(&bt_rpmsg_rx_queueid, &bt_rpmsg_rx_buf, PAL_OS_WAIT_FOREVER);
		bt_rpmsg_handler(bt_rpmsg_rx_buf.buf, bt_rpmsg_rx_buf.len);
		free(bt_rpmsg_rx_buf.buf);
	}
}

static void bt_xrlink_ept_task(void *arg)
{
	int ret = 0;
	PalOsThread_t thread;

	ret = bt_rpmsg_xradio_create();
	if (ret < 0) {
		printf("rpmsg_xradio_create failed\n");
	}
	printf("bt_rpmsg_xradio_init success\n");

	thread.handle = bt_xlink_ept_thread.handle;
	bt_xlink_ept_thread.handle = NULL;
	PalOsThreadDelete(&thread);
}

int bt_rpmsg_xradio_init(void)
{
	int ret = 0;

	ret = PalOsSemaphoreCreate(&bt_rpmsg_rx_semid, 0, 1);
	if (ret) {
		printf("rpmsg rx sem create failed\n");
		return -1;
	}

	ret = PalOsQueueCreate(&bt_rpmsg_rx_queueid, 4, sizeof(bt_rpmsg_buf_t));
	if (ret) {
		printf("rpmsg rx queue create failed\n");
		goto queue_err;
	}

	bt_xlink_rx_thread.handle = NULL;
	ret = PalOsThreadCreate(&bt_xlink_rx_thread,
					"bt_xlink_rx_thread",
					bt_xrlink_data_rx_task,
					(void *)NULL,
					PALOS_PRIO_NORMAL,
					(1024 * 8));
	if (ret) {
		printf("rx thread create failed\n");
		goto rx_thread_err;
	}

	bt_xlink_ept_thread.handle = NULL;
	ret = PalOsThreadCreate(&bt_xlink_ept_thread,
					"bt_xlink_ept_thread",
					bt_xrlink_ept_task,
					(void *)NULL,
					PALOS_PRIO_NORMAL,
					(1024 * 8));
	if (ret) {
		printf("ept thread create failed\n");
		goto ept_thread_err;
	}
	return 0;

ept_thread_err:
	PalOsThreadDelete(&bt_xlink_rx_thread);

rx_thread_err:
	PalOsQueueDelete(&bt_rpmsg_rx_queueid);

queue_err:
	PalOsSemaphoreDelete(&bt_rpmsg_rx_semid);

	return ret;
}

void bt_rpmsg_xradio_deinit(void)
{
	int ret;

	if (bt_xlink_ept_thread.handle) {
		PalOsThreadDelete(&bt_xlink_ept_thread);
		bt_xlink_ept_thread.handle = NULL;
	}

	if (rpmsg_xr.ept) {
		openamp_ept_close(rpmsg_xr.ept);
		rpmsg_xr.ept = NULL;
	}

	ret = PalOsThreadDelete(&bt_xlink_rx_thread);
	if (ret) {
		printf("rx thread delete failed\n");
	}
	ret = PalOsQueueDelete(&bt_rpmsg_rx_queueid);
	if (ret) {
		printf("rx queue delete failed\n");
	}
	ret = PalOsSemaphoreDelete(&bt_rpmsg_rx_semid);
	if (ret) {
		printf("rx sem delete failed\n");
	}
}

static int bt_rpmsg_bind_cb(struct rpmsg_ept_client *client)
{
	int ret;
	rpmsg_xr.ept = client->ept;

	printf("rpmsg_xradio_create success\n");

	ret = PalOsSemaphoreCreate(&bt_rpmsg_rx_semid, 0, 1);
	if (ret) {
		printf("rpmsg rx sem create failed\n");
		return -1;
	}

	ret = PalOsQueueCreate(&bt_rpmsg_rx_queueid, 4, sizeof(bt_rpmsg_buf_t));
	if (ret) {
		goto fail_rpmsg_rx_queue;
	}

	bt_xlink_rx_thread.handle = NULL;
	ret = PalOsThreadCreate(&bt_xlink_rx_thread,
					"bt_xlink_rx_thread",
					bt_xrlink_data_rx_task,
					(void *)NULL,
					PALOS_PRIO_NORMAL,
					(1024 * 8));
	if (ret) {
		printf("rx thread create failed\n");
		goto fail_xlink_rx_thread;
	}
	printf("bt_rpmsg_xradio_init success\n");

	return 0;

fail_xlink_rx_thread:
	PalOsQueueDelete(&bt_rpmsg_rx_queueid);

fail_rpmsg_rx_queue:
	PalOsSemaphoreDelete(&bt_rpmsg_rx_semid);
	return -1;
}

static int bt_rpmsg_unbind_cb(struct rpmsg_ept_client *client)
{
	PalOsThreadDelete(&bt_xlink_rx_thread);
	PalOsQueueDelete(&bt_rpmsg_rx_queueid);
	PalOsSemaphoreDelete(&bt_rpmsg_rx_semid);
	return 0;
}

int bt_zephyr_rpmsg_xradio_destory()
{
	printf("unbind %s rpmsg endpoint\r\n", BT_XRADIO_RPMSG_NAME);
	rpmsg_client_unbind(BT_XRADIO_RPMSG_NAME);
	return 0;
}


int bt_zephyr_rpmsg_xradio_create()
{
	printf("bind %s rpmsg endpoint\r\n", BT_XRADIO_RPMSG_NAME);

	rpmsg_client_bind(BT_XRADIO_RPMSG_NAME, bt_rpmsg_xradio_ept_callback, bt_rpmsg_bind_cb,
						bt_rpmsg_unbind_cb, 5, NULL);

	return 0;
}

int bt_zephyr_rpmsg_xradio_init(void)
{
	bt_zephyr_rpmsg_xradio_create();
	return 0;
}

int bt_zephyr_rpmsg_xradio_deinit(void)
{
	bt_zephyr_rpmsg_xradio_destory();
	return 0;
}

struct rpbuf_xradio_buffer_entry {
	int controller_id;
	struct rpbuf_buffer *buffer;
	u8 *buffer_tail;
	int buffer_used_len;
	int buffer_free_len;
	u16 buffer_agg_max;
	u16 buffer_agg_count;
	struct list_head list;
};

LIST_HEAD(rpbuf_xradio_buffers);
static hal_mutex_t rpbuf_xradio_buffers_mutex;

static struct rpbuf_xradio_buffer_entry *find_buffer_entry(const char *name)
{
	struct rpbuf_xradio_buffer_entry *buf_entry;

	list_for_each_entry(buf_entry, &rpbuf_xradio_buffers, list) {
		if (0 == strcmp(rpbuf_buffer_name(buf_entry->buffer), name))
			return buf_entry;
	}
	return NULL;
}

static void bt_rpbuf_xradio_buffer_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printf("buffer \"%s\" is available\n", rpbuf_buffer_name(buffer));
}

static int bt_rpbuf_xradio_buffer_rx_cb(struct rpbuf_buffer *buffer, void *data, int data_len, void *priv)
{
	return 0;
}

static int bt_rpbuf_xradio_buffer_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printf("buffer \"%s\": remote buffer destroyed\n", rpbuf_buffer_name(buffer));

	return 0;
}

static const struct rpbuf_buffer_cbs bt_rpbuf_xradio_cbs = {
	.available_cb = bt_rpbuf_xradio_buffer_available_cb,
	.rx_cb = bt_rpbuf_xradio_buffer_rx_cb,
	.destroyed_cb = bt_rpbuf_xradio_buffer_destroyed_cb,
};

static int bt_rpbuf_xradio_create(int controller_id, const char *name, int len)
{
	int ret;
	struct rpbuf_xradio_buffer_entry *buf_entry = NULL;
	struct rpbuf_controller *controller = NULL;
	struct rpbuf_buffer *buffer = NULL;

	hal_mutex_lock(rpbuf_xradio_buffers_mutex);
	buf_entry = find_buffer_entry(name);
	if (buf_entry) {
		printf("Buffer named \"%s\" already exists\n", name);
		hal_mutex_unlock(rpbuf_xradio_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	hal_mutex_unlock(rpbuf_xradio_buffers_mutex);

	buf_entry = hal_malloc(sizeof(struct rpbuf_xradio_buffer_entry));
	if (!buf_entry) {
		RPBUF_LOG(controller_id, name, len,
				"Failed to allocate memory for buffer entry\n");
		ret = -ENOMEM;
		goto err_out;
	}
	buf_entry->controller_id = controller_id;

	controller = rpbuf_get_controller_by_id(controller_id);
	if (!controller) {
		RPBUF_LOG(controller_id, name, len,
				"Failed to get controller%d, controller_id\n", controller_id);
		ret = -ENOENT;
		goto err_free_buf_entry;
	}

	buffer = rpbuf_alloc_buffer(controller, name, len, NULL, &bt_rpbuf_xradio_cbs, NULL);
	if (!buffer) {
		RPBUF_LOG(controller_id, name, len, "rpbuf_alloc_buffer failed\n");
		ret = -ENOENT;
		goto err_free_buf_entry;
	}
	buf_entry->buffer = buffer;
	buf_entry->buffer_tail = NULL;
	buf_entry->buffer_used_len = 0;
	buf_entry->buffer_agg_count = 0;
	buf_entry->buffer_agg_max = BT_RPBUF_BUFFER_TX_AGG_MAX;
	buf_entry->buffer_free_len = rpbuf_buffer_len(buffer);
	//printf("%s %d entry:%p name:%s tail:%p buf:%p len:%d\n", __func__, __LINE__,
			//buf_entry, name, buf_entry->buffer_tail, rpbuf_buffer_va(buffer), rpbuf_buffer_len(buffer));

	rpbuf_buffer_set_sync(buffer, true);

	hal_mutex_lock(rpbuf_xradio_buffers_mutex);
	list_add_tail(&buf_entry->list, &rpbuf_xradio_buffers);
	hal_mutex_unlock(rpbuf_xradio_buffers_mutex);

	return 0;

err_free_buf_entry:
	hal_free(buf_entry);
err_out:
	return ret;
}

static int bt_rpbuf_xradio_destroy(const char *name)
{
	int ret;
	struct rpbuf_xradio_buffer_entry *buf_entry;
	struct rpbuf_buffer *buffer;

	if (rpbuf_xradio_buffers_mutex)
		hal_mutex_lock(rpbuf_xradio_buffers_mutex);

	buf_entry = find_buffer_entry(name);
	if (!buf_entry) {
		printf("Buffer named \"%s\" not found\n", name);
		hal_mutex_unlock(rpbuf_xradio_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	buffer = buf_entry->buffer;

	ret = rpbuf_free_buffer(buffer);
	if (ret < 0) {
		RPBUF_LOG(buf_entry->controller_id,
				rpbuf_buffer_name(buffer),
				rpbuf_buffer_len(buffer),
				"rpbuf_free_buffer failed\n");
		hal_mutex_unlock(rpbuf_xradio_buffers_mutex);
		goto err_out;
	}

	list_del(&buf_entry->list);
	hal_free(buf_entry);

	hal_mutex_unlock(rpbuf_xradio_buffers_mutex);

	return 0;

err_out:
	return ret;
}

#ifdef CONFIG_XRADIO_RPBUF_PERF_TEST
#define RPBUF_AGG_PERF_DBG
#endif
#ifdef RPBUF_AGG_PERF_DBG
static int g_rpbuf_tx_cnt, g_rpbuf_agg_cnt, g_rpbuf_agg_min, g_rpbuf_agg_max;
#define RPBUF_AGG_PERF_DBG_THRESH    5000
#endif

static __inline__ int rpbuf_send_data(struct rpbuf_xradio_buffer_entry *buf_entry)
{
	int ret;
	struct rpbuf_buffer *buffer = buf_entry->buffer;

#ifdef RPBUF_AGG_PERF_DBG
	g_rpbuf_tx_cnt++;
	g_rpbuf_agg_cnt += buf_entry->buffer_agg_count;
	if (buf_entry->buffer_agg_count < g_rpbuf_agg_min)
		g_rpbuf_agg_min = buf_entry->buffer_agg_count;
	else if (g_rpbuf_agg_min == 0)
		g_rpbuf_agg_min = buf_entry->buffer_agg_count;
	if (buf_entry->buffer_agg_count > g_rpbuf_agg_max)
		g_rpbuf_agg_max = buf_entry->buffer_agg_count;
	else if (g_rpbuf_agg_max == 0)
		g_rpbuf_agg_max = buf_entry->buffer_agg_count;

	if (g_rpbuf_tx_cnt >= RPBUF_AGG_PERF_DBG_THRESH) {
		int avg = g_rpbuf_agg_cnt / g_rpbuf_tx_cnt;

		printf("cnt:%d agged:%d %d-%d-%d l:%d b:%d\n", g_rpbuf_tx_cnt, g_rpbuf_agg_cnt,
				g_rpbuf_agg_min, avg, g_rpbuf_agg_max, buf_entry->buffer_used_len, rpbuf_buffer_len(buffer));
		g_rpbuf_tx_cnt = g_rpbuf_agg_cnt = g_rpbuf_agg_min = g_rpbuf_agg_max = 0;
	}
#endif

	ret = rpbuf_transmit_buffer(buffer, 0, buf_entry->buffer_used_len);
	if (ret < 0) {
		printf("ERR buffer \"%s\": rpbuf_transmit_buffer failed, len:%d, ret:%d\n",
			rpbuf_buffer_name(buffer), buf_entry->buffer_used_len, ret);
	}

	buf_entry->buffer_tail = rpbuf_buffer_va(buffer);
	buf_entry->buffer_used_len = 0;
	buf_entry->buffer_agg_count = 0;
	buf_entry->buffer_free_len = rpbuf_buffer_len(buffer);

	return ret;
}

static int bt_rpbuf_xradio_transmit(const char *name, void *data, int data_len, uint8_t force)
{
	int ret = 0;
	struct rpbuf_xradio_buffer_entry *buf_entry;
	struct rpbuf_buffer *buffer;
	int buf_len;

	hal_mutex_lock(rpbuf_xradio_buffers_mutex);
	buf_entry = find_buffer_entry(name);
	if (!buf_entry) {
		printf("Buffer named \"%s\" not found\n", name);
		ret = -EINVAL;
		goto err_out;
	}
	buffer = buf_entry->buffer;

	/*
	 * Before putting data to buffer or sending buffer to remote, we should
	 * ensure that the buffer is available.
	 */
	if (!rpbuf_buffer_is_available(buffer)) {
		RPBUF_LOG(buf_entry->controller_id,
				rpbuf_buffer_name(buffer),
				rpbuf_buffer_len(buffer),
				"buffer not available\n");
		ret = -EACCES;
		goto err_out;
	}

	if (!buf_entry->buffer_tail) {
		buf_entry->buffer_tail = rpbuf_buffer_va(buffer);
	}
	buf_len = rpbuf_buffer_len(buffer);

	if (((buf_entry->buffer_used_len + data_len) > buf_len) || (force && !data_len)) {
		if (buf_entry->buffer_used_len) {
			//printf("rpbuf_xradio_transmit u:%d l:%d bufsz:%d force:%d\n",
				//buf_entry->buffer_used_len, data_len, buf_len, force);
			rpbuf_send_data(buf_entry);
		} else {
			printf("rpbuf_xradio_transmit (with data) failed len:%d bufsz:%d\n", data_len, rpbuf_buffer_len(buffer));
			ret = -EACCES;
			goto err_out;
		}
	}

	memcpy(buf_entry->buffer_tail, data, data_len);
	//printf("%s %d dlen:%d slen:%d addr:%p buf:%p hdr:%p agg:%d\n", __func__, __LINE__,
	//		hdr->len, data_len, buf_entry->buffer_tail, rpbuf_buffer_va(buffer), hdr,
	//		buf_entry->buffer_agg_count);
	//printf("%s %d entry:%p name:%s tail:%p buf:%p agg-max:%d\n", __func__, __LINE__,
	//		buf_entry, name, buf_entry->buffer_tail, rpbuf_buffer_va(buffer), buf_entry->buffer_agg_max);
	buf_entry->buffer_tail += data_len;
	buf_entry->buffer_used_len += data_len;
	buf_entry->buffer_agg_count += 1;
	buf_entry->buffer_free_len = buf_len - buf_entry->buffer_used_len;

	if (force || (buf_entry->buffer_agg_count >= buf_entry->buffer_agg_max)) {
		rpbuf_send_data(buf_entry);
	}

	hal_mutex_unlock(rpbuf_xradio_buffers_mutex);
	return ret;
err_out:
	hal_mutex_unlock(rpbuf_xradio_buffers_mutex);
	return ret;
}

int bt_rpbuf_xradio_txdata(void *data_send, int data_len, uint8_t force)
{
	int ret = 0;

	ret = bt_rpbuf_xradio_transmit(BT_RPBUF_BUFFER_NAME_XRADIO_TX, data_send, data_len, force);
	if (ret < 0) {
		printf("rpbuf_xradio_transmit (with data) failed\n");
	}
	return ret;
}

int bt_rpbuf_xradio_init(void)
{
	int ret = 0;

	ret = bt_rpbuf_xradio_create(0, BT_RPBUF_BUFFER_NAME_XRADIO_TX, BT_RPBUF_BUFFER_LENGTH_XRADIO_TX);
	if (ret < 0) {
		printf("tx bt_rpbuf_xradio_create failed\n");
		return ret;
	}
	ret = bt_rpbuf_xradio_create(0, BT_RPBUF_BUFFER_NAME_XRADIO_RX, BT_RPBUF_BUFFER_LENGTH_XRADIO_RX);
	if (ret < 0) {
		printf("rx bt_rpbuf_xradio_create failed\n");
		bt_rpbuf_xradio_destroy(BT_RPBUF_BUFFER_NAME_XRADIO_TX);
		return ret;
	}

	printf("bt_rpbuf_xradio_create sucess\n");

	return ret;
}

int bt_rpbuf_xradio_deinit(void)
{
	int ret = 0;

	ret = bt_rpbuf_xradio_destroy(BT_RPBUF_BUFFER_NAME_XRADIO_TX);
	if (ret < 0) {
		printf("bt_rpbuf_xradio_destroy failed\n");
	}
	ret = bt_rpbuf_xradio_destroy(BT_RPBUF_BUFFER_NAME_XRADIO_RX);
	if (ret < 0) {
		printf("bt_rpbuf_xradio_destroy failed\n");
	}

	return ret;
}

