#include <include.h>
#include "internal.h"

#if CONFIG_UART_PORT == 0
#define UART_IOBASE SUNXI_UART0_BASE
#endif
#if CONFIG_UART_PORT == 1
#define UART_IOBASE SUNXI_UART1_BASE
#define UART_TX_PIN  SUNXI_GPD(9)
#define UART_TX_FUNC 6
#define UART_RX_PIN  SUNXI_GPD(10)
#define UART_RX_FUNC 6
#endif
#if CONFIG_UART_PORT == 2
#define UART_IOBASE  SUNXI_UART2_BASE
#define UART_TX_PIN  SUNXI_GPC(4)
#define UART_TX_FUNC 4
#define UART_RX_PIN  SUNXI_GPC(5)
#define UART_RX_FUNC 4
#endif
#if CONFIG_UART_PORT == 3
#define UART_IOBASE SUNXI_UART3_BASE
#endif

static void boart_uart_init(void)
{
	sunxi_clock_init_uart(CONFIG_UART_PORT);

	sunxi_gpio_set_cfgpin(UART_TX_PIN, UART_TX_FUNC);
	sunxi_gpio_set_cfgpin(UART_RX_PIN, UART_RX_FUNC);

	uart_init(UART_IOBASE);
}

extern unsigned long boot_time[];

static void __jump(void)
{
#define RISCV_START_ADDR			(0x43030204)
#define BOOTTIME_RTC_REG(N)			(SUNXI_PRCM_BASE + 0x200 + 4 * N)

	u32 addr = CONFIG_RAM_START_ADDRESS;

	writel((u32)boot_time, BOOTTIME_RTC_REG(6));
	while (addr == CONFIG_RAM_START_ADDRESS || addr == 0) {
		addr = *(volatile u32 *)(RISCV_START_ADDR);
	}

	boot_time[3] = get_cycles();
	printf("jump to second firmware: 0x%08x\n", addr);

	asm volatile("mv s1, %0" : : "r"(addr) : "memory");
	asm volatile("jr s1");
}

#define TEST_IRQ				(0)

#if TEST_IRQ && defined(CONFIG_IRQCHIP_USED)
#define ARCH_TIMER_IRQn			(7)
#define RISCV_CLINT_BASE		(SUNXI_RISCV_CLINT_BASE)

typedef struct {
	volatile u64 MTIMECMP;            /*!< Offset: 0x4000 (R/W) Timer compare register */
	u32 RESERVED[8188];
	volatile u64 MTIME;               /*!< Offset: 0xBFF8 (R)  Timer current register */
} CORET_Type;
#define CORET					((CORET_Type   *)RISCV_CLINT_BASE)

static void program_next_time(u32 ms)
{
	CORET->MTIMECMP = CORET->MTIME + ms * 24000;
}

static volatile int systick;
static void systimer_handler(int irq, void *data)
{
	printf("irq:%d handled, systick: %d\r\n", irq, systick++);

	/* program next time event */
	interrupt_clear_pending(irq);
	program_next_time(1000);
}

static void test_irq(void)
{
	install_isr(ARCH_TIMER_IRQn, systimer_handler, NULL);
	interrupt_enable(ARCH_TIMER_IRQn);
	program_next_time(200);
	while(systick < 5);
	interrupt_disable(ARCH_TIMER_IRQn);
	udelay(500);
}
#endif

void arch_early_init(void)
{
	sysmap_init();
}

int main(void)
{
	boot_time[1] = get_cycles();
	sunxi_hosc_detect();
#if defined(CONFIG_IRQCHIP_USED)
	interrupt_init();
#endif

	boart_uart_init();

	printf("Hello, RISCV %ld\n", get_time_us());
#if defined CONFIG_VIN_USED
	csi_init();
#endif
	printf("Host Freq: %ldMHz\n", aw_get_hosc_freq());
	boot_time[2] = get_cycles();

#if TEST_IRQ && defined(CONFIG_IRQCHIP_USED)
	test_irq();
#endif
#if defined(CONFIG_IRQCHIP_USED)
	interrupt_exit();
#endif
	__jump();
	return 0;
}
