/*
 * (C) Copyright 2015 Hans de Goede <hdegoede@redhat.com>
 *
 * Sunxi PMIC bus access helpers
 *
 * The axp152 & axp209 use an twi bus, the axp221 uses the p2wi bus and the
 * axp223 uses the rsb bus, these functions abstract this.
 *
 */

#include <include.h>

#if CONFIG_PMU_TWI_ID == 0
#define PMU_TWI_IOBASE					(SUNXI_TWI0_BASE)
#endif
#if CONFIG_PMU_TWI_ID == 1
#define PMU_TWI_IOBASE					(SUNXI_TWI1_BASE)
#endif
#if CONFIG_PMU_TWI_ID == 2
#define PMU_TWI_IOBASE					(SUNXI_TWI2_BASE)
#endif
#if CONFIG_PMU_TWI_ID == 4
#define PMU_TWI_IOBASE					(SUNXI_TWI4_BASE)
#endif
#if CONFIG_PMU_TWI_ID == 5
#define PMU_TWI_IOBASE					(SUNXI_TWI5_BASE)
#endif

int pmic_bus_init(u32 speed)
{
	twi_init(PMU_TWI_IOBASE, speed);
	return 0;
}

int pmic_bus_read(u32 runtime_addr, u8 reg, u8 *data)
{
	return twi_read(PMU_TWI_IOBASE, runtime_addr, reg, 1, data, 1);
}

int pmic_bus_write(u32 runtime_addr, u8 reg, u8 data)
{
	return twi_write(PMU_TWI_IOBASE, runtime_addr, reg, 1, &data, 1);
}

int pmic_bus_setbits(u32 runtime_addr, u8 reg, u8 bits)
{
	int ret;
	u8 val;

	ret = pmic_bus_read(runtime_addr, reg, &val);
	if (ret)
		return ret;

	val |= bits;
	return pmic_bus_write(runtime_addr, reg, val);
}

int pmic_bus_clrbits(u32 runtime_addr, u8 reg, u8 bits)
{
	int ret;
	u8 val;

	ret = pmic_bus_read(runtime_addr, reg, &val);
	if (ret)
		return ret;

	val &= ~bits;
	return pmic_bus_write(runtime_addr, reg, val);
}
