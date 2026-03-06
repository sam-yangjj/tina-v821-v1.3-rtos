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
#include <hal_gpio.h>

#include "dev_powerkey_wakeup.h"
#include "dev_wuio_manager.h"

#ifdef CONFIG_DRIVERS_WUPIO
#include <sunxi_hal_wupio.h>
#endif

#define WUPIO_PIN    (WUPIO_PIN_PL7) /* PL7 */
#define WUPIO_MODE    WUPIO_MODE_POSITIVE_EDGE

typedef struct {
	int init_flag;
	char standy_wakeup_en;
	char poweroff_wakeup_en;
	struct wakelock wakelock;

} powerkey_wuio_t;

powerkey_wuio_t g_powerkey_wuio;

static int wupio_wakeup_callback(wupio_pin_t pin)
{
	printf("powerkey wakeup\n");
	//powerkey 按下一律都拉起大核
	pm_wakelocks_acquire(&g_powerkey_wuio.wakelock, PM_WL_TYPE_WAIT_ONCE, OS_WAIT_FOREVER); //拿住唤醒锁
	pm_wakesrc_relax(g_powerkey_wuio.wakelock.ws, PM_RELAX_WAKEUP); //该线程唤醒大核
	return 0;
}

//通过suppend 和 resume 等级，来切换普通gpio和 wakeup io
static int hal_powerkey_resume(struct pm_device *dev, suspend_mode_t mode)
{
	if (g_powerkey_wuio.poweroff_wakeup_en) { //启动不会走这里了；
		hal_wuio_manager(WUPIO_PIN, 0, wupio_wakeup_callback, mode, WUPIO_MODE);
	}
	return 0;
}

static int hal_powerkey_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	if (g_powerkey_wuio.poweroff_wakeup_en) {
		hal_wuio_manager(WUPIO_PIN, 1, wupio_wakeup_callback, mode, WUPIO_MODE);
	}
	return 0;
}

//用这个动作去切换wuio
static struct pm_devops pm_hal_powerkey_ops = {
    .suspend = hal_powerkey_suspend,
    .resume = hal_powerkey_resume,
};

static struct pm_device pm_hal_powerkey = {
    .name = "hal_powerkey",
    .ops = &pm_hal_powerkey_ops,
};

int hal_powerkey_wakeup_init(void)
{
	if (g_powerkey_wuio.init_flag) {
		return 0;
	}

	hal_wuio_manager(WUPIO_PIN, 0, wupio_wakeup_callback, 0, WUPIO_MODE);
	g_powerkey_wuio.standy_wakeup_en = 1;
	g_powerkey_wuio.poweroff_wakeup_en = 1;
	//注册休眠唤醒调用。
	pm_devops_register(&pm_hal_powerkey);

	g_powerkey_wuio.init_flag = 1;

	return 0;
}
int hal_powerkey_wakeup_deinit(void) //关闭大核才操作
{
	if (!g_powerkey_wuio.init_flag)
		return 0;
	pm_devops_unregister(&pm_hal_powerkey);

	g_powerkey_wuio.init_flag = 1;
	return 0;
}
int hal_powerkey_wakeup_poweroff_wakeup(int en) //不需要实现，因为powerkey_wakeup关机情况不触发。
{
	//保存变量。用于休眠唤醒的时候设置
	if (en)
		g_powerkey_wuio.standy_wakeup_en = 1;
	else
		g_powerkey_wuio.standy_wakeup_en = 0;
	return 0;
}
int hal_powerkey_wakeup_standy_set_wakeup(int en)
{
	//保存变量。用于休眠唤醒的时候设置
	if (en)
		g_powerkey_wuio.poweroff_wakeup_en = 1;
	else
		g_powerkey_wuio.poweroff_wakeup_en = 0;
	return 0;
}

#if 0
int cmd_test_hal_pk(int argc, char **argv)
{

	hal_powerkey_wakeup_poweroff_wakeup(1);
	return  0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_hal_pk, hal_pk_test, hal pk tests)
#endif
