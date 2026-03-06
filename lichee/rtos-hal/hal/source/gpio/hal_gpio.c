/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.
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
/* #include <init.h> */

#include <hal_mem.h>
#include <hal_gpio.h>
#include <hal_log.h>
#include <hal_cfg.h>
#include "gpio.h"
#include <sunxi_hal_common.h>
#include <interrupt.h>
#ifdef CONFIG_COMPONENTS_PM
#include <pm_wakesrc.h>
#include <pm_syscore.h>
#include <pm_debug.h>
#endif
#ifdef CONFIG_AMP_SHARE_IRQ
#include <openamp/openamp_share_irq.h>
#endif
#ifdef CONFIG_DRIVERS_GPIO_SHARE_IRQ
#include <hal_cpu.h>
#endif
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
#include "aw9523.h"
#endif

#include <hal_atomic.h>
#include <hal_mutex.h>

#ifdef CONFIG_ARCH_SUN55IW3
#define GPIO_FEATURE_SYSCONFIG_POW_MODSEL
#endif

#ifdef CONFIG_DRIVERS_GPIO_RECORD_USAGE
#include <aw_list.h>
#include <hal_cmd.h>
struct list_head used_gpio_list;
typedef struct
{
	int  pin_val;
	char port_group;
	int  port_num;
	int  mul_sel;
	int  pull;
	int  drv_level;
	uint32_t  irq_num;
	struct list_head node;
} used_gpio_info_t;

used_gpio_info_t* find_or_add_gpio_by_pinval(struct list_head *head, int val)
{
	struct list_head *pos;
	used_gpio_info_t *entry;

	list_for_each(pos, head) {
		entry = list_entry(pos, used_gpio_info_t, node);
		if (entry->pin_val == val) {
			return entry;
		}
	}

	used_gpio_info_t *gpio = (used_gpio_info_t *)malloc(sizeof(used_gpio_info_t));
	memset(gpio, 0, sizeof(used_gpio_info_t));
	gpio-> pin_val = val;
	list_add_tail(&gpio->node, head);

	return gpio;
}

#define MAX_PORT_NUM 15
void parse_pin_val(int pin_val, used_gpio_info_t *gpio)
{
	int i;
	char port_groups[MAX_PORT_NUM + 1] = "ABCDEFGHIJKLMNO";
	int port_base[MAX_PORT_NUM] = {
		PA_BASE, PB_BASE, PC_BASE, PD_BASE, PE_BASE, PF_BASE, PG_BASE, PH_BASE,
		PI_BASE, PJ_BASE, PK_BASE, PL_BASE, PM_BASE, PN_BASE, PO_BASE
	};
	int port_base_max = PO_BASE + 32;

	for (i = 0; i < (MAX_PORT_NUM -1); i++) {
		if (pin_val >= port_base[i] && pin_val < port_base[i + 1]) {
			gpio->port_group = port_groups[i];
			gpio->port_num = pin_val - port_base[i];
			break;
		}
	}

	if (pin_val >= port_base[MAX_PORT_NUM - 1] && pin_val < port_base_max) {
		gpio->port_group = port_groups[MAX_PORT_NUM - 1];
		gpio->port_num = pin_val - port_base[MAX_PORT_NUM - 1];
	}
}

void dump_used_gpio_list(void)
{
	struct list_head *pos;
	used_gpio_info_t *entry;

	printf("====== GPIO information in use ======\n");
	list_for_each(pos, &used_gpio_list) {
		entry = list_entry(pos, used_gpio_info_t, node);
		printf("[P%c%02d]: mul_sel:%d, pull:%d, drv:%d, irq:%d\n",
				entry->port_group, entry->port_num, entry->mul_sel,
				entry->pull, entry->drv_level, entry->irq_num);
	}
}

FINSH_FUNCTION_EXPORT_ALIAS(dump_used_gpio_list, hal_gpio_info, dump gpio used info);
#endif

static const struct gpio_desc **g_gpio_desc = NULL;

static hal_spinlock_t g_reg_spinlock[5];

/* spinlock for protecting mux, data, pull and drive capability reg */
static inline hal_spinlock_t *get_gpio_cfg_reg_spinlock(void)
{
	return &g_reg_spinlock[0];
}

static inline hal_spinlock_t *get_irq_trigger_reg_spinlock(void)
{
	return &g_reg_spinlock[1];
}

static inline hal_spinlock_t *get_irq_ctrl_reg_spinlock(void)
{
	return &g_reg_spinlock[2];
}

static inline hal_spinlock_t *get_debounce_reg_spinlock(void)
{
	return &g_reg_spinlock[3];
}

static inline hal_spinlock_t *get_desc_spinlock(void)
{
	return &g_reg_spinlock[4];
}

/*
 * The following inlines stuffs a configuration parameter and data value
 * into and out of an unsigned long argument, as used by the generic pin config
 * system. We put the parameter in the lower 8 bits and the argument in the
 * upper 24 bits.
 */

static inline pin_config_param_t pinconf_to_config_param(unsigned long config)
{
	return (pin_config_param_t)(config & 0xffUL);
}

static inline gpio_pin_t pinconf_to_config_argument(unsigned long config)
{
	return (uint32_t)((config >> 8) & 0xffffffUL);
}

static inline uint64_t pinconf_to_config_packed(pin_config_param_t param,
		unsigned long argument)
{
	return GPIO_CFG_PACK(param, argument);
}

/*
 * The sunXi PIO registers are organized as is:
 * 0x00 - 0x0c  Muxing values.
 *      8 pins per register, each pin having a 4bits value
 * 0x10     Pin values
 *      32 bits per register, each pin corresponding to one bit
 * 0x14 - 0x18  Drive level
 *      16 pins per register, each pin having a 2bits value
 * 0x1c - 0x20  Pull-Up values
 *      16 pins per register, each pin having a 2bits value
 *
 * This is for the first bank. Each bank will have the same layout,
 * with an offset being a multiple of 0x24.
 *
 * The following functions calculate from the pin number the register
 * and the bit offset that we should access.
 */

static struct gpio_desc *pin_to_gpio_desc(gpio_pin_t pin)
{
	if (pin < BANK_BOUNDARY) /* CPUX domain */
	{
		return (struct gpio_desc *)g_gpio_desc[0];
	}
	else /* CPUS domain */
	{
		return (struct gpio_desc *)g_gpio_desc[1];
	}
}

#ifdef SUNXI_GPIO_STANDARD
static inline u32 sunxi_pinctrl_recalc_offset(enum sunxi_gpio_hw_type hw_type, uint8_t bank)
{
	return bank * sunxi_gpio_hw_info[hw_type].bank_mem_size +
		sunxi_gpio_hw_info[hw_type].initial_bank_offset;
}
#endif

static inline uint32_t gpio_mux_reg(gpio_pin_t pin)
{
#ifdef SUNXI_GPIO_STANDARD
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);
#endif

	pin %= BANK_BOUNDARY;
	uint32_t bank = pin / PINS_PER_BANK;

#ifdef SUNXI_GPIO_STANDARD
	enum sunxi_gpio_hw_type hw_type = gpio_desc->hw_type;
	uint32_t offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
#else
	uint32_t offset = INITIAL_BANK_OFFSET + bank * BANK_MEM_SIZE;
#endif
	offset += MUX_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / MUX_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline uint32_t gpio_mux_offset(gpio_pin_t pin)
{
	gpio_pin_t pin_num = pin % MUX_PINS_PER_REG;
	return pin_num * MUX_PINS_BITS;
}

static inline uint32_t gpio_data_reg(gpio_pin_t pin)
{
#ifdef SUNXI_GPIO_STANDARD
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);
#endif

	pin %= BANK_BOUNDARY;
	uint32_t bank = pin / PINS_PER_BANK;
#ifdef SUNXI_GPIO_STANDARD
	enum sunxi_gpio_hw_type hw_type = gpio_desc->hw_type;
	uint32_t offset = sunxi_pinctrl_recalc_offset(hw_type, bank);

	if (sunxi_gpio_hw_info[hw_type].data_reg_irregular) {
		offset = sunxi_gpio_hw_info[hw_type].data_regs_offset +
			pin / PINS_PER_BANK * sunxi_gpio_hw_info[hw_type].data_mem_size;
	} else {
		offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
		offset += sunxi_gpio_hw_info[hw_type].data_regs_offset;
	}
#else
	uint32_t offset = INITIAL_BANK_OFFSET + bank * BANK_MEM_SIZE;
	offset += DATA_REGS_OFFSET;
#endif

	offset += pin % PINS_PER_BANK / DATA_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline uint32_t gpio_data_offset(gpio_pin_t pin)
{
	gpio_pin_t pin_num = pin % DATA_PINS_PER_REG;
	return pin_num * DATA_PINS_BITS;
}

static inline uint32_t gpio_dlevel_reg(gpio_pin_t pin)
{
#ifdef SUNXI_GPIO_STANDARD
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);
#endif

	pin %= BANK_BOUNDARY;
	uint32_t bank = pin / PINS_PER_BANK;
#ifdef SUNXI_GPIO_STANDARD
	enum sunxi_gpio_hw_type hw_type = gpio_desc->hw_type;
	uint32_t offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
#else
	uint32_t offset = INITIAL_BANK_OFFSET + bank * BANK_MEM_SIZE;
#endif
	offset += DLEVEL_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / DLEVEL_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline uint32_t gpio_dlevel_offset(gpio_pin_t pin)
{
	gpio_pin_t pin_num = pin % DLEVEL_PINS_PER_REG;
	return pin_num * DLEVEL_PINS_BITS;
}

static inline uint32_t gpio_pull_reg(gpio_pin_t pin)
{
#ifdef SUNXI_GPIO_STANDARD
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);
#endif

	pin %= BANK_BOUNDARY;
	uint32_t bank = pin / PINS_PER_BANK;
#ifdef SUNXI_GPIO_STANDARD
	enum sunxi_gpio_hw_type hw_type = gpio_desc->hw_type;
	uint32_t offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
	offset += sunxi_gpio_hw_info[hw_type].pull_regs_offset;
#else
	uint32_t offset = INITIAL_BANK_OFFSET + bank * BANK_MEM_SIZE;
	offset += PULL_REGS_OFFSET;
#endif
	offset += pin % PINS_PER_BANK / PULL_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline uint32_t gpio_pull_offset(gpio_pin_t pin)
{
	gpio_pin_t pin_num = pin % PULL_PINS_PER_REG;
	return pin_num * PULL_PINS_BITS;
}

static inline uint32_t gpio_irq_ctrl_reg_from_bank(struct gpio_desc *gpio_desc, u8 bank, unsigned bank_base)
{
#ifdef SUNXI_GPIO_STANDARD
	enum sunxi_gpio_hw_type hw_type = gpio_desc->hw_type;

	return sunxi_gpio_hw_info[hw_type].irq_ctrl_reg + (bank_base + bank) * sunxi_gpio_hw_info[hw_type].irq_mem_size;
#else
	return IRQ_CTRL_REG + (bank_base + bank) * IRQ_MEM_SIZE;
#endif
}

static inline uint32_t gpio_irq_ctrl_reg(struct gpio_desc *gpio_desc, uint32_t irq, unsigned bank_base)
{
	uint32_t bank = irq / IRQ_PER_BANK;

	return gpio_irq_ctrl_reg_from_bank(gpio_desc, bank, bank_base);
}

static inline uint32_t gpio_irq_ctrl_offset(uint32_t irq)
{
	uint32_t offset = irq % IRQ_CTRL_IRQ_PER_REG;
	return offset * IRQ_CTRL_IRQ_BITS;
}

static inline uint32_t gpio_get_pin_base_from_bank(u8 bank, unsigned bank_base)
{
	return (bank_base + bank) * IRQ_PER_BANK;
}

static inline uint32_t gpio_irq_status_reg_from_bank(struct gpio_desc *gpio_desc, u8 bank, unsigned bank_base)
{
#ifdef SUNXI_GPIO_STANDARD
	enum sunxi_gpio_hw_type hw_type = gpio_desc->hw_type;
	return sunxi_gpio_hw_info[hw_type].irq_status_reg +
		(bank_base + bank) * sunxi_gpio_hw_info[hw_type].irq_mem_size;
#else
	return IRQ_STATUS_REG + (bank_base + bank) * IRQ_MEM_SIZE;
#endif
}

static inline uint32_t gpio_irq_status_reg(struct gpio_desc *gpio_desc, uint32_t irq, unsigned bank_base)
{
	uint32_t bank = irq / IRQ_PER_BANK;

	return gpio_irq_status_reg_from_bank(gpio_desc, bank, bank_base);
}

static inline uint32_t gpio_irq_debounce_reg(struct gpio_desc *gpio_desc, uint32_t irq, unsigned bank_base)
{
	uint32_t bank = irq / IRQ_PER_BANK;

#ifdef SUNXI_GPIO_STANDARD
	enum sunxi_gpio_hw_type hw_type = gpio_desc->hw_type;

	return sunxi_gpio_hw_info[hw_type].irq_debounce_reg +
		(bank_base + bank) * sunxi_gpio_hw_info[hw_type].irq_mem_size;
#else
	return IRQ_DEBOUNCE_REG + (bank_base + bank) * IRQ_MEM_SIZE;
#endif
}

static inline uint32_t gpio_irq_status_offset(uint32_t irq)
{
	uint32_t index = irq % IRQ_STATUS_IRQ_PER_REG;
	return index * IRQ_STATUS_IRQ_BITS;
}

static inline uint32_t gpio_irq_cfg_reg(struct gpio_desc *gpio_desc, uint32_t irq, unsigned bank_base)
{
	uint32_t bank = irq / IRQ_PER_BANK;
	uint32_t reg = (irq % IRQ_PER_BANK) / IRQ_CFG_IRQ_PER_REG * 0x04;

#ifdef SUNXI_GPIO_STANDARD
	enum sunxi_gpio_hw_type hw_type = gpio_desc->hw_type;

	return sunxi_gpio_hw_info[hw_type].irq_cfg_reg +
		(bank_base + bank) * sunxi_gpio_hw_info[hw_type].irq_mem_size + reg;
#endif
	return IRQ_CFG_REG + (bank_base + bank) * IRQ_MEM_SIZE + reg;
}

static inline uint32_t gpio_irq_cfg_offset(uint32_t irq)
{
	uint32_t index = irq % IRQ_CFG_IRQ_PER_REG;
	return index * IRQ_CFG_IRQ_BITS;
}

static int gpio_pconf_reg(gpio_pin_t pin, pin_config_param_t param,
		uint32_t *offset, uint32_t *shift, uint32_t *mask)
{
	switch (param)
	{
		case GPIO_TYPE_DRV:
			*offset = gpio_dlevel_reg(pin);
			*shift = gpio_dlevel_offset(pin);
			*mask = DLEVEL_PINS_MASK;
			break;

		case GPIO_TYPE_PUD:
			*offset = gpio_pull_reg(pin);
			*shift = gpio_pull_offset(pin);
			*mask = PULL_PINS_MASK;
			break;

		case GPIO_TYPE_DAT:
			*offset = gpio_data_reg(pin);
			*shift = gpio_data_offset(pin);
			*mask = DATA_PINS_MASK;
			break;

		case GPIO_TYPE_FUNC:
			*offset = gpio_mux_reg(pin);
			*shift = gpio_mux_offset(pin);
			*mask = MUX_PINS_MASK;
			break;

		default:
			GPIO_ERR("Invalid mux type");
			return -1;
	}
	return 0;
}

#if defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
static uint32_t *gpio_banks_irq_map = NULL;

#ifndef CONFIG_COMPONENTS_OPENAMP
int sysconfig_gpio_map_init(const struct gpio_desc **g_gpio_desc)
{
	int i;
	int ret;
	int banks = 0;
	struct gpio_desc *gpio_desc = NULL;

	for (i = 0; g_gpio_desc[i] != NULL; i++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		banks += gpio_desc->banks;
	}

	gpio_banks_irq_map = hal_malloc(banks * sizeof(uint32_t));
	if( !gpio_banks_irq_map ) {
		return -1;
	}
	memset(gpio_banks_irq_map, 0, banks * sizeof(uint32_t));

	for(i = 0; i < banks; i++) {
		int num;
		for(num = 0; num < 2; num++) {
			int32_t config_value = 0;
			uint32_t init_value = 0;
			char name[32] = {0};
			int k = num * 16;
			snprintf(name, sizeof(name) - 1, "gpio_%c_%c", '1' + i, '1' + num);
			ret = hal_cfg_get_keyvalue("gpio_irq_map", name, &config_value, 1);
			if (!ret) {
				int j;
				for(j = 0; j < 32; j += 2) {
					uint8_t id = (config_value >> j) & 0x3;
					if (id == (hal_cpu_get_id() + 1)) {
						init_value |= (1 << k);
					}
					k++;
				}
				*(gpio_banks_irq_map + i) |= init_value;
			}
		}
	}
	return 0;
}
#else
static int irq2bank(uint32_t irq)
{
	switch (irq) {
#ifdef SUNXI_IRQ_GPIOA
	case SUNXI_IRQ_GPIOA: return 'A' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOB
	case SUNXI_IRQ_GPIOB: return 'B' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOC
	case SUNXI_IRQ_GPIOC: return 'C' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOD
	case SUNXI_IRQ_GPIOD: return 'D' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOE
	case SUNXI_IRQ_GPIOE: return 'E' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOF
	case SUNXI_IRQ_GPIOF: return 'F' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOG
	case SUNXI_IRQ_GPIOG: return 'G' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOH
	case SUNXI_IRQ_GPIOH: return 'H' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOI
	case SUNXI_IRQ_GPIOI: return 'I' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOJ
	case SUNXI_IRQ_GPIOJ: return 'J' - 'A';
#endif
#ifdef SUNXI_IRQ_GPIOK
	case SUNXI_IRQ_GPIOK: return 'K' - 'A';
#endif
#ifdef SUNXI_IRQ_R_GPIOL
	case SUNXI_IRQ_R_GPIOL: return 'L' - 'A';
#endif
#ifdef SUNXI_IRQ_R_GPIOM
	case SUNXI_IRQ_R_GPIOM: return 'M' - 'A';
#endif
#ifdef SUNXI_IRQ_R_GPION
	case SUNXI_IRQ_R_GPION: return 'N' - 'A';
#endif
	default: return -1;
	}
}

int sysconfig_gpio_map_init(const struct gpio_desc *g_gpio_desc[])
{
	int i, b, idx;
	int ret;
	int banks = 0;
	const struct gpio_desc *gpio_desc = NULL;

	for (i = 0; g_gpio_desc[i] != NULL; i++)
		banks += g_gpio_desc[i]->banks;

	gpio_banks_irq_map = hal_malloc(banks * sizeof(uint32_t));
	if(!gpio_banks_irq_map) {
		return -1;
	}
	memset(gpio_banks_irq_map, 0, banks * sizeof(uint32_t));

	idx = 0;
	for(b = 0; g_gpio_desc[b] != NULL; b++) {
		gpio_desc = g_gpio_desc[b];
		for(i = 0; i < gpio_desc->irq_arry_size; i++) {
			int32_t config_value = 0;
			char name[32] = {0};

			ret = irq2bank(gpio_desc->irq[i]);
			if (ret < 0)
				continue;

			snprintf(name, sizeof(name) - 1, "GPIO%c", ret + 'A');
			ret = hal_cfg_get_keyvalue("gpio_irq", name, &config_value, 1);
			if (ret)
				continue;
			/* currently, we only support unit of gpio bank */
			if (config_value != 0)
				*(gpio_banks_irq_map + idx + i) = 0xFFFFFFFF;
		}
		idx += gpio_desc->banks;
	}
	return 0;
}
#endif

int sysconfig_gpio_map_deinit(void)
{
	hal_free(gpio_banks_irq_map);
	gpio_banks_irq_map = NULL;
	return 0;
}

int sunxi_get_banks_mask(uint32_t hw_irq)
{
	int i;
	int j;
	int pos = 0;
	struct gpio_desc *gpio_desc = NULL;

	for (i = 0; g_gpio_desc[i] != NULL; i++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		for(j = 0; j < gpio_desc->irq_arry_size; j++) {
			if (gpio_desc->irq[j] == hw_irq) {
				goto out;
			}
			pos++;
		}
	}
out:
	return *(gpio_banks_irq_map + pos);
}
#endif

static uint32_t count_gpio_bank_mask(void)
{
	uint32_t max_bank = (uint32_t)GPIO_MAX_BANK;
	uint32_t mask = 0;
	do
	{
		mask |= 1 << (max_bank / PINS_PER_BANK);
		max_bank -= PINS_PER_BANK;
		if (max_bank == 0)
		{
			mask |= 1;
		}
	} while (max_bank);
	return mask;
}


static struct gpio_desc *irq_to_gpio_desc(uint32_t irq)
{
	int i, j;
	struct gpio_desc *gpio_desc;
	for (i = 0; g_gpio_desc[i] != NULL; i++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		for (j = 0; j < gpio_desc->irq_arry_size; j++)
		{
			if (gpio_desc->irq[j] == irq)
			{
				return gpio_desc;
			}
		}
	}
	GPIO_ERR("gpio to irq error!");
	return NULL;
}

static struct gpio_desc *virq_to_gpio_desc(uint32_t irq)
{
	int i, j;
	struct gpio_desc *gpio_desc;
	for (i = 0; g_gpio_desc[i] != NULL; i++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		for (j = 0; j < gpio_desc->irq_banks * IRQ_PER_BANK; j++)
		{
			if (gpio_desc->irq_desc[j].virq == irq)
			{
				return gpio_desc;
			}
		}
	}
	GPIO_ERR("gpio to virq error!");
	return NULL;
}

static void gpio_irq_ack(struct gpio_desc *gpio_desc, int i)
{
	struct gpio_irq_desc *dirq = &gpio_desc->irq_desc[i];
	uint32_t hw_irq = dirq->virq - gpio_desc->virq_offset - GPIO_IRQ_START;
	unsigned bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	uint32_t reg = gpio_irq_status_reg(gpio_desc, hw_irq, bank_base);
	uint32_t status_idx = gpio_irq_status_offset(hw_irq);

	/* clear the pending */
	hal_writel(1 << status_idx, gpio_desc->membase + reg);
}

static hal_irqreturn_t bad_gpio_irq_handle(void *data)
{
	GPIO_INFO("No irq registered handler for this calling !!");
	return 0;
}

static void gpio_irq_set_type(struct gpio_desc *gpio_desc, int irq_num, unsigned long type)
{
	unsigned long irq_flags;
	struct gpio_irq_desc *dirq = &gpio_desc->irq_desc[irq_num];
	uint32_t hw_irq = dirq->virq - gpio_desc->virq_offset - GPIO_IRQ_START;
	unsigned bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	uint32_t reg = gpio_irq_cfg_reg(gpio_desc, hw_irq, bank_base);
	uint32_t index = gpio_irq_cfg_offset(hw_irq);
	uint32_t mode, regval;

	switch (type)
	{
		case IRQ_TYPE_EDGE_RISING:
			mode = IRQ_EDGE_RISING;
			break;
		case IRQ_TYPE_EDGE_FALLING:
			mode = IRQ_EDGE_FALLING;
			break;
		case IRQ_TYPE_EDGE_BOTH:
			mode = IRQ_EDGE_BOTH;
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			mode = IRQ_LEVEL_HIGH;
			break;
		case IRQ_TYPE_LEVEL_LOW:
			mode = IRQ_LEVEL_LOW;
			break;
		default:
			mode = IRQ_EDGE_RISING;
			break;
	}

	irq_flags = hal_spin_lock_irqsave(get_irq_trigger_reg_spinlock());

	regval = hal_readl(gpio_desc->membase + reg);
	regval &= ~(IRQ_CFG_IRQ_MASK << index);

	hal_writel(regval | (mode << index), gpio_desc->membase + reg);

	hal_spin_unlock_irqrestore(get_irq_trigger_reg_spinlock(), irq_flags);

	//regval = hal_readl(gpio_desc->membase + reg);
	//GPIO_ERR("gpio_desc->membase + reg: 0x%x\n", regval);

}

static hal_irqreturn_t gpio_irq_handle(void *data)
{
	uint32_t hwirq = *((uint32_t *)data);
	uint32_t bank, reg, val, base_bank;
	struct gpio_desc *gpio_desc = irq_to_gpio_desc(hwirq);
	uint32_t irq_ctrl_reg, irq_ctrl_val;

	if (gpio_desc == NULL)
	{
		return 0;
	}

	for (bank = 0; bank < gpio_desc->irq_banks; bank ++)
	{
		if (hwirq == gpio_desc->irq[bank])
		{
			break;
		}
	}

	if (bank == gpio_desc->irq_banks)
	{
		return 0;
	}

	base_bank = gpio_desc->irq_bank_base[bank];
	reg = gpio_irq_status_reg_from_bank(gpio_desc, bank, base_bank);
	irq_ctrl_reg = gpio_irq_ctrl_reg_from_bank(gpio_desc, bank, base_bank);
#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
	uint32_t banks_mask = sunxi_get_banks_mask(gpio_desc->irq[bank]);
	val = hal_readl(gpio_desc->membase + reg) & banks_mask;
#else
	val = hal_readl(gpio_desc->membase + reg);
#endif
	irq_ctrl_val = hal_readl(gpio_desc->membase + irq_ctrl_reg);
	GPIO_INFO("hwirq = %d, gpio_desc address is 0x%lx.", hwirq, gpio_desc->membase);
	GPIO_INFO("base_bank is %d, hwirq is %d, val is %d.", base_bank, hwirq, val);
	if (val & irq_ctrl_val)
	{
		uint32_t irqoffset;
		uint32_t irq_pin;
		int i;
		for (irqoffset = 0; irqoffset < IRQ_PER_BANK; irqoffset++)
		{
			if ((1 << irqoffset) & val)
			{
				break;
			}
		}

		if (irqoffset >= IRQ_PER_BANK)
		{
			GPIO_INFO("return");
			return 0;
		}
		irq_pin = ((base_bank + bank) * IRQ_PER_BANK) + irqoffset + gpio_desc->virq_offset;

		for (i = 0; i < gpio_desc->irq_desc_size; i++)
		{
			if (irq_pin == gpio_desc->irq_desc[i].pin)
			{
				break;
			}
		}
		if (i >= gpio_desc->irq_desc_size)
		{
			return 0;
		}
		gpio_desc->irq_desc[i].handle_irq(gpio_desc->irq_desc[i].data);
		gpio_irq_ack(gpio_desc, i);
	}
#ifdef CONFIG_COMPONENTS_PM
	pm_wakesrc_relax(gpio_desc->ws[bank], PM_RELAX_WAKEUP);
#endif
	return 0;
}

bool hal_gpio_check_valid(gpio_pin_t pin)
{
	uint32_t bank = pin / PINS_PER_BANK;
	uint32_t mask = count_gpio_bank_mask();
	if (!((1 << bank) & mask))
	{
		return false;
	}
	return true;
}

static int gpio_conf_set(gpio_pin_t pin, unsigned long *gpio_config)
{
	unsigned long irq_flags;
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);
	if (gpio_desc == NULL)
	{
		GPIO_ERR("gpio_desc is not inited");
		return -1;
	}
	unsigned long config = (unsigned long)gpio_config;
	uint32_t offset, shift, mask, reg;
	uint32_t arg;
	pin_config_param_t param;
	int ret;

	param = pinconf_to_config_param(config);
	arg = pinconf_to_config_argument(config);

	ret = gpio_pconf_reg(pin, param, &offset, &shift, &mask);
	if (ret < 0)
	{
		GPIO_ERR("can't get reg for pin %u", pin);
		return -1;
	}

	irq_flags = hal_spin_lock_irqsave(get_gpio_cfg_reg_spinlock());

	reg = hal_readl(gpio_desc->membase + offset);
	reg &= ~(mask << shift);
	hal_writel(reg | arg << shift, gpio_desc->membase + offset);

	hal_spin_unlock_irqrestore(get_gpio_cfg_reg_spinlock(), irq_flags);
	return 0;
}

static int gpio_conf_get(gpio_pin_t pin, unsigned long *gpio_config)
{
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);
	if (gpio_desc == NULL)
	{
		GPIO_ERR("gpio_desc is not inited");
		return -1;
	}
	uint32_t offset, shift, mask;
	uint32_t arg, val;
	pin_config_param_t param = pinconf_to_config_param(*gpio_config);
	int ret = 0;

	ret = gpio_pconf_reg(pin, param, &offset, &shift, &mask);
	if (ret < 0)
	{
		GPIO_ERR("can't get reg for pin %u", pin);
		return -1;
	}

	val = (hal_readl(gpio_desc->membase + offset) >> shift) & mask;
	switch (param)
	{
		case GPIO_TYPE_DRV:
		case GPIO_TYPE_DAT:
		case GPIO_TYPE_PUD:
		case GPIO_TYPE_FUNC:
			arg = val;
			break;
		default:
			ret = -1;
			GPIO_ERR("Invalid mux type");
			return -1;
	}
	if (!ret)
	{
		*gpio_config = pinconf_to_config_packed(param, arg);
	}
	return ret;
}

int hal_gpio_get_data(gpio_pin_t pin, gpio_data_t *data)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return aw9523_gpio_get_data(pin, data);
	} else {
#endif
	if (NULL == data)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}

	config = GPIO_CFG_PACK(GPIO_TYPE_DAT, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*data = GPIO_CFG_UNPACK_VALUE(config);

	return ret;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	}
#endif
}

int hal_gpio_set_data(gpio_pin_t pin, gpio_data_t data)
{
	unsigned long config;
	int ret = 0;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return aw9523_gpio_set_data(pin, data);
	} else {
#endif
	config = GPIO_CFG_PACK(GPIO_TYPE_DAT, data);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set conf error!");
		return ret;
	}
	return ret;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	}
#endif
}

int hal_gpio_set_direction(gpio_pin_t pin, gpio_direction_t direction)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return aw9523_gpio_set_direction(pin, direction);
	} else {
#endif
	config = GPIO_CFG_PACK(GPIO_TYPE_FUNC, direction);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set conf error!");
		return ret;
	}
	return ret;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	}
#endif
}

int hal_gpio_get_direction(gpio_pin_t pin, gpio_direction_t *direction)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return aw9523_gpio_get_direction(pin, direction);
	} else {
#endif
	if (NULL == direction)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}
	config = GPIO_CFG_PACK(GPIO_TYPE_FUNC, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*direction = GPIO_CFG_UNPACK_VALUE(config);

	return ret;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	}
#endif
}

int hal_gpio_set_pull(gpio_pin_t pin, gpio_pull_status_t pull)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return 0;
	}
#endif

#ifdef CONFIG_DRIVERS_GPIO_RECORD_USAGE
	used_gpio_info_t *gpio_info;
	gpio_info = find_or_add_gpio_by_pinval(&used_gpio_list, pin);
	gpio_info->pull = pull;
#endif
	config = GPIO_CFG_PACK(GPIO_TYPE_PUD, pull);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set conf error!");
		return ret;
	}
	return ret;
}

int hal_gpio_get_pull(gpio_pin_t pin, gpio_pull_status_t *pull)
{
	unsigned long config;
	int ret = 0;

	if (NULL == pull)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		*pull = GPIO_PULL_DOWN_DISABLED;
		return 0;
	}
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_PUD, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*pull = GPIO_CFG_UNPACK_VALUE(config);

	return ret;

}

int hal_gpio_set_driving_level(gpio_pin_t pin, gpio_driving_level_t level)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return 0;
	}
#endif

#ifdef CONFIG_DRIVERS_GPIO_RECORD_USAGE
	used_gpio_info_t *gpio_info;
	gpio_info = find_or_add_gpio_by_pinval(&used_gpio_list, pin);
	gpio_info->drv_level = level;
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_DRV, level);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set conf error!");
		return ret;
	}
	return ret;
}

int hal_gpio_get_driving_level(gpio_pin_t pin, gpio_driving_level_t *level)
{
	unsigned long config;
	int ret = 0;

	if (NULL == level)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		*level = GPIO_DRIVING_LEVEL3;
		return 0;
	}
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_DRV, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*level = GPIO_CFG_UNPACK_VALUE(config);

	return ret;
}

int hal_gpio_pinmux_set_function(gpio_pin_t pin, gpio_muxsel_t function_index)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		if (function_index == GPIO_MUXSEL_OUT) {
			ret = aw9523_gpio_set_direction(pin, GPIO_DIRECTION_OUTPUT);
		} else if (function_index == GPIO_MUXSEL_IN) {
			ret = aw9523_gpio_set_direction(pin, GPIO_DIRECTION_INPUT);
		} else {
			GPIO_ERR("set ex pin mux error!");
			ret = -1;
		}
		return ret;
	}
#endif

#ifdef CONFIG_DRIVERS_GPIO_RECORD_USAGE
	used_gpio_info_t *gpio_info;
	gpio_info = find_or_add_gpio_by_pinval(&used_gpio_list, pin);
	parse_pin_val(pin, gpio_info);
	gpio_info->mul_sel = function_index;
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_FUNC, function_index);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set pin mux error!");
		return ret;
	}
	return ret;
}

int hal_gpio_pinmux_get_function(gpio_pin_t pin, gpio_muxsel_t *function_index)
{
	unsigned long config;
	int ret = 0;

	if (NULL == function_index)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		gpio_direction_t dir = GPIO_DIRECTION_OUTPUT;

		if ((ret = aw9523_gpio_get_direction(pin, &dir))) {
			GPIO_ERR("get ex pin mux error!");
			return ret;
		}

		*function_index = (dir == GPIO_DIRECTION_OUTPUT) ? GPIO_MUXSEL_OUT : GPIO_MUXSEL_IN;
		return 0;
	}
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_FUNC, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*function_index = GPIO_CFG_UNPACK_VALUE(config);

	return ret;
}

/*
 * 1.8 -> 3.3:
 *	1. Increase withstand
 *	2. Increase power voltage
 * 3.3 -> 1.8:
 *	1. Decrease power voltage
 *	2. Decrease withstand
 */
static void hal_gpio_power_switch_pf(struct gpio_desc *gpio_desc, u32 tar_vol, u32 pow_sel)
{
	u32 cur_vol;

	cur_vol = hal_readl(gpio_desc->membase + POWER_VOL_SEL) & BIT(0);
	tar_vol &= BIT(0);

	if (cur_vol < tar_vol) {
		hal_writel(pow_sel, gpio_desc->membase + POWER_MODE_SEL);
		hal_writel(tar_vol, gpio_desc->membase + POWER_VOL_SEL);
	} else if (cur_vol > tar_vol) {
		hal_writel(tar_vol, gpio_desc->membase + POWER_VOL_SEL);
		hal_writel(pow_sel, gpio_desc->membase + POWER_MODE_SEL);
	} else {
		hal_writel(pow_sel, gpio_desc->membase + POWER_MODE_SEL);
	}
}

static int hal_gpio_sel_vol_bank_mode(uint32_t bank, gpio_power_mode_t pm_sel)
{
	uint32_t temp;
	struct gpio_desc *gpio_desc;

	gpio_desc = pin_to_gpio_desc(bank * PINS_PER_BANK);
	if (gpio_desc == NULL)
	{
		return -1;
	}

	temp = hal_readl(gpio_desc->membase + POWER_MODE_SEL);
	temp &= ~(1 << bank);
	temp |= (pm_sel << bank);

	if (bank == 5) {
	/*
	 * In general platforms, the withstand voltage configuration of PF is consistent with other banks,
	 * PF is opposite to other banks in sun55iw3 platform
	*/
#ifdef CONFIG_ARCH_SUN55IW3
		hal_gpio_power_switch_pf(gpio_desc, pm_sel, temp);
#else
		hal_gpio_power_switch_pf(gpio_desc, ~pm_sel, temp);
#endif
	} else {
		hal_writel(temp, gpio_desc->membase + POWER_MODE_SEL);
	}

	return 0;
}

int hal_gpio_sel_vol_mode(gpio_pin_t pin, gpio_power_mode_t pm_sel)
{
	uint32_t bank;
	struct gpio_desc *gpio_desc;

	gpio_desc = pin_to_gpio_desc(pin);

	if (gpio_desc == NULL)
	{
		return -1;
	}

	bank = (pin - gpio_desc->pin_base) / PINS_PER_BANK;
	hal_gpio_sel_vol_bank_mode(bank, pm_sel);

	return 0;
}

int hal_gpio_set_debounce(gpio_pin_t pin, unsigned value)
{
	unsigned long irq_flags;
	uint32_t irq, hw_irq, reg, reg_val;
	struct gpio_desc *gpio_desc;
	unsigned bank_base;
	unsigned int val_clk_select, val_clk_per_scale;
	int ret = 0;

	gpio_desc = pin_to_gpio_desc(pin);

	if (gpio_desc == NULL)
	{
		return -1;
	}

	ret = hal_gpio_to_irq(pin, &irq);

	if (ret < 0)
	{
		GPIO_ERR("gpio to irq error");
		return -1;
	}

	hw_irq = irq - gpio_desc->virq_offset - GPIO_IRQ_START;
	bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	reg = gpio_irq_debounce_reg(gpio_desc, hw_irq, bank_base);

	irq_flags = hal_spin_lock_irqsave(get_debounce_reg_spinlock());

	reg_val = hal_readl(gpio_desc->membase + reg);
	val_clk_select = value & 1;
	val_clk_per_scale = (value >> 4) & 0x07;

	/*set debounce pio interrupt clock select */
	reg_val &= ~(1 << 0);
	reg_val |= val_clk_select;

	/* set debounce clock pre scale */
	reg_val &= ~(7 << 4);
	reg_val |= val_clk_per_scale << 4;
	hal_writel(reg_val, gpio_desc->membase + reg);

	hal_spin_unlock_irqrestore(get_debounce_reg_spinlock(), irq_flags);

	return 0;
}


int hal_gpio_to_irq(gpio_pin_t pin, uint32_t *irq)
{
	int i = 0;
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);

	for (i = 0; i < gpio_desc->irq_banks * IRQ_PER_BANK; i++)
	{
		if (pin != gpio_desc->irq_desc[i].pin)
		{
			continue;
		}
		GPIO_INFO("gpio %u to irq %d succeed!", pin, gpio_desc->irq_desc[i].virq);
		*irq = gpio_desc->irq_desc[i].virq;
#ifdef CONFIG_DRIVERS_GPIO_RECORD_USAGE
		used_gpio_info_t *gpio_info;
		gpio_info = find_or_add_gpio_by_pinval(&used_gpio_list, pin);
		gpio_info->irq_num = *irq;
#endif
		return 0;
	}

	return -1;
}


int hal_gpio_irq_request(uint32_t irq, hal_irq_handler_t hdle, unsigned long flags, void *data)
{
	struct gpio_desc *gpio_desc = virq_to_gpio_desc(irq);
	GPIO_INFO("[%s]gpio_desc address is 0x%lx.", __func__, gpio_desc->membase);
	int irq_max_num = gpio_desc->irq_desc_size + GPIO_IRQ_START;
	int ret = 0;
	unsigned long irq_flags;
#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
	uint32_t hw_irq = irq - gpio_desc->virq_offset - GPIO_IRQ_START;
	uint32_t bank_mask;
#endif
	irq -= gpio_desc->virq_offset;

#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
	bank_mask = sunxi_get_banks_mask(gpio_desc->irq[hw_irq / IRQ_PER_BANK]);
	bank_mask &= (1 << (hw_irq % IRQ_PER_BANK));

	if (!bank_mask) {
		GPIO_ERR("irq%d not belong to this chip,hwirq %d, mask=0x%x\n", irq,
						gpio_desc->irq[hw_irq / IRQ_PER_BANK],
						(uint32_t)sunxi_get_banks_mask(gpio_desc->irq[hw_irq / IRQ_PER_BANK]));
		return -1;
	}
#endif
	if (irq >= GPIO_IRQ_START && irq < irq_max_num)
	{
		if (hdle && gpio_desc->irq_desc[irq - GPIO_IRQ_START].handle_irq == bad_gpio_irq_handle)
		{
			irq_flags = hal_spin_lock_irqsave(get_desc_spinlock());
			gpio_desc->irq_desc[irq - GPIO_IRQ_START].handle_irq = hdle;
			gpio_desc->irq_desc[irq - GPIO_IRQ_START].flags = flags;
			gpio_desc->irq_desc[irq - GPIO_IRQ_START].data = data;
			hal_spin_unlock_irqrestore(get_desc_spinlock(), irq_flags);
		}
		/*set irq tpye*/
		gpio_irq_set_type(gpio_desc, irq - GPIO_IRQ_START, flags);

		/*set pin mux*/
		ret = hal_gpio_pinmux_set_function(gpio_desc->irq_desc[irq - GPIO_IRQ_START].pin, GPIO_MUXSEL_EINT);

		if (ret < 0)
		{
			GPIO_ERR("set pin mux error!");
			return -1;
		}
		GPIO_INFO("request irq %d succeed!", irq);
		return irq;
	}

	GPIO_ERR("Wrong irq NO.(%u) to request !!", (unsigned int)irq);
	return -1;
}

int hal_gpio_irq_free(uint32_t irq)
{
	struct gpio_desc *gpio_desc = virq_to_gpio_desc(irq);
	int irq_max_num = gpio_desc->irq_desc_size + GPIO_IRQ_START;
	unsigned long irq_flags;
	irq -= gpio_desc->virq_offset;
	if (irq >= GPIO_IRQ_START && irq < irq_max_num)
	{
		irq_flags = hal_spin_lock_irqsave(get_desc_spinlock());
		gpio_desc->irq_desc[irq - GPIO_IRQ_START].handle_irq = bad_gpio_irq_handle;
		gpio_desc->irq_desc[irq - GPIO_IRQ_START].flags = 0;
		gpio_desc->irq_desc[irq - GPIO_IRQ_START].data = NULL;
		hal_spin_unlock_irqrestore(get_desc_spinlock(), irq_flags);
		GPIO_INFO("free irq %d succeed!", irq);
		return irq;
	}

	GPIO_ERR("Wrong irq NO.(%u) to free !!", (unsigned int)irq);
	return -1;
}

int hal_gpio_irq_enable(uint32_t irq)
{
	unsigned long irq_flags;
	struct gpio_desc *gpio_desc = virq_to_gpio_desc(irq);
	GPIO_INFO("[%s]gpio_desc address is 0x%lx.", __func__, gpio_desc->membase);
	int irq_max_num = gpio_desc->irq_desc_size + GPIO_IRQ_START;
	uint32_t hw_irq = irq - gpio_desc->virq_offset - GPIO_IRQ_START;
	unsigned bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	uint32_t reg = gpio_irq_ctrl_reg(gpio_desc, hw_irq, bank_base);
	uint32_t index = gpio_irq_ctrl_offset(hw_irq);
	uint32_t val = 0;

	irq -= gpio_desc->virq_offset;

	if (irq < GPIO_IRQ_START || irq >= irq_max_num)
	{
		GPIO_ERR("Wrong irq NO.(%u) to enable !!", (unsigned int)irq);
		return -1;
	}

	irq_flags = hal_spin_lock_irqsave(get_irq_ctrl_reg_spinlock());

	/*clear pending*/
	gpio_irq_ack(gpio_desc, hw_irq);

	/*unmask the irq,should keep spin lock to protect*/
	val = hal_readl(gpio_desc->membase + reg);

	hal_writel(val | (1 << index), gpio_desc->membase + reg);
	hal_spin_unlock_irqrestore(get_irq_ctrl_reg_spinlock(), irq_flags);
	return 0;
}
#ifdef CONFIG_STANDBY
struct gpio_pm_reg_cache gpio_pm_reg;

static int gpio_pm_alloc_mem(uint32_t desc_index, uint32_t mem_size)
{
	if (desc_index > 1)
	{
		GPIO_ERR("index[%d] exceed desc_index range!", desc_index);
		return -1;
	}

	gpio_pm_reg.reg_dump[desc_index] = hal_malloc(mem_size);
	if (gpio_pm_reg.reg_dump[desc_index] == NULL)
	{
		GPIO_ERR("malloc reg_mem[%d] error!", desc_index);
		return -1;
	}

	gpio_pm_reg.reg_dump_size[desc_index] = mem_size;

	return 0;
}

int hal_gpio_suspend()
{
	int i;
	void *mem = NULL;
	uint32_t mem_size;
	uint32_t flags;
	struct gpio_desc *gpio_desc = NULL;

	GPIO_INFO("gpio suspend\n");

	flags = hal_interrupt_disable_irqsave();
	for (i = 0; g_gpio_desc[i] != NULL; i++) {
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		mem = gpio_pm_reg.reg_dump[i];
		mem_size = gpio_pm_reg.reg_dump_size[i];
		if (mem != NULL)
			memcpy(mem, (uint32_t *)gpio_desc->membase, mem_size);
	}
	hal_interrupt_enable_irqrestore(flags);

	return 0;
}

int hal_gpio_resume()
{
	int i;
	void *mem = NULL;
	uint32_t mem_size;
	uint32_t flags;
	struct gpio_desc *gpio_desc = NULL;

	flags = hal_interrupt_disable_irqsave();
	for (i = 0; g_gpio_desc[i] != NULL; i++) {
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		mem = gpio_pm_reg.reg_dump[i];
		mem_size = gpio_pm_reg.reg_dump_size[i];
		if (gpio_pm_reg.reg_dump[i] != NULL)
			memcpy((uint32_t *)gpio_desc->membase, mem, mem_size);
	}
	hal_interrupt_enable_irqrestore(flags);

	GPIO_INFO("gpio resume");

	return 0;
}
#endif
#ifdef CONFIG_COMPONENTS_PM
struct gpio_pm_reg_cache gpio_pm_reg;

static int gpio_pm_alloc_mem(uint32_t desc_index, uint32_t mem_size)
{
	if (desc_index > 1)
	{
		GPIO_ERR("index[%d] exceed desc_index range!", desc_index);
		return -1;
	}

	gpio_pm_reg.reg_dump[desc_index] = hal_malloc(mem_size);
	if (gpio_pm_reg.reg_dump[desc_index] == NULL)
	{
		GPIO_ERR("malloc reg_mem[%d] error!", desc_index);
		return -1;
	}

	gpio_pm_reg.reg_dump_size[desc_index] = mem_size;

	return 0;
}

static int hal_gpio_suspend(void *data, suspend_mode_t mode)
{
	int i;
	int j;
	uint32_t *mem = NULL;
	uint32_t mem_size;
	uint32_t flags;
	struct gpio_desc *gpio_desc = NULL;

	flags = hal_interrupt_disable_irqsave();
	for (i = 0; g_gpio_desc[i] != NULL; i++) {
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		mem = gpio_pm_reg.reg_dump[i];
		mem_size = gpio_pm_reg.reg_dump_size[i];
		if (mem != NULL) {
			for(j = 0; j < (mem_size >> 2); j++) {
				mem[j] = *(volatile unsigned int *)((unsigned long)gpio_desc->membase + j * 4);
			}
		}
	}
	hal_interrupt_enable_irqrestore(flags);

	pm_inf("gpio suspend end\n");
	return 0;
}

static void hal_gpio_resume(void *data, suspend_mode_t mode)
{
	int i;
	int j;
	uint32_t *mem = NULL;
	uint32_t mem_size;
	uint32_t flags;
	struct gpio_desc *gpio_desc = NULL;

	flags = hal_interrupt_disable_irqsave();
	for (i = 0; g_gpio_desc[i] != NULL; i++) {
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		mem = gpio_pm_reg.reg_dump[i];
		mem_size = gpio_pm_reg.reg_dump_size[i];
		if (gpio_pm_reg.reg_dump[i] != NULL) {
			for(j = 0; j < (mem_size >> 2); j++) {
				*(volatile unsigned int *)((unsigned long)gpio_desc->membase + j * 4) = mem[j];
			}
		}
	}
	hal_interrupt_enable_irqrestore(flags);

	pm_inf("gpio resume end\n");
}

static struct syscore_ops gpio_syscore_ops = {
	.name = "gpio_syscore_ops",
	.suspend = hal_gpio_suspend,
	.resume = hal_gpio_resume,
	.common_syscore = COMMON_SYSCORE,
};
#endif

int hal_gpio_irq_disable(uint32_t irq)
{
	unsigned long irq_flags;
	struct gpio_desc *gpio_desc = virq_to_gpio_desc(irq);
	GPIO_INFO("[%s]gpio_desc address is 0x%lx.", __func__, gpio_desc->membase);
	int irq_max_num = gpio_desc->irq_desc_size + GPIO_IRQ_START;
	uint32_t hw_irq = irq - gpio_desc->virq_offset - GPIO_IRQ_START;
	unsigned bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	uint32_t reg = gpio_irq_ctrl_reg(gpio_desc, hw_irq, bank_base);
	uint32_t index = gpio_irq_ctrl_offset(hw_irq);
	uint32_t val = 0;
	irq -= gpio_desc->virq_offset;
	if (irq < GPIO_IRQ_START || irq >= irq_max_num)
	{
		GPIO_ERR("Wrong irq NO.(%u) to enable !!", (unsigned int)irq);
		return -1;
	}

	irq_flags = hal_spin_lock_irqsave(get_irq_ctrl_reg_spinlock());

	val = hal_readl(gpio_desc->membase + reg);
	hal_writel(val & ~(1 << index), gpio_desc->membase + reg);

	hal_spin_unlock_irqrestore(get_irq_ctrl_reg_spinlock(), irq_flags);
	return 0;
}

int hal_gpio_init(void)
{
	int i, j, ret;
	struct gpio_desc *gpio_desc = NULL;
	struct gpio_irq_desc *irq_desc = NULL;
	int irq_desc_array_size = 0;
	char irqname[32] = {0};

	static int has_init = 0;
	if(has_init)
	{
		GPIO_ERR("GPIO already init\n");
		return -1;
	}
	has_init = 1;

	for (i = 0; i < ARRAY_SIZE(g_reg_spinlock); i++)
		hal_spin_lock_init(&g_reg_spinlock[i]);

	/* initialize g_gpio_desc */
	g_gpio_desc = gpio_get_platform_desc();
	if (g_gpio_desc == NULL)
	{
		GPIO_ERR("initialize global platform desc failed!");
		return -1;
	}

#ifdef CONFIG_DRIVERS_GPIO_RECORD_USAGE
	INIT_LIST_HEAD(&used_gpio_list);
#endif

#if defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
	sysconfig_gpio_map_init(g_gpio_desc);
#endif

	for (j = 0; g_gpio_desc[j] != NULL; j++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[j];
		irq_desc_array_size = gpio_desc->irq_banks * IRQ_PER_BANK;
		gpio_desc->irq_desc_size = irq_desc_array_size;

#ifdef CONFIG_STANDBY
		ret = gpio_pm_alloc_mem(j, gpio_desc->resource_size);
		if (ret)
		{
			GPIO_ERR("gpio[%d] pm alloc mem err!", j);
			return ret;
		}
#endif

		irq_desc = (struct gpio_irq_desc *)hal_malloc(irq_desc_array_size * sizeof(struct gpio_irq_desc));
		if (irq_desc == NULL)
		{
			GPIO_ERR("alloc memory failed!");
			return -1;
		}

		memset(irq_desc, 0, irq_desc_array_size * sizeof(struct gpio_irq_desc));
		for (i = 0; i < irq_desc_array_size; i++)
		{
			unsigned int n = i / IRQ_PER_BANK;
			unsigned int k = i % IRQ_PER_BANK;
			unsigned bank_base = gpio_desc->irq_bank_base[n];
			irq_desc[i].pin = gpio_get_pin_base_from_bank(n, bank_base) + gpio_desc->virq_offset + k;
			irq_desc[i].virq =  GPIO_IRQ_START + gpio_desc->virq_offset + i;
			irq_desc[i].handle_irq = bad_gpio_irq_handle;
		}

		gpio_desc->irq_desc = irq_desc;

#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
		for (i = 0; i < gpio_desc->irq_banks; i++)
		{
			/* mask all irq */
			unsigned bank_base = gpio_desc->irq_bank_base[i];
			uint32_t bank_mask, ctrl_val, sta_val;
			unsigned long ctrl_reg, sta_reg;

			ctrl_reg = gpio_desc->membase + gpio_irq_ctrl_reg_from_bank(gpio_desc, i, bank_base);
			sta_reg = gpio_desc->membase + gpio_irq_status_reg_from_bank(gpio_desc, i, bank_base);

			/* get gpio bank mask from share interrupt table */
			bank_mask = sunxi_get_banks_mask(gpio_desc->irq[i]);
			ctrl_val = hal_readl(ctrl_reg) & (~bank_mask);
			sta_val = 0xffffffff & bank_mask;

			hal_writel(ctrl_val, ctrl_reg);
			hal_writel(sta_val, sta_reg);
		}
#endif

		/* request irq */
		for (i = 0; i < gpio_desc->irq_arry_size; i++)
		{
#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
			uint32_t bank_mask = sunxi_get_banks_mask(gpio_desc->irq[i]);
			if (!bank_mask)
				continue;
#endif
			snprintf(irqname, 32, "gpio-ctl%d%d", j, i);
			ret = hal_request_irq(gpio_desc->irq[i], gpio_irq_handle, irqname, (void *)&gpio_desc->irq[i]);
			if (ret < 0) {
				GPIO_ERR("gpio request irq err!");
				return ret;
			}
		}

		/* enable irq */
		for (i = 0; i < gpio_desc->irq_arry_size; i++)
		{
#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
			uint32_t bank_mask = sunxi_get_banks_mask(gpio_desc->irq[i]);
			if (!bank_mask)
				continue;
#endif
			hal_enable_irq(gpio_desc->irq[i]);
		}

#ifdef CONFIG_COMPONENTS_PM
		ret = gpio_pm_alloc_mem(j, gpio_desc->resource_size);
		if (ret)
		{
			GPIO_ERR("gpio[%d] pm alloc mem err!", j);
			return ret;
		}
#ifdef CONFIG_PM_WAKESRC_GPIO
		for (i = 0; i < gpio_desc->irq_arry_size; i++) {
			gpio_desc->ws[i] = pm_wakesrc_register(gpio_desc->irq[i], "GPIO");
			if (gpio_desc->ws[i] == NULL)
				GPIO_ERR("gpio registers wakesrc fail\n");
		}
#endif
#endif

#if defined(GPIO_FEATURE_SYSCONFIG_POW_MODSEL) && defined(CONFIG_DRIVER_SYSCONFIG)
		for (i = 0; i < gpio_desc->banks; i++) {
			int val;
			char gpio_vol_str[7] = {0};

			sprintf(gpio_vol_str, "vcc-p%c", 'a' + gpio_desc->bank_base[i]);

			ret = hal_cfg_get_keyvalue("gpio", gpio_vol_str, (int32_t *)&val, 1);
			if (!ret)
				hal_gpio_sel_vol_bank_mode(gpio_desc->bank_base[i], val);
		}
#endif
	}
#ifdef CONFIG_COMPONENTS_PM
	if (pm_syscore_register(&gpio_syscore_ops))
		GPIO_ERR("register gpio syscore failed");
#endif

	GPIO_INFO("gpio init success!");
	return 0;
}

int hal_gpio_r_irq_disable(uint32_t irq)
{
	int i = 0;
	struct gpio_desc *gpio_desc = irq_to_gpio_desc(irq);
	if (gpio_desc == NULL)
	{
		GPIO_ERR("initialize global platform desc failed!");
		return -1;
	}

	for (i = 0; i < gpio_desc->irq_arry_size; i++)
	{
		if (gpio_desc->irq[i] == irq)
		{
			hal_disable_irq(gpio_desc->irq[i]);
		}
	}
	return 0;
}

int hal_gpio_r_irq_enable(uint32_t irq)
{
	int i = 0;
	struct gpio_desc *gpio_desc = irq_to_gpio_desc(irq);
	if (gpio_desc == NULL)
	{
		GPIO_ERR("initialize global platform desc failed!");
		return -1;
	}

	for (i = 0; i < gpio_desc->irq_arry_size; i++)
	{
		if (gpio_desc->irq[i] == irq)
		{
			hal_enable_irq(gpio_desc->irq[i]);
		}
	}
	return 0;
}

int hal_gpio_r_all_irq_disable(void)
{
	int i = 0, j = 0;
	struct gpio_desc *gpio_desc = NULL;
	for (j = 0; g_gpio_desc[j] != NULL; j++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[j];
		for (i = 0; i < gpio_desc->irq_arry_size; i++)
		{
			hal_disable_irq(gpio_desc->irq[i]);
		}
	}
	return 0;
}

