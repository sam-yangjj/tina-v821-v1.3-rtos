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

#include <hal_def.h>
#include <hal_sysctrl.h>
#include <hal_pwrctrl.h>
#include <hal_ccu.h>
#include <hal_sram.h>

void HAL_SRAM_Init(void)
{
	/* SRAM set la mode */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(SYSCTRL + SRAM_CTRL_REG), SRAM_LA_MODE_CTRL_SHIFT, SRAM_LA_MODE_CTRL_VMASK, 0U);
	/* SRAM set boot mode */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(SYSCTRL + SRAM_CTRL_REG), SRAM_BMODE_CTRL_SHIFT, SRAM_BMODE_CTRL_VMASK, 1U);

	/* vin reset */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_RESET0_REG), HRESETN_MCSI_SW_SHIFT, HRESETN_MCSI_SW_VMASK, 0U);
	/* ve reset */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_RESET1_REG), VE_RSTN_SW_SHIFT, VE_RSTN_SW_VMASK, 0U);
	/* vin clock reset*/
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_CLK_GATING0_REG), MCSI_AHB_CLK_EN_SHIFT, MCSI_AHB_CLK_EN_VMASK, 0U);
	/* ve clock reset */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_CLK_GATING1_REG), VE_AHB_CLK_EN_SHIFT, VE_AHB_CLK_EN_VMASK, 0U);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_CLK_GATING2_REG), VE_HCLK_EN_SHIFT, VE_HCLK_EN_VMASK, 0U);

	/* vin clock release */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_CLK_GATING0_REG), MCSI_AHB_CLK_EN_SHIFT, MCSI_AHB_CLK_EN_VMASK, 1U);
	/* ve clock release */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_CLK_GATING1_REG), VE_AHB_CLK_EN_SHIFT, VE_AHB_CLK_EN_VMASK, 1U);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_CLK_GATING2_REG), VE_HCLK_EN_SHIFT, VE_HCLK_EN_VMASK, 1U);
	/* vin release*/
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_RESET0_REG), HRESETN_MCSI_SW_SHIFT, HRESETN_MCSI_SW_VMASK, 1U);
	/* ve release */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_RESET1_REG), VE_RSTN_SW_SHIFT, VE_RSTN_SW_VMASK, 1U);

	/* sram clock on */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_AON + RAM_RET_CFG_REG), RAM_RET_CLKON_SHIFT, RAM_RET_CLKON_VMASK, 1U);
	/* vin sram entr retention after sleep */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_AON + RAM_RET_CFG_REG), VIN_BOOTRAM_REG_CFG_SHIFT, VIN_BOOTRAM_REG_CFG_VMASK, 1U);
	/* ve sram enter retention after sleep */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_AON + RAM_RET_CFG_REG), VE_BOOTRAM_RET_CFG_SHIFT, VE_BOOTRAM_RET_CFG_VMASK, 1U);
}

