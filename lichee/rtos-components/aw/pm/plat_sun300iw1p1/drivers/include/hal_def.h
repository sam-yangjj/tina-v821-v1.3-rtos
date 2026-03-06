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

#ifndef _STANDBY_HAL_DEF_H_
#define _STANDBY_HAL_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define     __IO    volatile
#define HAL_REG_32BIT(reg_addr)  (*((__IO uint32_t *)(reg_addr)))
#define HAL_REG_16BIT(reg_addr)  (*((__IO uint16_t *)(reg_addr)))
#define HAL_REG_8BIT(reg_addr)   (*((__IO uint8_t  *)(reg_addr)))
#define HAL_GET_BIT_VAL(reg, shift, vmask)  (((reg) >> (shift)) & (vmask))
#define HAL_SET_BIT_VAL(reg, shift, vmask, val) \
	((reg) = ((reg) & (~((vmask) << (shift)))) | (((val) & (vmask)) << (shift)))
#define HAL_MODIFY_REG(reg, clr_mask, set_mask) \
	((reg) = (((reg) & (~(clr_mask))) | (set_mask)))

#define HAL_GET_BITS(reg_addr, mask)   (HAL_REG_32BIT(reg_addr) & (mask))
#define HAL_CLR_BITS(reg_addr, clr)    (HAL_REG_32BIT(reg_addr) &= ~(clr))
#define HAL_SET_BITS(reg_addr, set)    (HAL_REG_32BIT(reg_addr) |= (set))
#define HAL_CLR_SET_BITS(reg_addr, clr, set) \
do {\
	HAL_REG_32BIT(reg_addr) &= ~(clr);\
	HAL_REG_32BIT(reg_addr) |= (set);\
} while (0)

extern void HAL_RTCWDG_Reboot();

#define HAL_ASSERT_REGISTER   (0x4a00020c)
#ifdef HAL_ASSERT_REGISTER
#define hal_assert_reboot(cond, err)\
do {\
	if (!(cond)) {\
		HAL_REG_32BIT(HAL_ASSERT_REGISTER) = (0xabab0000 | (err & 0xffff));\
		HAL_RTCWDG_Reboot();\
	}\
} while (0)
#else
#define hal_assert_reboot(cond, err)
#endif

//#define HAL_DEBUG_REACH_REGISTER   (0x4a000214)
#ifdef HAL_DEBUG_REACH_REGISTER
#define hal_debug_reach(index)\
do {\
	if (HAL_REG_32BIT(HAL_DEBUG_REACH_REGISTER) & (1 << (index))) {\
		HAL_RTCWDG_Reboot();\
	}\
} while (0)
#else
#define hal_debug_reach(index)
#endif

/* assert err value */
#define SPIF_FLUSH_CACHE_ADDR_UNALIGN    (1)
#define SPIF_FLUSH_CACHE_SIZE_UNALIGN    (2)
#define SPIF_INVALI_CACHE_ADDR_UNALIGN   (3)
#define SPIF_INVALI_CACHE_SIZE_UNALIGN   (4)

#define CCU_PLL_OPEN_PREPARE_ERR         (5)
#define CCU_PLL_OPEN_ERR                 (6)

#ifdef __cplusplus
}
#endif
#endif /* _STANDBY_HAL_DEF_H_ */

