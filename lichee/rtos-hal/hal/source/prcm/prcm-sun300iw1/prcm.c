/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 *the the People's Republic of China and other countries.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_clk.h>
#include <hal_time.h>
#include "prcm.h"


/*
 * bit field definition of PRCM->LPCLK_CTRL 0x0000
 */
void HAL_PRCM_SetSys32KClkSel(PRCM_SYS32KSrc source)
{
	HAL_MODIFY_REG(PRCM->LPCLK_CTRL, PRCM_SYS_32K_SEL_MASK, source);
}

void HAL_PRCM_SetLpclkClkSel(PRCM_LPCLK32KSrc source)
{
	HAL_MODIFY_REG(PRCM->LPCLK_CTRL, PRCM_LPCLK_32K_SEL_MASK, source);
}

void HAL_PRCM_SetRcoTrim(uint8_t trim)
{
	HAL_ASSERT_PARAM(trim <= PRCM_RCO_TRIM_MAX);
	HAL_MODIFY_REG(PRCM->LPCLK_CTRL, PRCM_RCO_TRIM_MASK, PRCM_RCO_TRIM_VAL(trim));
}

void HAL_PRCM_SetXO32KCLTrim(uint8_t trim)
{
	HAL_ASSERT_PARAM(trim <= PRCM_XO32K_CL_TRIM_MAX);
	HAL_MODIFY_REG(PRCM->LPCLK_CTRL, PRCM_XO32K_CL_TRIM_MASK, PRCM_XO32K_CL_TRIM_VAL(trim));
}

void HAL_PRCM_SetXO32KEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->LPCLK_CTRL, PRCM_XO32K_EN_BIT);
	else
		HAL_CLR_BIT(PRCM->LPCLK_CTRL, PRCM_XO32K_EN_BIT);
}

/*
 * bit field definition of PRCM->LPCLK_STATUS 0x0004
 */
int HAL_GET_XO32KReadyFlag(void)
{
	return HAL_GET_BIT(PRCM->LPCLK_STATUS, PRCM_XO32K_READY_FLAG_BIT);
}

/*
 * bit field definition of PRCM->RCOSC_CALIB_REG0~3 0x000c ~ 0x0018
 */
void HAL_PRCM_SetRcoCalib(uint32_t en, PRCM_RCOSC_WkModSel mode, PRCM_RCOSC_NormalWkTimesSel sel,
                          uint32_t phase1, uint32_t phase2, uint32_t wk_time_en, uint32_t wk_time)
{
	if (en) {
		uint32_t mask;

		/* method */
		mask = PRCM_RCOSC_WK_MODE_SEL_MASK | PRCM_RCOSC_NORMAL_WK_TIMES_SEL_MASK |
		       PRCM_RCOSC_SCALE_PHASE1_NUM_MASK | PRCM_RCOSC_SCALE_PHASE2_NUM_MASK |
		       PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_MASK | PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_MASK;
		HAL_MODIFY_REG(PRCM->RCOSC_CALIB_REG1, mask, mode | sel
				| PRCM_RCOSC_SCALE_PHASE1_NUM_VAL(phase1) | PRCM_RCOSC_SCALE_PHASE2_NUM_VAL(phase2));

		HAL_SET_BIT(PRCM->RCOSC_CALIB_REG0, PRCM_BLE_RCOSC_CALIB_EN_BIT);
	} else {
		HAL_CLR_BIT(PRCM->RCOSC_CALIB_REG0, PRCM_BLE_RCOSC_CALIB_EN_BIT);
	}

	if (wk_time_en) {
		/* eg. phase3 = wup_time * PRCM_RCOSC_SCALE_PHASE3_WUP_TIMES_10 = 2.5ms,
		 *     8 / 32000 = 0.25ms
		 */
		HAL_MODIFY_REG(PRCM->RCOSC_CALIB_REG0, PRCM_RCOSC_WK_TIME_MASK, PRCM_RCOSC_WK_TIME_VAL(wk_time));
		HAL_SET_BIT(PRCM->RCOSC_CALIB_REG0, PRCM_RCOSC_WK_TIME_EN_BIT);
	} else {
		HAL_CLR_BIT(PRCM->RCOSC_CALIB_REG0, PRCM_RCOSC_WK_TIME_EN_BIT);
	}
}

void HAL_PRCM_SetRcoscCntTarget(uint16_t rcosc_cnt)
{
	HAL_MODIFY_REG(PRCM->RCOSC_CALIB_REG2, PRCM_RCOSC_CNT_TARGET_MASK, PRCM_RCOSC_CNT_TARGET_VAL(rcosc_cnt));
}

void HAL_PRCM_SetRcoscDividend(uint16_t rcosc_div)
{
	HAL_MODIFY_REG(PRCM->RCOSC_CALIB_REG3, PRCM_RCOSC_DIVIDEND_MASK, PRCM_RCOSC_DIVIDEND_VAL(rcosc_div));
}

/*
 * bit field definition of PRCM->RESET_CTRL 0x001c
 */
void HAL_PRCM_SetResetMod(PRCM_ResetModule mod, uint8_t release_reset)
{
	switch (mod) {
		case PRCM_RESET_RCCAL:
			HAL_MODIFY_REG(PRCM->RESET_CTRL, PRCM_RCCAL_RSTN_MASK, PRCM_RCCAL_RSTN_VAL(release_reset));
			break;
		case PRCM_RESET_RTC_TIMER:
			HAL_MODIFY_REG(PRCM->RESET_CTRL, PRCM_RTC_TIMER_MASK, PRCM_RTC_TIMER_VAL(release_reset));
			break;
		case PRCM_RESET_RTC_WATCHDOG:
			HAL_MODIFY_REG(PRCM->RESET_CTRL, PRCM_RTC_WDG_RSTN_MASK, PRCM_RTC_WDG_RSTN_VAL(release_reset));
			break;
		default:
			break;
	}
}

/*
 * bit field definition of PRCM->POR_RST_CNT and POR_RST_CTRL  0x0020 ~ 0x0024
 */

void HAL_PRCM_SetPorRstCnt(uint32_t reset_cnt)
{
	if (reset_cnt > 0x3FFU)
		reset_cnt = 0x3FF;
	HAL_MODIFY_REG(PRCM->POR_RST_CNT, PRCM_POR_RESET_CNT_MASK, PRCM_POR_RESET_CNT_VAL(reset_cnt));
}

void HAL_PRCM_SetPorVccRstDet(PRCM_VsysDetRst vsys_det, PRCM_V33DetRst vcc_det)
{
	HAL_MODIFY_REG(PRCM->POR_RST_CTRL, PRCM_VSYS_DET_RSTN_MASK, vsys_det);
	HAL_MODIFY_REG(PRCM->POR_RST_CTRL, PRCM_V33_DET_RSTN_MASK, vcc_det);
}

/*
 * bit field definition of PRCM->OTP_CTRL  0x0028
 */

void HAL_PRCM_SetOtpConfig(PRCM_OTPParm config)
{
	HAL_MODIFY_REG(PRCM->OTP_CTRL, PRCM_OTP_IRQEN_MASK, PRCM_OTP_IRQEN_VAL(config.irq_en));
	HAL_MODIFY_REG(PRCM->OTP_CTRL, PRCM_OTP_CTRL_EN_MASK, PRCM_OTP_CTRL_EN_VAL(config.ctrl_en));
	HAL_MODIFY_REG(PRCM->OTP_CTRL, PRCM_OTP_DET_EN_MASK, PRCM_OTP_DET_EN_VAL(config.det_en));
}

uint8_t HAL_PRCM_GetOtpStatus(void)
{
	return HAL_GET_BIT_VAL(PRCM->OTP_CTRL, PRCM_OTP_STATUS_SHIFT, PRCM_OTP_STATUS_VMASK);
}

uint8_t HAL_PRCM_GetOtpIrqStatus(void)
{
	return HAL_GET_BIT_VAL(PRCM->OTP_CTRL, PRCM_OTP_IRQ_STATUS_SHIFT, PRCM_OTP_IRQ_STATUS_VMASK);
}

void HAL_PRCM_SetOtpIrqClean(void)
{
	HAL_SET_BIT(PRCM->OTP_CTRL, PRCM_OTP_IRQ_STATUS_MASK);
}

void HAL_PRCM_SetOtpTrim(PRCM_OtpTrim otp_trim)
{
	HAL_MODIFY_REG(PRCM->OTP_CTRL, PRCM_OTP_TRIM_MASK, PRCM_OTP_TRIM_VAL(otp_trim));
}

/*
 * bit field definition of PRCM->VCC33_DET_IRQ  0x002c
 */
uint8_t HAL_PRCM_GetV33DetOut(void)
{
	return HAL_GET_BIT_VAL(PRCM->VCC33_DET_IRQ, PRCM_VCC33_DET_OUT_SHIFT, PRCM_VCC33_DET_OUT_VMASK);
}

void HAL_PRCM_SetV33DetIrqEnable(uint8_t enable)
{
	if(enable)
		HAL_SET_BIT(PRCM->VCC33_DET_IRQ, PRCM_VCC33_DET_OUT_MASK);
	else
		HAL_CLR_BIT(PRCM->VCC33_DET_IRQ, PRCM_VCC33_DET_OUT_MASK);
}

uint8_t HAL_PRCM_GetVcc33DetIrqStatus(void)
{
	return HAL_GET_BIT_VAL(PRCM->VCC33_DET_IRQ, PRCM_VCC33_DET_IRQ_STATUS_SHIFT, PRCM_VCC33_DET_IRQ_STATUS_VMASK);
}

void HAL_PRCM_SetVcc33DetIrqStatusClean(void)
{
	HAL_SET_BIT(PRCM->VCC33_DET_IRQ, PRCM_VCC33_DET_IRQ_STATUS_MASK);
}

/*
 * bit field definition of PRCM->RTC_IO_WAKE_EN         0x0050
 * bit field definition of PRCM->RTC_IO_WAKE_DEB_CLK    0x0054
 * bit field definition of PRCM->RTC_IO_WAKE_ST         0x0058
 * bit field definition of PRCM->RTC_IO_HOLD_CTRL       0x005c
 * bit field definition of PRCM->RTC_IO_WUP_GEN         0x0060
 * bit field definition of PRCM->RTC_IO_WUP_DEB_CYCLES0 0x0064
 * bit field definition of PRCM->RTC_IO_WUP_DEB_CYCLES1 0x0068
 */

void HAL_PRCM_WakeupIOEnable(PRCM_WakeIO ioMask)
{
	HAL_SET_BIT(PRCM->RTC_IO_WAKE_EN, ioMask << PRCM_WAKEUP_IOx_EN_SHIFT);
}

void HAL_PRCM_WakeupIODisable(PRCM_WakeIO ioMask)
{
	HAL_CLR_BIT(PRCM->RTC_IO_WAKE_EN, ioMask << PRCM_WAKEUP_IOx_EN_SHIFT);
}

void HAL_PRCM_EnableWakeupIOx(uint8_t ioIndex, uint8_t enable)
{
	HAL_ASSERT_PARAM(ioIndex < WAKEUP_IO_MAX);

	if (enable)
		HAL_SET_BIT(PRCM->RTC_IO_WAKE_EN, 1 << ioIndex);
	else
		HAL_CLR_BIT(PRCM->RTC_IO_WAKE_EN, 1 << ioIndex);
}

void HAL_PRCM_WakeupIOSetRisingEvent(PRCM_WakeIO ioMask)
{
	HAL_SET_BIT(PRCM->RTC_IO_WAKE_EN, ioMask << PRCM_WAKEUP_IOx_MODE_SHIFT);
}

void HAL_PRCM_WakeupIOSetFallingEvent(PRCM_WakeIO ioMask)
{
	HAL_CLR_BIT(PRCM->RTC_IO_WAKE_EN, ioMask << PRCM_WAKEUP_IOx_MODE_SHIFT);
}

uint32_t HAL_PRCM_WakeupIOGetEventStatus(void)
{
	/*note: the func will return all WKUPIO event.*/
	return (PRCM->RTC_IO_WAKE_ST & WAKEUP_IO_MASK);
}

uint32_t HAL_PRCM_WakeupIOClearEventStatus(void)
{
	/*note: the func will clear all WKUPIO event.*/
	return (PRCM->RTC_IO_WAKE_ST |= WAKEUP_IO_MASK);
}

int HAL_PRCM_WakeupIOGetEventDetected(PRCM_WakeIO ioMask)
{
	return !!HAL_GET_BIT(PRCM->RTC_IO_WAKE_ST, ioMask);
}

void HAL_PRCM_WakeupIOClearEventDetected(PRCM_WakeIO ioMask)
{
	HAL_SET_BIT(PRCM->RTC_IO_WAKE_ST, ioMask);
}

void HAL_PRCM_WakeupIOEnableCfgHold(PRCM_WakeIO ioMask)
{
	HAL_SET_BIT(PRCM->RTC_IO_HOLD_CTRL, ioMask);
}

void HAL_PRCM_WakeupIODisableCfgHold(PRCM_WakeIO ioMask)
{
	HAL_CLR_BIT(PRCM->RTC_IO_HOLD_CTRL, ioMask);
}

void HAL_PRCM_WakeupIOEnableGlobal(void)
{
	HAL_SET_BIT(PRCM->RTC_IO_WUP_GEN, PRCM_WAKE_IO_GLOBAL_EN_BIT);
}

void HAL_PRCM_WakeupIODisableGlobal(void)
{
	HAL_CLR_BIT(PRCM->RTC_IO_WUP_GEN, PRCM_WAKE_IO_GLOBAL_EN_BIT);
}

void HAL_PRCM_SetWakeupIOxDebSrc(uint8_t ioIndex, PRCM_WakeIODebClkSrc src)
{
	HAL_ASSERT_PARAM(ioIndex < WAKEUP_IO_MAX);

	switch (src) {
	case PRCM_WAKEUP_IOX_DEB_CLK0_SRC:
		HAL_SET_BIT(PRCM->RTC_IO_WAKE_DEB_CLK, 1 << ioIndex);
		break;
	case PRCM_WAKEUP_IOX_DEB_CLK1_SRC:
		HAL_CLR_BIT(PRCM->RTC_IO_WAKE_DEB_CLK, 1 << ioIndex);
		break;
	default:
		break;
	}
}

void HAL_PRCM_SetWakeupDebClk0(uint8_t val)
{
	HAL_MODIFY_REG(PRCM->RTC_IO_WAKE_DEB_CLK, PRCM_WKAEUP_DEB_CLK0_MASK, val << PRCM_WKAEUP_DEB_CLK0_SHIFT);
}

void HAL_PRCM_SetWakeupDebClk1(uint8_t val)
{
	HAL_MODIFY_REG(PRCM->RTC_IO_WAKE_DEB_CLK, PRCM_WKAEUP_DEB_CLK1_MASK, val << PRCM_WKAEUP_DEB_CLK1_SHIFT);
}

void HAL_PRCM_SetWakeupIOxDebounce(PRCM_WakeIODebCyclesMode wakeio_cycle, uint8_t ioIndex, uint8_t val)
{
	HAL_ASSERT_PARAM(ioIndex < WAKEUP_IO_MAX);

	if (!wakeio_cycle) {
		HAL_MODIFY_REG(PRCM->RTC_IO_WUP_DEB_CYCLES0,
		               PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_L_MASK(ioIndex),
		               val << PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_L_SHIFT(ioIndex));
	} else {
		HAL_MODIFY_REG(PRCM->RTC_IO_WUP_DEB_CYCLES1,
		               PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_H_MASK(ioIndex),
		               val << PRCM_WAKEUP_IO0T7_DEDOUNCE_CYCLE_H_SHIFT(ioIndex));
	}
}


PRCM_WakeIO HAL_PRCM_WakeupIOxIndex2Mask(uint8_t ioIndex)
{
	HAL_ASSERT_PARAM(ioIndex < WAKEUP_IO_MAX);

	return 1 << ioIndex;
}

uint8_t HAL_PRCM_WakeupIOxMask2Index(PRCM_WakeIO ioMask)
{
	uint8_t ioIndex;
	switch (ioMask) {
	case HAL_BIT(7):
		ioIndex = 7;
		break;
	case HAL_BIT(6):
		ioIndex = 6;
		break;
	case HAL_BIT(5):
		ioIndex = 5;
		break;
	case HAL_BIT(4):
		ioIndex = 4;
		break;
	case HAL_BIT(3):
		ioIndex = 3;
		break;
	case HAL_BIT(2):
		ioIndex = 2;
		break;
	case HAL_BIT(1):
		ioIndex = 1;
		break;
	case HAL_BIT(0):
		ioIndex = 0;
		break;
	default:
		/* Force Halt, because it is inVaild. */
		HAL_ASSERT_PARAM(0);
		break;
	}
	return ioIndex;
}

/*
 * bit field definition of PRCM->WLAN_HIF_OV_CTRL 0x18c
 */

void HAL_PRCM_EnableWlanCPUClk(uint8_t enable)
{
	if (enable)
		HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_DISABLE_CPU_CLK_BIT); /* 0 is enable */
	else
		HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_DISABLE_CPU_CLK_BIT);
}

void HAL_PRCM_ReleaseWlanCPUReset(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_RESET_CPU_BIT);
}

void HAL_PRCM_ForceWlanCPUReset(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_RESET_CPU_BIT);
}

void HAL_PRCM_WakeUpWlan(uint8_t wakeup)
{
	if (wakeup)
		HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_WUP_BIT);
	else
		HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_WUP_BIT);
}

void HAL_PRCM_EnableWlanCPUClkOvrHIF(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_DISABLE_CPU_CLK_OVR_HIF_BIT);
}

void HAL_PRCM_DisableWlanCPUClkOvrHIF(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_DISABLE_CPU_CLK_OVR_HIF_BIT);
}

void HAL_PRCM_ReleaseWlanCPUOvrHIF(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_RESET_CPU_OVR_HIF_BIT);
}

void HAL_PRCM_ResetWlanCPUOvrHIF(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_RESET_CPU_OVR_HIF_BIT);
}

void HAL_PRCM_EnableWlanWUPOvrHIF(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_WUP_OVR_HIF_BIT);
}

void HAL_PRCM_DisableWlanWUPOvrHIF(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_WUP_OVR_HIF_BIT);
}

void HAL_PRCM_EnableWlanIRQOvrHIF(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_IRQ_OVR_HIF_BIT);
}

void HAL_PRCM_DisableWlanIRQOvrHIF(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_IRQ_OVR_HIF_BIT);
}

/*
 * bit field definition of PRCM->BOOT_FLAG 0x1c0
 * bit field definition of PRCM->BOOT_ADDR 0x1c4
 * bit field definition of PRCM->BOOT_ARG  0x1c8
 */

void HAL_PRCM_SetCPUBootFlag(PRCM_CPUTypeIndex cpu, PRCM_CPUBootFlag flag)
{
	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		PRCM->BOOT_FLAG = PRCM_CPU_BOOT_FLAG_WR_LOCK | flag;
		break;
	default:
		break;
	}
}

uint32_t HAL_PRCM_GetCPUBootFlag(PRCM_CPUTypeIndex cpu)
{
	volatile uint32_t temp = 0xFFFFFFFF;

	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		temp = HAL_GET_BIT(PRCM->BOOT_FLAG, PRCM_CPU_BOOT_FLAG_MASK);
		break;
	default:
		break;
	}

	return temp;
}

void HAL_PRCM_SetCPUBootAddr(PRCM_CPUTypeIndex cpu, uint32_t addr)
{
	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		PRCM->BOOT_ADDR = addr;
		break;
	default:
		break;
	}
}

uint32_t HAL_PRCM_GetCPUBootAddr(PRCM_CPUTypeIndex cpu)
{
	volatile uint32_t temp = 0xFFFFFFFF;

	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		temp =  PRCM->BOOT_ADDR;
		break;
	default:
		break;
	}

	return temp;
}

void HAL_PRCM_SetCPUBootArg(PRCM_CPUTypeIndex cpu, uint32_t arg)
{
	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		PRCM->BOOT_ARG = arg;
		break;
	default:
		break;
	}
}

uint32_t HAL_PRCM_GetCPUBootArg(PRCM_CPUTypeIndex cpu)
{
	volatile uint32_t temp = 0xFFFFFFFF;

	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		temp =  PRCM->BOOT_ARG;
		break;
	default:
		break;
	}

	return temp;
}

/*
 * bit field definition of PRCM->RESET_SRC_STATUS, 0x01d4
 */

uint8_t HAL_PRCM_ResetSrcStatusISPwronRst(void)
{
	return HAL_GET_BIT_VAL(PRCM->RESET_SRC_STATUS, PRCM_IS_PWRON_RST_SHIFT, PRCM_CPU_RST_STATUS_IS_YES);
}

uint8_t HAL_PRCM_ResetSrcStatusISDetRst(void)
{
	return HAL_GET_BIT_VAL(PRCM->RESET_SRC_STATUS, PRCM_IS_DET_RST_SHIFT, PRCM_CPU_RST_STATUS_IS_YES);
}

uint8_t HAL_PRCM_ResetSrcStatusISWdgRst(void)
{
	return HAL_GET_BIT_VAL(PRCM->RESET_SRC_STATUS, PRCM_IS_RTC_WDG_RST_SHIFT, PRCM_CPU_RST_STATUS_IS_YES);
}

void HAL_PRCM_ClrCpuRstSrc(void)
{
    HAL_SET_BIT(PRCM->RESET_SRC_STATUS, PRCM_CPU_RST_CLR_MASK);
}

/*
 * bit field definition of PRCM->SYSTEM_PRIV_REG0T7, 0x200~0x21c
 */

void HAL_PRCM_SetSystemPrivateData(uint32_t id, uint32_t data)
{
	if (id < PRCM_SYSTEM_PRIV_DATA_ID_NUM) {
		PRCM->SYSTEM_PRIV_REG0T7[id] = data;
	}
}

uint32_t HAL_PRCM_GetSystemPrivateData(uint32_t id)
{
	if (id < PRCM_SYSTEM_PRIV_DATA_ID_NUM) {
		return PRCM->SYSTEM_PRIV_REG0T7[id];
	} else {
		return 0;
	}
}
