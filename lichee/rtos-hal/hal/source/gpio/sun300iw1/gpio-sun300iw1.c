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
#ifdef CONFIG_COMPONENTS_PM
#include <pm_wakesrc.h>
#endif
#ifdef CONFIG_PM_STANDBY_MEMORY
#include <pm_mem.h>
#endif

#ifdef CONFIG_PM_STANDBY_MEMORY
__standby_unsaved_data struct sunxi_gpio_hw_info sunxi_gpio_hw_info[SUNXI_PCTL_HW_TYPE_CNT] = {
#else
struct sunxi_gpio_hw_info sunxi_gpio_hw_info[SUNXI_PCTL_HW_TYPE_CNT] = {
#endif
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset		= 0x0,
		.data_regs_offset		= 0x10,
		.dlevel_regs_offset		= 0x14,
		.bank_mem_size			= 0x30,
		.pull_regs_offset		= 0x24,
		.dlevel_pins_per_reg	= 8,
		.dlevel_pins_bits		= 4,
		.dlevel_pins_mask		= 0xF,
		.irq_mux_val			= 0xE,
		.irq_cfg_reg			= 0x200,
		.irq_ctrl_reg			= 0x210,
		.irq_status_reg			= 0x214,
		.irq_debounce_reg		= 0x218,
		.irq_mem_base			= 0x200,
		.irq_mem_size			= 0x20,
		.irq_mem_used			= 0x20,
		.power_mode_val_reg		= 0x348,
		.pio_pow_ctrl_reg		= 0x350,
	},
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset		= 0x0,
		.data_regs_offset		= 0x10,
		.data_reg_irregular		= true,
		.data_mem_size			= 0x10,
		.bank_mem_size			= 0x30,
		.pull_regs_offset		= 0x24,
		.irq_mux_val			= 0xE,
		.irq_cfg_reg			= 0x40,
		.irq_ctrl_reg			= 0x50,
		.irq_status_reg			= 0x54,
		.irq_debounce_reg		= 0x58,
		.irq_mem_base			= 0x40,
		.irq_mem_size			= 0x20,
		.irq_mem_used			= 0x20,
		.power_mode_val_reg		= 0x348,
		.pio_pow_ctrl_reg		= 0x350,
	},
};

static const unsigned int sun300iw1p1_irq_bank_base[] =
{
	SUNXI_PIO_BANK_BASE(PA_BASE, 0),
	SUNXI_PIO_BANK_BASE(PC_BASE, 1),
	SUNXI_PIO_BANK_BASE(PD_BASE, 2),
};

static const unsigned int sun300iw1p1_bank_base[] =
{
	SUNXI_PIO_BANK_BASE(PA_BASE, 0),
	SUNXI_PIO_BANK_BASE(PC_BASE, 1),
	SUNXI_PIO_BANK_BASE(PD_BASE, 2),
};

static const int sun300iw1p1_bank_irq_num[] =
{
	SUNXI_IRQ_GPIOA,
	SUNXI_IRQ_GPIOC,
	SUNXI_IRQ_GPIOD,
};

#ifdef CONFIG_COMPONENTS_PM
static pm_wakesrc_t *sun300iw1p1_bank_wakesrc[ARRAY_SIZE(sun300iw1p1_bank_irq_num)];
#endif

static struct gpio_desc sun300iw1p1_gpio_desc =
{
    .membase = SUNXI_GPIO_PBASE,
    .resource_size = SUNXI_GPIO_RES_SIZE,
    .virq_offset = 0,
    .irq_arry_size = ARRAY_SIZE(sun300iw1p1_bank_irq_num),
    .irq = (const uint32_t *)sun300iw1p1_bank_irq_num,
    .pin_base = PA_BASE,
    .banks = ARRAY_SIZE(sun300iw1p1_bank_base),
    .bank_base = (const uint32_t *)sun300iw1p1_bank_base,
    .irq_banks = ARRAY_SIZE(sun300iw1p1_irq_bank_base),
    .irq_bank_base = (const uint32_t *)sun300iw1p1_irq_bank_base,
    .hw_type = SUNXI_PCTL_HW_TYPE_0,
#ifdef CONFIG_COMPONENTS_PM
    .ws = sun300iw1p1_bank_wakesrc,
#endif
};

static const unsigned sun300iw1p1_r_irq_bank_base[] =
{
    SUNXI_R_PIO_BANK_BASE(PL_BASE, 0),
};

static const unsigned sun300iw1p1_r_bank_base[] =
{
    SUNXI_R_PIO_BANK_BASE(PL_BASE, 0),
};

static const int sun300iw1p1_r_bank_irq_num[] =
{
    SUNXI_IRQ_R_GPIOL,
};

#ifdef CONFIG_COMPONENTS_PM
static pm_wakesrc_t *sun300iw1p1_r_bank_wakesrc[ARRAY_SIZE(sun300iw1p1_r_bank_irq_num)];
#endif

static struct gpio_desc sun300iw1p1_r_gpio_desc =
{
    .membase = SUNXI_GPIO_R_PBASE,
    .resource_size = SUNXI_R_GPIO_RES_SIZE,
    .virq_offset = BANK_BOUNDARY,
    .irq_arry_size = ARRAY_SIZE(sun300iw1p1_r_bank_irq_num),
    .irq = (const uint32_t *)sun300iw1p1_r_bank_irq_num,
    .pin_base = PL_BASE,
    .banks = ARRAY_SIZE(sun300iw1p1_r_bank_base),
    .bank_base = (const uint32_t *)sun300iw1p1_r_bank_base,
    .irq_banks = ARRAY_SIZE(sun300iw1p1_r_irq_bank_base),
    .irq_bank_base = (const uint32_t *)sun300iw1p1_r_irq_bank_base,
    .hw_type = SUNXI_PCTL_HW_TYPE_1,
#ifdef CONFIG_COMPONENTS_PM
    .ws = sun300iw1p1_r_bank_wakesrc,
#endif
};

static const struct gpio_desc *platform_gpio_desc[] =
{
    &sun300iw1p1_gpio_desc,
    &sun300iw1p1_r_gpio_desc,
    NULL,
};

/*
 * Called by hal_gpio_init().
 */
const struct gpio_desc **gpio_get_platform_desc(void)
{
	return platform_gpio_desc;
}
