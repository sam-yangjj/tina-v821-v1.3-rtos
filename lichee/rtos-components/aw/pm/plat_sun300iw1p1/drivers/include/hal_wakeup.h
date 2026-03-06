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

#ifndef _STANDBY_HAL_WAKEUP_H_
#define _STANDBY_HAL_WAKEUP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_def.h>

#define WAKEUP_TIMER            (0x4A000400)
#define WUP_TMR_CTRL            (0x0000)
#define TMR_EN_SHIFT            (31)
#define TMR_EN_VMASK            (0x1U)
#define WUP_TMR_VAL             (0x0004)
#define TMR_IRQ_PENDING_SHIFT   (31)
#define TMR_IRQ_PENDING_VMASK   (0x1U)
#define TMR_INTV_VALUE_SHIFT    (0)
#define TMR_INTV_VALUE_VMASK    (0x7FFFFFFF)

#define WUPTMR_AOV_MODE_MASK	(0x1 << 31)

#define RTC_ALARM               (0x4A000C00)
#define ALM0_CNT                (0x0020)
#define ALM0_EN                 (0x0028)
#define ALM0_EN_SHIFT           (0)
#define ALM0_EN_VMASK           (0x1U)

void HAL_WUPTIMER_Release(void);
void HAL_WUPTIMER_Reset(void);
void HAL_WUPTIMER_ClrIrqPending(void);
uint32_t HAL_WUPTIMER_IsIrqPending(void);
void HAL_WUPTIMER_SetInterval(uint32_t intv);
uint32_t HAL_WUPTIMER_GetInterval(void);
void HAL_WUPTIMER_Enable(uint32_t en);
int32_t HAL_WUPTIMER_IsEnable(void);
uint32_t HAL_RTCALARM_GetInterval(void);
int32_t HAL_RTCALARM_IsEnable(void);
uint32_t HAL_WUPIO_IsIrqPending(uint8_t idx);
uint32_t HAL_ALARM_IsIrqPending(uint8_t idx);
void HAL_WAKEUP_SetWakeupMask(uint32_t mask);
void HAL_WAKEUP_ClrWakeupMask(uint32_t mask);
void HAL_WAKEUP_RstRccal(void);

#ifdef __cplusplus
}
#endif
#endif /* _STANDBY_HAL_WAKEUP_H_ */

