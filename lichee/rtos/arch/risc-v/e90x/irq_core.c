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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <irqflags.h>
#include <trace_event.h>

#include "irq_core.h"

#include "platform/platform_irq.h"

#include <hal_time.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#if (ROOT_IRQ_CONTROLLER_ID != 0)
#error "The ID of root interrupt controller must be zero!"
#endif

//#define IRQ_CORE_DEBUG_IC_IRQ_HANDLER

#define irq_core_debug(fmt,...)

/*
 * Currently the format of IRQ number(which is a identifier in software level) is not
 * suitable, Because it can only support at most 2 levels interrupt controller.
 * If we modify the format of IRQ number, we will need to modify many peripheral driver.
 * So we don't changed it at present.
 *
 * If the system has more than 2 levels interrupt controller in the future,
 * the IRQ number should be consisted of the interrupt controller id and
 * the interrupt id which the interrupt id is only valid with one interrupt controller.
 * Internally we are using this 2 field to identify the uniqued interrupt.
 */
#ifdef IRQ_CORE_LEGACY_IRQ_NUMBER_FORMAT
#define get_level1_irq_num(x) ((x) / 100)
#define get_level2_irq_num(x) ((x) % 100)
#define get_irq_num(_ic, _id) ((_ic) * 100 + (_id))
#else
#error "Please modify releated functions to support the new IRQ number format!"
#endif

typedef struct irq_desc
{
	uint16_t ic_id;
	uint16_t irq_id;
} irq_desc_t;


static int g_is_irq_core_init = 0;
static uint32_t g_max_ic_level = 1;

#ifdef IRQ_CORE_SUPPORT_RESERVED_SYS_IRQ
uint16_t g_reserved_sys_irq_id[IRQ_CORE_RESERVED_SYS_IRQ_CNT];
uint16_t g_reserved_irq_cnt = 0;
#endif

static int core_plat_irq_init(void)
{
	return g_plat_irq_info.init();
}

static const irq_controller_t *core_get_irq_controller(uint32_t ic_id)
{
	return g_plat_irq_info.get_irq_controller(ic_id);
}

static const irq_controller_t *core_get_root_irq_controller(void)
{
	return core_get_irq_controller(ROOT_IRQ_CONTROLLER_ID);
}

static irq_handler_desc_t *core_get_irq_handler(uint32_t ic_id, uint32_t irq_id)
{
	return g_plat_irq_info.get_irq_handler(ic_id, irq_id);
}
static int core_check_plat_irq_info(void)
{
	if ((!g_plat_irq_info.init)
			|| (!g_plat_irq_info.get_irq_controller)
			|| (!g_plat_irq_info.get_irq_handler))
		return -IRQ_RET_CORE_INVALID_PLAT_IRQ_INFO;

	return 0;
}

static uint32_t core_get_ic_level(uint16_t ic_id)
{
	const irq_controller_t *ic;
	uint32_t ic_level;

	if (ic_id == ROOT_IRQ_CONTROLLER_ID)
		return 1;

	ic = core_get_irq_controller(ic_id);

	ic_level = core_get_ic_level(ic->parent_id);

	return ic_level + 1;
}

static int core_check_ic_info(void)
{
	uint32_t i, j, root_ic_cnt = 0;
	const irq_controller_t *ic;

	uint16_t ic_id_arr[PLAT_IRQ_CONTROLLER_CNT];

	for (i = 0; i < PLAT_IRQ_CONTROLLER_CNT; i++)
	{
		ic = core_get_irq_controller(i);
		if (!ic)
			return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;

		if (ic->id == ROOT_IRQ_CONTROLLER_ID)
			root_ic_cnt++;

		if (!ic->irq_cnt)
			return -IRQ_RET_CODE_INVALID_IRQ_CNT;

		if (!ic->reg_base_addr)
			return -IRQ_RET_CODE_INVALID_REG_BASE_ADDR;

		if ((!ic->ops->init)
				|| (!ic->ops->irq_enable)
				|| (!ic->ops->irq_disable)
				|| (!ic->ops->irq_is_enabled))
			return -IRQ_RET_CODE_INVALID_IRQ_CONTROLLER_OPS;

		ic_id_arr[i] = ic->id;
	}

	if (root_ic_cnt != 1)
		return -IRQ_RET_CODE_INVALID_ROOT_CONTROLLER_CNT;

	for (i = 0; i < PLAT_IRQ_CONTROLLER_CNT; i++)
	{
		for (j = i + 1; j < PLAT_IRQ_CONTROLLER_CNT; j++)
		{
			if (ic_id_arr[i] == ic_id_arr[j])
				return -IRQ_RET_CODE_INVALID_IRQ_CONTROLLER_ID;
		}
	}

	return 0;
}

static int core_check_init(void)
{
	if (!g_is_irq_core_init)
		return -IRQ_RET_CODE_CORE_NOT_INIT;

	return 0;
}

#ifdef IRQ_CORE_SUPPORT_RESERVED_SYS_IRQ
static inline uint32_t core_get_reserved_sys_irq_cnt(void)
{
	return IRQ_CORE_RESERVED_SYS_IRQ_CNT;
}

static int core_is_reserved_system_irq(const irq_desc_t *desc)
{
	uint32_t i;

#ifdef IRQ_CORE_ONLY_ROOT_IC_HAS_SYS_IRQ
	if (desc->ic_id != ROOT_IRQ_CONTROLLER_ID)
		return 0;
#endif

	for (i = 0; i < g_reserved_irq_cnt; i++)
	{
		if (g_reserved_sys_irq_id[i] == desc->irq_id)
		{
			return 1;
		}
	}

	return 0;
}

static int core_reserve_system_irq(const irq_desc_t *desc)
{
#ifdef IRQ_CORE_ONLY_ROOT_IC_HAS_SYS_IRQ
	if (desc->ic_id != ROOT_IRQ_CONTROLLER_ID)
		return -IRQ_RET_CODE_ILLEGAL_IRQ_CONTROLLER_ID;
#endif

	if (g_reserved_irq_cnt >= core_get_reserved_sys_irq_cnt())
		return -IRQ_RET_CODE_RESERVE_IRQ_FAILED;

	g_reserved_sys_irq_id[g_reserved_irq_cnt] = desc->irq_id;
	g_reserved_irq_cnt++;
	return 0;
}
#else
static inline int core_is_reserved_system_irq(const irq_desc_t *desc)
{
	return 0;
}
#endif

#ifdef IRQ_CORE_LEGACY_IRQ_NUMBER_FORMAT
static inline uint32_t core_level1_irq_num_to_id(uint32_t irq_num)
{
	return irq_num;
}

static inline uint32_t core_level2_irq_num_to_id(uint32_t irq_num)
{
	return irq_num - 1;
}

static const irq_controller_t *core_get_level2_ic(uint32_t irq_id)
{
	uint32_t i;
	const irq_controller_t *ic;
	for (i = ROOT_IRQ_CONTROLLER_ID + 1; i < PLAT_IRQ_CONTROLLER_CNT; i++)
	{
		ic = core_get_irq_controller(i);
		if (!ic)
			continue;

		if (ic->irq_id == irq_id)
			return ic;
	}
	return NULL;
}

static int core_irq_num_to_irq_desc(int32_t irq_num, irq_desc_t *desc)
{
	if (irq_num < 0)
		return -IRQ_RET_CODE_INVALID_IRQ_NUM;

	uint32_t level1_irq_num = get_level1_irq_num(irq_num);
	uint32_t level2_irq_num = get_level2_irq_num(irq_num);

	uint32_t level1_irq_id, level2_irq_id, owner_irq_id;

	const irq_controller_t *level1_ic, *level2_ic, *owner_ic;

	level1_ic = core_get_root_irq_controller();
	if (!level1_ic)
		return -IRQ_RET_CODE_NO_ROOT_CONTROLLER;

	level1_irq_id = core_level1_irq_num_to_id(level1_irq_num);
	if (level1_irq_id >= level1_ic->irq_cnt)
		return -IRQ_RET_CODE_INVALID_LEVEL1_IRQ_ID;

	owner_ic = level1_ic;
	owner_irq_id = level1_irq_id;

	level2_ic = core_get_level2_ic(level1_irq_id);
	if (level2_ic)
	{
		level2_irq_id = core_level2_irq_num_to_id(level2_irq_num);
		if (level2_irq_id >= level2_ic->irq_cnt)
			return -IRQ_RET_CODE_INVALID_LEVEL2_IRQ_ID;

		owner_ic = level2_ic;
		owner_irq_id = level2_irq_id;
	}
	else
	{
		if (level2_irq_num)
		{
			return -IRQ_RET_CODE_INVALID_LEVEL2_IRQ_NUM;
		}
	}

	if (desc)
	{
		desc->ic_id = owner_ic->id;
		desc->irq_id = owner_irq_id;
	}
	return 0;
}

#else

/*
 * In the future, we counld directly get the interrupt controller id and
 * the interrupt id from the IRQ number.
 */
static int core_irq_num_to_irq_desc(int32_t irq_num, irq_desc_t *desc)
{
	return -IRQ_RET_CODE_INVALID_IRQ_CONTROLLER_ID;
}
#endif

static int core_request_irq(const irq_desc_t *desc, hal_irq_handler_t func, void *data)
{
	irq_handler_desc_t *irq_handler = core_get_irq_handler(desc->ic_id, desc->irq_id);
	if (!irq_handler)
	{
		return -IRQ_RET_CODE_HANDLER_DESC_NOT_FOUND;
	}

	if (irq_handler->func)
	{
		if ((irq_handler->func == func) && (irq_handler->data == data))
			return 0;

		return -IRQ_RET_CODE_ILLEGAL_IRQ_REQUEST;
	}

	irq_handler->func = func;
	irq_handler->data = data;
	// irq_handler->handled_cnt = 0; // should reset ?
	// irq_handler->unhandled_cnt = 0; // should reset ?

	return 0;
}

static int core_free_irq(const irq_desc_t *desc)
{
	irq_handler_desc_t *irq_handler = core_get_irq_handler(desc->ic_id, desc->irq_id);
	if (!irq_handler)
	{
		return -IRQ_RET_CODE_HANDLER_DESC_NOT_FOUND;
	}

	if (!irq_handler->func)
	{
		return -IRQ_RET_CODE_ILLEGAL_IRQ_FREE;
	}

	irq_handler->func = NULL;
	irq_handler->data = NULL;

	return 0;
}

static inline int core_get_irq_enable_state(const irq_desc_t *desc, int *enabled)
{
	int is_enabled;
	uint32_t level = 1;
	const irq_controller_t *ic, *parent_ic;

	ic = core_get_irq_controller(desc->ic_id);
	if (!ic)
	{
		irq_core_debug("core_get_irq_controller failed, ic id: %u", desc->ic_id);
		return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
	}

	is_enabled = ic->ops->irq_is_enabled(ic, desc->irq_id);
	if (!is_enabled)
	{
		*enabled = 0;
		return 0;
	}

	while (1)
	{
		if (ic->id == ROOT_IRQ_CONTROLLER_ID)
			break;

		if (level >= g_max_ic_level)
		{
			return -IRQ_RET_CODE_ENUM_IRQ_CONTROLLER_FAILED;
		}

		parent_ic = core_get_irq_controller(ic->parent_id);
		if (!parent_ic)
		{
			irq_core_debug("core_get_irq_controller failed, ic id: %u", ic->parent_id);
			return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
		}

		is_enabled = parent_ic->ops->irq_is_enabled(parent_ic, ic->irq_id);
		if (!is_enabled)
		{
			*enabled = 0;
			return 0;
		}

		ic = parent_ic;
		level++;
	}

	*enabled = 1;
	return 0;
}

static inline int core_enable_irq(const irq_desc_t *desc)
{
	int ret;
	uint32_t level = 1;
	const irq_controller_t *ic, *parent_ic;

	ic = core_get_irq_controller(desc->ic_id);
	if (!ic)
	{
		irq_core_debug("core_get_irq_controller failed, ic id: %u", desc->ic_id);
		return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
	}

	ret = ic->ops->irq_enable(ic, desc->irq_id);
	if (ret)
		return -IRQ_RET_CODE_ENABLE_IRQ_FAILED;

	while (1)
	{
		if (ic->id == ROOT_IRQ_CONTROLLER_ID)
			break;

		if (level >= g_max_ic_level)
		{
			return -IRQ_RET_CODE_ENUM_IRQ_CONTROLLER_FAILED;
		}

		parent_ic = core_get_irq_controller(ic->parent_id);
		if (!parent_ic)
		{
			irq_core_debug("core_get_irq_controller failed, ic id: %u", ic->parent_id);
			return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
		}

		ret = parent_ic->ops->irq_enable(parent_ic, ic->irq_id);
		if (ret)
			return -IRQ_RET_CODE_ENABLE_IRQ_FAILED;

		ic = parent_ic;
		level++;
	}

	return 0;
}

static inline int core_disable_irq(const irq_desc_t *desc)
{
	int ret;
	const irq_controller_t *ic;

	ic = core_get_irq_controller(desc->ic_id);
	if (!ic)
	{
		irq_core_debug("core_get_irq_controller failed, ic id: %u", desc->ic_id);
		return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
	}

	ret = ic->ops->irq_disable(ic, desc->irq_id);
	if (ret)
		return -IRQ_RET_CODE_DISABLE_IRQ_FAILED;

	//Currently there is not need to disable parent IC' irq
	return 0;
}

static inline int core_set_irq_trigger_type(const irq_desc_t *desc, irq_trigger_type_t trigger_type)
{
	int ret;
	const irq_controller_t *ic;

	ic = core_get_irq_controller(desc->ic_id);
	if (!ic)
	{
		irq_core_debug("core_get_irq_controller failed, ic id: %u", desc->ic_id);
		return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
	}

	ret = ic->ops->irq_set_trigger_type(ic, desc->irq_id, trigger_type);
	if (ret)
		return -IRQ_RET_CODE_SET_TRIGGER_TYPE_FAILED;

	return 0;
}

static hal_irqreturn_t core_handle_non_root_ic_irq(void *id)
{
	uint16_t ic_id = (unsigned long)id;
	uint32_t irq_id;
	const irq_controller_t *ic;
	irq_handler_desc_t *handler;

	ic = core_get_irq_controller(ic_id);

#ifdef IRQ_CORE_DEBUG_IC_IRQ_HANDLER
	if (!ic)
		return HAL_IRQ_ERR;
#endif

	for (irq_id = 0; irq_id < ic->irq_cnt; irq_id++)
	{
		if (!ic->ops->irq_is_enabled(ic, irq_id))
			continue;

		if (ic->ops->irq_is_pending && (!ic->ops->irq_is_pending(ic, irq_id)))
		{
			continue;
		}

		handler = core_get_irq_handler(ic->id, irq_id);

#ifdef IRQ_CORE_DEBUG_IC_IRQ_HANDLER
		if (!handler)
			return HAL_IRQ_ERR;
#endif

		trace_event_begin(EV_IRQ, "non root ic", ARG_UINT(ic_id));
		if (handler->func)
		{
			handler->func(handler->data);
#ifdef CONFIG_COMMAND_LIST_IRQ
			handler->handled_cnt++;
#endif
		}
		else
		{
#ifdef CONFIG_COMMAND_LIST_IRQ
			handler->unhandled_cnt++;
#endif
			printf("Error: no specific handler for non root IC irq(%u,%u)!\n", ic_id, irq_id);
			ic->ops->irq_disable(ic, irq_id);
		}
		trace_event_end(EV_IRQ, "non root ic", ARG_UINT(ic_id));
	}

	return HAL_IRQ_OK;
}

void irq_core_handle_root_ic_irq(uint32_t irq_id)
{
#ifdef CONFIG_COMMAND_IRQ_DEBUG
	uint64_t irq_starttime = 0, irq_endtime = 0, irq_timec = 0;
	TaskHandle_t  irq_tcb;
	irq_tcb = xTaskGetCurrentTaskHandle();
	irq_starttime = hal_get_timestamp();
#endif
	const irq_controller_t *ic = core_get_root_irq_controller();

#ifdef IRQ_CORE_DEBUG_IC_IRQ_HANDLER
	if (!ic)
		return;
#endif

	irq_handler_desc_t *handler = core_get_irq_handler(ic->id, irq_id);

#ifdef IRQ_CORE_DEBUG_IC_IRQ_HANDLER
	if (!handler)
		return;
#endif

	trace_event_begin(EV_IRQ, "root ic", ARG_UINT(irq_id));

	if (handler->func)
	{
		handler->func(handler->data);
#ifdef CONFIG_COMMAND_LIST_IRQ
		handler->handled_cnt++;
#endif
	}
	else
	{
		printf("Error: no specific handler for root IC irq %u!\n", irq_id);
		ic->ops->irq_disable(ic, irq_id);
#ifdef CONFIG_COMMAND_LIST_IRQ
		handler->unhandled_cnt++;
#endif
	}
	trace_event_end(EV_IRQ, "root ic", ARG_UINT(irq_id));

#ifdef CONFIG_COMMAND_IRQ_DEBUG
	irq_endtime = hal_get_timestamp();
	irq_timec = irq_endtime - irq_starttime;
	set_tcb_irqtotaltime(irq_tcb, irq_timec);
	handler->irqid_totaltime += irq_timec;
#endif
}

#ifdef CONFIG_COMMAND_IRQ_DEBUG
void core_irq_ic_total_time(void)
{
	uint32_t ic_id, irq_id;
	const irq_controller_t *ic;
	irq_handler_desc_t *handler;
	for (ic_id = 0; ic_id < PLAT_IRQ_CONTROLLER_CNT; ic_id++) {
		ic = core_get_irq_controller(ic_id);
		if (!ic) {
			printf("IC %d not found!\n", ic_id);
			continue;
		}

		printf("          IC %u:\n", ic_id);
		for (irq_id = 0; irq_id < ic->irq_cnt; irq_id++) {
			handler = core_get_irq_handler(ic_id, irq_id);
			if (!handler) {
				printf("IRQ handler(%u, %u) not found!\n", ic_id, irq_id);
				continue;
			}
			if (handler->irqid_totaltime != 0) {
				printf("           IRQ : %3d  total time : %10llu us   total trigger count : %10ld\n",irq_id,handler->irqid_totaltime,handler->handled_cnt);
			}
		}
		printf("\n");
	}
}
#endif

int irq_core_request_irq(uint32_t irq_num, hal_irq_handler_t func, void *data)
{
	int ret;
	irq_desc_t desc;

	if (!func)
		return -IRQ_RET_CODE_INVALID_HANDLER_FUNC;

	ret = core_check_init();
	if (ret)
		return ret;

	ret = core_irq_num_to_irq_desc(irq_num, &desc);
	if (ret)
		return ret;

	if (core_is_reserved_system_irq(&desc))
		return -IRQ_RET_CODE_RESERVED_IRQ;

	return core_request_irq(&desc, func, data);
}

static int __irq_core_free_irq(uint32_t irq_num, hal_irq_handler_t func, void *data, int is_check_handler)
{
	int ret;
	irq_desc_t desc;
	irq_handler_desc_t *irq_handler;

	ret = core_check_init();
	if (ret)
		return ret;

	ret = core_irq_num_to_irq_desc(irq_num, &desc);
	if (ret)
		return ret;

	if (core_is_reserved_system_irq(&desc))
		return -IRQ_RET_CODE_RESERVED_IRQ;


	if (is_check_handler)
	{
		irq_handler = core_get_irq_handler(desc.ic_id, desc.irq_id);
		if (!irq_handler)
			return -IRQ_RET_CODE_HANDLER_DESC_NOT_FOUND;

		if ((irq_handler->func != func) || (irq_handler->data != data))
			return -IRQ_RET_CODE_FREE_WITHOUT_SAME_USER;
	}

	ret = core_free_irq(&desc);
	if (ret)
		return ret;

	return 0;
}

int irq_core_free_irq(uint32_t irq_num, hal_irq_handler_t func, void *data)
{
	if (!func)
		return -IRQ_RET_CODE_INVALID_HANDLER_FUNC;

	return __irq_core_free_irq(irq_num, func, data, 1);
}

int irq_core_get_irq_enable_state(uint32_t irq_num, int *enabled)
{
	int ret;
	irq_desc_t desc;

	if (!enabled)
		return -IRQ_RET_CODE_INVALID_PARAMETER;

	ret = core_check_init();
	if (ret)
		return ret;

	ret = core_irq_num_to_irq_desc(irq_num, &desc);
	if (ret)
		return ret;

	return core_get_irq_enable_state(&desc, enabled);
}

int irq_core_enable_irq(uint32_t irq_num)
{
	int ret;
	irq_desc_t desc;

	ret = core_check_init();
	if (ret)
		return ret;

	ret = core_irq_num_to_irq_desc(irq_num, &desc);
	if (ret)
		return ret;

	if (core_is_reserved_system_irq(&desc))
		return -IRQ_RET_CODE_RESERVED_IRQ;

	return core_enable_irq(&desc);
}

int irq_core_disable_irq(uint32_t irq_num)
{
	int ret;
	irq_desc_t desc;

	ret = core_check_init();
	if (ret)
		return ret;

	ret = core_irq_num_to_irq_desc(irq_num, &desc);
	if (ret)
		return ret;

	if (core_is_reserved_system_irq(&desc))
		return -IRQ_RET_CODE_RESERVED_IRQ;

	return core_disable_irq(&desc);
}

int irq_core_disable_ic_irq(uint16_t ic_id)
{
	const irq_controller_t *ic;
	int irq_id, ret;
	irq_desc_t desc;

	ret = core_check_init();
	if (ret)
		return ret;

	ic = core_get_irq_controller(ic_id);
	if (!ic)
	{
		irq_core_debug("core_get_irq_controller failed, ic id:%u", ic_id);
		return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
	}

	desc.ic_id = ic_id;

	for (irq_id = 0; irq_id < ic->irq_cnt; irq_id++)
	{
		desc.irq_id = irq_id;
		ret = core_disable_irq(&desc);
		if (ret)
			return ret;
	}

	return 0;
}

int irq_core_disable_all_ic_irq(void)
{
	int ret, ic_id;

	for (ic_id = 0; ic_id < PLAT_IRQ_CONTROLLER_CNT; ic_id++)
	{
		ret = irq_core_disable_ic_irq(ic_id);
		if (ret)
			return ret;
	}

	return 0;
}


#ifdef IRQ_CORE_SUPPORT_RESERVED_SYS_IRQ
int irq_core_request_system_irq(uint32_t irq_num, hal_irq_handler_t func, void *data)
{
	int ret;
	irq_desc_t desc;

	ret = core_check_init();
	if (ret)
		return ret;

	ret = core_irq_num_to_irq_desc(irq_num, &desc);
	if (ret)
		return ret;

	ret = core_reserve_system_irq(&desc);
	if (ret)
		return ret;

	ret = core_request_irq(&desc, func, data);
	if (ret)
		return ret;

	return core_enable_irq(&desc);
}
#else
int irq_core_request_system_irq(uint32_t irq_num, hal_irq_handler_t func, void *data)
{
	int ret;
	ret = irq_core_request_irq(irq_num, func, data);
	if (ret)
		return ret;

	return irq_core_enable_irq(irq_num);
}
#endif

int irq_core_set_irq_trigger_type(uint32_t irq_num, irq_trigger_type_t trigger_type)
{
	int ret;
	irq_desc_t desc;

	ret = core_check_init();
	if (ret)
		return ret;

	ret = core_irq_num_to_irq_desc(irq_num, &desc);
	if (ret)
		return ret;

	return core_set_irq_trigger_type(&desc, trigger_type);
}

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
static inline int irq_core_priority_map(const irq_desc_t *desc, hal_irqprio_map_t map, hal_irq_prio_t *priority, uint32_t *preemptpriority, uint32_t *subpriority)
{
	int ret;
	const irq_controller_t *ic;

	ic = core_get_irq_controller(desc->ic_id);
	if (!ic)
	{
		irq_core_debug("core_get_irq_controller failed, ic id: %u", desc->ic_id);
		return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
	}

	ret = ic->ops->irq_priority_map(map, priority, preemptpriority, subpriority);
	if (ret)
		return -IRQ_RET_CODE_PRIORITY_MAP_FAILED;

	return 0;
}

static inline int irq_core_set_priority(const irq_desc_t *desc, uint32_t preemptpriority, uint32_t subpriority)
{
	int ret;
	const irq_controller_t *ic;

	ic = core_get_irq_controller(desc->ic_id);
	if (!ic)
	{
		irq_core_debug("core_get_irq_controller failed, ic id: %u", desc->ic_id);
		return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
	}

	ret = ic->ops->irq_set_priority(ic, desc->irq_id, preemptpriority, subpriority);
	if (ret)
		return -IRQ_RET_CODE_SET_PRIORITY_FAILED;

	return 0;
}

static inline int irq_core_get_priority(const irq_desc_t *desc, uint32_t *preemptpriority, uint32_t *subpriority)
{
	int ret;
	const irq_controller_t *ic;

	ic = core_get_irq_controller(desc->ic_id);
	if (!ic)
	{
		irq_core_debug("core_get_irq_controller failed, ic id: %u", desc->ic_id);
		return -IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND;
	}

	ret = ic->ops->irq_get_priority(ic, desc->irq_id, preemptpriority, subpriority);
	if (ret)
		return -IRQ_RET_CODE_GET_PRIORITY_FAILED;

	return 0;
}
#endif

int irq_core_init(void)
{
	int ret;
	uint32_t ic_level = 1;
	uint16_t i;
	const irq_controller_t *ic, *parent_ic;
	irq_handler_desc_t *handler;

	ret = core_check_plat_irq_info();
	if (ret)
		return ret;

	ret = core_plat_irq_init();
	if (ret)
		return -IRQ_RET_CODE_PLAT_IRQ_INIT_FAILED;

	ret = core_check_ic_info();
	if (ret)
		return ret;

	for (i = 0; i < PLAT_IRQ_CONTROLLER_CNT; i++)
	{
		ic = core_get_irq_controller(i);

		/* init every interrupt controller in system. */
		ret = ic->ops->init(ic);
		if (ret)
		{
			return ret;
		}

		if (ic->id == ROOT_IRQ_CONTROLLER_ID)
			continue;

		parent_ic = core_get_irq_controller(ic->parent_id);

		handler = core_get_irq_handler(parent_ic->id, ic->irq_id);
		handler->func = core_handle_non_root_ic_irq;
		handler->data = (void *)(unsigned long)i;
#ifdef CONFIG_COMMAND_LIST_IRQ
		handler->handled_cnt = 0;
		handler->unhandled_cnt = 0;
#endif

#ifdef CONFIG_COMMAND_IRQ_DEBUG
		handler->irqid_totaltime = 0;
#endif
		ic_level = core_get_ic_level(ic->id);
		if (ic_level > g_max_ic_level)
			g_max_ic_level = ic_level;
	}

	g_is_irq_core_init = 1;
	return 0;
}

static void core_dump_ic_info(const irq_controller_t *ic, int need_header)
{
	if (need_header)
		printf("--------IRQ Controller info--------\n");

	printf("id            :              %u\n", ic->id);
	//printf("name         :              '%s'\n", ic->name);
	printf("irq_cnt       :              %u\n", ic->irq_cnt);
	printf("parent_id     :              %u\n", ic->parent_id);
	printf("irq_id        :              %u\n", ic->irq_id);
	//printf("flag         :              %u\n", ic->flag);
	printf("reg_base_addr :              0x%lx\n", ic->reg_base_addr);

	printf("ops           :              %p\n", ic->ops);
#if defined(CONFIG_COMPONENTS_PM)
	printf("priv          :              %p\n", ic->priv);
#endif
}
static void core_dump_handler_info(const irq_handler_desc_t *handler, int need_header)
{
	if (need_header)
		printf("--------IRQ Handler info--------\n");
	printf("func          : %p\n", handler->func);
	printf("data          : %p\n", handler->data);
}

void irq_core_dump_sys_ic_info(void)
{
	uint32_t i;
	const irq_controller_t *ic;
	int ret;

	ret = core_check_init();
	if (ret)
	{
		printf("IRQ Core Framework is uninitialized!\n");
		return;
	}

	for (i = 0; i < PLAT_IRQ_CONTROLLER_CNT; i++)
	{
		ic = core_get_irq_controller(i);
		if (!ic)
		{
			printf("IC %d not found!\n", i);
			continue;
		}

		if (i == ROOT_IRQ_CONTROLLER_ID)
		{
			printf("--------Root IRQ Controller info--------\n");
			core_dump_ic_info(ic, 0);
		}
		else
		{
			core_dump_ic_info(ic, 1);
		}
	}
}

void irq_core_dump_irq_handler_info(void)
{
	uint32_t ic_id, irq_id;
	const irq_controller_t *ic;
	const irq_handler_desc_t *handler;
	int ret;

	ret = core_check_init();
	if (ret)
	{
		printf("IRQ Core Framework is uninitialized!\n");
		return;
	}

	for (ic_id = 0; ic_id < PLAT_IRQ_CONTROLLER_CNT; ic_id++)
	{
		ic = core_get_irq_controller(ic_id);
		if (!ic)
		{
			printf("IC %d not found!\n", ic_id);
			continue;
		}

		printf("IC %u:\n", ic_id);
		for (irq_id = 0; irq_id < ic->irq_cnt; irq_id++)
		{
			handler = core_get_irq_handler(ic_id, irq_id);
			if (!handler)
			{
				printf("IRQ handler(%u, %u) not found!\n", ic_id, irq_id);
				continue;
			}
			if ((!handler->func) && (!handler->data))
			{
				continue;
			}

			printf("IRQ   %3u     :  handler     %p:\n", irq_id, handler);
			core_dump_handler_info(handler, 0);
		}
		printf("\n");
	}

}

void irq_core_dump_irq_info(void)
{
	uint32_t ic_id, irq_id;
	const irq_controller_t *ic;
	irq_desc_t desc;
	int ret, enabled;

	ret = core_check_init();
	if (ret)
	{
		printf("IRQ Core Framework is uninitialized!\n");
		return;
	}

	for (ic_id = 0; ic_id < PLAT_IRQ_CONTROLLER_CNT; ic_id++)
	{
		ic = core_get_irq_controller(ic_id);
		if (!ic)
		{
			printf("IC %d not found!\n", ic_id);
			continue;
		}

		printf("IC %u:\n", ic_id);
		desc.ic_id = ic_id;
		enabled = 0;
		for (irq_id = 0; irq_id < ic->irq_cnt; irq_id++)
		{
			desc.irq_id = irq_id;
			ret = core_get_irq_enable_state(&desc, &enabled);
			if (ret)
			{
				printf("get IRQ(%u, %u) enable state failed! ret: %d\n", ic_id, irq_id, ret);
				continue;
			}

			if (!enabled)
				continue;

			printf("IRQ        %3u:              Enabled\n", irq_id);
		}
		printf("\n");
	}

}

#ifdef IRQ_CORE_PROVIDE_STANDARD_HAL_API
int32_t arch_request_irq(int32_t irq_num, hal_irq_handler_t func, void *data)
{
	return irq_core_request_irq(irq_num, func, data);
}

void arch_free_irq(int32_t irq_num)
{
	__irq_core_free_irq(irq_num, NULL, NULL, 0);
}

int arch_get_irq_enable_state(uint32_t irq_num, int *enabled)
{
	return irq_core_get_irq_enable_state(irq_num, enabled);
}

void arch_enable_irq(unsigned int irq_num)
{
	irq_core_enable_irq(irq_num);
}

void arch_disable_irq(unsigned int irq_num)
{
	irq_core_disable_irq(irq_num);
}

void arch_disable_all_irq(void)
{
	riscv_local_irq_disable();
}

void arch_enable_all_irq(void)
{
	riscv_local_irq_enable();
}

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
static int core_irq_common_check(int32_t irq_num, irq_desc_t *desc, uint8_t reserve_check)
{
	int ret;

	ret = core_check_init();
	if (ret)
		return ret;

	ret = core_irq_num_to_irq_desc(irq_num, desc);
	if (ret)
		return ret;

	if (reserve_check) {
		if (core_is_reserved_system_irq(desc))
			return -IRQ_RET_CODE_RESERVED_IRQ;
	}

	return 0;
}

int arch_irq_priority_map(int32_t irq_num, hal_irqprio_map_t map, hal_irq_prio_t *priority, uint32_t *preemptpriority, uint32_t *subpriority)
{
	int ret;
	irq_desc_t desc;

	ret = core_irq_common_check(irq_num, &desc, 0);
	if (ret)
		return ret;

	return irq_core_priority_map(&desc, map, priority, preemptpriority, subpriority);
}

int arch_irq_set_priority(int32_t irq_num, uint32_t preemptpriority, uint32_t subpriority)
{
	int ret;
	irq_desc_t desc;

	ret = core_irq_common_check(irq_num, &desc, 1);
	if (ret)
		return ret;

	return irq_core_set_priority(&desc, preemptpriority, subpriority);
}

int arch_irq_get_priority(int32_t irq_num, uint32_t *preemptpriority, uint32_t *subpriority)
{
	int ret;
	irq_desc_t desc;

	ret = core_irq_common_check(irq_num, &desc, 0);
	if (ret)
		return ret;

	return irq_core_get_priority(&desc, preemptpriority, subpriority);
}
#endif /* CONFIG_ARCH_RISCV_INTERRUPT_NEST */

void xport_interrupt_enable(unsigned long flags)
{
	return riscv_local_irq_restore(flags);
}

unsigned long xport_interrupt_disable(void)
{
	return riscv_local_irq_save();
}

unsigned long arch_irq_is_disable(void)
{
	return riscv_irqs_disabled_flags(riscv_arch_local_save_flags());
}
#ifdef CONFIG_COMMAND_LIST_IRQ
static void show_ic(const irq_controller_t *ic)
{
	printf("ic%u:\n", ic->id);
}

static const char *irq_name(uint32_t ic_id, uint32_t irq_id)
{
	if (ic_id != ROOT_IRQ_CONTROLLER_ID)
		return "(nil)"; // TODO

	if (irq_id < PLAT_CLIC_IRQ_CNT && irq_major_string[irq_id])
		return irq_major_string[irq_id];
	else
		return "(nil)";
}

#define IRQ_NUM_OFFSET		0
#if 0
static const char *irq_table_header =
      /* 123456 123 123 1234567890 1234567890 123 12345 12345678901234567890 12345678 12345678 123... */
	" irq  | l1| l2|   handled| unhandled| en| pend|                name|     fun|    data|desc";
#define IRQ_NUM_WIDTH		6
#define CORE_ICID_OFFSET	(IRQ_NUM_OFFSET + IRQ_NUM_WIDTH + 1)
#define CORE_ICID_WIDTH		3
#define CORE_IRQID_OFFSET	(CORE_ICID_OFFSET + CORE_ICID_WIDTH + 1)
#define CORE_IRQID_WIDTH	3
#define IRQ_CORE_HD_OFFSET	(CORE_IRQID_OFFSET + CORE_IRQID_WIDTH + 1)
#else
static const char *irq_table_header =
      /* 123 1234567890 1234567890 123 12345 12345678901234567890 12345678 12345678 123... */
	"irq|   handled| unhandled| en| pend|                name|     fun|    data|desc";
#define IRQ_NUM_WIDTH		3
#define IRQ_CORE_HD_OFFSET	(IRQ_NUM_OFFSET + IRQ_NUM_WIDTH + 1)
#endif
#define IRQ_CORE_HD_WIDTH	10
#define IRQ_UHD_OFFSET		(IRQ_CORE_HD_OFFSET + IRQ_CORE_HD_WIDTH + 1)
#define IRQ_UHD_WIDTH		10
#define IRQ_EN_STAT_OFFSET	(IRQ_UHD_OFFSET + IRQ_UHD_WIDTH + 1)
#define IRQ_EN_STAT_WIDTH	3
#define IRQ_PEND_STAT_OFFSET	(IRQ_EN_STAT_OFFSET + IRQ_EN_STAT_WIDTH + 1)
#define IRQ_PEND_STAT_WIDTH	5
#define IRQ_NAME_OFFSET		(IRQ_PEND_STAT_OFFSET + IRQ_PEND_STAT_WIDTH + 1)
#define IRQ_NAME_WIDTH		20
#define IRQ_FUN_OFFSET		(IRQ_NAME_OFFSET + IRQ_NAME_WIDTH + 1)
#define IRQ_FUN_WIDTH		8
#define IRQ_DATA_OFFSET		(IRQ_FUN_OFFSET + IRQ_FUN_WIDTH + 1)
#define IRQ_DATA_WIDTH		8
#define IRQ_DESC_OFFSET		(IRQ_DATA_OFFSET + IRQ_DATA_WIDTH + 1)
#define IRQ_DESC_WIDTH		16
#define FIXED_LEN		(IRQ_DESC_OFFSET + IRQ_DESC_WIDTH)

static irq_handler_desc_t fake_irq_handler = {
	.func = NULL,
	.data = NULL,
	.handled_cnt = 0,
	.unhandled_cnt = 0,
};
static void show_irq(const irq_controller_t *ic, uint32_t irq_id)
{
	irq_handler_desc_t *irq_handler = core_get_irq_handler(ic->id, irq_id);
	irq_desc_t desc = {
		.ic_id = ic->id,
		.irq_id = irq_id,
	};
	char buf[FIXED_LEN + 1];
	int is_reserved = 0;
	const char *en_stat = "-";
	const char *pend_stat = "-";

	if (irq_handler == NULL) {
		irq_handler = &fake_irq_handler;
	} else {
		is_reserved = core_is_reserved_system_irq(&desc);
		if (ic->ops && ic->ops->irq_is_enabled) {
			en_stat = ic->ops->irq_is_enabled(ic, irq_id) ? "en" : "off";
		}
		if (ic->ops && ic->ops->irq_is_pending) {
			pend_stat = ic->ops->irq_is_pending(ic, irq_id) ? "pend" : "clear";
		}
	}


	snprintf(buf, FIXED_LEN + 1,
		"%*u" " "
#if 0
		"%*u" " "
		"%*u" " "
#endif
		"%*lu" " "
		"%*lu" " "
		"%*.*s" " "
		"%*.*s" " "
		"%*.*s" " "
		"%*lx" " "
		"%*lx" " "
		"%s"
		,
		IRQ_NUM_WIDTH, get_irq_num(ic->id, irq_id),
#if 0
		CORE_ICID_WIDTH, ic->id,
		CORE_IRQID_WIDTH, irq_id,
#endif
		IRQ_CORE_HD_WIDTH, irq_handler->handled_cnt,
		IRQ_UHD_WIDTH, irq_handler->unhandled_cnt,
		IRQ_EN_STAT_WIDTH, IRQ_EN_STAT_WIDTH, en_stat,
		IRQ_PEND_STAT_WIDTH, IRQ_PEND_STAT_WIDTH, pend_stat,
		IRQ_NAME_WIDTH, IRQ_NAME_WIDTH, irq_name(ic->id, irq_id),
		IRQ_FUN_WIDTH, (unsigned long)irq_handler->func,
		IRQ_DATA_WIDTH, (unsigned long)irq_handler->data,
		(is_reserved ? " reserved" : "")
		);

	if (irq_handler == &fake_irq_handler) {
		buf[IRQ_CORE_HD_OFFSET + IRQ_CORE_HD_WIDTH - 1] = '-';
		buf[IRQ_UHD_OFFSET + IRQ_UHD_WIDTH - 1] = '-';
		buf[IRQ_FUN_OFFSET + IRQ_FUN_WIDTH - 1] = '-';
		buf[IRQ_DATA_OFFSET + IRQ_DATA_WIDTH - 1] = '-';
	}

	buf[FIXED_LEN] = 0;
	printf("%s\n", buf);
}

#define EN_IRQ		(0x1lu << 31)
#define OFF_IRQ		(0x1lu << 30)
#define USED_IRQ	(0x1lu << 29)
#define FREE_IRQ	(0x1lu << 28)
#define UHD_IRQ		(0x1lu << 27)
#define NUM_IRQ		(0x1lu << 26)
#define FULL_MATCH_MODE	(0x1lu << 24)

#define NUM_IRQ_MASK	((0x1lu << 24) - 1)
#define ALL_IRQ		(EN_IRQ | OFF_IRQ | USED_IRQ | FREE_IRQ | UHD_IRQ)

static int is_need_irq(const irq_controller_t *ic, uint32_t irq_id, unsigned long flags)
{
	int en_stat = 0, used = 0;
	irq_handler_desc_t *irq_handler = NULL;

	if ((flags & NUM_IRQ)) { // fast path
		if ((flags & NUM_IRQ_MASK) == get_irq_num(ic->id, irq_id))
			return 1;
		else
			return 0;
	}

	if ((flags & ALL_IRQ) == 0) // fast path
		return 0;

	if ((flags & ALL_IRQ) == ALL_IRQ) // fast path
		return 1;

	if (flags & (EN_IRQ | OFF_IRQ)) {
		if (ic->ops && ic->ops->irq_is_enabled)
			en_stat = ic->ops->irq_is_enabled(ic, irq_id);
	}

	if (flags & (USED_IRQ | FREE_IRQ | UHD_IRQ)) {
		irq_handler = core_get_irq_handler(ic->id, irq_id);
		if (irq_handler && irq_handler->func) {
			used = 1;
		}
	}

	if (flags & FULL_MATCH_MODE) {
		if ((flags & EN_IRQ) && !en_stat)
			return 0;

		if ((flags & OFF_IRQ) && en_stat)
			return 0;

		if ((flags & USED_IRQ) && !used)
			return 0;

		if ((flags & FREE_IRQ) && used)
			return 0;

		if ((flags & UHD_IRQ) && !(irq_handler && irq_handler->unhandled_cnt))
			return 0;

		return 1;
	} else {
		if ((flags & EN_IRQ) && en_stat)
			return 1;

		if ((flags & OFF_IRQ) && !en_stat)
			return 1;

		if ((flags & USED_IRQ) && used)
			return 1;

		if ((flags & FREE_IRQ) && used)
			return 1;

		if ((flags & UHD_IRQ) && irq_handler && irq_handler->unhandled_cnt)
			return 1;

		return 0;
	}
}

void show_irqs_filter(unsigned long flags)
{
	const irq_controller_t *level1_ic, *level2_ic;
	uint32_t l1_id, l2_id;

	level1_ic = core_get_root_irq_controller();
	if (!level1_ic)
		return;

	printf("%s\n", irq_table_header);

	show_ic(level1_ic);
	for (l1_id = 0; l1_id < level1_ic->irq_cnt; l1_id++) {
		level2_ic = core_get_level2_ic(l1_id);
		if (!level2_ic) {
			if (is_need_irq(level1_ic, l1_id, flags))
				show_irq(level1_ic, l1_id);
			continue;
		}

		show_ic(level1_ic);
		for (l2_id = 0; l2_id < level2_ic->irq_cnt; l2_id++) {
			if (is_need_irq(level2_ic, l2_id, flags))
				show_irq(level2_ic, l2_id);
		}
	}
}

#ifdef CONFIG_COMMAND_LIST_IRQ_ENHANCE

static unsigned long update_irq_flags(unsigned long flags)
{
	if (flags & NUM_IRQ)
		return NUM_IRQ | (flags & NUM_IRQ_MASK);

	if ((flags & ALL_IRQ) == 0)
		return 0;

	if ((flags & ALL_IRQ) == ALL_IRQ)
		return ALL_IRQ;

	if ((flags & EN_IRQ) && (flags & OFF_IRQ))
		return (flags & FULL_MATCH_MODE) ? 0 : ALL_IRQ;

	if ((flags & USED_IRQ) && (flags & FREE_IRQ))
		return (flags & FULL_MATCH_MODE) ? 0 : ALL_IRQ;

	return flags;
}

static void show_irqs_help(const char *cmd)
{
	printf("usage: %s [-a] [-d] [-e] [-u] [-f] [-U] [-F] [-i <num>] [-h]\n", cmd);
	printf("\t -a: show all irqs\n");
	printf("\t -d: show disabled irqs\n");
	printf("\t -e: show enabled irqs\n");
	printf("\t -u: show request irqs\n");
	printf("\t -f: show free irqs\n");
	printf("\t -U: show irq with non zero unhandled cnt\n");
	printf("\t -F: match all conditions, default is match any condition if not set\n");
	printf("\t -i: show irq with specified ID\n");
	printf("\t -h: show help info\n");
}

void show_irqs(int argc, char **argv)
{
	unsigned long new_flags, flags = EN_IRQ | USED_IRQ;
	int ch;

	if (argc < 2) {
		printf("default show request irqs + enabled irqs\n");
		goto end;
	}

	flags = 0;
	while((ch = getopt(argc, argv, "adeufUFi:h")) != -1) {
		switch (ch) {
		case 'a': flags |= ALL_IRQ; break;
		case 'd': flags |= OFF_IRQ; break;
		case 'e': flags |= EN_IRQ; break;
		case 'u': flags |= USED_IRQ; break;
		case 'f': flags |= FREE_IRQ; break;
		case 'U': flags |= UHD_IRQ; break;
		case 'F': flags |= FULL_MATCH_MODE; break;
		case 'i':
			flags = strtoul(optarg, NULL, 0);
			flags |= NUM_IRQ;
			break;
		case 'h':
		default:
			show_irqs_help(argv[0]);
			return;
		}
	}
	new_flags = update_irq_flags(flags);
	if (!new_flags) {
		printf("bad filter flags: %lx\n", flags);
		show_irqs_help(argv[0]);
		return;
	}
	flags = new_flags;
end:
	show_irqs_filter(flags);
}
#else

void show_irqs(void)
{
	show_irqs_filter(ALL_IRQ);
}
#endif /* CONFIG_COMMAND_LIST_IRQ_ENHANCE */
#endif /* CONFIG_COMMAND_LIST_IRQ*/
#endif /* IRQ_CORE_PROVIDE_STANDARD_HAL_API */

