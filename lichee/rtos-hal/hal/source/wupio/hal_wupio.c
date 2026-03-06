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
#include <hal_time.h>
#include <hal_interrupt.h>
#include <sunxi_hal_wupio.h>

static wupio_callback_t wupio_cb = NULL;

static hal_irqreturn_t hal_wupio_irq(void *dev_id)
{
	wupio_pin_t pin;
	uint32_t mask;

	for (pin = WUPIO_PIN_PL0; pin < WUPIO_PIN_MAX && pin < WUP_IO_NUN; pin++) {
		mask = 1 << pin;
		if (hal_readl(WUP_IO_STA_REG) & mask) {
			if (wupio_cb) {
				wupio_cb(pin);
			}
			hal_writel(mask, WUP_IO_STA_REG);
			/* must delay 3 cycles of 32k clock */
			hal_udelay(125);
		}
	}
	return HAL_IRQ_OK;
}

void hal_wupio_init(wupio_init_t *init)
{
	uint32_t val;

	/* clear pending */
	val = WUP_IO_MASK;
	hal_writel(val, WUP_IO_STA_REG);

	/* set debounce clock */
	val = hal_readl(WUP_IO_DEB_CLK_REG);
	val &= ~(WUP_IO_DEB_CLK_SET0_MASK | WUP_IO_DEB_CLK_SET1_MASK);
	val = (init->clk_sel_0 << WUP_IO_DEB_CLK_SET0_SHIFT) |
		  (init->clk_sel_1 << WUP_IO_DEB_CLK_SET1_SHIFT);
	hal_writel(val, WUP_IO_DEB_CLK_REG);

	/* set wupio irq callback */
	wupio_cb = init->callback;

	/* enbale gen interrupt */
	val = hal_readl(WUP_IO_WUP_GEN_REG);
	val |= WUP_IO_GEN_MASK;
	hal_writel(val, WUP_IO_WUP_GEN_REG);

	hal_request_irq(SUXNI_IRQ_WUPIO, hal_wupio_irq, "", NULL);
	hal_enable_irq(SUXNI_IRQ_WUPIO);
}

void hal_wupio_deinit(void)
{
	uint32_t val;

	wupio_cb = NULL;
	hal_disable_irq(SUXNI_IRQ_WUPIO);
	hal_free_irq(SUXNI_IRQ_WUPIO);

	/* disable gen interrupt */
	val = hal_readl(WUP_IO_WUP_GEN_REG);
	val &= ~WUP_IO_GEN_MASK;
	hal_writel(val, WUP_IO_WUP_GEN_REG);

	/* clear pending */
	val = WUP_IO_MASK;
	hal_writel(val, WUP_IO_STA_REG);
}

void hal_wupio_config(wupio_pin_t pin, wupio_config_t *config)
{
	uint32_t val;

	/* set mode */
	val = hal_readl(WUP_IO_EN_REG);
	val &= ~(1 << (pin + WUP_IO_MODE_SHIFT));
	val |= (config->mode << (pin + WUP_IO_MODE_SHIFT));
	hal_writel(val, WUP_IO_EN_REG);

	/* select debounce clock */
	val = hal_readl(WUP_IO_DEB_CLK_REG);
	val &= ~(1 << (pin + WUP_IO_DEB_CLK_SEL_SHIFT));
	val |= (config->deb_sel << (pin + WUP_IO_DEB_CLK_SEL_SHIFT));
	hal_writel(val, WUP_IO_DEB_CLK_REG);

	/* set debounce cycle for low level*/
	val = hal_readl(WUP_IO_WUP_CYC0_REG);
	val &= ~(WUP_IO_DEB_CYC_VMASK << (pin * WUP_IO_DEB_CYC_OFFSET + WUP_IO_DEB_CYC_SHIFT));
	val |= ((config->deb_cyc_l & WUP_IO_DEB_CYC_VMASK) << (pin * WUP_IO_DEB_CYC_OFFSET + WUP_IO_DEB_CYC_SHIFT));
	hal_writel(val, WUP_IO_WUP_CYC0_REG);

	/* set debounce cycle for high level*/
	val = hal_readl(WUP_IO_WUP_CYC1_REG);
	val &= ~(WUP_IO_DEB_CYC_VMASK << (pin * WUP_IO_DEB_CYC_OFFSET + WUP_IO_DEB_CYC_SHIFT));
	val |= ((config->deb_cyc_h & WUP_IO_DEB_CYC_VMASK) << (pin * WUP_IO_DEB_CYC_OFFSET + WUP_IO_DEB_CYC_SHIFT));
	hal_writel(val, WUP_IO_WUP_CYC1_REG);

}

void hal_wupio_set_hold(wupio_pin_t pin, uint32_t en)
{
	uint32_t val;

	en = (en ? 1 : 0);
	val = hal_readl(WUP_IO_HOLD_REG);
	val &= ~(1 << (pin + WUP_IO_HOLD_SHIFT));
	val |= (en << (pin + WUP_IO_HOLD_SHIFT));
	hal_writel(val, WUP_IO_HOLD_REG);
	/* must delay 3 cycles of 32k clock */
	hal_udelay(125);
}

void hal_wupio_enable(wupio_pin_t pin, uint32_t en)
{
	uint32_t val;

	en = (en ? 1 : 0);

	/* set wakeup detect */
	val = hal_readl(WUP_IO_EN_REG);
	val &= ~(1 << (pin + WUP_IO_EN_SHIFT));
	val |= (en << (pin + WUP_IO_EN_SHIFT));
	hal_writel(val, WUP_IO_EN_REG);
}

