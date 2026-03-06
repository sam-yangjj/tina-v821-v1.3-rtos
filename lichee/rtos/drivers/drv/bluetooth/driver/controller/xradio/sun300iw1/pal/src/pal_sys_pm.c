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
/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      System PM implementation.
 *
 *  Copyright (c) 2020-2021 Xradio, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/
#include "pm_base.h"
#include "pm_devops.h"
#include "pm_task.h"
#include "pm_wakecnt.h"
#include "pm_wakelock.h"
#include "pm_wakesrc.h"
#include "pm_state.h"
#include "pm_subsys.h"
#include "irqs-sun300iw1.h"
#include "pal_dbg_io.h"
#include "pal_sys_pm.h"
#include "pal_irq.h"
#include "errno.h"

extern int btc_resume(pal_suspend_mode_t mode);
extern int btc_suspend_noirq(pal_suspend_mode_t mode);
extern int btc_resume_noirq(pal_suspend_mode_t mode);
static int pal_resume(struct pm_device *dev, suspend_mode_t mode);
static int pal_suspend_noirq(struct pm_device *dev, suspend_mode_t mode);
static int pal_resume_noirq(struct pm_device *dev, suspend_mode_t mode);

static struct pm_devops btc_pm_devops = {
	.suspend_noirq = pal_suspend_noirq,
	.resume_noirq = pal_resume_noirq,
};

static struct pm_device btc_pm_device = {
	.name = "btc",
	.ops = &btc_pm_devops,
};


struct wakelock btc_wakelock = {
	.name = "btc_wakelock",
	.ref = 0,
};

struct wakelock ltask_wakelock = {
	.name = "ltask_wakelock",
	.ref = 0,
};

struct wakelock hcitx_wakelock = {
	.name = "hcitx_wakelock",
	.ref = 0,
};

struct wakelock hcirx_wakelock = {
	.name = "hcirx_wakelock",
	.ref = 0,
};

struct wakelock btc_bb_wakelock = {
	.name = "btc_bb_wakelock",
	.ref = 0,
};

struct pm_wakesrc *btc_wakesrc = NULL;

static struct wakelock *PalPmGetWakelock(pal_wakelock_id_t wl)
{
	struct wakelock *twl = NULL;

	if (PAL_WAKELOCK_ID_MAX >= PAL_WAKELOCK_ID_MAX) {
		return NULL;
	}
	switch (wl) {
	case PAL_WAKELOCK_ID_BTC:
		twl = &btc_wakelock;
		break;
	case PAL_WAKELOCK_ID_LTASK:
		twl = &ltask_wakelock;
		break;
	case PAL_WAKELOCK_ID_HCITX:
		twl = &hcitx_wakelock;
		break;
	case PAL_WAKELOCK_ID_HCIRX:
		twl = &hcirx_wakelock;
		break;
	case PAL_WAKELOCK_ID_BTC_BB:
		twl = &btc_bb_wakelock;
		break;
	default:
		break;
	}
	return twl;
}

static int pal_resume(struct pm_device *dev, suspend_mode_t mode)
{
	btc_resume((suspend_mode_t)mode);
	return 0;
}

static int pal_suspend_noirq(struct pm_device *dev, suspend_mode_t mode)
{
	btc_suspend_noirq((suspend_mode_t)mode);
	return 0;
}

static int pal_resume_noirq(struct pm_device *dev, suspend_mode_t mode)
{
	btc_resume_noirq((suspend_mode_t)mode);
	return 0;
}

int32_t PalPmDevopsRegister(void)
{
#if 0
	if (btc_wakesrc == NULL) {
		btc_wakesrc = pm_wakesrc_register(BTC_SLPTMR_IRQn, "btc_slptmr", PM_WAKESRC_MIGHT_WAKEUP);
		pm_set_wakeirq(btc_wakesrc);
	}
	return pm_devops_register(&btc_pm_device);
#endif
	
	return 0;
}
int32_t PalPmDevopsUnregister(void)
{
#if 0
	if (btc_wakesrc) {
		pm_clear_wakeirq(btc_wakesrc);
		pm_wakesrc_unregister(btc_wakesrc);
		btc_wakesrc = NULL;
	}
	return pm_devops_unregister(&btc_pm_device);
#endif
	
	return 0;
}

int32_t PalPmWakelockAcquire(pal_wakelock_id_t wl)
{
#if 0
	struct wakelock *twl;

	twl = PalPmGetWakelock(wl);
	return pm_wakelocks_acquire(twl, PM_WL_TYPE_WAIT_ONCE, PAL_OS_WAIT_FOREVER);
#endif
	
	return 0;
}

int32_t PalPmWakelockRelease(pal_wakelock_id_t wl)
{
#if 0
	struct wakelock *twl;

	twl = PalPmGetWakelock(wl);
	return pm_wakelocks_release(twl);
#endif
	
	return 0;
}

int32_t PalPmWakelockGetRefer(pal_wakelock_id_t wl)
{
#if 0
	struct wakelock *twl;

	twl = PalPmGetWakelock(wl);
	if(twl == NULL) {
		return -EINVAL;
	}
	return (int32_t)twl->ref;
#endif
	
	return 0;
}

void PalPmWakesrcAcquire(pal_wakesrc_id_t ws)
{
#if 0
	switch (ws) {
	case PAL_WAKESRC_ID_SLPTMR:
		pm_stay_awake(btc_wakesrc);
		break;
	default:
		break ;
	}
#endif
}

void PalPmWakesrcReleaseSleepy(pal_wakesrc_id_t ws)
{
#if 0
	switch (ws) {
	case PAL_WAKESRC_ID_SLPTMR:
		pm_relax(btc_wakesrc, PM_RELAX_SLEEPY);
		break;
	default:
		break ;
	}
#endif
}
void PalPmWakesrcReleaseWakeup(pal_wakesrc_id_t ws)
{
#if 0
	switch (ws) {
	case PAL_WAKESRC_ID_SLPTMR:
		pm_wakecnt_inc(btc_wakesrc);
		break;
	default:
		break ;
	}
#endif
}

int32_t PalPmTaskRegister(pal_task_handle_t handle)
{
#if 0
	return pm_task_register(handle, PM_TASK_TYPE_BT);
#endif
	
	return 0;
}

int32_t PalPmTaskUnregister(pal_task_handle_t handle)
{
#if 0
	return pm_task_unregister(handle);
#endif
	
	return 0;
}

int32_t PalPmIsHostReady(void)
{
#if 0
	if (pm_subsys_check_in_status(PM_SUBSYS_STATUS_NORMAL) & (1 << PM_SUBSYS_ID_RISCV)) {
		return 1;
	}
	return 0;
#endif
	
	return 1;
}

int32_t PalPmGetInterval(pal_pm_interval_t *interval)
{
#if 0
	if(pm_moment_has_been_recorded(PM_MOMENT_IRQ_DISABLE) &&
	   pm_moment_has_been_recorded(PM_MOMENT_SUSPEND_NOIRQ_END) &&
	   pm_moment_has_been_recorded(PM_MOMENT_ENTER_WFI) &&
	   pm_moment_has_been_recorded(PM_MOMENT_EXIT_WFI) &&
	   pm_moment_has_been_recorded(PM_MOMENT_RESUME_NOIRQ_BEGIN) &&
	   pm_moment_has_been_recorded(PM_MOMENT_IRQ_ENABLE)) {
		interval->suspend_noirq_interval = pm_moment_interval_us(PM_MOMENT_IRQ_DISABLE, PM_MOMENT_SUSPEND_NOIRQ_END);
		interval->enter_standby_interval = pm_moment_interval_us(PM_MOMENT_SUSPEND_NOIRQ_END, PM_MOMENT_ENTER_WFI);
		interval->exit_standby_interval = pm_moment_interval_us(PM_MOMENT_EXIT_WFI, PM_MOMENT_RESUME_NOIRQ_BEGIN);
		interval->resume_noirq_interval = pm_moment_interval_us(PM_MOMENT_RESUME_NOIRQ_BEGIN, PM_MOMENT_IRQ_ENABLE);
		return 1;
	}
	return 0;
#endif
	
	return 0;
}

void PalPmClearInterval(void)
{
	(void)PalPmGetWakelock;
	(void)btc_pm_device;
	(void)pal_resume;
#if 0
	pm_moment_clear(0xFFFFFFFF);
#endif
}
