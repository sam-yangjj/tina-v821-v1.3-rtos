#ifndef __INTC_H__
#define __INTC_H__

#define IRQ_SOUCE_MAX		CONFIG_IRQ_SOUCE_COUNT

typedef void (*interrupt_handler_t)(int, void *);

void interrupt_init(void);
void interrupt_exit(void);
s32 interrupt_enable(u32 intno);
s32 interrupt_disable(u32 intno);
s32 install_isr(u32 intno, interrupt_handler_t func, void *parg);
s32 uninstall_isr(u32 intno, interrupt_handler_t func);

s32 interrupt_query_pending(u32 intno);
s32 interrupt_clear_pending(u32 intno);
s32 interrupt_get_enabled(u32 intno);
u32 interrupt_get_current_intno(void);

#endif /* __INTC_H__ */
