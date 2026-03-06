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
#include "xrlink_version.h"
#include "xrlink.h"
#include "command.h"
#include "txrx.h"
#include "wlan.h"
#include "xradio_rpmsg.h"
#include "xradio_rpbuf.h"
#include "kernel/os/os.h"
#ifdef CONFIG_STA_SOFTAP_COEXIST
#include "net_ctrl.h"
#endif
#include "port/xr_types.h"
#ifdef CONFIG_COMPONENTS_PM
#include "pm_devops.h"
#endif

#define XRLINK_NO_WKUP_BH_THRE_DATA_RX_QUEUE    16//16
#define WIFI_DRV_NO_WKUP_BH_THRE_TX_QUEUE       0//10//5//10

#define FMAC_XRLINK__INIT_THREAD_SIZE           (1024)
static XR_OS_Thread_t wlan_fmac_init_thread;

static uint8_t xr_link_enable;
static uint8_t xrlink_tx_pause;
static uint32_t tx_data_pause_flags;
static XR_OS_Mutex_t tx_data_pause_mutex_lock = {XR_OS_INVALID_HANDLE};

#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
static XR_OS_Timer_t xrlink_flush_net2host_timer;
static XR_OS_Mutex_t xrlink_flush_net2host_timer_lock = {XR_OS_INVALID_HANDLE};
#define XRLINK_FLUSH_NET2HOST_TIME_MS    50 // need > 10 ms

int xr_rxq_queued_num(void);

static void xrlink_flush_net2host_timer_callback(void *arg)
{
	SLAVE_WLAN_DBG("timeout to flush buf\n"); //SLAVE_WLAN_DBG
	xrlink_flush_send_data2host();
}

static int xrlink_start_flush_net2host_timer(uint32_t time_ms)
{
	static uint32_t last_st_time = 0;
	uint32_t timeout;

	if (!xr_link_enable) {
		return -1;
	}
	timeout = last_st_time + ((XRLINK_FLUSH_NET2HOST_TIME_MS - 10) * (HZ / 1000));
	if (!time_after(jiffies, timeout))
		return 0;

	XR_OS_MutexLock(&xrlink_flush_net2host_timer_lock, XR_OS_WAIT_FOREVER);
	if (time_ms == 0) {
		if (XR_OS_TimerIsValid(&xrlink_flush_net2host_timer)) {
			XR_OS_TimerStop(&xrlink_flush_net2host_timer);
			XR_OS_TimerDelete(&xrlink_flush_net2host_timer);
			SLAVE_WLAN_DBG("Stoped xrlink_flush_net2host_timer!\n");
		}
		XR_OS_MutexUnlock(&xrlink_flush_net2host_timer_lock);
		return 0;
	}
	if (XR_OS_TimerIsActive(&xrlink_flush_net2host_timer)){
		XR_OS_TimerChangePeriod(&xrlink_flush_net2host_timer, time_ms);
	} else {
		if (XR_OS_TimerIsValid(&xrlink_flush_net2host_timer)) {
			XR_OS_TimerStop(&xrlink_flush_net2host_timer);
			XR_OS_TimerDelete(&xrlink_flush_net2host_timer);
		}
		if (XR_OS_TimerCreate(&xrlink_flush_net2host_timer, XR_OS_TIMER_ONCE,
		                   xrlink_flush_net2host_timer_callback,
		                   NULL, time_ms) != XR_OS_OK) {
			SLAVE_WLAN_ERR("timer create failed\n");
			XR_OS_MutexUnlock(&xrlink_flush_net2host_timer_lock);
			return -1;
		}
	}
	last_st_time = jiffies;
	SLAVE_WLAN_INF("flush_net2host_timer start %d!\n", time_ms);
	XR_OS_TimerStart(&xrlink_flush_net2host_timer);
	XR_OS_MutexUnlock(&xrlink_flush_net2host_timer_lock);

	return 0;
}

static void xrlink_stop_flush_net2host_timer(void)
{
	if (!xr_link_enable) {
		return;
	}

	XR_OS_MutexLock(&xrlink_flush_net2host_timer_lock, XR_OS_WAIT_FOREVER);
	if (XR_OS_TimerIsValid(&xrlink_flush_net2host_timer)) {
		XR_OS_TimerStop(&xrlink_flush_net2host_timer);
		XR_OS_TimerDelete(&xrlink_flush_net2host_timer);
		SLAVE_WLAN_DBG("Stoped xrlink_flush_net2host_timer!\n");
	}
	SLAVE_WLAN_INF("flush_net2host_timer stop!\n");
	XR_OS_MutexUnlock(&xrlink_flush_net2host_timer_lock);
}
#endif

// ret == 0: mbuf xrlink free
#ifdef CONFIG_STA_SOFTAP_COEXIST
int xrlink_net_to_host(uint8_t *buff, uint16_t len, int rxq_remain, void *nif, uint8_t mbuf)
#else
int xrlink_net_to_host(uint8_t *buff, uint16_t len, int rxq_remain, uint8_t mbuf)
#endif
{
	uint8_t force_tx = 0;

//	if (!xr_link_enable) {
//		return -1;
//	}

#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
	if ((rxq_remain == 0) && (!buff)) {
		xrlink_stop_flush_net2host_timer();
		return xrlink_flush_send_data2host();
	}

	if (rxq_remain == 0) {
		force_tx = 1;
		xrlink_stop_flush_net2host_timer();
	} else {
		xrlink_start_flush_net2host_timer(XRLINK_FLUSH_NET2HOST_TIME_MS);
	}
#endif

#ifdef CONFIG_STA_SOFTAP_COEXIST
	if (g_wlan_ap_netif == nif) {
		//SLAVE_WLAN_DBG("to host mode: AP\n");
		return xrlink_send_data2host(XR_TYPE_DATA_AP, buff, len, force_tx, mbuf);
	} else {
		//SLAVE_WLAN_DBG("to host mode: STA\n");
		return xrlink_send_data2host(XR_TYPE_DATA, buff, len, force_tx, mbuf);
	}
#else
	return xrlink_send_data2host(XR_TYPE_DATA, buff, len, force_tx, mbuf);
#endif
}

#if (defined CONFIG_WLAN_RAW_PACKET_FEATURE) || (defined CONFIG_WLAN_MONITOR)
int xrlink_send_raw_data(uint8_t *data, uint32_t len, void *info, xrlink_raw_type_t type)
{
	uint8_t force_tx = 1;
	int ret;
#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
	int rxq_remain;
#endif

	if (!xr_link_enable) {
		SLAVE_WLAN_ERR("must init xrlink when call this API: %s\n", __func__);
		return -1;
	}

#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
	rxq_remain = xr_rxq_queued_num();
	if (rxq_remain == 0) {
		force_tx = 1;
		xrlink_stop_flush_net2host_timer();
	} else {
		xrlink_start_flush_net2host_timer(XRLINK_FLUSH_NET2HOST_TIME_MS);
	}
	SLAVE_WLAN_DBG("r:%d f:%d e:%d\n", rxq_remain, force_tx, type);
#endif

	if (type == XRLINK_RAW_DATA_STA_AP_EXP)
		ret = ptc_command_expand_raw_data_upload(data, len, force_tx);
	else
		ret = ptc_command_raw_data_send(data, len, force_tx, type);

	return ret;
}

int xradio_link_reg_raw_data_recv_cb(xrlink_recv_cb cb)
{
	if (!xr_link_enable) {
		SLAVE_WLAN_ERR("must init xrlink when call this API: %s\n", __func__);
		return -1;
	}
	ptc_command_reg_raw_data_recv_cb((pro_raw_recv_cb)cb);

	return 0;
}
#endif

int xradio_link_tx_data_pause(uint32_t flag)
{
	int ret = -2;

	if (!xr_link_enable) {
		return -1;
	}
	XR_OS_MutexLock(&tx_data_pause_mutex_lock, XR_OS_WAIT_FOREVER);
	if (!xrlink_tx_pause) {
		if (!ptc_command_data_rx_pause_event()) {
			xrlink_tx_pause = 1;
			tx_data_pause_flags |= flag;
			ret = 0;
		}
		SLAVE_WLAN_DBG("%s flag:%x flags:%x pause:%d ret:%d\n", __func__, flag, tx_data_pause_flags, xrlink_tx_pause, ret);
	} else {
		tx_data_pause_flags |= flag;
		ret = 0;
	}
	//SLAVE_WLAN_DBG("%s flag:%x flags:%x pause:%d\n", __func__, flag, tx_data_pause_flags, xrlink_tx_pause);
	XR_OS_MutexUnlock(&tx_data_pause_mutex_lock);

	return ret;
}

int xradio_link_tx_data_resume(uint32_t flag)
{
	int ret = 0;

	if (!xr_link_enable) {
		return -1;
	}
	XR_OS_MutexLock(&tx_data_pause_mutex_lock, XR_OS_WAIT_FOREVER);
	tx_data_pause_flags &= ~flag;
	if (tx_data_pause_flags == 0) {
		if (!ptc_command_data_rx_resume_event()) {
			xrlink_tx_pause = 0;
		} else {
			tx_data_pause_flags |= flag;
			ret = -2;
		}
		SLAVE_WLAN_DBG("%s flag:%x flags:%x pause:%d ret:%d\n", __func__, flag, tx_data_pause_flags, xrlink_tx_pause, ret);
	}
	//SLAVE_WLAN_DBG("%s flag:%x flags:%x pause:%d\n", __func__, flag, tx_data_pause_flags, xrlink_tx_pause);
	XR_OS_MutexUnlock(&tx_data_pause_mutex_lock);

	return ret;
}

int xradio_link_wifi_drv_txq_lock(void)
{
	return (tx_data_pause_flags & XRLINK_RX_DATA_PAUSE_FLAG_WIFI_DRV_TXQ);
}

int xrlink_wifi_drv_tx_check_wakeup_bh(size_t queued)
{
	//static uint8_t wakeup_pause = 0;

	if ((xrlink_get_data_tx_queued_num() > XRLINK_NO_WKUP_BH_THRE_DATA_RX_QUEUE) &&
		(queued < WIFI_DRV_NO_WKUP_BH_THRE_TX_QUEUE)) {
		//if (!wakeup_pause && !xradio_link_tx_data_pause(XRLINK_RX_DATA_PAUSE_FLAG_XRLINK_TX))
			//wakeup_pause = 1;
		return false;
	} else {
		//if (wakeup_pause && !xradio_link_tx_data_resume(XRLINK_RX_DATA_PAUSE_FLAG_XRLINK_TX))
			//wakeup_pause = 0;
		return true;
	}
}

void xradio_link_rx_buf_status_ind(uint8_t req, uint32_t mem_sum, uint32_t mem_max)
{
#ifdef RPBUF_NO_CHECK_RSP
	static uint32_t rx_pause_cnt = 0;
	static uint8_t rx_pause = 0;

	if (!xr_link_enable) {
		return;
	}
	if (req && !rx_pause && (mem_sum > (mem_max * 7 / 10))) {
		if (!xradio_link_tx_data_pause(XRLINK_RX_DATA_PAUSE_FLAG_MBUF_LIMIT))
			rx_pause = 1;
		rx_pause_cnt++;
		if (rx_pause_cnt % 100 == 1)
			SLAVE_WLAN_SYSLOG("tx pause cnt:%d %d/%d\n", rx_pause_cnt, mem_sum, mem_max);
	} else if (rx_pause && (mem_sum < (mem_max * 2 / 10))){
		if (!xradio_link_tx_data_resume(XRLINK_RX_DATA_PAUSE_FLAG_MBUF_LIMIT))
			rx_pause = 0;
		if (rx_pause_cnt % 100 == 1)
			SLAVE_WLAN_SYSLOG("tx resume cnt:%d %d/%d\n", rx_pause_cnt, mem_sum, mem_max);
	}
#endif
}

int xradio_link_request_firmware(struct param_wlan_bin *param, void *data, ptc_cmd_dev_op_t type)
{
	if (!xr_link_enable) {
		SLAVE_WLAN_ERR("must init xrlink when call this API: %s\n", __func__);
		return -1;
	}

	return ptc_command_request_firmware(param, data, type);
}

void xradio_link_release_firmware(uint8_t type)
{
	if (!xr_link_enable) {
		SLAVE_WLAN_ERR("must init xrlink when call this API: %s\n", __func__);
		return ;
	}

	ptc_command_release_firmware(type);
}

#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_COMPONENTS_PM)
static int xrlink_suspend(struct pm_device *dev, suspend_mode_t state)
{
	SLAVE_WLAN_INF("pm_mode:%d\n", state);
	if (xr_link_enable == 0)
		xrlink_tranc_early_deinit();

	return 0;
}

static int xrlink_resume(struct pm_device *dev, suspend_mode_t state)
{
	int ret = 0;
	SLAVE_WLAN_INF("pm_mode:%d\n", state);

	if (xr_link_enable == 0)
		ret = xrlink_tranc_early_init();

	return ret;
}

static struct pm_devops xrlink_pm_ops = {
	.suspend_late = xrlink_suspend,
	.resume_early = xrlink_resume,
};
static struct pm_device xrlink_dev = {
	.name = "xrlink",
	.ops = &xrlink_pm_ops,
};

static int xrlink_pm_init(int pm_flag)
{
	/* we don't register pm dev_ops in pm wakeup process */
	if (pm_flag) {
		return 0;
	}

	return pm_devops_register(&xrlink_dev);
}

static int xrlink_pm_deinit(int pm_flag)
{
	/* we don't unregister pm dev_ops in pm wakeup process */
	if (pm_flag) {
		return 0;
	}

	return pm_devops_unregister(&xrlink_dev);
}
#else
static int xrlink_pm_init(int pm_flag)
{
	return 0;
}

static int xrlink_pm_deinit(int pm_flag)
{
	return 0;
}
#endif

static void xrlink_init_thread(void *arg)
{
	SLAVE_WLAN_SYSLOG("xrlink_init_thread start\n");
	int ret = 0;
	int is_ultra_standby = *(int *)arg;

	if (xr_link_enable) {
		SLAVE_WLAN_ERR("xrlink already init:%s\n", XRLINK_VERSION);
		goto pm_init_err;
	}

	SLAVE_WLAN_SYSLOG("xrlink version: %s\n", XRLINK_VERSION);
	ret = xrlink_pm_init(is_ultra_standby);
	if (ret)
		goto pm_init_err;

	ret = xrlink_tranc_early_init();
	if (ret < 0)
		goto tranc_early_err;

	tx_data_pause_flags = 0;
	if (XR_OS_MutexCreate(&tx_data_pause_mutex_lock) != XR_OS_OK)
		goto mutex_err;
#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
	if (XR_OS_MutexCreate(&xrlink_flush_net2host_timer_lock) != XR_OS_OK)
		goto mutex_err;
#endif

	ret = xrlink_tranc_init();
	if (ret)
		goto tranc_err;

	xr_link_enable = 1;

	ret = rpbuf_xradio_init();
	if (ret)
		goto rpbuf_err;

	ret = rpmsg_xradio_init();
	if (ret)
		goto rpmsg_err;

	XR_OS_ThreadDelete(&wlan_fmac_init_thread);
	return ;
rpmsg_err:
	rpbuf_xradio_deinit();
rpbuf_err:
	xr_link_enable = 0;
	xrlink_tranc_deinit();
tranc_err:
mutex_err:
#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
	if (XR_OS_MutexIsValid(&xrlink_flush_net2host_timer_lock))
		XR_OS_MutexDelete(&xrlink_flush_net2host_timer_lock);
#endif
	if (XR_OS_MutexIsValid(&tx_data_pause_mutex_lock))
		XR_OS_MutexDelete(&tx_data_pause_mutex_lock);
	xrlink_tranc_early_deinit();
tranc_early_err:
	xrlink_pm_deinit(is_ultra_standby);
pm_init_err:
	SLAVE_WLAN_ERR("xrlink init fail\n");

	XR_OS_ThreadDelete(&wlan_fmac_init_thread);
}

void wlan_fmac_xrlink_deinit(int is_ultra_standby)
{
	SLAVE_WLAN_SYSLOG("wlan_fmac_xrlink_deinit start\n");
	if (XR_OS_ThreadIsValid(&wlan_fmac_init_thread)) {
		SLAVE_WLAN_WRN("fmac_xrlink_init is running, now delete it");
		XR_OS_ThreadDelete(&wlan_fmac_init_thread);
	}
	if (!xr_link_enable) {
		SLAVE_WLAN_ERR("xrlink not init:%s\n", XRLINK_VERSION);
		return;
	}
#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
	xrlink_stop_flush_net2host_timer();
#endif
	xr_link_enable = 0;
	// remove xrlink net ctrl callback
	wlan_net_ctrl_deinit();

	xrlink_tranc_deinit();

	rpbuf_xradio_deinit();

	rpmsg_xradio_deinit();

#ifndef XRLINK_SEND_DATA2HOST_USE_THREAD
	XR_OS_MutexDelete(&xrlink_flush_net2host_timer_lock);
#endif
	XR_OS_MutexDelete(&tx_data_pause_mutex_lock);

	xrlink_tranc_early_deinit();

	xrlink_pm_deinit(is_ultra_standby);
}

int wlan_fmac_xrlink_init(int is_ultra_standby)
{
	XR_OS_Status status;
	XR_OS_ThreadSetInvalid(&wlan_fmac_init_thread);

	status = XR_OS_ThreadCreate(&wlan_fmac_init_thread,
					"fmac_xrlink_init",
					xrlink_init_thread,
					&is_ultra_standby,
					XR_OS_PRIORITY_NEAR_ABOVE_NORMAL,
					FMAC_XRLINK__INIT_THREAD_SIZE);
	if (status != XR_OS_OK) {
		TXRX_ERR("fmac_xrlink_init task creat fail %d.\n", status);
		return -1;
	}

	return status;
}
