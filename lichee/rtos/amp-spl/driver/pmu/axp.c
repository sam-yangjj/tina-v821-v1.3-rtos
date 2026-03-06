/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 */

#include <include.h>

static int pmu_chip_id;

int set_sys_voltage(int set_vol)
{
	switch (pmu_chip_id) {
#ifdef CONFIG_AXP2101_POWER
	case AXP2101_CHIP_ID:
		return axp2101_set_sys_voltage(set_vol, 1);
#endif
	default:
		return -1;
	}
}

int set_sys_voltage_ext(char *name, int set_vol)
{
	switch (pmu_chip_id) {
	default:
		return -1;
	}
}

int set_pll_voltage(int set_vol)
{
	switch (pmu_chip_id) {
#ifdef CONFIG_AXP2101_POWER
	case AXP2101_CHIP_ID:
		return axp2101_set_pll_voltage(set_vol);
#endif
	default:
		return -1;
	}
}

int set_efuse_voltage(int set_vol)
{
	switch (pmu_chip_id) {
#ifdef CONFIG_AXP2101_POWER
	case AXP2101_CHIP_ID:
		return axp2101_set_efuse_voltage(set_vol);
#endif
	default:
		return -1;
	}
}

int axp_reg_read(u8 addr, u8 *val)
{
	switch (pmu_chip_id) {
#ifdef CONFIG_AXP2101_POWER
	case AXP2101_CHIP_ID:
		return axp2101_reg_read(addr, val);
#endif
	default:
		return -1;
	}
}

int axp_reg_write(u8 addr, u8 val)
{
	switch (pmu_chip_id) {
#ifdef CONFIG_AXP2101_POWER
	case AXP2101_CHIP_ID:
		return axp2101_reg_write(addr, val);
#endif
	default:
		return -1;
	}
}


int axp_init(u8 power_mode)
{
	pmu_chip_id = -1;
#ifdef CONFIG_AXP2101_POWER
		if (axp2101_axp_init(power_mode) == AXP2101_CHIP_ID) {
		return (pmu_chip_id = AXP2101_CHIP_ID);
	} else
#endif
		return -1;
}

int get_pmu_exist(void)
{
	/* ensure whether pmu is exist or not */
	/* If have pmu: return that pmu's id ,If not: return -1*/
	return pmu_chip_id;
}
