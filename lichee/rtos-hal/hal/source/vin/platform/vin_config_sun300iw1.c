#include <sunxi_hal_common.h>
#include "vin_config_sun300iw1.h"
#ifdef CONFIG_DRIVERS_CCMU
#include "../../ccmu/sunxi-ng/ccu-sun300iw1-aon.h"
#include "../../ccmu/sunxi-ng/ccu-sun300iw1-app.h"
#include "../../ccmu/sunxi-ng/rst-sun300iw1-aon.h"
#include "../../ccmu/sunxi-ng/rst-sun300iw1-app.h"
#include "../../ccmu/common_ccmu.h"
#endif

bool vin_work_mode;
struct sensor_list global_sensors[VIN_MAX_CSI];

unsigned long syscfg_base = SYSCFG_REGS_BASE;
unsigned long rtc_base = RTC_REGS_BASE;

unsigned long vin_csi_base[VIN_MAX_CSI] = {
	CSI0_REGS_BASE,
	CSI1_REGS_BASE,
};

unsigned long vin_tdm_base[VIN_MAX_TDM] = {
	TDM_REGS_BASE,
};

unsigned long vin_isp_base[VIN_MAX_ISP/DEV_VIRT_NUM] = {
	ISP_REGS_BASE,
};

unsigned long vin_mipi_base[1 + VIN_MAX_MIPI] = {
	MIPI_REGS_BASE,
	MIPI_PHYA_REGS_BASE,
	MIPI_PHYB_REGS_BASE,
};

unsigned long vin_mipi_port_base[VIN_MAX_MIPI] = {
	MIPI_PORT0_REGS_BASE,
	MIPI_PORT1_REGS_BASE,
};

unsigned long vin_scaler_base[VIN_MAX_SCALER/DEV_VIRT_NUM] = {
	VIPP0_REGS_BASE,
	VIPP1_REGS_BASE,
};

unsigned int vin_tdm_irq[VIN_MAX_TDM] = {
	SUNXI_IRQ_TDM,
};

unsigned int vin_isp_irq[VIN_MAX_ISP] = {
	SUNXI_IRQ_ISP0,
	SUNXI_IRQ_ISP1,
};

unsigned int vin_vipp_irq[VIN_MAX_SCALER/DEV_VIRT_NUM] = {
	SUNXI_IRQ_VIPP0,
	SUNXI_IRQ_VIPP1,
};

struct vin_clk_info vind_default_clk[VIN_MAX_CLK] = {
	[VIN_TOP_CLK] = {
	.clock_id = CLK_MCSI,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_TOP_CLK_SRC] = {
	.clock_id = CLK_PLL_VIDEO_4X,
	.type = HAL_SUNXI_AON_CCU,
	},
	[VIN_TOP_CLK_SRC1] = {
	.clock_id = CLK_PLL_CSI_4X,
	.type = HAL_SUNXI_AON_CCU,
	},
};

struct vin_clk_info vind_default_isp_clk[VIN_ISP_MAX_CLK] = {
	[VIN_ISP_CLK] = {
	.type = NOT_USE_THIS_CLK,
	},
	[VIN_ISP_CLK_SRC] = {
	.type = NOT_USE_THIS_CLK,
	},
};

struct vin_clk_info vind_csi_isp_parent_clk[VIN_PARENT_MAX_CLK] = {
	[VIN_CSI_PARENT] = {
	.clock_id = CLK_PLL_VIDEO_4X,
	.type = HAL_SUNXI_AON_CCU,
	},
};

struct vin_clk_info vind_default_mbus_clk[VIN_MAX_BUS_CLK] = {
	[VIN_CSI_BUS_CLK] = {
	.clock_id = CLK_MCSI_AHB,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_CSI_MBUS_CLK] = {
	.clock_id = CLK_MBUS_MCSI,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_CSI_HBUS_CLK] = {
	.clock_id = CLK_HBUS_MCSI,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_CSI_SBUS_CLK] = {
	.clock_id = CLK_SBUS_MCSI,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_ISP_MBUS_CLK] = {
	.type = NOT_USE_THIS_CLK,
	},
	[VIN_ISP_SBUS_CLK] = {
	.clock_id = CLK_BUS_MISP,
	.type = HAL_SUNXI_CCU,
	},
};

struct vin_rst_clk_info vind_default_rst_clk[VIN_MAX_RET] = {
	[VIN_CSI_RET] = {
	.clock_id = RST_BUS_MCSI,
	.type = HAL_SUNXI_RESET,
	},
	[VIN_ISP_RET] = {
	.type = NOT_USE_THIS_CLK,
	},
};

struct vin_mipi_gpio_info vin_mipi_gpio[VIN_MAX_MIPI] = {
	{
		.pin = {
			GPIOA(5), GPIOA(6), GPIOA(7), GPIOA(8), -1, -1
			},
		.pin_func = {0x2, 0xf},
	},
	{
		.pin = {
			GPIOA(9), GPIOA(10), GPIOA(11), GPIOA(12), -1, -1
			},
		.pin_func = {0x2, 0xf},
	},
};

struct vin_csi_gpio_info vin_csi_gpio[VIN_MAX_CSI] = {
};

struct sensor_list global_sensors_list[2][MAX_DETECT_SENSOR] = {
	[0]= {
		[0] = {
			.sensor_name = "sc035hgs_mipi",
			.sensor_twi_addr = 0x60,
			.sensor_twi_id = 4,
			.mclk_id = 2,
			.use_isp = 1,
			.id = 0,
			.addr_width = 16,
			.data_width = 8,
			.reset_gpio = GPIOE(9),
			.pwdn_gpio = GPIOE(8),
			.ir_cut_gpio[0] = 0xffff, /*-cut*/
			.ir_cut_gpio[1] = 0xffff,  /*+cut*/
			.ir_led_gpio = 0xffff, //GPIOE(10)
		},
		[1] = {
			.sensor_name = "s5k5e8",
			.sensor_twi_addr = 0x30,
			.sensor_twi_id = 4,
			.mclk_id = 2,
			.use_isp = 1,
			.id = 1,
			.addr_width = 16,
			.data_width = 8,
			.reset_gpio = GPIOE(6),
			.pwdn_gpio = GPIOE(7),
			.ir_cut_gpio[0] = GPIOD(18), /*-cut*/
			.ir_cut_gpio[1] = GPIOD(8),  /*+cut*/
			.ir_led_gpio = 0xffff, //GPIOE(10)
		},
	},
	[1]= {
		[0] = {
			.sensor_name = "gc2053_mipi",
			.sensor_twi_addr = 0x6e,
			.sensor_twi_id = 0,
			.mclk_id = 1,
			.use_isp = 1,
			.id = 0,
			.addr_width = 8,
			.data_width = 8,
			.reset_gpio = GPIOE(8),
			.pwdn_gpio = GPIOE(9),
			.ir_cut_gpio[0] = 0xffff,/*-cut*/
			.ir_cut_gpio[1] = 0xffff,/*+cut*/
			.ir_led_gpio = 0xffff,
		},
		[1] = {
			.sensor_name = "gc4663_mipi",
			.sensor_twi_addr = 0x52,
			.sensor_twi_id = 0,
			.mclk_id = 1,
			.use_isp = 1,
			.id = 1,
			.addr_width = 16,
			.data_width = 8,
			.reset_gpio = GPIOE(8),
			.pwdn_gpio = GPIOE(9),
			.ir_cut_gpio[0] = 0xffff,/*-cut*/
			.ir_cut_gpio[1] = 0xffff,/*+cut*/
			.ir_led_gpio = 0xffff,
		},
	},
};

struct vin_mclk_info vind_default_mclk[VIN_MAX_CCI] = {
	{
		.mclk_id = CLK_CSI_MASTER0,
		.mclk_type = HAL_SUNXI_CCU,
		.clk_24m_id = CLK_PLL_PERI_CKO_24M,
		.clk_24m_type = HAL_SUNXI_AON_CCU,
		.clk_pll_id = CLK_PLL_CSI_4X,
		.clk_pll_type = HAL_SUNXI_AON_CCU,
		.pin = GPIOA(1),
		.pin_func = {0x2, 0xf},
	},
	{
		.mclk_id = CLK_CSI_MASTER1,
		.mclk_type = HAL_SUNXI_CCU,
		.clk_24m_id = CLK_PLL_PERI_CKO_24M,
		.clk_24m_type = HAL_SUNXI_AON_CCU,
		.clk_pll_id = CLK_PLL_CSI_4X,
		.clk_pll_type = HAL_SUNXI_AON_CCU,
		.pin = GPIOA(2),
		.pin_func = {0x2, 0xf},
	},
};

struct vin_core global_video[VIN_MAX_VIDEO] = {
	[0] = {
		.base = CSI_DMA0_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA0,
		//.fourcc = V4L2_PIX_FMT_SRGGB10,
		.fourcc = V4L2_PIX_FMT_NV12,
	},
	[1] = {
		.base = CSI_DMA0_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA0,
		//.fourcc = V4L2_PIX_FMT_SRGGB10,
		.fourcc = V4L2_PIX_FMT_NV12,
	},
	[4] = {
		.base = CSI_DMA1_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA1,
		//.fourcc = V4L2_PIX_FMT_SRGGB10,
		.fourcc = V4L2_PIX_FMT_NV12,
	},
	[5] = {
		.base = CSI_DMA1_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA1,
		//.fourcc = V4L2_PIX_FMT_SRGGB10,
		.fourcc = V4L2_PIX_FMT_NV12,
	},
};
