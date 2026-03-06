#include <sunxi_hal_common.h>
#include "vin_config_sun8iw20p1.h"
#include "../../ccmu/sunxi-ng/ccu-sun8iw20.h"
#include "../../ccmu/sunxi-ng/rst-sun8iw20.h"
#include "../../ccmu/common_ccmu.h"

unsigned long rtc_base = RTC_REGS_BASE;

unsigned long vin_csi_base[VIN_MAX_CSI] = {
	CSI0_REGS_BASE,
};

unsigned long vin_tdm_base[VIN_MAX_TDM] = {
};

unsigned long vin_isp_base[VIN_MAX_ISP/DEV_VIRT_NUM] = {
};

unsigned long vin_mipi_base[1 + VIN_MAX_MIPI] = {
};

unsigned long vin_mipi_port_base[VIN_MAX_MIPI] = {
};

unsigned long vin_scaler_base[VIN_MAX_SCALER/DEV_VIRT_NUM] = {
};

unsigned int vin_tdm_irq[VIN_MAX_TDM] = {
};

unsigned int vin_isp_irq[VIN_MAX_ISP] = {
};

unsigned int vin_vipp_irq[VIN_MAX_SCALER/DEV_VIRT_NUM] = {
};

struct vin_clk_info vind_default_clk[VIN_MAX_CLK] = {
	[VIN_TOP_CLK] = {
	.clock_id = CLK_CSI_TOP,
	.type = HAL_SUNXI_CCU,
	.frequency = 378000000,
	},
	[VIN_TOP_CLK_SRC] = {
	.clock_id = CLK_PLL_VIDEO1_2X,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_TOP_CLK_SRC1] = {
	.clock_id = CLK_PLL_VIDEO0_2X,
	.type = HAL_SUNXI_CCU,
	.frequency = 300000000,
	},
};

struct vin_clk_info vind_default_isp_clk[VIN_ISP_MAX_CLK] = {
	[VIN_ISP_CLK] = {

	},
	[VIN_ISP_CLK_SRC] = {

	},
};

struct vin_clk_info vind_default_mbus_clk[VIN_MAX_BUS_CLK] = {
	[VIN_CSI_BUS_CLK] = {
	.clock_id = CLK_BUS_CSI,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_CSI_MBUS_CLK] = {
	.clock_id = CLK_MBUS_CSI,
	.type = HAL_SUNXI_CCU,
	},
};

struct vin_rst_clk_info vind_default_rst_clk[VIN_MAX_RET] = {
	[VIN_CSI_RET] = {
	.clock_id = RST_BUS_CSI,
	.type = HAL_SUNXI_RESET,
	},
};

struct vin_mipi_gpio_info vin_mipi_gpio[VIN_MAX_MIPI] = {

};

struct vin_csi_gpio_info vin_csi_gpio[VIN_MAX_CSI] = {
	{
		.pin = {
			GPIOE(2),GPIOE(4), GPIOE(5), GPIOE(6),
			GPIOE(7), GPIOE(8), GPIOE(9), GPIOE(10), GPIOE(11),
			},
		.pin_func = {0x2, 0xf},
	},
};

struct sensor_list global_sensors_list[2][MAX_DETECT_SENSOR] = {
	[0]= {
		[0] = {
			.sensor_name = "n5",
			.sensor_twi_addr = 0x64,
			.sensor_twi_id = 1,
			.mclk_id = 0,
			.use_isp = 0,
			.id = 0,
			.addr_width = 8,
			.data_width = 8,
			.reset_gpio = GPIOE(13),
			.pwdn_gpio = 0xffff,
			.ir_cut_gpio[0] = 0xffff, /*-cut*/
			.ir_cut_gpio[1] = 0xffff,  /*+cut*/
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
	},
};

#if (CONFIG_ISP_NUMBER == 1)
struct vin_mclk_info vind_default_mclk[VIN_MAX_CCI] = {
	{
		.mclk_id = CLK_CSI0_MCLK,
		.clk_24m_id = CLK_SRC_HOSC24M,
		.clk_pll_id = CLK_PLL_VIDEO1,
		.pin = GPIOE(3),
		.pin_func = {0x2, 0xf},
	},
};

struct sensor_list global_sensors[VIN_MAX_CSI] = {
	/* parser0 */
	[0] = {
#ifdef CONFIG_SENSOR_N5_DVP
		.used = 1,
		.sensor_name = "n5",
		.sensor_twi_addr = 0x64,
		.sensor_twi_id = 3,
		.mclk_id = 0,
		.use_isp = 0,
		.id = 0,
		.addr_width = 8,
		.data_width = 8,
		.reset_gpio = GPIOE(13),
		.pwdn_gpio = 0xffff,
		.ir_cut_gpio[0] = 0xffff, /*-cut*/
		.ir_cut_gpio[1] = 0xffff,  /*+cut*/
		.not_frame_loss_gpio = 0xffff,
		.ir_led_gpio = 0xffff, //GPIOE(10)
#else //CONFIG_SENSOR_GC2053_MIPI
		.used = 1,
		.sensor_name = "gc2053_mipi",
		.sensor_twi_addr = 0x6e,
		.sensor_twi_id = 1,
		.mclk_id = 0,
		.use_isp = 1,
		.id = 0,
		.addr_width = 8,
		.data_width = 8,
		.reset_gpio = GPIOA(11),
		.pwdn_gpio = GPIOA(9),
		.ir_cut_gpio[0] = GPIOD(18), /*-cut*/
		.ir_cut_gpio[1] = GPIOD(8),  /*+cut*/
		.ir_led_gpio = 0xffff, //GPIOE(10)
#endif
	},
};

struct vin_core global_video[VIN_MAX_VIDEO] = {
	[0] = {
#ifdef CONFIG_SENSOR_N5_DVP
		.used = 1,
		.id = 0,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 0,
		.mipi_sel = 0xff,
		.isp_sel = 0,
		.tdm_rx_sel = 0xff,
		.isp_tx_ch = 0,
		.base = CSI_DMA0_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA0,
		.o_width = 1920,
		.o_height = 1080,
		//.fourcc = V4L2_PIX_FMT_SRGGB10,
		.fourcc = V4L2_PIX_FMT_NV12,
		.fps_fixed = 25,
		.use_sensor_list = 0,
		.sensor_lane = 2,
#else
		.used = 1,
		.id = 0,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 3,
		.mipi_sel = 3,
		.isp_sel = 0,
		.tdm_rx_sel = 0xff,
		.isp_tx_ch = 0,
		.base = CSI_DMA0_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA0,
		.o_width = 640,
		.o_height = 480,
		//.fourcc = V4L2_PIX_FMT_SRGGB10,
		.fourcc = V4L2_PIX_FMT_NV12,
		.use_sensor_list = 0,
		.sensor_lane = 2,
#endif
	},
#if 0
	[4] = {
		.used = 0,
		.id = 4,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 0,
		.mipi_sel = 0,
		.isp_sel = 0,
		.tdm_rx_sel = 0xff,
		.isp_tx_ch = 0,
		.base = CSI_DMA1_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA1,
		.o_width = 640,
		.o_height = 480,
		.fourcc = V4L2_PIX_FMT_LBC_2_5X,
	},
	[8] = {
		.used = 0,
		.id = 8,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 0,
		.mipi_sel = 0,
		.isp_sel = 0,
		.tdm_rx_sel = 0xff,
		.isp_tx_ch = 0,
		.base = CSI_DMA2_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA2,
		.o_width = 640,
		.o_height = 480,
		.fourcc = V4L2_PIX_FMT_NV12,
		},
	[12]= {
		.used = 0,
		.id = 12,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 0,
		.mipi_sel = 0,
		.isp_sel = 0,
		.tdm_rx_sel = 0xff,
		.isp_tx_ch = 0,
		.base = CSI_DMA3_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA3,
		.o_width = 1920,
		.o_height = 1080,
		.fourcc = V4L2_PIX_FMT_NV12,
	}
#endif
};
bool vin_work_mode = 0;
#else //CONFIG_ISP_NUMBER == 2
struct vin_mclk_info vind_default_mclk[VIN_MAX_CCI] = {
	{
		.mclk_id = CLK_CSI0_MCLK,
		.clk_24m_id = CLK_SRC_HOSC24M,
		.clk_pll_id = CLK_PLL_VIDEO1,
		.pin = GPIOE(3),
		.pin_func = {0x2, 0xf},
	},,
};

struct sensor_list global_sensors[VIN_MAX_CSI] = {
#ifdef CONFIG_SENSOR_N5_DVP
		.used = 1,
		.sensor_name = "n5",
		.sensor_twi_addr = 0x64,
		.sensor_twi_id = 1,
		.mclk_id = 0,
		.use_isp = 0,
		.id = 0,
		.addr_width = 8,
		.data_width = 8,
		.reset_gpio = GPIOE(13),
		.pwdn_gpio = 0xffff,
		.ir_cut_gpio[0] = 0xffff, /*-cut*/
		.ir_cut_gpio[1] = 0xffff,  /*+cut*/
		.not_frame_loss_gpio = 0xffff,
		.ir_led_gpio = 0xffff, //GPIOE(10)
#else //CONFIG_SENSOR_GC2053_MIPI
		.used = 1,
		.sensor_name = "gc2053_mipi",
		.sensor_twi_addr = 0x6e,
		.sensor_twi_id = 1,
		.mclk_id = 0,
		.use_isp = 1,
		.id = 0,
		.addr_width = 8,
		.data_width = 8,
		.reset_gpio = GPIOA(11),
		.pwdn_gpio = GPIOA(9),
		.ir_cut_gpio[0] = GPIOD(18), /*-cut*/
		.ir_cut_gpio[1] = GPIOD(8),  /*+cut*/
		.ir_led_gpio = 0xffff, //GPIOE(10)
#endif
};

struct vin_core global_video[VIN_MAX_VIDEO] = {
#ifdef CONFIG_SENSOR_N5_DVP
		.used = 1,
		.id = 0,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 0,
		.mipi_sel = 0xff,
		.isp_sel = 0,
		.tdm_rx_sel = 0xff,
		.isp_tx_ch = 0,
		.base = CSI_DMA0_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA0,
		.o_width = 1280,
		.o_height = 720,
		//.fourcc = V4L2_PIX_FMT_SRGGB10,
		.fourcc = V4L2_PIX_FMT_NV12,
		.use_sensor_list = 0,
		.sensor_lane = 2,
#else
		.used = 1,
		.id = 0,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 3,
		.mipi_sel = 3,
		.isp_sel = 0,
		.tdm_rx_sel = 0xff,
		.isp_tx_ch = 0,
		.base = CSI_DMA0_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA0,
		.o_width = 640,
		.o_height = 480,
		//.fourcc = V4L2_PIX_FMT_SRGGB10,
		.fourcc = V4L2_PIX_FMT_NV12,
		.use_sensor_list = 0,
		.sensor_lane = 2,
#endif
};
bool vin_work_mode = 1;
#endif
