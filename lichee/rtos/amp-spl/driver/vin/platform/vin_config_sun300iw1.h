#ifndef __CONFIG_VIN_SUN300IW1_H_
#define __CONFIG_VIN_SUN300IW1_H_

#define CCMU_REGS_BASE				0x42001000
#define CCMU_AON_REGS_BASE				0x4A010000
#define CSI_MASTER0_CLOCK_REG_OFFSET		0x28
#define CSI_MASTER1_CLOCK_REG_OFFSET		0x2c

#define VIN_MAX_CSI			2
#define TWI_CH_NUM			3
#define VIN_MAX_VIDEO			8

struct twi_gpio_info {
	u32 pin[2];
	unsigned char pin_func[2];
};

struct sensor_list {
	char sensor_name[16];
	int used;
	int sensor_twi_addr;
	int sensor_twi_id;
	int mclk_id;
	int id;
	int addr_width;
	int data_width;
	int reset_gpio;
	int pwdn_gpio;
	int ir_cut_gpio[2];
	int ir_led_gpio;
	struct sensor_fuc_core *sensor_core;
};

struct vin_mclk_info {
	u32 pin;
	unsigned char pin_func[2];
};

extern struct vin_mclk_info vind_default_mclk[VIN_MAX_CSI];
extern unsigned int mclk_address[VIN_MAX_CSI];
extern unsigned int twi_address[TWI_CH_NUM];
extern struct twi_gpio_info twi_gpio[TWI_CH_NUM];
extern struct sensor_list global_sensors[VIN_MAX_CSI];

#endif /* __CONFIG_VIN_SUN300IW1_H_ */
