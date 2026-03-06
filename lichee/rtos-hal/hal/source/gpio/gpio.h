/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#ifndef __GPIO_I_H__
#define __GPIO_I_H__

#include <stdbool.h>
#include "hal_interrupt.h"
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#include <pm_wakesrc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PA_BASE 0
#define PB_BASE 32
#define PC_BASE 64
#define PD_BASE 96
#define PE_BASE 128
#define PF_BASE 160
#define PG_BASE 192
#define PH_BASE 224
#define PI_BASE 256
#define PJ_BASE 288
#define PK_BASE 320
#define PL_BASE 352
#define PM_BASE 384
#define PN_BASE 416
#define PO_BASE 448
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
#define EXA_BASE 480
#endif

/* sunxi gpio name space */
#define GPIOA(n)    (PA_BASE + (n))
#define GPIOB(n)    (PB_BASE + (n))
#define GPIOC(n)    (PC_BASE + (n))
#define GPIOD(n)    (PD_BASE + (n))
#define GPIOE(n)    (PE_BASE + (n))
#define GPIOF(n)    (PF_BASE + (n))
#define GPIOG(n)    (PG_BASE + (n))
#define GPIOH(n)    (PH_BASE + (n))
#define GPIOI(n)    (PI_BASE + (n))
#define GPIOJ(n)    (PJ_BASE + (n))
#define GPIOK(n)    (PK_BASE + (n))
#define GPIOL(n)    (PL_BASE + (n))
#define GPIOM(n)    (PM_BASE + (n))
#define GPION(n)    (PN_BASE + (n))
#define GPIOO(n)    (PO_BASE + (n))
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
#define GPIOEXP0(n)    (EXA_BASE + (n))
#define GPIOEXP1(n)    (EXA_BASE + 8 + (n))
#endif

/* GPIO_IRQ_START need to greater than GIC_IRQ_NUM */
#define GPIO_IRQ_START		200

#if defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN300IW1)
#define INITIAL_BANK_OFFSET 0x0
#define BANK_MEM_SIZE       0x30
#define PULL_REGS_OFFSET    0x24
#define DLEVEL_PINS_PER_REG 8
#define DLEVEL_PINS_BITS    4
#define DLEVEL_PINS_MASK    0x0f
#elif defined(CONFIG_ARCH_SUN60IW1)
#define INITIAL_BANK_OFFSET 0x0
#define BANK_MEM_SIZE       0x80
#define PULL_REGS_OFFSET    0x30
#define DLEVEL_PINS_PER_REG 8
#define DLEVEL_PINS_BITS    4
#define DLEVEL_PINS_MASK    0x0f
#elif defined(CONFIG_ARCH_SUN55IW6)
#define DLEVEL_PINS_PER_REG 8
#define DLEVEL_PINS_BITS    4
#define DLEVEL_PINS_MASK    0x0f
#else
#define INITIAL_BANK_OFFSET 0x0
#define BANK_MEM_SIZE       0x24
#define PULL_REGS_OFFSET    0x1c
#define DLEVEL_PINS_PER_REG 16
#define DLEVEL_PINS_BITS    2
#define DLEVEL_PINS_MASK    0x03
#endif

#define MUX_REGS_OFFSET     0x0
#define DATA_REGS_OFFSET    0x10
#define DLEVEL_REGS_OFFSET  0x14

#define PINS_PER_BANK       32
#define MUX_PINS_PER_REG    8
#define MUX_PINS_BITS       4
#define MUX_PINS_MASK       0x0f
#define DATA_PINS_PER_REG   32
#define DATA_PINS_BITS      1
#define DATA_PINS_MASK      0x01
#define PULL_PINS_PER_REG   16
#define PULL_PINS_BITS      2
#define PULL_PINS_MASK      0x03

#define IRQ_PER_BANK        32

#define IRQ_CFG_REG     0x200
#define IRQ_MEM_SIZE    0x20
#define IRQ_CFG_IRQ_PER_REG     8
#define IRQ_CFG_IRQ_BITS        4
#define IRQ_CFG_IRQ_MASK        ((1 << IRQ_CFG_IRQ_BITS) - 1)
#define IRQ_CTRL_REG        0x210
#define IRQ_CTRL_IRQ_PER_REG        32
#define IRQ_CTRL_IRQ_BITS       1
#define IRQ_CTRL_IRQ_MASK       ((1 << IRQ_CTRL_IRQ_BITS) - 1)
#define IRQ_STATUS_REG      0x214
#define IRQ_STATUS_IRQ_PER_REG      32
#define IRQ_STATUS_IRQ_BITS     1
#define IRQ_STATUS_IRQ_MASK     ((1 << IRQ_STATUS_IRQ_BITS) - 1)
#define IRQ_DEBOUNCE_REG        0x218

#if defined(CONFIG_ARCH_SUN55IW3)
#define POWER_MODE_SEL 0x0380
#define POWER_MODE_VAL 0x0388
#define POWER_VOL_SEL 0x0390
#elif defined(CONFIG_ARCH_SUN55IW6)
#define POWER_MODE_SEL 0x40
#define POWER_MODE_VAL 0x48
#define POWER_VOL_SEL 0x70
#else
#define POWER_MODE_SEL 0x0340
#define POWER_MODE_VAL 0x0348
#define POWER_VOL_SEL 0x0350
#endif

#define IRQ_EDGE_RISING     0x00
#define IRQ_EDGE_FALLING    0x01
#define IRQ_LEVEL_HIGH      0x02
#define IRQ_LEVEL_LOW       0x03
#define IRQ_EDGE_BOTH       0x04

#define SUNXI_PIO_BANK_BASE(pin, irq_bank) \
    ((pin-PA_BASE)/PINS_PER_BANK - irq_bank)

#define SUNXI_R_PIO_BANK_BASE(pin, irq_bank) \
    ((pin-PL_BASE)/PINS_PER_BANK - irq_bank)

/*
 * This looks more complex than it should be. But we need to
 * get the type for the ~ right in round_down (it needs to be
 * as wide as the result!), and we want to evaluate the macro
 * arguments just once each.
 */
#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/*
 * gpio configuration (pull up/down and drive strength) type and its value are
 * packed together into a 32-bits. The lower 8-bits represent the configuration
 * type and the upper 24-bits hold the value of the configuration type.
 */
#define GPIO_CFG_PACK(type, value)  (((value) << 8) | ((unsigned long) type & 0xFFUL))
#define GPIO_CFG_UNPACK_TYPE(cfg)   ((cfg) & 0xFFUL)
#define GPIO_CFG_UNPACK_VALUE(cfg)  (((cfg) & 0xFFFFFF00UL) >> 8)

typedef enum
{
    GPIO_TYPE_FUNC,
    GPIO_TYPE_DAT,
    GPIO_TYPE_PUD,
    GPIO_TYPE_DRV,
    GPIO_TYPE_VOL,
    GPIO_CONFIG_END = 0x7F,
    GPIO_CONFIG_MAX = 0xFF,
} pin_config_param_t;

typedef enum
{
    IRQ_TYPE_NONE       = 0x00000000,
    IRQ_TYPE_EDGE_RISING    = 0x00000001,
    IRQ_TYPE_EDGE_FALLING   = 0x00000002,
    IRQ_TYPE_EDGE_BOTH  = (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING),
    IRQ_TYPE_LEVEL_HIGH = 0x00000004,
    IRQ_TYPE_LEVEL_LOW  = 0x00000008,
} gpio_interrupt_mode_t;

struct gpio_irq_desc
{
    uint32_t virq;
    uint32_t pin;
    unsigned long flags;
    hal_irq_handler_t handle_irq;
    void *data;
};

enum sunxi_gpio_hw_type {
	SUNXI_PCTL_HW_TYPE_0,  /* chips: sun55iw6-pio */
	SUNXI_PCTL_HW_TYPE_1,  /* chips: sun55iw6-r-pio */
	/* Add new types here ... */
	SUNXI_PCTL_HW_TYPE_CNT,
};

struct sunxi_gpio_hw_info {
	uint8_t initial_bank_offset;	/* First bank offset to pin base */
	uint8_t mux_regs_offset;	/* Configure Register's offset */
	uint32_t data_regs_offset;	/* Data Register's offset */
	uint8_t dlevel_regs_offset;	/* Multi Driving Register's offset */
	uint8_t bank_mem_size;  	/* Size of the basic registers (including CFG/DAT/DRV/PUL) of any bank  */
	uint8_t pull_regs_offset;	/* Pull Register's offset */
	uint8_t dlevel_pins_per_reg; /* How many pins does a 'Multi-Driving Register' contain? */
	uint8_t dlevel_pins_bits;	/* How many bits does a 'Multi-Driving Register' use for a pin? */
	uint8_t dlevel_pins_mask;	/* Bit mask for 'dlevel_pins_bits' */
	uint8_t irq_mux_val;		/* Mux value for IRQ function */
	uint32_t irq_cfg_reg;
	uint32_t irq_ctrl_reg;
	uint32_t irq_status_reg;
	uint32_t irq_debounce_reg;
	uint32_t irq_mem_size;
	uint32_t irq_mem_base;
	uint32_t irq_mem_used;
	uint32_t pio_pow_ctrl_reg;
	uint32_t power_mode_val_reg;
	bool power_mode_detect;  /* true: Config voltage withstand by reading power_mode_val_reg */
	bool data_reg_irregular;
	uint32_t data_mem_size;
};

extern struct sunxi_gpio_hw_info sunxi_gpio_hw_info[SUNXI_PCTL_HW_TYPE_CNT];

struct gpio_desc
{
    const unsigned long membase;
    const uint32_t resource_size; /* reg resource size */
    const uint32_t irq_arry_size;
    const uint32_t *irq;
    const uint32_t pin_base;
    const uint32_t banks;
    const uint32_t *bank_base;
    const uint32_t irq_banks;
    const uint32_t *irq_bank_base;
    const uint32_t virq_offset;
    uint32_t irq_desc_size;
    struct gpio_irq_desc *irq_desc;
    enum sunxi_gpio_hw_type	hw_type;

#ifdef CONFIG_COMPONENTS_PM
	pm_wakesrc_t **ws;
#endif
};

struct gpio_pm_reg_cache
{
    void *reg_dump[2];
    int reg_dump_size[2];
};

/*
 * include the platform gpio header file,
 * should be after the name space macro.
 */
#if defined(CONFIG_ARCH_SUN8IW19)
#include "sun8iw19/platform-gpio.h"
#endif
#if defined(CONFIG_ARCH_SUN8IW18P1)
#include "sun8iw18/platform-gpio.h"
#endif
#if defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1)
#include "sun8iw20/platform-gpio.h"
#endif
#if defined(CONFIG_ARCH_SUN8IW21) || defined(CONFIG_ARCH_SUN20IW3)
#include "sun8iw21/platform-gpio.h"
#endif
#if defined(CONFIG_ARCH_SUN50IW11)
#include "sun50iw11/platform-gpio.h"
#endif
#if defined(CONFIG_ARCH_SUN20IW2)
#include "sun20iw2/platform-gpio.h"
#endif
#if defined(CONFIG_ARCH_SUN55IW3)
#include "sun55iw3/platform-gpio.h"
#endif
#if defined(CONFIG_ARCH_SUN55IW6)
#include "sun55iw6/platform-gpio.h"
#endif
#if defined(CONFIG_ARCH_SUN60IW1)
#include "sun60iw1/platform-gpio.h"
#endif
#if defined(CONFIG_ARCH_SUN300IW1)
#include "sun300iw1/platform-gpio.h"
#endif

const struct gpio_desc **gpio_get_platform_desc(void);
#ifdef CONFIG_STANDBY
int hal_gpio_suspend(void);
int hal_gpio_resume(void);
#endif
int hal_gpio_r_irq_disable(uint32_t irq);
int hal_gpio_r_irq_enable(uint32_t irq);
int hal_gpio_r_all_irq_disable(void);
#ifdef __cplusplus
}
#endif
#endif /* __GPIO_I_H__ */
