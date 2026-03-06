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
 *  \brief      reset driver .
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

#include "pal_reset.h"
#include "hal_clk.h"
#include "hal_reset.h"
/************************************************************************************************
* @Function: pal_reset_force_btcore
* @Description: force BT core, signal hrestn is 0
*               set 0, assert as mean as Force
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/
void pal_reset_force_btcore(void)
{
	hal_rst_t reset;
	hal_rst_get(AW_APP_CCU, RST_APP_BUS_BT, &reset);
	hal_rst_assert(reset);
	hal_rst_put(reset);
}

/************************************************************************************************
* @Function: pal_reset_release_btcore
* @Description: release BT core, signal hrestn is 1
*               set 1,deassert as mean as release
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/
void pal_reset_release_btcore(void)
{
	hal_rst_t reset;
	hal_rst_get(AW_APP_CCU, RST_APP_BUS_BT, &reset);
	hal_rst_deassert(reset);
	hal_rst_put(reset);
}
/************************************************************************************************
* @Function: pal_reset_force_bt_rtc
* @Description: force the global reset of the BLE RTC
*               set 0,deassert as mean as release
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/

void pal_reset_force_bt_rtc(void)
{
	hal_rst_t reset;
	hal_rst_get(AW_APP_CCU, RST_APP_BUS_BT_RTC, &reset);
	hal_rst_assert(reset);
	hal_rst_put(reset);
}
/************************************************************************************************
* @Function: pal_reset_release_bt_rtc
* @Description: release the global reset of the BLE RTC
*               set 1,deassert as mean as release
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/

void pal_reset_release_bt_rtc(void)
{
	hal_rst_t reset;
	hal_rst_get(AW_APP_CCU, RST_APP_BUS_BT_RTC, &reset);
	hal_rst_deassert(reset);
	hal_rst_put(reset);
}
