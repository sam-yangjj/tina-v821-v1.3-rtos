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
#include <string.h>
#include <hal_interrupt.h>
#include <hal_cmd.h>
#include <pm_debug.h>
#include <pm_base.h>
#include <pm_platops.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#if defined(CONFIG_PM_TIME_RECORD)
#define INIT_TIME_RECORD_STRING(index, string)   [index] = (string)
const char *pm_time_record_string[PM_TIME_RECORD_MAX] = {
	/* notify stage */
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_NOTIFY_SYS_PERPARED, "notify_sys_prepared"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_NOTIFY_PERPARED, "notify_prepared"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_NOTIFY_FINISHED, "notify_finished"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_NOTIFY_SYS_FINISHED, "notify_sys_finished"),
	/* devops stage */
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DEVOPS_PREPARED, "devops_prepared"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DEVOPS_SUSPEND, "devops_suspend"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DEVOPS_SUSPEND_LATE, "devops_suspend_late"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DEVOPS_SUSPEND_NOIRQ, "devops_suspend_noirq"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DEVOPS_RESUME_NOIRQ, "devops_resume_noirq"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DEVOPS_RESUME_EARLY, "devops_resume_early"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DEVOPS_RESUME, "devops_resume"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DEVOPS_COMPLETE, "devops_complete"),
	/* syscore stage */
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_SYSCORE_SUSPEND, "syscore_suspend"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_SYSCORE_RESUME, "syscore_resume"),
	/* platops stage */
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_PREPARED_NOTIFY, "platops_prepared_notify"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_PRE_BEGIN, "platops_pre_begin"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_BEGIN, "platops_begin"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_PREPARE, "platops_prepare"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_PREPARE_LATE, "platops_prepare_late"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_ENTER_SUSPEND, "platops_enter_suspend"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_ENTER_RESUME, "platops_enter_resume"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_WAKE, "platops_wake"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_FINISH, "platops_finish"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_END, "platops_end"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_POST_END, "platops_post_end"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_RECOVER, "platops_recover"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_AGAIN, "platops_again"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_AGAIN_LATE, "platops_again_late"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_PLATOPS_FINISHED_NOTIFY, "platops_finished_notify"),
	/* framework stage */
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_TASK_FREEZE, "task_freeze"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_TASK_RESTORE, "task_restore"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_VIRTLOG_ENABLE, "virtlog_enable"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_VIRTLOG_DISABLE, "virtlog_disable"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_IRQ_ENABLE, "irq_enable"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_IRQ_DISABLE, "irq_disable"),
	/* private stage */
#if defined(CONFIG_ARCH_SUN300IW1)
	/* platops priv */
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_CLOSE_CPUX, "close_cpux"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_OPEN_CPUX, "open_cpux"),

	/* standby priv */
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DATA_BACKUP, "data_backup"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DRAM_SUSPEND, "dram_suspend"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_CLOCK_SUSPEND, "clock_suspend"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_POWER_SUSPEND, "power_suspend"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_CACHE_SUSPEND, "cache_suspend"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_CACHE_RESUME, "cache_resume"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_POWER_RESUME, "power_resume"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_CLOCK_RESUME, "clock_resume"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_SENSOR_RESUME, "sensor_resume"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DRAM_RESUME, "dram_resume"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_LOAD_RTOS, "load_rtos"),
	INIT_TIME_RECORD_STRING(PM_TIME_RECORD_DATA_RESTORE, "data_restore"),
#endif
};

#if defined(CONFIG_PM_TIME_RECORD_PLUS)
static struct pm_time_store tstore[PM_TIME_RECORD_MAX] = {{0}};
static uint32_t seq = 0;

void pm_time_record_start(pm_time_record_t stage)
{
	tstore[stage].start = pm_platops_get_timestamp();
}

void pm_time_record_end(pm_time_record_t stage)
{
	tstore[stage].end = pm_platops_get_timestamp();
	tstore[stage].seq = seq;
	seq += 1;
}

void pm_time_record_clr(pm_time_record_t stage, uint32_t is_start)
{
	if (is_start) {
		tstore[stage].start = 0;
	} else {
		tstore[stage].end = 0;
		tstore[stage].seq = 0;
	}
}

void pm_time_record_set(pm_time_record_t stage, uint32_t time, uint32_t is_start)
{
	if (is_start) {
		tstore[stage].start = time;
	} else {
		tstore[stage].end = time;
		tstore[stage].seq = seq;
		seq += 1;
	}
}

void pm_time_record_reset(void)
{
	printf("reset pm time record\n");
	seq = 0;
	memset(tstore, 0, sizeof(tstore));
}
FINSH_FUNCTION_EXPORT_CMD(pm_time_record_reset, pm_time_reset, pm_time_reset)

void pm_time_record_show(void)
{
	uint32_t i, j;

	printf("show pm time record, end record cnt: %d\n", seq + 1);
	printf("%-25s: %-25s %-25s %-25s\n", "seq stage_name", "period", "start", "end");
	for (i = 0; i < seq; i++) {
		for (j = PM_TIME_RECORE_BASE; j < PM_TIME_RECORD_MAX; j++) {
			if (tstore[j].seq == i) {
				printf("%d %-25s: %-25u %-25u %-25u\n",
					i, pm_time_record_string[j],
					(((tstore[j].start != 0) && (tstore[j].start < tstore[j].end))? (tstore[j].end - tstore[j].start): 0),
					tstore[j].start, tstore[j].end);
			}
		}

	}
}
FINSH_FUNCTION_EXPORT_CMD(pm_time_record_show, pm_time_show, pm_time_show)
#else
static uint32_t pm_time_record_value[PM_TIME_RECORD_MAX] = {0};

void pm_time_record_start(pm_time_record_t stage)
{
	pm_time_record_value[stage] = pm_platops_get_timestamp();
}

void pm_time_record_end(pm_time_record_t stage)
{
	pm_time_record_value[stage] = pm_platops_get_timestamp() - pm_time_record_value[stage];
}

void pm_time_record_clr(pm_time_record_t stage, uint32_t is_start)
{
	pm_time_record_value[stage] = 0;
}

void pm_time_record_set(pm_time_record_t stage, uint32_t time, uint32_t is_start)
{
	pm_time_record_value[stage] = time;
}

void pm_time_record_reset(void)
{
	printf("reset pm time record\n");
	memset(pm_time_record_value, 0, sizeof(pm_time_record_value));
}
FINSH_FUNCTION_EXPORT_CMD(pm_time_record_reset, pm_time_reset, pm_time_reset)

void pm_time_record_show(void)
{
	uint32_t i;

	printf("show pm time record\n");
	for (i = PM_TIME_RECORE_BASE; i < PM_TIME_RECORD_MAX; i++) {
		printf("%-25s: %u us\n", pm_time_record_string[i], pm_time_record_value[i]);
	}
}
FINSH_FUNCTION_EXPORT_CMD(pm_time_record_show, pm_time_show, pm_time_show)

#if defined(CONFIG_ARCH_SUN300IW1)
typedef struct {
	pm_time_record_t start_stage;
	pm_time_record_t end_stage;
	char *stage_string;
} simple_show_t;

const simple_show_t simple_show_array[] = {
	{PM_TIME_RECORD_NOTIFY_SYS_PERPARED,     PM_TIME_RECORD_NOTIFY_SYS_FINISHED,     "notify"},
	{PM_TIME_RECORD_PLATOPS_PREPARED_NOTIFY, PM_TIME_RECORD_PLATOPS_FINISHED_NOTIFY, "platops"},
	{PM_TIME_RECORD_DEVOPS_PREPARED,         PM_TIME_RECORD_DEVOPS_COMPLETE,         "devops"},
	{PM_TIME_RECORD_SYSCORE_SUSPEND,         PM_TIME_RECORD_SYSCORE_RESUME,          "syscore"},
	{PM_TIME_RECORD_TASK_FREEZE,             PM_TIME_RECORD_TASK_RESTORE,            "framework"},
	{PM_TIME_RECORD_DATA_BACKUP,             PM_TIME_RECORD_DATA_RESTORE,            "standby"},
	{PM_TIME_RECORD_PLATOPS_AGAIN,           PM_TIME_RECORD_PLATOPS_AGAIN,           "wait_again"},
};

void pm_time_record_show_simple(void)
{
	uint32_t i, j;
	uint32_t time_sum;

	printf("show pm time record simple\n");
	for (i = 0; i < ARRAY_SIZE(simple_show_array); i++) {
		time_sum = 0;
		for (j = simple_show_array[i].start_stage; j <= simple_show_array[i].end_stage; j++) {
			if (!strcmp (simple_show_array[i].stage_string, "platops") && j == PM_TIME_RECORD_PLATOPS_AGAIN) {
				continue;
			}
			time_sum += pm_time_record_value[j];
		}
		printf("%-25s: %u us\n", simple_show_array[i].stage_string, time_sum);
	}
}
FINSH_FUNCTION_EXPORT_CMD(pm_time_record_show_simple, pm_time_show_simple, pm_time_show_simple)
#endif /* CONFIG_ARCH_SUN300IW1 */
#endif /* CONFIG_PM_TIME_RECORD_PLUS */
#endif /* CONFIG_PM_TIME_RECORD */

#if defined(CONFIG_PM_HEAP_RECORD)
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
} BlockLink_t;

#define BLOCK_STRUCT_ALIGN_SIZE   (((sizeof(BlockLink_t)) + ((portBYTE_ALIGNMENT) - 1)) & ~((portBYTE_ALIGNMENT) - 1))
#define get_real_size(size)   (size & 0x7FFFFFFF)
extern BlockLink_t *pxStart;
extern BlockLink_t *pxEnd;

#define INIT_HEAP_RECORD_STRING(index, string)   [index] = (string)
const char *pm_heap_record_string[PM_HEAP_RECORD_MAX] = {
	/* notify stage */
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_NOTIFY_SYS_PERPARED, "notify_sys_prepared"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_NOTIFY_PERPARED, "notify_prepared"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_NOTIFY_FINISHED, "notify_finished"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_NOTIFY_SYS_FINISHED, "notify_sys_finished"),
	/* devops stage */
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_DEVOPS_PREPARED, "devops_prepared"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_DEVOPS_SUSPEND, "devops_suspend"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_DEVOPS_SUSPEND_LATE, "devops_suspend_late"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_DEVOPS_SUSPEND_NOIRQ, "devops_suspend_noirq"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_DEVOPS_RESUME_NOIRQ, "devops_resume_noirq"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_DEVOPS_RESUME_EARLY, "devops_resume_early"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_DEVOPS_RESUME, "devops_resume"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_DEVOPS_COMPLETE, "devops_complete"),
	/* syscore stage */
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_SYSCORE_SUSPEND, "syscore_suspend"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_SYSCORE_RESUME, "syscore_resume"),
	/* platops stage */
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_PREPARED_NOTIFY, "platops_prepared_notify"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_PRE_BEGIN, "platops_pre_begin"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_BEGIN, "platops_begin"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_PREPARE, "platops_prepare"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_PREPARE_LATE, "platops_prepare_late"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_ENTER, "platops_enter"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_WAKE, "platops_wake"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_FINISH, "platops_finish"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_END, "platops_end"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_POST_END, "platops_post_end"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_RECOVER, "platops_recover"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_AGAIN, "platops_again"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_AGAIN_LATE, "platops_again_late"),
	INIT_HEAP_RECORD_STRING(PM_HEAP_RECORD_PLATOPS_FINISHED_NOTIFY, "platops_finished_notify"),
};

static uint32_t pm_heap_record_value[PM_HEAP_RECORD_MAX][2] = {0};

static uint32_t pm_get_heap_info(void)
{
	unsigned long irq_flag;
	uint32_t used_size = 0;
	BlockLink_t *block = (BlockLink_t *)pxStart;

	irq_flag = hal_interrupt_disable_irqsave();
	while (block < (BlockLink_t *)pxEnd) {
		if (block->pxNextFreeBlock == NULL) {
			/* block is used */
			used_size += (get_real_size(block->xBlockSize) - BLOCK_STRUCT_ALIGN_SIZE);
		}
		block = (BlockLink_t *)((uint32_t)block + get_real_size(block->xBlockSize));
	}
	hal_interrupt_enable_irqrestore(irq_flag);

	return used_size;
}

void pm_heap_record_start(pm_heap_record_t stage)
{
	pm_heap_record_value[stage][0] = pm_get_heap_info();
}

void pm_heap_record_end(pm_heap_record_t stage)
{
	pm_heap_record_value[stage][1] = pm_get_heap_info();
}

void pm_heap_record_reset(void)
{
	printf("reset pm heap record\n");
	memset(pm_heap_record_value, 0, sizeof(pm_heap_record_value));
}
FINSH_FUNCTION_EXPORT_CMD(pm_heap_record_reset, pm_heap_reset, pm_heap_reset)

void pm_heap_record_show(void)
{
	uint32_t i;

	printf("show pm heap record\n");
	for (i = PM_HEAP_RECORE_BASE; i < PM_HEAP_RECORD_MAX; i++) {
		printf("%-25s enter: %-10u exit: %-10u change: %-10d\n",
		       pm_heap_record_string[i],
		       pm_heap_record_value[i][0],
		       pm_heap_record_value[i][1],
		       pm_heap_record_value[i][1] - pm_heap_record_value[i][0]);
	}
}
FINSH_FUNCTION_EXPORT_CMD(pm_heap_record_show, pm_heap_show, pm_heap_show)
#endif
