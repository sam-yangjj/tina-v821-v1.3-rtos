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

#ifndef _STANDBY_HAL_RTCWDG_H_
#define _STANDBY_HAL_RTCWDG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_def.h>

#define RTCWDG                  (0x4A001000)
#define RTCWDG_IRQ_EN_REG       (0x0000)
#define RTCWDG_SRST_REG         (0x0008)
#define RTCWDG_CTRL_REG         (0x0010)
#define RTCWDG_CFG_REG          (0x0014)
#define RTCWDG_MODE_REG         (0x0018)

#define RTCWDG_KEY_FIELD_16AA   (0x16AA << 16)
#define RTCWDG_KEY_FIELD_0A57   (0x0A57 << 1)
#define RTCWDG_INTERVAL_SHIFT   (4U)
#define RTCWDG_INTERVAL_VMASK   (0x3FFU)
#define RTCWDG_EN_SHIFT         (0U)
#define RTCWDG_EN_VMASK         (0x1U)
#define RTCWDG_RESTART_SHIFT    (0U)
#define RTCWDG_IRQ_EN_MASK      (0x00000001)
#define RTCWDG_CFG_MASK         (0x00000003)
#define RTCWDG_RST_SYS          (0x00000001)
#define RTCWDG_SOFT_RESET_ENABLE               (0x00000001)

#define MS_TO_RTCWDG_TICK(ms)      ((ms) / 500)
#define RTCWDG_TICK_TO_TICK_32K(tick)   ((tick) * 16000)
#define TICK_32K_TO_RTCWDG_TICK(tick)   ((tick) / 16000)

void HAL_RTCWDG_Init(void);
void HAL_RTCWDG_Deinit(void);
void HAL_RTCWDG_Backup(void);
void HAL_RTCWDG_Restore(void);
void HAL_RTCWDG_Enable(void);
void HAL_RTCWDG_Disable(void);
int32_t HAL_RTCWDG_IsEnable(void);
void HAL_RTCWDG_SetPeriod(uint32_t period);
uint32_t HAL_RTCWDG_GetPeriod(void);
void HAL_RTCWDG_Feed(void);
void HAL_RTCWDG_Reboot(void);

#ifdef __cplusplus
}
#endif

#endif /* _STANDBY_HAL_RTCWDG_H_ */

