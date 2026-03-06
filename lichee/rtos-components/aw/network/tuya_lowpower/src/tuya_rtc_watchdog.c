#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pm_wakelock.h>
#include <sunxi_hal_rtc_watchdog.h>
#include <pm_devops.h>
#include <hal_time.h>
#include <hal_cmd.h>
#include <tuya_rtc_watchdog.h>

#ifdef CONFIG_TUYA_RTC_WATCH_DOG

#if !CONFIG_DRIVERS_PRCM || !CONFIG_DRIVERS_RTC_WATCHDOG
#error "Tuya rtc watchdog need close CPUX rtc watchdog and open CONFIG_DRIVERS_PRCM & CONFIG_DRIVERS_RTC_WATCHDOG in CPUS!!"
#endif

//rtc看门狗计数位数为10bit. 每一个bit代表0.5s， 所以最大延时为512s
#define WDT_STANDY_TIMEOUT 500 // 一般设置最大超时时间，单位为秒

typedef struct tuya_wdt_ctl {
	char init_flag;
	char standy_en;  //默认休眠唤醒是否开看门狗
	char wdt_start;
	unsigned int timeout_s;
} tuya_wdt_ctl_t;

static tuya_wdt_ctl_t g_tuya_wdt_ctl;

#ifdef CONFIG_COMPONENTS_PM
//在这个函数判断是休眠还是关机；
static int tuya_wdt_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	rtc_pr_info("Suspend\n");
	//如果狗已经被手动停止，则休眠唤醒什么动作都不做；
	if (!g_tuya_wdt_ctl.wdt_start) {
		rtc_pr_info("tuya_wdt_ctl.wdt_start not set\n");
		return 0;
	}

	hal_rtc_watchdog_feed();//喂狗一次

	return 0;
}

static int tuya_wdt_resume(struct pm_device *dev, suspend_mode_t mode)
{
	rtc_pr_info("Resume\n");
	//如果狗已经被手动停止，则休眠唤醒什么动作都不做；
	if (!g_tuya_wdt_ctl.wdt_start) {
		rtc_pr_info("tuya_wdt_ctl.wdt_start not set\n");
		return 0;
	}

	hal_rtc_watchdog_feed();    //喂狗一次

	if (g_tuya_wdt_ctl.standy_en) {
		rtc_pr_info("resume wdt to timeout :%d \n", g_tuya_wdt_ctl.timeout_s);
		//针对正常启动重新设置超时时间
		hal_rtc_watchdog_stop();
		hal_rtc_watchdog_set_period(g_tuya_wdt_ctl.timeout_s * 2);
		hal_rtc_watchdog_start();
		hal_rtc_watchdog_feed();
		rtc_pr_info("tuya_wdt_resume Start Dog");

	}
	//启动后，就等待应用正常喂狗了
	return 0;
}

static int tuya_wdt_suspend_noirq(struct pm_device *dev, suspend_mode_t mode)
{
	rtc_pr_info("Suspend Noirq\n");
	//如果狗已经被手动停止，则休眠唤醒什么动作都不做；
	if (!g_tuya_wdt_ctl.wdt_start) {
		rtc_pr_info("tuya_wdt_ctl.wdt_start not set\n");
		return 0;
	}

	//喂狗一次
	hal_rtc_watchdog_feed();

	if (mode == PM_MODE_STANDBY) {
		if (g_tuya_wdt_ctl.standy_en) {
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

static int tuya_wdt_resume_noirq(struct pm_device *dev, suspend_mode_t mode)
{
	rtc_pr_info("Resume Noirq\n");
	//如果狗已经被手动停止，则休眠唤醒什么动作都不做；
	if (!g_tuya_wdt_ctl.wdt_start) {
		rtc_pr_info("tuya_wdt_ctl.wdt_start not set\n");
		return 0;
	}

	if (g_tuya_wdt_ctl.standy_en) {
		hal_rtc_watchdog_feed(); //针对休眠开了狗，则唤醒需要喂一次
	} else {
		hal_rtc_watchdog_start(); //针对休眠不开狗，唤醒的时候需要重新打开;
	}
	return 0;
}

static struct pm_devops pm_tuya_wdt_ops = {
    .suspend = tuya_wdt_suspend,
    .resume = tuya_wdt_resume,
    .suspend_noirq = tuya_wdt_suspend_noirq,
    .resume_noirq = tuya_wdt_resume_noirq,
};

static struct pm_device pm_tuya_wdt = {
    .name = "tuya_wdt",
    .ops = &pm_tuya_wdt_ops,
};
#endif

int tuya_rtc_wdt_init()
{
	if (g_tuya_wdt_ctl.init_flag) {
		rtc_pr_info("tuya_rtc_wdt already init!\n");
		return -1;
	}
	rtc_pr_info("Init Dog\n");
	hal_rtc_watchdog_init();
	hal_rtc_watchdog_sel_clk(RTC_WDG_CLK_32000);
	hal_rtc_watchdog_set_mode(RTC_WDG_MODE_RST_SYS);
#ifdef CONFIG_COMPONENTS_PM
	pm_devops_register(&pm_tuya_wdt);
#endif
	g_tuya_wdt_ctl.init_flag = 1;

	g_tuya_wdt_ctl.standy_en = 1; //默认休眠唤醒开看门狗
	return 0;
}

int tuya_rtc_wdt_deinit()
{
	if (!g_tuya_wdt_ctl.init_flag) {
		rtc_pr_info("tuya_wdt_ctl.init_flag not set\n");
		return -1;
	}
	rtc_pr_info("Deinit Dog\n");
	hal_rtc_watchdog_stop();
	hal_rtc_watchdog_deinit();
	pm_devops_unregister(&pm_tuya_wdt);
	g_tuya_wdt_ctl.wdt_start = 0;
	g_tuya_wdt_ctl.init_flag = 0;
	return 0;
}

int tuya_rtc_wdt_standy_enable(int en)
{
	if (!g_tuya_wdt_ctl.init_flag) {
		rtc_pr_info("tuya_wdt_ctl.init_flag not set\n");
		return -1;
	}

	g_tuya_wdt_ctl.standy_en = en;
	return 0;
}

int tuya_rtc_wdt_start_timeout(unsigned int time_out_s)
{
	if (!g_tuya_wdt_ctl.init_flag) {
		rtc_pr_info("tuya_wdt_ctl.init_flag not set\n");
		return -1;
	}

	if (time_out_s > 512) {
		rtc_pr_info("wdt timeout must < 512 s\n");
		return -1;
	}
	rtc_pr_info("Start Dog with timeout: %d s\n", time_out_s);

	g_tuya_wdt_ctl.timeout_s = time_out_s;
	hal_rtc_watchdog_set_period(time_out_s * 2);
	hal_rtc_watchdog_start();
	g_tuya_wdt_ctl.wdt_start = 1;
	return 0;
}

int tuya_rtc_wdt_stop()
{
	if (!g_tuya_wdt_ctl.init_flag) {
		rtc_pr_info("tuya_rtc_wdt_stop failed\n");
		return -1;
	}
	rtc_pr_info("Stop Dog\n");
	hal_rtc_watchdog_stop();
	g_tuya_wdt_ctl.wdt_start = 0;
	return 0;
}

int tuya_rtc_wdt_feed()
{
	if (!g_tuya_wdt_ctl.init_flag && g_tuya_wdt_ctl.wdt_start) {
		rtc_pr_info("tuya_rtc_wdt_feed failed\n");
		return -1;
	}

	hal_rtc_watchdog_feed();
	rtc_pr_info("Feed Dog\n");
	return 0;
}
int tuya_rtc_wdt_reboot()
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
		printf("tuya_wdt_ctl:\n" \
			"init_flag = %d\n" \
			"standy_en = %d\n" \
			"wdt_start = %d\n" \
			"timeout_s = %u\n",
			g_tuya_wdt_ctl.init_flag,
			g_tuya_wdt_ctl.standy_en,
			g_tuya_wdt_ctl.wdt_start,
			g_tuya_wdt_ctl.timeout_s);
	} else if (argc == 2) {
		timeout = strtoul(argv[1], NULL, 0);
		rtc_pr_info("set timeout %d,but not feed\n", timeout);
		if (!g_tuya_wdt_ctl.init_flag)
			tuya_rtc_wdt_init();
		tuya_rtc_wdt_start_timeout(timeout);
	} else {
		timeout = 10;
		rtc_pr_info("set timeout %d,and feed\n", timeout);
		tuya_rtc_wdt_init();
		tuya_rtc_wdt_start_timeout(timeout);

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
#endif
