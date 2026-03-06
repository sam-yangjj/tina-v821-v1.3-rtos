#include <include.h>
#include "vin_config_sun300iw1.h"

#define SENSOR_TWI_SPEED 400000

unsigned int twi_address[TWI_CH_NUM] =
{
	SUNXI_TWI0_BASE,
	SUNXI_TWI1_BASE,
	SUNXI_TWI2_BASE,
};

unsigned int mclk_address[VIN_MAX_CSI] =
{
    CCMU_REGS_BASE + CSI_MASTER0_CLOCK_REG_OFFSET,
    CCMU_REGS_BASE + CSI_MASTER1_CLOCK_REG_OFFSET,
};


struct twi_gpio_info twi_gpio[TWI_CH_NUM] = {
	{
		.pin = {
			SUNXI_GPA(3), SUNXI_GPA(4),
			},
		.pin_func = {0x4, 0xf},
	},
};

struct vin_mclk_info vind_default_mclk[VIN_MAX_CSI] = {
	{
		.pin = SUNXI_GPA(1),
		.pin_func = {0x2, 0xf},
	},
	{
		.pin = SUNXI_GPA(2),
		.pin_func = {0x2, 0xf},
	},
};

struct sensor_list global_sensors[VIN_MAX_CSI] = {
	[0] = {
		.used = 1,
		.sensor_name = "gc1084_mipi",
		.sensor_twi_addr = 0x6e,
		.sensor_twi_id = 0,
		.mclk_id = 0,
		.id = 0,
		.addr_width = 16,
		.data_width = 8,
		.reset_gpio = SUNXI_GPD(12),
		.pwdn_gpio = 0xffff,
		.ir_cut_gpio[0] =0xffff, /*-cut*/
		.ir_cut_gpio[1] = 0xffff,  /*+cut*/
		.ir_led_gpio = 0xffff, //GPIOE(10)
	},
#if (CONFIG_SENSOR_NUMBER == 2)
	[1] = {
		.used = 1,
		.sensor_name = "gc1084_mipi_2",
		.sensor_twi_addr = 0x6e,
		.sensor_twi_id = 0,
		.mclk_id = 1,
		.id = 0,
		.addr_width = 16,
		.data_width = 8,
		.reset_gpio = SUNXI_GPD(13),
		.pwdn_gpio = 0xffff,
		.ir_cut_gpio[0] =0xffff, /*-cut*/
		.ir_cut_gpio[1] = 0xffff,  /*+cut*/
		.ir_led_gpio = 0xffff, //GPIOE(10)
	},
#endif
};
