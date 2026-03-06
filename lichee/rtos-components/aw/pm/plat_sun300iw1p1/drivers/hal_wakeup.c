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

#include <hal_prcm.h>
#include <hal_pwrctrl.h>
#include <hal_wakeup.h>
#include <hal_ts.h>
#include <hal_alarm.h>

void HAL_WUPTIMER_Release(void)
{
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + RESET_CTRL), RTC_WAKEUP_TIMER_RSTN_SHIFT, RTC_WAKEUP_TIMER_RSTN_VMASK, 1);
}

void HAL_WUPTIMER_Reset(void)
{
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + RESET_CTRL), RTC_WAKEUP_TIMER_RSTN_SHIFT, RTC_WAKEUP_TIMER_RSTN_VMASK, 0);
}

void HAL_WUPTIMER_ClrIrqPending(void)
{
	HAL_SET_BIT_VAL(HAL_REG_32BIT(WAKEUP_TIMER + WUP_TMR_VAL), TMR_IRQ_PENDING_SHIFT, TMR_IRQ_PENDING_VMASK, 1);
	while (HAL_GET_BIT_VAL(HAL_REG_32BIT(WAKEUP_TIMER + WUP_TMR_VAL), TMR_IRQ_PENDING_SHIFT, TMR_IRQ_PENDING_VMASK));
}

uint32_t HAL_WUPTIMER_IsIrqPending(void)
{
	return HAL_GET_BIT_VAL(HAL_REG_32BIT(WAKEUP_TIMER + WUP_TMR_VAL), TMR_IRQ_PENDING_SHIFT, TMR_IRQ_PENDING_VMASK);
}

void HAL_WUPTIMER_SetInterval(uint32_t intv)
{
	HAL_SET_BIT_VAL(HAL_REG_32BIT(WAKEUP_TIMER + WUP_TMR_VAL), TMR_INTV_VALUE_SHIFT, TMR_INTV_VALUE_VMASK, intv);
	while (HAL_GET_BIT_VAL(HAL_REG_32BIT(WAKEUP_TIMER + WUP_TMR_VAL), TMR_INTV_VALUE_SHIFT, TMR_INTV_VALUE_VMASK) != intv);
}

uint32_t HAL_WUPTIMER_GetInterval(void)
{
	return HAL_GET_BIT_VAL(HAL_REG_32BIT(WAKEUP_TIMER + WUP_TMR_VAL), TMR_INTV_VALUE_SHIFT, TMR_INTV_VALUE_VMASK);
}

void HAL_WUPTIMER_Enable(uint32_t en)
{
	en = (en ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(WAKEUP_TIMER + WUP_TMR_CTRL), TMR_EN_SHIFT, TMR_EN_VMASK, en);
	while (HAL_GET_BIT_VAL(HAL_REG_32BIT(WAKEUP_TIMER + WUP_TMR_CTRL), TMR_EN_SHIFT, TMR_EN_VMASK) != en);
}

int32_t HAL_WUPTIMER_IsEnable(void)
{
	return HAL_GET_BIT_VAL(HAL_REG_32BIT(WAKEUP_TIMER + WUP_TMR_CTRL), TMR_EN_SHIFT, TMR_EN_VMASK);
}

uint32_t HAL_RTCALARM_GetInterval(void)
{
	return HAL_REG_32BIT(RTC_ALARM + ALM0_CNT);
}

int32_t HAL_RTCALARM_IsEnable(void)
{
	return HAL_GET_BIT_VAL(HAL_REG_32BIT(RTC_ALARM + ALM0_EN), ALM0_EN_SHIFT, ALM0_EN_VMASK);
}

uint32_t HAL_WUPIO_IsIrqPending(uint8_t idx)
{
	return (HAL_REG_32BIT(PRCM + RTC_WAKEUP_IO_STA) & (1 << idx));
}

uint32_t HAL_ALARM_IsIrqPending(uint8_t idx)
{
	if (idx == 0) {
		return HAL_GET_BIT_VAL(HAL_REG_32BIT(ALARM + ALARM0_IRQST), ALARM_IRQ_STA_SHIFT, ALARM_IRQ_STA_VMASK);
	} else if (idx == 1) {
		return HAL_GET_BIT_VAL(HAL_REG_32BIT(ALARM + ALARM1_IRQST), ALARM_IRQ_STA_SHIFT, ALARM_IRQ_STA_VMASK);
	}
	return 0;
}

void HAL_WAKEUP_SetWakeupMask(uint32_t mask)
{
	HAL_REG_32BIT(PWRCTRL_AON + WAKUP_MSK_REG0) |= mask;
	while ((HAL_REG_32BIT(PWRCTRL_AON + WAKUP_MSK_REG0) & mask) != mask);
}

void HAL_WAKEUP_ClrWakeupMask(uint32_t mask)
{
	HAL_REG_32BIT(PWRCTRL_AON + WAKUP_MSK_REG0) &= ~mask;
	while ((HAL_REG_32BIT(PWRCTRL_AON + WAKUP_MSK_REG0) & mask) == mask);
}

void HAL_WAKEUP_RstRccal(void)
{
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + RESET_CTRL), RCCAL_RSTN_SHIFT, RCCAL_RSTN_VMASK, 0);
}

