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
#ifndef __IRQ_CORE_H__
#define __IRQ_CORE_H__

#include <stdint.h>

#include <hal_interrupt.h>

#if defined(CONFIG_COMPONENTS_PM)
#include "pm_syscore.h"
#endif

/*
 * Due to the number of interrupt controller can be one.
 * In this situation, the id of root interrupt controller is zero.
 * So the ID of root interrupt controller must be zero!
 */
#define ROOT_IRQ_CONTROLLER_ID 0

#define IRQ_CORE_LEGACY_IRQ_NUMBER_FORMAT

#define IRQ_CORE_SUPPORT_RESERVED_SYS_IRQ
#define IRQ_CORE_ONLY_ROOT_IC_HAS_SYS_IRQ

#define IRQ_CORE_PROVIDE_STANDARD_HAL_API


/*
* notify how much core reserved sys irq we registered
* 1: RV_SOFT_INTERRUPT
* 2: RV_ARCH_TIMER_INTERRURT
* 3: RV_ARCH_WATCHDOG_INTERRUT registerd in hal_watchdog.c
*/
#ifdef CONFIG_ARCH_SUN55IW6
#define IRQ_CORE_RESERVED_SYS_IRQ_CNT 3
#else
#define IRQ_CORE_RESERVED_SYS_IRQ_CNT 2
#endif

enum irq_core_ret_code
{
	IRQ_RET_CODE_OK = 0,
	IRQ_RET_CODE_INVALID_IRQ_NUM = 5,
	IRQ_RET_CODE_INVALID_LEVEL1_IRQ_ID,
	IRQ_RET_CODE_INVALID_LEVEL2_IRQ_ID,
	IRQ_RET_CODE_INVALID_LEVEL2_IRQ_NUM,
	IRQ_RET_CODE_INVALID_IRQ_ID,

	IRQ_RET_CODE_NO_ROOT_CONTROLLER,
	IRQ_RET_CODE_INVALID_HANDLER_FUNC,

	IRQ_RET_CODE_IRQ_CONTROLLER_NOT_FOUND,
	IRQ_RET_CODE_HANDLER_DESC_NOT_FOUND,

	IRQ_RET_CODE_ILLEGAL_IRQ_CONTROLLER_ID,
	IRQ_RET_CODE_ILLEGAL_IRQ_REQUEST,
	IRQ_RET_CODE_ILLEGAL_IRQ_FREE,
	IRQ_RET_CODE_FREE_WITHOUT_SAME_USER,
	IRQ_RET_CODE_ENABLE_IRQ_FAILED,
	IRQ_RET_CODE_DISABLE_IRQ_FAILED,
	IRQ_RET_CODE_SET_TRIGGER_TYPE_FAILED,
	IRQ_RET_CODE_PRIORITY_MAP_FAILED,
	IRQ_RET_CODE_SET_PRIORITY_FAILED,
	IRQ_RET_CODE_GET_PRIORITY_FAILED,

	IRQ_RET_CODE_RESERVED_IRQ,
	IRQ_RET_CODE_RESERVE_IRQ_FAILED,

	IRQ_RET_CODE_PLAT_IRQ_INIT_FAILED,
	IRQ_RET_CODE_INVALID_PLAT_IC_CNT,
	IRQ_RET_CODE_ENUM_IRQ_CONTROLLER_FAILED,
	IRQ_RET_CORE_INVALID_PLAT_IRQ_INFO,
	IRQ_RET_CODE_INVALID_IRQ_CNT,
	IRQ_RET_CODE_INVALID_REG_BASE_ADDR,
	IRQ_RET_CODE_INVALID_IRQ_CONTROLLER_OPS,
	IRQ_RET_CODE_INVALID_ROOT_CONTROLLER_CNT,
	IRQ_RET_CODE_INVALID_IRQ_CONTROLLER_ID,
	IRQ_RET_CODE_CORE_NOT_INIT,

	IRQ_RET_CODE_INCORRECT_IRQ_CNT,
	IRQ_RET_CODE_INVALID_PARAMETER,
	IRQ_RET_CODE_MAX
};

typedef enum irq_trigger_type
{
	IRQ_TRIGGER_TYPE_LEVEL,
	IRQ_TRIGGER_TYPE_EDGE_RISING,
	IRQ_TRIGGER_TYPE_EDGE_FALLING,
	IRQ_TRIGGER_TYPE_EDGE_BOTH
} irq_trigger_type_t;

typedef struct irq_handler_desc
{
	hal_irq_handler_t func;
	void *data;
#ifdef CONFIG_COMMAND_LIST_IRQ
	unsigned long handled_cnt;
	unsigned long unhandled_cnt;
#endif
#ifdef CONFIG_COMMAND_IRQ_DEBUG
	uint64_t irqid_totaltime;
#endif
} irq_handler_desc_t;

struct irq_controller;

typedef struct irq_controller_ops
{
	int (*init)(const struct irq_controller *ic);
	int (*irq_enable)(const struct irq_controller *ic, uint32_t irq_id);
	int (*irq_disable)(const struct irq_controller *ic, uint32_t irq_id);
	int (*irq_is_enabled)(const struct irq_controller *ic, uint32_t irq_id);
	int (*irq_is_pending)(const struct irq_controller *ic, uint32_t irq_id);
	int (*irq_set_pending)(const struct irq_controller *ic, uint32_t irq_id, int pending);
	int (*irq_set_trigger_type)(const struct irq_controller *ic, uint32_t irq_id, irq_trigger_type_t trigger_type);
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	int (*irq_priority_map)(hal_irqprio_map_t map, hal_irq_prio_t *priority, uint32_t *preemptpriority, uint32_t *subpriority);
	int (*irq_set_priority)(const struct irq_controller *ic, uint32_t irq_id, uint32_t preemptpriority, uint32_t subpriority);
	int (*irq_get_priority)(const struct irq_controller *ic, uint32_t irq_id, uint32_t *preemptpriority, uint32_t *subpriority);
#endif
} irq_controller_ops_t;

typedef struct irq_controller
{
	uint16_t id;
	uint16_t irq_cnt;
	uint16_t parent_id;
	uint16_t irq_id;
	//uint32_t flag; /* currently useless */

	unsigned long reg_base_addr;
	//const char *name; /* currently useless */
	irq_controller_ops_t *ops;
#if defined(CONFIG_COMPONENTS_PM)
	struct syscore_ops pm_ops;
	void *priv;
#endif
} irq_controller_t;

#include <hal/sunxi_hal_common.h>
#define ic_readb(addr) hal_readb(addr)
#define ic_readl(addr) hal_readl(addr)

#define ic_writeb(data, addr) hal_writeb(data, addr)
#define ic_writel(data, addr) hal_writel(data, addr)

typedef struct platform_irq_info
{
	int (*init)(void);
	irq_controller_t *(*get_irq_controller)(unsigned int ic_id);
	irq_handler_desc_t *(*get_irq_handler)(uint32_t ic_id, uint32_t irq_id);
} platform_irq_info_t;

extern const platform_irq_info_t g_plat_irq_info;

int irq_core_init(void);

void irq_core_dump_sys_ic_info(void);
void irq_core_dump_irq_handler_info(void);
void irq_core_dump_irq_info(void);

void irq_core_handle_root_ic_irq(uint32_t irq_id);

int irq_core_request_irq(uint32_t irq_num, hal_irq_handler_t func, void *data);
int irq_core_free_irq(uint32_t irq_num, hal_irq_handler_t func, void *data);

int irq_core_get_irq_enable_state(uint32_t irq_num, int *enabled);

int irq_core_enable_irq(uint32_t irq_num);
int irq_core_disable_irq(uint32_t irq_num);

int irq_core_disable_ic_irq(uint16_t ic_id);
static inline int irq_core_disable_root_ic_irq(void)
{
	return irq_core_disable_ic_irq(ROOT_IRQ_CONTROLLER_ID);
}
int irq_core_disable_all_ic_irq(void);

int irq_core_request_system_irq(uint32_t irq_num, hal_irq_handler_t func, void *data);

int irq_core_set_irq_trigger_type(uint32_t irq_num, irq_trigger_type_t trigger_type);

#ifdef CONFIG_COMMAND_IRQ_DEBUG
void core_irq_ic_total_time(void);
#endif
#endif