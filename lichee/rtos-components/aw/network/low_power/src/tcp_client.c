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
#include "tcp_client.h"
#include "lp_rpmsg.h"
#include "lp_ctrl_msg.h"
#include "lp_rtc_watchdog.h"
#include "public.h"

#ifdef CONFIG_COMPONENTS_LOW_POWER_PIR
#include "dev_pir.h"
#endif

#ifdef CONFIG_COMPONENTS_LOW_POWER_POWERKEY
#include "dev_powerkey_wakeup.h"
#endif

 /* server default val */
char tcp_server_ip[MAX_SERVER_INFO_LEN] = "rtpeer.allwinnertech.com";
int tcp_server_port = 3001;

enum {
	WLAN_WAKE_NORMAL = 0,
	WLAN_WAKE_BSS_RECONNECT,
} wlan_wakeup_type;

// 状态机状态枚举
typedef enum {
	STATE_INIT = 0,                // 初始状态
	STATE_WAIT_INFO,               // 等待大核的连接信息
	STATE_WIFI_CONNECTED,          // WiFi已连接
	STATE_SERVER_CONNECTED,        // 服务器已连接
	STATE_SEND_HEARTBEAT,          // 发送心跳包
	STATE_RECV_RESPONSE,           // 等待响应
	STATE_WAKEUP_CPUX,             // 唤醒大核
	STATE_WIFI_DISCONNECTED,       // Wi-Fi断开
	STATE_BSS_LOST,                // BSS LOST处理
	STATE_BSS_RECONNECT_WIFI,      // 重新连接WiFi
	STATE_SERVER_RECONNECT,        // 重新连接服务器
	STATE_ERROR                    // 错误状态
} lp_tcp_state_t;
typedef struct {
	//socket
	int tcp_keepalive_sock;

	//interval
	int tcp_keepalive_interval;

	//server
	int server_connect_error_flags;
	int network_ok_flags;
	int tcp_reconnect_times;

	//thread
	hal_thread_t state_machine_thread;

	//pm
	struct wakelock tcp_keepalive_wakelock;

	// 状态机相关
	lp_tcp_state_t current_state;
	lp_tcp_state_t old_state;
	int need_task_notify_flags;

	// 用于select的参数
	int wait_heart_select_timeout_count;
	char recv_buff[64];
	char client_id_buf[32];

	// save Wi-Fi info to reconnect when bss lost
	uint8_t ssid[WLAN_SSID_MAX_LEN];
	uint8_t ssid_len;
	uint8_t psk[WLAN_PASSPHRASE_MAX_LEN];

	// handle bss lost
	volatile int bss_lost_flags;
	hal_sem_t bss_reconnect_sem;
	hal_sem_t wlan_up_connect_sem;
	int wait_wlan_bss_reconnect_flags;
	int wait_wlan_up_connect_flags;
} tcp_keep;

static tcp_keep lp_tcp_keepalive[1] = { 0 };

extern struct wakelock pm_connect_lock;

// WLAN定时回调
static int wlan_keepalive_wakeup_init(uint16_t type, unsigned int sec, void *arg);
static int wlan_keepalive_wakeup_deinit(void);
static int tcp_wifi_reconnect(void);  // WiFi BSS LOST后重连函数
static void lp_tcp_state_changed(lp_tcp_state_t old, lp_tcp_state_t new);

// 状态转换宏和函数定义
#define LP_TCP_STATE_CHANGE(new_state) do { \
	lp_tcp_keepalive->old_state = lp_tcp_keepalive->current_state; \
	lp_tcp_keepalive->current_state = (new_state); \
	lp_tcp_state_changed(lp_tcp_keepalive->old_state, lp_tcp_keepalive->current_state); \
} while(0)

#define LP_TCP_STATE_GET() (lp_tcp_keepalive->current_state)

static const char *lp_tcp_state_to_string(lp_tcp_state_t state)
{
	switch (state) {
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_WAIT_INFO:
		return "STATE_WAIT_INFO";
	case STATE_WIFI_CONNECTED:
		return "STATE_WIFI_CONNECTED";
	case STATE_SERVER_CONNECTED:
		return "STATE_SERVER_CONNECTED";
	case STATE_SEND_HEARTBEAT:
		return "STATE_SEND_HEARTBEAT";
	case STATE_RECV_RESPONSE:
		return "STATE_RECV_RESPONSE";
	case STATE_WAKEUP_CPUX:
		return "STATE_WAKEUP_CPUX";
	case STATE_WIFI_DISCONNECTED:
		return "STATE_WIFI_DISCONNECTED";
	case STATE_BSS_LOST:
		return "STATE_BSS_LOST";
	case STATE_BSS_RECONNECT_WIFI:
		return "STATE_BSS_RECONNECT_WIFI";
	case STATE_SERVER_RECONNECT:
		return "STATE_SERVER_RECONNECT";
	case STATE_ERROR:
		return "STATE_ERROR";
	default:
		return "UNKNOWN_STATE";
	}
}

static void lp_tcp_state_changed(lp_tcp_state_t old, lp_tcp_state_t new)
{
	LP_LOG_INFO("LP_TCP State Change: %s (%d) -> %s (%d)\n",
		lp_tcp_state_to_string(old), (int)old,
		lp_tcp_state_to_string(new), (int)new);
}

static int lp_tcp_wifi_get_config(void)
{
	wlan_sta_config_t config;  //保存wifi信息
	memset(&config, 0, sizeof(config));
	config.field = WLAN_STA_FIELD_SSID;  //get ssid
	if (wlan_sta_get_config(&config) != 0) {
		LP_LOG_INFO("wlan_sta_get_config config failed\n");
		return -1;
	}
	LP_LOG_INFO("wlan_sta_get config ssid:%s, ssid_len:%d\n", config.u.ssid.ssid, config.u.ssid.ssid_len);
	memcpy(lp_tcp_keepalive->ssid, config.u.ssid.ssid, config.u.ssid.ssid_len);
	lp_tcp_keepalive->ssid_len = config.u.ssid.ssid_len;
	config.field = WLAN_STA_FIELD_PSK;  //get psk
	if (wlan_sta_get_config(&config) != 0) {
		LP_LOG_INFO("wlan_sta_get_config config failed\n");
		return -1;
	}
	LP_LOG_INFO("wlan_sta_get config psk:%s\n", config.u.psk);
	memcpy(lp_tcp_keepalive->psk, config.u.psk, strlen((const char *)config.u.psk));
	return 0;
}

void lp_tcp_low_power_msg_proc(uint32_t event, uint32_t data, void *arg)
{
	int ret = -1;
	uint16_t type = EVENT_SUBTYPE(event);
	struct netif *nif = g_wlan_netif;
	LP_LOG_INFO("Low Power net event: %s\r\n", net_ctrl_msg_type_to_str(type));
	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA && NET_IS_IP4_VALID(nif)) {
			lp_tcp_keepalive->network_ok_flags = 1;
			ret = wlan_ext_low_power_param_set_default(10);
			if (ret == -2) {
				LP_LOG_ERR("Set Dtim 10 invalid arg\n");
			} else if (ret == -1) {
				LP_LOG_ERR("Set Dtim 10 exec failed\n");
			}
			if (lp_tcp_keepalive->state_machine_thread) {  //相关线程起来后再工作
				if (lp_tcp_keepalive->bss_lost_flags == 1) {
					lp_tcp_keepalive->bss_lost_flags = 0;
					LP_TCP_STATE_CHANGE(STATE_WIFI_CONNECTED);
					if (lp_tcp_keepalive->wait_wlan_up_connect_flags == 1)
						hal_sem_post(lp_tcp_keepalive->wlan_up_connect_sem);
					if (lp_tcp_keepalive->wait_wlan_bss_reconnect_flags == 1)
						hal_sem_post(lp_tcp_keepalive->bss_reconnect_sem);
				}
			} else {
				LP_LOG_WARN("state_machine_thread not work!!!\n");
			}
		}
		break;
	case NET_CTRL_MSG_NETWORK_UP:
		LP_LOG_INFO("### Network up\n");
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA) {
			lp_tcp_keepalive->network_ok_flags = 1;
			ret = wlan_ext_low_power_param_set_default(10);
			if (ret == -2) {
				LP_LOG_ERR("Set Dtim 10 invalid arg\n");
			} else if (ret == -1) {
				LP_LOG_ERR("Set Dtim 10 exec failed\n");
			}
			lp_tcp_wifi_get_config();
			LP_TCP_STATE_CHANGE(STATE_WIFI_CONNECTED);
		}
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		LP_LOG_INFO("### Wlan disconnect\n");
		if (lp_tcp_keepalive->state_machine_thread) {  //相关线程起来后再工作
			if (pm_state_get() != PM_STATUS_RUNNING &&
				nif && wlan_if_get_mode(nif) == WLAN_MODE_STA &&
				lp_tcp_keepalive->bss_lost_flags != 1) {  //避免重复触发
				wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, 0, NULL);
				if (pm_wakelocks_acquire(&lp_tcp_keepalive->tcp_keepalive_wakelock,
					PM_WL_TYPE_WAIT_ONCE,
					OS_WAIT_FOREVER)) {
					LP_LOG_ERR("pm_wakelocks_acquire error\n");
				}
				lp_tcp_keepalive->bss_lost_flags = 1;
				lp_tcp_keepalive->network_ok_flags = 0;
				LP_LOG_INFO("### Network disconnect, notify state machine thread to reconnect WiFi\n");
				LP_TCP_STATE_CHANGE(STATE_WIFI_DISCONNECTED);
				if (lp_tcp_keepalive->need_task_notify_flags == 1) {
					xTaskNotifyGive(lp_tcp_keepalive->state_machine_thread);
				}
			}
		} else {
			LP_LOG_WARN("state_machine_thread not work!!!\n");
		}

		break;
	case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
		LP_LOG_INFO("### Wlan ssid not found\n");
		break;
	case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
		LP_LOG_INFO("### Wlan connect loss(BSS LOST)\n");
		break;
	default:
		LP_LOG_WARN("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

/* dns */
static int domain_get_host(char *name, char *res)
{
	struct hostent *hptr = gethostbyname(name);
	if (hptr == NULL) {
		LP_LOG_INFO("gethostbyname error for host: %s: %d\n", name, h_errno);
		return -1;
	}
	LP_LOG_INFO("hostname: %s\n", hptr->h_name);
	memcpy(res, hptr->h_addr_list[0], INET_ADDRSTRLEN);
	LP_LOG_INFO("ipaddress: %s\n", inet_ntop(hptr->h_addrtype,
		hptr->h_addr_list[0],
		res,
		INET_ADDRSTRLEN));
	return 0;
}

static int lp_socket_get(void)
{
	return lp_tcp_keepalive->tcp_keepalive_sock;
}

static void lp_socket_close(int sockfd)
{
	if (sockfd < 0) {
		LP_LOG_ERR("socket err\n");
		return;
	}
	LP_LOG_INFO("close socket = %d\n", sockfd);
	if (!close(sockfd)) {
		LP_LOG_INFO("Socket closed successfully\n");
		lp_tcp_keepalive->tcp_keepalive_sock = -1;
	} else {
		LP_LOG_INFO("Error closing socket: %s\n", strerror(errno));
	}
}

static int lp_set_socket_o_onoblock(int sockfd, int block)
{
	LP_LOG_INFO("lp_set_socket_o_onoblock set mode = %s\n", block ? "nonblock" : "block");
	if (!block) {
		if (lwip_fcntl(sockfd, F_SETFL, lwip_fcntl(sockfd, F_GETFL, 0) & ~O_NONBLOCK) == -1) {
			LP_LOG_INFO("Error setting socket block mode: %s\n", strerror(errno));
			return -1;
		}
	} else {
		if (lwip_fcntl(sockfd, F_SETFL, lwip_fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
			LP_LOG_INFO("Error setting socket block mode: %s\n", strerror(errno));
			return -1;
		}
	}
	return 0;
}

/* connect */
static int tcp_keepalive_connect(void)
{
	struct sockaddr_in server;
	char s_ip[INET_ADDRSTRLEN] = { 0 };
	int sockfd = -1;
	int flags = 0;
	char str_info[32] = { 0 };
	static int retry_times = 0;
retry:
	if (++retry_times > 3) {
		LP_LOG_INFO("Retry_times = 3, connect failue!!\n");
		retry_times = 0;
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		LP_LOG_INFO("socket failure\n");
		return -1;
	}

	/* close Nagle */
	flags = 1;
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags))) {
		LP_LOG_INFO("Error close Nagle: %s\n", strerror(errno));
	}

	/* enable fast ack */
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &flags, sizeof(flags))) {
		LP_LOG_INFO("Error enable Quickack: %s\n", strerror(errno));
	}

	if (lp_set_socket_o_onoblock(sockfd, 1) == -1) {
		lp_socket_close(sockfd);
		return -1;
	}

	/* get domain ip */
	if (!domain_get_host(tcp_server_ip, s_ip)) {
		LP_LOG_INFO("domain_get_host success\n");
	} else {
		LP_LOG_INFO("domain_get_host error\n");
	}

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(tcp_server_port);
	server.sin_addr.s_addr = inet_addr(s_ip);

	if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		if (errno != EINPROGRESS) {
			LP_LOG_INFO("Error connecting to server: %s\n", strerror(errno));
			lp_socket_close(sockfd);
			hal_sleep(1);
			goto retry;
		}
		fd_set write_fds;
		FD_ZERO(&write_fds);
		FD_SET(sockfd, &write_fds);
		struct timeval timeout;
		timeout.tv_sec = 5; //timeout time
		timeout.tv_usec = 0;
		int result = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);
		if (result == -1) {
			LP_LOG_INFO("Error select in connect: %s\n", strerror(errno));
			lp_socket_close(sockfd);
			return -1;
		} else if (result == 0) {
			LP_LOG_INFO("Connection timed out\n");
			lp_socket_close(sockfd);
			hal_sleep(1);
			goto retry;
		} else {
			int error = 0;
			socklen_t len = sizeof(error);
			if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
				LP_LOG_INFO("Error getting socket option: %s\n", strerror(errno));
				lp_socket_close(sockfd);
				hal_sleep(1);
				goto retry;
			}
			if (error != 0) {
				LP_LOG_INFO("Socket error: %s\n", strerror(errno));
				lp_socket_close(sockfd);
				hal_sleep(1);
				goto retry;
			}
		}
	}
	if (lp_set_socket_o_onoblock(sockfd, 0) == -1) {
		lp_socket_close(sockfd);
		return -1;
	}
	retry_times = 0;
	LP_LOG_INFO("tcp connect success\n");

	/* simple authentication demo */
	memset(lp_tcp_keepalive->client_id_buf, 0, sizeof(lp_tcp_keepalive->client_id_buf));
	sprintf(lp_tcp_keepalive->client_id_buf, "%02X:%02X:%02X:%02X:%02X:%02X",
		g_wlan_netif->hwaddr[0],
		g_wlan_netif->hwaddr[1],
		g_wlan_netif->hwaddr[2],
		g_wlan_netif->hwaddr[3],
		g_wlan_netif->hwaddr[4],
		g_wlan_netif->hwaddr[5]);
	LP_LOG_DBG("client_id_buf = %s\n", lp_tcp_keepalive->client_id_buf);

	sprintf(str_info, "REGISTER:%s", lp_tcp_keepalive->client_id_buf);
	if (send(sockfd, str_info, strlen(str_info), 0) >= 0) {
		LP_LOG_INFO("send auth_info = %s, success!\n", str_info);
	} else {
		LP_LOG_ERR("send auth_info = %s, failed!\n", str_info);
	}
	memset(str_info, 0, sizeof(str_info));
	int ret = recv(sockfd, str_info, sizeof(str_info), 0);
	if (ret <= 0) {
		LP_LOG_INFO("recv auth_info failed %d\n", ret);
		close(sockfd);
		return -1;
	} else {
		LP_LOG_INFO("recv auth_info [%s][len:%d] success, start set fw keepalive\n", str_info, ret);
	}

	low_power_set_keepalive_use_strategy(sockfd, 1, DEFAULLT_USE_STRATEGY);
	return sockfd;
}

/* reconnet */
static int tcp_keepalive_reconnect(tcp_keep *tcp_kl)
{
	tcp_keep *ptcp_keepalive = tcp_kl;
	int reconnect_times = ptcp_keepalive->tcp_reconnect_times;

	while (reconnect_times) {
		if (lp_socket_get() >= 0) {
			low_power_set_keepalive_use_strategy(ptcp_keepalive->tcp_keepalive_sock, 0, DEFAULLT_USE_STRATEGY);
			lp_socket_close(ptcp_keepalive->tcp_keepalive_sock);
		}
		LP_LOG_INFO("tcp_keepalive_reconnect try times = %d\n",
			((ptcp_keepalive->tcp_reconnect_times + 1) - reconnect_times));
		ptcp_keepalive->tcp_keepalive_sock = tcp_keepalive_connect();
		if (ptcp_keepalive->tcp_keepalive_sock != -1) {
			// LP_LOG_INFO("tcp_keepalive_reconnect success\n");
			return 0;
		} else {
			if (--reconnect_times == 0) {
				LP_LOG_INFO("can't tcp_keepalive_connect and over\n");
				return -1;
			}
		}
		usleep(1000 * 500);
	}
	return 0;
}

// WiFi BSS LOST后重连函数
static int tcp_wifi_reconnect(void)
{
	uint32_t bss_lost_reconnect_retry_count = 0;
	int bss_reconnect_back_off_time_s = 20;
#ifdef CONFIG_COMPONENTS_LOW_POWER_APP_RTC_WATCH_DOG
	int bss_reconnect_back_off_time_s_tmp = 0;
	int bss_standby_per_time = 0;
#endif
	struct netif *nif = g_wlan_netif;

	LP_LOG_INFO("### Start WiFi reconnect process\n");
	do {
		lp_tcp_keepalive->wait_wlan_up_connect_flags = 1;
		hal_sem_timedwait(lp_tcp_keepalive->wlan_up_connect_sem, pdMS_TO_TICKS(20 * 1000));
		lp_tcp_keepalive->wait_wlan_up_connect_flags = 0;

		if (lp_tcp_keepalive->bss_lost_flags == 0) {
			LP_LOG_INFO("auto connect success\n");
			goto connect_success;
		} else {
			LP_LOG_WARN("auto connect failed, 1\n");
#ifdef CONFIG_COMPONENTS_LOW_POWER_APP_RTC_WATCH_DOG
			bss_reconnect_back_off_time_s_tmp = bss_reconnect_back_off_time_s; //tmp save
		release_again:
#endif
			if (pm_connect_lock.ref) {
				LP_LOG_INFO("pm_wakelocks_release pm_connect_lock\n");
				pm_wakelocks_release(&pm_connect_lock);  //释放20s的重连锁
			}

			if (wlan_ext_request(nif, WLAN_EXT_CMD_SET_STAY_AWAKE_TMO, 0)) { //确保wlan锁释放
				LP_LOG_ERR("WLAN_EXT_CMD_SET_STAY_AWAKE_TMO error\n");
			}

#ifdef CONFIG_COMPONENTS_LOW_POWER_APP_RTC_WATCH_DOG
			//看门狗最长500s，但是休眠最长是1200s，需要拆分来休眠
			if (bss_reconnect_back_off_time_s_tmp > LP_RTC_WATCHDOG_LP_MAX_TIME) {
				bss_standby_per_time = LP_RTC_WATCHDOG_LP_MAX_TIME;
				bss_reconnect_back_off_time_s_tmp = bss_reconnect_back_off_time_s_tmp - LP_RTC_WATCHDOG_LP_MAX_TIME;
			} else {
				bss_standby_per_time = bss_reconnect_back_off_time_s_tmp;
				bss_reconnect_back_off_time_s_tmp = 0;
			}
			LP_LOG_INFO("bss_reconnect_back_off_time_s = %d\n", bss_reconnect_back_off_time_s);
			LP_LOG_INFO("bss_reconnect_back_off_time_s_tmp = %d\n", bss_reconnect_back_off_time_s_tmp);
			LP_LOG_INFO("bss_standby_per_time = %d\n", bss_standby_per_time);
			wlan_keepalive_wakeup_init(WLAN_WAKE_BSS_RECONNECT, bss_standby_per_time, NULL);
#else
			wlan_keepalive_wakeup_init(WLAN_WAKE_BSS_RECONNECT, bss_reconnect_back_off_time_s, NULL);
#endif
			if (lp_tcp_keepalive->tcp_keepalive_wakelock.ref) {
				pm_wakelocks_release(&lp_tcp_keepalive->tcp_keepalive_wakelock);
				LP_LOG_INFO("pm_wakelocks_release\n");
			}
			lp_tcp_keepalive->wait_wlan_bss_reconnect_flags = 1;
			hal_sem_wait(lp_tcp_keepalive->bss_reconnect_sem);
			lp_tcp_keepalive->wait_wlan_bss_reconnect_flags = 0;

			if (lp_tcp_keepalive->bss_lost_flags == 0) {
				LP_LOG_INFO("auto connect success\n");
				goto connect_success;
			}
#ifdef CONFIG_COMPONENTS_LOW_POWER_APP_RTC_WATCH_DOG
			if (bss_reconnect_back_off_time_s_tmp > 0) {
				goto release_again;
			}
#endif

			bss_reconnect_back_off_time_s = bss_reconnect_back_off_time_s * 2;   //倍数退避
			if (bss_reconnect_back_off_time_s > 1200) bss_reconnect_back_off_time_s = 1200;
			LP_LOG_INFO("bss_reconnect_back_off_time_s updated to %d seconds\n", bss_reconnect_back_off_time_s);
		}
		LP_LOG_INFO("bss_lost_reconnect_retry_count == %d\n", bss_lost_reconnect_retry_count++);

		// set scan paramete
		wlan_ext_scan_param_t scan_param;
		memset(&scan_param, 0, sizeof(wlan_ext_scan_param_t));
		scan_param.num_probes = 3;   // 一个信道扫描次数
		scan_param.probe_delay = 80;   //每次扫描发送probe request的间隔时间
		scan_param.min_dwell = 100;  //在一个信道停留的最短时间
		scan_param.max_dwell = 200;  // 在一个信道停留的最长时间
		if (wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SCAN_PARAM, (uint32_t)(uintptr_t)(&scan_param)) == 0) {
			LP_LOG_INFO("set scan_param success\n");
		}

		wlan_ext_mgmt_timeout_and_tries_set_t assoc_param;
		memset(&assoc_param, 0, sizeof(wlan_ext_mgmt_timeout_and_tries_set_t));
		assoc_param.timeout = 1000; //关联超时时间1000ms
		assoc_param.tries = 5; //重试次数5次
		if (wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_ASSOC_TMO_AND_TRIES, (uint32_t)(uintptr_t)(&assoc_param)) == 0) {
			LP_LOG_INFO("set assoc_param success\n");
		} else {
			LP_LOG_ERR("set assoc_param failed\n");
		}

		if (wlan_sta_disable() == 0) {
			LP_LOG_INFO("wlan_sta_disable success\n");
		}

		if (wlan_sta_config(lp_tcp_keepalive->ssid, lp_tcp_keepalive->ssid_len, lp_tcp_keepalive->psk, 0) == 0) {
			LP_LOG_INFO("wlan_sta_config success\n");
		}

		if (wlan_sta_enable() == 0) {
			LP_LOG_INFO("wlan_sta_enable success\n");
		}
	} while (1);

connect_success:
	// 退避时间比较久了，说明很长时间没连上，关闭当前socket连接，否则只是短期断开，继续使用
	if (bss_reconnect_back_off_time_s >= 160 && lp_socket_get() >= 0) {
		lp_socket_close(lp_socket_get());
	}
	LP_LOG_INFO("### WiFi BSS reconnect successfully after %d attempts\n", bss_lost_reconnect_retry_count);
	return 0;
}

// WLAN定时回调
static void wlan_wakeup_callback(void *arg)
{
	LP_LOG_INFO("wlan_wakeup_callback run\n");
	// if (pm_state_get() == PM_STATUS_RUNNING) {  //正式使用可以加上
	// 	LP_LOG_WARN("pm_state_get is PM_STATUS_RUNNING\n");
	// 	return;
	// }
	if (pm_wakelocks_acquire(&(lp_tcp_keepalive->tcp_keepalive_wakelock),
		PM_WL_TYPE_WAIT_ONCE,
		OS_WAIT_FOREVER)) {
		LP_LOG_ERR("pm_wakelocks_acquire error\n");
	}

	// 通知状态机线程进行心跳发送或处理WiFi断连
	if (lp_tcp_keepalive->state_machine_thread) {
		// 根据状态决定下一步操作
		if (lp_tcp_keepalive->bss_lost_flags == 1) {
			// WiFi断连情况下，进入WiFi重连状态
			LP_TCP_STATE_CHANGE(STATE_WIFI_DISCONNECTED);
		} else {
			// 正常唤醒，进入发送心跳状态
			LP_TCP_STATE_CHANGE(STATE_SEND_HEARTBEAT);
		}
		if (lp_tcp_keepalive->need_task_notify_flags == 1) {
			xTaskNotifyGive(lp_tcp_keepalive->state_machine_thread);
		}
	} else {
		LP_LOG_ERR("state_machine_thread is NULL\n");
	}
	wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, lp_tcp_keepalive->tcp_keepalive_interval, NULL);
}

static void wlan_wakeup_bss_reconnetc_callback(void *arg)
{
	LP_LOG_INFO("wlan_wakeup_bss_reconnetc_callback run\n");

	if (pm_wakelocks_acquire(&(lp_tcp_keepalive->tcp_keepalive_wakelock),
		PM_WL_TYPE_WAIT_ONCE,
		OS_WAIT_FOREVER)) {
		LP_LOG_INFO("pm_wakelocks_acquire error\n");
	}

	if (lp_tcp_keepalive->wait_wlan_bss_reconnect_flags == 1)
		hal_sem_post(lp_tcp_keepalive->bss_reconnect_sem);
}

static int wlan_keepalive_wakeup_init(uint16_t type, unsigned int sec, void *arg)
{
	wlan_ext_host_keepalive_param_t param;

	param.time = sec;
	switch (type) {
	case WLAN_WAKE_NORMAL:
		if (sec == 0) {
			param.callback = NULL;
		} else {
			param.callback = wlan_wakeup_callback;
		}
		break;
	case WLAN_WAKE_BSS_RECONNECT:
		if (sec == 0) {
			param.callback = NULL;
		} else {
			param.callback = wlan_wakeup_bss_reconnetc_callback;
		}
		break;
	}
	param.arg = arg;

	LP_LOG_INFO("type %d, set wake after %d\n", type, sec);
	return wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_KEEPALIVE_PRTIOD, (uint32_t)&param);
}

static int wlan_keepalive_wakeup_deinit(void)
{
	wlan_ext_host_keepalive_param_t param;

	param.time = 0;
	param.callback = NULL;
	param.arg = NULL;
	LP_LOG_INFO("wlan_keepalive_wakeup_deinit\n");
	return wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_KEEPALIVE_PRTIOD, (uint32_t)&param);
}

static void lp_tcp_state_machine_thread(void *arg)
{
	LP_LOG_INFO("Start\n");
	fd_set rfds;
	struct timeval tv;
	int retval, maxfd = -1;
	int len = 0;

	lp_tcp_keepalive->tcp_keepalive_interval = TCP_HEART_INTERVAL;

	while (1) {
		switch (lp_tcp_keepalive->current_state) {
		case STATE_INIT:
			LP_LOG_INFO("State: STATE_INIT\n");
			while (lp_tcp_keepalive->network_ok_flags == 0) {
				hal_msleep(2);
			}
			// 检查WiFi连接状态
			if (lp_tcp_keepalive->bss_lost_flags == 1) {
				LP_TCP_STATE_CHANGE(STATE_BSS_RECONNECT_WIFI);
			} else if (lp_tcp_keepalive->server_connect_error_flags == 1) {
				LP_TCP_STATE_CHANGE(STATE_SERVER_RECONNECT);
			} else {
				LP_TCP_STATE_CHANGE(STATE_WIFI_CONNECTED);
			}
			break;
		case STATE_WIFI_CONNECTED:
			// LP_LOG_DBG("State: STATE_WIFI_CONNECTED\n");
			//等待大核信息
#ifdef CONFIG_COMPONENTS_LOW_POWER_APP_RUN_WITH_WERTC_EXIT
			if (lp_rpmsg_get_server_info_status() <= 0) {
				LP_LOG_DBG("Wait Server Info\n");
				hal_msleep(100);
			} else {
				LP_LOG_DBG("lp_rpmsg_get_server_info_status() = %d\n", lp_rpmsg_get_server_info_status());
				lp_rpmsg_set_server_info_status(0); //重置状态
				// update server info
				struct server_info *server_info_p = lp_rpmsg_get_server_info();
				memcpy(tcp_server_ip, server_info_p->extra_info, MAX_SERVER_INFO_LEN);
				tcp_server_port = server_info_p->port;
#endif
				// 检查服务器连接
				if (lp_socket_get() < 0) {
					// 尝试连接服务器
					lp_tcp_keepalive->tcp_keepalive_sock = tcp_keepalive_connect();
					if (lp_tcp_keepalive->tcp_keepalive_sock != -1) {
						LP_LOG_INFO("Server connected successfully\n");
						LP_TCP_STATE_CHANGE(STATE_SERVER_CONNECTED);
					} else {
						LP_LOG_INFO("Server connection failed, will retry\n");
						LP_TCP_STATE_CHANGE(STATE_SERVER_RECONNECT);
					}
				} else {
					LP_TCP_STATE_CHANGE(STATE_SERVER_CONNECTED);
				}

#ifdef CONFIG_COMPONENTS_LOW_POWER_APP_RUN_WITH_WERTC_EXIT
			}
#endif
			break;

		case STATE_SERVER_CONNECTED:
			LP_LOG_INFO("State: STATE_SERVER_CONNECTED\n");

			// 连接成功后直接进入发送心跳状态
			LP_TCP_STATE_CHANGE(STATE_SEND_HEARTBEAT);
			break;

		case STATE_SEND_HEARTBEAT:
			LP_LOG_INFO("State: STATE_SEND_HEARTBEAT\n");
			// 检查网络状态
			if (lp_tcp_keepalive->server_connect_error_flags || lp_tcp_keepalive->bss_lost_flags) {
				LP_LOG_INFO("Network error detected, switch to WiFi disconnected state\n");
				LP_TCP_STATE_CHANGE(STATE_WIFI_DISCONNECTED);
				break;
			}
			LP_LOG_INFO("Sending heartbeat: [%s]\n", lp_tcp_keepalive->client_id_buf);

			len = send(lp_tcp_keepalive->tcp_keepalive_sock,
				lp_tcp_keepalive->client_id_buf,
				strlen(lp_tcp_keepalive->client_id_buf),
				0);

			if (len < 0) {
				LP_LOG_INFO("Send error, socket = %d, %s\n",
					lp_tcp_keepalive->tcp_keepalive_sock,
					strerror(errno));
				LP_TCP_STATE_CHANGE(STATE_SERVER_RECONNECT);
			} else {
				LP_LOG_INFO("Heartbeat sent successfully, %d bytes\n", len);
				lp_tcp_keepalive->wait_heart_select_timeout_count = 0;
				wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, lp_tcp_keepalive->tcp_keepalive_interval, NULL);
				if (lp_tcp_keepalive->tcp_keepalive_wakelock.ref) {
					pm_wakelocks_release(&(lp_tcp_keepalive->tcp_keepalive_wakelock));
					LP_LOG_INFO("pm_wakelocks_release\n");
				}
				lp_rpmsg_set_keepalive_state(1);
				LP_TCP_STATE_CHANGE(STATE_RECV_RESPONSE);
			}
			break;

		case STATE_RECV_RESPONSE:
			if (lp_tcp_keepalive->wait_heart_select_timeout_count == 0) {
				LP_LOG_INFO("State: STATE_RECV_RESPONSE\n");
			}
			// 检查网络状态
			if (lp_tcp_keepalive->server_connect_error_flags || lp_tcp_keepalive->bss_lost_flags) {
				LP_LOG_INFO("Network error detected, switch to WiFi disconnected state\n");
				LP_TCP_STATE_CHANGE(STATE_WIFI_DISCONNECTED);
				break;
			}
			if (lp_tcp_keepalive->tcp_keepalive_sock >= 0) {
				FD_ZERO(&rfds);
				maxfd = lp_tcp_keepalive->tcp_keepalive_sock;
				FD_SET(lp_tcp_keepalive->tcp_keepalive_sock, &rfds);

				tv.tv_sec = 0;
				tv.tv_usec = TCP_SELECT_PER_TIMEOUT_US;  // 设置select超时时间

				retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
				if (retval == -1) {
					LP_LOG_INFO("Select error, %s\n", strerror(errno));
					if (errno == EBADF || errno == EINVAL) {
						LP_LOG_INFO("Socket invalid, need reconnect\n");
						LP_TCP_STATE_CHANGE(STATE_SERVER_RECONNECT);
					}
				} else if (retval == 0) {  // 超时，可能需要重新发送心跳或重新连接
					if (++lp_tcp_keepalive->wait_heart_select_timeout_count > TCP_SELECT_PER_TIMEOUT_MAX) {
						lp_tcp_keepalive->wait_heart_select_timeout_count = 0;
						if (pm_state_get() == PM_STATUS_ACTIVING) {  //自休眠阶段
							LP_LOG_WARN("Receive response timeout when PM_STATUS_ACTIVING, need reconnect\n");
							LP_TCP_STATE_CHANGE(STATE_SERVER_RECONNECT);
						} else {  //唤醒阶段
							LP_LOG_INFO("Receive response timeout when PM_STATUS_RUNNING, do nothing\n");
						}
					}
				} else {  // 有数据可读
					lp_tcp_keepalive->wait_heart_select_timeout_count = 0;  //重置计数
					if (FD_ISSET(lp_tcp_keepalive->tcp_keepalive_sock, &rfds)) {
						len = recv(lp_tcp_keepalive->tcp_keepalive_sock,
							lp_tcp_keepalive->recv_buff,
							sizeof(lp_tcp_keepalive->recv_buff),
							0);
						if (len > 0) {
							LP_LOG_INFO("Received data: [%s]\n", lp_tcp_keepalive->recv_buff);
							// 处理特殊命令
							if (pm_state_get() >= PM_STATUS_SLEEPED &&
								strstr(lp_tcp_keepalive->recv_buff, "wakeup")) {
								LP_LOG_INFO("Received wakeup command\n");
								LP_TCP_STATE_CHANGE(STATE_WAKEUP_CPUX);
								break;
							} else if (strstr(lp_tcp_keepalive->recv_buff, "ok")) {
								LP_LOG_INFO("Received ok response\n");
								lp_rpmsg_set_keepalive_state(1);
							}
							// 设置保活并等待下次唤醒
							wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, lp_tcp_keepalive->tcp_keepalive_interval, NULL);
							if (lp_tcp_keepalive->tcp_keepalive_wakelock.ref) {
								pm_wakelocks_release(&(lp_tcp_keepalive->tcp_keepalive_wakelock));
								LP_LOG_INFO("pm_wakelocks_release\n");
							}
						} else if (len < 0) {
							LP_LOG_INFO("Failed to receive the message:%s[%d]\n", strerror(errno), errno);
							LP_TCP_STATE_CHANGE(STATE_SERVER_RECONNECT);
						} else {
							// 连接关闭
							LP_LOG_INFO("Connection closed by server\n");
							LP_TCP_STATE_CHANGE(STATE_SERVER_RECONNECT);
						}
						memset(lp_tcp_keepalive->recv_buff, 0, sizeof(lp_tcp_keepalive->recv_buff));
					}
				}
			}
			break;

		case STATE_WAKEUP_CPUX:
			LP_LOG_INFO("State: STATE_WAKEUP_CPUX\n");
			// 唤醒大核
			pm_wakelocks_acquire(&(lp_tcp_keepalive->tcp_keepalive_wakelock),
				PM_WL_TYPE_WAIT_ONCE, OS_WAIT_FOREVER);
			pm_wakesrc_relax(lp_tcp_keepalive->tcp_keepalive_wakelock.ws, PM_RELAX_WAKEUP);
			if (lp_tcp_keepalive->tcp_keepalive_wakelock.ref) {
				pm_wakelocks_release(&(lp_tcp_keepalive->tcp_keepalive_wakelock));
				LP_LOG_INFO("pm_wakelocks_release\n");
			}
			lp_rpmsg_init();//？
			LP_TCP_STATE_CHANGE(STATE_RECV_RESPONSE); // 唤醒后回到接收状态
			break;

		case STATE_WIFI_DISCONNECTED:
			LP_LOG_INFO("State: STATE_WIFI_DISCONNECTED\n");
			LP_TCP_STATE_CHANGE(STATE_BSS_RECONNECT_WIFI);
			break;
		case STATE_BSS_LOST:
			LP_LOG_INFO("State: STATE_BSS_LOST\n");
			LP_TCP_STATE_CHANGE(STATE_BSS_RECONNECT_WIFI);
			break;
		case STATE_BSS_RECONNECT_WIFI:
			LP_LOG_INFO("State: STATE_BSS_RECONNECT_WIFI\n");
			// 重置等待计数器
			lp_tcp_keepalive->wait_heart_select_timeout_count = 0;

			// 尝试WiFi重连
			if (tcp_wifi_reconnect() == 0) {
				LP_LOG_INFO("WiFi reconnected successfully\n");
				LP_TCP_STATE_CHANGE(STATE_WIFI_CONNECTED);
			} else {
				LP_LOG_INFO("WiFi reconnection failed, will retry\n");
			}
			break;

		case STATE_SERVER_RECONNECT:
			LP_LOG_INFO("State: STATE_SERVER_RECONNECT\n");
			//更新保活状态
			lp_rpmsg_set_keepalive_state(0);
			//拿锁防止进休眠
			if (pm_wakelocks_acquire(&(lp_tcp_keepalive->tcp_keepalive_wakelock),
				PM_WL_TYPE_WAIT_ONCE,
				OS_WAIT_FOREVER)) {
				LP_LOG_INFO("pm_wakelocks_acquire error\n");
			}
			// 尝试重新连接
			if (tcp_keepalive_reconnect(lp_tcp_keepalive) == 0) {
				LP_LOG_INFO("Server reconnected successfully\n");
				lp_tcp_keepalive->server_connect_error_flags = 0;
				lp_tcp_keepalive->tcp_keepalive_interval = TCP_HEART_INTERVAL;
				LP_TCP_STATE_CHANGE(STATE_SERVER_CONNECTED);
			} else {
				LP_LOG_INFO("Server reconnection failed, will retry after %ds\n", TCP_RECONNECT_INTERVAL);
				lp_tcp_keepalive->server_connect_error_flags = 1;
				lp_tcp_keepalive->tcp_keepalive_interval = TCP_RECONNECT_INTERVAL;
				// 设置下次唤醒时间
				wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, lp_tcp_keepalive->tcp_keepalive_interval, NULL);
				if (lp_tcp_keepalive->tcp_keepalive_wakelock.ref) {
					pm_wakelocks_release(&(lp_tcp_keepalive->tcp_keepalive_wakelock));
					LP_LOG_INFO("pm_wakelocks_release\n");
				}
				// 等待下一次唤醒
				lp_tcp_keepalive->need_task_notify_flags = 1;
				ulTaskNotifyTake(pdTRUE, HAL_WAIT_FOREVER);
				lp_tcp_keepalive->need_task_notify_flags = 0;
			}
			break;

		case STATE_ERROR:
			LP_LOG_INFO("State: STATE_ERROR\n");
			// 错误状态处理
			LP_TCP_STATE_CHANGE(STATE_INIT);
			break;

		default:
			LP_LOG_INFO("Unknown state: %d\n", lp_tcp_keepalive->current_state);
			LP_TCP_STATE_CHANGE(STATE_INIT);
			break;
		}
	}
	hal_thread_stop(NULL);
}

/* init */
static int tcp_keepalive_init(tcp_keep *tcp_kl)
{
	tcp_keep *ptcp_keepalive = tcp_kl;
	memset(ptcp_keepalive, 0, sizeof(tcp_keep));

	ptcp_keepalive->tcp_keepalive_sock = -1;
	ptcp_keepalive->tcp_keepalive_wakelock.name = "lp_tcp_wakelock";
	ptcp_keepalive->tcp_keepalive_wakelock.type = PM_WL_TYPE_WAIT_ONCE;
	ptcp_keepalive->tcp_keepalive_interval = TCP_HEART_INTERVAL;
	ptcp_keepalive->tcp_reconnect_times = TCP_MAX_RECONNECT_ATTEMPTS;
	ptcp_keepalive->server_connect_error_flags = 0;
	ptcp_keepalive->network_ok_flags = 0;
	ptcp_keepalive->bss_lost_flags = 0;
	ptcp_keepalive->wait_heart_select_timeout_count = 0;
	ptcp_keepalive->need_task_notify_flags = 0;
	ptcp_keepalive->wait_wlan_bss_reconnect_flags = 0;
	ptcp_keepalive->wait_wlan_up_connect_flags = 0;

	// 状态机初始化
	ptcp_keepalive->current_state = STATE_INIT;
	ptcp_keepalive->old_state = STATE_INIT;

	ptcp_keepalive->wlan_up_connect_sem = hal_sem_create(0);
	if (ptcp_keepalive->wlan_up_connect_sem == NULL) {
		LP_LOG_ERR("create wlan_up_connect_sem failed\n");
		goto func_err2;
	}
	ptcp_keepalive->bss_reconnect_sem = hal_sem_create(0);
	if (ptcp_keepalive->bss_reconnect_sem == NULL) {
		LP_LOG_ERR("create bss_reconnect_sem failed\n");
		goto func_err1;
	}
	return 0;
func_err1:
	hal_sem_delete(ptcp_keepalive->wlan_up_connect_sem);
func_err2:
	return -1;
}

#ifdef CONFIG_COMPONENTS_PM
static int tcp_lp_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	LP_LOG_INFO("### tcp_lp_suspend run\n");
	return 0;
}

static int tcp_lp_resume(struct pm_device *dev, suspend_mode_t mode)
{
	LP_LOG_INFO("### tcp_lp_resume run\n");
	return 0;
}

static struct pm_devops tcp_lp_ops = {
	.suspend = tcp_lp_suspend,
	.resume = tcp_lp_resume,
};

static struct pm_device tcp_lp_pm = {
	.name = "tcp_lp",
	.ops = &tcp_lp_ops,
};
#endif

int tcp_low_power_keepalive_demo(void)
{
	int ret = -1;
	if (lp_rpmsg_init()) {
		LP_LOG_INFO("%s: lp_rpmsg_init failed\n", __func__);
		return -1;
	}

	lp_ctrl_msg_register(lp_tcp_low_power_msg_proc);

	ret = tcp_keepalive_init(lp_tcp_keepalive);
	if (ret == 0) {
		LP_LOG_INFO("tcp_keepalive_init success\n");
	} else {
		LP_LOG_ERR("tcp_keepalive_init failed\n");
		return -1;
	}

#ifdef CONFIG_COMPONENTS_LOW_POWER_APP_RTC_WATCH_DOG
	if (!lp_rtc_wdt_init()) {
		LP_LOG_INFO("lp_rtc_wdt_init success\n");
		/* if your app need use wdt, you can uncomment it and eeed the dog in the right place */
		// lp_rtc_wdt_start_timeout(90);
	} else {
		LP_LOG_ERR("lp_rtc_wdt_init failed\n");
	}
#endif

#ifdef CONFIG_COMPONENTS_LOW_POWER_PIR
	if (hal_pir_init()) {
		LP_LOG_ERR("hal_pir_init failed\n");
	}
#endif

#ifdef CONFIG_COMPONENTS_LOW_POWER_POWERKEY
	if (hal_powerkey_wakeup_init()) {
		LP_LOG_ERR("hal_powerkey_wakeup_init failed\n");
	}
#endif

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_register(&tcp_lp_pm);
#endif

	lp_tcp_keepalive->state_machine_thread = hal_thread_create(lp_tcp_state_machine_thread,
		NULL,
		"tcp_lp_state_machine",
		2048,
		17);

	if (!lp_tcp_keepalive->state_machine_thread) {
		LP_LOG_ERR("state_machine_thread create error\n");
		goto err_exit2;
	}

	if (pm_task_register(lp_tcp_keepalive->state_machine_thread, PM_TASK_TYPE_WLAN)) {
		LP_LOG_INFO("state_machine_thread pm_task_register error\n");
		goto err_exit1;
	}

	hal_thread_start(lp_tcp_keepalive->state_machine_thread);

	return 0;
err_exit1:
	hal_thread_stop(lp_tcp_keepalive->state_machine_thread);
err_exit2:
	hal_sem_delete(lp_tcp_keepalive->wlan_up_connect_sem);
	hal_sem_delete(lp_tcp_keepalive->bss_reconnect_sem);
	return -1;
}

int tcp_low_power_keepalive_demo_deinit(void)
{
	LP_LOG_INFO("tcp_low_power_keepalive_demo_deinit start\n");

	// 停止唤醒定时器
	wlan_keepalive_wakeup_deinit();

	// 重置保活状态
	lp_rpmsg_set_keepalive_state(0);

#ifdef CONFIG_COMPONENTS_LOW_POWER_APP_RTC_WATCH_DOG
	lp_rtc_wdt_deinit();
#endif

#ifdef CONFIG_COMPONENTS_LOW_POWER_PIR
	if (hal_pir_deinit()) {
		LP_LOG_ERR("hal_pir_deinit failed\n");
	}
#endif

#ifdef CONFIG_COMPONENTS_LOW_POWER_POWERKEY
	if (hal_powerkey_wakeup_deinit()) {
		LP_LOG_ERR("hal_powerkey_wakeup_deinit failed\n");
	}
#endif

	// 关闭socket
	if (lp_tcp_keepalive->tcp_keepalive_sock >= 0) {
		low_power_set_keepalive_use_strategy(lp_tcp_keepalive->tcp_keepalive_sock, 0, DEFAULLT_USE_STRATEGY);
		lp_socket_close(lp_tcp_keepalive->tcp_keepalive_sock);
	}

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&tcp_lp_pm);
#endif

	// 停止状态机线程
	if (lp_tcp_keepalive->state_machine_thread != NULL) {
		if (pm_task_unregister(lp_tcp_keepalive->state_machine_thread)) {
			LP_LOG_INFO("state_machine_thread pm_task_unregister error\n");
		}
		hal_thread_stop(lp_tcp_keepalive->state_machine_thread);
		lp_tcp_keepalive->state_machine_thread = NULL;
	}

	// 清理信号量
	if (lp_tcp_keepalive->wlan_up_connect_sem != NULL) {
		hal_sem_delete(lp_tcp_keepalive->wlan_up_connect_sem);
		lp_tcp_keepalive->wlan_up_connect_sem = NULL;
		LP_LOG_INFO("wlan_up_connect_sem deleted\n");
	}

	if (lp_tcp_keepalive->bss_reconnect_sem != NULL) {
		hal_sem_delete(lp_tcp_keepalive->bss_reconnect_sem);
		lp_tcp_keepalive->bss_reconnect_sem = NULL;
		LP_LOG_INFO("bss_reconnect_sem deleted\n");
	}

	// 清理rpmsg
	lp_rpmsg_deinit();

	LP_LOG_INFO("tcp_low_power_keepalive_demo_deinit complete\n");
	return 0;
}
// FINSH_FUNCTION_EXPORT_CMD(tcp_low_power_keepalive_demo, tcp_low_power_keepalive_demo, tcp_low_power_keepalive_demo)
