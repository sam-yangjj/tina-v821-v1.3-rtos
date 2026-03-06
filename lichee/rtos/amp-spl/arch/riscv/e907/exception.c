#include "cpu_i.h"

typedef struct pr_regs {
	unsigned long x[31]; /* x1 - x31 */
	unsigned long mepc;
	unsigned long mtval;
} __attribute__((packed)) pr_regs_t;

unsigned long hadle_trap(unsigned long mcause, pr_regs_t *regs)
{
	int i;
	char buf[512];
	int ret = 0;

	snprintf(buf, sizeof(buf), "RISCV Exception: 0x%08lx\n", mcause);
	printk("%s", buf);
	snprintf(buf, sizeof(buf), "mepc: 0x%08lx mtval: 0x%08lx\n", regs->mepc, regs->mtval);
	printk("%s", buf);

	ret += snprintf(buf, sizeof(buf), "x%02d: 0x%08lx ", 0, 0UL);
	for (i = 0; i < 31; i++) {
		ret += snprintf(buf + ret, sizeof(buf), "x%2d: 0x%08lx ", i + 1, regs->x[i]);
		if (((i + 2) % 4) == 0) {
			printk("%s\r\n", buf);
			ret = 0;
		}
	}

	while(1);
}

