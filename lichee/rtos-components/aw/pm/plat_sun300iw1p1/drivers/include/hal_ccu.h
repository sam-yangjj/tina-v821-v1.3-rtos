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

#ifndef _STANDBY_HAL_CCU_H_
#define _STANDBY_HAL_CCU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_def.h>

#define CCU_AON                         (0x4A010000)
#define PLL_CPU_CTRL_REG                (0x0000)
#define PLL_PERI_CTRL_REG0              (0x0020)
#define PLL_PERI_CTRL_REG1              (0x0024)
#define PLL_VIDEO_CTRL_REG              (0x0040)
#define PLL_CSI_CTRL_REG                (0x0048)
#define PLL_DDR_CTRL_REG                (0x0080)
#define PLL_CSI_PAT0_REG                (0x0148)
#define SPR_FREQ_MODE_SHIFT             (30)
#define SPR_FREQ_MODE_VMASK             (0x3U)
#define SPR_FREQ_MODE_MASK              (SPR_FREQ_MODE_VMASK << SPR_FREQ_MODE_SHIFT)
#define WAVE_BOT_SHIFT                  (0)
#define WAVE_BOT_VMASK                  (0x1FFFFU)
#define WAVE_BOT_MASK                   (WAVE_BOT_VMASK << WAVE_BOT_SHIFT)
#define PLL_CSI_PAT1_REG                (0x014CU)
#define SIG_DELT_PAT_EN_SHIFT           (31)
#define SIG_DELT_PAT_EN_VMASK           (0x1U)
#define SIG_DELT_PAT_EN_MASK            (SIG_DELT_PAT_EN_VMASK << SIG_DELT_PAT_EN_SHIFT)
#define PLL_EN_SHIFT                    (31)
#define PLL_EN_VMASK                    (0x1U)
#define PLL_EN_MASK                     (PLL_EN_VMASK << PLL_EN_SHIFT)
#define PLL_LDO_EN_SHIFT                (30)
#define PLL_LDO_EN_VMASK                (0x1U)
#define PLL_LDO_EN_MASK                 (PLL_LDO_EN_VMASK << PLL_LDO_EN_SHIFT)
#define PLL_LOCK_EN_SHIFT               (29)
#define PLL_LOCK_EN_VMASK               (0x1U)
#define PLL_LOCK_EN_MASK                (PLL_LOCK_EN_VMASK << PLL_LOCK_EN_SHIFT)
#define PLL_LOCK_SHIFT                  (28)
#define PLL_LOCK_VMASK                  (0x1U)
#define PLL_LOCK_MASK                   (PLL_LOCK_VMASK << PLL_LOCK_SHIFT)
#define PLL_OUTPUT_GATING_SHIFT         (27)
#define PLL_OUTPUT_GATING_VMASK         (0x1U)
#define PLL_OUTPUT_GATING_MASK          (PLL_OUTPUT_GATING_VMASK << PLL_OUTPUT_GATING_SHIFT)
#define PLL_LOCK_TIME_SHIFT             (24)
#define PLL_LOCK_TIME_VMASK             (0x7U)
#define PLL_LOCK_TIME_MASK              (PLL_LOCK_TIME_VMASK << PLL_LOCK_TIME_SHIFT)
#define AHB_CLK_REG                     (0x0500)
#define APB_CLK_REG                     (0x0504)
#define RTC_APB_CLK_REG                 (0x0508)
#define DCXO_CFG_REG                    (0x0570)
#define DCXO_TRIM_V09_SHIFT             (11)
#define DCXO_TRIM_V09_VMASK             (0x7FU)
#define DCXO_ICTRL_V09_SHIFT            (7)
#define DCXO_ICTRL_V09_VMASK            (0xFU)
#define DCXO_CFG1_REG                   (0x0574)
#define DCXO_CNT_TG_SHIFT               (0)
#define DCXO_CNT_TG_VMASK               (0x1FFFU)
#define APB_SPC_CLK_REG                 (0x0580)
#define CPUS_CLK_REG                    (0x0584)
#define CPUX_CLK_REG                   (0x0588)
#define CPUS_CLK_SEL_MASK               (0x7U << 24)
#define AHB_CLK_SEL_MASK                (0x3U << 24)
#define APB_CLK_SEL_MASK                (0x3U << 24)
#define APB_SPC_CLK_SEL_MASK            (0x3U << 24)
#define RTC_APB_CLK_SEL_MASK            (0x3U << 24)
#define CLK_DIV_SHIFT                   (0)
#define CLK_DIV_VMASK                   (0x1f)
#define CLK_SEL_SHIFT                   (24)
#define CLK_SEL_2BIT_VMASK              (0x3U)
#define CLK_SEL_3BIT_VMASK              (0x7U)
#define CPUX_CLK_EN_SHIFT              (31)
#define CPUX_CLK_EN_VMASK              (0x1U)
#define CPUS_CLK_SRC_PERI_PLL_1024M     (0x5U << 24)
#define CPUS_CLK_SRC_PERI_PLL_614M      (0x6U << 24)
#define CPUS_CLK_DIV_1                  (0x0U << 0)
#define CPUS_CLK_DIV_2                  (0x1U << 0)
#define CPUS_CLK_DIV_3                  (0x2U << 0)
#define CPUS_CLK_DIV_4                  (0x3U << 0)
#define CPUS_CLK_DIV_5                  (0x4U << 0)
#define CPUS_CLK_DIV_6                  (0x5U << 0)
#define CPUS_CLK_DIV_7                  (0x6U << 0)
#define CPUS_CLK_DIV_8                  (0x7U << 0)
#define CPUX_CLK_ON                      (0x1U << 31)
#define CPUX_CLK_SRC_CPU_PLL             (0x4U << 24)
#define CPUX_CLK_DIV_1                   (0x0U << 0)
#define AHB_CLK_SRC_PERI_PLL_768M       (0x1U << 24)
#define AHB_CLK_DIV_4                   (0x3U << 0)
#define APB_CLK_SRC_PERI_PLL_384M       (0x1U << 24)
#define APB_CLK_DIV_4                   (0x3U << 0)
#define APB_SPC_CLK_SRC_PERI_PLL_192M   (0x3U << 24)
#define APB_SPC_CLK_DIV_1               (0x0U << 0)
#define RTC_APB_CLK_SRC_HOSC            (0x2U << 24)
#define RTC_APB_CLK_DIV_2               (0x1U << 0)
#define PLL_N_SHIFT                     (8)
#define PLL_N_VMASK                     (0xFFU)
#define PLL_N_MASK                      (PLL_N_VMASK << PLL_N_SHIFT)
#define CPU_PLL_D_SHIFT                 (2)
#define CPU_PLL_D_VMASK                 (0x3U)
#define PERI_PLL_D_SHIFT                (0)
#define PERI_PLL_D_VMASK                (0x7U)
#define VIDEO_PLL_D_SHIFT               (1)
#define VIDEO_PLL_D_VMASK               (0x3U)
#define CSI_PLL_D_SHIFT                 (1)
#define CSI_PLL_D_VMASK                 (0x3U)
#define DDR_PLL_D_SHIFT                 (1)
#define DDR_PLL_D_VMASK                 (0x3U)
#define PLL_N_192                       (191)
#define PLL_N_67                        (66)
#define PLL_N_60                        (59)
#define PLL_N_56                        (55)
#define PLL_N_50                        (49)
#define PLL_N_48                        (47)
#define PLL_N_40                        (39)
#define PLL_N_30                        (29)
#define PLL_N_24                        (23)
#define PLL_D_5                         (4)
#define PLL_D_4                         (3)
#define PLL_D_3                         (2)
#define PLL_D_2                         (1)
#define PLL_D_1                         (0)
#define PLL_CSI_D_4                     (2)

#define CCU_APP                         (0x42001000)
#define CPUS_TS_CLOCK_REG               (0x000C)
#define CPUS_TS_CLK_EN_SHIFT            (31)
#define CPUS_TS_CLK_EN_VMASK            (0x1U)
#define BUS_RESET0_REG                  (0x0090)
#define HRESETN_MCSI_SW_SHIFT           (27)
#define HRESETN_MCSI_SW_VMASK           (0x1U)
#define BUS_RESET1_REG                  (0x0094)
#define CPUX_MSGBOX_RSTN_SHIFT           (27)
#define CPUX_MSGBOX_RSTN_VMASK           (0x1U)
#define CPUX_CFG_RSTN_SW_SHIFT           (28)
#define CPUX_CFG_RSTN_SW_VMASK           (0x1U)
#define CPUX_RSTN_SW_SHIFT               (26)
#define CPUX_RSTN_SW_VMASK               (0x1U)
#define VE_RSTN_SW_SHIFT                (3)
#define VE_RSTN_SW_VMASK                (0x1U)
#define BUS_CLK_GATING0_REG             (0x0080)
#define MCSI_AHB_CLK_EN_SHIFT           (28)
#define MCSI_AHB_CLK_EN_VMASK           (0x1U)
#define BUS_CLK_GATING1_REG             (0x0084)
#define VE_AHB_CLK_EN_SHIFT             (3)
#define VE_AHB_CLK_EN_VMASK             (0x1U)
#define ADDA_PCLK_EN_SHIFT              (6)
#define ADDA_PCLK_EN_VMASK              (0x1U)
#define BUS_CLK_GATING2_REG             (0x0088)
#define VE_HCLK_EN_SHIFT                (4)
#define VE_HCLK_EN_VMASK                (0x1U)
#define AHB_MONITOR_EN_SHIFT            (8)
#define AHB_MONITOR_EN_VMASK            (0x1U)
#define SD_MONITOR_EN_SHIFT             (9)
#define SD_MONITOR_EN_VMASK             (0x1U)
#define CPUX_MT_CLK_REG                (0x0010)
#define CPUX_MT_CLK_EN_SHIFT           (31)
#define CPUX_MT_CLK_EN_VMASK           (0x1U)
#define CCU_APP_CLK_REG                 (0x007C)
#define CPUX_MSGBOX_CLK_EN_SHIFT         (6)
#define CPUX_MSGBOX_CLK_EN_VMASK         (0x1U)

#define HOSC_CLOCK_24M      (24U * 1000U * 1000U)
#define HOSC_CLOCK_40M      (40U * 1000U * 1000U)

typedef enum {
	PLL_IDX_CPU = 0U,
	PLL_IDX_PERI,
	PLL_IDX_VIDEO,
	PLL_IDX_CSI,
	PLL_IDX_DDR,
	PLL_IDX_MAX,
	PLL_IDX_BASE = PLL_IDX_CPU,
}pll_idx_t;

typedef struct {
	uint32_t pll_n;
	uint32_t pll_d;
} pll_param_t;

uint32_t HAL_CCU_GetHoscFreq(void);
void HAL_CCU_ResetCPUXCfg(uint32_t rst);
void HAL_CCU_ResetCPUX(uint32_t rst);
void HAL_CCU_EnableCPUXClk(uint32_t en);
void HAL_CCU_EnableCPUXMtClk(uint32_t en);
void HAL_CCU_ResetCPUXMsgbox(uint32_t en);
void HAL_CCU_EnableCPUXMsgboxClk(uint32_t en);
int HAL_CCU_ConfigPll(pll_param_t *param, uint32_t pll_n, uint32_t pll_d);
int HAL_CCU_IsPllOn(pll_idx_t idx);
int HAL_CCU_OpenPll(pll_idx_t idx, pll_param_t *param);
int HAL_CCU_ClosePll(pll_idx_t idx);
int HAL_CCU_PeriPllOutput(uint32_t en);
int HAL_CCU_EnableMonitor(uint32_t en);

#ifdef __cplusplus
}
#endif
#endif /* _STANDBY_HAL_CCU_H_ */

