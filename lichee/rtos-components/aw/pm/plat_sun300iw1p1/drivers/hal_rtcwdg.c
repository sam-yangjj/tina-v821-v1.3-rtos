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

#include <hal_rtcwdg.h>
#include <hal_prcm.h>

#define RTCWDG_BACKUP_REG_NUM   (4)
static uint32_t rtcwdg_reg[RTCWDG_BACKUP_REG_NUM];

void HAL_RTCWDG_Init(void)
{
	uint32_t val;

	HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + RESET_CTRL), RTC_RTC_WDG_RSTN_SHIFT, RTC_RTC_WDG_RSTN_VMASK, 1);
	HAL_REG_32BIT(RTCWDG + RTCWDG_IRQ_EN_REG) &= ~RTCWDG_IRQ_EN_MASK;
	val = HAL_REG_32BIT(RTCWDG + RTCWDG_CFG_REG);
	val &= ~RTCWDG_CFG_MASK;
	val |= RTCWDG_KEY_FIELD_16AA;
	val |= RTCWDG_RST_SYS;
	HAL_REG_32BIT(RTCWDG + RTCWDG_CFG_REG) = val;
}

void HAL_RTCWDG_Deinit(void)
{
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + RESET_CTRL), RTC_RTC_WDG_RSTN_SHIFT, RTC_RTC_WDG_RSTN_VMASK, 0);
}

void HAL_RTCWDG_Backup(void)
{
	rtcwdg_reg[0] = HAL_REG_32BIT(PRCM + RESET_CTRL) & (RTC_RTC_WDG_RSTN_VMASK << RTC_RTC_WDG_RSTN_SHIFT);
	rtcwdg_reg[1] = HAL_REG_32BIT(RTCWDG + RTCWDG_IRQ_EN_REG);
	rtcwdg_reg[2] = HAL_REG_32BIT(RTCWDG + RTCWDG_CFG_REG);
	rtcwdg_reg[3] = HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG);
}

void HAL_RTCWDG_Restore(void)
{
	if (rtcwdg_reg[0]) {
		HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + RESET_CTRL), RTC_RTC_WDG_RSTN_SHIFT, RTC_RTC_WDG_RSTN_VMASK, 1);
		HAL_REG_32BIT(RTCWDG + RTCWDG_IRQ_EN_REG) = rtcwdg_reg[1];
		HAL_REG_32BIT(RTCWDG + RTCWDG_CFG_REG) = rtcwdg_reg[2] | RTCWDG_KEY_FIELD_16AA;
		HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG) = rtcwdg_reg[3] | RTCWDG_KEY_FIELD_16AA;
	} else {
		HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + RESET_CTRL), RTC_RTC_WDG_RSTN_SHIFT, RTC_RTC_WDG_RSTN_VMASK, 0);
	}
}

void HAL_RTCWDG_Enable(void)
{
	uint32_t val;

	val = HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG);
	val &= (RTCWDG_INTERVAL_VMASK << RTCWDG_INTERVAL_SHIFT);
	val |= ((RTCWDG_KEY_FIELD_16AA) | (1 << RTCWDG_EN_SHIFT));
	HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG) = val;
}

void HAL_RTCWDG_Disable(void)
{
	uint32_t val;

	val = HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG);
	val &= (RTCWDG_INTERVAL_VMASK << RTCWDG_INTERVAL_SHIFT);
	val |= RTCWDG_KEY_FIELD_16AA;
	HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG) = val;
}

int32_t HAL_RTCWDG_IsEnable(void)
{
	if (HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG) & (1 << RTCWDG_EN_SHIFT)) {
		return 1;
	}
	return 0;
}

void HAL_RTCWDG_SetPeriod(uint32_t period)
{
	uint32_t val;

	val = HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG);
	val &= ~(RTCWDG_INTERVAL_VMASK << RTCWDG_INTERVAL_SHIFT);
	val |= (RTCWDG_KEY_FIELD_16AA);
	val |= ((period & RTCWDG_INTERVAL_VMASK) << RTCWDG_INTERVAL_SHIFT);
	HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG) = val;
}

uint32_t HAL_RTCWDG_GetPeriod(void)
{
	return (HAL_REG_32BIT(RTCWDG + RTCWDG_MODE_REG) >> RTCWDG_INTERVAL_SHIFT) & RTCWDG_INTERVAL_VMASK;
}

void HAL_RTCWDG_Feed(void)
{
	uint32_t val;

	val = RTCWDG_KEY_FIELD_0A57 | (1 << RTCWDG_RESTART_SHIFT);
	HAL_REG_32BIT(RTCWDG + RTCWDG_CTRL_REG) = val;
}

void HAL_RTCWDG_Reboot(void)
{
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + RESET_CTRL), RTC_RTC_WDG_RSTN_SHIFT, RTC_RTC_WDG_RSTN_VMASK, 1);
	HAL_REG_32BIT(RTCWDG + RTCWDG_SRST_REG) = (RTCWDG_KEY_FIELD_16AA | RTCWDG_SOFT_RESET_ENABLE);
}

