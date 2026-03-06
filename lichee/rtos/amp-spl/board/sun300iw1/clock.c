/*
 * (C) Copyright 2018-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 */

#include <include.h>

void sunxi_hosc_detect(void)
{
    u32 counter_val = 0;
	writel(HOSC_FREQ_DET_HOSC_ENABLE_DETECT, CCMU_HOSC_FREQ_DET_REG);
	while (!(HOSC_FREQ_DET_HOSC_FREQ_READY_CLEAR_MASK &
			readl(CCMU_HOSC_FREQ_DET_REG)))
		;
	counter_val = (readl(CCMU_HOSC_FREQ_DET_REG) &
			  HOSC_FREQ_DET_HOSC_FREQ_DET_CLEAR_MASK) >>
			  HOSC_FREQ_DET_HOSC_FREQ_DET_OFFSET;
	if (counter_val < ((HOSC_24M_COUNTER + HOSC_40M_COUNTER)/2))
		aw_set_hosc_freq(24);
	else
		aw_set_hosc_freq(40);
}

void sunxi_clock_init_uart(int port)
{
	u32 i, reg;

	if (port < 0 || port > 3)
		return;

	/* reset */
	reg = readl(CCMU_UART_RST_REG);
	reg &= ~(1<<(BUS_Reset0_REG_PRESETN_UART0_SW_OFFSET + port));
	writel(reg, CCMU_UART_RST_REG);
	for (i = 0; i < 100; i++)
		;
	reg |= (1 << (BUS_Reset0_REG_PRESETN_UART0_SW_OFFSET + port));
	writel(reg, CCMU_UART_RST_REG);
	/* gate */
	reg = readl(CCMU_UART_BGR_REG);
	reg &= ~(1<<(BUS_CLK_GATING0_REG_UART0_PCLK_EN_OFFSET + port));
	writel(reg, CCMU_UART_BGR_REG);
	for (i = 0; i < 100; i++)
		;
	reg |= (1 << (BUS_CLK_GATING0_REG_UART0_PCLK_EN_OFFSET + port));
	writel(reg, CCMU_UART_BGR_REG);
}
