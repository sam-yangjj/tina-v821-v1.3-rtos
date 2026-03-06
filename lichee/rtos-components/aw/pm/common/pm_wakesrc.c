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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <hal_mem.h>
#include <hal_time.h>
#include <hal_sem.h>
#include <pm_debug.h>
#include <pm_wakesrc.h>
#include <hal_interrupt.h>

#ifdef CONFIG_COMPONENTS_PM
#define wakesrc_containerof(ptr_module) \
        __containerof(ptr_module, pm_wakesrc_t, node)

static struct list_head ws_list = LIST_HEAD_INIT(ws_list);
static hal_sem_t ws_sem = NULL;
static uint32_t ws_inpr = 0;
static uint32_t ws_inpr_intr = 0;
static uint32_t ws_cnt = 0;
static uint32_t ws_save_cnt = 0;
static int ws_irq = PM_WAKESRC_IRQ_NONE;

int32_t pm_wakesrc_get_irq(void)
{
	return ws_irq;
}

void pm_wakesrc_clr_irq(void)
{
	ws_irq = PM_WAKESRC_IRQ_NONE;
}

pm_wakesrc_t *pm_wakesrc_register(const int irq, const char *name)
{
	unsigned long irq_flag;
	pm_wakesrc_t *ws = NULL;

	irq_flag = hal_interrupt_disable_irqsave();
	if(name == NULL || strlen(name) >= PM_WAKESRC_NAME_LENTH) {
		goto exit;
	}
	ws = (pm_wakesrc_t *)hal_malloc(sizeof(*ws));
	if(ws == NULL) {
		goto exit;
	}
	ws->irq = irq;
	ws->active = 0;
	ws->mode = PM_WAKESRC_MODE_NORMAL;
	strcpy(ws->name, name);
	list_add_tail(&ws->node, &ws_list);
exit:
	hal_interrupt_enable_irqrestore(irq_flag);
	return ws;
}

int pm_wakesrc_unregister(pm_wakesrc_t *ws)
{
	unsigned long irq_flag;
	int ret = 0;

	irq_flag = hal_interrupt_disable_irqsave();
	if (ws == NULL) {
		ret = -EINVAL;
		goto exit;
	}
	if (ws->active) {
		ret = -EBUSY;
		goto exit;
	}
	list_del(&ws->node);
	ws->irq = -1;
	ws->active = 0;
	memset(ws->name, 0, sizeof(char) * PM_WAKESRC_NAME_LENTH);
	hal_free(ws);
	ws = NULL;
exit:
	hal_interrupt_enable_irqrestore(irq_flag);
	return ret;
}

void pm_wakesrc_set_mode(pm_wakesrc_t *ws, pm_wakesrc_mode_t mode)
{
	unsigned long irq_flag;

	irq_flag = hal_interrupt_disable_irqsave();
	ws->mode = mode;
	hal_interrupt_enable_irqrestore(irq_flag);
}

void pm_wakesrc_stay_awake(pm_wakesrc_t *ws)
{
	unsigned long irq_flag;

	irq_flag = hal_interrupt_disable_irqsave();
	if (ws == NULL) {
		goto exit;
	}
	if(!ws->active) {
		ws->active = 1;
		if (ws->mode == PM_WAKESRC_MODE_NORMAL) {
			ws_inpr++;
		} else {
			ws_inpr_intr++;
		}
	}
exit:
	hal_interrupt_enable_irqrestore(irq_flag);
}

void pm_wakesrc_relax(pm_wakesrc_t *ws, pm_relax_type_t type)
{
	unsigned long irq_flag;

	irq_flag = hal_interrupt_disable_irqsave();
	if (ws == NULL) {
		goto exit;
	}
	if (ws->active) {
		ws->active = 0;
		if (ws->mode == PM_WAKESRC_MODE_NORMAL) {
			ws_inpr--;
		} else {
			ws_inpr_intr--;
		}
	}
	if (type == PM_RELAX_WAKEUP) {
		ws_cnt++;
	}
	if (ws_irq == PM_WAKESRC_IRQ_NONE) {
		ws_irq = ws->irq;
	}
	hal_sem_post(ws_sem);
exit:
	hal_interrupt_enable_irqrestore(irq_flag);
}

void pm_wakesrc_get_inpr_cnt(uint32_t *cnt, uint32_t *inpr, uint32_t *inpr_intr)
{
	unsigned long irq_flag;

	irq_flag = hal_interrupt_disable_irqsave();
	*cnt = ws_cnt;
	*inpr = ws_inpr;
	*inpr_intr = ws_inpr_intr;
	hal_interrupt_enable_irqrestore(irq_flag);
}

int pm_wakesrc_save_cnt(uint32_t cnt)
{
	unsigned long irq_flag;
	uint32_t t_cnt;
	uint32_t t_inpr;
	uint32_t t_inpr_intr;
	int ret = 0;

	irq_flag = hal_interrupt_disable_irqsave();
	pm_wakesrc_get_inpr_cnt(&t_cnt, &t_inpr, &t_inpr_intr);
	if (t_cnt == cnt && !t_inpr) {
		ws_save_cnt = cnt;
		ret = 1;
	}
	hal_interrupt_enable_irqrestore(irq_flag);
	return ret;
}

int pm_wakesrc_cnt_changed(uint32_t cnt)
{
	unsigned long irq_flag;
	int ret = 0;

	irq_flag = hal_interrupt_disable_irqsave();
	if (cnt == ws_save_cnt) {
		goto exit;
	}
	ret = 1;
exit:
	hal_interrupt_enable_irqrestore(irq_flag);
	return ret;
}

int pm_wakesrc_wait_inpr(uint32_t *cnt, uint32_t timeout)
{
	uint32_t t_cnt;
	uint32_t t_inpr;
	uint32_t t_inpr_intr;
	uint32_t tick = hal_tick_get();

	while (1) {
		pm_wakesrc_get_inpr_cnt(&t_cnt, &t_inpr, &t_inpr_intr);
		if(t_inpr && (hal_tick_get() - tick <= timeout)) {
			hal_sem_timedwait(ws_sem, 2);
			continue;
		}
		*cnt = t_cnt;
		break;
	}
	return !t_inpr;
}

void pm_wakesrc_showall(void)
{
	struct list_head *list = &ws_list;
	struct list_head *pos = NULL;
	__maybe_unused pm_wakesrc_t *ws = NULL;

	pm_log("wakesrc showall\n");
	list_for_each(pos, list) {
		ws = wakesrc_containerof(pos);
		pm_log("name: %s\n", ws->name);
		pm_log("active: %d\n", ws->active);
	}
}

int pm_wakesrc_init(void)
{
	BaseType_t ret = 0;

	if (ws_sem) {
		ret = -EPERM;
		goto exit;
	}
	ws_sem = hal_sem_create(0);
	if (ws_sem == NULL) {
		ret = -ENOMEM;
		goto exit;
	}
exit:
	return ret;
}

int pm_wakesrc_deinit(void)
{
	if (ws_sem) {
		hal_sem_delete(ws_sem);
		ws_sem = NULL;
	}
	return 0;
}
#endif

#ifdef CONFIG_ARCH_SUN20IW2
/* restore pm API for compile check in old platform */
int pm_set_wakeirq(const int irq) { return 0; }
int pm_clear_wakeirq(const int irq) { return 0; }
void pm_wakecnt_inc(int32_t irq) {}
void pm_stay_awake(int irq) {}
int pm_wakecnt_init(void) { return 0; }
int pm_report_subsys_action(int subsys_id, int action) { return 0; }
void pm_relax(int32_t irq, pm_relax_type_t wakeup) {}
int pm_init(int argc, char **argv) { return 0; }

int rpc_pm_subsys_soft_wakeup(int affinity, int irq, int action) { return 0; }
int rpc_pm_trigger_suspend(int mode) { return 0; }
int rpc_pm_set_wakesrc(int wakesrc_id, int core, int status) { return 0; }
int rpc_pm_report_subsys_action(int subsys_id, int action) { return 0; }
#endif
