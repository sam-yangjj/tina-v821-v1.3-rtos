#ifndef __CONFIG_VIN_SUN8IW20_H_
#define __CONFIG_VIN_SUN8IW20_H_

#include <hal_gpio.h>
#include <hal_clk.h>

#include "../vin_video/vin_core.h"

#ifdef CONFIG_ARCH_RISCV_PMP
#define MEMRESERVE (CONFIG_ARCH_START_ADDRESS + CONFIG_ARCH_MEM_LENGTH)
#else
#define MEMRESERVE CONFIG_ISP_MEMRESERVE_ADDR
#endif

#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
#define MEMRESERVE_SIZE CONFIG_ISP_MEMRESERVE_LEN
#else
#define MEMRESERVE_SIZE 0xa00000
#endif

#define ISP_MAX_WIDTH				0
#define RTC_REGS_BASE				0x07090000

#define CSI_CCU_REGS_BASE			0x05800000
#define CSI_TOP_REGS_BASE			0x05800800

#define CSI0_REGS_BASE				0x05801000

#define ISP0_REGS_BASE				0x05809410
#define ISP1_REGS_BASE				0x05809420

#define VIPP0_REGS_BASE				0x05809430
#define VIPP1_REGS_BASE				0x05809440

#define CSI_DMA0_REG_BASE			0x05809000
#define CSI_DMA1_REG_BASE			0x05809200

#define SUNXIGIC_START 				16
#define SUNXI_IRQ_CSIC_DMA0			(127 - SUNXIGIC_START)
#define SUNXI_IRQ_CSIC_DMA1			(128 - SUNXIGIC_START)
#define SUNXI_IRQ_CSI_TOP_PKT 			(138 - SUNXIGIC_START)

#define DEV_VIRT_NUM			4

#define VIN_MAX_DEV			2
#define VIN_MAX_MIPI			0
#define VIN_MAX_CSI			1
#define VIN_MAX_CCI			1
#define VIN_MAX_TDM			0
#define VIN_MAX_ISP			0
#define VIN_MAX_SCALER			2
#define VIN_MAX_VIDEO			2
#define MAX_CH_NUM			4

#define MAX_DETECT_SENSOR		1

struct vin_clk_info {
	hal_clk_t clock;
	unsigned int clock_id;
	unsigned int type;
	unsigned long frequency;
};

struct vin_rst_clk_info {
	struct reset_control *clock;
	unsigned int clock_id;
	unsigned int type;
};

struct vin_mclk_info {
	hal_clk_t mclk;
	unsigned int mclk_id;
	unsigned int clk_24m_id;
	unsigned int clk_pll_id;
	unsigned int mclk_type;
	unsigned int clk_24m_type;
	unsigned int clk_pll_type;
	gpio_pin_t pin;
	unsigned char pin_func[2];
	unsigned long frequency;
	unsigned int use_count;
};

struct vin_mipi_gpio_info {
	gpio_pin_t pin[6];
	unsigned char pin_func[2];
};

struct vin_csi_gpio_info {
	gpio_pin_t pin[18];
	unsigned char pin_func[2];
};

struct sensor_list {
	char sensor_name[16];
	int used;
	int sensor_twi_addr;
	int sensor_twi_id;
	int mclk_id;
	int use_isp;
	int id;
	int addr_width;
	int data_width;
	gpio_pin_t reset_gpio;
	gpio_pin_t pwdn_gpio;
	gpio_pin_t ir_cut_gpio[2];
	gpio_pin_t ir_led_gpio;
	gpio_pin_t not_frame_loss_gpio;
	struct sensor_fuc_core *sensor_core;
};

extern struct vin_clk_info vind_default_clk[VIN_MAX_CLK];
extern struct vin_mclk_info vind_default_mclk[VIN_MAX_CCI];
extern struct vin_clk_info vind_default_isp_clk[VIN_ISP_MAX_CLK];
extern struct vin_mipi_gpio_info vin_mipi_gpio[VIN_MAX_MIPI];
extern struct vin_csi_gpio_info vin_csi_gpio[VIN_MAX_CSI];
extern struct vin_clk_info vind_default_mbus_clk[VIN_MAX_BUS_CLK];
extern struct vin_rst_clk_info vind_default_rst_clk[VIN_MAX_RET];

extern unsigned long rtc_base;
extern unsigned long vin_csi_base[VIN_MAX_CSI];
extern unsigned long vin_tdm_base[VIN_MAX_TDM];
extern unsigned long vin_isp_base[VIN_MAX_ISP/DEV_VIRT_NUM];
extern unsigned long vin_mipi_base[1 + VIN_MAX_MIPI];
extern unsigned long vin_mipi_port_base[VIN_MAX_MIPI];
extern unsigned long vin_scaler_base[VIN_MAX_SCALER/DEV_VIRT_NUM];

extern unsigned int vin_tdm_irq[VIN_MAX_TDM];
extern unsigned int vin_isp_irq[VIN_MAX_ISP];
extern unsigned int vin_vipp_irq[VIN_MAX_SCALER/DEV_VIRT_NUM];

extern struct vin_core global_video[VIN_MAX_VIDEO];
extern struct sensor_list global_sensors[VIN_MAX_CSI];
extern bool vin_work_mode;

extern struct sensor_list global_sensors_list[2][MAX_DETECT_SENSOR];

#endif /*__CONFIG_VIN_SUN8IW20P1_H_*/

