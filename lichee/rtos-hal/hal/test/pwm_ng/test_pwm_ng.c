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
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_mem.h>
#include <sunxi_hal_pwm_ng.h>
#include <hal_time.h>

#if defined(CONFIG_ARCH_DSP)
#include <delay.h>
#define sleep(sec) msleep(sec * 1000)
#endif

#ifndef printfFromISR
#define printfFromISR printf
#endif

static uint8_t global_port;
static uint8_t global_channel;
static uint8_t global_timeout_s;

hal_pwm_status_t pwm_capture_deinit(uint8_t port, uint32_t channel);

static void pwm_pulse_callback(void* data)
{
	printfFromISR("pulse mode callback, data is %s\n", (char *)data);
}

static int cmd_test_pwm(int argc, char **argv)
{
	struct pwm_config *config;
	uint8_t port;
	uint8_t channel;
	int period, duty, pulse_num;
	bool polarity, output_mode;
	char *data = "success";

	if (argc < 8) {
		hal_log_info("Usage: port | pwm channel | duty | period | polarity | pulse_num | output_mode\n");
		return -1;
	}

	port = strtol(argv[1], NULL, 0);
	channel = strtol(argv[2], NULL, 0);
	duty = strtoul(argv[3], NULL, 0);
	period = strtoul(argv[4], NULL, 0);
	polarity = strtoul(argv[5], NULL, 0);
	pulse_num = strtoul(argv[6], NULL, 0);
	output_mode = strtoul(argv[7], NULL, 0);

	if (pulse_num)
		hal_log_info("Rum pwm hal layer test case in pulse mode!\n");
	else
		hal_log_info("Run pwm hal layer test case in cycle mode!\n");

	config = (struct pwm_config *)hal_malloc(sizeof(struct pwm_config));
	memset(config, 0, sizeof(struct pwm_config));

	config->duty_ns = duty;
	config->period_ns = period;
	config->polarity = polarity;
	config->pulse_num = pulse_num;
	config->output_mode = output_mode;

	hal_log_info("duty_ns = %d \n", config->duty_ns);
	hal_log_info("period_ns = %d \n", config->period_ns);
	hal_log_info("polarity = %d \n", config->polarity);
	hal_log_info("pulse_num = %d \n", config->pulse_num);
	hal_log_info("output_mode = %d \n", config->output_mode);

	hal_pwm_init(port);

	if (pulse_num)
		hal_pwm_pulse_irq_enable(port, channel, pwm_pulse_callback, (void *)data);

	hal_pwm_control(port, channel, config);

	hal_free(config);

	hal_log_info("control pwm test finish\n");

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_pwm, hal_pwm, pwm hal APIs tests)

static int cmd_release_pwm_channel(int argc, char **argv)
{
	struct pwm_config *config = NULL;
	uint8_t port;
	uint8_t channel;

	if (argc < 3) {
		hal_log_info("Usage: pwm port\n");
		return -1;
	}

	hal_log_info("Run close pwm channel test case\n");


	config = (struct pwm_config *)hal_malloc(sizeof(struct pwm_config));
	memset(config, 0, sizeof(struct pwm_config));

	port = strtol(argv[1], NULL, 0);
	channel = strtol(argv[2], NULL, 0);
	hal_log_info("channel = %d", channel);

	hal_pwm_release(port, channel, config);
	hal_free(config);

	hal_log_info("release pwm channel finish\n");

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_release_pwm_channel, hal_pwm_close, release pwm channel hal APIs tests)

static void pwm_cap_callback(void* param)
{
	hal_pwm_cap_info *info = (hal_pwm_cap_info *)param;

	printfFromISR("pwm%d capture callback, cnt is %d, period is %d, duty is %d\n", info->channel, info->cnt, info->period, info->duty);

	/* disable */
	pwm_capture_deinit(global_port, global_channel);
}

hal_pwm_status_t pwm_capture_init(uint8_t port, uint32_t channel)
{

	hal_pwm_cap_enable(port, channel, pwm_cap_callback);

	return HAL_PWM_STATUS_OK;
}

hal_pwm_status_t pwm_capture_count_init(uint8_t port, uint32_t channel)
{

	hal_pwm_cap_count_enable(port, channel);

	return HAL_PWM_STATUS_OK;
}

hal_pwm_status_t pwm_capture_deinit(uint8_t port, uint32_t channel)
{
	hal_pwm_cap_disable(port, channel);

	return HAL_PWM_STATUS_OK;
}

void cmd_pwm_capture_help(void)
{
	printf("pwm capture test, capture once to get duty and period\n");
	printf("usage: pwm_capture_test<channel>\n");
	printf("\t<channel>: 0 ~ 15\n");
	printf("eg: pwm_capture_test 0, pwm0 capture function\n");
}

int cmd_pwm_capture(int argc, char *argv[])
{
	if(argc != 3) {
		cmd_pwm_capture_help();
		return -1;
	}
	global_port = strtol(argv[1], NULL, 0);
	global_channel = strtol(argv[2], NULL, 0);
	if (global_channel > 15) {
		cmd_pwm_capture_help();
		return -1;
	}

	/* capture setting */
	pwm_capture_init(global_port, global_channel);

	printf("[%s]: pwm stop capture ssd\n", __func__);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_pwm_capture, pwm_capture_test, pwm capture test)

void cmd_pwm_capture_count_help(void)
{
	printf("pwm capture count test, capture once to get pulse count\n");
	printf("usage: pwm_capture_count_test <port> <channel> <timeout_s>\n");
	printf("\t<channel>: 0 ~ 9\n");
	printf("eg: pwm_capture_count_test 1 0 1000, pwm1-0 capture count function\n");
}

int cmd_pwm_capture_count(int argc, char *argv[])
{
#if defined(CONFIG_ARCH_SUN300IW1)
	printf("SUN300IW1 does not support this test.\n");
	return -1;
#endif
	unsigned int pulse_num = 0;

	if (argc != 4) {
		cmd_pwm_capture_count_help();
		return -1;
	}
	global_port = strtol(argv[1], NULL, 0);
	global_channel = strtol(argv[2], NULL, 0);
	global_timeout_s = strtol(argv[3], NULL, 0);
	if (global_channel > 9) {
		cmd_pwm_capture_count_help();
		return -1;
	}

	/* start capture count */
	hal_pwm_cap_count_enable(global_port, global_channel);

	hal_sleep(global_timeout_s);

	pulse_num = hal_pwm_cap_get_pulse_num(global_port, global_channel);

	hal_pwm_cap_count_disable(global_port, global_channel);

	printf("[%s]: pulse_num is: %d\n", __func__, pulse_num);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_pwm_capture_count, pwm_capture_count_test, pwm capture count test)
