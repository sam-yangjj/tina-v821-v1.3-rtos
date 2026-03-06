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
#include <hal_reset.h>
#include <hal_log.h>
#include <hal_prcm.h>
#include "sunxi_hal_common.h"
#include "platform_rtc_watchdog.h"
#include "hal_interrupt.h"
#include <irq_core.h>

static rtc_wdg_callback_t rtc_wdg_cb = NULL;

static hal_irqreturn_t hal_rtc_wdg_irq(void *dev_id)
{
	if (hal_readl(RTC_WDG_IRQ_STA_REG) & RTC_WDG_IRQ_STA_MASK) {
		clr_word(RTC_WDG_IRQ_STA_REG, RTC_WDG_IRQ_STA_MASK);
		if (rtc_wdg_cb) {
			rtc_wdg_cb();
		}
	}
	return HAL_IRQ_OK;
}

void hal_rtc_watchdog_init(void)
{
	HAL_PRCM_SetResetMod(PRCM_RESET_RTC_WATCHDOG, 1);
	hal_disable_irq(SUNXI_IRQ_RTC_WDG);
	hal_free_irq(SUNXI_IRQ_RTC_WDG);
	rtc_wdg_cb = NULL;
}

void hal_rtc_watchdog_deinit(void)
{
	hal_disable_irq(SUNXI_IRQ_RTC_WDG);
	hal_free_irq(SUNXI_IRQ_RTC_WDG);
	rtc_wdg_cb = NULL;
	HAL_PRCM_SetResetMod(PRCM_RESET_RTC_WATCHDOG, 0);
}

int hal_rtc_watchdog_register_callback(rtc_wdg_callback_t callback)
{
	if (callback == NULL) {
		return -1;
	}
	rtc_wdg_cb = callback;
	return 0;
}

void hal_rtc_watchdog_sel_clk(RTC_WDG_Clk_t clk)
{
	uint32_t val;

	val = hal_readl(RTC_WDG_CFG_REG);
	val &= ~(RTC_WDG_KEY_FIELD_16AA);
	val &= ~(RTC_WDG_CLK_SEL_VMASK << RTC_WDG_CLK_SEL_SHIFT);
	val |= (RTC_WDG_KEY_FIELD_16AA);
	val |= ((clk & RTC_WDG_CLK_SEL_VMASK) << RTC_WDG_CLK_SEL_SHIFT);
	hal_writel(val, RTC_WDG_CFG_REG);
}

void hal_rtc_watchdog_set_mode(RTC_WDG_Mode_t mode)
{
	uint32_t val;

	val = hal_readl(RTC_WDG_CFG_REG);
	val &= ~(RTC_WDG_KEY_FIELD_16AA);
	val &= ~(RTC_WDG_RST_MODE_VMASK << RTC_WDG_RST_MODE_SHIFT);
	val |= (RTC_WDG_KEY_FIELD_16AA);
	val |= ((mode & RTC_WDG_RST_MODE_VMASK) << RTC_WDG_RST_MODE_SHIFT);
	hal_writel(val, RTC_WDG_CFG_REG);
	if (mode == RTC_WDG_MODE_INTERRUPT) {
		clr_word(RTC_WDG_IRQ_STA_REG, RTC_WDG_IRQ_STA_MASK);
		set_word(RTC_WDG_IRQ_EN_REG, RTC_WDG_IRQ_EN_MASK);
		hal_request_irq(SUNXI_IRQ_RTC_WDG, hal_rtc_wdg_irq, "", NULL);
		hal_enable_irq(SUNXI_IRQ_RTC_WDG);
	} else {
		clr_word(RTC_WDG_IRQ_EN_REG, RTC_WDG_IRQ_EN_MASK);
		clr_word(RTC_WDG_IRQ_STA_REG, RTC_WDG_IRQ_STA_MASK);
		hal_disable_irq(SUNXI_IRQ_RTC_WDG);
		hal_free_irq(SUNXI_IRQ_RTC_WDG);
	}
}

void hal_rtc_watchdog_set_period(uint32_t period)
{
	uint32_t val;

	val = hal_readl(RTC_WDG_MODE_REG);
	val &= ~(RTC_WDG_KEY_FIELD_16AA);
	val &= ~(RTC_WDG_INTERVAL_VMASK << RTC_WDG_INTERVAL_SHIFT);
	val |= (RTC_WDG_KEY_FIELD_16AA);
	val |= ((period & RTC_WDG_INTERVAL_VMASK) << RTC_WDG_INTERVAL_SHIFT);
	hal_writel(val, RTC_WDG_MODE_REG);
}

void hal_rtc_watchdog_start(void)
{
	uint32_t val;

	val = hal_readl(RTC_WDG_MODE_REG) & (RTC_WDG_INTERVAL_VMASK << RTC_WDG_INTERVAL_SHIFT);
	val |= ((RTC_WDG_KEY_FIELD_16AA) | (1 << RTC_WDG_EN_SHIFT));
	hal_writel(val, RTC_WDG_MODE_REG);
}

void hal_rtc_watchdog_stop(void)
{
	uint32_t val;

	val = hal_readl(RTC_WDG_MODE_REG) & (RTC_WDG_INTERVAL_VMASK << RTC_WDG_INTERVAL_SHIFT);
	val |= RTC_WDG_KEY_FIELD_16AA;
	hal_writel(val, RTC_WDG_MODE_REG);
}

void hal_rtc_watchdog_feed(void)
{
	uint32_t val;

	val = RTC_WDG_KEY_FIELD_0A57 | (1 << RTC_WDG_RESTART_SHIFT);
	hal_writel(val, RTC_WDG_CTRL_REG);
}

void hal_rtc_watchdog_reboot(void)
{
	uint32_t val;

	hal_rtc_watchdog_deinit();
	HAL_PRCM_SetResetMod(PRCM_RESET_RTC_WATCHDOG, 1);
	val = RTC_WDG_KEY_FIELD_16AA | (1 << RTC_WDG_SRST_EN_SHIFT);
	hal_writel(val, RTC_WDG_SRST_REG);
	while(1);
}

