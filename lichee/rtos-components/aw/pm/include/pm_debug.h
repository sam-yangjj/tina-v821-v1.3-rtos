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

#ifndef _PM_DEBUG_H_
#define _PM_DEBUG_H_

#include "stdio.h"

#ifdef CONFIG_COMPONENTS_PM_CORE_M33
#define CORE_NAME  "m33"
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_DSP
#define CORE_NAME  "dsp"
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_RISCV
#define CORE_NAME  "rv."
#endif

#ifndef CORE_NAME
#define CORE_NAME  "pm"
#endif

enum {
	PM_DEBUG_ID_AMPRPC = 0,
	PM_DEBUG_ID_RISCV,
	PM_DEBUG_ID_DSP,
	PM_DEBUG_ID_M33,

	PM_DEBUG_ID_BASE,
	PM_DEBUG_ID_WAKELOCK,
	PM_DEBUG_ID_SUSPEND,
	PM_DEBUG_ID_TASK,

	PM_DEBUG_ID_NOTIFY,
	PM_DEBUG_ID_SUBSYS,
	PM_DEBUG_ID_DEVOPS,
	PM_DEBUG_ID_PLATOPS,

	PM_DEBUG_ID_WAKESRC,
	PM_DEBUG_ID_WAKERES,
	PM_DEBUG_ID_WAKECNT,
	PM_DEBUG_ID_SYSCORE,

	PM_DEBUG_ID_OTHERS,
};

#define PM_DEBUG_INVALID	(0)

#define PM_DEBUG_AMPRPC		(0x1 << PM_DEBUG_ID_AMPRPC)
#define PM_DEBUG_RISCV		(0x1 << PM_DEBUG_ID_RISCV)
#define PM_DEBUG_DSP		(0x1 << PM_DEBUG_ID_DSP)
#define PM_DEBUG_M33		(0x1 << PM_DEBUG_ID_M33)
#define PM_DEBUG_BASE		(0x1 << PM_DEBUG_ID_BASE)
#define PM_DEBUG_WAKELOCK	(0x1 << PM_DEBUG_ID_WAKELOCK)
#define PM_DEBUG_SUSPEND	(0x1 << PM_DEBUG_ID_SUSPEND)
#define PM_DEBUG_TASK		(0x1 << PM_DEBUG_ID_TASK)
#define PM_DEBUG_NOTIFY		(0x1 << PM_DEBUG_ID_NOTIFY)
#define PM_DEBUG_SUBSYS		(0x1 << PM_DEBUG_ID_SUBSYS)
#define PM_DEBUG_DEVOPS		(0x1 << PM_DEBUG_ID_DEVOPS)
#define PM_DEBUG_PLATOPS	(0x1 << PM_DEBUG_ID_PLATOPS)
#define PM_DEBUG_WAKESRC	(0x1 << PM_DEBUG_ID_WAKESRC)
#define PM_DEBUG_WAKERES	(0x1 << PM_DEBUG_ID_WAKERES)
#define PM_DEBUG_WAKECNT	(0x1 << PM_DEBUG_ID_WAKECNT)
#define PM_DEBUG_SYSCORE	(0x1 << PM_DEBUG_ID_SYSCORE)
#define PM_DEBUG_OTHERS		(0x1 << PM_DEBUG_ID_OTHERS)

#ifndef PM_DEBUG_MODULE
#define PM_DEBUG_MODULE		PM_DEBUG_OTHERS
#endif

#ifndef PM_DEBUG_ENABLE
#define PM_DEBUG_ENABLE		0xffffffffU
#endif


#define PM_DEBUG_ERR		0x1
#define PM_DEBUG_LOG		0x2
#define PM_DEBUG_WARN		0x4
#define PM_DEBUG_INF		0x8
#define PM_DEBUG_DBG		0x10
#define PM_DEBUG_ABORT		0x20
#define PM_DEBUG_TRACE		0x40

#ifndef PM_DEBUG_MASK
#define PM_DEBUG_MASK		0xffffff67U
#endif

#ifndef __maybe_unused
#define __maybe_unused      __attribute__((unused))
#endif

#define pm_raw(fmt, arg...)	do { if (PM_DEBUG_ENABLE & PM_DEBUG_MODULE) printf(fmt, ##arg); } while(0)

#if (PM_DEBUG_MASK & PM_DEBUG_ERR)
#define pm_err(fmt,arg...)  pm_raw("[E(" CORE_NAME ")] " fmt, ##arg)
#define pm_invalid()\
	pm_err("args is invild, at %s:%d.\n", __func__, __LINE__);
#else
#define pm_err(fmt,arg...)  do {} while(0)
#define pm_invalid()        do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_LOG)
#define pm_log(fmt,arg...)  pm_raw("[L(" CORE_NAME ")] " fmt, ##arg)
#else
#define pm_log(fmt,arg...)  do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_WARN)
#define pm_warn(fmt,arg...)  pm_raw("[W(" CORE_NAME ")] " fmt, ##arg)
#define pm_semapbusy(_x)\
	pm_warn("Task Semap" #_x "timeout, at %s:%d.\n", __func__, __LINE__);
#else
#define pm_warn(fmt,arg...)  do {} while(0)
#define pm_semapbusy(_x)	do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_INF)
#define pm_inf(fmt,arg...)  pm_raw("[I(" CORE_NAME ")] " fmt, ##arg)
#else
#define pm_inf(fmt,arg...)  do {} while(0)
#endif


#if (PM_DEBUG_MASK & PM_DEBUG_DBG)
#define pm_dbg(fmt,arg...)  pm_raw("[D(" CORE_NAME ")] " fmt, ##arg)

#define pm_ser_trace()\
	pm_raw("[T(ser)]: %s().\n", __func__);
#define pm_ser_trace1(_x1)\
	pm_raw("[T(ser)]: %s(%d).\n", __func__, _x1);
#define pm_ser_trace2(_x1, _x2)\
	pm_raw("[T(ser)]: %s(%d, %d).\n", __func__, _x1, _x2);
#define pm_ser_trace3(_x1, _x2, _x3)\
	pm_raw("[T(ser)]: %s(%d, %d, %d).\n", __func__, _x1, _x2, _x3);

#define pm_stub_trace()\
	pm_raw("[T(stub)]: %s().\n", __func__);
#define pm_stub_trace1(_x1)\
	pm_raw("[T(stub)]: %s(%d).\n", __func__, _x1);
#define pm_stub_trace2(_x1, _x2)\
	pm_raw("[T(stub)]: %s(%d, %d).\n", __func__, _x1, _x2);
#define pm_stub_trace3(_x1, _x2, _x3)\
	pm_raw("[T(stub)]: %s(%d, %d, %d).\n", __func__, _x1, _x2, _x3);
#else
#define pm_dbg(fmt,arg...)  do {} while(0)

#define pm_ser_trace()			do {} while(0)
#define pm_ser_trace1(_x1)		do {} while(0)
#define pm_ser_trace2(_x1, _x2)		do {} while(0)
#define pm_ser_trace3(_x1, _x2, _x3)	do {} while(0)

#define pm_stub_trace()			do {} while(0)
#define pm_stub_trace1(_x1)		do {} while(0)
#define pm_stub_trace2(_x1, _x2)	do {} while(0)
#define pm_stub_trace3(_x1, _x2, _x3)	do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_ABORT)
#define pm_abort(_cont)     do { if (_cont) {pm_raw("[PM_ABORT(" CORE_NAME ")] at %s:%d\n", __func__, __LINE__); while (1);} } while(0)
#else
#define pm_abort(_cont)     do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_TRACE)
#define pm_trace(fmt, arg...)\
	pm_raw("[T(" CORE_NAME ")] at %s:%d" fmt, __func__, __LINE__, ##arg)

#define pm_trace_func(fmt, arg...)\
	pm_raw("[T(" CORE_NAME ")] call %s(" fmt ")" "\n", __func__, ##arg)

#define pm_trace_ret(_func, _ret)\
	pm_raw("[T(" CORE_NAME ")] call %s return %d(0x%x)" "\n", #_func, _ret, _ret)

#define pm_trace_info(_str1)\
	pm_trace(" %s\n", (_str1)?(_str1):"null")

#define pm_devops_start_trace(_x1)\
	pm_raw("[T("CORE_NAME")(devops)]: calling %s ...\n", _x1);
#define pm_devops_end_trace(_x1, _x2, _x3)\
	pm_raw("[T("CORE_NAME")(devops)]: call %s return %d after %" PRIu64 " usec.\n", _x1, _x2, _x3);

#define pm_stage(fmt, arg...)	do { printf("[STAGE(" CORE_NAME ")] PM: " fmt, ##arg); } while(0)
#else
#define pm_trace(fmt, arg...)		do {} while(0)
#define pm_trace_func(fmt, arg...)	do {} while(0)
#define pm_trace_ret(_func, _ret)	do {} while(0)
#define pm_trace_info(_str1)		do {} while(0)

#define pm_devops_start_trace(_x1)		do {} while(0)
#define pm_devops_end_trace(_x1, _x2, _x3)	do {} while(0)
#define pm_stage(fmt, arg...)			do {} while(0)
#endif

typedef enum {
	/* notify stage */
	PM_TIME_RECORD_NOTIFY_SYS_PERPARED = 0,
	PM_TIME_RECORD_NOTIFY_PERPARED,
	PM_TIME_RECORD_NOTIFY_FINISHED,
	PM_TIME_RECORD_NOTIFY_SYS_FINISHED,
	/* devops stage */
	PM_TIME_RECORD_DEVOPS_PREPARED,
	PM_TIME_RECORD_DEVOPS_SUSPEND,
	PM_TIME_RECORD_DEVOPS_SUSPEND_LATE,
	PM_TIME_RECORD_DEVOPS_SUSPEND_NOIRQ,
	PM_TIME_RECORD_DEVOPS_RESUME_NOIRQ,
	PM_TIME_RECORD_DEVOPS_RESUME_EARLY,
	PM_TIME_RECORD_DEVOPS_RESUME,
	PM_TIME_RECORD_DEVOPS_COMPLETE,
	/* syscore stage */
	PM_TIME_RECORD_SYSCORE_SUSPEND,
	PM_TIME_RECORD_SYSCORE_RESUME,
	/* platops stage */
	PM_TIME_RECORD_PLATOPS_PREPARED_NOTIFY,
	PM_TIME_RECORD_PLATOPS_PRE_BEGIN,
	PM_TIME_RECORD_PLATOPS_BEGIN,
	PM_TIME_RECORD_PLATOPS_PREPARE,
	PM_TIME_RECORD_PLATOPS_PREPARE_LATE,
	PM_TIME_RECORD_PLATOPS_ENTER_SUSPEND,
	PM_TIME_RECORD_PLATOPS_ENTER_RESUME,
	PM_TIME_RECORD_PLATOPS_WAKE,
	PM_TIME_RECORD_PLATOPS_FINISH,
	PM_TIME_RECORD_PLATOPS_END,
	PM_TIME_RECORD_PLATOPS_POST_END,
	PM_TIME_RECORD_PLATOPS_RECOVER,
	PM_TIME_RECORD_PLATOPS_AGAIN,
	PM_TIME_RECORD_PLATOPS_AGAIN_LATE,
	PM_TIME_RECORD_PLATOPS_FINISHED_NOTIFY,
	/* framework stage */
	PM_TIME_RECORD_TASK_FREEZE,
	PM_TIME_RECORD_IRQ_DISABLE,
	PM_TIME_RECORD_VIRTLOG_ENABLE,
	PM_TIME_RECORD_VIRTLOG_DISABLE,
	PM_TIME_RECORD_IRQ_ENABLE,
	PM_TIME_RECORD_TASK_RESTORE,
	/* private stage */
	/* when private stage is update, must update PM_TIME_RECORD_PRIV_START and PM_TIME_RECORD_PRIV_END */
#if defined(CONFIG_ARCH_SUN300IW1)
	PM_TIME_RECORD_CLOSE_CPUX,
	PM_TIME_RECORD_OPEN_CPUX,

	PM_TIME_RECORD_DATA_BACKUP,
	PM_TIME_RECORD_DRAM_SUSPEND,
	PM_TIME_RECORD_CLOCK_SUSPEND,
	PM_TIME_RECORD_POWER_SUSPEND,
	PM_TIME_RECORD_CACHE_SUSPEND,
	PM_TIME_RECORD_CACHE_RESUME,
	PM_TIME_RECORD_POWER_RESUME,
	PM_TIME_RECORD_CLOCK_RESUME,
	PM_TIME_RECORD_SENSOR_RESUME,
	PM_TIME_RECORD_DRAM_RESUME,
	PM_TIME_RECORD_LOAD_RTOS,
	PM_TIME_RECORD_DATA_RESTORE,
	PM_TIME_RECORD_STANDBY_START = PM_TIME_RECORD_DATA_BACKUP,
	PM_TIME_RECORD_STANDBY_END = PM_TIME_RECORD_DATA_RESTORE,
#endif
	/* max */
	PM_TIME_RECORD_MAX,
	PM_TIME_RECORE_BASE = PM_TIME_RECORD_NOTIFY_SYS_PERPARED,
} pm_time_record_t;

struct pm_time_store {
	uint32_t seq;
	uint32_t start;
	uint32_t end;
};

#if defined(CONFIG_PM_TIME_RECORD)
void pm_time_record_start(pm_time_record_t stage);
void pm_time_record_end(pm_time_record_t stage);
void pm_time_record_clr(pm_time_record_t stage, uint32_t is_start);
void pm_time_record_set(pm_time_record_t stage, uint32_t time, uint32_t is_start);
void pm_time_record_reset(void);
void pm_time_record_show(void);
#else
static inline void pm_time_record_start(pm_time_record_t stage) { ; }
static inline void pm_time_record_end(pm_time_record_t stage) { ; }
static inline void pm_time_record_clr(pm_time_record_t stage, uint32_t is_start) { ; }
static inline void pm_time_record_set(pm_time_record_t stage, uint32_t time, uint32_t is_start) { ; }
static inline void pm_time_record_reset(void) { ; }
static inline void pm_time_record_show(void){ ; }
#endif

typedef enum {
	/* notify stage */
	PM_HEAP_RECORD_NOTIFY_SYS_PERPARED = 0,
	PM_HEAP_RECORD_NOTIFY_PERPARED,
	PM_HEAP_RECORD_NOTIFY_FINISHED,
	PM_HEAP_RECORD_NOTIFY_SYS_FINISHED,
	/* devops stage */
	PM_HEAP_RECORD_DEVOPS_PREPARED,
	PM_HEAP_RECORD_DEVOPS_SUSPEND,
	PM_HEAP_RECORD_DEVOPS_SUSPEND_LATE,
	PM_HEAP_RECORD_DEVOPS_SUSPEND_NOIRQ,
	PM_HEAP_RECORD_DEVOPS_RESUME_NOIRQ,
	PM_HEAP_RECORD_DEVOPS_RESUME_EARLY,
	PM_HEAP_RECORD_DEVOPS_RESUME,
	PM_HEAP_RECORD_DEVOPS_COMPLETE,
	/* syscore stage */
	PM_HEAP_RECORD_SYSCORE_SUSPEND,
	PM_HEAP_RECORD_SYSCORE_RESUME,
	/* platops stage */
	PM_HEAP_RECORD_PLATOPS_PREPARED_NOTIFY,
	PM_HEAP_RECORD_PLATOPS_PRE_BEGIN,
	PM_HEAP_RECORD_PLATOPS_BEGIN,
	PM_HEAP_RECORD_PLATOPS_PREPARE,
	PM_HEAP_RECORD_PLATOPS_PREPARE_LATE,
	PM_HEAP_RECORD_PLATOPS_ENTER,
	PM_HEAP_RECORD_PLATOPS_WAKE,
	PM_HEAP_RECORD_PLATOPS_FINISH,
	PM_HEAP_RECORD_PLATOPS_END,
	PM_HEAP_RECORD_PLATOPS_POST_END,
	PM_HEAP_RECORD_PLATOPS_RECOVER,
	PM_HEAP_RECORD_PLATOPS_AGAIN,
	PM_HEAP_RECORD_PLATOPS_AGAIN_LATE,
	PM_HEAP_RECORD_PLATOPS_FINISHED_NOTIFY,

	/* max */
	PM_HEAP_RECORD_MAX,
	PM_HEAP_RECORE_BASE = PM_HEAP_RECORD_NOTIFY_SYS_PERPARED,
} pm_heap_record_t;

#if defined(CONFIG_PM_HEAP_RECORD)
void pm_heap_record_start(pm_heap_record_t stage);
void pm_heap_record_end(pm_heap_record_t stage);
void pm_heap_record_reset(void);
void pm_heap_record_show(void);
#else
static inline void pm_heap_record_start(pm_time_record_t stage) { ; }
static inline void pm_heap_record_end(pm_time_record_t stage) { ; }
static inline void pm_heap_record_reset(void) { ; }
static inline void pm_heap_record_show(void) { ; }
#endif

#endif /* _PM_DEBUG_H_ */


