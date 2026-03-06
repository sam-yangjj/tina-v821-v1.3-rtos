#ifndef _OPENAMP_SHARE_IRQ_H
#define _OPENAMP_SHARE_IRQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define RPROC_IRQ_TAB  2

int openamp_share_irq_early_init(void);
int openamp_share_irq_init(void *share_buf, int buf_len);
int openamp_share_irq_deinit(void *share_buf);
uint32_t sunxi_get_banks_mask(int banks);
bool sunxi_is_arch_irq(int irq);

#ifdef __cplusplus
}
#endif

#endif
