/*
 * for app clock
 */

#ifndef __CCMU_APP_H
#define __CCMU_APP_H

#include <arch/sun300iw1/clock_autogen_app.h>

#define sunxi_ccm_reg            CCU_APP_st
#define	ahb_reset0_cfg           bus_reset1_reg
#define ahb_gate0                bus_clk_gating1_reg
#define spi0_clk_cfg             spi_clk_reg
#define spif_clk_cfg		 spif_clk_reg
#define dma_gate_reset          bus_reset0_reg
#define mbus_gate               bus_clk_gating2_reg

/* These nand macros are only for compilation and will not be used */
#define nand0_clk0_clk_reg      spi1_clk_reg
#define nand0_clk1_clk_reg      spi1_clk_reg
#define nand_bgr_reg            spi1_clk_reg
#define mbus_mat_clk_gating_reg spi2_clk_reg

#define SUNXI_CCM_APP_BASE       (SUNXI_CCU_APP_BASE)

#define SUNXI_CCM_BASE           (SUNXI_CCU_APP_BASE)
#define CCMU_GPADC_BGR_REG       (SUNXI_CCM_APP_BASE + GPADC_CLK_REG)
#define CCMU_UART_BGR_REG        (SUNXI_CCM_APP_BASE + BUS_CLK_GATING0_REG)
#define CCMU_UART_RST_REG        (SUNXI_CCM_APP_BASE + BUS_Reset0_REG)
#define CCMU_BUS_CLK_GATING0_REG (SUNXI_CCM_APP_BASE + BUS_CLK_GATING0_REG)
#define CCMU_BUS_Reset0_REG      (SUNXI_CCM_APP_BASE + BUS_Reset0_REG)

#define CCMU_SDMMC0_CLK_REG (SUNXI_CCM_APP_BASE + SMHC_CTRL0_CLK_REG)
#define CCMU_SDMMC1_CLK_REG (SUNXI_CCM_APP_BASE + SMHC_CTRL1_CLK_REG)
#define CCMU_SMHC0_BGR_REG_GATING   (SUNXI_CCM_APP_BASE + BUS_CLK_GATING1_REG)
#define CCMU_SMHC0_BGR_REG_RESET   (SUNXI_CCM_APP_BASE + BUS_Reset1_REG)

#define SMHC0_BGR_REG_SMHC0_GATING_OFFSET (BUS_CLK_GATING1_REG_SMHC0_HCLK_EN_OFFSET)
#define SMHC0_BGR_REG_SMHC0_RST_OFFSET (BUS_Reset1_REG_HRESETN_SMHC0_SW_OFFSET)

#define TRNG_GATING_CLK_EN_OFFSET	BUS_CLK_GATING0_REG_TRNG_PCLK_EN_OFFSET
#define TRNG_RESET_CLK_EN_OFFSET	BUS_Reset0_REG_PRESETN_TRNG_SW_OFFSET

#define GATING_RESET_SHIFT	 (4)

#define	SPI_CLK_PLL_PERI0	(307000000)
#define SPI_GATING_RESET_SHIFT		(4)

/* TWI */
#define TWI_NR_MASTER				(3)
#define SUNXI_TWI0_GATE_BASE		(SUNXI_CCM_APP_BASE + BUS_CLK_GATING0_REG)
#define SUNXI_TWI0_GATE_BIT			(BUS_CLK_GATING0_REG_TWI0_PCLK_EN_OFFSET)
#define SUNXI_TWI0_RST_BASE			(SUNXI_CCM_APP_BASE + BUS_Reset0_REG)
#define SUNXI_TWI0_RST_BIT			(BUS_Reset0_REG_PRESETN_TWI0_SW_OFFSET)

#define SUNXI_TWI1_GATE_BASE		(SUNXI_CCM_APP_BASE + BUS_CLK_GATING1_REG)
#define SUNXI_TWI1_GATE_BIT			(BUS_CLK_GATING1_REG_TWI1_PCLK_EN_OFFSET)
#define SUNXI_TWI1_RST_BASE			(SUNXI_CCM_APP_BASE + BUS_Reset1_REG)
#define SUNXI_TWI1_RST_BIT			(BUS_Reset1_REG_PRESETN_TWI1_SW_OFFSET)

#define SUNXI_TWI2_GATE_BASE		(SUNXI_CCM_APP_BASE + BUS_CLK_GATING1_REG)
#define SUNXI_TWI2_GATE_BIT			(BUS_CLK_GATING1_REG_TWI2_PCLK_EN_OFFSET)
#define SUNXI_TWI2_RST_BASE			(SUNXI_CCM_APP_BASE + BUS_Reset1_REG)
#define SUNXI_TWI2_RST_BIT			(BUS_Reset1_REG_PRESETN_TWI2_SW_OFFSET)

/* E907 */
#define CCMU_E907_CFG_RST           (0x1 << 0)
#define CCMU_E907_SYS_APB_RST       (0x1 << 1)
#define CCMU_E907_CFG_CLK_GATING    (0x1 << 0)
#define CCMU_E907_RSTN_REG          (SUNXI_CCM_APP_BASE + E907_RSTN_REG)
#define E907_CFG_BASE               (0x43030000)
#define E907_STA_ADD_REG            (E907_CFG_BASE + 0x0204)

/* a27l2 */
#define CCMU_A27L2_MTCLK_REG          (SUNXI_CCM_APP_BASE + A27L2_MT_Clock_REG)
#define CCMU_A27L2_MTCLK_EN           (A27L2_MT_CLK_EN_CLOCK_IS_ON << REG_A27L2_MT_CLK_EN_OFFSET)
#define H_MTIME_REG                   (SUNXI_PLMT_BASE + 0x4)
#define L_MTIME_REG                   (SUNXI_PLMT_BASE)

//* SPIF clock bit field */
#define CCM_SPIF_CTRL_M(x)		((x) - 1)
#define CCM_SPIF_CTRL_N(x)		((x) << 16)
#define CCM_SPIF_CTRL_HOSC		(0x0 << 24)
#define CCM_SPIF_CTRL_PERI512M		(0x1 << 24)
#define CCM_SPIF_CTRL_PERI384M		(0x2 << 24)
#define CCM_SPIF_CTRL_PERI307M		(0x3 << 24)
#define CCM_SPIF_CTRL_ENABLE		(0x1 << 31)
#define GET_SPIF_CLK_SOURECS(x)		(x == CCM_SPIF_CTRL_PERI512M ? 512000000 : 384000000)
#define CCM_SPIF_CTRL_PERI		CCM_SPIF_CTRL_PERI384M
#define SPIF_GATING_RESET_SHIFT		(5)

#endif
