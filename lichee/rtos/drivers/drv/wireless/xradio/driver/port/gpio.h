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
#ifndef _XR_GPIO_H_
#define _XR_GPIO_H_

#include "../port/xr_types.h"

#if defined(__MELIS__)

#include "epdk.h"
int gpio_direction_output(unsigned gpio, int value);

void xr_wlan_irq_enable(void);
void xr_wlan_irq_subscribe(void *func);
void xr_wlan_irq_unsubscribe(void *func);
void xr_wlan_irq_clean(void);

#elif (defined(CONFIG_ARCH_STM32))

#include "gpio_api.h"
#define gpio_direction_output gpio_write
static XR_INLINE void xr_wlan_irq_disable(void) { }
//extern void xradio_gpio_irq(void);
static XR_INLINE void xr_wlan_irq_enable_clr(void) { }

void xr_wlan_irq_enable(void);
void xr_wlan_irq_subscribe(void *func);
void xr_wlan_irq_unsubscribe(void *func);
void xr_wlan_irq_clean(void);

#elif (defined(CONFIG_CHIP_XRADIO))

#include "driver/chip/chip.h"
#include "driver/chip/drivers/hal_nvic.h"
#include <stdio.h>

static XR_INLINE void xr_wlan_irq_subscribe(void *func)
{
	HAL_NVIC_SetIRQHandler(WIFIC_IRQn, (NVIC_IRQHandler)func);
	HAL_NVIC_SetPriority(WIFIC_IRQn, NVIC_PERIPH_PRIO_DEFAULT);
	HAL_NVIC_EnableIRQ(WIFIC_IRQn);
}

static XR_INLINE void xr_wlan_irq_clean(void)
{
	HAL_NVIC_ClearPendingIRQ(WIFIC_IRQn);
}

static XR_INLINE void xr_wlan_irq_enable(void)
{
	HAL_NVIC_EnableIRQ(WIFIC_IRQn);
}

static XR_INLINE void xr_wlan_irq_enable_clr(void)
{
	HAL_NVIC_ClearPendingIRQ(WIFIC_IRQn);
	HAL_NVIC_EnableIRQ(WIFIC_IRQn);
}

static __always_inline void xr_wlan_irq_disable(void)
{
	HAL_NVIC_DisableIRQ(WIFIC_IRQn);
}

static XR_INLINE void xr_wlan_irq_unsubscribe(void *func)
{
	HAL_NVIC_DisableIRQ(WIFIC_IRQn);
}

#elif (defined(CONFIG_OS_RTTHREAD))

void xr_wlan_irq_disable(void);

void xr_wlan_irq_enable(void);
void xr_wlan_irq_subscribe(void *func);
void xr_wlan_irq_unsubscribe(void *func);


#elif (defined(CONFIG_OS_YUNOS) || defined(CONFIG_OS_NUTTX))
#include <hal_gpio.h>
#include "kernel/os/os_time.h"

typedef enum {
	HAL_IRQ_TYPE_NONE			= 0x00000000,
	HAL_IRQ_TYPE_EDGE_RISING	= 0x00000001,
	HAL_IRQ_TYPE_EDGE_FALLING	= 0x00000002,
	HAL_IRQ_TYPE_EDGE_BOTH		= (HAL_IRQ_TYPE_EDGE_FALLING | HAL_IRQ_TYPE_EDGE_RISING),
	HAL_IRQ_TYPE_LEVEL_HIGH 	= 0x00000004,
	HAL_IRQ_TYPE_LEVEL_LOW		= 0x00000008,
}hal_gpio_interrupt_mode_t;

void xr_wlan_irq_disable(void);
void xr_wlan_irq_enable(void);
void xr_wlan_irq_enable_clr(void);
void xr_wlan_irq_subscribe(void *func);
void xr_wlan_irq_unsubscribe(void *func);
int  xradio_wlan_gpio_power(int on);

#elif defined(CONFIG_OS_TINA)
#include "FreeRTOS.h"
#include "portmacro.h"
#include "irqs.h"

#ifdef CONFIG_ARCH_ARM_ARMV8M
#define NVIC_WLAN_PRIO CONFIG_ARCH_ARM_ARMV8M_IRQ_DEFAULT_PRIORITY
#endif

void xr_wlan_irq_subscribe(void *func);
void xr_wlan_irq_enable(void);
void xr_wlan_irq_enable_clr(void);
void xr_wlan_irq_disable(void);
void xr_wlan_irq_unsubscribe(void *func);

#endif /* __MELIS__ */

extern void xradio_gpio_irq(void);

#endif /* _XR_GPIO_H_ */
