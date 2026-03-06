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
/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      BT clk driver.
 *
 *  Copyright (c) 2018-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/
#include "hal_clk.h"
#include "pal_clk.h"
#include "hal_reset.h"
#include "platform_prcm.h"

#define CCU_DCXO_CNT_REG                   (0x4a010510)
#define CCU_DIV32K_HALFCYCLE_TARGET_MASK   (0x000003ff)

void pal_clk_enable_ble_48m(void)
{
	hal_clk_t clk;
	hal_n_clk_get(AW_APP_CCU, CLK_APP_BUS_BT_CLK48M_GATE, &clk);
	hal_n_clk_enable(clk);
	hal_n_clk_put(clk);
}

void pal_clk_enable_ble_32m(void)
{
	hal_clk_t clk;
	hal_n_clk_get(AW_APP_CCU, CLK_APP_BUS_BT_CLK32M_GATE, &clk);
	hal_n_clk_enable(clk);
	hal_n_clk_put(clk);
}

void pal_clk_disable_ble_48m(void)
{
	hal_clk_t clk;
	hal_n_clk_get(AW_APP_CCU, CLK_APP_BUS_BT_CLK48M_GATE, &clk);
	hal_n_clk_disable(clk);
	hal_n_clk_put(clk);
}

void pal_clk_disable_ble_32m(void)
{
	hal_clk_t clk;
	hal_n_clk_get(AW_APP_CCU, CLK_APP_BUS_BT_CLK32M_GATE, &clk);
	hal_n_clk_disable(clk);
	hal_n_clk_put(clk);
}

void pal_clk_set_ble_sel_parent_losc_32k(void)
{
	hal_clk_t lf_sel_clk, ble_clk_32K_sel;

	hal_n_clk_get(AW_SRC_CCU, CLK_SRC_XO32K, &lf_sel_clk);
	hal_n_clk_get(AW_APP_CCU, CLK_APP_BT, &ble_clk_32K_sel);
	hal_n_clk_set_parent(ble_clk_32K_sel, lf_sel_clk);
	hal_n_clk_put(lf_sel_clk);
	hal_n_clk_put(ble_clk_32K_sel);
}

void pal_clk_set_ble_sel_parent_rccail_32k(void)
{
	hal_clk_t rccail32k, ble_clk_32K_sel;

	hal_n_clk_get(AW_SRC_CCU, CLK_SRC_RCCAL_32K, &rccail32k);
	hal_n_clk_get(AW_APP_CCU, CLK_APP_BT, &ble_clk_32K_sel);
	hal_n_clk_set_parent(rccail32k, ble_clk_32K_sel);
	hal_n_clk_put(rccail32k);
	hal_n_clk_put(ble_clk_32K_sel);
}

void pal_clk_enable_ble_div_32k(void)
{
	/* set the source clk of div_clk */
	hal_clk_t div_clk, div_clk_parent;
	uint32_t hosc_freq, div;

	hal_n_clk_get(AW_AON_CCU, CLK_AON_HOSC_DIV_32K, &div_clk_parent);
	hal_n_clk_get(AW_APP_CCU, CLK_APP_BT, &div_clk);
	hal_clk_set_parent(div_clk, div_clk_parent);
	//hal_n_clk_set_freq((hal_clk_t)CLK_AON_HOSC_DIV_32K, 32000);

	/* set div_clk to 32k */
	hal_n_clk_get_hosc_freq(&hosc_freq);
	div = hosc_freq / (32 * 1000) / 2 - 1;
	div &= CCU_DIV32K_HALFCYCLE_TARGET_MASK;
	HAL_MODIFY_REG(HAL_REG_32BIT(CCU_DCXO_CNT_REG), CCU_DIV32K_HALFCYCLE_TARGET_MASK, div);

	/* enable div clk */
	hal_n_clk_enable(div_clk);
	hal_n_clk_put(div_clk);
	hal_n_clk_put(div_clk_parent);
}

void pal_clk_disable_ble_div_32k(void)
{
	/* disable div clk */
	hal_clk_t div_clk;

	hal_n_clk_get(AW_AON_CCU, CLK_AON_HOSC_DIV_32K, &div_clk);
	hal_n_clk_disable(div_clk);
	hal_n_clk_put(div_clk);
}
