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
#include <pm_wakelock.h>
#include <sunxi_hal_rtc_watchdog.h>
#include <pm_devops.h>
#include <hal_time.h>
#include <hal_cmd.h>
#include <lp_rtc_watchdog.h>

#if !CONFIG_DRIVERS_PRCM || !CONFIG_DRIVERS_RTC_WATCHDOG
#error "RTC Watchdog need close CPUX rtc watchdog and open CONFIG_DRIVERS_PRCM & CONFIG_DRIVERS_RTC_WATCHDOG in CPUS!!"
/*
close CPUX rtc watchdog in bsp dir!

diff --git a/drivers/watchdog/sunxi_wdt.c b/drivers/watchdog/sunxi_wdt.c
index e643fd04b..4e23e8e25 100644
--- a/drivers/watchdog/sunxi_wdt.c
+++ b/drivers/watchdog/sunxi_wdt.c
@@ -479,15 +479,15 @@ static int sunxi_wdt_probe(struct platform_device *pdev)
        sunxi_wdt->wdt_dev.min_timeout = WDT_MIN_TIMEOUT;
        sunxi_wdt->wdt_dev.parent = dev;

-       watchdog_init_timeout(&sunxi_wdt->wdt_dev, timeout, dev);
+       // watchdog_init_timeout(&sunxi_wdt->wdt_dev, timeout, dev);
        watchdog_set_nowayout(&sunxi_wdt->wdt_dev, nowayout);
        watchdog_set_restart_priority(&sunxi_wdt->wdt_dev, 128);

        watchdog_set_drvdata(&sunxi_wdt->wdt_dev, sunxi_wdt);

-       sunxi_wdt_stop(&sunxi_wdt->wdt_dev);
+       // sunxi_wdt_stop(&sunxi_wdt->wdt_dev);

-       watchdog_stop_on_reboot(&sunxi_wdt->wdt_dev);
+       // watchdog_stop_on_reboot(&sunxi_wdt->wdt_dev);
        err = devm_watchdog_register_device(dev, &sunxi_wdt->wdt_dev);
        if (unlikely(err))
                return err;
@@ -512,7 +512,7 @@ static void sunxi_wdt_shutdown(struct platform_device *pdev)

 static struct platform_driver sunxi_wdt_driver = {
        .probe          = sunxi_wdt_probe,
-       .shutdown       = sunxi_wdt_shutdown,
+       // .shutdown       = sunxi_wdt_shutdown,
        .driver         = {
                .name           = DRV_NAME,
                .of_match_table = sunxi_wdt_dt_ids,
*/
#endif

//rtc看门狗计数位数为10bit. 每一个bit代表0.5s， 所以最大延时为512s
#define WDT_STANDY_TIMEOUT 500 // 一般设置最大超时时间，单位为秒

typedef struct lp_wdt_ctl {
	char init_flag;
	char standy_en;  //默认休眠唤醒是否开看门狗
	char wdt_start;
	unsigned int timeout_s;
} lp_wdt_ctl_t;

static lp_wdt_ctl_t g_lp_wdt_ctl;

#ifdef CONFIG_COMPONENTS_PM
//在这个函数判断是休眠还是关机；
static int lp_wdt_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	rtc_pr_info("Suspend\n");
	//如果狗已经被手动停止，则休眠唤醒什么动作都不做；
	if (!g_lp_wdt_ctl.wdt_start) {
		rtc_pr_info("lp_wdt_ctl.wdt_start not set\n");
		return 0;
	}

	hal_rtc_watchdog_feed();//喂狗一次

	return 0;
}

static int lp_wdt_resume(struct pm_device *dev, suspend_mode_t mode)
{
	rtc_pr_info("Resume\n");
	//如果狗已经被手动停止，则休眠唤醒什么动作都不做；
	if (!g_lp_wdt_ctl.wdt_start) {
		rtc_pr_info("lp_wdt_ctl.wdt_start not set\n");
		return 0;
	}

	hal_rtc_watchdog_feed();    //喂狗一次

	if (g_lp_wdt_ctl.standy_en) {
		rtc_pr_info("resume wdt to timeout :%d \n", g_lp_wdt_ctl.timeout_s);
		//针对正常启动重新设置超时时间
		hal_rtc_watchdog_stop();
		hal_rtc_watchdog_set_period(g_lp_wdt_ctl.timeout_s * 2);
		hal_rtc_watchdog_start();
		hal_rtc_watchdog_feed();
		rtc_pr_info("lp_wdt_resume Start Dog");

	}
	//启动后，就等待应用正常喂狗了
	return 0;
}

static int lp_wdt_suspend_noirq(struct pm_device *dev, suspend_mode_t mode)
{
	rtc_pr_info("Suspend Noirq\n");
	//如果狗已经被手动停止，则休眠唤醒什么动作都不做；
	if (!g_lp_wdt_ctl.wdt_start) {
		rtc_pr_info("lp_wdt_ctl.wdt_start not set\n");
		return 0;
	}

	//喂狗一次
	hal_rtc_watchdog_feed();

	if (mode == PM_MODE_STANDBY) {
		if (g_lp_wdt_ctl.standy_en) {
			//针对休眠重新设置超时时间
			rtc_pr_info("Suspend set wdt timeout %ds \n", WDT_STANDY_TIMEOUT);
			hal_rtc_watchdog_stop();
			hal_rtc_watchdog_set_period(WDT_STANDY_TIMEOUT * 2);
			hal_rtc_watchdog_start();
			hal_rtc_watchdog_feed();
		} else {
			hal_rtc_watchdog_stop(); //休眠不开狗，则需要把狗关掉
		}
	} else { //关机情况
		rtc_pr_info("stop wdt \n");
		hal_rtc_watchdog_stop();
		hal_rtc_watchdog_deinit();
	}

	return 0;
}

static int lp_wdt_resume_noirq(struct pm_device *dev, suspend_mode_t mode)
{
	rtc_pr_info("Resume Noirq\n");
	//如果狗已经被手动停止，则休眠唤醒什么动作都不做；
	if (!g_lp_wdt_ctl.wdt_start) {
		rtc_pr_info("lp_wdt_ctl.wdt_start not set\n");
		return 0;
	}

	if (g_lp_wdt_ctl.standy_en) {
		hal_rtc_watchdog_feed(); //针对休眠开了狗，则唤醒需要喂一次
	} else {
		hal_rtc_watchdog_start(); //针对休眠不开狗，唤醒的时候需要重新打开;
	}
	return 0;
}

static struct pm_devops pm_lp_wdt_ops = {
    .suspend = lp_wdt_suspend,
    .resume = lp_wdt_resume,
    .suspend_noirq = lp_wdt_suspend_noirq,
    .resume_noirq = lp_wdt_resume_noirq,
};

static struct pm_device pm_lp_wdt = {
    .name = "lp_wdt",
    .ops = &pm_lp_wdt_ops,
};
#endif

int lp_rtc_wdt_init()
{
	if (g_lp_wdt_ctl.init_flag) {
		rtc_pr_info("lp_rtc_wdt already init!\n");
		return -1;
	}
	rtc_pr_info("Init Dog\n");
	hal_rtc_watchdog_init();
	hal_rtc_watchdog_sel_clk(RTC_WDG_CLK_32000);
	hal_rtc_watchdog_set_mode(RTC_WDG_MODE_RST_SYS);
#ifdef CONFIG_COMPONENTS_PM
	pm_devops_register(&pm_lp_wdt);
#endif
	g_lp_wdt_ctl.init_flag = 1;

	g_lp_wdt_ctl.standy_en = 1; //默认休眠唤醒开看门狗
	return 0;
}

int lp_rtc_wdt_deinit()
{
	if (!g_lp_wdt_ctl.init_flag) {
		rtc_pr_info("lp_wdt_ctl.init_flag not set\n");
		return -1;
	}
	rtc_pr_info("Deinit Dog\n");
	hal_rtc_watchdog_stop();
	hal_rtc_watchdog_deinit();
	pm_devops_unregister(&pm_lp_wdt);
	g_lp_wdt_ctl.wdt_start = 0;
	g_lp_wdt_ctl.init_flag = 0;
	return 0;
}

int lp_rtc_wdt_standy_enable(int en)
{
	if (!g_lp_wdt_ctl.init_flag) {
		rtc_pr_info("lp_wdt_ctl.init_flag not set\n");
		return -1;
	}

	g_lp_wdt_ctl.standy_en = en;
	return 0;
}

int lp_rtc_wdt_start_timeout(unsigned int time_out_s)
{
	if (!g_lp_wdt_ctl.init_flag) {
		rtc_pr_info("lp_wdt_ctl.init_flag not set\n");
		return -1;
	}

	if (time_out_s > 512) {
		rtc_pr_info("wdt timeout must < 512 s\n");
		return -1;
	}
	rtc_pr_info("Start Dog with timeout: %d s\n", time_out_s);

	g_lp_wdt_ctl.timeout_s = time_out_s;
	hal_rtc_watchdog_set_period(time_out_s * 2);
	hal_rtc_watchdog_start();
	g_lp_wdt_ctl.wdt_start = 1;
	return 0;
}

int lp_rtc_wdt_stop()
{
	if (!g_lp_wdt_ctl.init_flag) {
		rtc_pr_info("lp_rtc_wdt_stop failed\n");
		return -1;
	}
	rtc_pr_info("Stop Dog\n");
	hal_rtc_watchdog_stop();
	g_lp_wdt_ctl.wdt_start = 0;
	return 0;
}

int lp_rtc_wdt_feed()
{
	if (!g_lp_wdt_ctl.init_flag && g_lp_wdt_ctl.wdt_start) {
		rtc_pr_info("lp_rtc_wdt_feed failed\n");
		return -1;
	}

	hal_rtc_watchdog_feed();
	rtc_pr_info("Feed Dog\n");
	return 0;
}
int lp_rtc_wdt_reboot()
{
	hal_rtc_watchdog_reboot();
	return 0;
}


#if 0
int cmd_test_hal_rtc_watchdog(int argc, char **argv)
{
	int timeout;
	int to;

	if (argc == 2 && strcmp(argv[1], "show") == 0) {
		printf("lp_wdt_ctl:\n" \
			"init_flag = %d\n" \
			"standy_en = %d\n" \
			"wdt_start = %d\n" \
			"timeout_s = %u\n",
			g_lp_wdt_ctl.init_flag,
			g_lp_wdt_ctl.standy_en,
			g_lp_wdt_ctl.wdt_start,
			g_lp_wdt_ctl.timeout_s);
	} else if (argc == 2) {
		timeout = strtoul(argv[1], NULL, 0);
		rtc_pr_info("set timeout %d,but not feed\n", timeout);
		if (!g_lp_wdt_ctl.init_flag)
			lp_rtc_wdt_init();
		lp_rtc_wdt_start_timeout(timeout);
	} else {
		timeout = 10;
		rtc_pr_info("set timeout %d,and feed\n", timeout);
		lp_rtc_wdt_init();
		lp_rtc_wdt_start_timeout(timeout);

		to = 20;
		while (to--) {
			rtc_pr_info("feed wdt \n");
			hal_rtc_watchdog_feed();
			hal_msleep(1 * 1000);
		}
	}
	rtc_pr_info("end feed wdt \n");
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_hal_rtc_watchdog, hal_rtc_watchdog, rtc_watchdog_test_tools);
#endif
