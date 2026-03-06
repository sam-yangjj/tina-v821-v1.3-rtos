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
#include <pm_wakelock.h>
#include <hal_gpio.h>
#ifdef CONFIG_DRIVERS_WUPIO
#include <sunxi_hal_wupio.h>
#endif
#include <hal_mutex.h>
#include "dev_wuio_manager.h"

//设置standy wuio 消抖时间
// debounce时钟源频率 = 32k / 2^init.clk_sel_x
//消抖时间计算 16*(7+1)=128ms,则高电平和低电平持续128ms后才可以唤醒
#define STANDY_clk_sel_0 9  //0-14   9的情况下是一个clk是16ms
#define STANDY_clk_sel_1 9
#define STANDY_deb_cyc_h 7  // 0-15 单位是 (1s / (32k / 2^clk_sel_x)) ms ===> clk_sel_x 为9时候，是16ms
#define STANDY_deb_cyc_l 7

//设置poweroff wuio 消抖时间
#define POWEROFF_clk_sel_0 14  //0-14   14的情况下是一个clk是512ms
#define POWEROFF_clk_sel_1 14
#define POWEROFF_deb_cyc_h 6
#define POWEROFF_deb_cyc_l 1

#define WUIO_MAX_COUNT 8
typedef struct wuio_manager_wuio {
	char use_flag;
	char sys_mode;
	wupio_mode_t wuio_mode;
	wupio_callback_t cb;
} wuio_manager_wuio_t;

typedef struct wuio_manager {
	int user_count;
	char first_sys_mode;
	hal_mutex_t wuio_lock;
	wuio_manager_wuio_t wuio[WUIO_MAX_COUNT];
}wuio_manager_t;

wuio_manager_t g_wuio_manager = {
    .user_count = 0,
};

static int wupio_wakeup_callback(wupio_pin_t pin)
{
	//printf("wupio_wakeup_callback \n");
	if (g_wuio_manager.wuio[pin].use_flag && g_wuio_manager.wuio[pin].cb)
		g_wuio_manager.wuio[pin].cb(pin);
	return 0;
}

static gpio_pin_t find_wuio_to_gpio(wupio_pin_t pin)
{

	switch (pin) {
	case WUPIO_PIN_PL0:
		return GPIO_PL0;
	case WUPIO_PIN_PL1:
		return GPIO_PL1;
	case WUPIO_PIN_PL2:
		return GPIO_PL2;
	case WUPIO_PIN_PL3:
		return GPIO_PL3;
	case WUPIO_PIN_PL4:
		return GPIO_PL4;
	case WUPIO_PIN_PL5:
		return GPIO_PL5;
	case WUPIO_PIN_PL6:
		return GPIO_PL6;
	case WUPIO_PIN_PL7:
		return GPIO_PL7;
	default:
		break;
	}
	return -1;
}

static int wuio_dev_init(int sys_mode)
{
	wupio_init_t init;
	//无人初始化wuio,需要初始化
	if (g_wuio_manager.user_count == 0) {
		/* init and set wupio clock period in 16ms (2^9*32.24us) */
		if (sys_mode == PM_MODE_STANDBY) {
			init.clk_sel_0 = STANDY_clk_sel_0;
			init.clk_sel_1 = STANDY_clk_sel_1;
		} else { //PM_MODE_HIBERNATION
			init.clk_sel_0 = POWEROFF_clk_sel_0;
			init.clk_sel_1 = POWEROFF_clk_sel_1;
		}
		init.callback = wupio_wakeup_callback; //唤醒回调。用来判断是否需要继续休眠
		hal_wupio_init(&init);
	}
	g_wuio_manager.user_count++;
	return 0;
}
//没有使用者，需要去初始化
static int wuio_dev_deinit(int sys_mode)
{
	g_wuio_manager.user_count--;
	//无人初始化wuio,需要初始化
	if (g_wuio_manager.user_count <= 0) {
		/* deinit wupio */
		hal_wupio_deinit();
		g_wuio_manager.user_count = 0;
	}
	return 0;
}


int hal_wuio_manager(wupio_pin_t wuio_num, int en, wupio_callback_t cb, int sys_mode, wupio_mode_t wuio_mode)
{
	wupio_config_t config;
	gpio_pin_t gpio_num = find_wuio_to_gpio(wuio_num);
	printf("wuio %d set %d", wuio_num, en);

	hal_mutex_lock(g_wuio_manager.wuio_lock);
	if (en) {

		/* disable io hold */
		hal_wupio_set_hold(wuio_num, 0);
		/* set gpio function */
		hal_gpio_set_direction(gpio_num, GPIO_DIRECTION_INPUT);
		hal_gpio_pinmux_set_function(gpio_num, GPIO_MUXSEL_IN);
		hal_gpio_set_pull(gpio_num, GPIO_PULL_DOWN_DISABLED);
		/* enable io hold */
		hal_wupio_set_hold(wuio_num, 1);

		if (g_wuio_manager.user_count == 0) {
			g_wuio_manager.first_sys_mode = sys_mode;
		} else if (g_wuio_manager.first_sys_mode != sys_mode) {
			printf("[warning] wuio io sys_mode is different \n");
		}

		g_wuio_manager.wuio[wuio_num].use_flag = 1;
		g_wuio_manager.wuio[wuio_num].sys_mode = sys_mode;
		g_wuio_manager.wuio[wuio_num].wuio_mode = wuio_mode;
		g_wuio_manager.wuio[wuio_num].cb = cb;
		wuio_dev_init(sys_mode);

		/* config wupio */
		config.mode = wuio_mode;
		config.deb_sel = WUPIO_DEBSEL_0; //设置消抖时间
		if (sys_mode == PM_MODE_STANDBY) {
			config.deb_cyc_h = STANDY_deb_cyc_h;
			config.deb_cyc_l = STANDY_deb_cyc_l;
		} else { //POWEROFF_MODE
			config.deb_cyc_h = POWEROFF_deb_cyc_h;
			config.deb_cyc_l = POWEROFF_deb_cyc_l;
		}
		hal_wupio_config(wuio_num, &config);

		/* enable io wakeup */
		hal_wupio_enable(wuio_num, 1);

	} else {
		g_wuio_manager.wuio[wuio_num].use_flag = 0;
		hal_wupio_enable(wuio_num, 0);
		wuio_dev_deinit(sys_mode);
		/* disable io hold */
		hal_wupio_set_hold(wuio_num, 0);

	}
	hal_mutex_unlock(g_wuio_manager.wuio_lock);

	return 0;
}
