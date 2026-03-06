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

#ifndef _PM_STATE_H_
#define _PM_STATE_H_

#include "hal/aw_list.h"

typedef enum {
	PM_STATUS_RUNNING = 0,
	PM_STATUS_PRESLEEP,
	PM_STATUS_SLEEPING,
	PM_STATUS_SLEEPED,
	PM_STATUS_ACTIVING,
	PM_STATUS_AGAINING,
	PM_STATUS_WAKEUPING,
	PM_STATUS_PRERUNNING,

	PM_STATUS_MAX,
	PM_STATUS_BASE = PM_STATUS_RUNNING,
} suspend_status_t;

#define pm_suspend_status_valid(_t) \
	((_t) >= PM_STATUS_BASE && (_t) < PM_STATUS_MAX)

typedef enum {
	PM_MOMENT_IRQ_DISABLE = 0,
	PM_MOMENT_SUSPEND_NOIRQ_END,
	PM_MOMENT_ENTER_WFI,
	PM_MOMENT_EXIT_WFI,
	PM_MOMENT_RESUME_NOIRQ_BEGIN,
	PM_MOMENT_IRQ_ENABLE,

	PM_MOMENT_MAX,
	PM_MOMENT_BASE = PM_MOMENT_IRQ_DISABLE,
} pm_moment_t;

#define pm_moment_valid(_t) \
	((_t) >= PM_MOMENT_BASE && (_t) < PM_MOMENT_MAX)

#define REC_FAILED_NUM	(2)
struct pm_suspend_stats {
	const char *name;
	char last_failed_unit[REC_FAILED_NUM][40];
	uint32_t last_failed_index;
	uint32_t unit_failed;
	int last_failed_errno;
	struct list_head node;
};
int pm_report_register(struct pm_suspend_stats *stats);
int pm_report_unregister(struct pm_suspend_stats *stats);

void pm_report_stats(void);

int pm_state_set(suspend_status_t status);
suspend_status_t pm_state_get(void);

void pm_moment_record(pm_moment_t moment, uint64_t timeval);
uint64_t pm_moment_interval_us(pm_moment_t start, pm_moment_t end);
void pm_moment_clear(uint32_t clear_mask);
int pm_moment_has_been_recorded(pm_moment_t moment);

#endif

