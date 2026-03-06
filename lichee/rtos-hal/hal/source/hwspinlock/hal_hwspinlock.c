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
#include <sunxi_hal_hwspinlock.h>
#include <hal_time.h>
#include <hal_interrupt.h>

#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#endif

#if defined CONFIG_COMPONENTS_PM && !defined CONFIG_HWSPINLOCK_CLK_INIT_IN_OTHER_CORE
/*
 * Other drivers which use hwspinlock get function
 * must put hwspinlock in suspend,
 * Because It will be invalid after hwspinlock resume
 */
struct hwspinlock_hardware_resource {
	hal_clk_t clk;
	struct reset_control *reset;
};
struct hwspinlock_hardware_resource priv;

static int hal_hwspinlock_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	if ((!priv.reset) || (!priv.clk))
		return HWSPINLOCK_PM_ERR;

	hal_clock_disable(priv.clk);
	hal_reset_control_assert(priv.reset);

	return 0;
}

static int hal_hwspinlock_resume(struct pm_device *dev, suspend_mode_t mode)
{
	if ((!priv.reset) || (!priv.clk))
		return HWSPINLOCK_PM_ERR;

	hal_reset_control_deassert(priv.reset);
	hal_clock_enable(priv.clk);

	return 0;
}

static struct pm_devops hwspinlock_devops = {
	.suspend = hal_hwspinlock_suspend,
	.resume = hal_hwspinlock_resume,
};

static struct pm_device hwspinlock_dev = {
	.name = "hwspinlock",
	.ops = &hwspinlock_devops,
};
#endif

void hal_hwspinlock_init(void)
{
#ifdef CONFIG_HWSPINLOCK_CLK_INIT_IN_OTHER_CORE
	/* hwspinlock clk should init in arm */
#else
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	u32 reset_id = RST_HWSPINLOCK;
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_id_t clk_id = CLK_HWSPINLOCK;
	hal_clk_t clk;
	struct reset_control *reset;

	reset = hal_reset_control_get(reset_type, reset_id);
	hal_reset_control_deassert(reset);

	clk = hal_clock_get(clk_type, clk_id);
	hal_clock_enable(clk);
#endif
#if defined CONFIG_COMPONENTS_PM && !defined CONFIG_HWSPINLOCK_CLK_INIT_IN_OTHER_CORE
	priv.clk = clk;
	priv.reset = reset;
	pm_devops_register(&hwspinlock_dev);
#endif
}

int hal_hwspinlock_check_taken(int num)
{
	return !!(readl(SPINLOCK_STATUS_REG) & (1 << num));
}

int hal_hwspinlock_get(int num)
{
	unsigned long addr = SPINLOCK_LOCK_REG(num);
	int status;

	if (num > SPINLOCK_NUM)
		return HWSPINLOCK_EXCEED_MAX;

	status = readl(addr);

	if (status == SPINLOCK_NOTTAKEN)
		return HWSPINLOCK_OK;

	return HWSPINLOCK_ERR;
}

int hal_hwspinlock_put(int num)
{
	unsigned long addr = SPINLOCK_LOCK_REG(num);

	if (num > SPINLOCK_NUM)
		return HWSPINLOCK_EXCEED_MAX;

	writel(SPINLOCK_NOTTAKEN, addr);

	return HWSPINLOCK_OK;
}

int hal_hwspin_lock(int num)
{
	if (num > SPINLOCK_NUM)
		return HWSPINLOCK_ERR;

	while(hal_hwspinlock_get(num) != HWSPINLOCK_OK);
	return HWSPINLOCK_OK;
}

int hal_hwspin_lock_timeout(int num, int ms_timeout)
{
	int us;

	hal_assert(!(hal_interrupt_get_nest() && ms_timeout == 0));

	if (num > SPINLOCK_NUM)
		return HWSPINLOCK_ERR;

	us = ms_timeout ? SPINLOCK_MAX_UDELAY : 0;
	while(hal_hwspinlock_get(num) != HWSPINLOCK_OK) {
		if (us-- > 0) {
			hal_udelay(1);
		} else if (ms_timeout-- > 0) {
			hal_msleep(1);
		} else {
			return HWSPINLOCK_TIMEOUT;
		}
	}
	return HWSPINLOCK_OK;
}

void hal_hwspin_unlock(int num)
{
	hal_hwspinlock_put(num);
}
