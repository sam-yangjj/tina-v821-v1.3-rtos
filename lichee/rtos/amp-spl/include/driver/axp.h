#ifndef __PMU_H__
#define __PMU_H__

#include <include.h>
#ifdef CONFIG_AXP2101_POWER
#include <driver/axp2101_reg.h>
#endif

#define pmu_err(format, arg...) printf("[pmu]: " format, ##arg)
#define pmu_info(format, arg...)

typedef struct _axp_step_info {
	u32 step_min_vol;
	u32 step_max_vol;
	u32 step_val;
	u32 regation;
} _axp_step_info;

typedef struct _axp_contrl_info {
	char name[16];
	u32 min_vol;
	u32 max_vol;
	u32 cfg_reg_addr;
	u32 cfg_reg_mask;
	u32 ctrl_reg_addr;
	u32 ctrl_bit_ofs;
	u32 reg_addr_offest;
	_axp_step_info axp_step_tbl[4];
} axp_contrl_info;

#define AXP_BOOT_SOURCE_BUTTON         0
#define AXP_BOOT_SOURCE_IRQ_LOW        1
#define AXP_BOOT_SOURCE_VBUS_USB       2
#define AXP_BOOT_SOURCE_CHARGER        3
#define AXP_BOOT_SOURCE_BATTERY        4

int axp_init(u8 power_mode);
int axp_reg_write(u8 addr, u8 val);
int axp_reg_read(u8 addr, u8 *val);
int set_sys_voltage(int set_vol);
int set_sys_voltage_ext(char *name, int set_vol);
int set_pll_voltage(int set_vol);
int get_pmu_exist(void);

/* internal function */
int pmic_bus_init(u32 speed);
int pmic_bus_read(u32 runtime_addr, u8 reg, u8 *data);
int pmic_bus_write(u32 runtime_addr, u8 reg, u8 data);
int pmic_bus_setbits(u32 runtime_addr, u8 reg, u8 bits);
int pmic_bus_clrbits(u32 runtime_addr, u8 reg, u8 bits);


#endif  /* __PMU_H__ */
