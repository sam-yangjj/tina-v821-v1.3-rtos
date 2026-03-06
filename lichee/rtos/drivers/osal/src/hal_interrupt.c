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
#include <stdio.h>
#include <interrupt.h>
#include <hal_interrupt.h>
#include <hal_status.h>
#include <port_misc.h>
#include <compiler.h>

#include <task.h>

int32_t arch_request_irq(int32_t irq, hal_irq_handler_t handler, void *data);
void arch_free_irq(int32_t irq);
void arch_enable_irq(int32_t irq);
void arch_disable_irq(int32_t irq);
void arch_irq_set_prioritygrouping(uint32_t group);
uint32_t arch_irq_get_prioritygrouping(void);
int arch_irq_priority_map(int32_t irq, hal_irqprio_map_t map, hal_irq_prio_t *priority, uint32_t *preemptpriority, uint32_t *subpriority);
int arch_irq_set_priority(int32_t irq, uint32_t preemptpriority, uint32_t subpriority);
int arch_irq_get_priority(int32_t irq, uint32_t *p_preemptpriority, uint32_t *p_subpriority);
void arch_nvic_irq_set_priority(int32_t irq, uint32_t priority);
uint32_t arch_nvic_irq_get_priority(int32_t irq);
void arch_enable_all_irq(void);
void arch_disable_all_irq(void);
unsigned long arch_irq_is_disable(void);
unsigned long xport_interrupt_disable(void);
void xport_interrupt_enable(unsigned long flags);

__attribute__((weak))
void irq_init(void)
{
}

__attribute__((weak))
void arch_irq_set_prioritygrouping(uint32_t group)
{
	return;
}

__attribute__((weak))
uint32_t arch_irq_get_prioritygrouping(void)
{
	return 0;
}

__attribute__((weak))
int arch_irq_priority_map(int32_t irq, hal_irqprio_map_t map, hal_irq_prio_t *priority, uint32_t *preemptpriority, uint32_t *subpriority)
{
	return 0;
}

__attribute__((weak))
int arch_irq_set_priority(int32_t irq, uint32_t preemptpriority, uint32_t subpriority)
{
	return 0;
}

__attribute__((weak))
int32_t arch_irq_get_priority(int32_t irq, uint32_t *p_preemptpriority, uint32_t *p_subpriority)
{
	return 0;
}

__attribute__((weak))
void arch_nvic_irq_set_priority(int32_t irq, uint32_t priority)
{
	return;
}

__attribute__((weak))
uint32_t arch_nvic_irq_get_priority(int32_t irq)
{
	return 0;
}

__attribute__((weak))
void arch_clear_pending(int32_t irq)
{
}

__attribute__((weak))
int arch_is_pending(int32_t irq)
{
	return 0;
}

__attribute__((weak))
void arch_set_pending(int32_t irq)
{
}

__attribute__((weak))
int irq_core_get_irq_enable_state(uint32_t irq_num, int *enabled)
{
	if (enabled)
		*enabled = 0;

	return -1;
}

__nonxip_text
void hal_interrupt_clear_pending(int32_t irq)
{
	arch_clear_pending(irq);
}

__nonxip_text
int32_t hal_interrupt_is_pending(int32_t irq)
{
	return arch_is_pending(irq);
}

__nonxip_text
void hal_interrupt_set_pending(int32_t irq)
{
	arch_set_pending(irq);
}

int32_t hal_request_irq(int32_t irq, hal_irq_handler_t handler, const char *name, void *data)
{
	return arch_request_irq(irq, handler, data);
}

void hal_free_irq(int32_t irq)
{
	arch_free_irq(irq);
}

__nonxip_text
int hal_enable_irq(int32_t irq)
{
	arch_enable_irq(irq);

	return HAL_OK;
}

__nonxip_text
void hal_disable_irq(int32_t irq)
{
	arch_disable_irq(irq);
}

int hal_get_irq_enable_state(int32_t irq_num, int *enabled)
{
	return irq_core_get_irq_enable_state(irq_num, enabled);
}

void hal_irq_set_prioritygrouping(uint32_t group)
{
	arch_irq_set_prioritygrouping(group);
}

uint32_t hal_irq_get_prioritygrouping(void)
{
	return arch_irq_get_prioritygrouping();
}

int hal_irq_set_specific_priority(int32_t irq, uint32_t preemptpriority, uint32_t subpriority)
{
	return arch_irq_set_priority(irq, preemptpriority, subpriority);
}

int hal_irq_get_specific_priority(int32_t irq, uint32_t *p_preemptpriority, uint32_t *p_subpriority)
{
	return arch_irq_get_priority(irq, p_preemptpriority, p_subpriority);
}

int hal_irq_set_priority(int32_t irq, hal_irq_prio_t priority)
{
	int ret;
	uint32_t preemptpriority;
	uint32_t subpriority;

	ret = arch_irq_priority_map(irq, HAL_IRQ_PRIO2PREEMPT, &priority, &preemptpriority, &subpriority);
	if (ret)
		return ret;

	return arch_irq_set_priority(irq, preemptpriority, subpriority);
}

int hal_irq_get_priority(int32_t irq, hal_irq_prio_t *priority)
{
	int ret;
	uint32_t preemptpriority;
	uint32_t subpriority;

	ret = arch_irq_get_priority(irq, &preemptpriority, &subpriority);
	if (ret)
		return ret;

	return arch_irq_priority_map(irq, HAL_IRQ_PREEMPT2PRIO, priority, &preemptpriority, &subpriority);
}

void hal_nvic_irq_set_priority(int32_t irq, uint32_t priority)
{
    return arch_nvic_irq_set_priority(irq, priority);
}

uint32_t hal_nvic_irq_get_priority(int32_t irq)
{
    return arch_nvic_irq_get_priority(irq);
}

__attribute__((section (".sram_text"), no_instrument_function))
uint32_t hal_interrupt_get_nest(void)
{
	return uGetInterruptNest();
}

__nonxip_text
void hal_interrupt_enable(void)
{
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskEXIT_CRITICAL();
#else
        taskEXIT_CRITICAL_FROM_ISR(0);
#endif
    } else {
        taskEXIT_CRITICAL_FROM_ISR(0);
    }
}

__nonxip_text
void hal_interrupt_disable(void)
{
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskENTER_CRITICAL();
#else
        taskENTER_CRITICAL_FROM_ISR();
#endif
    } else {
        taskENTER_CRITICAL_FROM_ISR();
    }
}

__nonxip_text
unsigned long hal_interrupt_disable_irqsave(void)
{
    unsigned long flag = 0;
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskENTER_CRITICAL();
#else
        taskENTER_CRITICAL(flag);
#endif
    } else {
        flag = taskENTER_CRITICAL_FROM_ISR();
    }
    return flag;
}

__nonxip_text
void hal_interrupt_enable_irqrestore(unsigned long flag)
{
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskEXIT_CRITICAL();
#else
        taskEXIT_CRITICAL(flag);
#endif
    } else {
        taskEXIT_CRITICAL_FROM_ISR(flag);
    }
}

__nonxip_text
unsigned long hal_interrupt_save(void)
{
    unsigned long flag = 0;
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskENTER_CRITICAL();
#else
        taskENTER_CRITICAL(flag);
#endif
    } else {
        flag = taskENTER_CRITICAL_FROM_ISR();
    }
    return flag;
}

__nonxip_text
void hal_interrupt_restore(unsigned long flag)
{
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskEXIT_CRITICAL();
#else
        taskEXIT_CRITICAL(flag);
#endif
    } else {
        taskEXIT_CRITICAL_FROM_ISR(flag);
    }
}

__nonxip_text
unsigned long hal_interrupt_is_disable(void)
{
    return arch_irq_is_disable();
}

void hal_interrupt_init(void)
{
	irq_init();
}
