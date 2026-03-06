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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "interrupt.h"
#include <portmacro.h>
#include "FreeRTOS.h"
#include "task.h"
#include <hal_time.h>

#include <console.h>

#ifdef CONFIG_DRIVERS_MSGBOX
#include <hal_msgbox.h>
#endif

#if defined(CONFIG_INIT_NET_STACK) || defined(CONFIG_DRIVERS_V821_USE_SIP_WIFI) || \
    defined(CONFIG_XRADIO_BT_CONTROLLER)
#include "drv/xr_coex/xr_coex.h"
#endif

#ifdef CONFIG_COMPONENTS_PM
extern int pm_init(int argc, char **argv);
#endif

#define RTC_DATA_REGS(n)			(0x4a000000 + 0x200 + (n) * 4)
#define AG_MODE_RTC_IDX				(1)
#define AG_MODE_RTC_OFS				(0)
#define AG_MODE_RTC_WIDTH			(4)

#define SET_AG_MODE_RTC_VAL(val)	\
		do { \
			u32 tmp; \
			tmp = readl(RTC_DATA_REGS(AG_MODE_RTC_IDX)); \
			tmp &= ~(((1 << AG_MODE_RTC_WIDTH) - 1)  << AG_MODE_RTC_OFS); \
			tmp |= (((val) & ((1 << AG_MODE_RTC_WIDTH) - 1)) << AG_MODE_RTC_OFS); \
			writel(tmp, RTC_DATA_REGS(AG_MODE_RTC_IDX)); \
		} while(0)

#define GET_AG_MODE_RTC_VAL()	\
		({  u32 __val; \
			__val = readl(RTC_DATA_REGS(AG_MODE_RTC_IDX)); \
			__val >>= AG_MODE_RTC_OFS; \
			__val &= ((1 << AG_MODE_RTC_WIDTH) - 1); \
			__val; })

#ifdef CONFIG_GEN_DIGEST
/* format: 4bytes image size + 16bytes digest */
__attribute__((section(".digest"))) const unsigned char digest_data[20];
#endif

#ifdef CONFIG_COMPONENTS_OPENAMP
#include <openamp/sunxi_helper/rpmsg_master.h>
#include <openamp/sunxi_helper/openamp.h>

#ifdef CONFIG_DRIVERS_VIN
extern int csi_init(int argc, const char **atgv);
#ifdef CONFIG_VIN_USE_PM
extern void csi_pm_init(void);
#endif
#endif

#ifdef CONFIG_COMPONENTS_RPBUF
extern int rpbuf_init(void);
#endif

#ifdef CONFIG_VIN_WAIT_AMP_INIT
extern int vin_continue_init(void);
#endif

void init_amp_app(void)
{
#ifdef CONFIG_VIN_WAIT_AMP_INIT
#ifdef CONFIG_COMPONENTS_AMP_USER_RESOURCE
	extern void show_all_user_resource(void);
	show_all_user_resource();
#endif
#ifdef CONFIG_COMPONENTS_RPBUF
	rpbuf_init();
#endif

	int ret = vin_continue_init();
	if (ret)
	{
		printf("notify vin driver continue to init failed, ret: %d\n", ret);
	}

#ifdef CONFIG_RPMSG_NOTIFY
	rpmsg_notify_init();
#endif

#ifdef CONFIG_RPMSG_CLIENT
	rpmsg_ctrldev_create();
#endif

#ifdef CONFIG_RPMSG_HEARBEAT
	extern int rpmsg_heart_init(void);
	rpmsg_heart_init();
#endif

#ifdef CONFIG_MULTI_CONSOLE
	extern int multiple_console_init(void);
	multiple_console_init();
#endif

#else
#ifdef CONFIG_COMPONENTS_AMP_USER_RESOURCE
	extern void show_all_user_resource(void);
	show_all_user_resource();
#endif

#ifdef CONFIG_RPMSG_CLIENT
	rpmsg_ctrldev_create();
#endif

#ifdef CONFIG_RPMSG_HEARBEAT
	extern int rpmsg_heart_init(void);
	rpmsg_heart_init();
#endif

#ifdef CONFIG_MULTI_CONSOLE
	extern int multiple_console_init(void);
	multiple_console_init();
#endif

#ifdef CONFIG_COMPONENTS_RPBUF
	rpbuf_init();
#endif
#endif
}

void deinit_amp_app(void)
{
#ifdef CONFIG_COMPONENTS_RPBUF
	extern void rpbuf_deinit(int rproc_id);
	rpbuf_deinit(0);
#endif

#ifdef CONFIG_MULTI_CONSOLE
	extern int multiple_console_deinit(void);
	multiple_console_deinit();
#endif

#ifdef CONFIG_RPMSG_HEARBEAT
	extern int rpmsg_heart_deinit(void);
	rpmsg_heart_deinit();
#endif

#ifdef CONFIG_RPMSG_CLIENT
	rpmsg_ctrldev_release();
#endif

#ifdef CONFIG_RPMSG_NOTIFY
	extern int rpmsg_notify_deinit(void);
	rpmsg_notify_deinit();
#endif
}

void init_amp_framework(void)
{
#if defined(CONFIG_ARCH_RISCV_PMP) && !defined(CONFIG_PMP_EARLY_ENABLE)
	__attribute__((__unused__)) int pmp_ret;

#ifdef CONFIG_COMPONENTS_RPBUF
	extern int set_pmp_for_rpbuf_reserved_mem(void);
	pmp_ret = set_pmp_for_rpbuf_reserved_mem();
	if (pmp_ret)
	{
		printf("set PMP for rpbuf reserved mem faild, ret: %d\n", pmp_ret);
	}
#endif

#endif

	openamp_init();
}

void deinit_amp_framework(void)
{
	openamp_deinit();
}

#ifdef CONFIG_COMPONENTS_RPBUF_UART
static int mode;
#endif

void openamp_init_thread(void *param)
{
	(void)param;
	init_amp_framework();
	init_amp_app();

#ifdef CONFIG_COMPONENTS_RPBUF_UART
	extern int aglink_rpbuff_init(int8_t uart_id);
#ifdef CONFIG_RPBUFF_UART_DEBUG_LOG
	extern void rpbuff_uart_set_debug_log(uint8_t flags);
	rpbuff_uart_set_debug_log(1);
#endif
	aglink_rpbuff_init(CONFIG_RPBUFF_UART_PORT);
#endif
#ifdef CONFIG_INIT_NET_STACK
	extern int wlan_fmac_xrlink_init(int is_ultra_standby);
	wlan_fmac_xrlink_init(0);
#endif

#ifdef CONFIG_DRIVERS_V821_USE_SIP_WIFI
	extern int wlan_smac_rpmsg_init(int is_ultra_standby);
#ifdef CONFIG_COMPONENTS_RPBUF_UART
	if (mode == 2 || mode == 3 || mode == 8 || mode == 15)
		wlan_smac_rpmsg_init(0);
	else
		printf("xradio rpmsg do not create");
#else
	wlan_smac_rpmsg_init(0);
#endif
#endif

	hal_thread_stop(NULL);
}

void pm_openamp_init_thread(void *param)
{
	(void)param;
	init_amp_framework();
	init_amp_app();

#ifdef CONFIG_INIT_NET_STACK
	extern int wlan_fmac_xrlink_init(int is_ultra_standby);
	wlan_fmac_xrlink_init(1);
#endif

#ifdef CONFIG_DRIVERS_V821_USE_SIP_WIFI
	extern int wlan_smac_rpmsg_init(int is_ultra_standby);
#ifdef CONFIG_COMPONENTS_RPBUF_UART
	if (mode == 2 || mode == 3 || mode == 8 || mode == 15)
		wlan_smac_rpmsg_init(1);
	else
		printf("xradio rpmsg do not create");
#else
	wlan_smac_rpmsg_init(1);
#endif
#endif

	hal_thread_stop(NULL);
}

int init_openamp(void)
{
	void *thread;
	thread = hal_thread_create(pm_openamp_init_thread, NULL,
							   "amp_init", 8 * 1024, HAL_THREAD_PRIORITY_SYS);
	if (thread != NULL)
		hal_thread_start(thread);
	else
		return -1;

	return 0;
}

int deinit_openamp_sync(void)
{
#ifdef CONFIG_DRIVERS_V821_USE_SIP_WIFI
	/* todo: implement this function */
	//wlan_smac_rpmsg_deinit();
#endif

#ifdef CONFIG_INIT_NET_STACK
	extern void wlan_fmac_xrlink_deinit(void);
//	wlan_fmac_xrlink_deinit();
#endif
	deinit_amp_app();
	deinit_amp_framework();

	return 0;
}
#endif


#ifdef CONFIG_COMMAND_AUTO_START_MEMTESTER
static void auto_memtester_thread(void *param)
{
	extern int cmd_memtest();
	cmd_memtest();
	hal_thread_stop(NULL);
}
#endif

#ifdef CONFIG_COMPONENTS_TCPIP
extern void cmd_tcpip_init(void);
#endif

#ifdef CONFIG_VIN_USE_PM
/* implementations of vin/vin.c */
void csi_init_thread(void *param)
{
	(void)param;
	int ret;

	ret = csi_init(0, NULL);
	if (ret) {
		rpmsg_notify("rt-media", NULL, 0);
		printf("csi init fail!\n");
	} else {
		rpmsg_notify("twi0", NULL, 0);
		// rpmsg_notify("tdm0", NULL, 0);
		rpmsg_notify("isp0", NULL, 0);
		rpmsg_notify("scaler0", NULL, 0);
		rpmsg_notify("scaler4", NULL, 0);
		rpmsg_notify("vinc0", NULL, 0);
		rpmsg_notify("vinc4", NULL, 0);
		printf("csi init success!\n");
	}
	hal_thread_stop(NULL);
}
#endif

void cpu0_app_entry(void *param)
{
	(void)param;

#ifdef CONFIG_COMPONENTS_RPBUF_UART
	extern int aglink_rpbuff_uart_init(int8_t uart_id);
	extern int aglink_get_mode(void);

	aglink_rpbuff_uart_init(CONFIG_RPBUFF_UART_PORT);

	mode = aglink_get_mode();

	printf("ai glass mode:%d\n", mode);

	SET_AG_MODE_RTC_VAL(mode);
#endif

#ifdef CONFIG_COMPONENTS_PM
	pm_init(1, NULL);
#endif

#ifdef CONFIG_COMPONENTS_LOW_POWER_APP
	extern int low_power_app(void);
	low_power_app();
#endif

#ifdef CONFIG_COMPONENTS_OPENAMP
	void *thread;
	thread = hal_thread_create(openamp_init_thread, NULL,
							   "amp_init", 8 * 1024, HAL_THREAD_PRIORITY_APP - 1);
	if (thread != NULL)
		hal_thread_start(thread);
#endif

#ifdef CONFIG_COMPONENTS_TCPIP
	//tcpip stack init
	cmd_tcpip_init();
#endif

#if defined(CONFIG_INIT_NET_STACK) || defined(CONFIG_DRIVERS_V821_USE_SIP_WIFI) || \
    defined(CONFIG_XRADIO_BT_CONTROLLER)
#ifdef CONFIG_COMPONENTS_RPBUF_UART
	if (mode == 2 || mode == 3 || mode == 8 || mode == 15)
		xr_coex_init();
	else
		printf("not call xr_coex_init\n");
#else
	xr_coex_init();
#endif
#endif

#if defined(CONFIG_COMPONENT_CLI) && !defined(CONFIG_UART_MULTI_CONSOLE_AS_MAIN)
	vCommandConsoleStart(0x1000, HAL_THREAD_PRIORITY_CLI, NULL);
#endif

#ifdef CONFIG_WAIT_SPIF_CONTROLLER
	extern void flash_load_data_async(void);
	flash_load_data_async();
#endif

#ifdef CONFIG_DRIVERS_VIN
	int ret;
#ifdef CONFIG_COMPONENTS_RPBUF_UART
	if (mode == 0 || mode == 1 || mode == 4 || mode == 8 || mode == 9 || mode == 10 || mode == 11) {  /* 0:photo, 1:vedio, 4:AI, 8:livestream, 9:AOV 10:vertical video 11:video preview*/
#endif
		ret = csi_init(0, NULL);
		if (ret) {
			rpmsg_notify("rt-media", NULL, 0);
			printf("csi init fail!\n");
		} else {
			rpmsg_notify("twi0", NULL, 0);
			rpmsg_notify("tdm0", NULL, 0);
			rpmsg_notify("isp0", NULL, 0);
			rpmsg_notify("scaler0", NULL, 0);
			rpmsg_notify("scaler4", NULL, 0);
			rpmsg_notify("vinc0", NULL, 0);
			rpmsg_notify("vinc4", NULL, 0);
			rpmsg_notify("isp1", NULL, 0);
			rpmsg_notify("scaler1", NULL, 0);
			rpmsg_notify("vinc1", NULL, 0);
			rpmsg_notify("scaler5", NULL, 0);
			rpmsg_notify("vinc5", NULL, 0);
			printf("csi init success!\n");
		}
#ifdef CONFIG_COMPONENTS_RPBUF_UART
	} else {
		printf("csi do not init");
	}
#endif
#ifdef CONFIG_VIN_USE_PM
	csi_pm_init();
#endif
#endif

#ifdef CONFIG_COMMAND_AUTO_START_MEMTESTER
	void *autotest_thread;
	autotest_thread = hal_thread_create(auto_memtester_thread, NULL,
			"auto_memtester", 8 * 1024, HAL_THREAD_PRIORITY_SYS);

	if (autotest_thread != NULL)
		hal_thread_start(autotest_thread);
#endif

	vTaskDelete(NULL);
}
