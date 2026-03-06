#ifndef __CPU_H__
#define __CPU_H__

#if defined(CONFIG_ARCH_SUN300IW1)
#include <arch/sun300iw1/cpu_sun300iw1.h>
#include <arch/sun300iw1/clock_sun300iw1_aon.h>
#include <arch/sun300iw1/clock_sun300iw1_app.h>
#else
#error "Unsupported plat"
#endif

/* cache operation fucntion */
void dcache_invalid(unsigned long start, unsigned long end);
void dcache_clean(unsigned long start, unsigned long end);
void dcache_invalid_all(void);
void dcache_clean_all(void);

void aw_set_hosc_freq(unsigned long hosc_freq);
unsigned long aw_get_hosc_freq(void);
unsigned long get_cycles(void);
unsigned long get_time_us(void);
void udelay(u32 us);

void exit(int i);

#endif /* __CPU_H__ */
