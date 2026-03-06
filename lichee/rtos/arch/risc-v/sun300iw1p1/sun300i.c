/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <aw_version.h>
#include <irqs.h>
#include <platform.h>
#include <memory.h>
#include <hal_thread.h>
#include <hal_time.h>

#include <serial.h>
#include <interrupt.h>

#ifdef CONFIG_DRIVERS_GPIO
#include <hal_gpio.h>
#endif
#ifdef CONFIG_DRIVERS_UART
#include <hal_uart.h>
#endif
#ifdef CONFIG_DRIVERS_MSGBOX
#include <hal_msgbox.h>
#endif
#include <hal_clk.h>
#ifdef CONFIG_DRIVERS_DMA
#include <hal_dma.h>
#endif
#ifdef CONFIG_DRIVERS_WATCHDOG
#include <sunxi_hal_watchdog.h>
#endif
#ifdef CONFIG_DRIVERS_CAR_REVERSE
#include <car_reverse/car.h>
#endif
#include <hal_cache.h>

#include "excep.h"

#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
#include <hal_cfg.h>
#endif

#include <compiler.h>

#ifdef CONFIG_DRIVERS_SPINOR
#include <sunxi_hal_spinor.h>
#endif

#ifdef CONFIG_DRIVERS_MSGBOX
#include <hal_msgbox.h>
#endif

#ifdef CONFIG_ARCH_RISCV_PMP
#include <pmp.h>
#endif

#ifdef CONFIG_KASAN
#include <kasan_rtos.h>
#endif

#include <e90x_sysmap.h>


#ifdef CONFIG_DEBUG_BACKTRACE
#include <backtrace.h>

const backtrace_valid_address_t bt_valid_addr[] = {
    { .start = CONFIG_ARCH_START_ADDRESS , \
      .end = CONFIG_ARCH_START_ADDRESS + CONFIG_ARCH_MEM_LENGTH - 1 },
    { },
};

#endif

#ifdef CONFIG_COMPONENTS_PM
#include <pm_init.h>
int platform_irq_pm_init(void);
#endif

#ifdef CONFIG_DRIVERS_HWSPINLOCK
#include <hal_hwspinlock.h>
#endif

#ifdef CONFIG_AMP_SHARE_IRQ
#include <openamp/openamp_share_irq.h>
#endif

void timekeeping_init(void);

#ifdef CONFIG_PROJECT_V821_E907
#define RV_PLATFORM "V821_E907"
#endif

int start_kernel_flag = 1;

static void print_banner()
{
    printf("\r\n");
    printf(" *******************************************\r\n");
    printf(" ** Welcome to %s FreeRTOS %-10s**\r\n", RV_PLATFORM, TINA_RT_VERSION_NUMBER);
    printf(" ** Copyright (C) 2019-2021 AllwinnerTech **\r\n");
    printf(" **                                       **\r\n");
    printf(" **      starting riscv FreeRTOS          **\r\n");
    printf(" *******************************************\r\n");
    printf("\r\n");
    printf("Date:%s, Time:%s\n", __DATE__, __TIME__);
}

__weak void sunxi_dma_init(void)
{
    return;
}

__weak void sunxi_gpio_init(void)
{
    return;
}

__weak int sunxi_soundcard_init(void)
{
    return 0;
}

__weak void heap_init(void)
{
    return;
}

int init_sysmap(void)
{
	sysmap_add_mem_region(0x00000000, 0x10000000, SYSMAP_MEM_ATTR_RAM);
	sysmap_add_mem_region(0x10000000, 0x02000000, SYSMAP_MEM_ATTR_RAM);
	sysmap_add_mem_region(0x12000000, 0x1E000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x30000000, 0x10000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x40000000, 0x28000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x68000000, 0x01000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x69000000, 0x17000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x80000000, 0x7FFFFFFF, SYSMAP_MEM_ATTR_RAM);
	return 0;
}

extern const unsigned long __text_start__[], __etext[];
/* default pmp config */
__weak void pmp_init(void)
{
#ifdef CONFIG_ARCH_RISCV_PMP
	/* default region */
	pmp_add_region(0x00000000, 0xe0000000, PMP_R);
	/* I/O region */
	pmp_add_region(0x40000000, 0x80000000, PMP_R | PMP_W);
	/* code region */
	pmp_add_region((unsigned long)__text_start__,
					(unsigned long)__etext, PMP_R | PMP_X);
	/* data,bss region */
	pmp_add_region((unsigned long)__etext,
					CONFIG_ARCH_START_ADDRESS + CONFIG_ARCH_MEM_LENGTH,
					PMP_R | PMP_W);

#ifdef CONFIG_KASAN
        /* kasan shadow region */
        pmp_add_region(KASAN_SHADOW_START, KASAN_SHADOW_END, PMP_R | PMP_W);
#endif

#ifdef CONFIG_PMP_EARLY_ENABLE

#ifdef CONFIG_DRIVERS_VIN
	extern int set_pmp_for_isp_reserved_mem(void);
	set_pmp_for_isp_reserved_mem();
#endif

#ifdef CONFIG_COMPONENTS_RPBUF
	extern int set_pmp_for_rpbuf_reserved_mem(void);
	set_pmp_for_rpbuf_reserved_mem();
#endif

#ifdef CONFIG_COMPONENTS_OPENAMP
	/*
	 * make sure this region
	 * the same as board.dts rv_vdev0buffer,rv_vdev0vring0,rv_vdev0vring1
	 */
	unsigned long start_addr = CONFIG_VDEV_BUF_RESERVED_MEM_ADDR;
	unsigned long end_addr = start_addr + CONFIG_VDEV_BUF_RESERVED_MEM_SIZE;
	pmp_add_region(start_addr, end_addr, PMP_R | PMP_W);

	start_addr = CONFIG_VDEV_VRING0_RESERVED_MEM_ADDR;
	end_addr = start_addr + CONFIG_VDEV_VRING0_RESERVED_MEM_SIZE;
	pmp_add_region(start_addr, end_addr, PMP_R | PMP_W);

	start_addr = CONFIG_VDEV_VRING1_RESERVED_MEM_ADDR;
	end_addr = start_addr + CONFIG_VDEV_VRING1_RESERVED_MEM_SIZE;
	pmp_add_region(start_addr, end_addr, PMP_R | PMP_W);
#endif /* CONFIG_COMPONENTS_OPENAMP */
	pmp_enable();
#endif /* CONFIG_PMP_EARLY_ENABLE */

#endif
	return;
}

void prvSetupHardware(void)
{

    pmp_init();
    timekeeping_init();

#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
    hal_cfg_init();
#endif

    hal_clock_init();

#ifdef CONFIG_DRIVERS_GPIO
    hal_gpio_init();
#endif
    serial_init();

#ifdef CONFIG_DRIVERS_DMA
    hal_dma_init();
#endif

#ifdef CONFIG_COMPONENTS_PM
	int ret = platform_irq_pm_init();
	if (ret)
		printf("platform_irq_pm_init failed, ret: %d", ret);
#endif
}

unsigned long _24M_long_counter(void)
{
         unsigned long cnt=0;

         __asm__ __volatile__(
                         "rdtime %0\n"
                         :"=r"(cnt)
        );

        return (cnt);
}

void systeminit(void)
{
}

#ifdef CONFIG_PM_HEAP_RETAIN
extern void heap_retain_init(void);
extern void heap_retain_start(void);
#endif

extern void six_test_irq(void);
extern void six_check_irq_status(void);
void start_kernel(unsigned long boot_time)
{
	portBASE_TYPE ret;
#ifdef CONFIG_COMPONENTS_STACK_PROTECTOR
	//void stack_protector_init(void);
	//stack_protector_init();
#endif

	extern void hardware_config(void);
	hardware_config();

#ifdef CONFIG_ARCH_HAVE_ICACHE
	hal_icache_init();
#endif

#ifdef CONFIG_ARCH_HAVE_DCACHE
	hal_dcache_init();
#endif

	systeminit();

#ifdef CONFIG_KASAN
        kasan_init();
#endif

	/* Init heap */
	heap_init();

#ifdef CONFIG_KASAN
        extern void do_ctors(void);
        do_ctors();
#endif

#ifdef CONFIG_PM_HEAP_RETAIN
	heap_retain_init();
	heap_retain_start();
#endif

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_init();
	pm_syscore_init();
#endif

#ifdef CONFIG_AMP_SHARE_IRQ
	openamp_share_irq_early_init();
#endif

	/* Init hardware devices */
	prvSetupHardware();

#ifdef CONFIG_MULTI_CONSOLE
	extern int multiple_console_early_init(void);
	multiple_console_early_init();
#endif
	/* Setup kernel components */
	print_banner();

	setbuf(stdout, 0);
	setbuf(stdin, 0);
	setvbuf(stdin, NULL, _IONBF, 0);

#ifdef CONFIG_COMPONENT_CPLUSPLUS
	/* It should be called after the stdout is ready, otherwise the std:cout can't work  */
    int cplusplus_system_init(void);
    cplusplus_system_init();
#endif

#ifdef CONFIG_DRIVERS_HWSPINLOCK
	hal_hwspinlock_init();
#endif

#ifdef CONFIG_DRIVERS_WATCHDOG
	hal_watchdog_init();
#endif

#ifdef CONFIG_DRIVERS_MSGBOX
	hal_msgbox_init();
#endif

    start_kernel_flag = 0;

#ifdef CONFIG_COMPONENTS_AMP_HW_WATCHDOG
	extern int amp_hw_wdog_init(void);
	amp_hw_wdog_init();
#endif

#ifdef CONFIG_COMPONENTS_AMP_TIMESTAMP
	extern int amp_timestamp_init(void);
	amp_timestamp_init();
#endif

#ifdef CONFIG_COMPONENTS_AW_RPC
	extern int sunxi_rpc_init(void);
	sunxi_rpc_init();
#endif

#ifdef CONFIG_COMPONENTS_RPCFS_AUTO_MOUNT
	extern int rpcfs_register_vfs(const char *mnt_path);
	if (rpcfs_register_vfs(CONFIG_COMPONENTS_RPCFS_PATH))
		printf("mount rpcfs to '%s' failed\r\n", CONFIG_COMPONENTS_RPCFS_PATH);

#endif

	printf("Boot time: %d us\r\n", (uint32_t)boot_time / (hal_get_count_freq() / 1000000));
	printf("SystemInit Done: cost %d us\r\n",
					(((uint32_t)hal_get_count_value()) - (uint32_t)boot_time) / (hal_get_count_freq() / 1000000));

	extern void cpu0_app_entry(void *);
	ret = xTaskCreate(cpu0_app_entry, "init-thread-0", CONFIG_INIT_THREAD_STACK_SIZE, NULL, HAL_THREAD_PRIORITY_APP, NULL);
	if (ret != pdPASS)
	{
		printf("Error creating task, status was %d\n", ret);
		while (1);
	}

	vTaskStartScheduler();
}
