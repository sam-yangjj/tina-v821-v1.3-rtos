/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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
#include "clint.h"
#include "platform/platform_irq.h"
#include "platform/platform_clock.h"
#include <hal/sunxi_hal_common.h>
#ifdef CONFIG_DRIVERS_CCMU
#include "hal_clk.h"
#endif

static unsigned int arch_counter_freq;

unsigned int rv_get_arch_counter_clk_freq(void)
{
	return arch_counter_freq;
}

uint64_t rv_read_arch_counter(void)
{
	uint64_t low, high;
	unsigned long high_addr, low_addr;

	low_addr = PLAT_CLINT_BASE_ADDR + CLINT_MTIMELO_REG_OFF;
	high_addr = low_addr + 4;

	do
	{
		high = hal_readl(high_addr);
		low = hal_readl(low_addr);
	}
	while (high != hal_readl(high_addr));

	return (high << 32) | low;
}

void rv_arch_timer_set_compare_value(uint64_t value)
{
	unsigned long high_addr, low_addr;
	uint32_t mask = -1U;

	low_addr = PLAT_CLINT_BASE_ADDR + CLINT_MTIMECMPLO_REG_OFF;
	high_addr = low_addr + 4;

	hal_writel(value & mask, low_addr);
	hal_writel(value >> 32, high_addr);
}

uint64_t rv_arch_timer_get_compare_value(void)
{
	uint64_t low, high;
	unsigned long high_addr, low_addr;

	low_addr = PLAT_CLINT_BASE_ADDR + CLINT_MTIMECMPLO_REG_OFF;
	high_addr = low_addr + 4;

	low = hal_readl(low_addr);
	high = hal_readl(high_addr);

	return (high << 32) | low;
}

static void rv_arch_timer_reload(uint64_t interval)
{
	uint64_t value;

	value = rv_read_arch_counter() + interval;
	rv_arch_timer_set_compare_value(value);
}

void rv_arch_timer_init(void)
{
	/* for certain platform, config timer clock is needed */
	plat_arch_timer_clock_init();

#ifdef CONFIG_DRIVERS_CCMU
	arch_counter_freq = hal_clock_get_hosc_freq();
#endif
	if (!arch_counter_freq)
	{
		arch_counter_freq = PLAT_ARCH_COUNTER_CLK_FREQ;
	}

	rv_arch_timer_reload(arch_counter_freq / CONFIG_HZ);
}

void rv_arch_timer_irq_handler_non_vec_mode(void)
{
	printf("ERROR: RV arch timer interrupt is not vector mode!\n");
}

void rv_soft_irq_handler_non_vec_mode(void)
{
	printf("ERROR: RV software interrupt is not vector mode!\n");
}

void rv_arch_timer_irq_handler(void)
{
	int ret;

	rv_arch_timer_reload(arch_counter_freq / CONFIG_HZ);

	ret = xTaskIncrementTick();
	if (ret)
		vTaskSwitchContext();
}

void rv_soft_irq_handler(void)
{
	volatile uint32_t *msip_reg = (volatile uint32_t *)(PLAT_CLINT_BASE_ADDR + CLINT_MSIP_REG_OFF);

	*msip_reg = 0;
	__asm__ volatile("fence iorw,iorw");
	__asm__ volatile(".insn r 0xb, 0, 0, x0, x0, x24");

	vTaskSwitchContext();
}

void rv_trigger_soft_interrupt(void)
{
	volatile uint32_t *msip_reg = (volatile uint32_t *)(PLAT_CLINT_BASE_ADDR + CLINT_MSIP_REG_OFF);

	*msip_reg = 1;
	__asm__ volatile("fence iorw,iorw");
	/*
	 * The inline assembly code below is T-head extension instruction "sync".
	 * It is similar to "__asm__ volatile(".long 0x0180000B");" which will be
	 * treated as data(since addr2line tool will report incorrect line number).
	 * So we need use the RISC-V dependent directive ".insn" other than ".long" or ".word"
	 */
	__asm__ volatile(".insn r 0xb, 0, 0, x0, x0, x24");
}

#if defined(CONFIG_COMPONENTS_PM)
#include "pm_syscore.h"

static uint64_t g_interval_after_wake_up = 0;

static int clint_suspend(void *data, suspend_mode_t mode)
{
	uint64_t current_count = rv_read_arch_counter();
	uint64_t target_count_value = rv_arch_timer_get_compare_value();

	if (current_count < target_count_value)
	{
		g_interval_after_wake_up = target_count_value - current_count;
	}
	else
	{
		g_interval_after_wake_up = arch_counter_freq / CONFIG_HZ;
	}
	return 0;
}

static void clint_resume(void *data, suspend_mode_t mode)
{
	rv_arch_timer_reload(g_interval_after_wake_up);
}

static struct syscore_ops g_clint_syscore_ops =
{
	.name = "clint_syscore_ops",
	.suspend = clint_suspend,
	.resume = clint_resume,
};

int clint_pm_init(void)
{
	int ret = pm_syscore_register(&g_clint_syscore_ops);
	if (ret)
		printf("Warning: pm_devops_register for clic failed, ret: %d", ret);

	return ret;
}
#endif
