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
 *  \brief      UART driver definition.
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

#include "pal_irq.h"
#include "hal_interrupt.h"
#include "irqs-sun300iw1.h"
#include "hal_status.h"
#include "errno.h"

static int32_t PalGetIrq(pal_irq_type_t type)
{
	int32_t irq = -EINVAL;

	switch (type) {
	case PAL_IRQ_TYPE_LL:
		irq = MAKE_IRQn(BLE_LL_IRQn, 0);;
		break;
	case PAL_IRQ_TYPE_BB:
		irq = MAKE_IRQn(BTC_BB_IRQn, 0);;
		break;
	case PAL_IRQ_TYPE_WLANCOEX:
		irq = MAKE_IRQn(BTCOEX_IRQn, 0);;
		break;
	case PAL_IRQ_TYPE_DBG:
		irq = MAKE_IRQn(BTC_DBG_IRQn, 0);;
		break;
	case PAL_IRQ_TYPE_SLPTMR:
		irq = MAKE_IRQn(BTC_SLPTMR_IRQn, 0);;
		break;
	default:
		break;
	}
	return irq;
}

int32_t PalIrqRegister(pal_irq_type_t type, PalIrqHandler_t handler)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return -EINVAL;
	}
	hal_request_irq(irq, (hal_irq_handler_t)handler, NULL, NULL);
	//hal_nvic_irq_set_priority(irq, 4);
	hal_interrupt_clear_pending(irq);
	hal_enable_irq(irq);
	return 0;
}

int32_t PalIrqUnregister(pal_irq_type_t type)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return -EINVAL;
	}
	hal_disable_irq(irq);
	hal_interrupt_clear_pending(irq);
	return 0;
}

void PalIrqClearPending(pal_irq_type_t type)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return ;
	}
	hal_interrupt_clear_pending(irq);
}

uint32_t PalIrqGetNest(void)
{
	return hal_interrupt_get_nest();
}

void PalIrqDisable(pal_irq_type_t type)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return ;
	}
	hal_disable_irq(irq);
}

int32_t PalIrqEnable(pal_irq_type_t type)
{
	int32_t irq;

	irq = PalGetIrq(type);
	if (irq == -EINVAL) {
		return -EINVAL;
	}
	if (hal_enable_irq(irq) != HAL_OK) {
		return -EPERM;
	}
	return 0;
}

