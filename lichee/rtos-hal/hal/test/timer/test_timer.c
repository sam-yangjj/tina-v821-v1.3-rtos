/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the people's Republic of China and other countries.
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_thread.h>
#include <hal_timer.h>
#include <hal_gpio.h>
#ifdef CONFIG_DRIVERS_TIMER
#include <sunxi_hal_timer.h>
#endif

#ifdef CONFIG_DRIVERS_HTIMER
#include <sunxi_hal_htimer.h>
#endif

#ifdef CONFIG_DRIVERS_TIMER
static void hal_timer_irq_callback(void *param)
{
	hal_log_err("timer interrupt!!\n");
}

int cmd_test_hal_timer(int argc, char **argv)
{
	int timer = SUNXI_TMR0;
	int delay_ms = 5 * 1000;

	if (argc >= 2) {
		if (strcmp("-h", argv[1]) == 0) {
			printf("usage: hal_timer <timer> <delay_ms>\n");
			return 0;
		}
	}

	if (atoi(argv[1]) < SUNXI_TMR_NUM)
		timer = SUNXI_TMR0 + atoi(argv[1]);

	if (argc >= 3)
		delay_ms = atoi(argv[2]);

	printf("test timer%d, wait %fs for interrupt\n", timer, delay_ms/1000.0);
	hal_timer_init(timer);
	hal_timer_set_oneshot(timer, delay_ms * 1000, hal_timer_irq_callback, NULL);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_hal_timer, hal_timer, timer hal APIs tests)

int cmd_test_hal_timer_periodic_mode(int argc, char **argv)
{
	int timer = SUNXI_TMR0;
	int delay_ms = 5 * 1000;
	int test_time_s = 30;
	if (argc >= 2) {
		if (strcmp("-h", argv[1]) == 0) {
			printf("usage: hal_timer_periodic_mode  <timer> <delay_ms> <test_time_s>\n");
			return 0;
		}
	}

	if (atoi(argv[1]) < SUNXI_TMR_NUM)
		timer = SUNXI_TMR0 + atoi(argv[1]);

	if (argc >= 3)
		delay_ms = atoi(argv[2]);
	if (argc >= 4)
		test_time_s = atoi(argv[3]);
	printf("test timer%d, wait %fs for interrupt\n", timer, delay_ms/1000.0);
	hal_timer_init(timer); /* enable timer and apply irq */
	hal_timer_set_periodic(timer, delay_ms * 1000, hal_timer_irq_callback, NULL);
	sleep(test_time_s);
	printf("after %ds,close the timer%d\n", test_time_s,timer);
	hal_timer_stop(timer); /* disable timer */
	hal_timer_uninit(timer); /* free the irq */
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_hal_timer_periodic_mode, hal_timer_periodic_mode, timer hal APIs tests)
#endif

#ifdef CONFIG_DRIVERS_WAKEUP_TIMER
static void hal_wuptimer_irq_callback(void *param)
{
	hal_log_info("wuptimer interrupt!!\n");
}

int cmd_test_hal_wuptimer(int argc, char **argv)
{
	int timer = SUNXI_TMR0;
	int delay_ms = 5 * 1000;
	if (argc >= 2) {
		if (strcmp("-h", argv[1]) == 0) {
			printf("usage: hal_wuptimer <timer> <delay_ms>\n");
			return 0;
		}
	}

	if (atoi(argv[1]) < SUNXI_TMR_NUM)
		timer = SUNXI_TMR0 + atoi(argv[1]);

	if (argc >= 3)
		delay_ms = atoi(argv[2]);

	printf("test wuptimer%d, wait %fs for interrupt\n", timer, delay_ms/1000.0);
	hal_wuptimer_init(timer);
	hal_wuptimer_set_oneshot(timer, delay_ms * 1000, hal_wuptimer_irq_callback, NULL);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_hal_wuptimer, hal_wuptimer, wup timer hal APIs tests)
#endif

#ifdef CONFIG_DRIVERS_HTIMER
#define HTIMER_IDLE	(0)
#define HTIMER_STOP	(1)
#define HTIMER_START	(2)
static int timer_irq_flag = HTIMER_IDLE;
static int test_htimer = HAL_HRTIMER0;
static int test_time_s = 30;

static void hal_htimer_irq_callback(void *param)
{
	timer_irq_flag = HTIMER_STOP;
}

void hal_htimer_thread(void *param)
{
	while(1) {
		if (timer_irq_flag == HTIMER_STOP) {
			/* clsoe htimer */
			hal_htimer_deinit(test_htimer);
			hal_htimer_base_deinit(test_htimer);
			printf("htimer interrupt!!\n");
			break;
		}
		hal_msleep(1);
	}
	timer_irq_flag = HTIMER_IDLE;
	hal_thread_stop(NULL);
}

void hal_htimer_periodic_thread(void *param)
{
	int ms_cnt = 0;
	while(1) {
		if (timer_irq_flag == HTIMER_STOP) {
			printf("htimer interrupt!!\n");
			timer_irq_flag = HTIMER_IDLE;
		}
		hal_msleep(1);
		if (++ms_cnt > test_time_s * 1000) {
			/* clsoe htimer */
			hal_htimer_deinit(test_htimer);
			hal_htimer_base_deinit(test_htimer);
			break;
		}
	}
	timer_irq_flag = HTIMER_IDLE;
	hal_thread_stop(NULL);
}

int cmd_test_hal_htimer(int argc, char **argv)
{
	int ret;
	int delay_ms = 5 * 1000;
	void *thread;

	if (argc >= 2) {
		if (strcmp("-h", argv[1]) == 0) {
			printf("usage: hal_htimer <timer> <delay_ms>\n");
			return 0;
		}
	}

	if (atoi(argv[1]) < HTIMER_MAX_NUM)
		test_htimer = HAL_HRTIMER0 + atoi(argv[1]);
	else {
		printf("use htimer err, max htimer %d\n", (HTIMER_MAX_NUM - 1));
		return 0;
	}

	if (argc >= 3)
		delay_ms = atoi(argv[2]);

	if (timer_irq_flag != HTIMER_IDLE) {
		printf("htimer busy, timer_irq_flag = 0x%x\n", timer_irq_flag);
		return 0;
	}

	timer_irq_flag = HTIMER_START;
	printf("test htimer%d, wait %fs for interrupt\n", test_htimer, delay_ms/1000.0);

	hal_htimer_base_init(test_htimer);

	hal_htimer_init(test_htimer);

	ret = hal_htimer_set_oneshot(test_htimer, delay_ms * 1000, hal_htimer_irq_callback, NULL);

	if (ret)
		printf("hal_htimer_set_oneshot fail!!\n");
	else
		printf("hal_htimer_set_oneshot OK!!\n");

	thread = hal_thread_create(hal_htimer_thread, NULL, \
					"hal htimer thread", 512, 29);
	if (thread != NULL)
		hal_thread_start(thread);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_hal_htimer, hal_htimer, htimer hal APIs tests)

int cmd_test_hal_htimer_periodic_mode(int argc, char **argv)
{
	int delay_ms = 5 * 1000;
	void *thread;

	test_time_s = 30;

	if (argc >= 2) {
		if (strcmp("-h", argv[1]) == 0) {
			printf("usage: hal_htimer_periodic_mode  <timer> <delay_ms> <test_time_s>\n");
			return 0;
		}
	}

	if (atoi(argv[1]) < HTIMER_MAX_NUM)
		test_htimer = HAL_HRTIMER0 + atoi(argv[1]);
	else {
		printf("use htimer err, max htimer %d\n", (HTIMER_MAX_NUM - 1));
		return 0;
	}

	if (argc >= 3)
		delay_ms = atoi(argv[2]);

	if (argc >= 4)
		test_time_s = atoi(argv[3]);

	printf("test time = %d s\n", test_time_s);
	printf("test htimer%d, wait %fs for interrupt\n", test_htimer, delay_ms/1000.0);
	hal_htimer_base_init(test_htimer);
	hal_htimer_init(test_htimer);
	hal_htimer_set_periodic(test_htimer, delay_ms * 1000, hal_htimer_irq_callback, NULL);

	thread = hal_thread_create(hal_htimer_periodic_thread, NULL, \
					"hal htimer periodic thread", 512, 29);
	if (thread != NULL)
		hal_thread_start(thread);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_hal_htimer_periodic_mode, hal_htimer_periodic_mode, timer hal APIs tests)

static int str2pin(char *str, gpio_pin_t *pin)
{
	int num;

	if (str[0] != 'P')
		return -1;

	if (str[1] < 'A' || str[1] > 'O')
		return -1;

	num = atoi(&str[2]);
	if (num < 0 || num > PINS_PER_BANK)
		return -1;

	*pin = (str[1] - 'A') * PINS_PER_BANK + num;

	return 0;
}

static void hal_htimer_gpio_callback(void *param)
{
	gpio_pin_t pin = *(gpio_pin_t *)param;

	hal_gpio_set_data(pin, 0);

	printf("pin%d output low level.\n", pin);
}

int cmd_test_hal_htimer_by_gpio(int argc, char **argv)
{
	int delay_us = 5 * 1000;
	gpio_pin_t pin = 0;
	int ret = 0;

	if ((strcmp("-h", argv[1]) == 0) || (argc < 4)) {
		printf("usage: hal_htimer_gpio <timer> <delay_us> <gpio>\n");
		return 0;
	}

	if (atoi(argv[1]) < HTIMER_MAX_NUM)
		test_htimer = HAL_HRTIMER0 + atoi(argv[1]);
	else {
		printf("use htimer err, max htimer %d\n", (HTIMER_MAX_NUM - 1));
		return 0;
	}

	delay_us = atoi(argv[2]);
	str2pin(argv[3], &pin);

	printf("test htimer by pin:%d[%s]\n", pin, argv[3]);

	hal_gpio_set_pull(pin, GPIO_PULL_UP);
	hal_gpio_set_direction(pin, GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_data(pin, 0);

	printf("test htimer%d by gpio, gpio output high level.\n", test_htimer);
	printf("wait %dus for interrupt, gpio output low level.\n", delay_us);

	hal_htimer_base_init(test_htimer);

	hal_htimer_init(test_htimer);

	hal_gpio_set_data(pin, 1);

	ret = hal_htimer_set_oneshot(test_htimer, delay_us, hal_htimer_gpio_callback, &pin);

	if (ret < 0) {
		printf("timer set oneshot failed\n");
		hal_htimer_deinit(test_htimer);
		hal_htimer_base_deinit(test_htimer);
	}

	hal_usleep(delay_us * 2);

	hal_htimer_deinit(test_htimer);
	hal_htimer_base_deinit(test_htimer);
	printf("test finish... close htimer:%d\n", test_htimer);

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_hal_htimer_by_gpio, hal_htimer_gpio, htimer hal APIs tests)

#endif
