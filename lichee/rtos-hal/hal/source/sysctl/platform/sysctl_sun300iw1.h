/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.


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

#ifndef SYSCTL_SUN300IW1_H
#define SYSCTL_SUN300IW1_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_status.h>
typedef hal_status_t HAL_Status;

#define SYSCTL_BASE         (0x43000000U)

/**
 * @brief SYSCTL register block structure
 */
typedef struct {
         uint32_t RESERVED0;                   /* offset: 0x00 */
         uint32_t SRAM_CTRL_REG;               /* offset: 0x04 */
         uint32_t RESERVED1[7];                /* offset: 0x08 */
         uint32_t VER_REG;                     /* offset: 0x24 */
         uint32_t RESERVED2[2];                /* offset: 0x28 */
         uint32_t EMAC_EPHY_CLK_REG0;          /* offset: 0x30 */
         uint32_t RESERVED3[5];                /* offset: 0x34 */
         uint32_t SRAM_TEST_REG[2];            /* offset: 0x48 */
         uint32_t RESERVED4[8];                /* offset: 0x50 */
         uint32_t BUS_RDY_TIMEOUT_CTRL_REG[4]; /* offset: 0x70 */
         uint32_t BUS_RDY_TIMEOUT_INT_STA_REG; /* offset: 0x80 */
         uint32_t BUS_RDY_TIMEOUT_INT_EN_REG;  /* offset: 0x84 */
         uint32_t RESERVED5[2];                /* offset: 0x88 */
         uint32_t TEST_DBG_REG[6];             /* offset: 0x90 */
         uint32_t RESERVED6[2];                /* offset: 0xA8 */
         uint32_t GENRAL_DBG_REG[2];           /* offset: 0xB0 */
         uint32_t RESERVED7[2];                /* offset: 0xB8 */
         uint32_t GENRAL_SIM_REG[2];           /* offset: 0xC0 */
         uint32_t RESERVED8[2];                /* offset: 0xC8 */
         uint32_t TRUST_BOOT_SEL_REG;          /* offset: 0xD0 */
         uint32_t RESERVED9[3];               /* offset: 0xD4 */
         uint32_t SRAM_ADDR_TWIST_REG;         /* offset: 0xE0 */
         uint32_t RESERVED10;                  /* offset: 0xE4 */
         uint32_t PS_CTRL_REG;                 /* offset: 0xE8 */
         uint32_t PS_CNT_REG;                  /* offset: 0xEC */
         uint32_t RESERVED11[20];              /* offset: 0xF0 */
         uint32_t CPUS_REGION_SRAM_TEST_REG;   /* offset: 0x140 */
         uint32_t RESERVED12[7];               /* offset: 0x144 */
         uint32_t RESCAL_CTRL_REG;             /* offset: 0x160 */
         uint32_t RES240_CTRL_REG[2];          /* offset: 0x164 */
         uint32_t RESCAL_STATUS_REG;           /* offset: 0x16C */
         uint32_t RESERVED13[4];               /* offset: 0x170 */
         uint32_t SIP_IO_MODE_REG;             /* offset: 0x180 */
         uint32_t SIP_IO_DRV0_REG;             /* offset: 0x184 */
         uint32_t SIP_IO_DRV1_REG;             /* offset: 0x188 */
         uint32_t SIP_IO_PULL0_REG;            /* offset: 0x18C */
         uint32_t SIP_IO_PULL1_REG;            /* offset: 0x190 */
} SYSCTL_T;


typedef enum {
	SYSCTRL_LA_MODE_NORMAL = 0,
	SYSCTRL_LA_MODE_WLAN = 1,
	SYSCTRL_LA_MODE_BTCORE_MAIN = 2,
	SYSCTRL_LA_MODE_BTCORE_CO = 3,
} SYSCTRL_LaModeSel;

#define SYSCTL ((SYSCTL_T *)SYSCTL_BASE) /* address: 0x43000000 */

/* SYSCTL->SRAM_CTRL */
#define SYSCTRL_SRAM_BOOT_MODE_EN_BIT HAL_BIT(24)

#define SYSCTRL_SRAM_LA_MODE_SEL_SHIFT (0)
#define SYSCTRL_SRAM_LA_MODE_SEL_VMASK (0x3U)

/* SYSCTL->BUS_RDY_TIMEOUT_CTRL */
#define SYSCTRL_BUS_TIMEOUT_INT_CNT_VMASK  (0x3FU)

typedef enum {
	SYSCTRL_BUS_TIMEOUT_INTERVAL0, /* interval_value*2^10+2^10-1 */
	SYSCTRL_BUS_TIMEOUT_INTERVAL1, /* interval_value*2^16+2^16-1 */
	SYSCTRL_BUS_TIMEOUT_INVALID,
} SYSCTRL_BusTmIntervalSel;

/* SYSCTL->TEST_DBG[0] */
#define SYSCTRL_DBG_CLK_GRP_SEL_SHIFT (8)
#define SYSCTRL_DBG_CLK_GRP_SEL_VMASK (0xFU)
#define SYSCTRL_DBG_CLK_DIV_SHIFT (12)
#define SYSCTRL_DBG_CLK_DIV_VMASK (0x3U)
#define SYSCTRL_DAP_SEL_SHIFT (30)
#define SYSCTRL_DAP_SEL_VMASK (0x3U)

typedef enum {
	SYSCTRL_DBGCLK_PLL,
	SYSCTRL_DBGCLK_DCXO,
	SYSCTRL_DBGCLK_PERI24M,
	SYSCTRL_DBGCLK_SYS32K,
	SYSCTRL_DBGCLK_USB_PYH_PLL,
	SYSCTRL_DBGCLK_RCOSC_1M,
	SYSCTRL_DBGCLK_LOSC_32K,
	SYSCTRL_DBGCLK_MCLK,
	SYSCTRL_DBGCLK_E907CLK,
	SYSCTRL_DBGCLK_HCLK,
	SYSCTRL_DBGCLK_PCLK0,
	SYSCTRL_DBGCLK_PCLK1,
	SYSCTRL_DBGCLK_A27L2CLK,
	SYSCTRL_DBGCLK_INVALID,
} SYSCTRL_DebugIOClkSel;

typedef enum {
	SYSCTRL_DBGCLK_DIV_1,
	SYSCTRL_DBGCLK_DIV_16,
	SYSCTRL_DBGCLK_DIV_64,
	SYSCTRL_DBGCLK_DIV_256,
	SYSCTRL_DBGCLK_DIV_INVALID,
} SYSCTRL_DebugIOClkDiv;

typedef enum {
	SYSCTRL_DBG_DAPSEL_E907JTAG,
	SYSCTRL_DBG_DAPSEL_A27L2JTAG,
	SYSCTRL_DBG_DAPSEL_EXPSPI,
	SYSCTRL_DBG_DAPSEL_INVALID,
} SYSCTRL_DAPSel;

/* SYSCTL->TEST_DBG[1~5] */
#define SYSCTRL_DBGSIGNAL_VE_SHIFT (0)
#define SYSCTRL_DBGSIGNAL_VE_VMASK (0xFFFFFFFFU)
#define SYSCTRL_DBGSIGNAL_USB0_SHIFT (0)
#define SYSCTRL_DBGSIGNAL_USB0_VMASK (0xFFFFU)
#define SYSCTRL_DBGSIGNAL_USB1_SHIFT (16)
#define SYSCTRL_DBGSIGNAL_USB1_VMASK (0xFFFFU)
#define SYSCTRL_DBGSIGNAL_ADIE_SHIFT (0)
#define SYSCTRL_DBGSIGNAL_ADIE_VMASK (0x3FU)
#define SYSCTRL_DBGSIGNAL_WLAN_SHIFT (6)
#define SYSCTRL_DBGSIGNAL_WLAN_VMASK (0xFFU)
#define SYSCTRL_DBGSIGNAL_PLL_LOCK_SHIFT (14)
#define SYSCTRL_DBGSIGNAL_PLL_LOCK_VMASK (0x7FU)
#define SYSCTRL_DBGSIGNAL_SPIF_SHIFT1 (21)
#define SYSCTRL_DBGSIGNAL_SPIF_VMASK1 (0x7FFU)
#define SYSCTRL_DBGSIGNAL_SPIF_SHIFT2 (0)
#define SYSCTRL_DBGSIGNAL_SPIF_VMASK2 (0x1FU)

/* common define and typedef */
#define SYSCTRL_BUS_TYPE_NUM (6)

/* these offset are related TIMEOUT_EN bitfield */
#define SYSCTRL_BUS_TIMEOUT_INT_SEL_OFFSET (7)
#define SYSCTRL_BUS_TIMEOUT_INT_CNT_OFFSET (1)
#define SYSCTRL_BUS_TIMEOUT_EN_OFFSET      (0)

#define SYSCTRL_BUS_TIMEOUT_SH0_SHIFT  (0)
#define SYSCTRL_BUS_TIMEOUT_SH2_SHIFT  (16)
#define SYSCTRL_BUS_TIMEOUT_SH3_SHIFT  (24)
#define SYSCTRL_BUS_TIMEOUT_SH4_SHIFT  (0)
#define SYSCTRL_BUS_TIMEOUT_SP0_SHIFT  (0)
#define SYSCTRL_BUS_TIMEOUT_SP1_SHIFT  (8)

/** @brief Type define of Bus Timeout IRQ callback function */
typedef void (*SYSCTRL_BusTmIRQCallback)(void *arg);

typedef enum {
	SYSCTRL_BUS_SH0 = 0U,
	SYSCTRL_BUS_SH2,
	SYSCTRL_BUS_SH3,
	SYSCTRL_BUS_SH4,
	SYSCTRL_BUS_SP0,
	SYSCTRL_BUS_SP1,
	SYSCTRL_BUS_NUM,
} SYSCTRL_BusType;

typedef enum {
	SYSCTRL_BUS_CTRL_BIT_POS_SH0 = SYSCTRL_BUS_TIMEOUT_SH0_SHIFT,
	SYSCTRL_BUS_CTRL_BIT_POS_SH2 = SYSCTRL_BUS_TIMEOUT_SH2_SHIFT,
	SYSCTRL_BUS_CTRL_BIT_POS_SH3 = SYSCTRL_BUS_TIMEOUT_SH3_SHIFT,
	SYSCTRL_BUS_CTRL_BIT_POS_SH4 = 32U + SYSCTRL_BUS_TIMEOUT_SH4_SHIFT,
	SYSCTRL_BUS_CTRL_BIT_POS_SP0 = 64U + SYSCTRL_BUS_TIMEOUT_SP0_SHIFT,
	SYSCTRL_BUS_CTRL_BIT_POS_SP1 = 64U + SYSCTRL_BUS_TIMEOUT_SP1_SHIFT,
	SYSCTRL_BUS_CTRL_BIT_POS_INVALID,
} SYSCTRL_BusCtrlBitPos;

typedef enum {
	SYSCTRL_BUS_INT_BIT_POS_SH0 = 0U,
	SYSCTRL_BUS_INT_BIT_POS_SH2 = 2U,
	SYSCTRL_BUS_INT_BIT_POS_SH3 = 3U,
	SYSCTRL_BUS_INT_BIT_POS_SH4 = 4U,
	SYSCTRL_BUS_INT_BIT_POS_SP0 = 8U,
	SYSCTRL_BUS_INT_BIT_POS_SP1 = 9U,
	SYSCTRL_BUS_INT_BIT_POS_INVALID,
} SYSCTRL_BusInterruptBitPos;

typedef struct {
	SYSCTRL_BusTmIntervalSel interval_sel;
	uint32_t interval_cnt;
} SYSCTRL_BusTmParam;

/**
 * @brief debugio sigal mask parameters
 * @note each parameter is value mask whose every bit decides
 *       which sub-signal is output
 */
typedef struct {
	uint32_t ve_mask;       //value range: [0~0xffffffff]
	uint32_t usb0_mask;     //value range: [0~0xffff]
	uint32_t usb1_mask;     //value range: [0~0xffff]
	uint32_t adie_mask;     //value range: [0~0x3f]
	uint32_t wlan_mask;     //value range: [0~0xff]
	uint32_t pll_lock_mask; //value range: [0~0x7f]
	uint32_t spif_mask;     //value range: [0~0xffff]
} SYSCTRL_DebugIOSignalMaskParam;

HAL_Status HAL_SYSCTRL_BusTimeout_ConfigParam(SYSCTRL_BusType bus_type, SYSCTRL_BusTmParam *param);
HAL_Status HAL_SYSCTRL_BusTimeout_Enable(SYSCTRL_BusType bus_type, uint8_t en);
HAL_Status HAL_SYSCTRL_BusTimeout_EnableIRQ(SYSCTRL_BusType bus_type, SYSCTRL_BusTmIRQCallback cb, void *arg);
HAL_Status HAL_SYSCTRL_BusTimeout_DisableIRQ(SYSCTRL_BusType bus_type);
void HAL_SYSCTRL_DebugIO_ConfigClkOutput(SYSCTRL_DebugIOClkSel clk_sel, SYSCTRL_DebugIOClkDiv clk_div);
void HAL_SYSCTRL_DebugIO_ConfigSignalOutput(SYSCTRL_DebugIOSignalMaskParam *param);
void HAL_SYSCTRL_DCU_SetDAP(SYSCTRL_DAPSel dap_sel);
void HAL_SYSCTRL_SramRemap_EnableBootMode(uint8_t en);
void HAL_SYSCTRL_SramRemap_SetLaMode(SYSCTRL_LaModeSel la_sel);

#ifdef __cplusplus
}
#endif
#endif  /*SYSCTL_SUN300IW1_H*/
