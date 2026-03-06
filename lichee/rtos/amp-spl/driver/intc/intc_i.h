#ifndef __INTC_I_H__
#define __INTC_I_H__

#include "include.h"

struct int_isr_node {
	interrupt_handler_t func;
	void *parg;
};

/*local functions*/
s32 intc_init(void);
s32 intc_exit(void);
s32 intc_enable_interrupt(u32 intno);
s32 intc_disable_interrupt(u32 intno);
u32 intc_get_current_interrupt(void);
s32 intc_interrupt_is_enabled(u32 intno);
s32 intc_set_mask(u32 intno, u32 mask);
s32 intc_interrupt_query_pending(u32 intno);
s32 intc_interrupt_clear_pending(u32 intno);

#endif /*__INTC_I_H__*/
