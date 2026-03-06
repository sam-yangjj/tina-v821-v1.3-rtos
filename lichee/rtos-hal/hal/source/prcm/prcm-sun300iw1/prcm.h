/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DRIVER_CHIP_HAL_PRCM_H_
#define _DRIVER_CHIP_HAL_PRCM_H_

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Bitwise operation
 */
#define HAL_BIT(pos)                        (1U << (pos))

#define HAL_SET_BIT(reg, mask)              ((reg) |= (mask))
#define HAL_CLR_BIT(reg, mask)              ((reg) &= ~(mask))
#define HAL_GET_BIT(reg, mask)              ((reg) & (mask))
#define HAL_GET_BIT_VAL(reg, shift, vmask)  (((reg) >> (shift)) & (vmask))

#define HAL_MODIFY_REG(reg, clr_mask, set_mask) \
    ((reg) = (((reg) & (~(clr_mask))) | (set_mask)))

#ifndef HAL_ASSERT_PARAM
#define HAL_ASSERT_PARAM(x)  do { if (!(x)) while (1); } while (0)
#endif

/*
 * Macros for accessing LSBs of a 32-bit register (little endian only)
 */
#define HAL_REG_32BIT(reg_addr)  (*((__IO uint32_t *)(reg_addr)))
#define HAL_REG_16BIT(reg_addr)  (*((__IO uint16_t *)(reg_addr)))
#define HAL_REG_8BIT(reg_addr)   (*((__IO uint8_t  *)(reg_addr)))


#ifdef __cplusplus
  #define   __I     volatile             /*!< Defines 'read only' permissions */
#else
  #define   __I     volatile const       /*!< Defines 'read only' permissions */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions */
#define     __IO    volatile             /*!< Defines 'read / write' permissions */



/* auto generate name by macro __LINE__ */
#define RESERVED_HELPER(a,b)  a##b
#define RESERVED_HELP(a, b)   RESERVED_HELPER(a, b)
#define RESERVED              RESERVED_HELP(RES, __LINE__)


#ifdef CONFIG_FPGA_PLATFORM
#define LOSC_CLOCK          (32000U)
#else
#define LOSC_CLOCK          (32768U)
#endif

#define SYS_LFCLOCK         LOSC_CLOCK


/* prcm system module control */
typedef struct {
    __IO uint32_t LPCLK_CTRL;               /* offset: 0x0000 Low Frequency Clock Control register */
    __IO uint32_t LPCLK_STATUS;             /* offset: 0x0004 Low Frequency Clock Status register */

         uint32_t RESERVED[1];

    __IO uint32_t RCOSC_CALIB_REG0;         /* offset: 0x000c RCOSC calibration control register0 */
    __IO uint32_t RCOSC_CALIB_REG1;			/* offset: 0x0010 RCOSC calibration control register1 */
    __IO uint32_t RCOSC_CALIB_REG2;     	/* offset: 0x0014 RCOSC calibration control register2 */
    __IO uint32_t RCOSC_CALIB_REG3;     	/* offset: 0x0018 RCOSC calibration control register3 */
	__IO uint32_t RESET_CTRL;               /* offset: 0x001c Reset Control Register */
	__IO uint32_t POR_RST_CNT;              /* offset: 0x0020 POR RESET CNT Register */
	__IO uint32_t POR_RST_CTRL;             /* offset: 0x0024 POR RESET CTRL Register */
	__IO uint32_t OTP_CTRL;					/* offset: 0x0028 OTP Control Register */
	__IO uint32_t VCC33_DET_IRQ;			/* offset: 0x002c VCC33 DET IRQ Register */
	__IO uint32_t RF1PRAM_CFG;				/* offset: 0x0030 RF1PRAM Configuration Register */
	__IO uint32_t RF2PRAM_CFG;				/* offset: 0x0034 RF2PRAM Configuration Register */
	__IO uint32_t RFP2PRAM_CFG;				/* offset: 0x0038 RFP2PRAM Configuration Register */
	__IO uint32_t SRAM_CFG;					/* offset: 0x003c SRAM Configuration Register */
	__IO uint32_t ROM_CFG;					/* offset: 0x0040 ROM Configuration Register */

         uint32_t RESERVED[3];

	__IO uint32_t RTC_IO_WAKE_EN;			/* offset: 0x0050 RTC IO Wakeup Enable */
	__IO uint32_t RTC_IO_WAKE_DEB_CLK;		/* offset: 0x0054 RTC IO Wakeup Debounce clock */
	__IO uint32_t RTC_IO_WAKE_ST;			/* offset: 0x0058 RTC IO wakeup status register */
    __IO uint32_t RTC_IO_HOLD_CTRL;         /* offset: 0x005c RTC IO hold control register */

    __IO uint32_t RTC_IO_WUP_GEN;           /* offset: 0x0060 RTC IO wakeup global enable register */
    __IO uint32_t RTC_IO_WUP_DEB_CYCLES0;   /* offset: 0x0064 RTC IO wakeup debouce clock cycles 0 register */
    __IO uint32_t RTC_IO_WUP_DEB_CYCLES1;   /* offset: 0x0068 RTC IO wakeup debouce clock cycles 1 register */

         uint32_t RESERVED[1];

    __IO uint32_t PWR_EN_CFG;				/* offset: 0x0070 Power en pin Control register */

         uint32_t RESERVED[3];

	__IO uint32_t CRY_CONFIG_REG;           /* offset: 0x0080 Crypt Configuration Register */
	__IO uint32_t CRY_KEY_REG;				/* offset: 0x0084 Crypt Key Register */
	__IO uint32_t CRY_EN_REG;				/* offset: 0x0088 Crypt Enable Register */

         uint32_t RESERVED[1];

	__IO uint32_t SID_HW_LOCK_CTRL;			/* offset: 0x0090 SID HW Lock Control Register */

         uint32_t RESERVED[3];

	__IO uint32_t MCO_CTRL;					/* offset: 0x0100 MCO Control Register */
	__IO uint32_t DCXO_EN_DLY_CNT_REG;		/* offset: 0x0104 DCXO Enable Delay Count Register */

         uint32_t RESERVED[33];

    __IO uint32_t WLAN_HIF_OV_CTRL;         /* offset: 0x018c WLAN HIF override control register */

         uint32_t RESERVED[12];

    __IO uint32_t BOOT_FLAG;				/* offset: 0x01c0 APPCPU Boot Flag register */
    __IO uint32_t BOOT_ADDR;				/* offset: 0x01c4 APPCPU Boot Address register */
    __IO uint32_t BOOT_ARG;					/* offset: 0x01c8 APPCPU Boot Argument register */

         uint32_t RESERVED[1];

    __IO uint32_t UPGRADE_ARG;				/* offset: 0x01d0 Upgrade Flag register */
    __IO uint32_t RESET_SRC_STATUS;			/* offset: 0x01d4 Reset Source Status register */

         uint32_t RESERVED[10];

    __IO uint32_t SYSTEM_PRIV_REG0T7[8];    /* offset: 0x0200~0x021c, system private register. */

         uint32_t RESERVED[8];

    __IO uint32_t BROM_PRIV_REG0T7[8];		/* offset: 0x0240~0x025c, BROM private register. */
} PRCM_T;

#define PRCM_BASE      0x4a000000
#define PRCM           ((PRCM_T *)PRCM_BASE)

/*
 * bit field definition of PRCM->LPCLK_CTRL 0x0000
 */
#define PRCM_SYS_32K_SEL_SHIFT		24              /* R/W */
#define PRCM_SYS_32K_SEL_MASK		(0x1U << PRCM_SYS_32K_SEL_SHIFT)
typedef enum {
        PRCM_SYS_32K_SEL_LPCLK		= (0x0U << PRCM_SYS_32K_SEL_SHIFT),
        PRCM_SYS_32K_SEL_RCCAL		= (0x1U << PRCM_SYS_32K_SEL_SHIFT)
} PRCM_SYS32KSrc;

#define PRCM_LPCLK_32K_SEL_SHIFT    20              /* R/W */
#define PRCM_LPCLK_32K_SEL_MASK     (0x1U << PRCM_LPCLK_32K_SEL_SHIFT)
typedef enum {
        PRCM_LPCLK_32K_SEL_RCDIV = (0x0U << PRCM_LPCLK_32K_SEL_SHIFT),
        PRCM_LPCLK_32K_SEL_XO32K = (0x1U << PRCM_LPCLK_32K_SEL_SHIFT)
} PRCM_LPCLK32KSrc;

#define PRCM_RCO_TRIM_MAX           0xFF
#define PRCM_RCO_TRIM_SHIFT         12
#define PRCM_RCO_TRIM_MASK          (0XFFU << PRCM_RCO_TRIM_SHIFT)
#define PRCM_RCO_TRIM_VAL(v)        (((v) * 0xFFU) << PRCM_RCO_TRIM_SHIFT)

#define PRCM_XO32K_CL_TRIM_MAX      0x3F
#define PRCM_XO32K_CL_TRIM_SHIFT    4
#define PRCM_XO32K_CL_TRIM_MASK     (0X3FU << PRCM_XO32K_CL_TRIM_SHIFT)
#define PRCM_XO32K_CL_TRIM_VAL(v)	( ((v) & 0x3FU) << PRCM_XO32K_CL_TRIM_SHIFT)

#define PRCM_XO32K_EN_BIT			HAL_BIT(2)

/*
 * bit field definition of PRCM->LPCLK_STATUS 0x0004
 */

#define PRCM_XO32K_READY_FLAG_BIT	HAL_BIT(0)

/*
 * bit field definition of
 *     - PRCM->RCOSC_CALIB_REG0 0x000c (R/W)
 *     - PRCM->RCOSC_CALIB_REG1 0x0010 (R/W)
 *     - PRCM->RCOSC_CALIB_REG2 0x0014 (R/W)
 *     - PRCM->RCOSC_CALIB_REG3 0x0018 (R/W)
 */
/* bit field definition of PRCM->RCOSC_CALIB_REG0 0x000c */
#define PRCM_BLE_RCOSC_CALIB_EN_BIT             HAL_BIT(29)
#define PRCM_BLE_RCOSC_CALIB_RST_PUL_BIT        HAL_BIT(28) /* write 1 to reset */
#define PRCM_RCOSC_CALIB_CS_SHIFT               24
#define PRCM_RCOSC_CALIB_CS_MASK                (0x7U << PRCM_RCOSC_CALIB_CS_SHIFT)
typedef enum {
	PRCM_RCOSC_CALIB_CS_IDLE                = (0x0U << PRCM_RCOSC_CALIB_CS_SHIFT),
	PRCM_RCOSC_CALIB_CS_WAIT_DCXO_READY     = (0x1U << PRCM_RCOSC_CALIB_CS_SHIFT),
	PRCM_RCOSC_CALIB_CS_WAIT_CAL_FINISH     = (0x3U << PRCM_RCOSC_CALIB_CS_SHIFT),
	PRCM_RCOSC_CALIB_CS_CAL_FINISH          = (0x6U << PRCM_RCOSC_CALIB_CS_SHIFT),
	PRCM_RCOSC_CALIB_CS_WAIT_TIMER          = (0x2U << PRCM_RCOSC_CALIB_CS_SHIFT),
} PRCM_RCOSC_CALIB_CurrentState;
#define PRCM_RCOSC_CALIB_SW_REQ_PUL_BIT         HAL_BIT(20)
#define PRCM_RCOSC_WK_TIME_EN_BIT               HAL_BIT(16)
#define PRCM_RCOSC_WK_TIME_SHIFT                0
#define PRCM_RCOSC_WK_TIME_MASK                 (0x01FFFU << PRCM_RCOSC_WK_TIME_SHIFT)
#define PRCM_RCOSC_WK_TIME_VAL(n)               (n << PRCM_RCOSC_WK_TIME_SHIFT)


/*
 * bit field definition of PRCM->RCOSC_CALIB_REG1 0x0010
 */
#define PRCM_RCOSC_SCALE_PHASE2_NUM_SHIFT       16
#define PRCM_RCOSC_SCALE_PHASE2_NUM_MASK        (0x0FU << PRCM_RCOSC_SCALE_PHASE2_NUM_SHIFT)
#define PRCM_RCOSC_SCALE_PHASE2_NUM_VAL(n)      (n << PRCM_RCOSC_SCALE_PHASE2_NUM_SHIFT)
#define PRCM_RCOSC_SCALE_PHASE1_NUM_SHIFT       12
#define PRCM_RCOSC_SCALE_PHASE1_NUM_MASK        (0x0FU << PRCM_RCOSC_SCALE_PHASE1_NUM_SHIFT)
#define PRCM_RCOSC_SCALE_PHASE1_NUM_VAL(n)      (n << PRCM_RCOSC_SCALE_PHASE1_NUM_SHIFT)
#define PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT 8
#define PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_MASK  (0x07U << PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT)
typedef enum {
	PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_10     = (0x0U << PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_16     = (0x1U << PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_20     = (0x2U << PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_24     = (0x3U << PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_32     = (0x4U << PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_40     = (0x5U << PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_64     = (0x6U << PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_128    = (0x7U << PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_SHIFT),
} PRCM_RCOSC_SCALE_PHASE3_WkTimes;
#define PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT  4
#define PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_MASK   (0x07U << PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT)
typedef enum {
	PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_2      = (0x0U << PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_4      = (0x1U << PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_6      = (0x2U << PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_8      = (0x3U << PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_10     = (0x4U << PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_12     = (0x5U << PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_14     = (0x6U << PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT),
	PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_16     = (0x7U << PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_SHIFT),
} PRCM_RCOSC_SCALE_PHASE2_WkTimes;
#define PRCM_RCOSC_NORMAL_WK_TIMES_SEL_SHIFT    1
#define PRCM_RCOSC_NORMAL_WK_TIMES_SEL_MASK     (0x01U << PRCM_RCOSC_NORMAL_WK_TIMES_SEL_SHIFT)
typedef enum {
	PRCM_RCOSC_NORMAL_WK_TIMES_SEL_PHASE2   = (0x0U << PRCM_RCOSC_NORMAL_WK_TIMES_SEL_SHIFT),
	PRCM_RCOSC_NORMAL_WK_TIMES_SEL_PHASE3   = (0x1U << PRCM_RCOSC_NORMAL_WK_TIMES_SEL_SHIFT),
} PRCM_RCOSC_NormalWkTimesSel;
#define PRCM_RCOSC_WK_MODE_SEL_SHIFT            0
#define PRCM_RCOSC_WK_MODE_SEL_MASK             (0x01U << PRCM_RCOSC_WK_MODE_SEL_SHIFT)
typedef enum {
	PRCM_RCOSC_WK_MODE_SEL_SCALE            = (0x1U << PRCM_RCOSC_WK_MODE_SEL_SHIFT),
	PRCM_RCOSC_WK_MODE_SEL_NORMAL           = (0x0U << PRCM_RCOSC_WK_MODE_SEL_SHIFT),
} PRCM_RCOSC_WkModSel;

/*
 * bit field definition of PRCM->RCOSC_CALIB_REG2 0x0014
 */

#define PRCM_RCOSC_CNT_TARGET_SHIFT            0
#define PRCM_RCOSC_CNT_TARGET_MASK             (0xFFFFU << PRCM_RCOSC_CNT_TARGET_SHIFT)
#define PRCM_RCOSC_CNT_TARGET_VAL(v)		   ( ((v) & 0xFFFFU) << PRCM_RCOSC_CNT_TARGET_SHIFT)

/*
 * bit field definition of PRCM->RCOSC_CALIB_REG3 0x0018
 */

#define PRCM_RCOSC_DIVIDEND_SHIFT            0
#define PRCM_RCOSC_DIVIDEND_MASK             (0xFFFFFFFFU << PRCM_RCOSC_CNT_TARGET_SHIFT)
#define PRCM_RCOSC_DIVIDEND_VAL(v)			 (((v) & 0xFFFFFFFFU) << PRCM_RCOSC_DIVIDEND_SHIFT)

/*
 * bit field definition of PRCM->RESET_CTRL 0x001c
 */

#define PRCM_RCCAL_RSTN_SHIFT				 0
#define PRCM_RCCAL_RSTN_MASK				 0x1U << PRCM_RCCAL_RSTN_SHIFT
#define PRCM_RCCAL_RSTN_VAL(v)               (((v) & 0x01) << PRCM_RCCAL_RSTN_SHIFT)
#define PRCM_RTC_TIMER_SHIFT				 1
#define PRCM_RTC_TIMER_MASK					 0x1U << PRCM_RTC_TIMER_SHIFT
#define PRCM_RTC_TIMER_VAL(v)                (((v) & 0x01) << PRCM_RTC_TIMER_SHIFT)
#define PRCM_RTC_WDG_RSTN_SHIFT				 3
#define PRCM_RTC_WDG_RSTN_MASK				 0x1U << PRCM_RTC_WDG_RSTN_SHIFT
#define PRCM_RTC_WDG_RSTN_VAL(v)             (((v) & 0x01) << PRCM_RTC_WDG_RSTN_SHIFT)

typedef enum {
	PRCM_RESET_RCCAL = 0U,
	PRCM_RESET_RTC_TIMER,
	PRCM_RESET_RTC_WATCHDOG,
	PRCM_RESET_MAX,
} PRCM_ResetModule;

/*
 * bit field definition of PRCM->POR_RST_CNT and POR_RST_CTRL  0x0020 ~ 0x0024
 */

#define PRCM_VSYS_DET_RSTN_SHIFT           0
#define PRCM_VSYS_DET_RSTN_MASK            0x1U << PRCM_VSYS_DET_RSTN_SHIFT
typedef enum {
	PRCM_VSYS_DET_RST_ENABLE = 0x0U << PRCM_VSYS_DET_RSTN_SHIFT,
	PRCM_VSYS_DET_RST_BYPASS = 0x1U << PRCM_VSYS_DET_RSTN_SHIFT,
} PRCM_VsysDetRst;

#define PRCM_V33_DET_RSTN_SHIFT            4
#define PRCM_V33_DET_RSTN_MASK             0X1U << PRCM_V33_DET_RSTN_SHIFT
typedef enum {
	PRCM_v33_DET_RST_ENABLE = 0X0U << PRCM_V33_DET_RSTN_SHIFT,
	PRCM_v33_DET_RST_BYPASS = 0X1U << PRCM_V33_DET_RSTN_SHIFT,
} PRCM_V33DetRst;

#define PRCM_POR_RESET_CNT_SHIFT           16
#define PRCM_POR_RESET_CNT_MASK            0x3FFU << PRCM_POR_RESET_CNT_SHIFT
#define PRCM_POR_RESET_CNT_VAL(v)          (((v) & 0x3FFU) << PRCM_POR_RESET_CNT_SHIFT)

/*
 * bit field definition of PRCM->OTP_CTRL  0x0028
 */

#define PRCM_OTP_STATUS_SHIFT              24
#define PRCM_OTP_STATUS_VMASK              0x01

#define PRCM_OTP_IRQEN_SHIFT               20
#define PRCM_OTP_IRQEN_MASK                0x01 << PRCM_OTP_IRQEN_SHIFT
#define PRCM_OTP_IRQEN_VAL(v)              v << PRCM_OTP_IRQEN_SHIFT

#define PRCM_OTP_IRQ_STATUS_SHIFT          16
#define PRCM_OTP_IRQ_STATUS_MASK           0x1 << PRCM_OTP_IRQ_STATUS_SHIFT
#define PRCM_OTP_IRQ_STATUS_VMASK          0x1

#define PRCM_OTP_CTRL_EN_SHIFT             12
#define PRCM_OTP_CTRL_EN_MASK              0x01 << PRCM_OTP_CTRL_EN_SHIFT
#define PRCM_OTP_CTRL_EN_VAL(v)            v << PRCM_OTP_CTRL_EN_SHIFT

#define PRCM_OTP_TRIM_SHIFT                4
#define PRCM_OTP_TRIM_MASK                 0X07 << PRCM_OTP_TRIM_SHIFT
#define PRCM_OTP_TRIM_VAL(v)               v << PRCM_OTP_TRIM_SHIFT

#define PRCM_OTP_DET_EN_SHIFT              0
#define PRCM_OTP_DET_EN_MASK               0X01 << PRCM_OTP_DET_EN_SHIFT
#define PRCM_OTP_DET_EN_VAL(v)             v << PRCM_OTP_DET_EN_SHIFT

typedef enum {
    PRCM_OTP_TRIM_DEGREES_105 = 0U,
    PRCM_OTP_TRIM_DEGREES_110 = 1U,
    PRCM_OTP_TRIM_DEGREES_115 = 2U,
    PRCM_OTP_TRIM_DEGREES_120 = 3U,
    PRCM_OTP_TRIM_DEGREES_125 = 4U,
    PRCM_OTP_TRIM_DEGREES_130 = 5U,
    PRCM_OTP_TRIM_DEGREES_135 = 6U,
    PRCM_OTP_TRIM_DEGREES_140 = 7U,
    PRCM_OTP_TRIM_DEGREES_MAX,
} PRCM_OtpTrim;

typedef struct {
	uint8_t det_en;
	uint8_t ctrl_en;
	uint8_t irq_en;
} PRCM_OTPParm;

/*
 * bit field definition of PRCM->VCC33_DET_IRQ  0x002c
 */
#define PRCM_VCC33_DET_OUT_SHIFT           24
#define PRCM_VCC33_DET_OUT_MASK            0x1 << PRCM_VCC33_DET_OUT_SHIFT
#define PRCM_VCC33_DET_OUT_VMASK           0x1

#define PRCM_VCC33_DET_IRQ_EN_SHIFT        20
#define PRCM_VCC33_DET_IRQ_EN_MASK         0x1 << PRCM_VCC33_DET_IRQ_EN_SHIFT

#define PRCM_VCC33_DET_IRQ_STATUS_SHIFT    16
#define PRCM_VCC33_DET_IRQ_STATUS_MASK     0x1 << PRCM_VCC33_DET_IRQ_STATUS_SHIFT
#define PRCM_VCC33_DET_IRQ_STATUS_VMASK    0x1

/*
 * bit field definition of PRCM->RTC_IO_WAKE_EN         0x0050
 * bit field definition of PRCM->RTC_IO_WAKE_DEB_CLK    0x0054
 * bit field definition of PRCM->RTC_IO_WAKE_ST         0x0058
 * bit field definition of PRCM->RTC_IO_HOLD_CTRL       0x005c
 * bit field definition of PRCM->RTC_IO_WUP_GEN         0x0060
 * bit field definition of PRCM->RTC_IO_WUP_DEB_CYCLES0 0x0064
 * bit field definition of PRCM->RTC_IO_WUP_DEB_CYCLES1 0x0068
 */

typedef enum {
	PRCM_WAKE_IO_0 = HAL_BIT(0),
	PRCM_WAKE_IO_1 = HAL_BIT(1),
	PRCM_WAKE_IO_2 = HAL_BIT(2),
	PRCM_WAKE_IO_3 = HAL_BIT(3),
	PRCM_WAKE_IO_4 = HAL_BIT(4),
	PRCM_WAKE_IO_5 = HAL_BIT(5),
	PRCM_WAKE_IO_6 = HAL_BIT(6),
	PRCM_WAKE_IO_7 = HAL_BIT(7),
#define WAKEUP_IO_NUM	8
#define WAKEUP_IO_MAX	8
} PRCM_WakeIO;


#define WAKEUP_IO_MASK                        ((1 << WAKEUP_IO_MAX) - 1)
#define PRCM_WAKEUP_IOx_MODE_SHIFT            16
#define PRCM_WAKEUP_IOx_EN_SHIFT              0
#define PRCM_WAKE_IO_GLOBAL_EN_BIT            HAL_BIT(0)

#define PRCM_WKAEUP_DEB_CLK1_SHIFT            28
#define PRCM_WKAEUP_DEB_CLK1_MASK             (0xFU << PRCM_WKAEUP_DEB_CLK1_SHIFT)
#define PRCM_WKAEUP_DEB_CLK0_SHIFT            24
#define PRCM_WKAEUP_DEB_CLK0_MASK             (0xFU << PRCM_WKAEUP_DEB_CLK0_SHIFT)
#define PRCM_WAKEUP_IOX_DEB_CLK_SRC_SHIFT     0
#define PRCM_WAKEUP_IOX_DEB_CLK_SRC_MASK      (0x3U << PRCM_WAKEUP_IOX_DEB_CLK_SRC_SHIFT)
typedef enum {
	PRCM_WAKEUP_IOX_DEB_CLK0_SRC = 0U,
	PRCM_WAKEUP_IOX_DEB_CLK1_SRC = 1U,
} PRCM_WakeIODebClkSrc;

#define PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_L_SHIFT(n)  (0 + (n * 4))
#define PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_L_MASK(n)   (0xFU << PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_L_SHIFT(n))
#define PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_H_SHIFT(n)  (0 + (n * 4))
#define PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_H_MASK(n)   (0xFU << PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_H_SHIFT(n))

typedef enum {
	PRCM_WAKEUP_IO_DEB_CYCLES_L = 0U,
	PRCM_WAKEUP_IO_DEB_CYCLES_H = 1U,
} PRCM_WakeIODebCyclesMode;

/*
 * bit field definition of PRCM->WLAN_HIF_OV_CTRL 0x18c
 */
#define PRCM_WLAN_IRQ_OVR_HIF_BIT               HAL_BIT(7)
#define PRCM_WLAN_WUP_OVR_HIF_BIT               HAL_BIT(6)
#define PRCM_WLAN_RESET_CPU_OVR_HIF_BIT         HAL_BIT(5)
#define PRCM_WLAN_DISABLE_CPU_CLK_OVR_HIF_BIT   HAL_BIT(4)
#define PRCM_WLAN_WUP_BIT                       HAL_BIT(2)
#define PRCM_WLAN_RESET_CPU_BIT                 HAL_BIT(1)
#define PRCM_WLAN_DISABLE_CPU_CLK_BIT           HAL_BIT(0)

/*
 * bit field definition of PRCM->BOOT_FLAG 0x1c0
 * bit field definition of PRCM->BOOT_ADDR 0x1c4
 * bit field definition of PRCM->BOOT_ARG  0x1c8
 */
#define PRCM_CPU_BOOT_FLAG_WR_LOCK		(0x429BU << 16)	/* W */

#define PRCM_CPU_BOOT_FLAG_SHIFT		0	/* R/W */
#define PRCM_CPU_BOOT_FLAG_MASK			(0xFU << PRCM_CPU_BOOT_FLAG_SHIFT)

typedef enum {
	PRCM_CPU_TYPE_INDEX_AR800A = 0,
} PRCM_CPUTypeIndex;

typedef enum {
	PRCM_CPU_BOOT_FROM_COLD_RESET	= (0U << PRCM_CPU_BOOT_FLAG_SHIFT),
	PRCM_CPU_BOOT_FROM_STANDBY		= (1U << PRCM_CPU_BOOT_FLAG_SHIFT),
	PRCM_CPU_BOOT_FROM_DEEP_SLEEP	= (3U << PRCM_CPU_BOOT_FLAG_SHIFT),
	PRCM_CPU_BOOT_FROM_USB_UPDATE_HOSC	= (4U << PRCM_CPU_BOOT_FLAG_SHIFT)
} PRCM_CPUBootFlag;

/*
 * bit field definition of PRCM->RESET_SRC_STATUS 0x01d4
 */

#define PRCM_CPU_RST_STATUS_IS_YES      0x01U
#define PRCM_IS_PWRON_RST_SHIFT         0
#define PRCM_IS_DET_RST_SHIFT           1
#define PRCM_IS_RTC_WDG_RST_SHIFT       3

typedef enum {
    PRCM_CPU_RST_SRC_PWRON = HAL_BIT(0),
    PRCM_CPU_RST_SRC_DET   = HAL_BIT(1),
    PRCM_CPU_RST_SRC_RTC_WDG = HAL_BIT(3),
} PRCM_CpuRstSrc;
#define PRCM_CPU_RST_CLR_MASK \
	        (PRCM_CPU_RST_SRC_PWRON | PRCM_CPU_RST_SRC_DET | PRCM_CPU_RST_SRC_RTC_WDG)

/*
 * bit field definition of PRCM->SYSTEM_PRIV_REG0T7 0x200~0x21c
 */
#define PRCM_SYSTEM_PRIV_DATA_ID_NUM 8

/*
 * ******************************************************************
 * PRCM FUNCTION
 * ******************************************************************
 */

/*
 * bit field definition of PRCM->LPCLK_CTRL 0x0000
 */
void HAL_PRCM_SetSys32KClkSel(PRCM_SYS32KSrc source);
void HAL_PRCM_SetLpclkClkSel(PRCM_LPCLK32KSrc source);
void HAL_PRCM_SetRcoTrim(uint8_t trim);
void HAL_PRCM_SetXO32KCLTrim(uint8_t trim);
void HAL_PRCM_SetXO32KEnable(uint8_t enable);

/*
 * bit field definition of PRCM->LPCLK_STATUS 0x0004
 */
int HAL_GET_XO32KReadyFlag(void);

/*
 * bit field definition of PRCM->RCOSC_CALIB_REG0~3 0x000c ~ 0x0018
 */
void HAL_PRCM_SetRcoCalib(uint32_t en, PRCM_RCOSC_WkModSel mode, PRCM_RCOSC_NormalWkTimesSel sel,
                          uint32_t phase1, uint32_t phase2, uint32_t wk_time_en, uint32_t wk_time);
void HAL_PRCM_SetRcoscCntTarget(uint16_t rcosc_cnt);
void HAL_PRCM_SetRcoscDividend(uint16_t rcosc_div);

/*
 * bit field definition of PRCM->RESET_CTRL 0x001c
 */
void HAL_PRCM_SetResetMod(PRCM_ResetModule mod, uint8_t release_reset);

/*
 * bit field definition of PRCM->POR_RST_CNT and POR_RST_CTRL  0x0020 ~ 0x0024
 */
void HAL_PRCM_SetPorRstCnt(uint32_t reset_cnt);
void HAL_PRCM_SetPorVccRstDet(PRCM_VsysDetRst vsys_det, PRCM_V33DetRst vcc_det);

/*
 * bit field definition of PRCM->OTP_CTRL  0x0028
 */
void HAL_PRCM_SetOtpConfig(PRCM_OTPParm config);
uint8_t HAL_PRCM_GetOtpStatus(void);
uint8_t HAL_PRCM_GetOtpIrqStatus(void);
void HAL_PRCM_SetOtpIrqClean(void);
void HAL_PRCM_SetOtpTrim(PRCM_OtpTrim otp_trim);

/*
 * bit field definition of PRCM->VCC33_DET_IRQ  0x002c
 */
uint8_t HAL_PRCM_GetV33DetOut(void);
void HAL_PRCM_SetV33DetIrqEnable(uint8_t enable);
uint8_t HAL_PRCM_GetVcc33DetIrqStatus(void);
void HAL_PRCM_SetVcc33DetIrqStatusClean(void);

/*
 * bit field definition of PRCM->RTC_IO_WAKE_EN         0x0050
 * bit field definition of PRCM->RTC_IO_WAKE_DEB_CLK    0x0054
 * bit field definition of PRCM->RTC_IO_WAKE_ST         0x0058
 * bit field definition of PRCM->RTC_IO_HOLD_CTRL       0x005c
 * bit field definition of PRCM->RTC_IO_WUP_GEN         0x0060
 * bit field definition of PRCM->RTC_IO_WUP_DEB_CYCLES0 0x0064
 * bit field definition of PRCM->RTC_IO_WUP_DEB_CYCLES1 0x0068
 */
void HAL_PRCM_WakeupIOEnable(PRCM_WakeIO ioMask);
void HAL_PRCM_WakeupIODisable(PRCM_WakeIO ioMask);
void HAL_PRCM_EnableWakeupIOx(uint8_t ioIndex, uint8_t enable);
void HAL_PRCM_WakeupIOSetRisingEvent(PRCM_WakeIO ioMask);
void HAL_PRCM_WakeupIOSetFallingEvent(PRCM_WakeIO ioMask);
uint32_t HAL_PRCM_WakeupIOGetEventStatus(void);
uint32_t HAL_PRCM_WakeupIOClearEventStatus(void);
int HAL_PRCM_WakeupIOGetEventDetected(PRCM_WakeIO ioMask);
void HAL_PRCM_WakeupIOClearEventDetected(PRCM_WakeIO ioMask);
void HAL_PRCM_WakeupIOEnableCfgHold(PRCM_WakeIO ioMask);
void HAL_PRCM_WakeupIODisableCfgHold(PRCM_WakeIO ioMask);
void HAL_PRCM_WakeupIOEnableGlobal(void);
void HAL_PRCM_WakeupIODisableGlobal(void);
void HAL_PRCM_SetWakeupIOxDebSrc(uint8_t ioIndex, PRCM_WakeIODebClkSrc src);
void HAL_PRCM_SetWakeupDebClk0(uint8_t val);
void HAL_PRCM_SetWakeupDebClk1(uint8_t val);
void HAL_PRCM_SetWakeupIOxDebounce(PRCM_WakeIODebCyclesMode wakeio_cycle, uint8_t ioIndex, uint8_t val);
PRCM_WakeIO HAL_PRCM_WakeupIOxIndex2Mask(uint8_t ioIndex);
uint8_t HAL_PRCM_WakeupIOxMask2Index(PRCM_WakeIO ioMask);

/*
 * bit field definition of PRCM->WLAN_HIF_OV_CTRL 0x18c
 */
void HAL_PRCM_EnableWlanCPUClk(uint8_t enable);
void HAL_PRCM_ReleaseWlanCPUReset(void);
void HAL_PRCM_ForceWlanCPUReset(void);
void HAL_PRCM_WakeUpWlan(uint8_t wakeup);
void HAL_PRCM_EnableWlanCPUClkOvrHIF(void);
void HAL_PRCM_DisableWlanCPUClkOvrHIF(void);
void HAL_PRCM_ReleaseWlanCPUOvrHIF(void);
void HAL_PRCM_ResetWlanCPUOvrHIF(void);
void HAL_PRCM_EnableWlanWUPOvrHIF(void);
void HAL_PRCM_DisableWlanWUPOvrHIF(void);
void HAL_PRCM_EnableWlanIRQOvrHIF(void);
void HAL_PRCM_DisableWlanIRQOvrHIF(void);

/*
 * bit field definition of PRCM->BOOT_FLAG 0x1c0
 * bit field definition of PRCM->BOOT_ADDR 0x1c4
 * bit field definition of PRCM->BOOT_ARG  0x1c8
 */
void HAL_PRCM_SetCPUBootFlag(PRCM_CPUTypeIndex cpu, PRCM_CPUBootFlag flag);
uint32_t HAL_PRCM_GetCPUBootFlag(PRCM_CPUTypeIndex cpu);
void HAL_PRCM_SetCPUBootAddr(PRCM_CPUTypeIndex cpu, uint32_t addr);
uint32_t HAL_PRCM_GetCPUBootAddr(PRCM_CPUTypeIndex cpu);
void HAL_PRCM_SetCPUBootArg(PRCM_CPUTypeIndex cpu, uint32_t arg);
uint32_t HAL_PRCM_GetCPUBootArg(PRCM_CPUTypeIndex cpu);

/*
 * bit field definition of PRCM->RESET_SRC_STATUS 0x01d4
 */
uint8_t HAL_PRCM_ResetSrcStatusISPwronRst(void);
uint8_t HAL_PRCM_ResetSrcStatusISDetRst(void);
uint8_t HAL_PRCM_ResetSrcStatusISWdgRst(void);
void HAL_PRCM_ClrCpuRstSrc(void);

/*
 * bit field definition of PRCM->SYSTEM_PRIV_REG0T7 0x200~0x21c
 */
void HAL_PRCM_SetSystemPrivateData(uint32_t id, uint32_t data);
uint32_t HAL_PRCM_GetSystemPrivateData(uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_CHIP_HAL_PRCM_H_ */

