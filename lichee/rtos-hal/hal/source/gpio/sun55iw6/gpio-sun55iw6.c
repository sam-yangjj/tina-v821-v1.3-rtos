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

#include <stdio.h>
#include <string.h>

#include <hal_gpio.h>
#include "../gpio.h"

struct sunxi_gpio_hw_info sunxi_gpio_hw_info[SUNXI_PCTL_HW_TYPE_CNT] = {
	{
		.initial_bank_offset	= 0x80,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.bank_mem_size		= 0x80,
		.pull_regs_offset	= 0x30,
		.irq_mux_val		= 0xE,
		.irq_cfg_reg		= 0xC0,
		.irq_ctrl_reg		= 0xD0,
		.irq_status_reg		= 0xD4,
		.irq_debounce_reg	= 0xD8,
		.irq_mem_base		= 0xC0,
		.irq_mem_size		= 0x80,
		.irq_mem_used		= 0x20,
		.power_mode_val_reg	= 0x48,
		.pio_pow_ctrl_reg	= 0x70,
	},
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x500,
		.data_reg_irregular	= true,
		.data_mem_size		= 0x10,
		.bank_mem_size          = 0x30,
		.pull_regs_offset       = 0x24,
		.irq_mux_val         	= 0xE,
		.irq_cfg_reg		= 0x200,
		.irq_ctrl_reg		= 0x210,
		.irq_status_reg		= 0x214,
		.irq_debounce_reg	= 0x218,
		.irq_mem_base		= 0x200,
		.irq_mem_size		= 0x20,
		.irq_mem_used		= 0x20,
		.power_mode_val_reg	= 0x348,
		.pio_pow_ctrl_reg	= 0x350,
	},
};

static const unsigned int sun55iw6p1_irq_bank_base[] =
{
	SUNXI_PIO_BANK_BASE(PA_BASE, 0),
	SUNXI_PIO_BANK_BASE(PB_BASE, 1),
	SUNXI_PIO_BANK_BASE(PC_BASE, 2),
	SUNXI_PIO_BANK_BASE(PD_BASE, 3),
	SUNXI_PIO_BANK_BASE(PE_BASE, 4),
	SUNXI_PIO_BANK_BASE(PF_BASE, 5),
	SUNXI_PIO_BANK_BASE(PG_BASE, 6),
	SUNXI_PIO_BANK_BASE(PH_BASE, 7),
	SUNXI_PIO_BANK_BASE(PI_BASE, 8),
	SUNXI_PIO_BANK_BASE(PJ_BASE, 9),
	SUNXI_PIO_BANK_BASE(PK_BASE, 10),
};

static const unsigned int sun55iw6p1_bank_base[] =
{
	SUNXI_PIO_BANK_BASE(PA_BASE, 0),
	SUNXI_PIO_BANK_BASE(PB_BASE, 0),
	SUNXI_PIO_BANK_BASE(PC_BASE, 0),
	SUNXI_PIO_BANK_BASE(PD_BASE, 0),
	SUNXI_PIO_BANK_BASE(PE_BASE, 0),
	SUNXI_PIO_BANK_BASE(PF_BASE, 0),
	SUNXI_PIO_BANK_BASE(PG_BASE, 0),
	SUNXI_PIO_BANK_BASE(PH_BASE, 0),
	SUNXI_PIO_BANK_BASE(PI_BASE, 0),
	SUNXI_PIO_BANK_BASE(PJ_BASE, 0),
	SUNXI_PIO_BANK_BASE(PK_BASE, 0),
};

static const int sun55iw6p1_bank_irq_num[] =
{
	SUNXI_IRQ_GPIOA,
	SUNXI_IRQ_GPIOB,
	SUNXI_IRQ_GPIOC,
	SUNXI_IRQ_GPIOD,
	SUNXI_IRQ_GPIOE,
	SUNXI_IRQ_GPIOF,
	SUNXI_IRQ_GPIOG,
	SUNXI_IRQ_GPIOH,
	SUNXI_IRQ_GPIOI,
	SUNXI_IRQ_GPIOJ,
	SUNXI_IRQ_GPIOK,
};

static struct gpio_desc sun55iw6p1_gpio_desc =
{
    .membase = SUNXI_GPIO_PBASE,
    .resource_size = SUNXI_GPIO_RES_SIZE,
    .virq_offset = 0,
    .irq_arry_size = ARRAY_SIZE(sun55iw6p1_bank_irq_num),
    .irq = (const uint32_t *)sun55iw6p1_bank_irq_num,
    .pin_base = PA_BASE,
    .banks = ARRAY_SIZE(sun55iw6p1_bank_base),
    .bank_base = (const uint32_t *)sun55iw6p1_bank_base,
    .irq_banks = ARRAY_SIZE(sun55iw6p1_irq_bank_base),
    .irq_bank_base = (const uint32_t *)sun55iw6p1_irq_bank_base,
    .hw_type = SUNXI_PCTL_HW_TYPE_0,
};

static const unsigned sun55iw6p1_r_irq_bank_base[] =
{
    SUNXI_R_PIO_BANK_BASE(PL_BASE, 0),
    SUNXI_R_PIO_BANK_BASE(PM_BASE, 1),
};

static const unsigned sun55iw6p1_r_bank_base[] =
{
    SUNXI_PIO_BANK_BASE(PL_BASE, 0),
    SUNXI_PIO_BANK_BASE(PM_BASE, 0),
};

static const int sun55iw6p1_r_bank_irq_num[] =
{
    SUNXI_IRQ_R_GPIOL,
    SUNXI_IRQ_R_GPIOM,
};

static struct gpio_desc sun55iw6p1_r_gpio_desc =
{
    .membase = SUNXI_GPIO_R_PBASE,
    .resource_size = SUNXI_R_GPIO_RES_SIZE,
    .virq_offset = BANK_BOUNDARY,
    .irq_arry_size = ARRAY_SIZE(sun55iw6p1_r_bank_irq_num),
    .irq = (const uint32_t *)sun55iw6p1_r_bank_irq_num,
    .pin_base = PL_BASE,
    .banks = ARRAY_SIZE(sun55iw6p1_r_bank_base),
    .bank_base = (const uint32_t *)sun55iw6p1_r_bank_base,
    .irq_banks = ARRAY_SIZE(sun55iw6p1_r_irq_bank_base),
    .irq_bank_base = (const uint32_t *)sun55iw6p1_r_irq_bank_base,
    .hw_type = SUNXI_PCTL_HW_TYPE_1,
};

static const struct gpio_desc *platform_gpio_desc[] =
{
    &sun55iw6p1_gpio_desc,
    &sun55iw6p1_r_gpio_desc,
    NULL,
};

/*
 * Called by hal_gpio_init().
 */
const struct gpio_desc **gpio_get_platform_desc(void)
{
	return platform_gpio_desc;
}
