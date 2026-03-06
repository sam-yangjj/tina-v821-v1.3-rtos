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

#include <hal_ccu.h>
#include <hal_ts.h>

typedef struct {
	uint32_t reg_off;
	uint32_t pll_d_shift;
	uint32_t pll_d_vmask;
} pll_priv_t;

const pll_priv_t pll_priv[] = {
	{.reg_off = PLL_CPU_CTRL_REG ,   .pll_d_shift = CPU_PLL_D_SHIFT,   .pll_d_vmask = CPU_PLL_D_VMASK},
	{.reg_off = PLL_PERI_CTRL_REG0 , .pll_d_shift = PERI_PLL_D_SHIFT,  .pll_d_vmask = PERI_PLL_D_VMASK},
	{.reg_off = PLL_VIDEO_CTRL_REG , .pll_d_shift = VIDEO_PLL_D_SHIFT, .pll_d_vmask = VIDEO_PLL_D_VMASK},
	{.reg_off = PLL_CSI_CTRL_REG ,   .pll_d_shift = CSI_PLL_D_SHIFT,   .pll_d_vmask = CSI_PLL_D_VMASK},
	{.reg_off = PLL_DDR_CTRL_REG ,   .pll_d_shift = DDR_PLL_D_SHIFT,   .pll_d_vmask = DDR_PLL_D_VMASK},
};

uint32_t HAL_CCU_GetHoscFreq(void)
{
	if (*(volatile uint32_t *)(CCU_AON + 0x404) & (1 << 31)) {
		return HOSC_CLOCK_24M;
	}
	return HOSC_CLOCK_40M;
}

void HAL_CCU_ResetCPUXCfg(uint32_t rst)
{
	rst = (rst ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_RESET1_REG), CPUX_CFG_RSTN_SW_SHIFT, CPUX_CFG_RSTN_SW_VMASK, rst);
}

void HAL_CCU_ResetCPUX(uint32_t rst)
{
	rst = (rst ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_RESET1_REG), CPUX_RSTN_SW_SHIFT, CPUX_RSTN_SW_VMASK, rst);
}

void HAL_CCU_EnableCPUXClk(uint32_t en)
{
	en = (en ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_AON + CPUX_CLK_REG), CPUX_CLK_EN_SHIFT, CPUX_CLK_EN_VMASK, en);
}

void HAL_CCU_EnableCPUXMtClk(uint32_t en)
{
	en = (en ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + CPUX_MT_CLK_REG), CPUX_MT_CLK_EN_SHIFT, CPUX_MT_CLK_EN_VMASK, en);
}

void HAL_CCU_ResetCPUXMsgbox(uint32_t en)
{
	en = (en ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + CCU_APP_CLK_REG), CPUX_MSGBOX_CLK_EN_SHIFT, CPUX_MSGBOX_CLK_EN_VMASK, en);
}

void HAL_CCU_EnableCPUXMsgboxClk(uint32_t en)
{
	en = (en ? 1 : 0);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + BUS_RESET1_REG), CPUX_MSGBOX_RSTN_SHIFT, CPUX_MSGBOX_RSTN_VMASK, en);
}

int HAL_CCU_ConfigPll(pll_param_t *param, uint32_t pll_n, uint32_t pll_d)
{
	param->pll_n = pll_n;
	param->pll_d = pll_d;
	return 0;
}

int HAL_CCU_IsPllOn(pll_idx_t idx)
{
	if (HAL_REG_32BIT(CCU_AON + pll_priv[idx].reg_off) & (1 << PLL_EN_SHIFT)) {
		return 1;
	}
	return 0;
}

int HAL_CCU_OpenPll(pll_idx_t idx, pll_param_t *param)
{
	uint32_t reg_addr;
	uint32_t check_cnt, check_retry;
	uint32_t pll_n, pll_d, pll_d_mask;

	reg_addr = CCU_AON + pll_priv[idx].reg_off;

	if (HAL_CCU_IsPllOn(idx)) {
		if (!HAL_GET_BITS(reg_addr, PLL_OUTPUT_GATING_MASK)) {
			HAL_SET_BITS(reg_addr, 1 << PLL_OUTPUT_GATING_SHIFT);
		}
		return 0;
	}

	HAL_SET_BITS(reg_addr, 1 << PLL_LDO_EN_SHIFT);
	HAL_CLR_BITS(reg_addr, PLL_EN_MASK | PLL_OUTPUT_GATING_MASK | PLL_LOCK_EN_MASK);
	/* confirm pll_en, pll_lock_en, pll_output is clear */
	check_retry = 0;
	while (check_retry < 10) {
		if (!HAL_GET_BITS(reg_addr, PLL_EN_MASK | PLL_OUTPUT_GATING_MASK | PLL_LOCK_EN_MASK)) {
			break;
		}
		HAL_TS_Udelay(3);
		check_retry++;
	}

	hal_assert_reboot((!HAL_GET_BITS(reg_addr, PLL_EN_MASK | PLL_OUTPUT_GATING_MASK | PLL_LOCK_EN_MASK)), CCU_PLL_OPEN_PREPARE_ERR);

	/* set lock time in different hosc for CPU PLL */
	if (idx == PLL_IDX_CPU) {
		if (HAL_CCU_GetHoscFreq() == HOSC_CLOCK_40M) {
			/* hosc freq is 40M */
			HAL_CLR_SET_BITS(reg_addr, PLL_LOCK_TIME_MASK, 0x03 << PLL_LOCK_TIME_SHIFT);
		} else {
			/* hosc freq is 24M */
			HAL_CLR_SET_BITS(reg_addr, PLL_LOCK_TIME_MASK, 0x02 << PLL_LOCK_TIME_SHIFT);
		}
	}

	/* set pll param if param is not NULL */
	if (param) {
		pll_n = (param->pll_n & PLL_N_VMASK) << PLL_N_SHIFT;
		pll_d = (param->pll_d & pll_priv[idx].pll_d_vmask) << pll_priv[idx].pll_d_shift;
		pll_d_mask = pll_priv[idx].pll_d_vmask << pll_priv[idx].pll_d_shift;
		HAL_CLR_SET_BITS(reg_addr, pll_d_mask, pll_d);
		HAL_CLR_SET_BITS(reg_addr, PLL_N_MASK, pll_n);
	}

	/* open pll */
	HAL_SET_BITS(reg_addr, 1 << PLL_EN_SHIFT);
	HAL_SET_BITS(reg_addr, 1 << PLL_LOCK_EN_SHIFT);
	check_cnt = 0;
	check_retry = 0;
	while (check_cnt < 3 && check_retry < 100) {
		if ((HAL_REG_32BIT(reg_addr) & PLL_LOCK_MASK)) {
			check_cnt++;
		} else {
			check_cnt = 0;
		}
		check_retry++;
		HAL_TS_Udelay(3);
	}

	hal_assert_reboot(check_cnt == 3, CCU_PLL_OPEN_ERR);

	HAL_SET_BITS(reg_addr, 1 << PLL_OUTPUT_GATING_SHIFT);

	return 0;
}

int HAL_CCU_ClosePll(pll_idx_t idx)
{
	uint32_t reg_addr;

	reg_addr = CCU_AON + pll_priv[idx].reg_off;

	HAL_CLR_BITS(reg_addr, PLL_EN_MASK | PLL_LDO_EN_MASK | PLL_OUTPUT_GATING_MASK);
	HAL_CLR_BITS(reg_addr, PLL_LOCK_EN_MASK);
	return 0;
}

int HAL_CCU_PeriPllOutput(uint32_t en)
{
	if (en) {
		HAL_REG_32BIT(CCU_AON + PLL_PERI_CTRL_REG1) = 0x0000ffff;
	} else {
		HAL_REG_32BIT(CCU_AON + PLL_PERI_CTRL_REG1) = 0x00000000;
	}
	return 0;
}

int HAL_CCU_EnableMonitor(uint32_t en)
{
	uint32_t val;

	val = (AHB_MONITOR_EN_VMASK << AHB_MONITOR_EN_SHIFT) | (SD_MONITOR_EN_VMASK << SD_MONITOR_EN_SHIFT);
	if (en) {
		HAL_REG_32BIT(CCU_APP + BUS_CLK_GATING2_REG) |= val;
	} else {
		HAL_REG_32BIT(CCU_APP + BUS_CLK_GATING2_REG) &= ~val;
	}
	return 0;
}

