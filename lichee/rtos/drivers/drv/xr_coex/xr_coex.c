/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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

#include <sunxi_hal_efuse.h>
#include <sunxi-chips.h>
#include "xr_coex.h"
#include "kernel/os/os.h"
#include "sys/io.h"
#ifdef CONFIG_DRIVERS_V821_USE_SIP_WIFI
#include "xradio_smac_wifi.h"
#endif
#if CONFIG_BT_CLOSE_WLAN_FREQ_AUTO_ALIGN
#include "net/wlan/wlan_ext_req.h"
#include "../adapter/net_ctrl/net_ctrl.h"
#endif

#define COEX_LOG_ERR    1
#define COEX_LOG_DBG    0

#define COEX_SYSLOG    printf
#define COEX_LOG(flags, fmt, arg...)        \
	do {                                    \
		if (flags)                          \
			COEX_SYSLOG(fmt, ##arg);        \
	} while (0)

#define COEX_ERR(fmt, arg...)                               \
	do {                                                    \
		COEX_LOG(COEX_LOG_ERR, "[COEX ERR] %s():%d, "fmt,   \
			__func__, __LINE__, ##arg);                     \
	} while (0)

#define COEX_DBG(fmt, arg...)                               \
	do {                                                    \
		COEX_LOG(COEX_LOG_DBG, "[COEX DBG] %s():%d, "fmt,   \
			__func__, __LINE__, ##arg);                     \
	} while (0)


#define HIF_OV_CTRL                0x4A00018c
#define OV_DISABLE_CPU_CLK         (1<<0)
#define OV_RESET_CPU               (1<<1)
#define OV_WLAN_WAKE_UP            (1<<2)
#define OV_DISABLE_CPU_CLK_EN      (1<<4)
#define OV_RESET_CPU_EN            (1<<5)
#define OV_WLAN_WAKE_UP_EN         (1<<6)
#define OV_WLAN_IRQ_EN             (1<<7)
#define HIF_OV_CTRL_RESET_VALUE    (OV_WLAN_WAKE_UP_EN   | OV_RESET_CPU_EN | \
                                    OV_DISABLE_CPU_CLK_EN| OV_WLAN_IRQ_EN  | \
                                    OV_DISABLE_CPU_CLK   | OV_RESET_CPU)

#define HIF_WLAN_STATE             0x4A011068
#define WLAN_STATE_ACTIVE          (1<<0)

#define CCM_MOD_RST_CTRL           0x4A010518
#define CCM_WLAN_RST_BIT           (1<<0)

#define APB_SIP_IO_PN_MUX_MD       0x7AD84000
#define MOD_IOT                    0x1

#define APB_SIP_IO_PN_DRV0         0x7AD84004
#define DRV0_LEVEL2                0xAAAAAAAA

#define APB_SIP_IO_PN_DRV1         0x7AD84008
#define DRV1_LEVEL2                0xAA

#define APB_SIP_IO_AFE_CTRL_AON    0x7AD84024
#define DA_AFE_BG_START_UP_POS     4
#define DA_BG_TRIM_POS             0
#define RFIP0_DA_BG_TRIM_START     312
#define RFIP0_DA_BG_TRIM_WIDTH     4
#define DA_BG_TRIM_DEFAULT         0x8
#define RFIP0_ADC_OFFSET_START     388
#define RFIP0_ADC_OFFSET_WIDTH     8
#define RFIP0_ADC_LSB_START        396
#define RFIP0_ADC_LSB_WIDTH        9
#define RFIP0_Tnormal_START        439
#define RFIP0_Tnormal_WIDTH        12

#define APB_SIP_IO_PAD_MODE_SEL_CTRL    0x7AD8402C
#define SIP_PAD_MODE                    0x3

#define EFPG_DCXO_TRIM_FLAG_START       (1216)
#define EFPG_DCXO_TRIM_FLAG_NUM         (2)
#define EFPG_DCXO_TRIM_LEN              (8)

#define EFPG_DCXO_TRIM_START            (352)
#define EFPG_DCXO_TRIM_NUM              (EFPG_DCXO_TRIM_LEN)

#define EFPG_DCXO_TRIM1_START           (1218)
#define EFPG_DCXO_TRIM1_NUM             (EFPG_DCXO_TRIM_LEN)
#define EFPG_DCXO_TRIM2_START           (EFPG_DCXO_TRIM1_START + EFPG_DCXO_TRIM1_NUM)
#define EFPG_DCXO_TRIM2_NUM             (EFPG_DCXO_TRIM_LEN)

static uint8_t xr_coex_wsys_release_cnt;
static uint8_t xr_coex_wsys_wakeup_cnt;
static XR_OS_Mutex_t coex_mtx;

#if CONFIG_BT_CLOSE_WLAN_FREQ_AUTO_ALIGN
/* record wlan and bt coexistence state change, such as:
 * wlan or bt is open -> wlan open and bt open, record 1;
 * wlan open and bt open -> wlan open and bt close, record 0;
 * wlan open and bt open -> wlan close and bt open, record 0;
 */
static uint8_t xr_coex_status;
static uint8_t xr_coex_sta_connect;
static XR_OS_Mutex_t coex_freq_align_mtx;
#endif

#if defined(CONFIG_CHIP_V821B) && defined(CONFIG_INIT_NET_STACK)
#define XR_WF_SW_FW_BUF_FORCE_SMALL_BUF  0
#define XR_WF_SW_FW_BUF_FORCE_BIG_BUF    1
#define XR_WF_SW_FW_BUF_FORCE_DISABLE    2
#define XR_WF_SW_FW_BUF_FORCE_ENABLE     3
int xradio_wlan_switch_epta_stat(uint8_t release_num, uint8_t force_buf);
#endif

static void xr_coex_lock(void)
{
	if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821B) {
		COEX_DBG("lock\n");
		if (XR_OS_MutexLock(&coex_mtx, XR_OS_WAIT_FOREVER) != XR_OS_OK)
			COEX_ERR("lock err\n");
	}
}

static void xr_coex_unlock(void)
{
	if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821B) {
		COEX_DBG("unlock\n");
		if (XR_OS_MutexUnlock(&coex_mtx) != XR_OS_OK)
			COEX_ERR("unlock err\n");
	}
}

static void xr_coex_set_wireless_sip_io(void)
{
	uint8_t afe_da_bg_trim = DA_BG_TRIM_DEFAULT;
	uint32_t IsAntiFuseSet = 0;
	uint8_t u8AntiFuseBits[(RFIP0_Tnormal_WIDTH + 7)/8];
	writel(MOD_IOT, APB_SIP_IO_PN_MUX_MD);
	writel(DRV0_LEVEL2, APB_SIP_IO_PN_DRV0);
	writel(DRV1_LEVEL2, APB_SIP_IO_PN_DRV1);
	hal_efuse_read_ext(RFIP0_Tnormal_START, RFIP0_Tnormal_WIDTH, u8AntiFuseBits);
	IsAntiFuseSet |= (u8AntiFuseBits[0] | u8AntiFuseBits[1]);
	hal_efuse_read_ext(RFIP0_ADC_OFFSET_START, RFIP0_ADC_OFFSET_WIDTH, u8AntiFuseBits);
	IsAntiFuseSet |= u8AntiFuseBits[0];
	hal_efuse_read_ext(RFIP0_ADC_LSB_START, RFIP0_ADC_LSB_WIDTH, u8AntiFuseBits);
	IsAntiFuseSet |= (u8AntiFuseBits[0] | u8AntiFuseBits[1]);
	if (IsAntiFuseSet)
		hal_efuse_read_ext(RFIP0_DA_BG_TRIM_START, RFIP0_DA_BG_TRIM_WIDTH, &afe_da_bg_trim);
	writel((1 << DA_AFE_BG_START_UP_POS) | (afe_da_bg_trim << DA_BG_TRIM_POS),
	       APB_SIP_IO_AFE_CTRL_AON);
	if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821) {
		uint32_t val;
		val = readl(APB_SIP_IO_PAD_MODE_SEL_CTRL) | (SIP_PAD_MODE);
		writel(val, APB_SIP_IO_PAD_MODE_SEL_CTRL);
	}
}

int xr_coex_get_wlan_status(void)
{
	return !(readl(HIF_OV_CTRL) & OV_RESET_CPU);
}

#if CONFIG_BT_CLOSE_WLAN_FREQ_AUTO_ALIGN
static void xr_coex_freq_align_mtx_lock(void)
{
	COEX_DBG("lock\n");
	if (XR_OS_MutexLock(&coex_freq_align_mtx, XR_OS_WAIT_FOREVER) != XR_OS_OK)
		COEX_ERR("lock err\n");
}

static void xr_coex_freq_align_mtx_unlock(void)
{
	COEX_DBG("unlock\n");
	if (XR_OS_MutexUnlock(&coex_freq_align_mtx) != XR_OS_OK)
		COEX_ERR("unlock err\n");
}

int xr_coex_set_freq_auto_align(uint8_t msg_call, uint8_t sta_connect, uint8_t coex_status)
{
	int ret = 0;
	bool need_set = false;
	uint32_t mode = 0;
	wlan_ext_freq_auto_align_cfg_t cfg = {0};

	xr_coex_freq_align_mtx_lock();

	if (msg_call)
		xr_coex_sta_connect = sta_connect;
	else
		xr_coex_status = coex_status;

	if (msg_call) {
		if (xr_coex_sta_connect) {
			if (!xr_coex_status) {
				mode = FFA_EXT_MODE_OPEN_USE_DEFAULT;
				need_set = true;
			}
		} else {
			mode = FFA_EXT_MODE_CLOSE_RECOVER_TRIM;
			need_set = true;
		}
	} else {
		if (xr_coex_get_wlan_status() && xr_coex_sta_connect) {
			mode = xr_coex_status ? FFA_EXT_MODE_CLOSE_RECOVER_TRIM : FFA_EXT_MODE_OPEN_USE_DEFAULT;
			need_set = true;
		}
	}

	if (need_set) {
		cfg.mode = mode;
		ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_FREQ_AUTO_ALIGN, (uint32_t)(uintptr_t)(&cfg));
		if (ret)
			COEX_ERR("set freq auto align fail, call:%d cfg.mode:%d\n", msg_call, cfg.mode);
	}

	xr_coex_freq_align_mtx_unlock();
	return ret;
}
#endif

int xr_coex_release_wireless_sys(uint8_t enable)
{
	int ret = 0;
	uint32_t val;

	xr_coex_lock();

	COEX_DBG("%d, %d\n", enable, xr_coex_wsys_release_cnt);
	if (enable) {
		if (xr_coex_wsys_release_cnt == 0) {
			writel(HIF_OV_CTRL_RESET_VALUE, HIF_OV_CTRL);
			val = readl(CCM_MOD_RST_CTRL) & ~(CCM_WLAN_RST_BIT);
			writel(val, CCM_MOD_RST_CTRL);

			while (1) {
				if (readl(HIF_WLAN_STATE) & WLAN_STATE_ACTIVE)
					COEX_ERR("wlan still active\n");
				else
					break;
			}
			val = readl(CCM_MOD_RST_CTRL) | CCM_WLAN_RST_BIT;
			writel(val, CCM_MOD_RST_CTRL);
			XR_OS_MSleep(20);
		}
		xr_coex_wsys_release_cnt++;

#if defined(CONFIG_CHIP_V821B) && defined(CONFIG_INIT_NET_STACK)
		if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821B) {
#ifdef CONFIG_XR_WF_SW_FW_BUF_DISABLE
			xradio_wlan_switch_epta_stat(xr_coex_wsys_release_cnt, XR_WF_SW_FW_BUF_FORCE_DISABLE);
#else
			xradio_wlan_switch_epta_stat(xr_coex_wsys_release_cnt, 0);
#endif
		}
#endif

	} else {
		if (xr_coex_wsys_release_cnt > 0) {
			if (--xr_coex_wsys_release_cnt == 0) {
				val = readl(CCM_MOD_RST_CTRL) & ~(CCM_WLAN_RST_BIT);
				writel(val, CCM_MOD_RST_CTRL);
				writel(OV_DISABLE_CPU_CLK|OV_RESET_CPU, HIF_OV_CTRL);
			}

#if defined(CONFIG_CHIP_V821B) && defined(CONFIG_INIT_NET_STACK)
			if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821B) {
#ifdef CONFIG_XR_WF_SW_FW_BUF_DISABLE
				xradio_wlan_switch_epta_stat(xr_coex_wsys_release_cnt, XR_WF_SW_FW_BUF_FORCE_DISABLE);
#else
				xradio_wlan_switch_epta_stat(xr_coex_wsys_release_cnt, 0);
#endif
			}
#endif
		} else {
			COEX_ERR("decrease cnt %d is invalid\n", xr_coex_wsys_release_cnt);
			ret = -1;
		}
	}
#ifdef CONFIG_DRIVERS_V821_USE_SIP_WIFI
	xradio_notify_wlan_bt_power_state(xr_coex_wsys_release_cnt);
#endif

	xr_coex_unlock();

#ifdef CONFIG_BT_CLOSE_WLAN_FREQ_AUTO_ALIGN
	if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821B) {
		if (enable && (xr_coex_wsys_release_cnt == 2)) {
			xr_coex_set_freq_auto_align(0, 0, 1);
		} else if (!enable && (xr_coex_wsys_release_cnt == 1)) {
			xr_coex_set_freq_auto_align(0, 0, 0);
		}
	}
#endif
	return ret;

}

int xr_coex_wakeup_wireless_sys(uint8_t enable)
{
	int ret = 0;

	xr_coex_lock();

	COEX_DBG("%d, %d\n", enable, xr_coex_wsys_wakeup_cnt);
	if (enable) {
		if (xr_coex_wsys_wakeup_cnt == 0) {
			int i = 0;
			set_wbit(HIF_OV_CTRL, OV_WLAN_WAKE_UP);
			while (!(readl(HIF_WLAN_STATE) & WLAN_STATE_ACTIVE) && (i < 500)) {
				XR_OS_MSleep(1);
				i++;
			}
			if (i >= 500) { /*time out*/
				COEX_ERR("WLAN device is not responding:OV_CTRL=0x%08x, HIF_WLAN_STATE=0x%08x.\n",
				         readl(HIF_OV_CTRL), readl(HIF_WLAN_STATE));
				ret = -1;
			}
			if (readl(HIF_OV_CTRL) & OV_RESET_CPU) {
				xr_coex_set_wireless_sip_io();
			}
		}
		xr_coex_wsys_wakeup_cnt++;
	} else {
		if (xr_coex_wsys_wakeup_cnt > 0) {
			if (--xr_coex_wsys_wakeup_cnt == 0) {
				clr_wbit(HIF_OV_CTRL, OV_WLAN_WAKE_UP);
			}
		} else {
			COEX_ERR("decrease cnt %d is invalid\n", xr_coex_wsys_wakeup_cnt);
			ret = -1;
		}
	}

	xr_coex_unlock();
	return ret;
}

int xr_coex_get_freq_offset_from_efuse(uint8_t *freq_offset)
{
	uint8_t flag, data;
	uint32_t start_bit;

	COEX_DBG("flag %d %d, trim %d %d, trim1:%d %d, trim2:%d %d\n",
	      EFPG_DCXO_TRIM_FLAG_START, EFPG_DCXO_TRIM_FLAG_NUM,
	      EFPG_DCXO_TRIM_START, EFPG_DCXO_TRIM_LEN,
	      EFPG_DCXO_TRIM1_START, EFPG_DCXO_TRIM_LEN,
	      EFPG_DCXO_TRIM2_START, EFPG_DCXO_TRIM_LEN);

	/* flag */
	if (hal_efuse_read_ext(EFPG_DCXO_TRIM_FLAG_START, EFPG_DCXO_TRIM_FLAG_NUM, (uint8_t *)&flag)) {
		return -1;
	}
	flag &= ((1 << EFPG_DCXO_TRIM_FLAG_NUM) - 1);
	COEX_DBG("r start %d, bits %d, flag 0x%x\n", EFPG_DCXO_TRIM_FLAG_START, EFPG_DCXO_TRIM_FLAG_NUM, flag);

	if (flag == 0)
		return -4;
	else if (flag == 1)
		start_bit = EFPG_DCXO_TRIM1_START;
	else if (flag == 3)
		start_bit = EFPG_DCXO_TRIM2_START;
	else {
		COEX_ERR("%s(), flag (%d, %d) = 0x%x is invalid\n",
		      __func__, EFPG_DCXO_TRIM_FLAG_START, EFPG_DCXO_TRIM_FLAG_NUM, flag);
		return -2;
	}

	/* data */
	data = 0;
	COEX_DBG("r data, start %d, bits %d\n", start_bit, EFPG_DCXO_TRIM_LEN);
	if (hal_efuse_read_ext(start_bit, EFPG_DCXO_TRIM_LEN, &data)) {
		return -3;
	}
	COEX_SYSLOG("efuse freqoffset:%d\n", data);
	if (data == 0)
		return -4;
	*freq_offset = data;

	return 0;
}

int xr_coex_init(void)
{
	int ret;

	XR_OS_MutexSetInvalid(&coex_mtx);
	ret = XR_OS_MutexCreate(&coex_mtx);
	if (ret != XR_OS_OK) {
		COEX_ERR("coex_init fail!!!\n");
		return -1;
	}

#ifdef CONFIG_BT_CLOSE_WLAN_FREQ_AUTO_ALIGN
	XR_OS_MutexSetInvalid(&coex_freq_align_mtx);
	ret = XR_OS_MutexCreate(&coex_freq_align_mtx);
	if (ret != XR_OS_OK) {
		COEX_ERR("coex_freq_align_mtx fail!!!\n");
		XR_OS_MutexDelete(&coex_mtx);
		return -1;
	}
#endif
	COEX_DBG("coex_init success\n");
	return 0;
}

int xr_coex_deinit(void)
{
	int ret;

#ifdef CONFIG_BT_CLOSE_WLAN_FREQ_AUTO_ALIGN
	xr_coex_status = 0;
	xr_coex_sta_connect = 0;
	ret = XR_OS_MutexDelete(&coex_freq_align_mtx);
	if (ret != XR_OS_OK) {
		COEX_ERR("delete coex_freq_align_mtx fail!!!\n");
		return -1;
	}
#endif

	xr_coex_wsys_release_cnt = 0;
	xr_coex_wsys_wakeup_cnt = 0;
	ret = XR_OS_MutexDelete(&coex_mtx);
	if (ret != XR_OS_OK) {
		COEX_ERR("delete coex_mtx fail!!!\n");
		return -1;
	}

	COEX_DBG("coex_deinit success\n");
	return 0;
}

