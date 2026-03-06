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
#ifndef SUNXI_HAL_EVENT_H
#define SUNXI_HAL_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include <aw_list.h>

#define HAL_EVENT_OPTION_CLEAR		(1 << 0)
#define HAL_EVENT_OPTION_AND		(1 << 1)
#define HAL_EVENT_OPTION_OR			(1 << 2)

typedef StaticEventGroup_t hal_event;
typedef EventGroupHandle_t hal_event_t;
typedef EventBits_t hal_event_bits_t;


#elif defined(CONFIG_RTTKERNEL)
#include <rtdef.h>
#include <rtthread.h>

#define HAL_EVENT_OPTION_CLEAR		RT_EVENT_FLAG_CLEAR
#define HAL_EVENT_OPTION_AND		RT_EVENT_FLAG_AND
#define HAL_EVENT_OPTION_OR         RT_EVENT_FLAG_OR

typedef rt_event_t hal_event_t;
typedef rt_uint32_t hal_event_bits_t;

#else
#error "can not support the RTOS!!"
#endif

int hal_event_init(hal_event_t ev);
int hal_event_datach(hal_event_t ev);

hal_event_t hal_event_create(void);
hal_event_t hal_event_create_initvalue(int init_value);
int hal_event_delete(hal_event_t ev);

/*
 * wait for events
 * @ev: hal_event_t handler
 * @evs: events
 * @option: it can be HAL_EVENT_OPTION_*
 * @timeout:wait time(ms)
 *
 * @return: return new events value if success,otherwise return negative value
 */
hal_event_bits_t hal_event_wait(hal_event_t ev, hal_event_bits_t evs, uint8_t option, unsigned long timeout);

#define hal_ev_wait_all(ev, evs, timeout) hal_event_wait(ev, evs, HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout)
#define hal_ev_wait_any(ev, evs, timeout) hal_event_wait(ev, evs, HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_OR, timeout)
#define hal_ev_wait_all_no_clear(ev, evs, timeout) hal_event_wait(ev, evs, HAL_EVENT_OPTION_AND, timeout)
#define hal_ev_wait_any_no_clear(ev, evs, timeout) hal_event_wait(ev, evs, HAL_EVENT_OPTION_OR, timeout)

/*
 * set event bit
 * @ev: hal_event_t handler
 * @evs: event bit to set
 *
 * @return: return 0 if success
 */
int hal_event_set_bits(hal_event_t ev, hal_event_bits_t evs);

/*
 * get event bit
 * @ev: hal_event_t handler
 *
 * @return: return events if success
 */
hal_event_bits_t hal_event_get(hal_event_t ev);

/*
 * clear events
 * @ev: hal_event_t handler
 * @evs: events
 *
 * @return: return old events value if success,otherwise return negative value
 */
hal_event_bits_t hal_event_clear_bits(hal_event_t ev, hal_event_bits_t evs);

#ifdef __cplusplus
}
#endif
#endif

