#include "cpu_i.h"

#define L1_CACHE_BYTES			(32)

static unsigned long arch_timer_freq = 24;

void exit(s32 i)
{
	printk("system exit\n");
	while (1);
}

void aw_set_hosc_freq(unsigned long hosc_freq)
{
	arch_timer_freq = hosc_freq;
}

unsigned long aw_get_hosc_freq(void)
{
	return arch_timer_freq;
}

unsigned long get_cycles(void)
{
	u32 cyc;

	cyc = readl(CONFIG_TIMESTAMP_BASE_ADDR + 0);
	/* read latch */
	cyc |= (1 << 1);
	writel(cyc, CONFIG_TIMESTAMP_BASE_ADDR + 0);

	cyc = readl(CONFIG_TIMESTAMP_BASE_ADDR + 4);

	return cyc;
}

unsigned long get_time_us(void)
{
	return get_cycles() / arch_timer_freq;
}

void udelay(u32 us)
{
	u32 cyc = get_cycles();
	u32 tmp;

	cyc += us * arch_timer_freq;

	do {
		tmp = get_cycles();
	} while (cyc < tmp);
}

void dcache_invalid(unsigned long start, unsigned long end)
{
	unsigned long i = start & ~(L1_CACHE_BYTES - 1);

	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.ipa %0\n"::"r"(i):"memory");
	asm volatile("fence");
}

void dcache_clean(unsigned long start, unsigned long end)
{
	unsigned long i = start & ~(L1_CACHE_BYTES - 1);

	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.cpa %0\n"::"r"(i):"memory");
	asm volatile("fence");
}

void dcache_invalid_all(void)
{
	asm volatile("dcache.iall\n":::"memory");
}

void dcache_clean_all(void)
{
	asm volatile("dcache.call\n":::"memory");
}
