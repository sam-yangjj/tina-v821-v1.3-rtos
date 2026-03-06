/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 */
#include <include.h>

static inline void clrsetbits_le32(void *addr, u32 clear, u32 set)
{
	u32 tmp;

	tmp = readl(addr);
	tmp &= ~(clear);
	tmp |= set;
	writel(tmp, addr);
}

void sunxi_gpio_set_cfgbank(struct sunxi_gpio *pio, int bank_offset, u32 val)
{
	u32 index = GPIO_CFG_INDEX(bank_offset);
	u32 offset = GPIO_CFG_OFFSET(bank_offset);

	clrsetbits_le32(&pio->cfg[0] + index, 0xf << offset, val << offset);
}

void sunxi_gpio_set_cfgpin(u32 pin, u32 val)
{
	u32 bank = GPIO_BANK(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	sunxi_gpio_set_cfgbank(pio, pin, val);
}

int sunxi_gpio_get_cfgbank(struct sunxi_gpio *pio, int bank_offset)
{
	u32 index = GPIO_CFG_INDEX(bank_offset);
	u32 offset = GPIO_CFG_OFFSET(bank_offset);
	u32 cfg;

	cfg = readl(&pio->cfg[0] + index);
	cfg >>= offset;

	return cfg & 0xf;
}

int sunxi_gpio_get_cfgpin(u32 pin)
{
	u32 bank = GPIO_BANK(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	return sunxi_gpio_get_cfgbank(pio, pin);
}

int sunxi_gpio_set_drv(u32 pin, u32 val)
{
	u32 bank = GPIO_BANK(pin);
	u32 index = GPIO_DRV_INDEX(pin);
	u32 offset = GPIO_DRV_OFFSET(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	clrsetbits_le32(&pio->drv[0] + index, 0x3 << offset, val << offset);

	return 0;
}

int sunxi_gpio_set_pull(u32 pin, u32 val)
{
	u32 bank = GPIO_BANK(pin);
	u32 index = GPIO_PULL_INDEX(pin);
	u32 offset = GPIO_PULL_OFFSET(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);
	clrsetbits_le32(&pio->pull[0] + index, 0x3 << offset, val << offset);

	return 0;
}

void sunxi_gpio_set_data(u32 pin, u32 set)
{
	u32 bank = GPIO_BANK(pin);
	u32 offset = GPIO_DAT_OFFSET(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);
	u32 tmp;

	tmp = readl(&pio->dat);

	if (set)
		tmp |= (1 << offset);
	else
		tmp &= ~(1 << offset);

	writel(tmp, &pio->dat);
}

int sunxi_gpio_get_data(u32 pin)
{
	u32 bank = GPIO_BANK(pin);
	u32 offset = GPIO_DAT_OFFSET(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	if (readl(&pio->dat) & (1 << offset))
		return 1;
	else
		return 0;
}

