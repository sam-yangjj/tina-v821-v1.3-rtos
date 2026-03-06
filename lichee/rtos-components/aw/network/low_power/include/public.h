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
#ifndef __PUBLIC__H__
#define __PUBLIC__H__

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "port_time.h"
#include <console.h>
#include <pthread.h>
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

#include "sysinfo.h"
#include "net_ctrl.h"
#include "net/wlan/wlan_ext_req.h"

#define LOG_INFO_ENABLE  1
#define LOG_WARN_ENABLE  1
#define LOG_ERR_ENABLE   1
#define LOG_DEBUG_ENABLE 0

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define COLOR_RESET "\033[0m"

#define LP_LOG_INFO(fmt, ...) \
	do { \
		if (LOG_INFO_ENABLE) \
			printf("[lp_keep][info]%s(%d):"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#define LP_LOG_WARN(fmt, ...) \
	do { \
		if (LOG_WARN_ENABLE) \
			printf(COLOR_YELLOW "[lp_keep][warn]%s(%d):"fmt COLOR_RESET, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#define LP_LOG_ERR(fmt, ...) \
	do { \
		if (LOG_ERR_ENABLE) \
			printf(COLOR_RED "[lp_keep][err]%s(%d):"fmt COLOR_RESET, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#define LP_LOG_DBG(fmt, ...) \
	do { \
		if (LOG_DEBUG_ENABLE) \
			printf(BG_GREEN "[lp_keep][dbg]%s(%d):"fmt COLOR_RESET, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)


#define LP_RPMSG_LOG(fmt, ...) \
		do { \
			if (LOG_INFO_ENABLE) \
				printf("[lp_rpmsg]%s(%d):"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		} while(0)

#define LP_RPMSG_LOG_DBG(fmt, ...) \
		do { \
			if (LOG_DEBUG_ENABLE) \
				printf(BG_GREEN "[lp_rpmsg]%s(%d):"fmt COLOR_RESET, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		} while(0)

typedef enum {
	LOW_POWER_STRATEGY_DISABLE = 0U,
	LOW_POWER_STRATEGY_WAIT_SER_REPLY,
	LOW_POWER_STRATEGY_NOT_WAIT_SER_REPLY,
} low_power_strategy_t;

#if (defined CONFIG_COMPONENTS_LOW_POWER_STRATEGY_STANDBY_WAIT_SER_REPLY)
#define DEFAULLT_USE_STRATEGY    LOW_POWER_STRATEGY_WAIT_SER_REPLY
#elif (defined CONFIG_COMPONENTS_LOW_POWER_STRATEGY_STANDBY_NOT_WAIT_SER_REPLY)
#define DEFAULLT_USE_STRATEGY    LOW_POWER_STRATEGY_NOT_WAIT_SER_REPLY
#else
#define DEFAULLT_USE_STRATEGY    LOW_POWER_STRATEGY_DISABLE
#endif

int low_power_wlan_set_fw_try_wait_tcp_pkt_tmo(uint16_t timeout_ms);
int low_power_set_keepalive_use_strategy(int socket, int enable, int strategy);

#endif
