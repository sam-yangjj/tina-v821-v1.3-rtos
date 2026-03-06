#include <stdint.h>

extern volatile uint32_t ullPortInterruptNesting;
extern int cur_cpu_id(void);

uint32_t uGetInterruptNest(void)
{
    return ullPortInterruptNesting;
}
