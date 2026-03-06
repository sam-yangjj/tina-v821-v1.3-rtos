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

#ifndef _STANDBY_HAL_SYSCTRL_H_
#define _STANDBY_HAL_SYSCTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SYSCTRL                     (0x43000000)
#define SRAM_CTRL_REG               (0x0004)
#define SRAM_BMODE_CTRL_SHIFT       (24)
#define SRAM_BMODE_CTRL_VMASK       (0x1U)
#define SRAM_LA_MODE_CTRL_SHIFT     (0)
#define SRAM_LA_MODE_CTRL_VMASK     (0x1U)
#define EXT_DDR_RES_CTRL_REG        (0x164)
#define RES_TRIM_KEY_FIELD          (6449)
#define RES_240_TRIM_MASK           (0xFFFF)
#define INT_USB_RES_CTRL_REG        (0x178)
#define USB_RES_TRIM_MASK           (0xFF)
#define INT_CSI_RES_CTRL_REG        (0x17C)
#define CSI_RES_TRIM_MASK           (0xFF)

#define SYSRTC_CTRL_REG             (0x4a000c00)
#define SYSRTC_CLK_SRC_MASK         (0x7)
#define SYSRTC_LFCLK_SRC_SEL_SHIFT  (2)
#define SYSRTC_RCOSC_CAL_MASK       (0x1)
#define SYSRTC_RCOSC_CAL_SHIFT      (8)

#ifdef __cplusplus
}
#endif
#endif /* _STANDBY_HAL_SYSCTRL_H_ */

