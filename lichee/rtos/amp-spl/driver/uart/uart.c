/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 */
/*
 * COM1 NS16550 support
 * originally from linux source (arch/powerpc/boot/ns16550.c)
 * modified to use CONFIG_SYS_ISA_MEM and new defines
 */

#include <include.h>

#ifdef CCM_UART_PLATFORM_CLK
#define CCM_UART_CLK              (CCM_UART_PLATFORM_CLK)
#else
#define CCM_UART_CLK              (24000000)
#endif

typedef struct serial_hw {
	volatile u32 rbr;
	volatile u32 ier;
	volatile u32 fcr;
	volatile u32 lcr;
	volatile u32 mcr;
	volatile u32 lsr;
	volatile u32 msr;
	volatile u32 sch;
} serial_hw_t;

#define thr rbr
#define dll rbr
#define dlh ier
#define iir fcr

static serial_hw_t *serial_ctrl_base = NULL;

void uart_init(unsigned long iobase)
{
	u32 uart_clk;

	serial_ctrl_base = (serial_hw_t *)(iobase);

	serial_ctrl_base->mcr = 0x3;
	uart_clk = (CCM_UART_CLK + 8 * CONFIG_UART_BAUD) / (16 * CONFIG_UART_BAUD);
	serial_ctrl_base->lcr |= 0x80;
	serial_ctrl_base->dlh = uart_clk >> 8;
	serial_ctrl_base->dll = uart_clk & 0xff;
	serial_ctrl_base->lcr &= ~0x80;
	serial_ctrl_base->lcr = ((CONFIG_UART_PARITY & 0x03) << 3) |
			((CONFIG_UART_STOP & 0x01) << 2) | (CONFIG_UART_DATELEN & 0x03);
	serial_ctrl_base->fcr = 0x7;

	return;
}

void uart_putc(char c)
{
	if (!serial_ctrl_base)
		return;

	while ((serial_ctrl_base->lsr & (1 << 6)) == 0)
		;
	serial_ctrl_base->thr = c;
}

char uart_getc(void)
{
	if (!serial_ctrl_base)
		return 0;

	while ((serial_ctrl_base->lsr & 1) == 0)
		;
	return serial_ctrl_base->rbr;
}
