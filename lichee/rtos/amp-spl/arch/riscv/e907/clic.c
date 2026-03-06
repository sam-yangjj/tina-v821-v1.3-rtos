#include "cpu_i.h"

#define RISCV_CLIC_BASE		(CONFIG_RISCV_CLIC_BASE)

/* interrput controller registers offset */
#define CLIC_CFG			(RISCV_CLIC_BASE + 0x0)
#define CLIC_INFO			(RISCV_CLIC_BASE + 0x4)
#define CLIC_MINTTHRESH		(RISCV_CLIC_BASE + 0x8)
#define CLIC_INT_IP(n)		(RISCV_CLIC_BASE + 0x1000 + 0x4*n) // byte
#define CLIC_INT_IE(n)		(RISCV_CLIC_BASE + 0x1001 + 0x4*n) // byte
#define CLIC_INT_ATTR(n)	(RISCV_CLIC_BASE + 0x1002 + 0x4*n) // byte
#define CLIC_INT_CTL(n)		(RISCV_CLIC_BASE + 0x1003 + 0x4*n) // byte
#define CLIC_INT_REG(n)		(RISCV_CLIC_BASE + 0x1000 + 0x4*n) // word

void intc_init(void)
{
	int i;
	unsigned long val;

	for (i = 0; i < IRQ_SOUCE_MAX; i++) {
		writeb(0, CLIC_INT_IP(i));
		writeb(1, CLIC_INT_ATTR(i)); /* vector interrupt */
	}

	/* enable interrupt */
	val = 0x8;
	__asm volatile("csrs mstatus, %0" : : "r"(val): "memory");
}

void intc_exit(void)
{
	unsigned long val;

	/* disable interrupt */
	val = 0x8;
	__asm volatile("csrc mstatus, %0" : : "r"(val): "memory");
}

s32 intc_enable_interrupt(u32 intno)
{
	writeb(readb(CLIC_INT_IE(intno)) | 0x1, CLIC_INT_IE(intno));

	printf("intno:%d interrupt enable\n", intno);

	return 0;
}

s32 intc_disable_interrupt(u32 intno)
{
	writeb(readb(CLIC_INT_IE(intno)) & (~0x1), CLIC_INT_IE(intno));

	printf("intno:%d interrupt disable\n", intno);

	return 0;
}

s32 intc_interrupt_query_pending(u32 intno)
{
	return (readb(CLIC_INT_IP(intno)) & 0x1);
}

s32 intc_interrupt_clear_pending(u32 intno)
{
	writeb(readb(CLIC_INT_IP(intno)) | 0x1, CLIC_INT_IP(intno));

	return 0;
}

s32 intc_interrupt_is_enabled(u32 intno)
{
	return (readb(CLIC_INT_IE(intno)) & 0x1);
}

u32 intc_get_current_interrupt(void)
{
	unsigned long val;

	__asm volatile("csrr %0, mcause" : "=r"(val));

	return (val & 0xff);
}

s32 intc_set_mask(u32 intno, u32 mask)
{
	writeb((readb(CLIC_INT_IE(intno)) & (~0x1)) | (mask & 0x1), CLIC_INT_IE(intno));

	return 0;
}
