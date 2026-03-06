/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************
 *
 * @file    rcosc_calib.c
 * @brief   rcosc calib
 *
 *
 * Copyright (C) 2019 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
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
 *
 ******************************************************************************
 */

#include "pal_clk.h"
#include "pal_rcosc_cali.h"

void ble_hosc_32k_start(void)
{
	pal_clk_enable_ble_div_32k();
}

void ble_hosc_32k_stop(void)
{
	pal_clk_disable_ble_div_32k();
}

void ble_rcosc_cal_32k_start(void)
{
	pal_clk_set_ble_sel_parent_rccail_32k();
}

void ble_lfclkOrRcosc_32k_start(void)
{
	pal_clk_set_ble_sel_parent_losc_32k();
}

void ble_32k_init(BLE_32KClockType type)
{
	switch (type) {
	case BLE_32K_CLOCK_RCCAL:
		ble_rcosc_cal_32k_start();
		break;
	case BLE_32K_CLOCK_HOSC32K:
		ble_hosc_32k_start();
		break;
	case BLE_32K_CLOCK_LFCLKORRCOSC:
	default:
		ble_lfclkOrRcosc_32k_start();
		break;
	}
	return;
}

void ble_32k_deinit(void)
{
	ble_lfclkOrRcosc_32k_start();
	return;
}

#if BLE_RCCAL_INACCURATE_WALKAROUND
/* set rccal clk parm */
void ble_rccal_set(uint8_t phase1_num, uint8_t phase2_num, uint16_t wup_time, uint32_t phase2_time)
{
#if 0
	RCOCALI_ConfigParam configParm = {
		.mode = PRCM_RCOSC_WK_MODE_SEL_SCALE,
		.phase2_times = phase2_time,
		.phase3_times = PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_128,
		.phase1_num = phase1_num,
		.phase2_num = phase2_num,
		.wup_time = wup_time,
	};
	HAL_RcoscCali_Config(&configParm);
#endif
}
#endif

#if 0
#include "common_types.h"
#include "logAssert.h"

#include "rcosc_cali.h"
#include "hal_defs.h"
#include "gprcm.h"

/** Constants -------------------------------------------------------------- */

#define RCOCAL_DFT_HOSC_CLK                   24000000
#define RCOCAL_DFT_WUP_TIME                   640
#define RCOCAL_DFT_PHASE_1_CNT                5
#define RCOCAL_DFT_PHASE_2_TIME               3
#define RCOCAL_DFT_PHASE_2_CNT                3
#define RCOCAL_DFT_PHASE_3_TIME               3

#define RCOCAL_OUT_CLK                        32000
#define RCOCAL_CNT_TAGT                       8192
#define RCOCAL_DIVIDEND(hosc)                 (RCOCAL_CNT_TAGT * ((hosc) / RCOCAL_OUT_CLK))

#define RCOCALI_BASE                          (0x40046000)

#define RCO_CNT_TARGET                        (RCOCALI_BASE + 0x00)
#define RCO_CNT_TARGET_MASK                     (0xFFFF)

#define RCO_DIVIDEND_REG                      (RCOCALI_BASE + 0x04)

#define RCO_DCXO_CNT                          (RCOCALI_BASE + 0x08)
#define RCO_DCXO_CNT_MASK                       (0x3FFFFF)

#define RCO_ABNORMAL_MONITOR                  (RCOCALI_BASE + 0x0C)
#define QUOTIENT_INVALID_FLAG                   (0x1<<0)
#define DENOMINATOR_INVALID_FLAG                (0x1<<1)
#define PARAM_ERR_CLR                           (0x1<<2)
#define RCOSC_MONITOR_EN                        (0x1<<4)
#define RCOSC_ABNORMAL_FLAG_CLR                 (0x1<<5)
#define RCOSC_LONG_LEVEL_FOUND                  (0x1<<6)
#define RCOSC_GLITCH_FOUND                      (0x1<<7)
#define RCOSC_GLITCH_MAX_WIDTH_MASK             (0x7F<<8)
#define RCOSC_LEVEL_MAX_WIDTH_MASK              (0xFFFF<<16)

#define RCO_INTERRUPT_CTRL                    (RCOCALI_BASE + 0x10)
#define INT_CLR                                 (0x1<<1)
#define INT_EN                                  (0x1<<0)

#define REG_GPRCM_CLK32K_SW_CTRL              (GPRCM_BASE + 0x0220)
#define CLK32K_SW_FORCE_EN                      (0x01000000) /* 24 bit */
#define CLK32K_SW_FORCE_DOWN                    (0x04000000) /* 26 bit */
#define CLK32K_SW_FORCE_READY                   (0x08000000) /* 27 bit */

#define CLK32K_SW_FORCE_MASK                    (0x0D000000)

/** Data Types ------------------------------------------------------------- */

/** Variable ----------------------------------------------------------------*/

static uint32_t rcocal_hosc_clk = 0;
static uint32_t rcocal_wup_time = 0;
static uint32_t rcocal_wup_time_phase = 0;

/** Function --------------------------------------------------------------- */

void rcosc_cali_init(void)
{
	rcosc_cali_hosc_set(RCOCAL_DFT_HOSC_CLK);
	rcosc_cali_wup_set(RCOCAL_DFT_WUP_TIME, RCOCAL_DFT_PHASE_1_CNT,
	                   RCOCAL_DFT_PHASE_2_TIME, RCOCAL_DFT_PHASE_2_CNT,
	                   RCOCAL_DFT_PHASE_3_TIME);

	rcosc_cali_restart();
	ble_clk32k_switch(0);
}

void rcosc_cali_deinit(void)
{
	/* disable RCOSC_CALI clock gating */
	HAL_CLR_MASK32(GPRCM_WKUP_SRC_BUS_CLK, 0x1 << RCCAL_CLK_GATING);

	/* hold RCOSC_CALI module reset */
	HAL_CLR_MASK32(GPRCM_WKUP_SRC_RST, 0x1 << RCCAL_RSTN);
}

void rcosc_cali_restart(void)
{
	/* enable RCOSC_CALI clock gating */
	HAL_SET_MASK32(GPRCM_WKUP_SRC_BUS_CLK, 0x1 << RCCAL_CLK_GATING);

	/* release RCOSC_CALI module reset */
	HAL_SET_MASK32(GPRCM_WKUP_SRC_RST, 0x1 << RCCAL_RSTN);

	/* config dividend and dcxo_cnt */
	HAL_WRITE32(RCO_CNT_TARGET, RCOCAL_CNT_TAGT);
	HAL_WRITE32(RCO_DIVIDEND_REG, RCOCAL_DIVIDEND(rcocal_hosc_clk));

	/* phase 1 wakeup config */
	HAL_WRITE_MASK32(BLE_RCOSC_CALIB_REG0, RCO_WUP_TIME_MASK, 0, rcocal_wup_time);
	HAL_SET_MASK32(BLE_RCOSC_CALIB_REG0, RCO_WUP_TIME_EN);

	/* enable rcosc calibration function */
	HAL_SET_MASK32(BLE_RCOSC_CALIB_REG0, RCO_CALIB_EN);

	/* config phase 1/2/3 time and cnt */
	HAL_WRITE32(BLE_RCOSC_CALIB_REG1, (rcocal_wup_time_phase &
	              (RCO_SCALE_PH2_WUP_TIMES
	              | RCO_SCALE_PH3_WUP_TIMES
	              | RCO_SCALE_PH1_NUM_MASK
	              | RCO_SCALE_PH2_NUM_MASK)));

	/* select scale wake up method, and enable */
	HAL_SET_MASK32(BLE_RCOSC_CALIB_REG1, RCO_WUP_MODE_SCALE); // scale
	//HAL_CLR_MASK32(BLE_RCOSC_CALIB_REG1, RCO_WUP_MODE_SCALE); // nomal

	/* RCOSC Calibration Start Source*/
	HAL_MODIFY_MASK32(GPRCM_POWERCTRL_CFG, RCOCAL_START_SRC_LDO1_OFF, RCOCAL_START_SRC_APP_SLP);

	/* ble clk 32k switch config
	 * switch : enable
	 * div clock src : hosc
	 * sel rccal 32k : rcosc calibration
	 */
	HAL_SET_MASK32(BLE_CLK32K_SWITCH_REG0, CLK32K_AUTO_SW_EN);

	/* Note: SDK config CLK32M, FPGA CLK32M = DCXO = 24MHz */
	HAL_CLR_MASK32(BLE_CLK32K_SWITCH_REG0, DIV_CLK_SRC_SEL_32M); /* HFCLK */
	/* output 32k */
	HAL_WRITE_MASK32(BLE_CLK32K_SWITCH_REG0, DIV_HALFCYCLE_TARGET, 16, rcocal_hosc_clk / (2 * RCOCAL_OUT_CLK) - 1);

	HAL_SET_MASK32(BLE_CLK32K_SWITCH_REG0, CLK_32K_SRC_SEL_RCCAL);
}

void rcosc_cali_hosc_set(uint32_t hosc_clk)
{
	rcocal_hosc_clk = hosc_clk;
}

void rcosc_cali_wup_set(
                uint16_t phase_1_wup_time,
                uint8_t phase_1_wup_cnt,
                uint8_t phase_2_wup_time,
                uint8_t phase_2_wup_cnt,
                uint8_t phase_3_wup_time)
{
	rcocal_wup_time = phase_1_wup_time & RCO_WUP_TIME_MASK;
	rcocal_wup_time_phase = (phase_2_wup_time & 0x07) << 4;
	rcocal_wup_time_phase |= (phase_3_wup_time & 0x07) << 8;
	rcocal_wup_time_phase |= (phase_1_wup_cnt & 0x0F) << 12;
	rcocal_wup_time_phase |= (phase_2_wup_cnt & 0x0F) << 16;
}

void ble_clk32k_switch(uint8_t switch_to_rcosc)
{
	/* enable, DPLL clk switch to RCOSC clk */
	if (switch_to_rcosc) {

		/* en = 1, down = 1, ready = 1 */
		HAL_MODIFY_MASK32(REG_GPRCM_CLK32K_SW_CTRL,
		                  CLK32K_SW_FORCE_MASK,
		                  CLK32K_SW_FORCE_EN
		                  | CLK32K_SW_FORCE_DOWN
		                  | CLK32K_SW_FORCE_READY);

		/* delay 100us */
		os_delay_us(200);

		/* en = 1, down = 0, ready = 0 */
		HAL_MODIFY_MASK32(REG_GPRCM_CLK32K_SW_CTRL,
		                  CLK32K_SW_FORCE_MASK,
		                  CLK32K_SW_FORCE_EN);
	}

	/* disable, RCOSC clk switch to DPLL clk */
	else {
		/* en = 1, down = 0, ready = 1 */
		HAL_MODIFY_MASK32(REG_GPRCM_CLK32K_SW_CTRL,
		                  CLK32K_SW_FORCE_MASK,
		                  CLK32K_SW_FORCE_EN
		                  | CLK32K_SW_FORCE_READY);
	}
}

uint32_t ble_clk32k_switch_loss_get(void)
{
	uint32_t loss_clk = 0;
	uint32_t hclk = rcocal_hosc_clk / 1000000;

	loss_clk = HAL_READ_MASK32(BLE_CLK32K_SWITCH_REG1,
	                    CLK32K_SW_OFFSET_DOWN_MASK,
	                    16);

	loss_clk += HAL_READ_MASK32(BLE_CLK32K_SWITCH_REG1,
	                CLK32K_SW_OFFSET_ON_MASK,
	                0);

	loss_clk /= hclk;

	return loss_clk;
}

#endif /* SUPPORT_RCOSC_CALIB */

/******************* (C) COPYRIGHT 2019 XRADIO, INC. ******* END OF FILE *****/
