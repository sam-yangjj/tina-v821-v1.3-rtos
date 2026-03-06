/*
 * (C) Copyright allwinnertech
 *
 */

#ifndef _CPU_SUN300IW1_H
#define _CPU_SUN300IW1_H

#include <arch/sun300iw1/cpu_autogen.h>

#define SUNXI_RTC_DATA_BASE                 (SUNXI_PRCM_BASE + 0x200)
#define SUNXI_RTC_BOOT_FLAG_ADDR            (SUNXI_PRCM_BASE + 0x1c0)
#define SUNXI_WDOG_BASE                     (SUNXI_E907_WDG_BASE)
#define SUNXI_PIO_BASE                      (SUNXI_GPIO_BASE)
#define SUNXI_R_PIO_BASE                    (0x42000540)
#define SUNXI_RTWI_BASE                     (SUNXI_TWI0_BASE)
#define SUNXI_SPIF_BASE			    (SUNXI_SPI_FLASH_BASE)
#define SUNXI_DMA_BASE              (SUNXI_DMAC0__SGDMA__BASE)

#define GPIO_BIAS_MAX_LEN (32)
#define GPIO_BIAS_MAIN_NAME "gpio_bias"
#define GPIO_POW_MODE_REG (0x0340)
#define GPIO_POW_MODE_VAL_REG		(0x0348)
#define GPIO_3_3V_MODE 0
#define GPIO_1_8V_MODE 1


#define PIOC_REG_o_POW_MOD_SEL   0x340
#define PIOC_REG_o_POW_MS_CTL   0x344
#define PIOC_REG_o_POW_MS_VAL   0x348

#define PIOC_REG_POW_MOD_SEL  (SUNXI_PIO_BASE + PIOC_REG_o_POW_MOD_SEL)
#define PIOC_REG_POW_MS_CTL   (SUNXI_PIO_BASE + PIOC_REG_o_POW_MS_CTL)
#define PIOC_REG_POW_VAL   (SUNXI_PIO_BASE + PIOC_REG_o_POW_MS_VAL)

#define PIOC_SEL_Px_3_3V_VOL 0
#define PIOC_SEL_Px_1_8V_VOL 1

#define PIOC_CTL_Px_ENABLE 0
#define PIOC_CTL_Px_DISABLE 1

#define PIOC_VAL_Px_3_3V_VOL 0
#define PIOC_VAL_Px_1_8V_VOL 1

#define PIOC_CTL_Px_DEFUALT PIOC_CTL_Px_ENABLE
#define PIOC_SEL_Px_DEFAULT PIOC_SEL_Px_1_8V_VOL

#define CCM_UART_PLATFORM_CLK     (192000000)

#endif /*  _CPU_SUN300IW1_H*/
