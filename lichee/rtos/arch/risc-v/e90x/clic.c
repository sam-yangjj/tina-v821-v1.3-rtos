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
#include "irq_core.h"
#include "platform/platform_irq.h"
#include "clic.h"

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
static void clic_irq_priority_init(void);
#endif

static inline void clic_set_irq_ctrl_bit(uint32_t reg_addr, uint8_t mask, int is_set)
{
	uint8_t reg_data = ic_readb(reg_addr);

	if (is_set)
		reg_data |= mask;
	else
		reg_data &= ~mask;

	ic_writeb(reg_data, reg_addr);
}

static inline void clic_set_enable(uint32_t reg_addr, int enabled)
{
	clic_set_irq_ctrl_bit(reg_addr, IE_BIT_MASK, enabled);
}

static inline int clic_set_pending(uint32_t reg_addr, int pending)
{
	clic_set_irq_ctrl_bit(reg_addr, IP_BIT_MASK, pending);
	return 0;
}

static inline void clic_set_vec_mode(uint32_t reg_addr, int vec_mode)
{
	clic_set_irq_ctrl_bit(reg_addr, HW_VECTOR_IRQ_BIT_MASK, vec_mode);
}

static inline void clic_set_trigger_type(uint32_t reg_addr, irq_trigger_type_t type)
{
	uint8_t reg_data, field_value;

	if (type == IRQ_TRIGGER_TYPE_LEVEL)
	{
		field_value = 0;
	}
	else if (type == IRQ_TRIGGER_TYPE_EDGE_RISING)
	{
		field_value = 1;
	}
	else if (type == IRQ_TRIGGER_TYPE_EDGE_FALLING)
	{
		field_value = 3;
	}
	else
	{
		return;
	}

	reg_data = ic_readb(reg_addr);

	reg_data &= ~TRIGGER_TYPE_BIT_MASK;
	reg_data |= field_value << TRIGGER_TYPE_SHIFT;

	ic_writeb(reg_data, reg_addr);
}

int clic_init(const struct irq_controller *ic)
{
	uint32_t i, reg_addr, reg_data, irq_cnt, preemption_bits;

	reg_addr = ic->reg_base_addr + CLIC_INFO_REG_OFF;
	reg_data = ic_readl(reg_addr);

	irq_cnt = (reg_data & IRQ_CNT_MASK) >> IRQ_CNT_SHIFT;
	if (ic->irq_cnt != irq_cnt)
		return -IRQ_RET_CODE_INCORRECT_IRQ_CNT;

	preemption_bits = (reg_data & CTRL_REG_BITS_MASK) >> CTRL_REG_BITS_SHIFT;
	preemption_bits <<= PREEMPTION_PRIORITY_BITS_SHIFT;
	preemption_bits &= PREEMPTION_PRIORITY_BITS_MASK;

	reg_addr = ic->reg_base_addr + CLIC_CFG_REG_OFF;
	ic_writel(preemption_bits, reg_addr);

	for (i = 0; i < irq_cnt; i++)
	{
		//disable all interrupt
		reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(i);
		clic_set_enable(reg_addr, 0);

		//set all interrupt to hardware vector mode
		reg_addr = ic->reg_base_addr + CLIC_INT_X_ATTR_REG_OFF(i);
		clic_set_vec_mode(reg_addr, 1);

		//clear pending
		reg_addr = ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(i);
		clic_set_pending(reg_addr, 1);
	}

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	clic_irq_priority_init();
#endif

	return 0;
}

int clic_irq_enable(const struct irq_controller *ic, uint32_t irq_id)
{
	uint32_t reg_addr;

	reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(irq_id);
	clic_set_enable(reg_addr, 1);
	return 0;
}

int clic_irq_disable(const struct irq_controller *ic, uint32_t irq_id)
{
	uint32_t reg_addr;

	reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(irq_id);
	clic_set_enable(reg_addr, 0);
	return 0;
}

int clic_irq_is_enabled(const struct irq_controller *ic, uint32_t irq_id)
{
	if (ic_readb(ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(irq_id)) & IE_BIT_MASK)
		return 1;

	return 0;
}

int clic_irq_is_pending(const struct irq_controller *ic, uint32_t irq_id)
{
	if (ic_readb(ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(irq_id)) & IP_BIT_MASK)
		return 1;

	return 0;
}

int clic_irq_set_pending(const struct irq_controller *ic, uint32_t irq_id, int pending)
{
	uint32_t reg_addr;

	reg_addr = ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(irq_id);
	clic_set_pending(reg_addr, pending);
	return 0;
}

int clic_irq_set_trigger_type(const struct irq_controller *ic, uint32_t irq_id, irq_trigger_type_t type)
{
	uint32_t reg_addr;

	if (type == IRQ_TRIGGER_TYPE_EDGE_BOTH)
		return -1;

	reg_addr = ic->reg_base_addr + CLIC_INT_X_ATTR_REG_OFF(irq_id);
	clic_set_trigger_type(reg_addr, type);
	return 0;
}

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
/*
 * | 7    7 - CLICINTCTL_NLBITS |   ......     | 7 - PLAT_CLIC_CLICINTCTLBITS   0 |
 * |   preempt priority         | sub priority | Invalid bit, value 1             |
 */
#define PRIORITY_REG_MAX		(0xff)
#define CLICINTCTL_NLBITS_MASK		(PRIORITY_REG_MAX >> CLICINTCTL_NLBITS)
#define PRIORITY_INVALID_BIT_MASK	(PRIORITY_REG_MAX >> PLAT_CLIC_CLICINTCTLBITS)
#define PREEMPTPRIORITY_VALID_SHIFT	(8 - CLICINTCTL_NLBITS)
#define SUBPRIORITY_VALID_SHIFT		(8 - PLAT_CLIC_CLICINTCTLBITS)

#define PREEMPTPRIORITY_REG_MAX		(PRIORITY_REG_MAX)
#define PREEMPTPRIORITY_REG_MIN		(CLICINTCTL_NLBITS_MASK)
#define SUBPRIORITY_REG_MAX		(PRIORITY_REG_MAX)
#define SUBPRIORITY_REG_MIN		(PRIORITY_INVALID_BIT_MASK)

#define PREEMPTPRIORITY_MAX		((~CLICINTCTL_NLBITS_MASK & PRIORITY_REG_MAX) >> PREEMPTPRIORITY_VALID_SHIFT)
#define PREEMPTPRIORITY_MIN		(0)
#define SUBPRIORITY_MAX			(((~PRIORITY_INVALID_BIT_MASK & CLICINTCTL_NLBITS_MASK) & PRIORITY_REG_MAX) >> SUBPRIORITY_VALID_SHIFT)
#define	SUBPRIORITY_MIN			(0)

#ifdef CONFIG_INTERRUPT_NEST_DEBUG_LOG
#define nest_dbg(fmt, arg...)		do { printf(fmt, ##arg); } while(0)
#else
#define nest_dbg(fmt, arg...)		do { } while(0)
#endif

extern uint32_t __freertos_irq_stack_bottom[];
/* Arrange from low to high */
static uint8_t hal_priority_set[HAL_IRQ_PRIO_MAX] = {0};

void clic_show_hal_priority_set(void)
{
	int i;

	nest_dbg("PRIORITY_INVALID_BIT_MASK: 0x%x\n", PRIORITY_INVALID_BIT_MASK);
	nest_dbg("CLICINTCTL_NLBITS_MASK: 0x%x\n", CLICINTCTL_NLBITS_MASK);
	nest_dbg("PREEMPTPRIORITY_VALID_SHIFT: %d\n", PREEMPTPRIORITY_VALID_SHIFT);
	nest_dbg("SUBPRIORITY_VALID_SHIFT: %d\n", SUBPRIORITY_VALID_SHIFT);
	nest_dbg("\n");
	nest_dbg("PREEMPTPRIORITY_REG_MAX: %d\n", PREEMPTPRIORITY_REG_MAX);
	nest_dbg("PREEMPTPRIORITY_REG_MIN: %d\n", PREEMPTPRIORITY_REG_MIN);
	nest_dbg("SUBPRIORITY_REG_MAX: %d\n", SUBPRIORITY_REG_MAX);
	nest_dbg("SUBPRIORITY_REG_MIN: %d\n", SUBPRIORITY_REG_MIN);
	nest_dbg("\n");
	printf("sum of preempt priority: %d\n", (0x1 << CLICINTCTL_NLBITS));
	printf("PREEMPTPRIORITY_MAX: %d\n", PREEMPTPRIORITY_MAX);
	printf("PREEMPTPRIORITY_MIN: %d\n", PREEMPTPRIORITY_MIN);
	printf("SUBPRIORITY_MAX: %d\n", SUBPRIORITY_MAX);
	printf("SUBPRIORITY_MIN: %d\n", SUBPRIORITY_MIN);

	for (i = 0; i < HAL_IRQ_PRIO_MAX; i++)
		printf("hal_priority(%d) = PREEMPTPRIORITY(%d)\n", i, hal_priority_set[i]);
}

static void clic_irqstack_overflow_check_init(StackType_t *addr, StackType_t size)
{
	StackType_t i;

	for (i = 0; i < (size / sizeof(StackType_t)); i++) {
		*(addr + i) = IRQSTACK_BOTTOM_MAGIC;
	}
}

static void clic_irq_priority_init(void)
{
	int i;
	StackType_t *overflow_check = __freertos_irq_stack_bottom;

	clic_irqstack_overflow_check_init(overflow_check, 1 * sizeof(StackType_t));

	if (HAL_IRQ_PRIO_MAX < 2) {
		nest_dbg("irq priority init error, there is only one priority. HAL_IRQ_PRIO_MAX: %d\n", HAL_IRQ_PRIO_MAX);
		return;
	}

	hal_priority_set[0] = PREEMPTPRIORITY_MIN;
	hal_priority_set[HAL_IRQ_PRIO_MAX - 1] = PREEMPTPRIORITY_MAX;

	/* different value of hal_irq_priority_t may lead to the same priority */
	for (i = 1; i < (HAL_IRQ_PRIO_MAX - 1); i++) {
		hal_priority_set[i] = PREEMPTPRIORITY_MIN + i;
		if (hal_priority_set[i] == PREEMPTPRIORITY_MAX)
			/* make sure there is only one TOP priority */
			hal_priority_set[i] = hal_priority_set[i - 1];
	}
}

static uint8_t clic_halprio_to_preemptprio(hal_irq_prio_t priority)
{
	return hal_priority_set[priority];
}

int clic_irq_set_priority(const struct irq_controller *ic, uint32_t irq_id, uint32_t preemptpriority, uint32_t subpriority)
{
	uint32_t reg_addr;
	uint8_t priority_val;

	if ((preemptpriority < PREEMPTPRIORITY_MIN) || (preemptpriority > PREEMPTPRIORITY_MAX)) {
		nest_dbg("preemptpriority %d invalid\n", preemptpriority);
		return -1;
	}

	if ((subpriority < SUBPRIORITY_MIN) || (subpriority > SUBPRIORITY_MAX)) {
		nest_dbg("subpriority %d invalid\n", subpriority);
		return -2;
	}

	reg_addr = ic->reg_base_addr + CLIC_INT_X_CTRL_REG_OFF(irq_id);
	priority_val = ic_readb(reg_addr) & PRIORITY_INVALID_BIT_MASK;
	priority_val |= (uint8_t)(preemptpriority << PREEMPTPRIORITY_VALID_SHIFT);
	priority_val |= (uint8_t)(subpriority << SUBPRIORITY_VALID_SHIFT);
	ic_writeb(priority_val, reg_addr);
	nest_dbg("set priority reg(0x%x) value(0x%x), preemptpriority(%d), subpriority(%d)\n", reg_addr, priority_val, preemptpriority, subpriority);

	return 0;
}

int clic_irq_get_priority(const struct irq_controller *ic, uint32_t irq_id, uint32_t *preemptpriority, uint32_t *subpriority)
{
	uint32_t reg_addr;
	uint8_t priority_val;

	if (!preemptpriority || !subpriority) {
		nest_dbg("param invalid, preemptpriority or subpriority is NULL\n");
		return -1;
	}

	reg_addr = ic->reg_base_addr + CLIC_INT_X_CTRL_REG_OFF(irq_id);
	priority_val = ic_readb(reg_addr);

	*subpriority =(uint32_t)(((priority_val | PRIORITY_INVALID_BIT_MASK) & CLICINTCTL_NLBITS_MASK) >> SUBPRIORITY_VALID_SHIFT);
	*preemptpriority =(uint32_t)((priority_val | CLICINTCTL_NLBITS_MASK) >> PREEMPTPRIORITY_VALID_SHIFT);
	nest_dbg("get priority reg(0x%x) value(0x%x), preemptpriority(%d), subpriority(%d)\n", reg_addr, priority_val, *preemptpriority, *subpriority);

	return 0;
}

int clic_irq_priority_map(hal_irqprio_map_t map, hal_irq_prio_t *priority, uint32_t *preemptpriority, uint32_t *subpriority)
{
	int i;

	if (!preemptpriority || !priority) {
		nest_dbg("invalid param NULL\n");
		return -1;
	}

	if (map == HAL_IRQ_PRIO2PREEMPT) {
		if (!hal_irq_prio_t_valid(*priority)) {
			nest_dbg("priority %d invalid\n", *priority);
			return -2;
		}

		*preemptpriority = (uint32_t)clic_halprio_to_preemptprio(*priority);
		/* subpriority of hal_prio fix to SUBPRIORITY_MIN */
		*subpriority = SUBPRIORITY_MIN;
		nest_dbg("priority_map: halprio(%d) to preemtprio(%d), subpriority(%d)\n", *priority, *preemptpriority, *subpriority);
	} else if (map == HAL_IRQ_PREEMPT2PRIO) {
		if ((*preemptpriority < PREEMPTPRIORITY_MIN) || (*preemptpriority > PREEMPTPRIORITY_MAX)) {
			nest_dbg("preemptpriority %d invalid\n", *preemptpriority);
			return -3;
		}

	       for (i = 0; i < HAL_IRQ_PRIO_MAX; i++) {
	               if (hal_priority_set[i] == *preemptpriority) {
	                       *priority = i;
				nest_dbg("priority_map: preemtprio(%d) to halprio(%d)\n", *preemptpriority, *priority);
				/* no subpriority map */
				return 0;
	               }
	       }

	       nest_dbg("hal_priority fail not find, preemtprio: %d\n", *preemptpriority);
	       return -4;
	} else {
		nest_dbg("invalid irqprio map param\n");
		return -5;
	}

	return 0;
}

#endif /* CONFIG_ARCH_RISCV_INTERRUPT_NEST */

irq_controller_ops_t g_rv_clic_ops =
{
	.init = clic_init,
	.irq_enable = clic_irq_enable,
	.irq_disable = clic_irq_disable,
	.irq_is_enabled = clic_irq_is_enabled,
	.irq_is_pending = clic_irq_is_pending,
	.irq_set_pending = clic_irq_set_pending,
	.irq_set_trigger_type = clic_irq_set_trigger_type,
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	.irq_priority_map = clic_irq_priority_map,
	.irq_set_priority = clic_irq_set_priority,
	.irq_get_priority = clic_irq_get_priority,
#endif
};

void clic_irq_handler_non_vec_mode(void)
{
	printf("ERROR: CLIC interrupt is not vector mode!\n");
}

#if defined(CONFIG_STANDBY) || defined(CONFIG_COMPONENTS_PM)
#include "pm_syscore.h"

/* following defines should be used for structure members */
#define     __IM     volatile const       /*! Defines 'read only' structure member permissions */
#define     __OM     volatile             /*! Defines 'write only' structure member permissions */
#define     __IOM    volatile             /*! Defines 'read / write' structure member permissions */
#define __STATIC_INLINE                        static inline

/**
  \brief Access to the structure of a vector interrupt controller.
 */
typedef struct {
    __IOM uint8_t IP;           /*!< Offset: 0x000 (R/W)  Interrupt set pending register */
    __IOM uint8_t IE;           /*!< Offset: 0x004 (R/W)  Interrupt set enable register */
    __IOM uint8_t ATTR;         /*!< Offset: 0x008 (R/W)  Interrupt set attribute register */
    __IOM uint8_t CTL;          /*!< Offset: 0x00C (R/W)  Interrupt control register */
} CLIC_INT_Control;

typedef struct {
    __IOM uint32_t CLICCFG:8;                 /*!< Offset: 0x000 (R/W)  CLIC configure register */
    __IM  uint32_t CLICINFO;
    __IOM uint32_t MINTTHRESH;
    uint32_t RESERVED[1021];
    CLIC_INT_Control CLICINT[4096];
} CLIC_Type;

#define CLIC_BASE           (0x30800000UL)                            /*!< CLIC Base Address */

#define CLIC                ((CLIC_Type    *)     CLIC_BASE   )       /*!< CLIC configuration struct */
#define CLIC_INTIP_IP_Pos                      0U                                    /*!< CLIC INTIP: IP Position */
#define CLIC_INTIP_IP_Msk                      (0x1UL << CLIC_INTIP_IP_Pos)          /*!< CLIC INTIP: IP Mask */

#define CLIC_INTIE_IE_Pos                      0U                                    /*!< CLIC INTIE: IE Position */
#define CLIC_INTIE_IE_Msk                      (0x1UL << CLIC_INTIE_IE_Pos)          /*!< CLIC INTIE: IE Mask */

#define Machine_Software_IRQn           (3)
#define CORET_IRQn                      (7)

/**
  \brief   Enable External Interrupt
  \details Enable a device-specific interrupt in the VIC interrupt controller.
  \param [in]      IRQn  External interrupt number. Value cannot be negative.
 */
__STATIC_INLINE void csi_vic_enable_irq(int32_t IRQn)
{
    CLIC->CLICINT[IRQn].IE |= CLIC_INTIE_IE_Msk;
}

/**
  \brief   Disable External Interrupt
  \details Disable a device-specific interrupt in the VIC interrupt controller.
  \param [in]      IRQn  External interrupt number. Value cannot be negative.
 */
__STATIC_INLINE void csi_vic_disable_irq(int32_t IRQn)
{
    CLIC->CLICINT[IRQn].IE &= ~CLIC_INTIE_IE_Msk;
}

/**
  \brief   Clear Pending Interrupt
  \details Clear the pending bit of an external interrupt.
  \param [in]      IRQn  External interrupt number. Value cannot be negative.
 */
__STATIC_INLINE void csi_vic_clear_pending_irq(int32_t IRQn)
{
    CLIC->CLICINT[IRQn].IP &= ~CLIC_INTIP_IP_Msk;
}

static int clic_suspend(void *data, suspend_mode_t mode)
{
	irq_controller_t *ic;
	uint32_t irq_id, reg_addr, reg_data;
	uint32_t *irq_status;

	ic = (irq_controller_t *)data;
	irq_status = ic->priv;

	for (irq_id = 0; irq_id < ic->irq_cnt; irq_id++)
	{
		reg_addr = ic->reg_base_addr + CLIC_INX_X_32BIT_REG_OFF(irq_id);
		reg_data = ic_readl(reg_addr);
		irq_status[irq_id] = reg_data;
	}
	csi_vic_disable_irq(CORET_IRQn);
	csi_vic_clear_pending_irq(CORET_IRQn);
	csi_vic_disable_irq(Machine_Software_IRQn);
	csi_vic_clear_pending_irq(Machine_Software_IRQn);
	return 0;
}

static void clic_resume(void *data, suspend_mode_t mode)
{
	irq_controller_t *ic;
	uint32_t irq_id, reg_addr;
	uint32_t *irq_status;

	ic = (irq_controller_t *)data;
	irq_status = ic->priv;

	for (irq_id = 0; irq_id < ic->irq_cnt; irq_id++)
	{
		reg_addr = ic->reg_base_addr + CLIC_INX_X_32BIT_REG_OFF(irq_id);
		ic_writel(irq_status[irq_id], reg_addr);
	}
}

int clic_pm_init(void *obj)
{
	int ret;
	irq_controller_t *ic;

	if (!obj)
		return -1;

	ic = (irq_controller_t *)obj;
	ic->pm_ops.name = "clic_pm";
	ic->pm_ops.suspend = clic_suspend;
	ic->pm_ops.resume = clic_resume;
	ic->pm_ops.data = obj;

	ret = pm_syscore_register(&ic->pm_ops);
	if (ret)
		printf("Warning: pm_devops_register for clic failed, ret: %d", ret);

	return ret;
}
#endif
