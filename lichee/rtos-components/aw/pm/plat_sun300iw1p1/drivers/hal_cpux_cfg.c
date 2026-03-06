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
#include <stdio.h>
#include <hal_cpux_cfg.h>

uint32_t HAL_CPUX_CFG_IsEnterWfi(void)
{
	return HAL_GET_BIT_VAL(HAL_REG_32BIT(CPUX_CFG + RISCV_STATUS_REG), HART0_CORE_WFI_MODE_SHIFT, HART0_CORE_WFI_MODE_VMASK);
}

void HAL_CPUX_CFG_EnableWfiMode(uint32_t en)
{
	en = (en ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CPUX_CFG + RISCV_WFI_MODE_REG), WFI_MODE_SHIFT, WFI_MODE_VMASK, en);
}

void HAL_CPUX_CFG_SetStartAddr(uint32_t addr)
{
	HAL_REG_32BIT(CPUX_CFG + RISCV_STA_ADD_REG) = addr;
	while (HAL_REG_32BIT(CPUX_CFG + RISCV_STA_ADD_REG) != addr) {
		printf("cpux address: 0x%08x\n", HAL_REG_32BIT(CPUX_CFG + RISCV_STA_ADD_REG));
	}
}

