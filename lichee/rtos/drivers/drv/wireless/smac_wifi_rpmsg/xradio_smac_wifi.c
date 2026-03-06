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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "xradio_smac_wifi.h"
#include "drv/xr_coex/xr_coex.h"
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/rpmsg_master.h>


#define RPMSG_DEBUG_LEVEL 1
#define ALOGD(format, args...)           \
		do {                             \
			if (RPMSG_DEBUG_LEVEL > 1)   \
				printf(format, ##args);  \
		} while (0)
#define ALOGA(format, args...)           \
		do {                             \
			if (RPMSG_DEBUG_LEVEL > 0)   \
				printf(format, ##args);  \
		} while (0)
#define ALOGE(format, args...)           \
		printf(format, ##args);          \

#define SMAC_INIT_THREAD_SIZE    (1024)
static void *smac_init_thread = NULL;
static struct rpmsg_endpoint *smac_rpmsg_ept;


#ifndef CONFIG_DRIVERS_XRADIO
int xradio_debug_level = 2;
HIF_QUEUE *pHif_bypass = NULL;
uint32_t FW_ADDR_TO_APP_ADDR(uint32_t addr)
{
	if ((addr & 0xFFF00000) == 0xFFF00000)
		addr = (SILICON_AHB_MEMORY_ADDRESS + (addr & 0x1FFFF)); /*max 128K*/
	else if ((addr & PAS_MEMORY_ADDRESS) == PAS_MEMORY_ADDRESS) {
		u32 offset = (addr & 0x3FFFF);
		if ((addr & 0x09400000) == 0x09400000) {
			if (!pHif_bypass) {
				ALOGE("%s:HIF_QUEUE is not init! Error Addr=0x%08x.\n", __func__, addr);
				return (u32)NULL;
			}
			if (offset >= pHif_bypass->rx_fifo_len)
				offset = offset - pHif_bypass->rx_fifo_len;
			addr = pHif_bypass->rx_fifo_addr + offset;
			ALOGD("%s: Rx Addr=0x%08x(offset=%d).\n", __func__, addr, offset);
		} else {
			addr = (SILICON_PAC_BASE_ADDRESS + offset);
		}
	}
	return XRADIO_FWMEM_FW2NET(addr);
}
#endif

int xradio_notify_wlan_bt_power_state(uint8_t release_cnt)
{
	int ret;
	uint16_t data;

	if (smac_rpmsg_ept == NULL) {
		ALOGE("rpmsg link not established\n");
		return -1;
	}

	data = XRADIO_WIFI_BT_RELEASE_CNT_ID | release_cnt;
	ret = openamp_rpmsg_send(smac_rpmsg_ept, &data, 2);
	if (ret < 0) {
		ALOGE("rpmsg send fail\n");
		return -2;
	}

	return 0;
}

static int xradio_rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	char *buff = (char *)data;
	uint16_t id = *(uint16_t *)data;


	if (id & XRADIO_FUNC_CALL_HOST_TO_SLAVE(1))
	{
		ALOGD("recv host id 0x%x\n", id);
		if ((id & XRADIO_FUNC_CALL_SLAVE_TO_HOST_ID_MASK) ==
			XRADIO_FUNC_CALL_SLAVE_TO_HOST_CONNECTED_ID) {
				ALOGE("rpmsg link has been established\n");
				return 0;
		}
		id &= XRADIO_FUNC_CALL_FUNCTION_ID_MASK;
		switch (id) {
			case XRADIO_FUNC_CALL_INIT_ID:
				xradio_func_call_init();
				break;
			case XRADIO_FUNC_CALL_DEINIT_ID:
				xradio_func_call_deinit();
				break;
			case XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID:
				xradio_fw_call_host_func_handle();
				break;
			case XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID: {
				struct wsm_buf wsm_buf;
				struct wsm_hdr *wsm = (struct wsm_hdr *)(buff + 2);
				wsm_buf.begin = (u8 *)&wsm[0];
				wsm_buf.data = (u8 *)&wsm[1];
				wsm_buf.end = &wsm_buf.begin[(uint32_t)(wsm->len)];
				ALOGD("%s len = %d", __func__, (uint32_t)(wsm->len));
				xradio_func_call_info_indication_handle(&wsm_buf);
				break;
			}
			case XRADIO_FUNC_CALL_VERSION_CHECK_ID:
				xradio_func_call_version_check(buff + 2);
				break;
			case XRADIO_FUNC_CALL_INIT_ID_ETF:
				xradio_func_call_init_etf();
				break;
			case XRADIO_FUNC_CALL_DEINIT_ID_ETF:
				xradio_func_call_deinit_etf();
				break;
			case XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID_ETF:
				xradio_fw_call_host_func_handle_etf();
				break;
			case XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID_ETF: {
				struct wsm_buf wsm_buf;
				struct wsm_hdr *wsm = (struct wsm_hdr *)(buff + 2);
				wsm_buf.begin = (u8 *)&wsm[0];
				wsm_buf.data = (u8 *)&wsm[1];
				wsm_buf.end = &wsm_buf.begin[(uint32_t)(wsm->len)];
				ALOGD("%s len = %d", __func__, (uint32_t)(wsm->len));
				xradio_func_call_info_indication_handle_etf(&wsm_buf);
				break;
			}
			case XRADIO_FUNC_CALL_VERSION_CHECK_ID_ETF:
				xradio_func_call_version_check_etf(buff + 2);
				break;
			case XRADIO_FUNC_CALL_XR_COEX_RELEASE: {
				uint8_t enable = *(buff + 2);
				xr_coex_release_wireless_sys(enable);
				break;
			}
			case XRADIO_FUNC_CALL_XR_COEX_WAKEUP: {
				uint8_t enable = *(buff + 2);
				xr_coex_wakeup_wireless_sys(enable);
				break;
			}
			default :
				ALOGE("receive unknow function id from host %02x\n",id);
		}
		id |= XRADIO_FUNC_CALL_SLAVE_TO_HOST_ACK_ID;
		openamp_rpmsg_send(ept, &id, 2);
		ALOGD("send ack id %02x to host\n", id);
	} else {
		ALOGE("receive error msg from host %02x\n",id);
	}
	return 0;

}

static void xradio_rpmsg_unbind_callback(struct rpmsg_endpoint *ept)
{

}

static void xradio_rpmsg_create(void *param)
{
	(void)param;

	uint32_t src_addr = RPMSG_ADDR_ANY;
	uint32_t dst_addr = RPMSG_ADDR_ANY;
	short connected = XRADIO_FUNC_CALL_SLAVE_TO_HOST_CONNECTED_ID;

	ALOGA("create %s rpmsg endpoint\r\n", XRADIO_RPMSG_NAME);

	smac_rpmsg_ept = openamp_ept_open(XRADIO_RPMSG_NAME, 0, src_addr, dst_addr,
					NULL, xradio_rpmsg_ept_callback, xradio_rpmsg_unbind_callback);
	if (!smac_rpmsg_ept) {
		ALOGE("Failed to Create Endpoint\r\n");
		goto out;
	}

	openamp_rpmsg_send(smac_rpmsg_ept, &connected, 2);
	ALOGA("creat rpmsg succeed\n");
out:
	smac_init_thread = NULL;
	hal_thread_stop(smac_init_thread);
}

int wlan_smac_rpmsg_init(int is_ultra_standby)
{
	ALOGA("wlan_smac_rpmsg_init!\n");

	smac_init_thread = hal_thread_create(xradio_rpmsg_create, NULL,"smac_rpmsg_init",
						                 SMAC_INIT_THREAD_SIZE, HAL_THREAD_PRIORITY_SYS);
	if (smac_init_thread != NULL)
		hal_thread_start(smac_init_thread);
	else {
		ALOGE("fail to create smac_rpmsg_init thread\n");
		return -1;
	}
	return 0;
}

void wlan_smac_rpmsg_deinit(int is_ultra_standby)
{
	ALOGA("wlan_smac_rpmsg_deinit!\n");
	if (smac_init_thread) {
		ALOGE("smac_rpmsg_init is running, now delete it\n");
		hal_thread_stop(smac_init_thread);
	}

	if (smac_rpmsg_ept) {
		openamp_ept_close(smac_rpmsg_ept);
		smac_rpmsg_ept = NULL;
	}
}

