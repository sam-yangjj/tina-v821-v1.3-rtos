/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *	  1. Redistributions of source code must retain the above copyright
 *		 notice, this list of conditions and the following disclaimer.
 *	  2. Redistributions in binary form must reproduce the above copyright
 *		 notice, this list of conditions and the following disclaimer in the
 *		 documentation and/or other materials provided with the
 *		 distribution.
 *	  3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *		 its contributors may be used to endorse or promote products derived
 *		 from this software without specific prior written permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *	OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <hal_mutex.h>
#include <hal_timer.h>
#include <pm_debug.h>
#include <pm_wakelock.h>
#include <pm_wakesrc.h>
#include <hal_interrupt.h>

#ifdef CONFIG_COMPONENTS_PM

static struct list_head wl_list = LIST_HEAD_INIT(wl_list);
#define wakelock_containerof(ptr_module) \
        __containerof(ptr_module, struct wakelock, node)
static struct list_head wl_timer_list = LIST_HEAD_INIT(wl_timer_list);
#define timer_containerof(ptr_module) \
        __containerof(ptr_module, struct wakelock, timer_node)
#define time_after(a,b)    ((long)(b) - (long)(a) < 0)
#define time_before(a,b)   time_after(b,a)

static osal_timer_t wl_timer = NULL;
static uint32_t min_wl_expired = OS_WAIT_FOREVER;
static pm_wakesrc_t *wl_ws = NULL;
static uint32_t wl_cnt = 0;
static pm_wakesrc_t *wl_ws_int = NULL;
static uint32_t wl_cnt_int = 0;
static uint32_t ws_last_tick = 0;

static void pm_wakelock_cnt_inc(struct wakelock *wl)
{
	if (wl->ws_mode == PM_WAKESRC_MODE_NORMAL) {
		wl_cnt++;
		if (wl_cnt == 1) {
			pm_wakesrc_stay_awake(wl->ws);
		}
	} else if (wl->ws_mode == PM_WAKESRC_MODE_INTERNAL) {
		wl_cnt_int++;
		if (wl_cnt_int == 1) {
			pm_wakesrc_stay_awake(wl->ws);
		}
	}
}

static void pm_wakelock_cnt_dec(struct wakelock *wl)
{
	if (wl->ws_mode == PM_WAKESRC_MODE_NORMAL) {
		wl_cnt--;
		if (wl_cnt == 0) {
			pm_wakesrc_relax(wl->ws, PM_RELAX_SLEEPY);
		}
	} else if (wl->ws_mode == PM_WAKESRC_MODE_INTERNAL) {
		wl_cnt_int--;
		if (wl_cnt_int == 0) {
			pm_wakesrc_relax(wl->ws, PM_RELAX_SLEEPY);
		}
	}
}

static int pm_wakesrc_time_before(struct wakelock *wl_a, struct wakelock *wl_b)
{
	if (wl_a->wrap > wl_b->wrap) {
		return 0;
	}
	if (wl_a->wrap < wl_b->wrap) {
		return 1;
	}
	if (wl_a->expired >= wl_b->expired) {
		return 0;
	}
	return 1;
}

static void pm_wakelock_handle_time_wrap(void)
{
	struct list_head *list = &wl_timer_list;
	struct list_head *pos = NULL;
	struct list_head *save_node = NULL;
	struct wakelock *t_wl = NULL;
	uint32_t cur_tick = hal_tick_get();

	if (ws_last_tick > cur_tick) {
		list_for_each_safe(pos, save_node, list) {
			t_wl = timer_containerof(pos);
			if (!t_wl->wrap) {
				t_wl->expired = OS_WAIT_FOREVER;
				t_wl->wrap = 0;
				t_wl->ref--;
				if(!t_wl->ref) {
					pm_wakelock_cnt_dec(t_wl);
				}
				list_del(&t_wl->timer_node);
			} else {
				t_wl->wrap--;
			}
		}
	}
	ws_last_tick = cur_tick;
}

static void pm_wakelock_update_timer(void)
{
	struct list_head *list = &wl_timer_list;
	struct list_head *pos = NULL;
	struct list_head *save_node = NULL;
	struct wakelock *t_wl = NULL;
	uint32_t tick = 0;
	uint32_t cur_tick = hal_tick_get();

	/* handle timeout wakelock node */
	list_for_each_safe(pos, save_node, list) {
		t_wl = timer_containerof(pos);
		if (t_wl->wrap || t_wl->expired > cur_tick) {
			break;
		}
		t_wl->expired = OS_WAIT_FOREVER;
		t_wl->wrap = 0;
		t_wl->ref--;
		if(!t_wl->ref) {
			pm_wakelock_cnt_dec(t_wl);
		}
		list_del(&t_wl->timer_node);
	}

	if (list_empty(list)) {
		min_wl_expired = OS_WAIT_FOREVER;
		osal_timer_stop(wl_timer);
	} else if (t_wl->expired != min_wl_expired) {
		min_wl_expired = t_wl->expired;
		tick = (BaseType_t)min_wl_expired - cur_tick;
		osal_timer_control(wl_timer, OSAL_TIMER_CTRL_SET_TIME, &tick);
	}
}

static int pm_wakelock_insert_timer_list(struct wakelock *wl)
{
	struct list_head *list = &wl_timer_list;
	struct list_head *pos = NULL;
	struct wakelock *t_wl = NULL;

	if(wl == NULL) {
		return -EINVAL;
	}

	list_del(&wl->timer_node);
	pm_wakelock_handle_time_wrap();
	if (wl->type == PM_WL_TYPE_WAIT_TIMEOUT) {
		list_for_each(pos, list) {
			t_wl = timer_containerof(pos);
			if (pm_wakesrc_time_before(wl, t_wl)) {
				break;
			}
		}
		list_add_tail(&wl->timer_node, pos);
	}
	pm_wakelock_update_timer();

	return 0;
}

static int pm_wakelock_remove_timer_list(struct wakelock *wl)
{
	if(wl == NULL) {
	return -EINVAL;
	}

	list_del(&wl->timer_node);
	pm_wakelock_handle_time_wrap();
	pm_wakelock_update_timer();
	return 0;
}

static void pm_wakelock_cb(void *p)
{
	unsigned long irq_flag;

	irq_flag = hal_interrupt_disable_irqsave();
	pm_wakelock_handle_time_wrap();
	pm_wakelock_update_timer();
	hal_interrupt_enable_irqrestore(irq_flag);
}

int pm_wakelocks_acquire(struct wakelock *wl, enum pm_wakelock_t type, uint32_t mesc)
{
	unsigned long irq_flag;
	uint32_t cur_tick;

	if(wl == NULL || mesc == 0) {
		return -EINVAL;
	}
	if((type == PM_WL_TYPE_WAIT_TIMEOUT && mesc == OS_WAIT_FOREVER) ||
	   (type != PM_WL_TYPE_WAIT_TIMEOUT && mesc != OS_WAIT_FOREVER)) {
		return -EINVAL;
	}

	if (wl_ws == NULL) {
		pm_err("wakelock ws is NULL\n");
		return -EINVAL;
	}

	irq_flag = hal_interrupt_disable_irqsave();
	if(wl->ws == NULL) {
		if (wl->ws_mode == PM_WAKESRC_MODE_NORMAL) {
			wl->ws = wl_ws;
		} else if (wl->ws_mode == PM_WAKESRC_MODE_INTERNAL) {
			wl->ws = wl_ws_int;
		}
		list_add_tail(&wl->node, &wl_list);
	}

	wl->type = type;
	if(!wl->ref) {
		wl->ref++;
		pm_wakelock_cnt_inc(wl);
	} else {
		if (wl->type == PM_WL_TYPE_WAIT_INC) {
			wl->ref++;
		}
	}

	cur_tick = hal_tick_get();
	if (wl->type == PM_WL_TYPE_WAIT_TIMEOUT) {
		wl->expired = cur_tick + mesc;
		/*
		 * wrap is 0 means wakelock is the current peroid
		 * wrap is 1 means wakelock is the next peroid
		 * if the insert wakelock expired time is wrap, the wrap shold be 1
		 */
		wl->wrap = 0;
		if (wl->expired < cur_tick) {
			wl->wrap = 1;
		}
	} else {
		wl->expired = OS_WAIT_FOREVER;
		wl->wrap = 0;
	}
	pm_wakelock_insert_timer_list(wl);
	hal_interrupt_enable_irqrestore(irq_flag);
	return 0;
}

int pm_wakelocks_release(struct wakelock *wl)
{
	unsigned long irq_flag;

	if(wl == NULL || wl->ws == NULL) {
		return -EINVAL;
	}
	if (wl_ws == NULL) {
		pm_err("wakelock ws is NULL\n");
		return -EINVAL;
	}

	irq_flag = hal_interrupt_disable_irqsave();
	if(wl->ref) {
		wl->ref--;
		if(!wl->ref) {
			pm_wakelock_cnt_dec(wl);
			wl->expired = OS_WAIT_FOREVER;
			wl->wrap = 0;
			pm_wakelock_remove_timer_list(wl);
		}
	}
	hal_interrupt_enable_irqrestore(irq_flag);
	return 0;
}

void pm_wakelocks_setname(struct wakelock *wl, const char *name)
{
	unsigned long irq_flag;

	irq_flag = hal_interrupt_disable_irqsave();
	if (name) {
		wl->name = name;
	}
	hal_interrupt_enable_irqrestore(irq_flag);
}

void pm_wakelock_break_link(void)
{
	unsigned long irq_flag;
	struct list_head *list = &wl_list;
	struct list_head *pos = NULL;
	struct list_head *save_node = NULL;
	struct wakelock *t_wl = NULL;

	irq_flag = hal_interrupt_disable_irqsave();
	list_for_each_safe(pos, save_node, list) {
		t_wl = wakelock_containerof(pos);
		t_wl->ws = NULL;
		list_del(&t_wl->node);
	}
	hal_interrupt_enable_irqrestore(irq_flag);
}

void pm_show_wakelocks(uint8_t active)
{
	struct list_head *list = &wl_list;
	struct list_head *pos = NULL;
	struct wakelock *t_wl = NULL;

	pm_log("##wakelock show all##\n");
	pm_log("wl_cnt: %u\n", wl_cnt);
	pm_log("wl_cnt_int: %u\n", wl_cnt_int);
	list_for_each(pos, list) {
		t_wl = wakelock_containerof(pos);
		pm_log("name: %s\n", t_wl->name);
		pm_log("type: %u\n", t_wl->type);
		pm_log("ref: %u\n", t_wl->ref);
		pm_log("wrap: %u\n", t_wl->wrap);
		pm_log("expired: %u\n", t_wl->expired);
		pm_log("ws_mode: %u\n", t_wl->ws_mode);
	}
}

int pm_wakelocks_init(void)
{
	int ret = 0;

	wl_timer = osal_timer_create("",
						         (timeout_func)pm_wakelock_cb,
						         NULL,
						         MS_TO_OSTICK(10),
						         OSAL_TIMER_FLAG_ONE_SHOT);
	if (wl_timer == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	wl_ws_int = pm_wakesrc_register(PM_WAKESRC_IRQ_SOFT, "wl_ws_int");
	if (wl_ws_int == NULL) {
		ret = -ENOMEM;
		goto erro_wakesrc_int;
	}
	pm_wakesrc_set_mode(wl_ws_int , PM_WAKESRC_MODE_INTERNAL);

	wl_ws = pm_wakesrc_register(PM_WAKESRC_IRQ_SOFT, "wl_ws");
	if (wl_ws == NULL) {
		ret = -ENOMEM;
		goto erro_wakesrc;
	}
	goto exit;


erro_wakesrc:
	pm_wakesrc_unregister(wl_ws_int);
	wl_ws_int = NULL;

erro_wakesrc_int:
	osal_timer_delete(wl_timer);
	wl_timer = NULL;
exit:
	return ret;
}

int pm_wakelocks_deinit(void)
{
	if (wl_ws_int) {
		pm_wakesrc_unregister(wl_ws_int);
		wl_ws_int = NULL;
	}
	if (wl_ws) {
		pm_wakesrc_unregister(wl_ws);
		wl_ws = NULL;
	}
	if (wl_timer) {
		osal_timer_stop(wl_timer);
		osal_timer_delete(wl_timer);
		wl_timer = NULL;
	}
	return 0;
}

#endif

