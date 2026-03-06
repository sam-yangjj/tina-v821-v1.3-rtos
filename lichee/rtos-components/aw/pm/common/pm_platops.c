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
#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#else
#error "PM do not support the RTOS!!"
#endif
#include <task.h>
#include <errno.h>

#include "pm_debug.h"
#include "pm_base.h"
#include "pm_platops.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_PLATOPS

static suspend_ops_t *suspend_ops = NULL;

static int pm_platops_call_enter(suspend_ops_type_t type, suspend_mode_t mode)
{
	switch (type) {
	case PM_SUSPEND_OPS_TYPE_PREPARED_NOTIFY:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_PREPARED_NOTIFY);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_PREPARED_NOTIFY);
		break;
	case PM_SUSPEND_OPS_TYPE_PRE_BEGIN:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_PRE_BEGIN);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_PRE_BEGIN);
		break;
	case PM_SUSPEND_OPS_TYPE_BEGIN:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_BEGIN);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_BEGIN);
		break;
	case PM_SUSPEND_OPS_TYPE_PREPARE:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_PREPARE);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_PREPARE);
		break;
	case PM_SUSPEND_OPS_TYPE_PREPARE_LATE:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_PREPARE_LATE);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_PREPARE_LATE);
		break;
	case PM_SUSPEND_OPS_TYPE_ENTER:
		/* should not record here because contain wfi sleep time */
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_ENTER);
		break;
	case PM_SUSPEND_OPS_TYPE_WAKE:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_WAKE);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_WAKE);
		break;
	case PM_SUSPEND_OPS_TYPE_FINISH:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_FINISH);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_FINISH);
		break;
	case PM_SUSPEND_OPS_TYPE_END:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_END);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_END);
		break;
	case PM_SUSPEND_OPS_TYPE_POST_END:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_POST_END);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_POST_END);
		break;
	case PM_SUSPEND_OPS_TYPE_RECOVER:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_RECOVER);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_RECOVER);
		break;
	case PM_SUSPEND_OPS_TYPE_AGAIN:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_AGAIN);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_AGAIN);
		break;
	case PM_SUSPEND_OPS_TYPE_AGAIN_LATE:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_AGAIN_LATE);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_AGAIN_LATE);
		break;
	case PM_SUSPEND_OPS_TYPE_FINISHED_NOTIFY:
		pm_time_record_start(PM_TIME_RECORD_PLATOPS_FINISHED_NOTIFY);
		pm_heap_record_start(PM_HEAP_RECORD_PLATOPS_FINISHED_NOTIFY);
		break;
	default:
		break;
	}
	return 0;
}

static int pm_platops_call_exit(suspend_ops_type_t type, suspend_mode_t mode)
{
	switch (type) {
	case PM_SUSPEND_OPS_TYPE_PREPARED_NOTIFY:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_PREPARED_NOTIFY);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_PREPARED_NOTIFY);
		break;
	case PM_SUSPEND_OPS_TYPE_PRE_BEGIN:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_PRE_BEGIN);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_PRE_BEGIN);
		break;
	case PM_SUSPEND_OPS_TYPE_BEGIN:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_BEGIN);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_BEGIN);
		break;
	case PM_SUSPEND_OPS_TYPE_PREPARE:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_PREPARE);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_PREPARE);
		break;
	case PM_SUSPEND_OPS_TYPE_PREPARE_LATE:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_PREPARE_LATE);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_PREPARE_LATE);
		break;
	case PM_SUSPEND_OPS_TYPE_ENTER:
		/* should not record here because contain wfi sleep time */
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_ENTER);
		break;
	case PM_SUSPEND_OPS_TYPE_WAKE:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_WAKE);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_WAKE);
		break;
	case PM_SUSPEND_OPS_TYPE_FINISH:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_FINISH);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_FINISH);
		break;
	case PM_SUSPEND_OPS_TYPE_END:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_END);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_END);
		break;
	case PM_SUSPEND_OPS_TYPE_POST_END:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_POST_END);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_POST_END);
		break;
	case PM_SUSPEND_OPS_TYPE_RECOVER:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_RECOVER);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_RECOVER);
		break;
	case PM_SUSPEND_OPS_TYPE_AGAIN:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_AGAIN);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_AGAIN);
		break;
	case PM_SUSPEND_OPS_TYPE_AGAIN_LATE:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_AGAIN_LATE);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_AGAIN_LATE);
		break;
	case PM_SUSPEND_OPS_TYPE_FINISHED_NOTIFY:
		pm_time_record_end(PM_TIME_RECORD_PLATOPS_FINISHED_NOTIFY);
		pm_heap_record_end(PM_HEAP_RECORD_PLATOPS_FINISHED_NOTIFY);
		break;
	default:
		break;
	}
	return 0;
}

int pm_platops_register(suspend_ops_t *ops)
{
	pm_dbg("platops: register ops %s(%p) ok\n",
			(ops && ops->name)?ops->name:"UNKOWN", ops);

	suspend_ops = ops;

	return 0;
}

int pm_platops_call(suspend_ops_type_t type, suspend_mode_t mode)
{
	int ret = 0;

	pm_platops_call_enter(type, mode);

	//pm_trace_func("%d, %d", type, mode);

	if (!suspend_ops || !pm_suspend_mode_valid(mode)) {
		pm_invalid();
		ret = -EINVAL;
		goto exit;
	}

#if 0
	/* check suspend*/
	ret = pm_suspend_assert();
	if (ret)
		return ret;
#endif

	switch (type) {
	case PM_SUSPEND_OPS_TYPE_VALID:
		if (suspend_ops->valid)
			ret = suspend_ops->valid(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_PREPARED_NOTIFY:
		if (suspend_ops->prepare_notify)
			ret = suspend_ops->prepare_notify(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_PRE_BEGIN:
		if (suspend_ops->pre_begin)
			ret = suspend_ops->pre_begin(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_BEGIN:
		if (suspend_ops->begin)
			ret = suspend_ops->begin(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_PREPARE:
		if (suspend_ops->prepare)
			ret = suspend_ops->prepare(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_PREPARE_LATE:
		if (suspend_ops->prepare_late)
			ret = suspend_ops->prepare_late(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_ENTER:
		if (suspend_ops->enter)
			ret = suspend_ops->enter(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_WAKE:
		if (suspend_ops->wake)
			ret = suspend_ops->wake(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_FINISH:
		if (suspend_ops->finish)
			ret = suspend_ops->finish(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_END:
		if (suspend_ops->end)
			ret = suspend_ops->end(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_POST_END:
		if (suspend_ops->post_end)
			ret = suspend_ops->post_end(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_RECOVER:
		if (suspend_ops->recover)
			ret = suspend_ops->recover(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_AGAIN:
		if (suspend_ops->again)
			ret = suspend_ops->again(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_AGAIN_LATE:
		if (suspend_ops->again_late)
			ret = suspend_ops->again_late(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_FINISHED_NOTIFY:
		if (suspend_ops->finish_notify)
			ret = suspend_ops->finish_notify(mode);
		break;
	default:
		ret = -EINVAL;
		break;
	}

exit:
	pm_platops_call_exit(type, mode);
	return ret;
}

uint32_t pm_platops_get_timestamp(void)
{
	if (suspend_ops->get_timestamp) {
		return suspend_ops->get_timestamp();
	}
	pm_err("platops get timestamp function is NULL");
	return 0;
}

hardware_wakesrc_mask_t pm_platops_get_hardware_wakesrc(void)
{
	if (suspend_ops->get_hardware_wakesrc_mask) {
		return suspend_ops->get_hardware_wakesrc_mask();
	}
	pm_err("platops get hardware wakesrc function is NULL");
	return 0;
}

