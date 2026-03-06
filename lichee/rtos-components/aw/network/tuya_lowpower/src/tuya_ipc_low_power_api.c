/*
 * tuya_ipc_lowpower_api.c
 */
#include <console.h>
#include <hal_mutex.h>
#include <hal_mem.h>
#include <rpbuf.h>
 // #include <mqueue.h>
#include "lp_auth.h"
#include "lp_cli.h"
#include "tuya_ipc_low_power_api.h"

#include "pm_wakelock.h"
#include "pm_wakesrc.h"
#include "pm_task.h"
#include "pm_mem.h"

#include <pm_base.h>
#include <pm_testlevel.h>
#include <pm_state.h>
#include <pm_rpcfunc.h>

#include "errno.h"
#include <hal_osal.h>
#include <semaphore.h>

#include "net_ctrl.h"
#include "net/wlan/wlan_ext_req.h"

#include "tuya_rpmsg.h"
#include "tuya_rpbuf.h"
#include "tuya_com.h"
#include "lp_ctrl_msg.h"
#ifdef CONFIG_TUYA_RTC_WATCH_DOG
#include "tuya_rtc_watchdog.h"
#endif

#define TUYA_RECV_BUF_SIZE              (256)
#define TUYA_KEEPALIVE_TIME             (60)
#define TUYA_RECONNECT_TIME             (300)
#define IDLE_SELECT_TIMEOUT_MAX         (5)  //唤醒后，大核没连上最多 IDLE_SELECT_TIMEOUT_MAX * TUYA_SELECT_TIMEOUT 后重启
#define TUYA_SERVER_RECONNECT_MAX       (10)
#define TUYA_SELECT_TIMEOUT             (15)  //总时长15s
#define TUYA_SELECT_PER_TIMEOUT_US      (5*1000)  //每次5ms
#define TUYA_SELECT_PER_TIMEOUT_MAX     (TUYA_SELECT_TIMEOUT*1000*1000/TUYA_SELECT_PER_TIMEOUT_US) //超时次数 3000次
#define TUYA_RTC_WATCHDOG_LP_MAX_TIME   (480) //最长500s，预留20s空挡

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
} tuya_lp_state_t;

typedef struct {
	TUYA_IP_ADDR_T ip;
	int port;
	char devid[COMM_LEN];
	char local_key[COMM_LEN];
} tuya_lowpower_info;

typedef struct tuya_pcb {
#define WAKEDATE_LEN 36
#define HEART_BEAT_LEN 12
	//tuya
	char wakeData[WAKEDATE_LEN];
	int wake_data_len;
	char heart_beat[HEART_BEAT_LEN];
	int heart_beat_len;
	tuya_lowpower_info tuya_lp_info;
	lp_auth_dev_msg_s g_devMsg;
	unsigned int low_power_wake_up_date_seed;

	// pm
	struct wakelock tuya_wakelock;

	//rpmsg
	int tuya_lp_rpmsg_init_flags;
	volatile int tuya_lp_rpmsg_msg_flags;

	//tcp
	int low_power_socket;

	// 状态机相关
	tuya_lp_state_t current_state;
	tuya_lp_state_t old_state;

	int tuya_lp_tcp_keepalive_interval;
	int need_task_notify_flags;

	//thread
	hal_thread_t state_machine_thread;
	hal_thread_t wait_start_thread;
	hal_thread_t delay_thread;

	// save Wi-Fi info to reconnect when bss lost
	uint8_t ssid[WLAN_SSID_MAX_LEN];
	uint8_t ssid_len;
	uint8_t psk[WLAN_PASSPHRASE_MAX_LEN];

	// handle bss lost
	volatile int bss_lost_flags;
	hal_sem_t bss_reconnect_sem;
	int wait_wlan_bss_reconnect_flags;
	hal_sem_t wlan_up_connect_sem;
	int wait_wlan_up_connect_flags;

	int idle_select_timeout_count;
	int wait_heart_select_timeout_count;
	int recv_wakeup_flags;
}tuya_pcb_t;

static tuya_pcb_t lp_tuya_pcb = { 0 };

extern struct wakelock pm_connect_lock;
static int wlan_keepalive_wakeup_init(uint16_t type, unsigned int sec, void *arg);
static int tuya_wifi_reconnect(void);
static void tuya_ipc_low_power_server_close();
static void tuya_lp_state_machine_thread(void *arg);
static void tuya_lp_state_changed(tuya_lp_state_t old, tuya_lp_state_t new);

#define TUYA_LP_STATE_CHANGE(new_state) do { \
	lp_tuya_pcb.old_state = lp_tuya_pcb.current_state; \
	lp_tuya_pcb.current_state = (new_state); \
	tuya_lp_state_changed(lp_tuya_pcb.old_state, new_state); \
} while(0)

#define TUYA_LP_GET_OLD_STATE() (lp_tuya_pcb.old_state)

static const char *tuya_lp_state_to_string(tuya_lp_state_t state)
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

static void tuya_lp_state_changed(tuya_lp_state_t old, tuya_lp_state_t new)
{
	TUYA_LOG_INFO("LP_TUYA State Will change: %s (%d) -> %s (%d)\n",
		tuya_lp_state_to_string(old), (int)old,
		tuya_lp_state_to_string(new), (int)new);
}

static int tuya_wifi_get_config(void)
{
	wlan_sta_config_t config;  //保存wifi信息
	memset(&config, 0, sizeof(config));
	config.field = WLAN_STA_FIELD_SSID;  //get ssid
	if (wlan_sta_get_config(&config) != 0) {
		TUYA_LOG_ERR("wlan_sta_get_config config failed\n");
		return -1;
	}
	TUYA_LOG_INFO("wlan_sta_get config ssid:%s, ssid_len:%d\n", config.u.ssid.ssid, config.u.ssid.ssid_len);
	memcpy(lp_tuya_pcb.ssid, config.u.ssid.ssid, config.u.ssid.ssid_len);
	lp_tuya_pcb.ssid_len = config.u.ssid.ssid_len;
	config.field = WLAN_STA_FIELD_PSK;  //get psk
	if (wlan_sta_get_config(&config) != 0) {
		TUYA_LOG_ERR("wlan_sta_get_config config failed\n");
		return -1;
	}
	TUYA_LOG_INFO("wlan_sta_get config psk:%s\n", config.u.psk);
	memcpy(lp_tuya_pcb.psk, config.u.psk, strlen((const char *)config.u.psk));
	return 0;
}

static int tuya_wifi_reconnect(void)
{
	uint32_t bss_lost_reconnect_retry_count = 0;
	int bss_reconnect_back_off_time_s = 20;
#ifdef CONFIG_TUYA_RTC_WATCH_DOG
	int bss_reconnect_back_off_time_s_tmp = 0;
	int bss_standby_per_time = 0;
#endif
	struct netif *nif = g_wlan_netif;

	TUYA_LOG_INFO("### Start WiFi reconnect process\n");
	do {
		lp_tuya_pcb.wait_wlan_up_connect_flags = 1;
		hal_sem_timedwait(lp_tuya_pcb.wlan_up_connect_sem, pdMS_TO_TICKS(20 * 1000));
		lp_tuya_pcb.wait_wlan_up_connect_flags = 0;

		if (lp_tuya_pcb.bss_lost_flags == 0) {
			TUYA_LOG_INFO("auto connect success\n");
			goto connect_success;
		} else {
			TUYA_LOG_WARN("auto connect faild, 1\n");
#ifdef CONFIG_TUYA_RTC_WATCH_DOG
			bss_reconnect_back_off_time_s_tmp = bss_reconnect_back_off_time_s; //tmp save
		release_again:
#endif
			if (pm_connect_lock.ref) {
				TUYA_LOG_INFO("pm_wakelocks_release pm_connect_lock\n");
				pm_wakelocks_release(&pm_connect_lock);  //释放20s的重连锁
			}

			if (wlan_ext_request(nif, WLAN_EXT_CMD_SET_STAY_AWAKE_TMO, 0)) { //确保wlan锁释放
				TUYA_LOG_ERR("WLAN_EXT_CMD_SET_STAY_AWAKE_TMO error\n");
			}

#ifdef CONFIG_TUYA_RTC_WATCH_DOG
			//看门狗最长500s，但是休眠最长是1200s，需要拆分来休眠
			if (bss_reconnect_back_off_time_s_tmp > TUYA_RTC_WATCHDOG_LP_MAX_TIME) {
				bss_standby_per_time = TUYA_RTC_WATCHDOG_LP_MAX_TIME;
				bss_reconnect_back_off_time_s_tmp = bss_reconnect_back_off_time_s_tmp - TUYA_RTC_WATCHDOG_LP_MAX_TIME;
			} else {
				bss_standby_per_time = bss_reconnect_back_off_time_s_tmp;
				bss_reconnect_back_off_time_s_tmp = 0;
			}
			TUYA_LOG_INFO("bss_reconnect_back_off_time_s = %d\n", bss_reconnect_back_off_time_s);
			TUYA_LOG_INFO("bss_reconnect_back_off_time_s_tmp = %d\n", bss_reconnect_back_off_time_s_tmp);
			TUYA_LOG_INFO("bss_standby_per_time = %d\n", bss_standby_per_time);
			wlan_keepalive_wakeup_init(WLAN_WAKE_BSS_RECONNECT, bss_standby_per_time, NULL);
#else
			wlan_keepalive_wakeup_init(WLAN_WAKE_BSS_RECONNECT, bss_reconnect_back_off_time_s, NULL);
#endif
			if (lp_tuya_pcb.tuya_wakelock.ref) {
				pm_wakelocks_release(&lp_tuya_pcb.tuya_wakelock);
				TUYA_LOG_INFO("pm_wakelocks_release\n");
			}
			lp_tuya_pcb.wait_wlan_bss_reconnect_flags = 1;
			hal_sem_wait(lp_tuya_pcb.bss_reconnect_sem);
			lp_tuya_pcb.wait_wlan_bss_reconnect_flags = 0;

			if (lp_tuya_pcb.bss_lost_flags == 0) {
				TUYA_LOG_INFO("auto connect success\n");
				goto connect_success;
			}
#ifdef CONFIG_TUYA_RTC_WATCH_DOG
			if (bss_reconnect_back_off_time_s_tmp > 0) {
				goto release_again;
			}
#endif

			bss_reconnect_back_off_time_s = bss_reconnect_back_off_time_s * 2;   //倍数退避
			if (bss_reconnect_back_off_time_s > 1200) bss_reconnect_back_off_time_s = 1200;
		}
		TUYA_LOG_INFO("bss_lost_reconnect_retry_count == %d\n", bss_lost_reconnect_retry_count++);

		// set scan paramete
		wlan_ext_scan_param_t scan_param;
		memset(&scan_param, 0, sizeof(wlan_ext_scan_param_t));
		scan_param.num_probes = 3;   // 一个信道扫描次数
		scan_param.probe_delay = 80;   //每次扫描发送probe request的间隔时间
		scan_param.min_dwell = 100;  //在一个信道停留的最短时间
		scan_param.max_dwell = 200;  // 在一个信道停留的最长时间
		if (wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SCAN_PARAM, (uint32_t)(uintptr_t)(&scan_param)) == 0) {
			TUYA_LOG_INFO("set scan_param success\n");
		}

		wlan_ext_mgmt_timeout_and_tries_set_t assoc_param;
		memset(&assoc_param, 0, sizeof(wlan_ext_mgmt_timeout_and_tries_set_t));
		assoc_param.timeout = 1000; //关联超时时间1000ms
		assoc_param.tries = 5; //重试次数5次
		if (wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_ASSOC_TMO_AND_TRIES, (uint32_t)(uintptr_t)(&assoc_param))) {
			TUYA_LOG_INFO("set assoc_param ssuccess\n");
		}

		if (wlan_sta_disable() == 0) {
			TUYA_LOG_INFO("wlan_sta_disable success\n");
		}

		if (wlan_sta_config(lp_tuya_pcb.ssid, lp_tuya_pcb.ssid_len, lp_tuya_pcb.psk, 0) == 0) {
			TUYA_LOG_INFO("wlan_sta_config success\n");
		}

		if (wlan_sta_enable() == 0) {
			TUYA_LOG_INFO("wlan_sta_enable success\n");
		}
	} while (1);

connect_success:
	// 退避时间比较久了，说明很长时间没连上，关闭当前socket连接，否则只是短期断开，继续使用
	if (bss_reconnect_back_off_time_s >= 160 && lp_tuya_pcb.low_power_socket >= 0) {
		tuya_ipc_low_power_server_close();
	}
	TUYA_LOG_INFO("### WiFi BSS reconnect successfully after %d attempts\n", bss_lost_reconnect_retry_count);
	return 0;
}

void tuya_low_power_msg_proc(uint32_t event, uint32_t data, void *arg)
{
	int ret = -1;
	uint16_t type = EVENT_SUBTYPE(event);
	struct netif *nif = g_wlan_netif;
	TUYA_LOG_INFO("Tuya Low Power net event: %s\r\n", net_ctrl_msg_type_to_str(type));
	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA && NET_IS_IP4_VALID(nif)) {
			ret = wlan_ext_low_power_param_set_default(10);
			if (ret == -2) {
				TUYA_LOG_ERR("Set Dtim 10 invalid arg\n");
			} else if (ret == -1) {
				TUYA_LOG_ERR("Set Dtim 10 exec failed\n");
			}
			if (lp_tuya_pcb.state_machine_thread) {  //相关线程起来后再工作
				if (lp_tuya_pcb.bss_lost_flags == 1) {
					lp_tuya_pcb.bss_lost_flags = 0;
					TUYA_LP_STATE_CHANGE(STATE_WIFI_CONNECTED);
					if (lp_tuya_pcb.wait_wlan_up_connect_flags == 1)
						hal_sem_post(lp_tuya_pcb.wlan_up_connect_sem);
					if (lp_tuya_pcb.wait_wlan_bss_reconnect_flags == 1)
						hal_sem_post(lp_tuya_pcb.bss_reconnect_sem);
				}
			} else {
				TUYA_LOG_WARN("state_machine_thread not work!!!\n");
			}
		}
		break;
	case NET_CTRL_MSG_NETWORK_UP:
		TUYA_LOG_INFO("### Network up\n");
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA) {
			ret = wlan_ext_low_power_param_set_default(10);
			if (ret == -2) {
				TUYA_LOG_ERR("Set Dtim 10 invalid arg\n");
			} else if (ret == -1) {
				TUYA_LOG_ERR("Set Dtim 10 exec failed\n");
			}
			tuya_wifi_get_config();
			TUYA_LP_STATE_CHANGE(STATE_WIFI_CONNECTED);
		}
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		TUYA_LOG_INFO("### Wlan disconnect\n");
		if (lp_tuya_pcb.state_machine_thread) {  //相关线程起来后再工作
			if (pm_state_get() != PM_STATUS_RUNNING &&
				nif && wlan_if_get_mode(nif) == WLAN_MODE_STA &&
				lp_tuya_pcb.bss_lost_flags != 1) {  //避免重复触发
				wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, 0, NULL);
				if (pm_wakelocks_acquire(&lp_tuya_pcb.tuya_wakelock, PM_WL_TYPE_WAIT_ONCE, OS_WAIT_FOREVER)) {
					TUYA_LOG_ERR("pm_wakelocks_acquire error\n");
				}
				lp_tuya_pcb.bss_lost_flags = 1;
				TUYA_LOG_INFO("### Network disconnect, notify state machine thread to reconnect WiFi\n");
				TUYA_LP_STATE_CHANGE(STATE_WIFI_DISCONNECTED);
			}
		} else {
			TUYA_LOG_WARN("state_machine_thread not work!!!\n");
		}
		break;
	case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
		TUYA_LOG_INFO("### Wlan ssid not found\n");
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA) {
			// TUYA_LP_STATE_CHANGE(STATE_ERROR);
		}
		break;
	case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
		TUYA_LOG_INFO("### Wlan connect loss(BSS LOST)\n");

		break;
	default:
		TUYA_LOG_WARN("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

static void tuya_ipc_low_power_server_close(void)
{
	if (lp_tuya_pcb.g_devMsg.sock < 0)
		return;
	TUYA_LOG_INFO("close socket  = %d\n", lp_tuya_pcb.g_devMsg.sock);
	close(lp_tuya_pcb.g_devMsg.sock);
	lp_tuya_pcb.g_devMsg.sock = -1;
	lp_tuya_pcb.low_power_socket = -1;  /* sync update lp_tuya_pcb.low_power_socket */
	return;
}

int tuya_ipc_low_power_socket_fd_get()
{
	return lp_tuya_pcb.g_devMsg.sock;
}

int tuya_ipc_low_power_wakeup_data_get(char *pdata, int *plen)
{
	return lp_protocol_wakeup(pdata, plen, lp_tuya_pcb.low_power_wake_up_date_seed);
}

int tuya_ipc_low_power_heart_beat_get(char *pdata, int *plen)
{
	return lp_protocol_heartbeat(pdata, plen);
}

int tuya_ipc_low_power_network_reconnect(void)
{
	int ret = 1;
	int i = 0;
	int retry_interval = 200;
	while (ret != 0 && (i++ < TUYA_SERVER_RECONNECT_MAX)) {
		TUYA_LOG_INFO("Connecting to server... Attempt %d/%d\n", i, TUYA_SERVER_RECONNECT_MAX);
		ret = tuya_ipc_low_power_server_connect(lp_tuya_pcb.tuya_lp_info.ip,
			lp_tuya_pcb.tuya_lp_info.port,
			lp_tuya_pcb.tuya_lp_info.devid,
			strlen(lp_tuya_pcb.tuya_lp_info.devid),
			lp_tuya_pcb.tuya_lp_info.local_key,
			strlen(lp_tuya_pcb.tuya_lp_info.local_key));

		if (ret != 0) {
			TUYA_LOG_INFO("Connection attempt %d failed, retrying after %d ms\n", i, retry_interval);
			hal_msleep(retry_interval);
			retry_interval = MIN(retry_interval * 2, 5000);//TODO: 后续改成休眠，降低功耗
		}
	}
	if (ret != 0) {
		TUYA_LOG_ERR("tuya_ipc_low_power_network_reconnect error\n");
		return -1;
	}

	if (lp_tuya_pcb.low_power_socket == -1) {
		lp_tuya_pcb.low_power_socket = tuya_ipc_low_power_socket_fd_get();
	}

	tuya_ipc_low_power_wakeup_data_get(lp_tuya_pcb.wakeData, &lp_tuya_pcb.wake_data_len);
	TUYA_LOG_INFO("wake up date is { ");
	for (i = 0; i < lp_tuya_pcb.wake_data_len; i++) {
		printf("0x%x ", lp_tuya_pcb.wakeData[i]);
	}
	printf(" }\n");

	tuya_ipc_low_power_heart_beat_get(lp_tuya_pcb.heart_beat, &lp_tuya_pcb.heart_beat_len);
	TUYA_LOG_INFO("heart beat data is { ");
	for (i = 0; i < lp_tuya_pcb.heart_beat_len; i++) {
		printf("0x%x ", lp_tuya_pcb.heart_beat[i]);
	}
	printf(" }\n");

	TUYA_LOG_INFO("tuya_ipc_low_power_network_reconnect success\n");
	return 0;
}

int tuya_ipc_low_power_server_connect(TUYA_IP_ADDR_T serverIp, signed int port, char *pdevId, int idLen, char *pkey, int keyLen)
{
	if (pdevId == NULL || pkey == NULL || idLen == 0 || keyLen == 0) {
		TUYA_LOG_ERR("Invalid parameters: pdevId=%p, pkey=%p, idLen=%d, keyLen=%d\n",
			pdevId, pkey, idLen, keyLen);
		return -2;
	}
	// TODO 重复连接处理
	if (lp_tuya_pcb.g_devMsg.sock >= 0) {
		TUYA_LOG_WARN("Already connected to server (socket=%d), closing now!\n", lp_tuya_pcb.g_devMsg.sock);
		tuya_ipc_low_power_server_close();
	}

	lp_tuya_pcb.g_devMsg.sock = lp_cli_connect_server(serverIp, port);  /* socket连接 */
	if (lp_tuya_pcb.g_devMsg.sock < 0) {
		TUYA_LOG_ERR("connect lowpower server is error\n");
		memset(&lp_tuya_pcb.g_devMsg, 0, sizeof(lp_tuya_pcb.g_devMsg));
		lp_tuya_pcb.g_devMsg.sock = -1;
		return -1;
	}

	if (lp_cli_init_dev_info(&lp_tuya_pcb.g_devMsg, pdevId, idLen, pkey, keyLen) < 0)  /* 加密deviceid */
	{
		TUYA_LOG_ERR("init dev info error\n");
		return -1;
	}

	if (lp_cli_auth(lp_tuya_pcb.g_devMsg.sock, &lp_tuya_pcb.g_devMsg) < 0) {
		TUYA_LOG_ERR("auth lowpower is error\n");
		close(lp_tuya_pcb.g_devMsg.sock);
		memset(&lp_tuya_pcb.g_devMsg, 0, sizeof(lp_tuya_pcb.g_devMsg));
		lp_tuya_pcb.g_devMsg.sock = -1;
		return -1;
	}
	lp_tuya_pcb.low_power_wake_up_date_seed = lp_cli_wake_up_data_seed_computer(pkey, keyLen);

	TUYA_LOG_INFO("connect lowpower is success socket=%d,seed is =%d\n", lp_tuya_pcb.g_devMsg.sock, lp_tuya_pcb.low_power_wake_up_date_seed);

	// if (lp_cli_set_socket_o_block(lp_tuya_pcb.g_devMsg.sock, 1) == -1) {
	// 	TUYA_LOG_INFO("Error setting socket block mode: %s\n", strerror(errno));
	// }

	return 0;
}

static void wlan_wakeup_callback(void *arg)
{
	TUYA_LOG_INFO("wlan_wakeup_callback run\n");
	if (pm_state_get() == PM_STATUS_RUNNING) {
		TUYA_LOG_WARN("pm_state_get is PM_STATUS_RUNNING\n");
		return;
	}
	if (pm_wakelocks_acquire(&lp_tuya_pcb.tuya_wakelock, PM_WL_TYPE_WAIT_ONCE, OS_WAIT_FOREVER)) {
		TUYA_LOG_ERR("pm_wakelocks_acquire error\n");
	}
	// 通知状态机线程进行心跳发送
	if (lp_tuya_pcb.state_machine_thread) {
		// 唤醒后直接进入发送心跳状态
		TUYA_LP_STATE_CHANGE(STATE_SEND_HEARTBEAT);
		if (lp_tuya_pcb.need_task_notify_flags == 1)
			xTaskNotifyGive(lp_tuya_pcb.state_machine_thread);
	} else {
		TUYA_LOG_WARN("lp_tuya_pcb.state_machine_thread is NULL\n");
	}
	wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, lp_tuya_pcb.tuya_lp_tcp_keepalive_interval, NULL);
}

static void wlan_wakeup_bss_reconnetc_callback(void *arg)
{
	TUYA_LOG_INFO("wlan_wakeup_bss_reconnetc_callback run\n");
	if (pm_wakelocks_acquire(&lp_tuya_pcb.tuya_wakelock, PM_WL_TYPE_WAIT_ONCE, OS_WAIT_FOREVER)) {
		TUYA_LOG_ERR("pm_wakelocks_acquire error\n");
	}
	if (lp_tuya_pcb.wait_wlan_bss_reconnect_flags == 1)
		hal_sem_post(lp_tuya_pcb.bss_reconnect_sem);
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

	TUYA_LOG_INFO("set wake after %d\n", sec);
	return wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_KEEPALIVE_PRTIOD, (uint32_t)&param);
}

// rpmsg
static int tuya_rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
	size_t len, uint32_t src, void *priv)
{
	struct ept_test_entry *eptdev = ept->priv;

	TUYA_RPMSG_LOG("rpmsg%d: rx %zu Bytes \r\n", (int)eptdev->client->id, len);

	memcpy(&lp_tuya_pcb.tuya_lp_info, (tuya_lowpower_info *)data, sizeof(lp_tuya_pcb.tuya_lp_info));
	// TUYA_LOG_INFO("lp_tuya_pcb.tuya_lp_info.ip.u_addr.ip4 = %d\n", lp_tuya_pcb.tuya_lp_info.ip.u_addr.ip4);
	// TUYA_LOG_INFO("lp_tuya_pcb.tuya_lp_info.port = %d\n", lp_tuya_pcb.tuya_lp_info.port);
	// TUYA_LOG_INFO("lp_tuya_pcb.tuya_lp_info.devid = %s\n", lp_tuya_pcb.tuya_lp_info.devid);
	// TUYA_LOG_INFO("lp_tuya_pcb.tuya_lp_info.local_key = %s\n", lp_tuya_pcb.tuya_lp_info.local_key);
	// 通知状态机线程处理新的配置信息

	lp_tuya_pcb.tuya_lp_rpmsg_msg_flags = 1;
	if (lp_tuya_pcb.wait_start_thread) {
		lp_tuya_pcb.current_state = STATE_INIT;
		xTaskNotifyGive(lp_tuya_pcb.wait_start_thread);
	}
	return 0;
}
// end rpmsg

/* 状态机线程已完全替代原来的发送和接收线程
 * 所有TCP通信逻辑都在状态机中统一处理
 * 状态机线程函数 - 合并了发送和接收功能
*/
static void tuya_lp_state_machine_thread(void *arg)
{
	TUYA_LOG_INFO("Start\n");

	char recBuf[TUYA_RECV_BUF_SIZE] = { 0 };
	int len = 0;

	fd_set rfds;
	struct timeval tv;
	int retval, maxfd = -1;

	lp_tuya_pcb.tuya_lp_tcp_keepalive_interval = TUYA_KEEPALIVE_TIME;
	lp_tuya_pcb.idle_select_timeout_count = 0;
	lp_tuya_pcb.recv_wakeup_flags = 0;

	wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, lp_tuya_pcb.tuya_lp_tcp_keepalive_interval, NULL);

	while (1) {

		switch (lp_tuya_pcb.current_state) {
		case STATE_INIT:
			TUYA_LOG_INFO("State: STATE_INIT\n");
			if (lp_tuya_pcb.bss_lost_flags == 0) {
				TUYA_LP_STATE_CHANGE(STATE_WIFI_CONNECTED);
			} else {
				TUYA_LP_STATE_CHANGE(STATE_BSS_RECONNECT_WIFI);
			}
			break;

		case STATE_WIFI_CONNECTED:
			TUYA_LOG_INFO("State: STATE_WIFI_CONNECTED\n");
			if (tuya_ipc_low_power_socket_fd_get() < 0) {
				if (tuya_ipc_low_power_network_reconnect() == 0) {
					TUYA_LOG_INFO("Server connected successfully\n");
					TUYA_LP_STATE_CHANGE(STATE_SERVER_CONNECTED);
				} else {
					TUYA_LOG_WARN("Server connection failed, will retry\n");
					TUYA_LP_STATE_CHANGE(STATE_SERVER_RECONNECT);
				}
			} else {
				TUYA_LP_STATE_CHANGE(STATE_SERVER_CONNECTED);
			}
			break;

		case STATE_SERVER_CONNECTED:
			TUYA_LOG_INFO("State: STATE_SERVER_CONNECTED\n");
			TUYA_LP_STATE_CHANGE(STATE_SEND_HEARTBEAT);
			break;

		case STATE_SEND_HEARTBEAT:
#ifdef CONFIG_TUYA_RTC_WATCH_DOG
			tuya_rtc_wdt_feed();  //feed dog
#endif
			TUYA_LOG_INFO("State: STATE_SEND_HEARTBEAT\n");
			TUYA_LOG_INFO("============send heart beat: len = %d==============\n", lp_tuya_pcb.heart_beat_len);
			for (int i = 0; i < lp_tuya_pcb.heart_beat_len; i++) {
				printf("0x%x ", lp_tuya_pcb.heart_beat[i]);
			}
			printf(" }\n");

			len = send(lp_tuya_pcb.low_power_socket, lp_tuya_pcb.heart_beat, lp_tuya_pcb.heart_beat_len, 0);
			if (len < 0) {
				TUYA_LOG_ERR("socket = %d, %s\n", lp_tuya_pcb.low_power_socket, strerror(errno));
				TUYA_LOG_ERR("send error, need close socket and reconnect\n");
				TUYA_LP_STATE_CHANGE(STATE_SERVER_RECONNECT);
				if (lp_tuya_pcb.tuya_wakelock.ref) {
					pm_wakelocks_release(&lp_tuya_pcb.tuya_wakelock);
					TUYA_LOG_INFO("pm_wakelocks_release\n");
				}
			} else {
				TUYA_LOG_INFO("News: %d \t send, sent a total of %d bytes!\n",
					lp_tuya_pcb.heart_beat_len, len);
				TUYA_LP_STATE_CHANGE(STATE_RECV_RESPONSE);
			}
			break;
		case STATE_RECV_RESPONSE:
			if (lp_tuya_pcb.wait_heart_select_timeout_count == 0)
				TUYA_LOG_INFO("State: STATE_RECV_RESPONSE\n");
			if (lp_tuya_pcb.low_power_socket >= 0) {
				FD_ZERO(&rfds);
				maxfd = lp_tuya_pcb.low_power_socket;
				FD_SET(lp_tuya_pcb.low_power_socket, &rfds);
				tv.tv_sec = 0;
				tv.tv_usec = TUYA_SELECT_PER_TIMEOUT_US;
				if (lp_tuya_pcb.wait_heart_select_timeout_count == 0)
					TUYA_LOG_INFO("## select now! socket=%d\n", lp_tuya_pcb.low_power_socket);
				retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
				if (retval == -1) {
					TUYA_LOG_ERR("select error, %s\n", strerror(errno));
					if (errno == EBADF || errno == EINVAL) {
						TUYA_LOG_ERR("socket invalid, need reconnect\n");
						TUYA_LP_STATE_CHANGE(STATE_SERVER_RECONNECT);
					}
				} else if (retval == 0) {
					if (++lp_tuya_pcb.wait_heart_select_timeout_count % 100 == 0) {
						TUYA_LOG_WARN("select timeout times: %d/%d \n", lp_tuya_pcb.wait_heart_select_timeout_count, TUYA_SELECT_PER_TIMEOUT_MAX);
					}
					if (lp_tuya_pcb.wait_heart_select_timeout_count > TUYA_SELECT_PER_TIMEOUT_MAX) {  //需要考虑唤醒大核，但是没有网络情况
						lp_tuya_pcb.wait_heart_select_timeout_count = 0;
						if (pm_state_get() == PM_STATUS_ACTIVING) {
							wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, 0, NULL); /* stop send keepalive packet */
							TUYA_LOG_WARN("select timeout when PM_STATUS_ACTIVING, need reconnect now!\n");
							TUYA_LP_STATE_CHANGE(STATE_SERVER_RECONNECT);

						} else {
							TUYA_LOG_WARN("select timeout when PM_STATUS_RUNNING, do nothing: %d/%d\n",
								lp_tuya_pcb.idle_select_timeout_count++, IDLE_SELECT_TIMEOUT_MAX);
							if (lp_tuya_pcb.idle_select_timeout_count > IDLE_SELECT_TIMEOUT_MAX) {
								TUYA_LOG_WARN("select_timeout when PM_STATUS_RUNNING more then %d, please check network!\n",
									IDLE_SELECT_TIMEOUT_MAX);
								TUYA_LOG_WARN("### Will reboot now\n\n");
								tuya_lp_reboot();  //唤醒后一直没连上，就需要重启来处理?
								break;
							}
						}
					}
				} else {
					TUYA_LOG_INFO("select success\n");
					lp_tuya_pcb.wait_heart_select_timeout_count = 0;
					if (FD_ISSET(lp_tuya_pcb.low_power_socket, &rfds)) {
						len = recv(lp_tuya_pcb.low_power_socket, recBuf, TUYA_RECV_BUF_SIZE, 0);
						if (len > 0) {
							TUYA_LOG_INFO("Successfully received the message: is {");
							for (int i = 0; i < len; i++)
								printf("0x%02x ", recBuf[i]);
							printf("}\n");
							if (strncmp(recBuf, lp_tuya_pcb.wakeData, lp_tuya_pcb.wake_data_len) == 0 &&
								lp_tuya_pcb.recv_wakeup_flags == 0 &&
								pm_state_get() >= PM_STATUS_SLEEPED) {
								TUYA_LOG_INFO("receive wake up command\n");
								wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, 0, NULL);
								pm_wakelocks_acquire(&lp_tuya_pcb.tuya_wakelock, PM_WL_TYPE_WAIT_ONCE, OS_WAIT_FOREVER);
								pm_wakesrc_relax(lp_tuya_pcb.tuya_wakelock.ws, PM_RELAX_WAKEUP);
								if (lp_tuya_pcb.tuya_wakelock.ref) {
									pm_wakelocks_release(&lp_tuya_pcb.tuya_wakelock);
									TUYA_LOG_INFO("pm_wakelocks_release\n");
								}
								lp_tuya_pcb.recv_wakeup_flags = 1;
							}
							if (strncmp(recBuf, lp_tuya_pcb.heart_beat, lp_tuya_pcb.heart_beat_len) == 0) {
								TUYA_LOG_INFO("Received heart beat response\n");
								// 重置选择超时计数
								lp_tuya_pcb.idle_select_timeout_count = 0;
								// 设置保活并等待下次唤醒
								wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, lp_tuya_pcb.tuya_lp_tcp_keepalive_interval, NULL);
								if (lp_tuya_pcb.tuya_wakelock.ref) {
									pm_wakelocks_release(&lp_tuya_pcb.tuya_wakelock);
									TUYA_LOG_INFO("pm_wakelocks_release\n");
								}
							}
						} else {
							if (len < 0) {
								TUYA_LOG_ERR("Failed to receive the message! The error code is %d, error message is '%s'\n",
									errno, strerror(errno));
								TUYA_LP_STATE_CHANGE(STATE_SERVER_RECONNECT);
							} else {
								if (pm_state_get() == PM_STATUS_RUNNING) {
									wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, 0, NULL);
									TUYA_LOG_ERR("Chat to terminate len=0x%x!\n", len);
									tuya_ipc_low_power_server_close();
									if (lp_tuya_pcb.tuya_lp_rpmsg_init_flags == 0) {
										lp_tuya_pcb.tuya_lp_rpmsg_init_flags = 1;
										low_power_rpmsg_init(tuya_rpmsg_ept_callback);
									}
									goto exit_thread;
								}
								if (pm_state_get() == PM_STATUS_ACTIVING) {
									TUYA_LOG_ERR("Chat to terminate when PM_STATUS_ACTIVING, close socket and reconnect\n");
									TUYA_LP_STATE_CHANGE(STATE_SERVER_RECONNECT);
								}
							}
						}
						memset(recBuf, 0, sizeof(recBuf));
					}
				}
			}
			break;
		case STATE_WAKEUP_CPUX:
			TUYA_LOG_INFO("State: STATE_WAKEUP_CPUX\n");
			break;
		case STATE_WIFI_DISCONNECTED:
			TUYA_LOG_INFO("State: STATE_WIFI_DISCONNECTED\n");
			TUYA_LP_STATE_CHANGE(STATE_BSS_RECONNECT_WIFI);
			break;
		case STATE_BSS_LOST:
			TUYA_LOG_INFO("State: STATE_BSS_LOST\n");
			TUYA_LP_STATE_CHANGE(STATE_BSS_RECONNECT_WIFI);
			break;
		case STATE_BSS_RECONNECT_WIFI:
			TUYA_LOG_INFO("State: STATE_BSS_RECONNECT_WIFI\n");
			lp_tuya_pcb.wait_heart_select_timeout_count = 0; //断开后需要重置
			if (tuya_wifi_reconnect() == 0) {
				TUYA_LOG_INFO("WiFi reconnected successfully\n");
				TUYA_LP_STATE_CHANGE(STATE_WIFI_CONNECTED);
			} else {
				TUYA_LOG_WARN("WiFi reconnection failed, will retry\n");
			}
			break;
		case STATE_SERVER_RECONNECT:
			TUYA_LOG_INFO("State: STATE_SERVER_RECONNECT\n");
			tuya_ipc_low_power_server_close();
			if (tuya_ipc_low_power_network_reconnect() == 0) {
				TUYA_LOG_INFO("Server reconnected success\n");
				lp_tuya_pcb.tuya_lp_tcp_keepalive_interval = TUYA_KEEPALIVE_TIME;
				TUYA_LP_STATE_CHANGE(STATE_SERVER_CONNECTED);
			} else {
				TUYA_LOG_WARN("Server reconnection failed, will retry after %ds\n", TUYA_RECONNECT_TIME);
				lp_tuya_pcb.tuya_lp_tcp_keepalive_interval = TUYA_RECONNECT_TIME;
				wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, lp_tuya_pcb.tuya_lp_tcp_keepalive_interval, NULL);
				if (lp_tuya_pcb.tuya_wakelock.ref) {
					pm_wakelocks_release(&lp_tuya_pcb.tuya_wakelock);
					TUYA_LOG_INFO("pm_wakelocks_release\n");
				}
				lp_tuya_pcb.need_task_notify_flags = 1;
				ulTaskNotifyTake(pdTRUE, HAL_WAIT_FOREVER);
				lp_tuya_pcb.need_task_notify_flags = 0;
			}
			break;

		case STATE_ERROR:
			TUYA_LOG_ERR("State: STATE_ERROR\n");
			if (lp_tuya_pcb.bss_lost_flags == 1) {
				TUYA_LP_STATE_CHANGE(STATE_BSS_RECONNECT_WIFI);
			} else {
				hal_msleep(2);  //避免线程卡死
				// TUYA_LP_STATE_CHANGE(STATE_SERVER_RECONNECT);
			}
			break;
		default:
			TUYA_LOG_WARN("Unknown state: %d\n", lp_tuya_pcb.current_state);
			TUYA_LP_STATE_CHANGE(STATE_INIT);
			break;
		}
	}

exit_thread:
	if (lp_tuya_pcb.low_power_socket >= 0) {
		wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, 0, NULL);
		tuya_ipc_low_power_server_close();
		lp_tuya_pcb.low_power_socket = tuya_ipc_low_power_socket_fd_get();
	}

	if (pm_task_unregister(lp_tuya_pcb.state_machine_thread)) {
		TUYA_LOG_ERR("lp_tuya_pcb.state_machine_thread pm_task_unregister error\n");
	}

	lp_tuya_pcb.state_machine_thread = NULL;
	TUYA_LOG_WARN("lp_tuya_pcb.state_machine_thread = NULL\n");
	TUYA_LOG_WARN("Stop\n");
	hal_thread_stop(NULL);
	return;
}

int tuya_lowpower_start(char *devbuf, char *keybuf, TUYA_IP_ADDR_T ip, int port)
{
	int ret = 1;
	int i = 0;
	int retry_interval = 200;

	// 停止可能存在的状态机线程，需要放前面，因为可能大核MQTT连上后，不给小核发FIN，这样线程可能无法退出
	if (lp_tuya_pcb.state_machine_thread) {
		if (tuya_ipc_low_power_socket_fd_get() >= 0) {
			tuya_ipc_low_power_server_close();
		}
		if (pm_task_unregister(lp_tuya_pcb.state_machine_thread)) {
			TUYA_LOG_ERR("lp_tuya_pcb.state_machine_thread pm_task_unregister error\n");
		}
		hal_thread_stop(lp_tuya_pcb.state_machine_thread);
		lp_tuya_pcb.state_machine_thread = NULL;
		TUYA_LOG_INFO("Stop state_machine_thread Now\n");
	}

	while (ret != 0 && (i++ < TUYA_SERVER_RECONNECT_MAX)) {
		TUYA_LOG_INFO("Connecting to server... Attempt %d/%d\n", i, TUYA_SERVER_RECONNECT_MAX);
		ret = tuya_ipc_low_power_server_connect(ip, port, devbuf, strlen(devbuf), keybuf, strlen(keybuf));

		if (ret != 0) {
			TUYA_LOG_WARN("Connection attempt %d failed, retrying after %d ms\n", i, retry_interval);
			hal_msleep(retry_interval);
			retry_interval = MIN(retry_interval * 2, 5000);
		}
	}

	if (i >= TUYA_SERVER_RECONNECT_MAX) {
		TUYA_LOG_ERR("Failed to connect after %d attempts, please check network!\n", TUYA_SERVER_RECONNECT_MAX);
		TUYA_LOG_ERR("### Will reboot now\n\n");
		tuya_lp_reboot();
	}

	TUYA_LP_STATE_CHANGE(STATE_SERVER_CONNECTED);

	while (lp_tuya_pcb.low_power_socket == -1) {
		lp_tuya_pcb.low_power_socket = tuya_ipc_low_power_socket_fd_get();
	}

	tuya_ipc_low_power_wakeup_data_get(lp_tuya_pcb.wakeData, &lp_tuya_pcb.wake_data_len);
	TUYA_LOG_INFO("wake up date is { ");
	for (i = 0; i < lp_tuya_pcb.wake_data_len; i++) {
		printf("0x%x ", lp_tuya_pcb.wakeData[i]);
	}
	printf(" }\n");

	tuya_ipc_low_power_heart_beat_get(lp_tuya_pcb.heart_beat, &lp_tuya_pcb.heart_beat_len);
	TUYA_LOG_INFO("heart beat data is { ");
	for (i = 0; i < lp_tuya_pcb.heart_beat_len; i++) {
		printf("0x%x ", lp_tuya_pcb.heart_beat[i]);
	}
	printf(" }\n");

	/* disabled ps in resume */
	int enable = 1;
	if (wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_DISABLE_PS_IN_RESUME, (uint32_t)enable) == 0) {
		TUYA_LOG_INFO("Set DISABLE_PS_IN_RESUME success\n");
	}

	// 创建状态机线程
	if (!lp_tuya_pcb.state_machine_thread) {
		TUYA_LOG_INFO("create state machine thread\n");
		lp_tuya_pcb.state_machine_thread = hal_thread_create(tuya_lp_state_machine_thread,
			NULL,
			"tuya_lp_state_machine",
			2048,
			17);

		if (!lp_tuya_pcb.state_machine_thread) {
			TUYA_LOG_ERR("tuya_lp_state_machine_thread create error\n");
			return 0;
		}
		hal_thread_start(lp_tuya_pcb.state_machine_thread);

		if (pm_task_register(lp_tuya_pcb.state_machine_thread, PM_TASK_TYPE_WLAN)) {
			TUYA_LOG_ERR("lp_tuya_pcb.state_machine_thread pm_task_register error\n");
			hal_thread_stop(lp_tuya_pcb.state_machine_thread);
			return -1;
		}
	}
	return 0;
}

void lp_tuya_wait_start_thread(void *arg)
{
	TUYA_LOG_INFO("Start\n");
	int ret = -1;

	while (1) {
#ifdef CONFIG_TUYA_RTC_WATCH_DOG
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(60 * 1000));  //定时喂狗
#else
		ulTaskNotifyTake(pdTRUE, HAL_WAIT_FOREVER);  //定时喂狗
#endif
		if (lp_tuya_pcb.tuya_lp_rpmsg_msg_flags == 1) {
			lp_tuya_pcb.tuya_lp_rpmsg_msg_flags = 0;
			TUYA_LOG_INFO("lp_tuya_pcb.tuya_lp_info.ip.u_addr.ip4 = %d\n", lp_tuya_pcb.tuya_lp_info.ip.u_addr.ip4);
			TUYA_LOG_INFO("lp_tuya_pcb.tuya_lp_info.port = %d\n", lp_tuya_pcb.tuya_lp_info.port);
			TUYA_LOG_INFO("lp_tuya_pcb.tuya_lp_info.devid = %s\n", lp_tuya_pcb.tuya_lp_info.devid);
			TUYA_LOG_INFO("lp_tuya_pcb.tuya_lp_info.local_key = %s\n", lp_tuya_pcb.tuya_lp_info.local_key);
			tuya_lowpower_start(lp_tuya_pcb.tuya_lp_info.devid, lp_tuya_pcb.tuya_lp_info.local_key, lp_tuya_pcb.tuya_lp_info.ip, lp_tuya_pcb.tuya_lp_info.port);
			/* rtos ack temporary */
			char hello[20] = { 0 };
			sprintf(hello, "rtos ok");
			ret = low_power_rpmsg_to_cpux(hello, strlen(hello));
			if (ret < 0) {
				TUYA_LOG_ERR("low_power_rpmsg_to_cpux (with data) failed\n");
			}
		}
#ifdef CONFIG_TUYA_RTC_WATCH_DOG
		tuya_rtc_wdt_feed();  //feed dog
#endif
	}
	low_power_rpmsg_deinit();
	hal_thread_stop(NULL);
	return;
}

#ifdef CONFIG_COMPONENTS_PM

void tuya_delay_thread(void *arg)
{
	TUYA_LOG_INFO("tuya_delay_thread run\n");
	vTaskDelay(pdMS_TO_TICKS(5000));
	low_power_rpmsg_init(tuya_rpmsg_ept_callback);
	hal_thread_stop(NULL);
}

static int tuya_lp_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	TUYA_LOG_INFO("### tuya_lp_suspend run\n");
	lp_tuya_pcb.tuya_lp_rpmsg_init_flags = 0;
	return 0;
}

static int tuya_lp_resume(struct pm_device *dev, suspend_mode_t mode)
{
	TUYA_LOG_INFO("### tuya_lp_resume run\n");
	wlan_keepalive_wakeup_init(WLAN_WAKE_NORMAL, 0, NULL); /* stop send keepalive packet */
	lp_tuya_pcb.tuya_lp_rpmsg_init_flags = 1;
	lp_tuya_pcb.delay_thread = hal_thread_create(tuya_delay_thread, NULL, "tuya_delay_thread", 256, 17);
	if (!lp_tuya_pcb.delay_thread) {
		TUYA_LOG_ERR("tuya_delay_thread create error\n");
	}
	hal_thread_start(lp_tuya_pcb.delay_thread);
	return 0;
}

static struct pm_devops tuya_lp_ops = {
	.suspend = tuya_lp_suspend,
	.resume = tuya_lp_resume,
};

static struct pm_device tuya_lp_pm = {
	.name = "tuya_lp",
	.ops = &tuya_lp_ops,
};
#endif

void lp_tuya_pcb_init(void)
{
	memset(&lp_tuya_pcb, 0, sizeof(lp_tuya_pcb));
	lp_tuya_pcb.g_devMsg.sock = -1;
	lp_tuya_pcb.low_power_socket = -1;
	lp_tuya_pcb.wake_data_len = WAKEDATE_LEN;
	lp_tuya_pcb.heart_beat_len = HEART_BEAT_LEN;
	lp_tuya_pcb.bss_reconnect_sem = hal_sem_create(0);
	lp_tuya_pcb.wlan_up_connect_sem = hal_sem_create(0);
	lp_tuya_pcb.tuya_wakelock.name = "tuya";
	lp_tuya_pcb.tuya_wakelock.type = PM_WL_TYPE_WAIT_ONCE;
}

int tuya_low_power_main(void)
{
	TUYA_LOG_INFO("Run tuya_low_power_main ok\n");
	lp_ctrl_msg_set_cb(tuya_low_power_msg_proc);
	lp_tuya_pcb_init();
	low_power_rpmsg_init(tuya_rpmsg_ept_callback);

#ifdef CONFIG_TUYA_RTC_WATCH_DOG
	if (!tuya_rtc_wdt_init()) {
		TUYA_LOG_INFO("tuya_rtc_wdt_init success\n");
	} else {
		TUYA_LOG_ERR("tuya_rtc_wdt_init failed\n");
	}
	tuya_rtc_wdt_start_timeout(90); //tmp set rtc watcdog timeout is 90
#endif

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_register(&tuya_lp_pm);
#endif

	lp_tuya_pcb.wait_start_thread = hal_thread_create(lp_tuya_wait_start_thread, NULL, "lp_tuya_wait_start_thread", 768, 17);
	if (!lp_tuya_pcb.wait_start_thread) {
		TUYA_LOG_ERR("lp_tuya_wait_start_thread create error\n");
		goto err_exit1;
	}
	hal_thread_start(lp_tuya_pcb.wait_start_thread);

	if (pm_task_register(lp_tuya_pcb.wait_start_thread, PM_TASK_TYPE_WLAN)) {
		TUYA_LOG_ERR("tcp_keepalive_thread pm_task_register error\n");
		goto err_exit;
	}

	return 0;
err_exit:
	hal_thread_stop(lp_tuya_pcb.wait_start_thread);
err_exit1:
	return -1;
}
