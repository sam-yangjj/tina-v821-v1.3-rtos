/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "platform_sun60iw1.h"
#include "platform_irq.h"

const char *irq_major_string[PLAT_CLIC_IRQ_CNT] = {
    [0 ... 2] = NULL,
    "Machine_Software",
    [4 ... 6] = NULL,
    "CORET",
    [8 ... 10] = NULL,
    "Machine_External",
    [12 ... 15] = NULL,
    "E906_WDG",
    "E906_MBOX",
    "RISCV_TIMER0",
    "RISCV_TIMER1",
    "RISCV_TIMER2",
    "RISCV_TIMER3",
    "CPUX_MBOX_IRQ_RISCV",
    "CPUX_MBOX_IRQ_CPUS",
    "SPLOCK_IRQ",
    "CPUX_MBOX_IRQ_DSP",
    [26] = NULL,
    "DMAC_IRQ1_NS",
    "DMAC_IRQ1_S",
    "DMAC1_IRQ1_NS",
    "DMAC1_IRQ1_S",
    "INT_SCRI9",
    "INT_SCRI10",
    "INT_SCRI11",
    "INT_SCRI12",
    "INT_SCRI13",
    "INT_SCRI14",
    "INT_SCRI15",
    "INT_SCRI16",
    "INT_SCRI17",
    "INT_SCRI18",
    "INT_SCRI19",
    "INT_SCRI20",
    "INT_SCRI21",
    "INT_SCRI22",
    "INT_SCRI23",
    "INT_SCRI24",
    "INT_SCRI25",
    "INT_SCRI26",
    "INT_SCRI27",
    "INT_SCRI28",
    "INT_SCRI29",
    "INT_SCRI30",
    "INT_SCRI31",
    "INT_SCRI32",
    "INT_SCRI33",
    "INT_SCRI34",
    "INT_SCRI35",
    "INT_SCRI36",
    "INT_SCRI37",
    "INT_SCRI38",
    "INT_SCRI39",
    "INT_SCRI40",
    "INT_SCRI41",
    "INT_SCRI42",
    "INT_SCRI43",
    "INT_SCRI44",
    "INT_SCRI45",
    "INT_SCRI46",
    "INT_SCRI47",
    "INT_SCRI48",
    [71 ... 84] = NULL,
    "MCU_TBU_IRQ",
    "NMI_IRQ",
    "R_PPU",
    "R_TWD",
    "R_WDT",
    "R_TIMER0",
    "R_TIMER1",
    "R_TIMER2",
    "RTC_ALARM",
    "GPIOL_S",
    "GPIOL_NS",
    "GPIOM_S",
    "GPIOM_NS",
    "R_UART0",
    "R_UART1",
    "R_TWI0",
    "R_TWI1",
    "R_TWI2",
    "R_CAN0_TOP",
    "R_UART2",
    "R_IRRX",
    "R_PWM",
    "R_TZMA",
    "CPUS_AHBS_TIMEOUT",
    "R_SPI",
    "R_SRAMECC_IRQ",
    "R_MBOX_CPUX_W",
    "R_MBOX_DSP_W",
    "R_MBOX_RV_W",
    "PCK600_CPU",
    "R_TIMER3",
    "R_CAN0_0",
    "R_CAN0_1",
    "DOUBLE_EXCEPTION_ERROR",
    "P_FATAL_ERROR",
    "DSP_WDG_IRQ",
    "DSP_MBOX_IRQ_CPUX",
    "DSP_MBOX_IRQ_CPUS",
    "DSP_MBOX_IRQ_RISCV",
    "TZMA0_IRQ",
    "DSP_TIMER0_IRQ",
    "DSP_TIMER1_IRQ",
    "DSP_TIMER2_IRQ",
    "DSP_TIMER3_IRQ",
    "MCU_DMACIRQ_DSP_NS",
    "MCU_DMACIRQ_DSP_S",
    "MCU_DMACIRQ_S",
    "MCU_DMACIRQ_NS",
    "MCU_TZMA",
    "MCU_TZMA1",
    "MCU_I2S_IRQ",
    "MCU_DMIC_IEQ",
    "MCU_SPLOCK_IRQ",
    "MCU_AHBS_TIMEOUT_IRQ",
    "PLL_AUDIO1_UNLOCK_IRQ",
    "MSI_LITE2_IRQ",
    "PUB_SRAM0_ECC_IRQ",
    "PUB_SRAM1_ECC_IRQ",
    "DSP_AHBS_TIMEOUT"
};

#ifdef PLAT_HAS_NON_ROOT_IC
const char *irq_group_string[PLAT_NON_ROOT_IRQ_CONTROLLER_CNT * PLAT_NON_ROOT_IC_IRQ_CNT] = {
    /* group9 gic 34-39 */
    NULL,          NULL,           "UART0",                "UART1",            "UART2",            "UART3",            "UART4",          "UART5",
    /* group10  gic 40-47 */
    "UARt6",       "UART7",        "UART8",                "TWI0",             "TWI1",             "TWI2",             "TWI3",           "TWI4",
    /* group11  gic 48-55 */
    "TWI5",        "TWI6",         "TWI7",                 "TWI8",             "SPI0",             "SPI1",             "SPI2",           "PWM0",
    /* group12 gic 56-63 */
    "SPI_FLASH",   "SMHC3",        "SPI3",                 "SPI4",             "SPI5",             "IR_TX",            "IR_RX",          "LEDC",
    /* group13 gic 64-71 */
    "PLL_UNLOCK",  "SPI6",         NULL,                   "VE_ENC0",          "VE_ENC1",          "VE_DEC",           "USB3.1",         "LOCALBUS",
    /* group14 gic 72-79 */
    "UFS",         "NAND",         "UFS_FATAL",            "SMHC0",            "SMHC1",            "SMHC2",            "NSI",            "DMA1_CPUX_NS",
    /* group15 gic 80-82, 85-86 */
    "DMA1_CPUX_S", "CCU_FREE",     "AHB_HREADY_TIME_OUT",  NULL,               NULL,               "CE_NS",            "CE_S",           NULL,
    /* group16 gic 88-95 */
    "TIMER0",      "TIMER1",       "TIMER2",               "TIMER3",           "TIMER4",           "TIMER5",           "GPADC",          "THS",
    /* group17 gic 96-103 */
    "WDG0",        "PWM1",         "LRADC",                "GPIOA_NS",         "GPIOA_S",          "GPIOB_NS",         "GPIOB_S",        "GPIOC_NS",
    /* group18 gic 104-111 */
    "GPIOC_S",     "GPIOD_NS",     "GPIOD_S",              "GPIOE_NS",         "GPIOE_S",          "GPIOF_NS",         "GPIOF_S",        "GPIOG_NS",
    /* group19 gic 112-119 */
    "GPIOG_S",     "GPIOH_NS",     "GPIOH_S",              "GPIOI_NS",         "GPIOI_S",          "GPIOJ_NS",         "GPIOJ_S",        "GPIOK_NS",
    /* group20 gic 120-127 */
    "GPIOK_S",     "DE0",          "DE1",                  "G2D",              "DI",               "DE_FREEZE",        "EDP",            "TCON0_LCD0",
    /* group21 gic 128-135 */
    "TCON0_LCD1",  "TCON0_LCD2",   "TCON0_LCD3",           "TV0",              "TV1",              "TV2",              "DSI0",           "DSI1",
    /* group22 gic 136-143 */
    "HDMI",        "GPU",          "GPU_DVFS",             "GPU_OS0",          "GPU_OS1",          "GPU_0S2",          "GPU_OS3",        NULL,
    /* group23 gic 144-151 */
    NULL,          NULL,           NULL,                   "CSI_DMA0",         "CSI_DMA1",         "CSI_DMA2",         "CSI_DMA3",       "CSI_DMA4",
    /* group24 gic 152-159 */
    "CSI_DMA5",    "CSI_DMA6",     "CSI_DMA7",             "CSI_DMA8",         "CSI_DMA9",         "CSI_DMA10",        "CSI_DMA11",      "CSI_VIPP0",
    /* group25 gic 160-167 */
    "CSI_VIPP1",   "CSI_VIPP2",    "CSI_VIPP3",            "CSI_VIPP4",        "CSI_VIPP5",        "CSI_VIPP6",        "CSI_VIPP7",      "CSI_VIPP8",
    /* group26 gic 168-175 */
    "CSI_VIPP9",   "CSI_VIPP10",   "CSI_VIPP11",           "CSI_PARSER0",      "CSI_PARSER1",      "CSI_PARSER2",      "CSI_PARSER3",    "CSI_ISP0",
    /* group27 gic 176-183 */
    "CSI_ISP1",    "CSI_ISP2",     "CSI_ISP3",             "CSI_CMB",          "CSI_TDM",          "CSI_TOP_PKT",      "PCIE0_EDMA0",    "PCIE0_EDMA1",
    /* group28 gic 184-191 */
    "PCIE0_EDMA2", "PCIE0_EDMA3",  "PCIE0_EDMA4",          "PCIE0_EDMA5",      "PCIE0_EDMA6",      "PCIE0_EDMA7",      "PCIE0_SII",      "PCIE0_MSI",
    /* group29 gic 192-199 */
    "PCIE0_EDMA8", "PCIE0_EDMA9",  "PCIE0_EDMA10",         "PCIE0_EDMA11",     "PCIE0_EDMA12",     "PCIE0_EDMA13",     "PCIE0_EDMA14",   "PCIE0_EDMA15",
    /* group30 gic 200-207 */
    "PCIE1_EDMA0", "PCIE1_EDMA1",  "PCIE1_EDMA2",          "PCIE1_EDMA3",      "PCIE1_EDMA4",      "PCIE1_EDMA5",      "PCIE1_EDMA6",    "PCIE1_EDMA7",
    /* group31 gic 208-215 */
    "PCIE1_SII",   "PCIE1_MSI",    "PCIE1_EDMA8",          "PCIE1_EDMA9",      "PCIE1_EDMA10",     "PCIE1_EDMA11",     "PCIE1_EDMA12",   "PCIE1_EDMA13",
    /* group32 gic 216-223 */
    "PCIE1_EDMA14","PCIE1_EDMA15", "COMB_PHY0_LANE0",      "COMB_PHY0_LANE1",  "COMB_PHY0_LANE2",  "COMB_PHY0_LANE3",  "COMB_PHY1_LANE0","COMB_PHY1_LANE1",
    /* group33 gic 224-231 */
    "USB_OTG",     "USB_EHCI",     "USB_OHCI",             "USB1_EHCI",        "USB1_OHCI",        "USB2_EHCI",        "USB2_OHCI",      "MEMC0_DERATE",
    /* group34 gic 232-239 */
    "A76_MBOX_R",  "A76_MBOX_R",   "A55_MBOX_W",           "A55_MBOX_W",       "MEMC0_SMC0",       "MEMC0_SMC1",       "MEMC0_DFS",      "MEMC0_ECC0",
    /* group35 gic 240-247 */
    "MEMC0_ECC1",  NULL,           NULL,                   "MEMC0_AP_ECC0",    "MEMC0_AP_ECC1",    NULL,               NULL,             "DDRPHY",
    /* group36 gic 248-255 */
    "TIMER6",      "TIMER7",       "TIMER8",               "TIMER9",           "TIMER10",          "TIMER11",          "TIMER12",        "TIMER13",
    /* group37 gic 256-263 */
    "TIMER14",     "TIMER15",      "MEMC0_SRAM_ECC0",      "MEMC0_SRAM_ECC1",  NULL,               "WDG1",             "I2S1",           "I2S2",
    /* group38 gic 264-271 */
    "I2S3",        "I2S4",         "I2S5",                 "SPDIF",            "SMMU_TCU",         "SMMU_TBU_ISP",     "SMMU_TBU_CSI0",  "SMMU_TBU_CSI1",
    /* group39 gic 272-279 */
    "SMMU_TBU_DI", "SMMU_TBU_G2D", "SMMU_TBU_VE_ENC0",     "SMMU_TBU_VE_ENC1", "SMU_TBU_VE_DEC0",  "SMMU_TBU_VE_DEC1", "SMMU_TBU_DE0",   "SMMU_TBU_DE1",
    /* group40 gic 280-287 */
    "SMMU_TBU_GMAC0", "SMMU_TBU_GMAC1", "SMMU_TBU_USB3",   "SMMU_TBU_MSI0",    "SMU_TBU_MSI1",     "SMMU_TBU_PCIE0",   "SMMU_TBU_PCIE1", "SMMU_TBU_GPU0",
    /* group41 gic 288-295 */
    "SMMU_TBU_GPU1",  "SMMU_TBU_AIPU",  "SMMU_TBU_NPU",    "AIPU",             "NPU",              "NPU_TZMA_ERR",     "CAN0_0",         "CAN0_1",
    /* group42 gic 296-303 */
    "CAN0_TOP",    "CAN1_0",       "CAN1_1",               "CAN1_TOP",         "CAN2_0",           "CAN2_1",           "CAN2_TOP",       "GMAC0_TOP",
    /* group43 gic 304-311 */
    "GMAC0_SFTY",  "GMAC0_PWR_CLK_CTRL","GMAC0_PERCH_TX0", "GMAC0_PERCH_TX1",  "GMAC0_PERCH_TX2", "GMAC0_PERCH_TX3",  "GMAC0_PERCH_TX4","GMAC0_PERCH_TX5",
    /* group44 gic 312-319 */
    "GMAC0_PERCH_TX6","GMAC0_PERCH_TX7","GMAC0_PERCH_RX0", "GMAC0_PERCH_RX1",  "GMAC0_PERCH_RX2", "GMAC0_PERCH_RX3",  "GMAC0_PERCH_RX4","GMAC0_PERCH_RX5",
    /* group45 gic 320-327 */
    "GMAC0_PERCH_RX6","GMAC0_PERCH_RX7","GMAC1_TOP",       "GMAC1_SFTY",       "GMAC0_PWR_CLK_CTRL","GMAC1_PERCH_TX0","GMAC1_PERCH_TX1","GMAC1_PERCH_TX2",
    /* group46 gic 328-335 */
    "GMAC1_PERCH_TX3","GMAC1_PERCH_TX4","GMAC1_PERCH_TX5", "GMAC1_PERCH_TX6",  "GMAC1_PERCH_TX7", "GMAC1_PERCH_RX0",  "GMAC1_PERCH_RX1","GMAC1_PERCH_RX2",
    /* group47 gic 336-343 */
    "GMAC1_PERCH_RX3","GMAC1_PERCH_RX4","GMAC1_PERCH_RX5", "GMAC1_PERCH_RX6",  "GMAC1_PERCH_RX7", "GPADC1",           "MEMC_SBR",       "NMI",
    /* group48 gic 344-347 */
    "PCK600_QCHANNEL","R_TWD",          "R_WDG0",          "R_TIMER0",         NULL,              NULL,               NULL,             NULL,

};
#endif

