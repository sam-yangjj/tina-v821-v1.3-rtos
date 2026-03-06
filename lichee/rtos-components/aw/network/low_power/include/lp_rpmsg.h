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
#ifndef __LOW_POWER_RPMSG__H__
#define __LOW_POWER_RPMSG__H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <aw_list.h>
#include <hal_atomic.h>
#include <hal_mutex.h>
#include <hal_mem.h>
#include <hal_sem.h>
#include <hal_queue.h>
#include <hal_thread.h>
#include <console.h>
#include <openamp/sunxi_helper/openamp_log.h>
#include <openamp/sunxi_helper/rpmsg_master.h>

#include <stdint.h>
#include <openamp/open_amp.h>
#include <hal_sem.h>

/* 消息类型定义 */
enum msg_type {
	MSG_TYPE_HEARTBEAT = 0,  // 心跳包
	MSG_TYPE_PRE_SLEEP,  // 休眠前通知
	MSG_TYPE_PRE_SLEEP_ACK,  // 允许休眠
	MSG_TYPE_WAKEUP_CONFIRM ,  // 唤醒确认
	MSG_TYPE_KEEPALIVE_STATUS,  // 保活状态
	MSG_TYPE_NORMAL_MSG,  // 通用信息
	MSG_TYPE_ERROR,  // 错误报告
	MSG_TYPE_ACK,  // 确认响应
	MSG_MAX
};

/* 服务器信息结构体 */
#define MAX_SERVER_INFO_LEN 64
struct server_info {
    uint32_t ip;        // 服务器IP地址
    uint16_t port;      // 服务器端口
    uint8_t mac[6];     // MAC地址
    char extra_info[MAX_SERVER_INFO_LEN]; // 额外信息
};

/* 消息头结构体 */
struct msg_header {
    uint8_t type;       // 消息类型
    uint16_t len;       // 消息体长度
    uint32_t seq;       // 消息序列号
};

/* 通用消息结构体 */
#define MAX_MSG_BODY_SIZE 256
struct lp_rpmsg_msg {
    struct msg_header header;             // 消息头
    union {
        uint8_t heart_data;               // 心跳数据
        struct server_info server_info;   // 服务器信息
        uint8_t keepalive_status;         // 保活状态
        uint32_t error_code;              // 错误码
        uint8_t ack_data;                 // 确认数据
        uint8_t raw_data[MAX_MSG_BODY_SIZE]; // 原始数据缓冲区
    } body;
};

/* 缓冲区大小定义 */
// #define RPMSG_BUFFER_SIZE (sizeof(struct lp_rpmsg_msg))

/* 函数声明 */
int lp_rpmsg_init(void);
int lp_rpmsg_deinit(void);
int lp_rpmsg_to_cpux(char *send_msg, int len);
int8_t lp_rpmsg_set_keepalive_state(int8_t state);
int8_t lp_rpmsg_get_keepalive_state(void);

struct server_info *lp_rpmsg_get_server_info(void);
int8_t lp_rpmsg_get_server_info_status(void);
int8_t lp_rpmsg_set_server_info_status(int8_t statue);

#endif /* __LOW_POWER_RPMSG__H__ */
