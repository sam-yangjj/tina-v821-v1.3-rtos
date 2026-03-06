/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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
#include "platform_irq.h"

#include "../irq_core.h"
#include "../aw_intc.h"
#include "../clic.h"

static irq_controller_t g_plat_ic_arr[PLAT_IRQ_CONTROLLER_CNT];

static irq_handler_desc_t g_root_ic_handler_desc[PLAT_ROOT_IC_IRQ_CNT];

#ifdef PLAT_HAS_NON_ROOT_IC
static irq_handler_desc_t g_non_root_ic_handler_desc[PLAT_NON_ROOT_IRQ_CONTROLLER_CNT][PLAT_NON_ROOT_IC_IRQ_CNT];
#endif

#if defined(CONFIG_COMPONENTS_PM)
static uint32_t g_root_ic_status[PLAT_ROOT_IC_IRQ_CNT];

#ifdef PLAT_HAS_NON_ROOT_IC
static uint32_t g_non_root_ic_status[PLAT_NON_ROOT_IRQ_CONTROLLER_CNT];
#endif
#endif

int plat_irq_init(void);
irq_controller_t *plat_get_irq_controller(unsigned int ic_id);
irq_handler_desc_t *plat_get_irq_handler(uint32_t ic_id, uint32_t irq_id);

__attribute__((weak)) int plat_irq_init(void)
{
	uint32_t i;
	irq_controller_t *ic;

	for (i = 0; i < PLAT_IRQ_CONTROLLER_CNT; i++)
	{
		ic = &g_plat_ic_arr[i];
		ic->id = i;
	}

	ic = &g_plat_ic_arr[ROOT_IRQ_CONTROLLER_ID];
	ic->reg_base_addr = PLAT_ROOT_IC_REG_BASE_ADDR;
	ic->irq_cnt = PLAT_ROOT_IC_IRQ_CNT;
	ic->parent_id = 0;
	ic->irq_id = 0;
	ic->ops = &g_rv_clic_ops;
#if defined(CONFIG_COMPONENTS_PM)
	ic->priv = &g_root_ic_status;
#endif

#ifdef PLAT_HAS_NON_ROOT_IC
	for (i = 1; i < PLAT_IRQ_CONTROLLER_CNT; i++)
	{
		ic = &g_plat_ic_arr[i];
		ic->reg_base_addr = PLAT_NON_ROOT_IC_REG_BASE_ADDR + i - 1;
		ic->irq_cnt = PLAT_NON_ROOT_IC_IRQ_CNT;
		ic->parent_id = ROOT_IRQ_CONTROLLER_ID;
		ic->irq_id = PLAT_NON_ROOT_IC_START_IRQ_ID + i - 1;
		ic->ops = &PLAT_NON_ROOT_IC_OPS;
#if defined(CONFIG_COMPONENTS_PM)
		ic->priv = &g_non_root_ic_status[i - 1];
#endif
	}
#else
	if (PLAT_IRQ_CONTROLLER_CNT != 1)
		return -IRQ_RET_CODE_INVALID_PLAT_IC_CNT;
#endif

	return 0;
}

static inline irq_controller_t *__plat_get_irq_controller(unsigned int ic_id)
{
	return &g_plat_ic_arr[ic_id];
}

__attribute__((weak)) irq_controller_t *plat_get_irq_controller(unsigned int ic_id)
{
	if (ic_id >= PLAT_IRQ_CONTROLLER_CNT)
		return NULL;

	return &g_plat_ic_arr[ic_id];
}

__attribute__((weak)) irq_handler_desc_t *plat_get_irq_handler(uint32_t ic_id, uint32_t irq_id)
{
	if (ic_id >= PLAT_IRQ_CONTROLLER_CNT)
		return NULL;

	irq_controller_t *ic = __plat_get_irq_controller(ic_id);
	if (ic_id == ROOT_IRQ_CONTROLLER_ID)
	{
		if (irq_id >= ic->irq_cnt)
			return NULL;

		return &g_root_ic_handler_desc[irq_id];
	}
	else
	{
#ifdef PLAT_HAS_NON_ROOT_IC
		if (irq_id >= ic->irq_cnt)
			return NULL;

		return &g_non_root_ic_handler_desc[ic->id - 1][irq_id];
#else
		return NULL;
#endif
	}

	return NULL;
}

const platform_irq_info_t g_plat_irq_info =
{
	.init = plat_irq_init,
	.get_irq_controller = plat_get_irq_controller,
	.get_irq_handler = plat_get_irq_handler,
};

#if defined(CONFIG_COMPONENTS_PM)
#include "../clint.h"

int platform_irq_pm_init(void)
{
	int ret;

	ret = clint_pm_init();
	if (ret)
		return ret;

	ret = clic_pm_init(&g_plat_ic_arr[ROOT_IRQ_CONTROLLER_ID]);
	if (ret)
		return ret;

#ifdef PLAT_HAS_NON_ROOT_IC
	int i;
	for (i = 1; i < PLAT_IRQ_CONTROLLER_CNT; i++)
	{
		ret = aw_intc_pm_init(&g_plat_ic_arr[i]);
		if (ret)
			return ret;
	}
#endif

	return 0;
}

/* Provide legacy functions to avoid compilation error */
void irq_suspend(void)
{

}

void irq_resume(void)
{

}


void clic_suspend(void)
{

}

void clic_resume(void)
{

}
#endif

