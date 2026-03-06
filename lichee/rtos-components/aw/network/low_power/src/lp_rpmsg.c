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
#include "lp_rpmsg.h"
#include "public.h"
#include "hal_timer.h"

#define BIND_CNT_MAX 10

const char *name = "rpmsg_lp";

static uint32_t tx_seq = 0;  // 消息序列号

struct lp_rpmsg_msg tx_msg;  // 发送消息缓冲区
struct lp_rpmsg_msg rx_msg;  // 接收消息缓冲区
struct ept_test_entry {
	struct rpmsg_ept_client *client;
	unsigned long print_perf_data;
	uint16_t keep_alive;
	struct server_info last_server_info;  // 保存大核传递的服务器信息

	//状态信息
	uint8_t cpux_run_status;  //1表示在运行
	uint8_t cpux_server_info_status;  //1表示拿到server info
#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_HEART
	#define HEARTBEAT_TIMEOUT_MS 30000  // 大核心跳超时相关配置
	// 心跳相关字段
	hal_thread_t heart_thread;
	uint8_t heartbeat_thread_running;  // 心跳检测线程运行标志
#endif
};

struct ept_test_entry *eptdev = NULL;

static int lp_rpmsg_send_packet(struct rpmsg_endpoint *ept, uint8_t msg_type, void *data, size_t len, uint32_t seq)
{
	if (ept == NULL) {
		LP_RPMSG_LOG("Error: endpoint is NULL\n");
		return -1;
	}

	memset(&tx_msg, 0, sizeof(tx_msg));

	tx_msg.header.type = msg_type;
	tx_msg.header.len = len < MAX_MSG_BODY_SIZE ? len : MAX_MSG_BODY_SIZE;

	// 如果指定了seq号，则使用指定的seq，否则生成新的seq
	if (seq == 0) {
		tx_msg.header.seq = ++tx_seq;
	} else {
		tx_msg.header.seq = seq;
	}

	if (data != NULL && len > 0) {
		memcpy(tx_msg.body.raw_data, data, tx_msg.header.len);
	}

	LP_RPMSG_LOG_DBG("Send message: type=0x%x, len=%d, seq=%u\n",
		tx_msg.header.type, tx_msg.header.len, tx_msg.header.seq);

	return rpmsg_send(ept, &tx_msg, sizeof(tx_msg.header) + tx_msg.header.len);
}

// 发送确认消息
static int send_ack(struct rpmsg_endpoint *ept, uint32_t seq, uint8_t ack_data)
{
	uint8_t data = ack_data;
	LP_RPMSG_LOG_DBG("Send ACK: seq=%u, data=%u\n", seq, ack_data);
	// 使用大核发来的seq号发送确认消息
	return lp_rpmsg_send_packet(ept, MSG_TYPE_ACK, &data, sizeof(data), seq);
}

// 处理心跳包
static void handle_heartbeat(struct rpmsg_endpoint *ept, struct lp_rpmsg_msg *msg)
{
#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_HEART
	LP_RPMSG_LOG("Received heartbeat: data=%u, seq=%u\n",
		msg->body.heart_data, msg->header.seq);
	struct ept_test_entry *dev = ept->priv;
	xTaskNotifyGive(dev->heart_thread); //通知心跳线程
#endif
	// 发送确认
	send_ack(ept, msg->header.seq, 1);
}

// 处理休眠前通知
static void handle_pre_sleep(struct rpmsg_endpoint *ept, struct lp_rpmsg_msg *msg)
{
	LP_RPMSG_LOG("Received pre-sleep notification, seq=%u\n", msg->header.seq);

	// 保存服务器信息
	if (msg->header.len >= sizeof(struct server_info)) {
		memcpy(&eptdev->last_server_info, &msg->body.server_info, sizeof(struct server_info));
		LP_RPMSG_LOG("Server info saved: IP=0x%x, Port=%u, extra_info = %s\n",
			eptdev->last_server_info.ip, eptdev->last_server_info.port, eptdev->last_server_info.extra_info);
	}
	lp_rpmsg_set_server_info_status(1);
	// 发送确认
	send_ack(ept, msg->header.seq, 1);
}

// 处理唤醒确认请求
static void handle_wakeup_confirm(struct rpmsg_endpoint *ept, struct lp_rpmsg_msg *msg)
{
	LP_RPMSG_LOG("Received wakeup confirm, seq=%u\n", msg->header.seq);
	//更新cpux状态
	eptdev->cpux_run_status = 1;
	// 发送确认
	send_ack(ept, msg->header.seq, 1);
}

// 处理保活状态请求
static void handle_keepalive_status(struct rpmsg_endpoint *ept, struct lp_rpmsg_msg *msg)
{
	LP_RPMSG_LOG("Received keepalive status request, seq=%u\n", msg->header.seq);

	// 使用封装函数发送保活状态
	uint8_t status = lp_rpmsg_get_keepalive_state();
	LP_LOG_INFO("Get Keepalive status = %d\n", status);
	lp_rpmsg_send_packet(ept, MSG_TYPE_KEEPALIVE_STATUS, &status, sizeof(status), 0);
}

// 处理错误报告
static void handle_error(struct rpmsg_endpoint *ept, struct lp_rpmsg_msg *msg)
{
	LP_RPMSG_LOG("Received error report: code=%u, seq=%u\n",
		msg->body.error_code, msg->header.seq);

	// 发送确认
	send_ack(ept, msg->header.seq, 1);
}

// 处理确认响应
static void handle_ack(struct rpmsg_endpoint *ept, struct lp_rpmsg_msg *msg)
{
	LP_RPMSG_LOG("Received ACK: seq=%u, data=%u\n",
		msg->header.seq, msg->body.ack_data);
	// 确认消息通常不需要回复
}

static int rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
	size_t len, uint32_t src, void *priv)
{
	struct ept_test_entry *eptdev = ept->priv;

	// 检查消息长度
	if (len < sizeof(struct msg_header)) {
		LP_RPMSG_LOG("Message too short: %zu bytes\n", len);
		return -1;
	}

	// 复制消息到接收缓冲区
	memset(&rx_msg, 0, sizeof(rx_msg));
	memcpy(&rx_msg, data, len > sizeof(rx_msg) ? sizeof(rx_msg) : len);

	LP_RPMSG_LOG_DBG("rpmsg%d: rx %zu Bytes, type=0x%x, seq=%u\r\n",
		(int)eptdev->client->id, len, rx_msg.header.type, rx_msg.header.seq);

	// 根据消息类型进行处理
	switch (rx_msg.header.type) {
	case MSG_TYPE_HEARTBEAT:
		handle_heartbeat(ept, &rx_msg);
		break;
	case MSG_TYPE_PRE_SLEEP:
		handle_pre_sleep(ept, &rx_msg);
		break;
	case MSG_TYPE_WAKEUP_CONFIRM:
		handle_wakeup_confirm(ept, &rx_msg);
		break;
	case MSG_TYPE_KEEPALIVE_STATUS:
		handle_keepalive_status(ept, &rx_msg);
		break;
	case MSG_TYPE_ERROR:
		handle_error(ept, &rx_msg);
		break;
	case MSG_TYPE_ACK:
		handle_ack(ept, &rx_msg);
		break;
	default:
		LP_RPMSG_LOG("Unknown message type: 0x%x\n", rx_msg.header.type);
		// 使用封装函数发送错误响应
		uint32_t error_code = 1;  // 未知消息类型错误
		lp_rpmsg_send_packet(ept, MSG_TYPE_ERROR, &error_code, sizeof(error_code), 0);
		break;
	}
	return 0;
}

#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_HEART
// 心跳检测线程
static void heartbeat_check_thread(void *arg)
{
	struct ept_test_entry *dev = (struct ept_test_entry *)arg;
	uint32_t task_notify_val;

	LP_RPMSG_LOG("Heartbeat check thread started\n");

	while (dev->heartbeat_thread_running) {
		task_notify_val = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(HEARTBEAT_TIMEOUT_MS));
		if (task_notify_val == 0) {
			LP_LOG_ERR("Heartbeat timeout!\n");
			//TODO: 唤醒状态下出现rpmsg 心跳超时的判断，例如大核卡死进行复位
			if (dev->cpux_run_status == 1) {
				LP_LOG_ERR("CPUX running, but timeout need reset!!\n");
			}
		}
	}
	LP_RPMSG_LOG("Heartbeat check thread exited\n");
	hal_thread_stop(NULL);
}
#endif

static int rpmsg_bind_cb(struct rpmsg_ept_client *client)
{
	LP_RPMSG_LOG("rpmsg%d: binding\r\n", client->id);

	eptdev = hal_malloc(sizeof(*eptdev));
	if (!eptdev) {
		openamp_err("failed to alloc client entry\r\n");
		return -ENOMEM;
	}

	memset(eptdev, 0, sizeof(*eptdev));
	eptdev->client = client;
	client->ept->priv = eptdev;
	eptdev->print_perf_data = (unsigned long)client->priv;

#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_HEART
	eptdev->heartbeat_thread_running = 1;
	// 创建心跳检测线程
	eptdev->heart_thread = hal_thread_create(heartbeat_check_thread,
		eptdev,
		"heartbeat_check",
		1024,
		17);
	if (!eptdev->heart_thread) {
		LP_RPMSG_LOG("Failed to create heartbeat check thread\n");
		// 线程创建失败不会影响基本功能，但会失去心跳检测能力
	}
#endif
	return 0;
}

static int rpmsg_unbind_cb(struct rpmsg_ept_client *client)
{
	struct ept_test_entry *eptdev = client->ept->priv;

	LP_RPMSG_LOG("rpmsg%d: unbinding\r\n", client->id);

#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_HEART
	// 停止心跳检测线程
	eptdev->heartbeat_thread_running = 0;
	xTaskNotifyGive(eptdev->heart_thread);
	hal_msleep(100);  // 给线程时间退出
#endif
	hal_free(eptdev);
	eptdev = NULL;
	return 0;
}

int lp_rpmsg_init(void)
{
	int cnt = 10;
	unsigned long print_perf_data = 0;
	int bind_state = -1;
	int bind_cnt = 0;

rebind:
	LP_RPMSG_LOG("wait cpux bind: %s, try cnt: %d/%d\r\n",
		name, ++bind_cnt, BIND_CNT_MAX);
	bind_state = rpmsg_client_bind(name, rpmsg_ept_callback, rpmsg_bind_cb,
		rpmsg_unbind_cb, cnt, (void *)print_perf_data);
	if (bind_state == -EBUSY) {
		LP_RPMSG_LOG("rpmsg_client_bind Already bind\n");
	} else if (bind_state != 0) {
		if (bind_cnt >= BIND_CNT_MAX) return -1;
		hal_msleep(500);
		goto rebind;
	}

	return 0;
}

int lp_rpmsg_deinit(void)
{
	LP_RPMSG_LOG("unbind cpux: %s\r\n", name);
	rpmsg_client_unbind(name); /* self-unbind */
	return 0;
}

int lp_rpmsg_to_cpux(char *send_msg, int len)
{
	if (eptdev == NULL) {
		LP_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}
	// 使用封装函数发送数据
	return lp_rpmsg_send_packet(eptdev->client->ept, MSG_TYPE_NORMAL_MSG, send_msg, len, 0);
}

// 发送服务器信息
int lp_rpmsg_send_server_info(struct server_info *info)
{
	if (eptdev == NULL) {
		LP_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}

	return lp_rpmsg_send_packet(eptdev->client->ept, MSG_TYPE_PRE_SLEEP, info, sizeof(struct server_info), 0);
}

int8_t lp_rpmsg_set_keepalive_state(int8_t state)
{
	if (eptdev == NULL) {
		LP_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}
	eptdev->keep_alive = state;
	return 0;
}

int8_t lp_rpmsg_get_keepalive_state(void)
{
	if (eptdev == NULL) {
		LP_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}
	return eptdev->keep_alive;
}

// 获取保存的服务器信息
struct server_info *lp_rpmsg_get_server_info(void)
{
	if (eptdev == NULL) {
		LP_RPMSG_LOG("warn:eptdev not init\n");
		return NULL;
	}
	return &eptdev->last_server_info;
}

// 获取cpux状态
int8_t lp_rpmsg_get_cpux_status(void)
{
	if (eptdev == NULL) {
		LP_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}
	return eptdev->cpux_run_status;
}

// 获取server_info状态
int8_t lp_rpmsg_get_server_info_status(void)
{
	if (eptdev == NULL) {
		LP_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}
	return eptdev->cpux_server_info_status;
}

// 设置server_info状态
int8_t lp_rpmsg_set_server_info_status(int8_t statue)
{
	if (eptdev == NULL) {
		LP_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}
	eptdev->cpux_server_info_status = statue;
	return 0;
}
