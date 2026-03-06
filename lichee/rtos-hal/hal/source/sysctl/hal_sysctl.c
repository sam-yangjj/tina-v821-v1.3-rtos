
/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.


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
#include <stdint.h>
#include <hal_log.h>
#include <hal_time.h>
#include <compiler.h>
#include <hal_atomic.h>
#include "irqs.h"
#include <hal_interrupt.h>
#include <sunxi_hal_common.h>
#include <sunxi_hal_sysctl.h>
/*
 * Bitwise operation
 */
#define HAL_BIT(pos)                        (1U << (pos))

#define HAL_SET_BIT(reg, mask)              ((reg) |= (mask))
#define HAL_CLR_BIT(reg, mask)              ((reg) &= ~(mask))
#define HAL_GET_BIT(reg, mask)              ((reg) & (mask))
#define HAL_GET_BIT_VAL(reg, shift, vmask)  (((reg) >> (shift)) & (vmask))
#define HAL_SET_BIT_VAL(reg, shift, vmask, val) \
    ((reg) = ((reg) & (~((vmask) << (shift)))) | (((val) & (vmask)) << (shift)))

#define HAL_MODIFY_REG(reg, clr_mask, set_mask) \
    ((reg) = (((reg) & (~(clr_mask))) | (set_mask)))

#define HAL_ASSERT_PARAM(exp)                                           \
    do {                                                                \
        if (!(exp)) {                                                   \
            hal_log_info("Invalid param at %s:%d\n", __func__, __LINE__); \
        }                                                               \
    } while (0)

#ifdef CONFIG_ARCH_SUN20IW2P1
void HAL_SYSCTL_SetSipFlashTestMapMode(uint32_t en)
{
    if (en)
        HAL_SET_BIT(SYSCTL->SIP_TEST_MAP, SYSCTL_SIP_TEST_MAP_WRITE_EN | SYSCTL_SIP_FLASH_TEST_MAP_EN);
    else
        SYSCTL->SIP_TEST_MAP = SYSCTL_SIP_TEST_MAP_WRITE_EN;
}

void HAL_SYSCTL_RamUseAsBT(uint32_t en)
{
    if (en)
        HAL_SET_BIT(SYSCTL->RAM_REMAP_REG, SYSCTL_BT_RAM_REMAP_MASK);
    else
        HAL_CLR_BIT(SYSCTL->RAM_REMAP_REG, SYSCTL_BT_RAM_REMAP_MASK);
}

void HAL_SYSCTL_RamUseAsCSI(uint32_t en)
{
    if (en)
        HAL_SET_BIT(SYSCTL->RAM_REMAP_REG, SYSCTL_CSI_RAM_REMAP_MASK);
    else
        HAL_CLR_BIT(SYSCTL->RAM_REMAP_REG, SYSCTL_CSI_RAM_REMAP_MASK);
}

void HAL_SYSCTL_RomSecureSel(uint32_t en)
{
    if (en)
        HAL_SET_BIT(SYSCTL->RAM_REMAP_REG, SYSCTL_ROM_SECURE_SEL_MASK);
    else
        HAL_CLR_BIT(SYSCTL->RAM_REMAP_REG, SYSCTL_ROM_SECURE_SEL_MASK);
}

void HAL_SYSCTL_SetPsensorControl(uint32_t PsensorId, uint32_t OSCSelect,
		uint32_t ps_n, uint32_t en)
{
    SYSCTL_PsensorId id;
    SYSCTL_OSCSelect osc_sel;
    switch (PsensorId) {
		case 0:
			id = SYSCTL_PSENSOR_0;
			break;
		case 1:
			id = SYSCTL_PSENSOR_1;
			break;
		default:
			HAL_ASSERT_PARAM(PsensorId < 2);
			return;
    }
    switch (OSCSelect) {
		case 0:
			osc_sel = SYSCTL_OSC_DISABLE;
			break;
		case 1:
			osc_sel = SYSCTL_OSC_RVT_CASECODE;
			break;
		case 2:
			osc_sel = SYSCTL_OSC_LVT_CASECODE;
			break;
		case 3:
			osc_sel = SYSCTL_OSC_RVT_NORMAL;
			break;
		default:
			HAL_ASSERT_PARAM(OSCSelect < 4);
			return;
    }
    HAL_ASSERT_PARAM(ps_n < 8);

    if (en)
        SYSCTL->PS_CTL_REG = id | osc_sel | SYSCTL_PS_N_PRD_VAL(ps_n) | SYSCTL_PS_EN_MASK;
    else
        SYSCTL->PS_CTL_REG = id | osc_sel | SYSCTL_PS_N_PRD_VAL(ps_n);
}

void HAL_SYSCTL_WaitPsensorRdyAndClean(void)
{
    while (!(SYSCTL->PS_CTL_REG & SYSCTL_CLK250M_CNT_RDY_MASK))
        ;
    HAL_SET_BIT(SYSCTL->PS_CTL_REG, SYSCTL_CLK250M_CNT_RDY_MASK);
}

uint32_t HAL_SYSCTL_GetPsensorCnt(void)
{
    return SYSCTL->PS_CNT_REG;
}

uint32_t HAL_SYSCTL_GetPsensor(uint32_t PsensorId, uint32_t OSCSelect,
		uint32_t ps_n)
{
    uint32_t value;
    HAL_SYSCTL_SetPsensorControl(PsensorId, OSCSelect, ps_n, 1);

    hal_usleep(1);
    HAL_SYSCTL_WaitPsensorRdyAndClean();
    HAL_SET_BIT(SYSCTL->PS_CTL_REG, SYSCTL_CLK250M_CNT_RDY_MASK); // clear ready bit

    value = HAL_SYSCTL_GetPsensorCnt();
    HAL_CLR_BIT(SYSCTL->PS_CTL_REG, SYSCTL_PS_EN_MASK); // disable psensor

    return value;
}
#endif

void HAL_SYSCTL_SetDbgData(uint32_t id, uint32_t data)
{
#ifdef CONFIG_ARCH_SUN20IW2P1
    HAL_ASSERT_PARAM(id < 6);
#endif
#ifdef CONFIG_ARCH_SUN300IW1P1
    HAL_ASSERT_PARAM(id < 2);
#endif

    SYSCTL->GENRAL_DBG_REG[id] = data;
}

uint32_t HAL_SYSCTL_GetDegData(uint32_t id)
{
#ifdef CONFIG_ARCH_SUN20IW2P1
    HAL_ASSERT_PARAM(id < 6);
#endif
#ifdef CONFIG_ARCH_SUN300IW1P1
    HAL_ASSERT_PARAM(id < 2);
#endif

    return SYSCTL->GENRAL_DBG_REG[id];
}

#ifdef CONFIG_ARCH_SUN300IW1P1
typedef struct {
	SYSCTRL_BusCtrlBitPos      ctrl_bit_pos;
	SYSCTRL_BusInterruptBitPos int_bit_pos;
} SYSCTRL_BusTmTablePriv;

typedef struct {
	SYSCTRL_BusTmIRQCallback   callback;
	void                       *arg;
} SYSCTRL_BusTmPrivate;

hal_spinlock_t sysctl_lock;

static const SYSCTRL_BusTmTablePriv gSysctrlBusTmConfigTable[SYSCTRL_BUS_NUM] = {
	{SYSCTRL_BUS_CTRL_BIT_POS_SH0, SYSCTRL_BUS_INT_BIT_POS_SH0},
	{SYSCTRL_BUS_CTRL_BIT_POS_SH2, SYSCTRL_BUS_INT_BIT_POS_SH2},
	{SYSCTRL_BUS_CTRL_BIT_POS_SH3, SYSCTRL_BUS_INT_BIT_POS_SH3},
	{SYSCTRL_BUS_CTRL_BIT_POS_SH4, SYSCTRL_BUS_INT_BIT_POS_SH4},
	{SYSCTRL_BUS_CTRL_BIT_POS_SP0, SYSCTRL_BUS_INT_BIT_POS_SP0},
	{SYSCTRL_BUS_CTRL_BIT_POS_SP1, SYSCTRL_BUS_INT_BIT_POS_SP1},
};

static SYSCTRL_BusTmPrivate gSysctrlBusTmPrivate[SYSCTRL_BUS_NUM];

static void SYSCTRL_BusTimeout_EnableIRQ(SYSCTRL_BusType bus_type)
{
	HAL_SET_BIT(SYSCTL->BUS_RDY_TIMEOUT_INT_EN_REG, HAL_BIT(gSysctrlBusTmConfigTable[bus_type].int_bit_pos));
}

static void SYSCTRL_BusTimeout_DisableIRQ(SYSCTRL_BusType bus_type)
{
	HAL_CLR_BIT(SYSCTL->BUS_RDY_TIMEOUT_INT_EN_REG, HAL_BIT(gSysctrlBusTmConfigTable[bus_type].int_bit_pos));
}

__nonxip_text
static __inline uint8_t SYSCTRL_BusTimeout_IsEnableIRQ(SYSCTRL_BusType bus_type)
{
	return HAL_GET_BIT(SYSCTL->BUS_RDY_TIMEOUT_INT_EN_REG, HAL_BIT(gSysctrlBusTmConfigTable[bus_type].int_bit_pos));
}

__nonxip_text
static __inline uint8_t SYSCTRL_BusTimeout_IsPendingIRQ(SYSCTRL_BusType bus_type)
{
	return HAL_GET_BIT(SYSCTL->BUS_RDY_TIMEOUT_INT_STA_REG, HAL_BIT(gSysctrlBusTmConfigTable[bus_type].int_bit_pos));
}

__nonxip_text
static __inline void SYSCTRL_BusTimeout_ClearPendingIRQ(SYSCTRL_BusType bus_type)
{
	HAL_SET_BIT(SYSCTL->BUS_RDY_TIMEOUT_INT_STA_REG, HAL_BIT(gSysctrlBusTmConfigTable[bus_type].int_bit_pos));
}

__nonxip_text
hal_irqreturn_t SYSCTRL_BusTimeout_IRQHandler(void *data)
{
	uint32_t i;
	SYSCTRL_BusTmPrivate *priv;

	for (i = SYSCTRL_BUS_SH0; i < SYSCTRL_BUS_NUM; i++) {
		if (SYSCTRL_BusTimeout_IsPendingIRQ(SYSCTRL_BUS_SH0)) {
			SYSCTRL_BusTimeout_ClearPendingIRQ(SYSCTRL_BUS_SH0);
			priv = &gSysctrlBusTmPrivate[i];
			if (SYSCTRL_BusTimeout_IsEnableIRQ(i) && priv->callback) {
				priv->callback(priv->arg);
			}
		}
	}

    return HAL_IRQ_OK;
}

HAL_Status HAL_SYSCTRL_BusTimeout_ConfigParam(SYSCTRL_BusType bus_type, SYSCTRL_BusTmParam *param)
{
	uint32_t reg_idx, reg_offset;

	if (bus_type >= SYSCTRL_BUS_NUM || bus_type < SYSCTRL_BUS_SH0) {
		printf("[SYSCTRL]invalid bus type(%d)\n", bus_type);
		return HAL_INVALID;
	}
	reg_idx = gSysctrlBusTmConfigTable[bus_type].ctrl_bit_pos / 32;
	reg_offset = gSysctrlBusTmConfigTable[bus_type].ctrl_bit_pos % 32;

	if (param->interval_sel == SYSCTRL_BUS_TIMEOUT_INTERVAL0) {
		HAL_CLR_BIT(SYSCTL->BUS_RDY_TIMEOUT_CTRL_REG[reg_idx], HAL_BIT(reg_offset + SYSCTRL_BUS_TIMEOUT_INT_SEL_OFFSET));
	} else if (param->interval_sel == SYSCTRL_BUS_TIMEOUT_INTERVAL1) {
		HAL_SET_BIT(SYSCTL->BUS_RDY_TIMEOUT_CTRL_REG[reg_idx], HAL_BIT(reg_offset + SYSCTRL_BUS_TIMEOUT_INT_SEL_OFFSET));
	}
	HAL_SET_BIT_VAL(SYSCTL->BUS_RDY_TIMEOUT_CTRL_REG[reg_idx], reg_offset + SYSCTRL_BUS_TIMEOUT_INT_CNT_OFFSET, SYSCTRL_BUS_TIMEOUT_INT_CNT_VMASK, param->interval_cnt);

	return HAL_OK;
}

HAL_Status HAL_SYSCTRL_BusTimeout_Enable(SYSCTRL_BusType bus_type, uint8_t en)
{
	uint32_t reg_idx, reg_offset;

	if (bus_type >= SYSCTRL_BUS_NUM || bus_type < SYSCTRL_BUS_SH0) {
		printf("[SYSCTRL]invalid bus type(%d)\n", bus_type);
		return HAL_INVALID;
	}
	reg_idx = gSysctrlBusTmConfigTable[bus_type].ctrl_bit_pos / 32;
	reg_offset = gSysctrlBusTmConfigTable[bus_type].ctrl_bit_pos % 32;

	if (en) {
		HAL_SET_BIT(SYSCTL->BUS_RDY_TIMEOUT_CTRL_REG[reg_idx], HAL_BIT(reg_offset + SYSCTRL_BUS_TIMEOUT_EN_OFFSET));
	} else {
		HAL_CLR_BIT(SYSCTL->BUS_RDY_TIMEOUT_CTRL_REG[reg_idx], HAL_BIT(reg_offset + SYSCTRL_BUS_TIMEOUT_EN_OFFSET));
	}

	return HAL_OK;
}

HAL_Status HAL_SYSCTRL_BusTimeout_EnableIRQ(SYSCTRL_BusType bus_type, SYSCTRL_BusTmIRQCallback cb, void *arg)
{
	SYSCTRL_BusTmPrivate *priv;
	unsigned long flags;

	if (bus_type >= SYSCTRL_BUS_NUM || bus_type < SYSCTRL_BUS_SH0) {
		printf("[SYSCTRL]invalid bus type(%d)\n", bus_type);
		return HAL_INVALID;
	}
	/* register callback */
	flags = hal_spin_lock_irqsave(&sysctl_lock);
	priv = &gSysctrlBusTmPrivate[bus_type];
	priv->callback = cb;
	priv->arg = arg;
	hal_spin_unlock_irqrestore(&sysctl_lock, flags);

	/* clear irq pending and enable irq */
	if (SYSCTRL_BusTimeout_IsPendingIRQ(bus_type)) {
		SYSCTRL_BusTimeout_ClearPendingIRQ(bus_type);
	}

    if (hal_request_irq(MAKE_IRQn(HRDY_TOUT_IRQn, 0), SYSCTRL_BusTimeout_IRQHandler, "HRDY_TOUT_IRQ", NULL) < 0) {
			printf("[SYSCTRL]request irq error\n");
			return HAL_ERROR;
	}
	hal_enable_irq(MAKE_IRQn(HRDY_TOUT_IRQn, 0));
    SYSCTRL_BusTimeout_EnableIRQ(bus_type);

	return HAL_OK;
}

HAL_Status HAL_SYSCTRL_BusTimeout_DisableIRQ(SYSCTRL_BusType bus_type)
{
	SYSCTRL_BusTmPrivate *priv;
	unsigned long flags;

	if (bus_type >= SYSCTRL_BUS_NUM || bus_type < SYSCTRL_BUS_SH0) {
		printf("[SYSCTRL]invalid bus type(%d)\n", bus_type);
		return HAL_INVALID;
	}
	/* unregister callback */
    flags = hal_spin_lock_irqsave(&sysctl_lock);
	priv = &gSysctrlBusTmPrivate[bus_type];
	priv->callback = NULL;
	priv->arg = NULL;
	hal_spin_unlock_irqrestore(&sysctl_lock, flags);

	/* disable irq and clear irq pending */
	SYSCTRL_BusTimeout_DisableIRQ(bus_type);
	hal_disable_irq(MAKE_IRQn(HRDY_TOUT_IRQn, 0));
	if (SYSCTRL_BusTimeout_IsPendingIRQ(bus_type)) {
		SYSCTRL_BusTimeout_ClearPendingIRQ(bus_type);
	}

	return HAL_OK;
}

void HAL_SYSCTRL_DebugIO_ConfigClkOutput(SYSCTRL_DebugIOClkSel clk_sel, SYSCTRL_DebugIOClkDiv clk_div)
{
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[0], SYSCTRL_DBG_CLK_DIV_SHIFT, SYSCTRL_DBG_CLK_DIV_VMASK, clk_div);
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[0], SYSCTRL_DBG_CLK_GRP_SEL_SHIFT, SYSCTRL_DBG_CLK_GRP_SEL_VMASK, clk_sel);
}

void HAL_SYSCTRL_DebugIO_ConfigSignalOutput(SYSCTRL_DebugIOSignalMaskParam *param)
{
	uint32_t spif_mask_val1, spif_mask_val2;

	spif_mask_val1 = param->spif_mask & SYSCTRL_DBGSIGNAL_SPIF_VMASK1;
	spif_mask_val2 = param->spif_mask / (SYSCTRL_DBGSIGNAL_SPIF_VMASK1 + 1);
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[2], SYSCTRL_DBGSIGNAL_VE_SHIFT, SYSCTRL_DBGSIGNAL_VE_VMASK, param->ve_mask);
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[3], SYSCTRL_DBGSIGNAL_USB0_SHIFT, SYSCTRL_DBGSIGNAL_USB0_VMASK, param->usb0_mask);
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[3], SYSCTRL_DBGSIGNAL_USB1_SHIFT, SYSCTRL_DBGSIGNAL_USB1_VMASK, param->usb1_mask);
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[4], SYSCTRL_DBGSIGNAL_ADIE_SHIFT, SYSCTRL_DBGSIGNAL_ADIE_VMASK, param->adie_mask);
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[4], SYSCTRL_DBGSIGNAL_WLAN_SHIFT, SYSCTRL_DBGSIGNAL_WLAN_VMASK, param->wlan_mask);
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[4], SYSCTRL_DBGSIGNAL_PLL_LOCK_SHIFT, SYSCTRL_DBGSIGNAL_PLL_LOCK_VMASK, param->pll_lock_mask);
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[4], SYSCTRL_DBGSIGNAL_SPIF_SHIFT1, SYSCTRL_DBGSIGNAL_SPIF_VMASK1, spif_mask_val1);
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[5], SYSCTRL_DBGSIGNAL_SPIF_SHIFT2, SYSCTRL_DBGSIGNAL_SPIF_VMASK2, spif_mask_val2);
}

void HAL_SYSCTRL_DCU_SetDAP(SYSCTRL_DAPSel dap_sel)
{
	HAL_SET_BIT_VAL(SYSCTL->TEST_DBG_REG[0], SYSCTRL_DAP_SEL_SHIFT, SYSCTRL_DAP_SEL_VMASK, dap_sel);
}

void HAL_SYSCTRL_SramRemap_EnableBootMode(uint8_t en)
{
	if (en) {
		HAL_SET_BIT(SYSCTL->SRAM_CTRL_REG, SYSCTRL_SRAM_BOOT_MODE_EN_BIT);
	} else {
		HAL_CLR_BIT(SYSCTL->SRAM_CTRL_REG, SYSCTRL_SRAM_BOOT_MODE_EN_BIT);
	}
}

void HAL_SYSCTRL_SramRemap_SetLaMode(SYSCTRL_LaModeSel la_sel)
{
	HAL_SET_BIT_VAL(SYSCTL->SRAM_CTRL_REG, SYSCTRL_SRAM_LA_MODE_SEL_SHIFT,
                    SYSCTRL_SRAM_LA_MODE_SEL_VMASK, la_sel);
}
#endif