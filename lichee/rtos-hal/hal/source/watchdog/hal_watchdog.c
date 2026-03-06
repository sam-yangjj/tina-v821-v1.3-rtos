/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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
//#include <hal_osal.h>
#include <hal_time.h>
//#include <barrier.h>
#include <hal_reset.h>
#include <hal_log.h>
#include "sunxi_hal_common.h"
#include "platform_watchdog.h"
#if defined(CONFIG_ENABLE_WDG_IRQ)
#include "hal_interrupt.h"
#include <irq_core.h>

extern int set_finished_dummy(void);
#endif

#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#endif

#ifdef CONFIG_COMPONENTS_PM
static unsigned int timeout_save = 0; /* used for suspend/resume to save watchdog timeout data */
#endif

extern void udelay(unsigned int us);

#define writel_wdt	hal_writel
#define readl_wdt	hal_readl

static unsigned long running;
static unsigned long running_saved;

#if defined(WDT_TIMEOUT_MAP_V2)
static const int wdt_timeout_map[] =
{
    [1]  = 0x5,  /* 1s  32000cycles*/
    [2]  = 0x6,  /* 2s  64000*/
    [3]  = 0x7,  /* 3s  96000*/
    [4]  = 0x8,  /* 4s  128000*/
    [5]  = 0x9,  /* 5s  160000*/
    [6]  = 0xa,  /* 6s  192000*/
    [8]  = 0xb,  /* 8s  256000*/
    [10] = 0xc,  /* 10s 320000*/
    [12] = 0xd,  /* 12s 384000*/
    [14] = 0xe,  /* 14s 448000*/
    [16] = 0xf,  /* 16s 512000*/
};
#else
static const int wdt_timeout_map[] =
{
    [1]  = 0x1,  /* 1s  32000cycles*/
    [2]  = 0x2,  /* 2s  64000*/
    [3]  = 0x3,  /* 3s  96000*/
    [4]  = 0x4,  /* 4s  128000*/
    [5]  = 0x5,  /* 5s  160000*/
    [6]  = 0x6,  /* 6s  192000*/
    [8]  = 0x7,  /* 8s  256000*/
    [10] = 0x8,  /* 10s 320000*/
    [12] = 0x9,  /* 12s 384000*/
    [14] = 0xA,  /* 14s 448000*/
    [16] = 0xB,  /* 16s 512000*/
};
#endif

int hal_watchdog_is_running(void)
{
	return running_saved;
}

int hal_watchdog_suspend(int timeout)
{
	running_saved = running;

	pr_debug("%s()\n", __func__);
	if (hal_watchdog_is_running()) {
		pr_debug("%s()\n", __func__);
		hal_watchdog_stop(timeout);
	}

	return 0;
}

int hal_watchdog_resume(int timeout)
{
	pr_debug("%s()\n", __func__);
	if (hal_watchdog_is_running()) {
		pr_debug("%s()\n", __func__);
		hal_watchdog_start(timeout);
	}

	return 0;
}
void hal_watchdog_info(void)
{
    struct hal_sunxi_wdt *wdt = (struct hal_sunxi_wdt *)WDT_BASE;
    printf("Base:0x%x mode: 0x%x, cfg=0x%x, ctl=0x%x\n",
	(WDT_BASE), (unsigned int)(wdt->mode), (unsigned int)(wdt->cfg), (unsigned int)(wdt->ctl));
}

void hal_watchdog_disable(void)
{
    struct hal_sunxi_wdt *wdt = (struct hal_sunxi_wdt *)WDT_BASE;
    unsigned int wtmode;

    pr_debug("%s()\n", __func__);
    wtmode = readl_wdt(&wdt->mode);
    wtmode &= ~WDT_MODE_EN;
    wtmode |= KEY_FIELD_MAGIC;

    writel_wdt(wtmode, &wdt->mode);
    isb();
    running = 0;
}

void hal_watchdog_clean(void)
{
    struct hal_sunxi_wdt *wdt = (struct hal_sunxi_wdt *)WDT_BASE;

    pr_debug("%s()\n", __func__);
    writel_wdt(KEY_FIELD_MAGIC, &wdt->mode);
    isb();
    writel_wdt(0, &wdt->irq_en);
    writel_wdt(1, &wdt->sta);
    writel_wdt(KEY_FIELD_MAGIC, &wdt->cfg);
    running = 0;
}

void hal_watchdog_reset(int timeout)
{
    int timeout_set = timeout;
    struct hal_sunxi_wdt *wdt = (struct hal_sunxi_wdt *)WDT_BASE;
    unsigned int wtmode;

    pr_debug("%s()\n", __func__);
    hal_watchdog_disable();

#if defined(CONFIG_ENABLE_WDG_IRQ)
    writel_wdt(0x01, &wdt->irq_en);
#endif

    if (timeout > 16)
    {
        timeout_set = 16;
    }

    if (wdt_timeout_map[timeout_set] == 0)
    {
        timeout_set++;
    }

    wtmode = KEY_FIELD_MAGIC | (wdt_timeout_map[timeout_set] << WDT_TIMEOUT_OFFSET) | WDT_MODE_EN;

#if defined(CONFIG_ENABLE_WDG_IRQ)
    writel_wdt(KEY_FIELD_MAGIC | WDT_CFG_IRQ, &wdt->cfg);
#else
    writel_wdt(KEY_FIELD_MAGIC | WDT_CFG_RESET, &wdt->cfg);
#endif

    writel_wdt(wtmode, &wdt->mode);

    isb();
    writel_wdt(WDT_CTRL_KEY | WDT_CTRL_RESTART, &wdt->ctl);
    running = 1;
}

int hal_watchdog_is_enable(void)
{
    struct hal_sunxi_wdt *wdt = (struct hal_sunxi_wdt *)WDT_BASE;
    unsigned int wtmode;

    pr_debug("%s()\n", __func__);
    wtmode = readl_wdt(&wdt->mode) & WDT_MODE_EN;

    return wtmode ? 1 : 0;
}

void hal_watchdog_restart(void)
{
    struct hal_sunxi_wdt *wdt = (struct hal_sunxi_wdt *)WDT_BASE;

    pr_debug("%s()\n", __func__);
    hal_watchdog_disable();

    /* add delay for watchdog disable */
    udelay(500);
    /* Set the watchdog for its shortest interval (.5s) and wait */
    writel_wdt(KEY_FIELD_MAGIC | WDT_CFG_RESET, &wdt->cfg);
    isb();
    writel_wdt(KEY_FIELD_MAGIC | WDT_MODE_EN, &wdt->mode);

    while (1) {};
}

#ifdef CONFIG_COMPONENTS_PM
static int hal_watchdog_pm_resume(struct pm_device *dev, suspend_mode_t mode)
{
#ifndef CONFIG_COMPONENTS_AMP_HW_WATCHDOG
	/* when enable CONFIG_COMPONENTS_AMP_HW_WATCHDOG, the AMP WDT
	* will enabled in Linux, rproc only do reset
	*/
	hal_watchdog_resume(timeout_save);
#endif
	return 0;
}

static int hal_watchdog_pm_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	unsigned int wtmode;
	struct hal_sunxi_wdt *wdt = (struct hal_sunxi_wdt *)WDT_BASE;

	wtmode = readl_wdt(&wdt->mode);
	timeout_save = (wtmode >> WDT_TIMEOUT_OFFSET) & WDT_TIMEOUT_MASK;

	hal_watchdog_suspend(timeout_save);

	return 0;
}

struct pm_devops pm_watchdog_ops = {
	.suspend = hal_watchdog_pm_suspend,
	.resume = hal_watchdog_pm_resume,
};

struct pm_device pm_watchdog = {
	.name = "sunxi_pm_watchdog",
	.ops = &pm_watchdog_ops,
};
#endif

#if defined(CONFIG_ENABLE_WDG_IRQ)
static hal_irqreturn_t irq_watchdog_handler(void *p)
{
    struct hal_sunxi_wdt * wdt = (struct hal_sunxi_wdt *)WDT_BASE;

    //disable wdg irq
    writel_wdt(0x00, &wdt->irq_en);

    //clear irq pending bit
    writel_wdt(0x01, &wdt->sta);

    hal_interrupt_disable();
    irq_core_disable_root_ic_irq();
    set_finished_dummy();
    __asm__ volatile("wfi":::"memory", "cc");

    return HAL_IRQ_OK;
}
#endif

void hal_watchdog_init(void)
{
#if defined(CONFIG_ARCH_SUN20IW2)
    struct reset_control *reset;
    hal_reset_type_t reset_type = HAL_SUNXI_RESET;
    u32 reset_id;
    reset_id = WDT_CCMU_CLK;

    reset = hal_reset_control_get(reset_type, reset_id);
    hal_reset_control_assert(reset);
    hal_reset_control_deassert(reset);
    /* pr_debug("watchdog init ok\n"); */
#else
    pr_debug("%s()\n", __func__);
#endif

    hal_watchdog_clean();
#ifdef CONFIG_COMPONENTS_PM
    pm_devops_register(&pm_watchdog);
#endif /* end of CONFIG_COMPONENTS_PM */

#if defined (CONFIG_ENABLE_WDG_IRQ)
    irq_core_request_system_irq(IRQ_WDG, (void *)irq_watchdog_handler, NULL);
#endif

    running = 0;
    running_saved = 0;
}

void hal_watchdog_stop(int timeout)
{
    pr_debug("%s()\n", __func__);
    hal_watchdog_disable();
}

void hal_watchdog_start(int timeout)
{
    pr_debug("%s()\n", __func__);
    hal_watchdog_reset(timeout);
}

void hal_watchdog_feed(void)
{
    struct hal_sunxi_wdt *wdt = (struct hal_sunxi_wdt *)WDT_BASE;
    pr_debug("%s()\n", __func__);
    writel_wdt(WDT_CTRL_KEY | WDT_CTRL_RESTART, &wdt->ctl);
}
