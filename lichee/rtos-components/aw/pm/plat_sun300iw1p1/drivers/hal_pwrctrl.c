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
#include <hal_pwrctrl.h>
#include <hal_gpio.h>
#include <hal_prcm.h>
#include <hal_ts.h>

void HAL_PWRCTRL_EnableCPUXPowerDomain(uint32_t en)
{
	en = (en ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_AON + WAKUP_CTRL_REG), CPUX_WUP_EN_SHIFT, CPUX_WUP_EN_VMASK, en);
	while (HAL_GET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_AON + LP_STATUS_REG), CPUX_STATUS_SHIFT, CPUX_STATUS_VMASK) != en);
}

void HAL_PWRCTRL_EnablePhyPadHold(uint32_t en)
{
	en = (en ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_RTC + VDD_OFF_GATING_CTRL_REG), DRAM_ZQ_PAD_HOLD_SHIFT, DRAM_ZQ_PAD_HOLD_VMASK, en);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_RTC + VDD_OFF_GATING_CTRL_REG), DRAM_CH_PAD_HOLD_SHIFT, DRAM_CH_PAD_HOLD_VMASK, en);
}

void HAL_PWRCTRL_EnableDramPower(uint32_t is_suspend, uint32_t en)
{
#ifdef CONFIG_PMC_PL6_FOR_PWRCTRL
	if (!is_suspend) {
		/* clear io hold in resume, must delay 125us for hardware handle */
		HAL_CLR_BITS(PRCM + RTC_IO_HOLD_CTRL, 1 << 6);
		HAL_TS_Udelay(125);
	}

	HAL_SET_BIT_VAL(HAL_REG_32BIT(RTC_GPIO + PL_CFG0), PL6_SELECT_SHIFT, PL_SELECT_VMASK, 1);
	if (en) {
		HAL_REG_32BIT(RTC_GPIO + PL_DAT) |= (1 << 6);
	} else {
		HAL_REG_32BIT(RTC_GPIO + PL_DAT) &= ~(1 << 6);
	}

	if (is_suspend) {
		/* set io hold in suspend, must delay 125us for hardware handle */
		HAL_SET_BITS(PRCM + RTC_IO_HOLD_CTRL, 1 << 6);
		HAL_TS_Udelay(125);
	}
#endif
#ifdef CONFIG_PMC_PL7_FOR_PWRCTRL
	if (!is_suspend) {
		/* clear io hold in resume, must delay 125us for hardware handle */
		HAL_CLR_BITS(PRCM + RTC_IO_HOLD_CTRL, 1 << 7);
		HAL_TS_Udelay(125);
	}

	HAL_SET_BIT_VAL(HAL_REG_32BIT(RTC_GPIO + PL_CFG0), PL7_SELECT_SHIFT, PL_SELECT_VMASK, 1);
	if (en) {
		HAL_REG_32BIT(RTC_GPIO + PL_DAT) &= ~(1 << 7);

	} else {
		HAL_REG_32BIT(RTC_GPIO + PL_DAT) |= (1 << 7);
	}

	if (is_suspend) {
		/* set io hold in suspend, must delay 125us for hardware handle */
		HAL_SET_BITS(PRCM + RTC_IO_HOLD_CTRL, 1 << 7);
		HAL_TS_Udelay(125);
	}
#endif
}

void HAL_PWRCTRL_EnableAnaVddon(uint32_t en)
{
	en = (en ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_RTC + ANA_PWR_RST_REG), SYS_ANA_VDDON_CTRL0_SHIFT, SYS_ANA_VDDON_CTRL0_VMASK, en);
}

void HAL_PWRCTRL_SetSleepMode(PWRCTRL_Mode_t mode)
{
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_AON + SLP_MODE_CTRL_REG), WIF_MODE_SHIFT, WIF_MODE_VMASK, mode);
}

void HAL_PWRCTRL_StopAppPwrReq(void)
{
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_APP + RES_VLD_CLR_REG), VLD_CLR_SHIFT, VLD_CLR_VMASK, 1U);
}

