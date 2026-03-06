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

#include <stdlib.h>
#include <sunxi_hal_prcm.h>
#include <hal_clk.h>
#include <hal_interrupt.h>
#include <sunxi_hal_wuptimer.h>

static wuptimer_callback_t wuptimer_cb = NULL;

static hal_irqreturn_t hal_wuptimer_irq(void *dev_id)
{
	uint32_t val;

	if (hal_readl(WUP_TMR_VAL_REG) & WUP_TMR_IRQ_PEND_MASK) {
		val = hal_readl(WUP_TMR_VAL_REG);
		val |= WUP_TMR_IRQ_PEND_MASK;
		hal_writel(val, WUP_TMR_VAL_REG);
		while ((hal_readl(WUP_TMR_VAL_REG) & WUP_TMR_IRQ_PEND_MASK));
	}

	if (wuptimer_cb) {
		wuptimer_cb();
	}
	return HAL_IRQ_OK;
}

void hal_wuptimer_init(void)
{
	HAL_PRCM_SetResetMod(PRCM_RESET_RTC_TIMER, 1);
	hal_request_irq(SUXNI_IRQ_WUPTIMER, hal_wuptimer_irq, "", NULL);
	hal_enable_irq(SUXNI_IRQ_WUPTIMER);
	wuptimer_cb = NULL;
}

void hal_wuptimer_deinit(void)
{
	wuptimer_cb = NULL;
	hal_disable_irq(SUXNI_IRQ_WUPTIMER);
	hal_free_irq(SUXNI_IRQ_WUPTIMER);
	HAL_PRCM_SetResetMod(PRCM_RESET_RTC_TIMER, 0);
}

int hal_wuptimer_register_callback(wuptimer_callback_t callback)
{
	if (callback == NULL) {
		return -1;
	}
	wuptimer_cb = callback;
	return 0;
}

void hal_wuptimer_set_interval(uint32_t intv)
{
	uint32_t val;

	val = hal_readl(WUP_TMR_VAL_REG);
	val &= ~WUP_TMR_INTV_VALUE_MASK;
	val |= (intv & WUP_TMR_INTV_VALUE_MASK);
	hal_writel(val, WUP_TMR_VAL_REG);
	while ((hal_readl(WUP_TMR_VAL_REG) & WUP_TMR_INTV_VALUE_MASK) != (intv & WUP_TMR_INTV_VALUE_MASK));
}

void hal_wuptimer_enable(uint32_t en)
{
	uint32_t val;

	if (en) {
		val = hal_readl(WUP_TMR_CTRL_REG);
		val |= WUP_TMR_EN_MASK;
		hal_writel(val, WUP_TMR_CTRL_REG);
		while (!(hal_readl(WUP_TMR_CTRL_REG) & WUP_TMR_EN_MASK));
	} else {
		val = hal_readl(WUP_TMR_CTRL_REG);
		val &= ~WUP_TMR_EN_MASK;
		hal_writel(val, WUP_TMR_CTRL_REG);
		while (hal_readl(WUP_TMR_CTRL_REG) & WUP_TMR_EN_MASK);
	}
}

