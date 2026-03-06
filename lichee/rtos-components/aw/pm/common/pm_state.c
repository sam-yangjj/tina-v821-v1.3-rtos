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
#include <stdlib.h>
#include <string.h>
#include <osal/hal_interrupt.h>
#include <errno.h>
#include <hal_time.h>

#include <hal/aw_list.h>
#include "pm_debug.h"
#include "pm_state.h"
#include "pm_adapt.h"

#define MOMENT_INIT	(0)

#define PM_REPORT	("PM_REPORT")
#define pm_susnpend_stats_containerof(ptr_module) \
        __containerof(ptr_module, struct pm_suspend_stats, node)
static struct list_head pm_report_list = LIST_HEAD_INIT(pm_report_list);

static suspend_status_t pm_suspend_status = PM_STATUS_RUNNING;

static uint64_t moment_record[PM_MOMENT_MAX] = {MOMENT_INIT};

int pm_state_set(suspend_status_t status)
{
	if (!pm_suspend_status_valid(status))
		return -EINVAL;

	pm_suspend_status = status;

	return 0;
}

suspend_status_t pm_state_get(void)
{
	return pm_suspend_status;
}

int pm_report_register(struct pm_suspend_stats *stats) {
	if (!stats || !stats->name)
		return -EINVAL;

	/* new stats insert as head */
	list_add(&stats->node, &pm_report_list);

	return 0;
}

int pm_report_unregister(struct pm_suspend_stats *stats) {
	if (stats == NULL) {
	return -EINVAL;
	}
	list_del(&stats->node);

	return 0;
}

void pm_report_stats(void) {
	struct pm_suspend_stats *stats = NULL;
	struct list_head *node = NULL;
	struct list_head *head = &pm_report_list;

	list_for_each(node, head) {
		stats = pm_susnpend_stats_containerof(node);
		if (stats->unit_failed) {
			pm_err("%s[%s]: unit failed cnt %d, last fail unit0: %s\n", PM_REPORT,
				stats->name, stats->unit_failed, stats->last_failed_unit[0]);
			if (stats->last_failed_unit[1] && (stats->last_failed_index == 1))
				pm_err("last fail unit1: %s\n", stats->last_failed_unit[1]);
			stats->last_failed_index = 0;
			stats->unit_failed = 0;
		}

		if (stats->last_failed_errno) {
			pm_err("%s[%s]: last fail errno: %d\n", PM_REPORT,
				stats->name, stats->last_failed_errno);
			stats->last_failed_errno = 0;
		}
	}
}

void pm_moment_record(pm_moment_t moment, uint64_t timeval)
{
	if (!pm_moment_valid(moment)) {
		pm_err("record moment %d invalid\n", moment);
		return;
	}

	if (moment_record[moment] == MOMENT_INIT) {
		if (timeval)
			moment_record[moment] = timeval;
		else
			moment_record[moment] = hal_gettime_ns();

		pm_dbg("pm record moment %d\n", moment);
	} else {
		pm_err("pm moment %d has been recorded\n", moment);
	}
}

uint64_t pm_moment_interval_us(pm_moment_t start, pm_moment_t end)
{

	if (!pm_moment_valid(start) || !pm_moment_valid(end) || end < start) {
		pm_err("pm moment interval invalid, start: %d, end: %d\n", start, end);
		return 0;
	}

	if (moment_record[start] == MOMENT_INIT) {
		pm_err("start moment %d is unrecorded, get interval: 0\n", start);
		return 0;
	}

	if (moment_record[end] == MOMENT_INIT) {
		pm_err("end moment %d is unrecorded, get interval: 0\n", end);
		return 0;
	}

	return ((moment_record[end] - moment_record[start]) / 1000LL);
}

void pm_moment_clear(uint32_t clear_mask)
{
	int i;

	for (i = PM_MOMENT_BASE; i < PM_MOMENT_MAX; i++) {
		if (clear_mask & (0x1 << i)) {
			moment_record[i] = MOMENT_INIT;
			pm_dbg("clear moment record %d\n", i);
		}
	}

}

int pm_moment_has_been_recorded(pm_moment_t moment)
{
	if (!pm_moment_valid(moment)) {
		pm_err("record moment %d invalid\n", moment);
		return 0;
	}

	if (moment_record[moment] != MOMENT_INIT)
		return 1;

	return 0;
}

