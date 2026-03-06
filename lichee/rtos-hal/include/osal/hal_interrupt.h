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
#ifndef SUNXI_HAL_INTERRUPT_H
#define SUNXI_HAL_INTERRUPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef enum hal_irqreturn {
	HAL_IRQ_OK		= (0 << 0),
	HAL_IRQ_ERR		= (1 << 0),
} hal_irqreturn_t;

typedef hal_irqreturn_t (*hal_irq_handler_t)(void *);

#if defined(CONFIG_ARCH_RISCV)
#if defined(CONFIG_ARCH_SUN20IW2P1)
#define MAKE_IRQn(major, sub)					(major)
#else
#define MAKE_IRQn(major, sub)					((major) * 100 + (sub))
#endif
#elif defined(CONFIG_ARCH_DSP)
#define MAKE_IRQn(major, sub)					((sub) * 200 + major)
#else
#define MAKE_IRQn(major, sub)					(major)
#endif

#define in_interrupt(...)       hal_interrupt_get_nest()
#define in_nmi(...)             (0)

typedef enum {
	HAL_IRQ_PRIO2PREEMPT = 0,
	HAL_IRQ_PREEMPT2PRIO,
} hal_irqprio_map_t;

typedef enum {
	HAL_IRQ_PRIO_LOW = 0,
	HAL_IRQ_PRIO_MIDDLE,
	HAL_IRQ_PRIO_HIGH,
	HAL_IRQ_PRIO_TOP,

	HAL_IRQ_PRIO_MAX,
	HAL_IRQ_PRIO_BASE = HAL_IRQ_PRIO_LOW,
} hal_irq_prio_t;

#define hal_irq_prio_t_valid(_t) \
	(((_t) >= HAL_IRQ_PRIO_BASE) && ((_t) < HAL_IRQ_PRIO_MAX))

void hal_interrupt_enable(void);
void hal_interrupt_disable(void);
unsigned long hal_interrupt_is_disable(void);
unsigned long hal_interrupt_disable_irqsave(void);
void hal_interrupt_enable_irqrestore(unsigned long flag);
uint32_t hal_interrupt_get_nest(void);

int32_t hal_request_irq(int32_t irq, hal_irq_handler_t handler, const char *name, void *data);
void hal_free_irq(int32_t irq);
int hal_enable_irq(int32_t irq);
void hal_disable_irq(int32_t irq);
int hal_get_irq_enable_state(int32_t irq_num, int *enabled);

void hal_irq_set_prioritygrouping(uint32_t group);
uint32_t hal_irq_get_prioritygrouping(void);
int hal_irq_set_specific_priority(int32_t irq, uint32_t preemptpriority, uint32_t subpriority);
int hal_irq_get_specific_priority(int32_t irq, uint32_t *p_preemptpriority, uint32_t *p_subpriority);
int hal_irq_set_priority(int32_t irq, hal_irq_prio_t priority);
int hal_irq_get_priority(int32_t irq, hal_irq_prio_t *priority);

void hal_nvic_irq_set_priority(int32_t irq, uint32_t priority);
uint32_t hal_nvic_irq_get_priority(int32_t irq);

void hal_interrupt_clear_pending(int32_t irq);
int32_t hal_interrupt_is_pending(int32_t irq);
void hal_interrupt_set_pending(int32_t irq);

void hal_interrupt_enter(void);
void hal_interrupt_leave(void);

#ifdef __cplusplus
}
#endif

#endif
