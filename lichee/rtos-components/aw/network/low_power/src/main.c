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
#include "mqtt_client.h"
#include "tcp_client.h"
#include "wlan_fw_keepalive_client.h"
#include "lp_rpmsg.h"
#include "lp_ctrl_msg.h"
#include "public.h"
#include "sys_ctrl.h"
#include "wlan.h"

static int low_power_status = 0;

int low_power_wlan_set_fw_try_wait_tcp_pkt_tmo(uint16_t timeout_ms)
{
	int ret;
	uint32_t tmo = timeout_ms;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_FW_TRY_RX_TCP_PKT_TMO, tmo);
	if (ret) {
		LP_LOG_INFO("%s: set fail %d\n", __func__, ret);
	}

	return ret;
}

int low_power_set_keepalive_use_strategy(int socket, int enable, int strategy)
{
	int ret = 0;
	struct sockaddr_in ser_addr;
	socklen_t addr_len = sizeof(ser_addr);
	wlan_ext_host_keepalive_link_config_t param;
	static int init_status = 0;

	if (enable) {
		if (init_status != LOW_POWER_STRATEGY_DISABLE) {
			LP_LOG_INFO("%s already use strategy %d, please disable first.\n", __func__, init_status);
			return -1;
		}

		if (strategy == LOW_POWER_STRATEGY_NOT_WAIT_SER_REPLY) {
			ret = getsockname(socket, (struct sockaddr *)&ser_addr, &addr_len);
			if (ret) {
				LP_LOG_INFO("get socket ip fail %d\n", ret);
				LP_LOG_INFO("enable wlan fw store frame fail.\n");
				return ret;
			}

			param.enable = 1;
			param.ip_addr = ser_addr.sin_addr.s_addr;
			param.port = ntohs(ser_addr.sin_port);
			ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_HOST_KEEPALIVE_NETLINK, (uint32_t)(uintptr_t)&param);
			if (ret) {
				LP_LOG_INFO("%s: command invalid arg\n", __func__);
				LP_LOG_INFO("enable wlan fw store frame fail.\n");
				return ret;
			}
		} else if (strategy == LOW_POWER_STRATEGY_WAIT_SER_REPLY) {
			wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_DISABLE_PS_IN_RESUME, (uint32_t)enable);
			lwip_set_suspend_check_ramheap(enable);
		} else {
			LP_LOG_INFO("%s: strategy %d invalid arg\n", __func__, strategy);
			return -1;
		}

		init_status = strategy;
		LP_LOG_INFO("%s enable strategy %d sucess\n", __func__, strategy);
	} else {
		if (init_status == LOW_POWER_STRATEGY_DISABLE) {
			LP_LOG_INFO("%s: low power strategy is disable now.\n", __func__);
			return 0;
		}

		if (init_status == LOW_POWER_STRATEGY_NOT_WAIT_SER_REPLY) {
			memset(&param, 0, sizeof(param));
			wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_HOST_KEEPALIVE_NETLINK, (uint32_t)(uintptr_t)&param);
		} else if (init_status == LOW_POWER_STRATEGY_WAIT_SER_REPLY) {
			wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_DISABLE_PS_IN_RESUME, (uint32_t)enable);
			lwip_set_suspend_check_ramheap(enable);
		}

		LP_LOG_INFO("%s: disable strategy %d sucess\n", __func__, init_status);
		init_status = LOW_POWER_STRATEGY_DISABLE;
	}

	return 0;
}

#ifdef CONFIG_COMPONENTS_LOW_POWER_APP_NOTIFY
static int notify_id = 0;
static int low_power_notify(suspend_mode_t mode, pm_event_t event, void *arg)
{
	LP_RPMSG_LOG("low_power_notify notify event = %d\n", event);
	switch (event) {
	case PM_EVENT_PERPARED:

		break;
	case PM_EVENT_FINISHED:

		break;
	case PM_EVENT_SYS_FINISHED:

		break;
	default:
		break;
	}
	return 0;
}

static pm_notify_t lp_notify = {
	.name = "lp_notify",
	.pm_notify_cb = low_power_notify,
	.arg = NULL,
};

static int low_power_app_notify_register(void)
{
	notify_id = pm_notify_register(&lp_notify);
	if (notify_id < 0) {
		LP_RPMSG_LOG("pm_notify_register failed\n");
		return -1;
	}
	LP_RPMSG_LOG("pm_notify_register success\n");
	return 0;
}

static int low_power_app_notify_unregister(void)
{
	if (pm_notify_unregister(notify_id)) {
		LP_RPMSG_LOG("pm_notify_unregister failed\r\n");
		return -1;
	}
	LP_RPMSG_LOG("pm_notify_unregister success\n");
	return 0;
}
#endif /* CONFIG_COMPONENTS_LOW_POWER_APP_NOTIFY */

static int low_power_keepalive_demo_start(void)
{
	LP_LOG_INFO("low_power_keepalive_demo_start\n");
#if defined(CONFIG_COMPONENTS_LOW_POWER_APP_NOTIFY)
	if (low_power_app_notify_register()) {
		LP_LOG_INFO("low_power_app_notify_register failed\n");
		return -1;
	}
#endif

#if defined(CONFIG_COMPONENTS_LOW_POWER_APP_MQTT)
	mqtt_low_power_keepalive_demo();
#elif defined(CONFIG_COMPONENTS_LOW_POWER_APP_TCP)
	tcp_low_power_keepalive_demo();
#elif defined(CONFIG_COMPONENTS_LOW_POWER_WLAN_FW_TCP)
	wlan_fw_low_power_keepalive_demo();
#else
	LP_LOG_INFO("no keepalive\n");
#endif
	low_power_status = 1;
	return 0;
}

static int low_power_keepalive_demo_stop(void)
{
	LP_LOG_INFO("low_power_keepalive_demo_stop\n");
#if defined(CONFIG_COMPONENTS_LOW_POWER_APP_NOTIFY)
	if (low_power_app_notify_unregister()) {
		LP_LOG_INFO("low_power_app_notify_unregister failed\n");
	}
#endif

#if defined(CONFIG_COMPONENTS_LOW_POWER_APP_MQTT)
	mqtt_low_power_keepalive_demo_deinit();
#elif defined(CONFIG_COMPONENTS_LOW_POWER_APP_TCP)
	tcp_low_power_keepalive_demo_deinit();
#elif defined(CONFIG_COMPONENTS_LOW_POWER_WLAN_FW_TCP)
	wlan_fw_low_power_keepalive_demo_deinit();
#else
	LP_LOG_INFO("no keepalive\n");
#endif
	low_power_status = 0;
	return 0;
}

int low_power_app(void)
{
	int ret = -1;
	LP_LOG_INFO("low_power_app\n");
	ret = low_power_keepalive_demo_start();
	if (ret) {
		low_power_keepalive_demo_stop();
	}
	return 0;
}

