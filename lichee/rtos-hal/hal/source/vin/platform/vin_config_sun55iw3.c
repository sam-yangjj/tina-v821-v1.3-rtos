#include <sunxi_hal_common.h>
#include "vin_config_sun55iw3.h"
#include "../../ccmu/sunxi-ng/ccu-sun55iw3.h"
#include "../../ccmu/sunxi-ng/rst-sun55iw3.h"
#include "../../ccmu/common_ccmu.h"

unsigned long rtc_base = RTC_REGS_BASE;

unsigned long vin_csi_base[VIN_MAX_CSI] = {
	CSI0_REGS_BASE,
	CSI1_REGS_BASE,
	CSI2_REGS_BASE,
	CSI3_REGS_BASE,
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
	MIPI_PHYC_REGS_BASE,
	MIPI_PHYD_REGS_BASE,
};

unsigned long vin_mipi_port_base[VIN_MAX_MIPI] = {
	MIPI_PORT0_REGS_BASE,
	MIPI_PORT1_REGS_BASE,
	MIPI_PORT2_REGS_BASE,
	MIPI_PORT3_REGS_BASE,
};

unsigned long vin_scaler_base[VIN_MAX_SCALER/DEV_VIRT_NUM] = {
	VIPP0_REGS_BASE,
	VIPP1_REGS_BASE,
	VIPP2_REGS_BASE,
	VIPP3_REGS_BASE,
};

unsigned int vin_tdm_irq[VIN_MAX_TDM] = {
	SUNXI_IRQ_TDM,
};

unsigned int vin_isp_irq[VIN_MAX_ISP] = {
	SUNXI_IRQ_ISP0,
	SUNXI_IRQ_ISP1,
	SUNXI_IRQ_ISP2,
	SUNXI_IRQ_ISP3,
};

unsigned int vin_vipp_irq[VIN_MAX_SCALER/DEV_VIRT_NUM] = {
	SUNXI_IRQ_VIPP0,
	SUNXI_IRQ_VIPP1,
	SUNXI_IRQ_VIPP2,
	SUNXI_IRQ_VIPP3,
};

#if defined CONFIG_SENSOR_SC035HGS_MIPI
struct vin_clk_info vind_default_clk[VIN_MAX_CLK] = {
	[VIN_TOP_CLK] = {
	.clock_id = CLK_CSI,
	.type = HAL_SUNXI_CCU,
	.frequency = 200000000,
	},
	[VIN_TOP_CLK_SRC] = {
	.clock_id = CLK_PLL_VIDEO3_4X,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_TOP_CLK_SRC1] = {
	.clock_id = CLK_PLL_PERI0_300M,
	.type = HAL_SUNXI_CCU,
	.frequency = 300000000,
	},
};

struct vin_clk_info vind_default_isp_clk[VIN_ISP_MAX_CLK] = {
	[VIN_ISP_CLK] = {
	.clock_id = CLK_ISP,
	.type = HAL_SUNXI_CCU,
	.frequency = 192000000,
	},
	[VIN_ISP_CLK_SRC] = {
	.clock_id = CLK_PLL_VIDEO2_4X,
	.type = HAL_SUNXI_CCU,
	.frequency = 768000000,
	},
};

struct vin_clk_info vind_csi_isp_parent_clk[VIN_PARENT_MAX_CLK] = {
	[VIN_CSI_PARENT] = {
	.clock_id = CLK_PLL_VIDEO3_PARENT,
	.type = HAL_SUNXI_CCU,
	.frequency = 1800000000,
	},
	[VIN_ISP_PARENT] = {
	.clock_id = CLK_PLL_VIDEO2_PARENT,
	.type = HAL_SUNXI_CCU,
	.frequency = 1536000000,
	},
};
#else
struct vin_clk_info vind_default_clk[VIN_MAX_CLK] = {
	[VIN_TOP_CLK] = {
	.clock_id = CLK_CSI,
	.type = HAL_SUNXI_CCU,
	.frequency = 360000000,
	},
	[VIN_TOP_CLK_SRC] = {
	.clock_id = CLK_PLL_VIDEO3_4X,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_TOP_CLK_SRC1] = {
	.clock_id = CLK_PLL_PERI0_300M,
	.type = HAL_SUNXI_CCU,
	.frequency = 300000000,
	},
};

struct vin_clk_info vind_default_isp_clk[VIN_ISP_MAX_CLK] = {
	[VIN_ISP_CLK] = {
	.clock_id = CLK_ISP,
	.type = HAL_SUNXI_CCU,
	.frequency = 288000000,
	},
	[VIN_ISP_CLK_SRC] = {
	.clock_id = CLK_PLL_VIDEO2_4X,
	.type = HAL_SUNXI_CCU,
	.frequency = 864000000,
	},
};

struct vin_clk_info vind_csi_isp_parent_clk[VIN_PARENT_MAX_CLK] = {
	[VIN_CSI_PARENT] = {
	.clock_id = CLK_PLL_VIDEO3_PARENT,
	.type = HAL_SUNXI_CCU,
	.frequency = 1800000000,
	},
	[VIN_ISP_PARENT] = {
	.clock_id = CLK_PLL_VIDEO2_PARENT,
	.type = HAL_SUNXI_CCU,
	.frequency = 1728000000,
	},
};
#endif
struct vin_clk_info vind_default_mbus_clk[VIN_MAX_BUS_CLK] = {
	[VIN_CSI_BUS_CLK] = {
	.clock_id = CLK_BUS_CSI,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_CSI_MBUS_CLK] = {
	.clock_id = CLK_CSI_MBUS_GATE,
	.type = HAL_SUNXI_CCU,
	},
	[VIN_ISP_MBUS_CLK] = {
	.clock_id = CLK_ISP_MBUS_GATE,
	.type = HAL_SUNXI_CCU,
	},
};

struct vin_rst_clk_info vind_default_rst_clk[VIN_MAX_RET] = {
	[VIN_CSI_RET] = {
	.clock_id = RST_BUS_CSI,
	.type = HAL_SUNXI_RESET,
	},
	[VIN_ISP_RET] = {
	.clock_id = RST_BUS_ISP,
	.type = HAL_SUNXI_RESET,
	},
};

struct vin_mipi_gpio_info vin_mipi_gpio[VIN_MAX_MIPI] = {
	{
		.pin = {
			GPIOK(0), GPIOK(1), GPIOK(2), GPIOK(3), GPIOK(4), GPIOK(5),
			},
		.pin_func = {0x2, 0xf},
	},
	{
		.pin = {
			GPIOK(6), GPIOK(7), GPIOK(8), GPIOK(9), GPIOK(10), GPIOK(11),
			},
		.pin_func = {0x2, 0xf},
	},
	{
		.pin = {
			GPIOK(12), GPIOK(13), GPIOK(14), GPIOK(15), GPIOK(16), GPIOK(17),
			},
		.pin_func = {0x2, 0xf},
	},
	{
		.pin = {
			GPIOK(18), GPIOK(19), GPIOK(20), GPIOK(21), GPIOK(22), GPIOK(23),
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

#if (CONFIG_ISP_NUMBER == 1)
struct vin_mclk_info vind_default_mclk[VIN_MAX_CCI] = {
	{
		.mclk_id = CLK_CSI_MASTER0,
		.clk_24m_id = CLK_SRC_HOSC24M,
		.clk_pll_id = CLK_PLL_VIDEO1_4X,
		.pin = GPIOE(0),
		.pin_func = {0x2, 0xf},
	},
	{
		.mclk_id = CLK_CSI_MASTER1,
		.clk_24m_id = CLK_SRC_HOSC24M,
		.clk_pll_id = CLK_PLL_VIDEO1_4X,
		.pin = GPIOE(5),
		.pin_func = {0x2, 0xf},
	},
	{
		.mclk_id = CLK_CSI_MASTER2,
		.clk_24m_id = CLK_SRC_HOSC24M,
		.clk_pll_id = CLK_PLL_VIDEO1_4X,
		.pin = GPIOE(15),
		.pin_func = {0x2, 0xf},
	},
	{
		.mclk_id = CLK_CSI_MASTER3,
		.clk_24m_id = CLK_SRC_HOSC24M,
		.clk_pll_id = CLK_PLL_VIDEO1_4X,
		.pin = GPIOE(10),
		.pin_func = {0x2, 0xf},
	},
};

struct sensor_list global_sensors[VIN_MAX_CSI] = {
	/*mipi0 parser0*/
	[0] = {
#ifdef CONFIG_SENSOR_SC035HGS_MIPI
		.used = 1,
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
		.not_frame_loss_gpio = 0xffff,
		.ir_led_gpio = 0xffff, //GPIOE(10)
#elif defined CONFIG_SENSOR_TP2815_MIPI
		.used = 1,
		.sensor_name = "tp2815_mipi",
		.sensor_twi_addr = 0x88,
		.sensor_twi_id = 2,
		.mclk_id = 0,
		.use_isp = 0,
		.id = 0,
		.addr_width = 8,
		.data_width = 8,
		.reset_gpio = 0xffff,
		.pwdn_gpio = 0xffff,
		.ir_cut_gpio[0] = 0xffff, /*-cut*/
		.ir_cut_gpio[1] = 0xffff,  /*+cut*/
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
	/*mipi1 parser1*/
	[1] = {
		.used = 0,
		.sensor_name = "s5k5e8",
		.sensor_twi_addr = 0x30,
		.sensor_twi_id = 3,
		.mclk_id = 2,
		.use_isp = 1,
		.id = 1,
		.addr_width = 16,
		.data_width = 8,
		.reset_gpio = GPIOA(20),
		.pwdn_gpio = GPIOA(21),
		.ir_cut_gpio[0] = 0xffff,/*-cut*/
		.ir_cut_gpio[1] = 0xffff,/*+cut*/
		.ir_led_gpio = 0xffff,
	},
};

struct vin_core global_video[VIN_MAX_VIDEO] = {
	[0] = {
#ifdef CONFIG_SENSOR_SC035HGS_MIPI
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
#elif defined CONFIG_SENSOR_TP2815_MIPI
		.used = 1,
		.id = 0,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 0,
		.mipi_sel = 0,
		.isp_sel = 4,
		.tdm_rx_sel = 0xff,
		.isp_tx_ch = 0,
		.base = CSI_DMA0_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA0,
		.o_width = 1920,
		.o_height = 1080,
		.fourcc = V4L2_PIX_FMT_NV12,
		.use_sensor_list = 0,
		.sensor_lane = 4,
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
		.mclk = HAL_CLK_PERIPH_CSI_MASTER0,
		.clk_24m = HAL_CLK_SRC_HOSC,
		.clk_pll = HAL_CLK_PLL_CSI,
		.pin = GPIOE(12),
		.pin_func = {0x5, 0xf},
	},
	{
		.mclk = HAL_CLK_PERIPH_CSI_MASTER1,
		.clk_24m = HAL_CLK_SRC_HOSC,
		.clk_pll = HAL_CLK_PLL_CSI,
		.pin = GPIOE(13),
		.pin_func = {0x5, 0xf},
	},
	{
		.mclk = HAL_CLK_PERIPH_CSI_MASTER1,
		.clk_24m = HAL_CLK_SRC_HOSC,
		.clk_pll = HAL_CLK_PLL_CSI,
		.pin = GPIOE(1),
		.pin_func = {0x2, 0xf},
	},
};

struct sensor_list global_sensors[VIN_MAX_CSI] = {
	/*mipi0 parser0*/
	[0] = {
		.used = 0,
		.sensor_name = "gc2053_mipi",
		.sensor_twi_addr = 0x6e,
		.sensor_twi_id = 1,
		.mclk_id = 0,
		.use_isp = 1,
		.id = 0,
		.addr_width = 8,
		.data_width = 8,
		.reset_gpio = GPIOE(6),
		.pwdn_gpio = GPIOE(7),
		.ir_cut_gpio[0] = 0xffff,/*-cut*/
		.ir_cut_gpio[1] = 0xffff,/*+cut*/
		.ir_led_gpio = 0xffff,
	},
	/*mipi1 parser1*/
	[1] = {
		.used = 0,
		.sensor_name = "gc2053_mipi",
		.sensor_twi_addr = 0x6e,
		.sensor_twi_id = 0,
		.mclk_id = 1,
		.use_isp = 1,
		.id = 1,
		.addr_width = 8,
		.data_width = 8,
		.reset_gpio = GPIOE(8),
		.pwdn_gpio = GPIOE(9),
		.ir_cut_gpio[0] = 0xffff,/*-cut*/
		.ir_cut_gpio[1] = 0xffff,/*+cut*/
		.ir_led_gpio = 0xffff,
	},
};

struct vin_core global_video[VIN_MAX_VIDEO] = {
	[0] = {
		.used = 0,
		.id = 0,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 0,
		.mipi_sel = 0,
		.isp_sel = 0,
		.tdm_rx_sel = 0,
		.isp_tx_ch = 0,
		.base = CSI_DMA0_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA0,
		.o_width = 512,
		.o_height = 288,
		//.fourcc = V4L2_PIX_FMT_LBC_2_5X,
		.fourcc = V4L2_PIX_FMT_NV12,
		.use_sensor_list = 0,
	},
	[1] = {
		.used = 0,
		.id = 1,
		.rear_sensor = 0,
		.front_sensor = 0,
		.csi_sel = 1,
		.mipi_sel = 1,
		.isp_sel = 1,
		.tdm_rx_sel = 1,
		.isp_tx_ch = 0,
		.base = CSI_DMA0_REG_BASE,
		.irq = SUNXI_IRQ_CSIC_DMA0,
		.o_width = 512,
		.o_height = 288,
		//.fourcc = V4L2_PIX_FMT_LBC_2_5X,
		.fourcc = V4L2_PIX_FMT_NV12,
		.use_sensor_list = 0,
	},
};
bool vin_work_mode = 1;
#endif
