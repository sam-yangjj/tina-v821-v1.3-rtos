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
#include <stdio.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <console.h>
#include <aw_list.h>
#include <context.h>

#ifdef CONFIG_COMMAND_IRQ_DEBUG
#include <irq_core.h>
#include <hal_time.h>
#endif

#ifndef configAPPLICATION_NORMAL_PRIORITY
#define configAPPLICATION_NORMAL_PRIORITY (15)
#endif

__attribute__((weak)) size_t xPortGetTotalHeapSize( void )
{
    return CONFIG_TOTAL_HEAP_SIZE;
}

#ifdef CONFIG_HEAP_MULTIPLE
#include <aw_malloc.h>
// sync from cmd_free
static void show_heap_info(int heapID, uint32_t totalsize, char *ram_name)
{
    uint32_t freesize = 0;
    uint32_t minfreesize = 0;

    totalsize = aw_xPortGetTotalHeapSize(heapID);
    freesize = aw_xPortGetFreeHeapSize(heapID);
    minfreesize = aw_xPortGetMinimumEverFreeHeapSize(heapID);

    printf( "       %s Heap:\n", ram_name);
    printf( "           Total Size : %8ld Bytes    (%5ld KB  100.00%%)\n"
            "                 Free : %8ld Bytes    (%5ld KB  %6.2f%%)\n"
            "             Min Free : %8ld Bytes    (%5ld KB  %6.2f%%)\n",
        totalsize, totalsize >> 10,
        freesize, freesize >> 10, freesize * 100.0f / totalsize,
        minfreesize, minfreesize >> 10, minfreesize * 100.0f / totalsize);
    printf("\r\n");
}
static void show_memory_info(void)
{
#ifdef CONFIG_SRAM_HEAP
    show_heap_info(SRAM_HEAP_ID, CONFIG_SRAM_HEAP_SIZE, "sram");
#endif
#ifdef CONFIG_DRAM_HEAP
    show_heap_info(DRAM_HEAP_ID, CONFIG_DRAM_HEAP_SIZE, "dram");
#endif
#ifdef CONFIG_LPSRAM_HEAP
    show_heap_info(LPSRAM_HEAP_ID, CONFIG_LPSRAM_HEAP_SIZE, "lpsram");
#endif
#ifdef CONFIG_HPSRAM_HEAP
    show_heap_info(HPSRAM_HEAP_ID, CONFIG_HPSRAM_HEAP_SIZE, "hpsram");
#endif
}
#else
static void show_memory_info(void)
{
    uint32_t totalsize = 0;
    uint32_t freesize = 0;
    uint32_t minfreesize = 0;

    totalsize = xPortGetTotalHeapSize();
    freesize = xPortGetFreeHeapSize();
    minfreesize = xPortGetMinimumEverFreeHeapSize();

    printf( "\n       KiB Mem: %5u total, %5u used, %5u free, %5u min_free\n",
            totalsize >> 10, (totalsize - freesize) >> 10, freesize >> 10, minfreesize >> 10);
    printf( "       Total Heap Size : %8u Bytes    ( 100.00%% )\n"
            "                  Free : %8u Bytes    ( %6.2f%% )\n"
            "              Min Free : %8u Bytes    ( %6.2f%% )\n",
            totalsize,
            freesize, freesize * 100.0 / totalsize,
            minfreesize, minfreesize * 100.0 / totalsize);
}
#endif

static int exit_flag = 0;
static int gDelay = 0;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
static int flag = 0 ;
static uint64_t irqruntotal = 0;
static float irqoccu = 0.00;
static int top_i = 0;
#endif
typedef struct {
    uint32_t taskid;
    uint32_t cpuid;
    uint32_t counter;
    uint32_t status;
    struct list_head list;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
    uint64_t irqtime;
#endif
} tasklist_t;
static LIST_HEAD(gTaskListHead);
static unsigned int gLastCPUCounter[configNR_CPUS] = {0};
static unsigned int gNewCPUCounter[configNR_CPUS] = {0};
static unsigned int gLastContextSwitches[configNR_CPUS] = {0};
static unsigned int gNewContextSwitches[configNR_CPUS] = {0};

static tasklist_t *tasklist_find(TaskStatus_t *task)
{
    tasklist_t *t = NULL;

    list_for_each_entry(t, &gTaskListHead, list) {
#ifdef CONFIG_SMP
        if (t->cpuid == task->bind_cpu &&
            t->taskid == task->xTaskNumber)
            return t;
#else
        if (t->taskid == task->xTaskNumber)
            return t;
#endif
    }
    return NULL;
}

static void tasklist_insert(TaskStatus_t *task)
{
    tasklist_t *t = NULL;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
    xTaskHandle tcb_irq = task->xHandle;
#endif
    t = tasklist_find(task);
    if (t != NULL) {
        t->counter = task->ulRunTimeCounter;
        t->status = 1;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
        t->irqtime = get_tcb_irqtotaltime(tcb_irq);
#endif
        return;
    }
    t = pvPortMalloc(sizeof(tasklist_t));
    if (!t) {
        printf("no memory\n");
        return;
    }
    INIT_LIST_HEAD(&t->list);
    t->taskid = task->xTaskNumber;
#ifdef CONFIG_SMP
    t->cpuid = task->bind_cpu;
#else
    t->cpuid = 0;
#endif
    t->counter = task->ulRunTimeCounter;
    t->status = 1;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
    t->irqtime = get_tcb_irqtotaltime(tcb_irq);
#endif
    list_add_tail(&t->list, &gTaskListHead);
}

static void tasklist_clear_status(void)
{
    tasklist_t *t = NULL;
    list_for_each_entry(t, &gTaskListHead, list) {
        t->status = 0;
    }
}

static void tasklist_update_info(void)
{
    tasklist_t *t = NULL, *tmp = NULL;
    list_for_each_entry_safe(t, tmp, &gTaskListHead, list) {
        if (t->status == 0) {
            list_del(&t->list);
            vPortFree(t);
        }
    }
}

__attribute__((weak)) TickType_t xTaskGetTickCountCore( uint32_t cpuid )
{
	return xTaskGetTickCount();
}

__attribute__((weak)) TickType_t xTaskGetContextSwitchesCore( uint32_t cpuid )
{
#ifndef CONFIG_SMP
extern uint32_t ulTaskSwitchedInTime;
    return ulTaskSwitchedInTime;
#else
    return 0;
#endif
}

static void tasklist_update_cpu_counter(void)
{
    int i;
    for (i = 0; i< configNR_CPUS; i++)
        gLastCPUCounter[i] = gNewCPUCounter[i];
}

static void tasklist_update_new_cpu_counter(void)
{
    int i;
    for (i = 0; i< configNR_CPUS; i++)
        gNewCPUCounter[i] = xTaskGetTickCountCore(i);
}

TickType_t xTaskGetContextSwitchesCore( uint32_t cpuid );
static void tasklist_update_ctxt(void)
{
    int i;
    for (i = 0; i< configNR_CPUS; i++)
        gLastContextSwitches[i] = gNewContextSwitches[i];
}

static void tasklist_update_new_ctxt(void)
{
    int i;
    for (i = 0; i< configNR_CPUS; i++)
        gNewContextSwitches[i] = xTaskGetContextSwitchesCore(i);
}

static float tasklist_cal_cpu_usage(TaskStatus_t *task , int irqflag)
{
    uint32_t task_t = 0, task_c = 0;
    tasklist_t *t = NULL;

#ifdef CONFIG_COMMAND_IRQ_DEBUG
    uint64_t irq_t = 0;
    TaskHandle_t irq_tcb = task->xHandle;
#endif

#ifdef CONFIG_SMP
    int bind_cpu = task->bind_cpu;
#else
    int bind_cpu = 0;
#endif

    t = tasklist_find(task);
    task_c = gNewCPUCounter[bind_cpu] - gLastCPUCounter[bind_cpu];
    if (!t) {
        task_t = task->ulRunTimeCounter;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
        irq_t = get_tcb_irqtotaltime(irq_tcb);
#endif
    } else {
        task_t = task->ulRunTimeCounter - t->counter;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
        irq_t = get_tcb_irqtotaltime(irq_tcb) - t->irqtime;
#endif
    }

    /* If there is no Context Switches, set the Running task CPU usage to 100% */
    if (gNewContextSwitches[bind_cpu] == gLastContextSwitches[bind_cpu])
        if (task->eCurrentState == eRunning)
            task_t = task_c;

    if(task_t >= task_c)
        task_t = task_c;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
    if (irqflag == 1) {
        task_t = 1000 * OSTICK_TO_MS(task_t);
        task_c = 1000 * OSTICK_TO_MS(task_c);

        irqruntotal = irq_t;

        if (task_t != 0) {
            task_t = task_t - irq_t;
        }

        irqoccu = ((float)irq_t * 100) / ((float)task_c);
    }

#endif
    return ((float)task_t * 100) / ((float)task_c);
}

static void tasklist_clean_all(void)
{
    tasklist_t *t = NULL, *tmp = NULL;
    list_for_each_entry_safe(t, tmp, &gTaskListHead, list) {
            list_del(&t->list);
            vPortFree(t);
    }
    INIT_LIST_HEAD(&gTaskListHead);
    memset(gLastCPUCounter, 0, sizeof(gLastCPUCounter));
    memset(gNewCPUCounter, 0, sizeof(gNewCPUCounter));
}

int cmp( const void *a , const void *b  )
{
    TaskStatus_t *c = (TaskStatus_t *)a;
    TaskStatus_t *d = (TaskStatus_t *)b;

#ifdef CONFIG_SMP
    if(c->bind_cpu != d->bind_cpu)
        return c->bind_cpu - d->bind_cpu;
#endif

#if( configGENERATE_RUN_TIME_STATS == 1 )
    return (tasklist_cal_cpu_usage(d, 0) > tasklist_cal_cpu_usage(c, 0)) ? 1 : -1;
#else
    return 0;
#endif
}

static void monitor_start(void)
{
    char *stat = NULL;
    float stk_usage = 0.0f, cpu_usage = 0.0f;
    int pxStackSize = 0;
    void *entry = NULL;

    TaskStatus_t *pxTaskStatusArray;
    UBaseType_t uxArraySize, x, i;

#ifdef CONFIG_SMP
    uint32_t cpsr_flag;
    taskENTER_CRITICAL(cpsr_flag);
#else
    taskENTER_CRITICAL();
#endif

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

    if( pxTaskStatusArray != NULL )
    {
        memset(pxTaskStatusArray, 0, uxArraySize * sizeof( TaskStatus_t ));
        uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );
    }

#ifdef CONFIG_SMP
    taskEXIT_CRITICAL(cpsr_flag);
#else
    taskEXIT_CRITICAL();
#endif

    printf("\r\n");
    printf("    -----------------------------------------------TSK Usage Report----------------------------------------------------------------------\n");
    printf("\n");

    if( pxTaskStatusArray != NULL )
    {
#ifdef CONFIG_COMMAND_IRQ_DEBUG
        float irq_totaloccupancy = 0.00;
        uint64_t irq_totaltime = 0;
#endif

        tasklist_update_new_cpu_counter();
        tasklist_update_new_ctxt();

        qsort(pxTaskStatusArray, uxArraySize, sizeof(TaskStatus_t), cmp);

        for( i = 0; i < configNR_CPUS; i++ )
        {
#ifdef CONFIG_COMMAND_IRQ_DEBUG
            printf("        CPU%d:    num     entry       stat   prio     tcb       stacksize  stkusg    stackWaterMark  cputime       command     irqtime\n", (int)i);
#else
            printf("        CPU%d:    num     entry       stat   prio     tcb       stacksize  stkusg    stackWaterMark  cputime       command\n", (int)i);
#endif

            for( x = 0; x < uxArraySize; x++ )
            {
                TaskStatus_t *temp = &pxTaskStatusArray[x];
                uint8_t status = pxTaskStatusArray[ x ].eCurrentState;
                if (status == eRunning)
                {
                    stat = "running";
                }
                else if (status == eSuspended)
                {
                    stat = "suspend";
                }
                else if (status == eDeleted)
                {
                    stat = "delete";
                }
                else if (status == eReady)
                {
                    stat = "ready";
                }
                else if (status == eBlocked)
                {
                    stat = "block";
                }
                else
                {
                    stat = "unknown";
                }

#ifdef CONFIG_SMP
#if portSTACK_GROWTH < 0
                uint8_t * ptr = (uint8_t *)temp->pxTopOfStack;
                stk_usage = (float)(temp->pxStackSize - ((uint32_t) ptr - (uint32_t) temp->pxStackBase)) * 100 / (float)temp->pxStackSize;
#else
                uint8_t * ptr = (uint8_t *)temp->pxEndOfStack;
                stk_usage = (float)(temp->pxStackSize - ((uint32_t) temp->pxStackBase - (uint32_t)ptr)) * 100 / (float)temp->pxStackSize;
#endif
#endif

#if( configGENERATE_RUN_TIME_STATS == 1 )
                cpu_usage = tasklist_cal_cpu_usage(temp, 1);
#endif

#ifdef CONFIG_COMMAND_IRQ_DEBUG
                irq_totaltime += irqruntotal;
                irq_totaloccupancy += irqoccu;
                if (flag == 0) {
                    cpu_usage = 0.00;
                    irqruntotal = 0;
                    irqoccu = 0.00;
                    irq_totaltime = 0;
                }
#endif

#ifdef CONFIG_SMP
                pxStackSize = temp->pxStackSize;
                entry = temp->entry;
                if (temp->bind_cpu == i)
                {
#endif
                    printf("               %4ld   0x%08lx %9s %4d   0x%08lx  %8d    %5.2f%%     %8ld       %6.2f%%  %12s   ", \
                        (long int)temp->xTaskNumber,
                        (unsigned long)entry,
                        stat,
                        (int)temp->uxCurrentPriority,
                        (unsigned long)temp->xHandle,
                        pxStackSize,
                        stk_usage,
                        (long int)temp->usStackHighWaterMark * sizeof(StackType_t),
                        cpu_usage,
                        temp->pcTaskName);

#ifdef CONFIG_COMMAND_IRQ_DEBUG
                        printf("%llu us(%.2f%%)\n", \
                        irqruntotal,
                        irqoccu);
                        irqruntotal = 0;
#else
                    printf("\n");
#endif

#ifdef CONFIG_SMP
                }
                else
                    continue;
#endif
            }

#ifdef CONFIG_COMMAND_IRQ_DEBUG
            printf("      IRQ :      00   0x00000000     00000   00   0x00000000         0     0.00          0000         %.2f%%     totaltime   %llu us\n",irq_totaloccupancy,irq_totaltime);
            irq_totaloccupancy = 0.00;
            irq_totaltime = 0;
            if (top_i == 1) {
                printf("    -------------------------------------------------IRQ information----------------------------------------------------------------------\n");
                core_irq_ic_total_time();
                printf("    --------------------------------------------------------------------------------------------------------------------------------------\n");
            }
#endif

            printf("\n");
        }

        /* update tasklist info */
        tasklist_clear_status();
        for (x = 0; x < uxArraySize; x++) {
            tasklist_insert(&pxTaskStatusArray[x]);
        }
        tasklist_update_info();
        tasklist_update_cpu_counter();
        tasklist_update_ctxt();
    }
    printf("    ------------------------------------------------Memory information--------------------------------------------------------------------\n");
    show_memory_info();

    /*  printf("\n    Please enter Ctrl-C or 'q' to quit the command!\n"); */

#ifdef CONFIG_COMMAND_IRQ_DEBUG
    printf("\n    Please enter 'top -i' or 'top -d num' for view the interrupt details!\n");
    flag = 1;
#endif
    printf("\n    Please enter 'top_exit' to quit the command!\n");
    vPortFree(pxTaskStatusArray);
}

static void monitor_thread_entry(void * param)
{
    TickType_t timeout =  (gDelay*1000) / portTICK_PERIOD_MS;
    TickType_t last_delay = gDelay;

    while(1)
    {
        printf("\e[1;1H\e[2J");
        monitor_start();
        if (last_delay != gDelay) {
            timeout = (gDelay * 1000) / portTICK_PERIOD_MS;
            last_delay = gDelay;
        }
        vTaskDelay(timeout);
        if(exit_flag == 1)
        {
            break;
        }
    }
    tasklist_clean_all();
    vTaskDelete(NULL);
}

int cmd_top(int argc, char ** argv)
{
    static TaskHandle_t perf_task = NULL;
    portBASE_TYPE ret;
    int c = 0;
    int delay = 0;
    optind = 0;
    gDelay = 3;

    while ((c = getopt(argc, argv, "d:it")) != -1) {
        switch (c) {
        case 'd':
            delay = atoi(optarg);
            if (delay <= 0) {
              printf("Invalid delay value: %s\n", optarg);
              return -1;
            } else {
                gDelay = delay;
            }
            break;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
        case 'i':
            if (top_i == 0) {
                top_i = 1;
            } else {
                top_i = 0;
            }
            break;
#endif
        default:
            break;
        }
    }
    if (exit_flag == 1) {
        perf_task = NULL;
        exit_flag = 0;
    }
    if (perf_task == NULL) {
	ret = xTaskCreate(monitor_thread_entry, "top", 4096, NULL, configAPPLICATION_NORMAL_PRIORITY, &perf_task);
	if (ret != pdPASS) {
            printf("Error creating task, status was %d\n", (int)ret);
            return -1;
        }
    }
#if 0
#if defined(CONFIG_COMPONENT_CLI) || defined(CONFIG_COMPONENT_FINSH_CLI)
    while(1)
    {
        char cRxed = 0;

        cRxed = getchar();
        if(cRxed == 'q' || cRxed == 3)
        {
            exit_flag = 1;
            return 0;
        }
    }
#endif
#endif
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_top, top, Performance monitor);

static int cmd_top_exit(int argc, char **argv)
{
    exit_flag = 1;
#ifdef CONFIG_COMMAND_IRQ_DEBUG
    flag = 0;
#endif
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_top_exit, top_exit, Top Exit);
