/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <hal_cmd.h>
#include <sunxi_hal_wuptimer.h>
#include <sunxi_hal_rtc.h>
#include <hal_gpio.h>
#include <hal_interrupt.h>
#include <hal_ccu.h>
#include <hal_log.h>
#include <hal_pwrctrl.h>
#include <pm_debug.h>
#include <pm_suspend.h>

#define CPUS_WUPIO_IRQ      (MAKE_IRQn(186U, 0))
#define CPUS_WUPTIMER_IRQ   (MAKE_IRQn(181U, 0))
#define CPUS_ALARM0_IRQ     (MAKE_IRQn(182U, 0))
#define CPUS_ALARM1_IRQ     (MAKE_IRQn(183U, 0))
#define CPUS_WLAN_IRQ       (MAKE_IRQn(175U, 0))

#define MS_TO_32K(ms)               (ms * 100000 /3125)
static uint32_t wuptimer_cnt = 0;
static uint32_t wuptimer_times = 0;
static pm_wakesrc_t *wuptimer_ws = NULL;
static int cmd_wuptimer_callback(void)
{
	hal_log_info("wuptimer enter irq\n");
	pm_wakesrc_stay_awake(wuptimer_ws);
	hal_wuptimer_enable(0);
	pm_wakesrc_relax(wuptimer_ws, PM_RELAX_WAKEUP);
	return 0;
}

int cmd_rtos_test_wuptimer(int argc, char **argv)
{
	uint32_t interval;
	uint32_t mode;
	uint32_t sub_mode;

	hal_log_info("cmd test wuptimer\n");

	if (argc != 5) {
		hal_log_info("param error\n");
		return -1;
	}

	mode = strtol(argv[1], NULL, 0);
	sub_mode = strtol(argv[2], NULL, 0);
	interval = strtol(argv[3], NULL, 0);
	wuptimer_times = strtol(argv[4], NULL, 0);
	wuptimer_cnt = 0;

	if (wuptimer_ws == NULL) {
		wuptimer_ws = pm_wakesrc_register(CPUS_WUPTIMER_IRQ, "test_wuptimer");
		if (wuptimer_ws == NULL) {
			return -1;
		}
	}

	HAL_CCU_ResetCPUX(0);
	HAL_CCU_EnableCPUXClk(0);
	HAL_CCU_EnableCPUXMtClk(0);
	HAL_PWRCTRL_EnableCPUXPowerDomain(0);
	pm_log("cpux reset and power down\n");

	hal_wuptimer_init();
	hal_wuptimer_register_callback(cmd_wuptimer_callback);
	hal_wuptimer_set_interval(MS_TO_32K(interval));
	hal_wuptimer_enable(1);
	pm_log("init wuptimer\n");
	/* standby mode is 2, hibernation mode is 3 */
	if (mode == 2) {
		pm_set_standby_mode(sub_mode);
	}
	pm_suspend_request(PM_MODE_STANDBY);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rtos_test_wuptimer, rtos_test_wuptimer, wakeup_test_tools)

#if 0
#define TEST_WAKEUP_IO           (GPIO_PL7)
#define TEST_WAKEUP_IO_MASK      (PRCM_WAKE_IO_7)
#define TEST_WAKEUP_IO_WAKEEVT   (PM_WAKEEVT_WAKEUP_IO_7)
#define SUXNI_IRQ_WUPIO          (CPUS_WUPIO_IRQ)
static pm_wakesrc_t *wupio_ws = NULL;
static hal_irqreturn_t hal_wupio_irq(void *dev_id)
{
	uint32_t index;

	if (HAL_PRCM_WakeupIOGetEventDetected(TEST_WAKEUP_IO_MASK)) {
		HAL_PRCM_WakeupIOClearEventDetected(TEST_WAKEUP_IO_MASK);
		hal_log_info("wupio enter irq\n");
		pm_wakesrc_relax(wupio_ws, PM_RELAX_WAKEUP);
		index = HAL_PRCM_WakeupIOxMask2Index(TEST_WAKEUP_IO_MASK);
		HAL_PRCM_EnableWakeupIOx(index, 0);
		HAL_PRCM_WakeupIODisableGlobal();
		hal_disable_irq(SUXNI_IRQ_WUPIO);
		hal_free_irq(SUXNI_IRQ_WUPIO);
	}
	HAL_PRCM_WakeupIOClearEventStatus();
	return HAL_IRQ_OK;
}

int cmd_rtos_test_wupio(int argc, char **argv)
{
	uint32_t index;
	uint32_t val;
	uint32_t mode;
	uint32_t sub_mode;

	hal_log_info("cmd test wuptimer\n");

	if (argc != 3) {
		hal_log_info("param error\n");
		return -1;
	}

	mode = strtol(argv[1], NULL, 0);
	sub_mode = strtol(argv[2], NULL, 0);

	if (wupio_ws == NULL) {
		wupio_ws = pm_wakesrc_register(CPUS_WUPIO_IRQ, "test_wupio");
		if (wupio_ws == NULL) {
			return -1;
		}
	}

	HAL_CCU_ResetCPUX(0);
	HAL_CCU_EnableCPUXClk(0);
	HAL_CCU_EnableCPUXMtClk(0);
	HAL_PWRCTRL_EnableCPUXPowerDomain(0);
	pm_log("cpux reset and power down\n");

	/* set pwrctrl wakeup mask */
	val = hal_readl(0x4A011020);
	val |= (1 << 2);
	hal_writel(val, 0x4A011020);
	while (!(hal_readl(0x4A011020) & (1 << 2)));

	/* enable irq */
	hal_request_irq(SUXNI_IRQ_WUPIO, hal_wupio_irq, "", NULL);
	hal_enable_irq(SUXNI_IRQ_WUPIO);

	/* pinmux set input */
	hal_gpio_pinmux_set_function(TEST_WAKEUP_IO, GPIO_MUXSEL_IN);
	hal_gpio_set_pull(TEST_WAKEUP_IO, GPIO_PULL_UP);
	hal_gpio_set_direction(TEST_WAKEUP_IO, GPIO_DIRECTION_INPUT);

	/* config wup io */
	index = HAL_PRCM_WakeupIOxMask2Index(TEST_WAKEUP_IO_MASK);
	HAL_PRCM_SetWakeupDebClk0(0);
	HAL_PRCM_SetWakeupIOxDebSrc(index, PRCM_WAKEUP_IOX_DEB_CLK0_SRC);
	HAL_PRCM_SetWakeupIOxDebounce(PRCM_WAKEUP_IO_DEB_CYCLES_L, index, 0);
	HAL_PRCM_WakeupIOEnableCfgHold(TEST_WAKEUP_IO_MASK);
	HAL_PRCM_WakeupIOSetFallingEvent(TEST_WAKEUP_IO_MASK);
	HAL_PRCM_WakeupIOClearEventDetected(TEST_WAKEUP_IO_MASK);
	HAL_PRCM_WakeupIOEnableGlobal();
	HAL_PRCM_EnableWakeupIOx(index, 1);
	pm_log("inti wupio\n");
	/* standby mode is 2, hibernation mode is 3 */
	if (mode == 2) {
		pm_set_standby_mode(sub_mode);
	}
	pm_suspend_request(PM_MODE_STANDBY);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rtos_test_wupio, rtos_test_wupio, wakeup_test_tools)
#endif

#ifdef CONFIG_DRIVERS_RTC
static pm_wakesrc_t *rtc_alarm_ws = NULL;

static int rtc_alarm_callback(void)
{
	hal_log_info("rtc alarm interrupt\n");
	pm_wakesrc_relax(rtc_alarm_ws, PM_RELAX_WAKEUP);
	return 0;
}

static int cmd_rtos_test_alarm(int argc, char **argv)
{
	unsigned int enable = 1;
	unsigned int sec = 0;
	struct rtc_time rtc_tm;
	struct rtc_wkalrm wkalrm;

	if (argc != 2) {
		hal_log_info("param error\n");
		return -1;
	}

	sec = atoi(argv[1]);
	if (sec > 60) {
		hal_log_info("please take less than 60 seconds\n");
	}
	hal_log_info("rtc alarm sec: %d\n", sec);

	if (rtc_alarm_ws == NULL) {
		rtc_alarm_ws = pm_wakesrc_register(CPUS_ALARM0_IRQ, "rtc_alarm_ws");
		if (rtc_alarm_ws  == NULL) {
			hal_log_info("rtc_alarm_ws create fail\n");
		}
	}

	hal_rtc_init();
	hal_rtc_register_callback(rtc_alarm_callback);

	if (hal_rtc_gettime(&rtc_tm))
		hal_log_info("sunxi rtc gettime error\n");

	wkalrm.enabled = 1;
	wkalrm.time = rtc_tm;
	wkalrm.time.tm_sec = 0;
	wkalrm.time.tm_sec += sec;
	rtc_tm.tm_sec = 0;

	hal_log_info("set rtc time %04d-%02d-%02d %02d:%02d:%02d\n",
			rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	if (hal_rtc_settime(&rtc_tm))
		hal_log_info("sunxi rtc settime error\n");

	hal_log_info("set alarm time %04d-%02d-%02d %02d:%02d:%02d\n",
			wkalrm.time.tm_year + 1900, wkalrm.time.tm_mon + 1, wkalrm.time.tm_mday,
			wkalrm.time.tm_hour, wkalrm.time.tm_min, wkalrm.time.tm_sec);

	if (hal_rtc_setalarm(&wkalrm))
		hal_log_info("sunxi rtc setalarm error\n");

	hal_rtc_alarm_irq_enable(enable);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rtos_test_alarm, rtos_test_alarm, wakeup_test_tools)
#endif

