#ifndef __TUYA_RTC_WDT_H__
#define __TUYA_RTC_WDT_H__

#define rtc_pr_info(fmt, ...) printf("[tuya_rtc]:%s[%d]--" fmt, __func__, __LINE__, ##__VA_ARGS__)

//初始化和反初始化函数
int tuya_rtc_wdt_init();
int tuya_rtc_wdt_deinit();
//是否开启休眠看门狗（默认开启），如果开启，则休眠不按时唤醒会触发看门狗重启
int tuya_rtc_wdt_standy_enable(int en);
//看门狗操作函数
int tuya_rtc_wdt_start_timeout(unsigned int time_out_s);
int tuya_rtc_wdt_stop();
int tuya_rtc_wdt_feed();
int tuya_rtc_wdt_reboot();

#endif
