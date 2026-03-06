#include "intc_i.h"

struct int_isr_node isr_table[IRQ_SOUCE_MAX] = { 0 };

void interrupt_init(void)
{
	intc_init();
}

void interrupt_exit(void)
{
	intc_exit();
}

s32 interrupt_enable(u32 intno)
{
	return intc_enable_interrupt(intno);
}

s32 interrupt_disable(u32 intno)
{
	return intc_disable_interrupt(intno);
}

s32 interrupt_set_mask(u32 intno, u32 mask)
{
	return intc_set_mask(intno, mask);
}

s32 install_isr(u32 intno, interrupt_handler_t func, void *parg)
{
	/*default isr, install directly */
	printf("install isr %x\n", intno);
	isr_table[intno].func = func;
	isr_table[intno].parg = parg;

	return 0;
}

s32 uninstall_isr(u32 intno, interrupt_handler_t func)
{
	if (isr_table[intno].func == func) {
		isr_table[intno].func = NULL;
		isr_table[intno].parg = NULL;
	} else {
		printf("ISR not installed!\n");
		return -1;
	}

	return 0;
}

void interrupt_entry(void)
{
	u32 intno = intc_get_current_interrupt();

	(isr_table[intno].func)(intno, isr_table[intno].parg);
}

s32 interrupt_query_pending(u32 intno)
{
	return intc_interrupt_query_pending(intno);
}

s32 interrupt_clear_pending(u32 intno)
{
	return intc_interrupt_clear_pending(intno);
}

u32 interrupt_get_current_intno(void)
{
	return intc_get_current_interrupt();
}

s32 interrupt_get_enabled(u32 intno)
{
	return intc_interrupt_is_enabled(intno);
}

