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

#ifndef _STANDBY_HAL_PRCM_H_
#define _STANDBY_HAL_PRCM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_def.h>

#define PRCM                          (0x4A000000)
#define RCOSC_CALIB_REG1              (0x0010)
#define RCO_WUP_MODE_SEL_SHIFT        (0)
#define RCO_WUP_MODE_SEL_VMASK        (0x1U)
#define RESET_CTRL                    (0x001C)
#define RCCAL_RSTN_SHIFT              (0)
#define RCCAL_RSTN_VMASK              (0x1U)
#define RTC_WAKEUP_TIMER_RSTN_SHIFT   (1)
#define RTC_WAKEUP_TIMER_RSTN_VMASK   (0x1U)
#define RTC_RTC_WDG_RSTN_SHIFT        (3)
#define RTC_RTC_WDG_RSTN_VMASK        (0x1U)
#define DCXO_EN_DLY_CNT_REG           (0x0104)
#define DCXO_EN_DLY_CNT_SHIFT         (0)
#define DCXO_EN_DLY_CNT_VMASK         (0xFFU)
#define BOOT_FLAG_REG                 (0x01C0)
#define BOOT_ADDR_REG                 (0x01C0)
#define PRCM_SYS_PRIV0                (0x0200)
#define PRCM_RTOS_PARTITION_SHIFT     (6)
#define PRCM_RTOS_PARTITION_VMASK     (0x1U)
#define RTC_WAKEUP_IO_STA             (0x0058)
#define PWR_EN_CFG                    (0x0070)
#define PWR_EN2_WAKE_MODE_MASK        (0x3 << 26)
#define PWR_EN2_MODE_WAKEUP_IO_MASK   (0x1 << 26)
#define PWR_EN0_PULL_MASK             (0x3 << 16)
#define PWR_EN1_PULL_MASK             (0x3 << 18)
#define PWR_EN2_PULL_MASK             (0x3 << 20)
#define PWR_EN2_PULL_UP_MASK          (0x1 << 20)
#define PWR_EN0_DRV_MASK              (0x3 << 0)
#define PWR_EN1_DRV_MASK              (0x3 << 2)
#define PWR_EN2_DRV_MASK              (0x3 << 4)
#define RTC_IO_HOLD_CTRL              (0x005C)

#ifdef __cplusplus
}
#endif
#endif /* _STANDBY_HAL_PRCM_H_ */

