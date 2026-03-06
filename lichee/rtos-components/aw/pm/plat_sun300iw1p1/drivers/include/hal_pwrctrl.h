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

#ifndef _STANDBY_HAL_PWRCTRL_H_
#define _STANDBY_HAL_PWRCTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_def.h>

#define PWRCTRL_RTC                 (0x4A000800)
#define LDO_RTC_CTRL                (0x0008)
#define LDO_RTC_0V9_VSELH_SHIFT     (8)
#define LDO_RTC_0V9_VSELL_SHIFT     (0)
#define LDO_2V8_CTRL_REG            (0x0010)
#define LDO_2V8_EN_SHIFT            (0)
#define LDO_2V8_EN_VMASK            (0x1U)
#define ANA_PWR_RST_REG             (0x003C)
#define SYS_ANA_VDDON_CTRL1_SHIFT   (1)
#define SYS_ANA_VDDON_CTRL1_VMASK   (0x1U)
#define SYS_ANA_VDDON_CTRL0_SHIFT   (0)
#define SYS_ANA_VDDON_CTRL0_VMASK   (0x1U)
#define VDD_OFF_GATING_CTRL_REG     (0x0038)
#define DRAM_ZQ_PAD_HOLD_SHIFT      (1)
#define DRAM_ZQ_PAD_HOLD_VMASK      (0x1U)
#define DRAM_CH_PAD_HOLD_SHIFT      (0)
#define DRAM_CH_PAD_HOLD_VMASK      (0x1U)
#define POR_CTRL                    (0x0030)
#define POR_SW_OVR_EN_MASK          (1 << 13)
#define POR_SW_OVR_MASK             (1 << 12)
#define POR_SW_HIB_EN_MASK          (1 << 8)
#define VSYS_DET_CTRL               (0x0060)
#define VSYS_DET_OVR_EN_MASK        (1 << 1)
#define VSYS_DET_OVR_MASK           (1 << 0)
#define VCC_DET_CTRL                (0x0064)
#define VCC33_DET_OVR_EN_MASK       (1 << 2)
#define VCC33_DET_OVR_MASK          (1 << 1)
#define VCC33_VSYS_DET_HIB_EN_MASK  (1 << 0)

#define PWRCTRL_AON                 (0x4A011000)
#define WAKUP_MSK_REG0              (0x0020)
#define RTC_TIMER_WAKEUP_MASK       (1 << 0)
#define ALARM_WAKEUP_MASK           (1 << 1)
#define WAKEUP_IO_WAKEUP_MASK       (1 << 2)
#define WLAN_WAKEUP_MASK            (1 << 4)
#define WLAN_ACT_RES                (0x0040)
#define WLAN_SLP_RES                (0x0044)
#define WAKUP_CTRL_REG              (0x0064)
#define CPUX_WUP_EN_SHIFT           (8)
#define CPUX_WUP_EN_VMASK           (0x1U)
#define LP_STATUS_REG               (0x0068)
#define CPUX_STATUS_SHIFT           (12)
#define CPUX_STATUS_VMASK           (0x1U)
#define SLP_MODE_CTRL_REG           (0x0060)
#define WIF_MODE_SHIFT              (0)
#define WIF_MODE_VMASK              (0x1U)
#define RAM_RET_CFG_REG             (0x006C)
#define RAM_RET_CLKON_SHIFT         (16)
#define RAM_RET_CLKON_VMASK         (0x1U)
#define VE_BOOTRAM_RET_CFG_SHIFT    (4)
#define VE_BOOTRAM_RET_CFG_VMASK    (0x1U)
#define VIN_BOOTRAM_REG_CFG_SHIFT   (0)
#define VIN_BOOTRAM_REG_CFG_VMASK   (0x1U)
#define PWR_CFG_REG                 (0x0070)
#define APP_SLP_LIST_EXT_SHIFT      (12)
#define APP_SLP_LIST_EXT_VMASK      (0x1U)
#define APP_SLP_LIST_EXT_MASK       (APP_SLP_LIST_EXT_VMASK << APP_SLP_LIST_EXT_SHIFT)

#define PWRCTRL_APP                 (0x43045000)
#define RES_REQ_CTRL0_REG           (0x0000)
#define RES_REQ_CTRL1_REG           (0x0004)
#define PWRSRC_PMC_EN1_MASK         (1 << 1)
#define PWRSRC_PMC_EN2_MASK         (1 << 2)
#define PWRSRC_LDO1V8_MASK          (1 << 3)
#define RES_RDY_REG                 (0x0008)
#define RES_VLD_CLR_REG             (0x000C)
#define VLD_CLR_SHIFT               (0)
#define VLD_CLR_VMASK               (0x1U)

typedef enum {
	PWRCTRL_MODE_STANDBY     = 0U,
	PWRCTRL_MODE_HIBERNATION = 1U,
	PWRCTRL_MODE_MAX,
}PWRCTRL_Mode_t;

void HAL_PWRCTRL_EnableCPUXPowerDomain(uint32_t en);
void HAL_PWRCTRL_EnablePhyPadHold(uint32_t en);
void HAL_PWRCTRL_EnableDramPower(uint32_t is_suspend, uint32_t en);
void HAL_PWRCTRL_EnableDcdc3V3(uint32_t en);
void HAL_PWRCTRL_EnableAnaVddon(uint32_t en);
void HAL_PWRCTRL_SetSleepMode(PWRCTRL_Mode_t mode);
void HAL_PWRCTRL_StopAppPwrReq(void);

#ifdef __cplusplus
}
#endif
#endif /* _STANDBY_HAL_PWRCTRL_H_ */


