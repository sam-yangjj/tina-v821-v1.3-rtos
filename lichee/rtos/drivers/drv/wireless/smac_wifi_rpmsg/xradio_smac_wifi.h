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
#ifndef __SMAC_WIFI_H__
#define __SMAC_WIFI_H__

#include <aw_list.h>

#define XRADIO_RPMSG_NAME "xradio_rpmsg"

#define XRADIO_FUNC_CALL_HOST_TO_SLAVE(n)                 (n<<15)
#define XRADIO_FUNC_CALL_SLAVE_TO_HOST_ACK_ID             (1<<14)
#define XRADIO_FUNC_CALL_SLAVE_TO_HOST_CONNECTED_ID       (1<<13)
#define XRADIO_WIFI_BT_RELEASE_CNT_ID                     (1<<12)
#define XRADIO_FUNC_CALL_SLAVE_TO_HOST_ID_MASK            (7<<12)
#define XRADIO_FUNC_CALL_INIT_ID                          (1)
#define XRADIO_FUNC_CALL_DEINIT_ID                        (2)
#define XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID              (3)
#define XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID        (4)
#define XRADIO_FUNC_CALL_VERSION_CHECK_ID                 (5)
#define XRADIO_FUNC_CALL_INIT_ID_ETF                      (6)
#define XRADIO_FUNC_CALL_DEINIT_ID_ETF                    (7)
#define XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID_ETF          (8)
#define XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID_ETF    (9)
#define XRADIO_FUNC_CALL_VERSION_CHECK_ID_ETF             (10)

#define XRADIO_FUNC_CALL_XR_COEX_RELEASE                  (11)
#define XRADIO_FUNC_CALL_XR_COEX_WAKEUP                   (12)

#define XRADIO_FUNC_CALL_FUNCTION_ID_MASK                 (0xF)

struct wsm_hdr {
	u16 len;
	u16 id;
};

struct wsm_buf {
	u8 *begin;
	u8 *data;
	u8 *end;
};

#ifndef CONFIG_DRIVERS_XRADIO
#define PAS_MEMORY_ADDRESS              0x09000000
#define XRADIO_FWMEM_FW2NET(addr)       ((addr) | 0x60000000)
#define SILICON_SYS_BASE_ADDR           (XRADIO_FWMEM_FW2NET(0))
#define SILICON_AHB_MEMORY_ADDRESS      (SILICON_SYS_BASE_ADDR + 0x08000000)
#define SILICON_PAC_BASE_ADDRESS        (SILICON_SYS_BASE_ADDR + 0x08018000)

#define TX_QUEUE_MAX  32
#define RX_QUEUE_MAX  64
#define TX_QUEUE_MASK  (TX_QUEUE_MAX-1)
#define RX_QUEUE_MASK  (RX_QUEUE_MAX-1)
typedef struct __queue_ctrl {
	volatile u32 read_idx;
	volatile u32 write_idx;
	volatile u32 buffer_idx;
} QUEUE_CTRL;

typedef struct __hif_queue {
	QUEUE_CTRL tx_ctrl;
	QUEUE_CTRL rx_ctrl;
	u32 tx_queue[TX_QUEUE_MAX];
	u32 rx_queue[RX_QUEUE_MAX];
	volatile u32 host_state;
  #define HIF_BYPASS_HOST_STAT_HOST_HAS_DATA_WAIT_TO_TX   (1 << 0)
	volatile u32 exception;
	volatile u32 rx_fifo_addr;
	volatile u32 rx_fifo_len;
} HIF_QUEUE;
#endif

int xradio_func_call_init(void);
void xradio_func_call_deinit(void);
void xradio_fw_call_host_func_handle(void);
int xradio_func_call_info_indication_handle(struct wsm_buf *buf);
void xradio_func_call_version_check(char *fw_version);
int xradio_func_call_init_etf(void);
void xradio_func_call_deinit_etf(void);
void xradio_fw_call_host_func_handle_etf(void);
int xradio_func_call_info_indication_handle_etf(struct wsm_buf *buf);
void xradio_func_call_version_check_etf(char *fw_version);

int wlan_smac_rpmsg_init(int is_ultra_standby);
void wlan_smac_rpmsg_deinit(int is_ultra_standby);
int xradio_notify_wlan_bt_power_state(uint8_t release_cnt);
#endif
