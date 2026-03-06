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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <hal_log.h>
#include <hal_time.h>
#include <hal_gpio.h>
#include <sunxi_hal_rtc.h>
#include <hal_interrupt.h>
#include <hal_cmd.h>
#include <pm_wakelock.h>

#include "dev_pir.h"
#include "dev_wuio_manager.h"

#ifdef CONFIG_DRIVERS_WUPIO
#include <sunxi_hal_wupio.h>
#endif

#include <pm_state.h>

#define WUPIO_PIN        (WUPIO_PIN_PL7)
#define WUPIO_MODE       WUPIO_MODE_POSITIVE_EDGE

/* PIR 置 1 的保持时长 */
#define PIR_HOLD_S          3

/* ---------------- 内部状态 ---------------- */
static volatile int  g_pir_status = 0;     /* 1=检测到人形，0=无 */
static char           g_inited = 0;
static char           g_wuio_standy_en = 0;
static char           g_wuio_poweroff_en = 0;
static unsigned int   g_wuio_sleep = 0;
struct rtc_time     g_set_rtc_day;
struct wakelock     g_pir_wakelock;

//pir唤醒中断回调
static int wupio_wakeup_callback(wupio_pin_t pin)
{
	printf("pir wakeup\n");

	struct rtc_time now_rtc_tm;
	unsigned int old_time_s;
	unsigned int now_time_s;

	pm_wakelocks_acquire(&g_pir_wakelock, PM_WL_TYPE_WAIT_ONCE, OS_WAIT_FOREVER); //拿住唤醒锁

	//pir频繁触发，所以需要
	if (g_wuio_sleep) {
		if (!hal_rtc_gettime(&now_rtc_tm)) {
			//判断pir休眠时间是否充足
			if (now_rtc_tm.tm_year > g_set_rtc_day.tm_year) {
				g_wuio_sleep = 0;
				goto pir_wakeup;
			} else if (now_rtc_tm.tm_mon > g_set_rtc_day.tm_mon) {
				g_wuio_sleep = 0;
				goto pir_wakeup;
			} else if (now_rtc_tm.tm_mday > g_set_rtc_day.tm_mday) {
				g_wuio_sleep = 0;
				goto pir_wakeup;
			} else {
				old_time_s = (g_set_rtc_day.tm_hour * 60 * 60) + (g_set_rtc_day.tm_min * 60) + g_set_rtc_day.tm_sec;
				now_time_s = (now_rtc_tm.tm_hour * 60 * 60) + (now_rtc_tm.tm_min * 60) + now_rtc_tm.tm_sec;
				if ((now_time_s - old_time_s) >= g_wuio_sleep) {
					g_wuio_sleep = 0;
					goto pir_wakeup;
				}
			}

			pm_wakelocks_release(&g_pir_wakelock); //继续休眠
			return 0;
		} else {
			printf("hal_rtc_gettime failed\n");
			pm_wakelocks_release(&g_pir_wakelock);
			return -1;
		}
	}

pir_wakeup:
	// 更新PIR状态
	g_pir_status = 1;
	// 记录触发时间用于状态保持
	if (hal_rtc_gettime(&g_set_rtc_day)) {
		printf("hal_rtc_gettime failed\n");
	}

	pm_wakesrc_relax(g_pir_wakelock.ws, PM_RELAX_WAKEUP); //该线程唤醒大核
	if (g_pir_wakelock.ref) { //如果有其他人需要继续休眠，则继续休眠
		pm_wakelocks_release(&g_pir_wakelock); //这里面会再调用一次pm_wakesrc_relax(ajc_wakelock.ws, PM_RELAX_SLEEPY);
	}

	return 0;
}

static void switch_wuio_gpio(int is_wuio, int mode)
{
	printf("pir:switch wuio %d \n", is_wuio);
	if (is_wuio) {
		hal_wuio_manager(WUPIO_PIN, 1, wupio_wakeup_callback, mode, WUPIO_MODE);
	} else {
		hal_wuio_manager(WUPIO_PIN, 0, wupio_wakeup_callback, mode, WUPIO_MODE);
	}
}

//通过suppend 和 resume 等级，来切换普通gpio和 wakeup io
static int hal_pir_resume(struct pm_device *dev, suspend_mode_t mode)
{
	if (mode == PM_MODE_STANDBY && g_wuio_standy_en) {
		switch_wuio_gpio(0, mode); //从wakeup io切换回普通GPIO
	} else if (g_wuio_poweroff_en) {
		switch_wuio_gpio(0, mode); //从wakeup io切换回普通GPIO
	}
	return 0;
}

static int hal_pir_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	if (mode == PM_MODE_STANDBY && g_wuio_standy_en) {
		switch_wuio_gpio(1, mode); //切换为wakeup io
	} else if (g_wuio_poweroff_en) {
		switch_wuio_gpio(1, mode); //切换为wakeup io
	}
	return 0;
}

//用这个动作去切换io 还未 wuio
static struct pm_devops pm_hal_pir_ops = {
    .suspend = hal_pir_suspend,
    .resume = hal_pir_resume,
};

static struct pm_device pm_hal_pir = {
    .name = "hal_pir",
    .ops = &pm_hal_pir_ops,
};

/* ---------------- 对外 API ---------------- */

/* 初始化：配置引脚、OUT 中断、置空闲电平与状态 */
int hal_pir_init(void)
{
	if (g_inited) return 0;

	//初始化rtc，用于获取时间。 每次都初始化，是担心休眠唤醒后相关资源没有初始化
	if (hal_rtc_init() != 0) {
		printf("hal_rtc_init failed\n");
		return -1;
	}

	//初始化唤醒锁
	memset(&g_pir_wakelock, 0, sizeof(struct wakelock));
	g_pir_wakelock.name = "pir_wakelock";
	g_pir_wakelock.type = PM_WL_TYPE_WAIT_ONCE;

	//注册休眠唤醒调用
#ifdef CONFIG_COMPONENTS_PM
	if (pm_devops_register(&pm_hal_pir) != 0) {
		printf("pm_devops_register failed\n");
		return -1;
	}
#endif

	g_pir_status = 0;
	g_wuio_poweroff_en = 0; //默认不开关机Pir唤醒
	g_wuio_standy_en = 1; //默认开启standy pir 唤醒
	g_wuio_sleep = 0;
	memset(&g_set_rtc_day, 0, sizeof(struct rtc_time));

	g_inited = 1;
	return 0;
}

/* 反初始化：关中断/释放 */
int hal_pir_deinit(void)
{
	if (!g_inited) return 0;

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&pm_hal_pir);
#endif
	g_pir_status = 0;
	g_inited = 0;
	return 0;
}

int hal_pir_power_en(int en)
{
	if (en)
		g_wuio_poweroff_en = 1;
	else
		g_wuio_poweroff_en = 0;
	return 0;
}

int hal_pir_standy_set_wakeup(int en, unsigned int suppend_s) //enable是开关， suppend_s 屏蔽响应一段时间,从接口调用开始计时
{
	struct rtc_time rtc_tm;

	if (en)
		g_wuio_standy_en = 1;
	else
		g_wuio_standy_en = 0;

	if (suppend_s) {
		if (!hal_rtc_gettime(&rtc_tm)) {
			g_set_rtc_day = rtc_tm; //记录设置的时间
			g_wuio_sleep = suppend_s;
		} else {
			printf("hal_rtc_gettime failed\n");
			return -1;
		}
	}
	return 0;
}

int hal_pir_poweroff_wakeup(int en)
{
	if (en)
		g_wuio_poweroff_en = 1;
	else
		g_wuio_poweroff_en = 0;
	return 0;
}

/* 读取当前 PIR 状态（中断置 1，普通 timer 到期后自动清 0） */
int hal_pir_get_status(int *status)
{
	struct rtc_time now_rtc_tm;
	unsigned int old_time_s;
	unsigned int now_time_s;
	if (!status) return -1;

	if (!hal_rtc_gettime(&now_rtc_tm)) {
		//判断pir休眠时间是否充足
		if (now_rtc_tm.tm_year > g_set_rtc_day.tm_year) {
			*status = 0;
		} else if (now_rtc_tm.tm_mon > g_set_rtc_day.tm_mon) {
			*status = 0;
		} else if (now_rtc_tm.tm_mday > g_set_rtc_day.tm_mday) {
			*status = 0;
		} else {
			old_time_s = (g_set_rtc_day.tm_hour * 60 * 60) + (g_set_rtc_day.tm_min * 60) + g_set_rtc_day.tm_sec;
			now_time_s = (now_rtc_tm.tm_hour * 60 * 60) + (now_rtc_tm.tm_min * 60) + now_rtc_tm.tm_sec;
			if ((now_time_s - old_time_s) > PIR_HOLD_S) {
				*status = 0;
			} else {
				*status = 1;
			}
		}
	} else {
		printf("hal_rtc_gettime failed\n");
		return -1;
	}
	return 0;
}

#if 0
int cmd_test_hal_pir(int argc, char **argv)
{
	hal_pir_standy_set_wakeup(1, 60);

	return  0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_hal_pir, hal_pir_test, hal pir tests)
#endif
