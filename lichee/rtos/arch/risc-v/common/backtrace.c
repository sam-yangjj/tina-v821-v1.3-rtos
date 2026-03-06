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
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <context.h>
#include <backtrace.h>
#ifdef CONFIG_COMPONENTS_KALLSYMS
#include <kallsyms.h>
#endif

#include <hal_interrupt.h>

#define CONFIG_MELIS_RUN_IN_S_MODE

#if defined(__GNUC__)
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#error "Can not support big-endian!"
#endif
#endif

#define printk printf
//#define BACKTRACE_DEBUG

#ifndef BACKTRACE_DEBUG
#define backtrace_debug(fmt, ...)
#else
#define backtrace_debug(fmt, ...) \
    printk("[%s:%d]:"fmt, __func__, __LINE__, ##__VA_ARGS__)
#endif

#define BT_SCAN_MAX_LIMIT   0x2000
#define BT_LEVEL_LIMIT    64

#define PC2ADDR(pc)          ((char *)(((uintptr_t)(pc)) & 0xfffffffe))

#define PRINT_CALL(print_func, fmt, ...) \
    if (print_func) { \
        print_func(fmt, ##__VA_ARGS__); \
    } \

#define IS_VALID_TEXT_ADDRESS(pc)     backtrace_check_address((uintptr_t)(pc))

__attribute__((weak)) const backtrace_valid_address_t bt_valid_addr[] = {
    { .start = 0UL, .end = ~0UL },
    { },
};

int backtrace_check_address(uintptr_t pc)
{
    int i;
    for (i = 0; bt_valid_addr[i].end; i++) {
        if (pc >= bt_valid_addr[i].start && pc <= bt_valid_addr[i].end)
        return 1;
    }
    return 0;
}

#define insn_length(x) \
    (((x) & 0x03) < 0x03 ? 2 : \
     ((x) & 0x1f) < 0x1f ? 4 : \
     ((x) & 0x3f) < 0x3f ? 6 : \
     8)

#define BITS(x, high, low) ((x) & (((1<<((high)-(low)+1))-1) << (low)))
#define BITS_SHIFT(x, high, low) (((x) >> (low)) & ((1<<((high)-(low)+1))-1))
#define SIGN_EXTEND(val, topbit) (BIT(val, topbit) ? ((val) | (0xffffffff << (topbit))) : (val))

typedef struct
{
    switch_ctx_regs_t regs_ctx;
} switch_context_t;

enum task_states
{
    TASK_SUSPEND = 0,
    TASK_READY,
    TASK_INTERRUPTED,
    TASK_RUNNING,
};

extern char *_mmu_text_start;
extern char *_mmu_text_end;

extern void ret_from_create_c(void);
extern void prvTaskExitError(void);

static void *get_task_stack_bottom(void)
{
    return &prvTaskExitError;
}

static void *find_task(char *name)
{
    TaskStatus_t *pxTaskStatusArray;
    UBaseType_t uxArraySize, x;
    TaskStatus_t *temp, *ret = NULL;
    UBaseType_t xTaskNumber = 0;
    char *err = NULL;

    if (hal_interrupt_get_nest() == 0)
        taskENTER_CRITICAL();

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    if (!pxTaskStatusArray)
        return ret;

    memset(pxTaskStatusArray, 0, uxArraySize * sizeof(TaskStatus_t));

    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);

    for (x = 0; x < uxArraySize; x++) {
        temp = &pxTaskStatusArray[x];
        if (strspn(name, "0123456789") == strlen(name))
            xTaskNumber = strtoul(name, &err, 0);
        else
            xTaskNumber = 0;
        if (strcmp(name, temp->pcTaskName) && (xTaskNumber != temp->xTaskNumber))
            continue;
        else {
            ret = temp;
            break;
        }
    }

    vPortFree(pxTaskStatusArray);

    if (hal_interrupt_get_nest() == 0)
        taskEXIT_CRITICAL();

    return ret;
}

static int check_task_is_running(void *task)
{
    TaskHandle_t thread = (TaskHandle_t)task;

    if (thread == NULL)
        return TASK_SUSPEND;
    if (thread == xTaskGetCurrentTaskHandle() && hal_interrupt_get_nest() == 0)
        return TASK_RUNNING;
    else if (thread == xTaskGetCurrentTaskHandle())
        return TASK_INTERRUPTED;
    else if (eTaskGetState(thread) == eReady)
        return TASK_READY;
    else
        return TASK_SUSPEND;
}

/*
 * convert long to string
 */
static char *long2str(long num, char *str)
{
    char         index[] = "0123456789ABCDEF";
    unsigned long usnum   = (unsigned long)num;

    str[7] = index[usnum % 16];
    usnum /= 16;
    str[6] = index[usnum % 16];
    usnum /= 16;
    str[5] = index[usnum % 16];
    usnum /= 16;
    str[4] = index[usnum % 16];
    usnum /= 16;
    str[3] = index[usnum % 16];
    usnum /= 16;
    str[2] = index[usnum % 16];
    usnum /= 16;
    str[1] = index[usnum % 16];
    usnum /= 16;
    str[0] = index[usnum % 16];
    usnum /= 16;

    return str;
}

static void print_backtrace(print_function print_func, unsigned long addr)
{
    char backtrace_output_buf[] = "backtrace : 0X         ";
#ifdef CONFIG_COMPONENTS_KALLSYMS
    char sym_buffer[KSYM_NAME_LEN];
#endif
    if (print_func) {
        long2str(addr, &backtrace_output_buf[14]);
        print_func(backtrace_output_buf);
#ifdef CONFIG_COMPONENTS_KALLSYMS
        sprint_symbol(sym_buffer, addr);
        print_func(" : %s", sym_buffer);
#endif
        print_func("\r\n");
    }
}

static void get_register_from_task_stack(void *context, char **PC, char **LR, long **SP)
{
    switch_ctx_regs_t *task_ctx;

    task_ctx = (switch_ctx_regs_t *)context;

    *PC = (char *)task_ctx->mepc;
    *LR = (char *)task_ctx->ra;
    *SP = (long *)(task_ctx->fp);
}

static int find_lr_offset(char *LR, print_function print_func)
{
    char *LR_fixed;
    unsigned short ins16;
    //char backtrace_output_buf[] = "backtrace : 0X         \r\n";
    int offset = 4;

#if 0
    unsigned long *irq_entry = (unsigned long *)&riscv_cpu_handle_interrupt;

    irq_entry ++;
#endif

    LR_fixed = LR;

#if 0
    /* meet irq_entry, it is irq handler exit address. */
    if (LR_fixed == PC2ADDR(irq_entry))
    {
        if (print_func != NULL)
        {
            long2str((long)irq_entry, &backtrace_output_buf[14]);
            print_func(backtrace_output_buf);
        }
        return 0;
    }
#endif

    if (LR_fixed == PC2ADDR(get_task_stack_bottom()))
    {
        print_backtrace(print_func, (unsigned long)get_task_stack_bottom());
        return 0;
    }

    if (IS_VALID_TEXT_ADDRESS(LR_fixed - 4) == 0)
        return 0;

    ins16 = *(unsigned short *)(LR_fixed - 4);
    offset = insn_length(ins16);

    print_backtrace(print_func, (unsigned long)LR_fixed - offset);

    return offset;
}

/**
 * @param offset :offset will be update if find ra push tack
 * @return int :if meet alloc stack frame instruction return =0 else = -1
 */
int riscv_ins32_get_push_lr_framesize(unsigned int inst, int *offset)
{
    int ret = -1;
    int stack_width = sizeof(long);

    if ((inst & 0x01FFF07F) == 0x113023)
    {
        /* sd ra, (offset)sp  */
        int immed = (inst & 0xF80);
        immed >>= 7;
        immed |= ((inst & 0xFE000000) >> 25) << 5;
        if (((immed >> 11) & 0x01) != 0)
            immed = 0xFFF - immed + 1;
        *offset = immed / stack_width;
        ret = -1;
        backtrace_debug("inst:0x%x offset:%d\n", inst, *offset);
    }
    else if ((inst & 0x01FFF07F) == 0x112023)
    {
        /* sw ra, (offset)sp  */
        int immed = (inst & 0xF80);
        immed >>= 7;
        immed |= ((inst & 0xFE000000) >> 25) << 5;
        if (((immed >> 11) & 0x01) != 0)
            immed = 0xFFF - immed + 1;
        *offset = immed / stack_width;
        ret = -1;
        backtrace_debug("inst:0x%x offset:%d\n", inst, *offset);
    }
    else if ((inst & 0x000FFFFF) == 0x10113)
    {
        /*  addi sp, sp, #imm  */
        int immed = BITS(inst, 31, 20);
        immed >>= 20;
        immed &= 0xFFF;
        if ((immed >> 11) != 0) {
            immed = 0xFFF - immed + 1;
            ret = 0;
        }
        else
            ret = -1;
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }
    else if ((inst & 0x000FFFFF) == 0x1011B)
    {
        /*  addiw sp, sp, #imm  */
        int immed = BITS(inst, 31, 20);
        immed >>= 20;
        immed &= 0xFFF;
        if ((immed >> 11) != 0)
        {
            immed = 0xFFF - immed + 1;
            ret = 0;
        }
        else
            ret = -1;
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }

    return ret;
}

static int riscv_ins16_get_push_lr_framesize(unsigned short inst, int *offset)
{
    int ret = -1;
    int stack_width = sizeof(long);

    if ((inst & 0xE07F) == 0xE006)
    {
        /* c.sd ra, (offset)sp  */
        int immed_6_8 = (inst >> 7) & 0x07 ;
        int immed_3_5 = (inst >> 10) & 0x07 ;
        int immed = immed_6_8 << 6 | immed_3_5 << 3;
        *offset = immed / stack_width;

        backtrace_debug("inst:0x%x offset:%d\n", inst, *offset);
        ret = -1;
    }
    else if ((inst & 0xE07F) == 0xC006)
    {
        /* c.sw ra, (offset)sp  */
        int immed_6_7 = (inst >> 7) & 0x03 ;
        int immed_2_5 = (inst >> 9) & 0x0F ;
        int immed = immed_6_7 << 6 | immed_2_5 << 2;
        *offset = immed / stack_width;

        backtrace_debug("inst:0x%x offset:%d\n", inst, *offset);
        ret = -1;
    }
    else if ((inst & 0xEF83) == 0x6101)
    {
        /*  c.addi16sp #imm  */
        int immed_5 = (inst >> 2) & 0x01 ;
        int immed_7_8 = (inst >> 3) & 0x3 ;
        int immed_6 = (inst >> 5) & 0x1 ;
        int immed_4 = (inst >> 6) & 0x1 ;
        int immed_9 = (inst >> 12) & 0x1 ;
        int immed = immed_5 << 5 | immed_7_8 << 7 | immed_6 << 6 | immed_4 << 4 | immed_9 << 9;

        if ((immed >> 9) != 0)
        {
            immed = 0x3FF - immed + 1;
            ret = 0;
        }
        else
        {
            ret = -1;
        }
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }
    else if ((inst & 0xEF03) == 0x101)
    {
        /*  c.addi sp, sp, #imm  */
        int immed_0_4 = (inst >> 2) & 0x1F ;
        int immed_5 = (inst >> 12) & 0x1 ;
        int immed = immed_5 << 5 | immed_0_4;

        if ((immed >> 5) != 0)
        {
            immed = 0x3F - immed + 1;
            ret = 0;
        }
        else
        {
            ret = -1;
        }
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }
    else if ((inst & 0xEF03) == 0x2101)
    {
        /*  c.addiw sp, #imm  */
        int immed_0_4 = (inst >> 2) & 0x1F ;
        int immed_5 = (inst >> 12) & 0x1 ;
        int immed = immed_5 << 5 | immed_0_4;

        if ((immed >> 5) != 0)
        {
            immed = 0x3F - immed + 1;
            ret = 0;
        }
        else
        {
            ret = -1;
        }
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }
#if 0
    else if ((inst & 0xE01F) == 0x0)
    {
        /*  c.addi4spn #imm  */
        return 0;
    }
#endif

    backtrace_debug("ret = %d\n", ret);
    return ret;
}

static int riscv_ins32_backtrace_return_pop(unsigned int inst)
{

    backtrace_debug("inst:0x%x\n", inst);

#if 0
    /*  addi sp, sp, #imm  */
    if ((inst & 0x000FFFFF) == 0x10113)
    {
        int immed = BITS(inst, 31, 20);
        immed >>= 20;
        immed &= 0xFFF;
        if ((immed >> 11) != 0)
        {
            immed = 0xFFF - immed + 1;
        }
        return immed / (int)sizeof(long);
    }
#endif
    /* ret */
    if ((inst) == 0x00008067)
    {
        return 0;
    }

    return -1;
}

static int riscv_ins16_backtrace_return_pop(unsigned short inst)
{
    int ret = -1;

    if ((inst) == 0x8082)
    {
        ret = 0;;
    }

    backtrace_debug("inst:0x%x, ret = %d\n", inst, ret);
    return ret;
}

int riscv_ins32_backtrace_stask_push(unsigned int inst)
{
    int ret = 0;
    int stack_width = sizeof(long);

    if ((inst & 0x000FFFFF) == 0x10113)
    {
        /*  addi sp, sp, #imm  */
        int immed = BITS(inst, 31, 20);
        immed >>= 20;
        immed &= 0xFFF;
        if ((immed >> 11) != 0)
        {
            immed |= ~0xFFF;
        }
        ret = immed / stack_width;
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }
    else if ((inst & 0x000FFFFF) == 0x1011B)
    {
        /*  addiw sp, sp, #imm  */
        int immed = BITS(inst, 31, 20);
        immed >>= 20;
        immed &= 0xFFF;
        if ((immed >> 11) != 0)
        {
            immed |= ~0xFFF;
        }
        ret = immed / stack_width;
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }

    backtrace_debug("inst:0x%x, ret = %d\n", inst, ret);

    return ret;
}
static int riscv_ins16_backtrace_stask_push(unsigned int inst)
{
    int ret = 0;
    int stack_width = sizeof(long);
    if ((inst & 0xEF83) == 0x6101)
    {
        /*  c.addi16sp #imm  */
        int immed_4 = (inst >> 6) & 0x01;
        int immed_5 = (inst >> 2) & 0x01;
        int immed_6 = (inst >> 5) & 0x01;
        int immed_7_8 = (inst >> 3) & 0x3;
        int immed_9 = (inst >> 12) & 0x1;
        int immed = (immed_4 << 4) | (immed_5 << 5) | (immed_6 << 6) | (immed_7_8 << 7) | (immed_9 << 9);
        if ((immed >> 9) != 0)
        {
            immed |= ~0x3FF;
        }
        ret = immed / stack_width;
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }
    else if ((inst & 0xEF03) == 0x101)
    {
        /*  c.addi sp, sp, #imm  */
        int immed_5 = (inst >> 12) & 0x01;
        int immed_0_4 = (inst >> 2) & 0x1F;
        int immed = (immed_0_4) | (immed_5 << 5);
        if ((immed >> 5) != 0)
        {
            immed |= ~0x3F;
        }
        ret = immed / stack_width;
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }
    else if ((inst & 0xEF03) == 0x2101)
    {
        /*  c.addiw sp, #imm  */
        int immed_5 = (inst >> 12) & 0x01;
        int immed_0_4 = (inst >> 2) & 0x1F;
        int immed = (immed_0_4) | (immed_5 << 5);
        if ((immed >> 5) != 0)
        {
            immed |= ~0x3F;
        }
        ret = immed / stack_width;
        backtrace_debug("inst:0x%x imm:%d\n", inst, immed);
    }
#if 0
    else if ((inst & 0xE01F) == 0x0)
    {
        /*  c.addi4spn #imm  */
        return 0;
    }
#endif

    backtrace_debug("inst:0x%x, ret = %d\n", inst, ret);

    return ret;
}

static int riscv_ins32_backtrace_stack_pop(unsigned int inst)
{
    int ret = -1;
    int stack_width = sizeof(long);

    /*  addi sp, sp, #imm  */
    if ((inst & 0x000FFFFF) == 0x10113)
    {
        int immed = BITS(inst, 31, 20);
        immed >>= 20;
        immed &= 0xFFF;
        if ((immed >> 11) != 0)
        {
            ret = -1;
        }
        else
        {
            immed = 0xFFF - immed + 1;
            ret = immed / stack_width;
        }
    }
    else if ((inst & 0x000FFFFF) == 0x1011B)
    {
        /*  addiw sp, sp, #imm  */
        int immed = BITS(inst, 31, 20);
        immed >>= 20;
        immed &= 0xFFF;
        if ((immed >> 11) != 0)
        {
            ret = -1;
        }
        else
        {
            immed = 0xFFF - immed + 1;
            ret = immed / stack_width;
        }
    }

    backtrace_debug("inst:0x%x\n", inst);
    return ret;
}

static int riscv_ins16_backtrace_stack_pop(unsigned short inst)
{
    int ret = -1;
    int stack_width = sizeof(long);

    backtrace_debug("inst:0x%x\n", inst);
    if ((inst & 0xEF83) == 0x6101)
    {
        /*  c.addi16sp #imm  */
        int immed_4 = (inst >> 6) & 0x01;
        int immed_5 = (inst >> 2) & 0x01;
        int immed_6 = (inst >> 5) & 0x01;
        int immed_7_8 = (inst >> 3) & 0x3;
        int immed_9 = (inst >> 12) & 0x1;
        int immed = (immed_4 << 4) | (immed_5 << 5) | (immed_6 << 6) | (immed_7_8 << 7) | (immed_9 << 9);
        if ((immed >> 9) != 0)
        {
            immed = 0x3FF - immed + 1;
        }
        ret = immed / stack_width;
    }
    else if ((inst & 0xEF03) == 0x101)
    {
        /*  c.addi sp, sp, #imm  */
        int immed_5 = (inst >> 12) & 0x01;
        int immed_0_4 = (inst >> 2) & 0x1F;
        int immed = (immed_0_4) | (immed_5 << 5);
        if ((immed >> 5) != 0)
        {
            ret = -1;
        }
        else
        {
            immed = 0x3F - immed + 1;
        }
        ret = immed / stack_width;
    }
    else if ((inst & 0xEF03) == 0x2101)
    {
        /*  c.addiw sp, #imm  */
        int immed_5 = (inst >> 12) & 0x01;
        int immed_0_4 = (inst >> 2) & 0x1F;
        int immed = (immed_0_4) | (immed_5 << 5);
        if ((immed >> 5) != 0)
        {
            ret = -1;
        }
        else
        {
            immed = 0x3F - immed + 1;
        }
        ret = immed / stack_width;
    }
#if 0
    else if ((inst & 0xE01F) == 0x0)
    {
        /*  c.addi4spn #imm  */
        return 0;
    }
#endif

    backtrace_debug("inst:0x%x\n", inst);
    return ret;
}

#define USE_LR_REG_DATA_TO_BACKTRACE 6

static int riscv_backtrace_from_stack(long **pSP, char **pPC, char **pLR,
                                      print_function print_func, int is_top_level)
{
    char *parse_addr = NULL;
    long  *SP = *pSP;
    char *PC = *pPC;
    char *LR = *pLR;
    int i;
    int temp;
    int framesize = 0;
    int offset = 0;
    unsigned int ins32 = 0;
    unsigned short ins16 = 0;
    unsigned short ins16_h = 0;
    unsigned short ins16_l = 0;
    unsigned short ins16_ll = 0;

    if (SP == get_task_stack_bottom())
    {
        backtrace_debug("meet stack buttom!\r\n");
        print_backtrace(print_func, (unsigned long)get_task_stack_bottom());
        return 1;
    }

#ifndef CONFIG_COMPONENTS_KALLSYMS
    for (i = 2; i < BT_SCAN_MAX_LIMIT; i += 2)
    {
        int result = 0;
        parse_addr = PC - i;
        if (IS_VALID_TEXT_ADDRESS(parse_addr) == 0)
        {
            if (print_func)
                print_func("backtrace fail!\r\n");
            return -1;
        }
        ins16_h = *(unsigned short *)parse_addr;

        if (IS_VALID_TEXT_ADDRESS(parse_addr - 2) == 0)
        {
            if (print_func)
                print_func("backtrace fail!\r\n");
            return -1;
        }
        ins16_l = *(unsigned short *)(parse_addr - 2);

        if (IS_VALID_TEXT_ADDRESS(parse_addr - 4) == 0)
        {
            if (print_func)
            {
                print_func("backtrace fail!\r\n");
            }
            return -1;
        }
        ins16_ll = *(unsigned short *)(parse_addr - 4);

        backtrace_debug("parse_addr = %p:0x%x:0x%x\r\n", parse_addr, ins16_l, ins16_h);

        if ((insn_length(ins16_l) == 4) && (insn_length(ins16_ll) == 4 ))
        {
            ins16 = ins16_h;
            result = riscv_ins16_get_push_lr_framesize(ins16, &offset);
        }
        else if (insn_length(ins16_l) == 4)
        {
            ins32 = (ins16_h << 16) | ins16_l;
            result = riscv_ins32_get_push_lr_framesize(ins32, &offset);
            i += 2;
        }
        else
        {
            ins16 = ins16_h;
            result = riscv_ins16_get_push_lr_framesize(ins16, &offset);
        }

        if (result >= 0)
            break;
    }

    parse_addr = PC - i;

    backtrace_debug("boundary_addr = %p, start_pc = %p, offset = %d\n", parse_addr, PC, offset);

    if (i == BT_SCAN_MAX_LIMIT)
    {
        if (print_func != NULL)
            print_func("backtrace fail!\r\n");
        return -1;
    }
#else
    if (!kallsyms_lookup_size_offset((unsigned long)PC, NULL, (unsigned long *)&offset)) {
         if (print_func != NULL)
         {
             print_func("backtrace fail!\r\n");
         }
         return -1;
    }
    parse_addr = (char *)((unsigned long)PC - offset);
    backtrace_debug("function boundary: %p for %p\n", parse_addr, PC);
    offset = -1;
    for (i = 0; parse_addr + i < PC; i += 2)
    {
        int result = 0;
        char *parse = parse_addr + i;
        if (IS_VALID_TEXT_ADDRESS(parse) == 0)
        {
            if (print_func)
            {
                print_func("backtrace fail!\r\n");
            }
            return -1;
        }
        ins16_h = *(unsigned short *)parse;

        if (IS_VALID_TEXT_ADDRESS(parse- 2) == 0)
        {
            if (print_func)
            {
                print_func("backtrace fail!\r\n");
            }
            return -1;
        }
        ins16_l = *(unsigned short *)(parse - 2);

        if (IS_VALID_TEXT_ADDRESS(parse_addr - 4) == 0)
        {
            if (print_func)
            {
                print_func("backtrace fail!\r\n");
            }
            return -1;
        }
        ins16_ll = *(unsigned short *)(parse_addr - 4);

        backtrace_debug("parse = %p:", parse);

        if ((insn_length(ins16_l) == 4) && (insn_length(ins16_ll) == 4 ))
        {
            ins16 = ins16_h;
            result = riscv_ins16_get_push_lr_framesize(ins16, &offset);
        }
        else if (insn_length(ins16_l) == 4)
        {
            ins32 = (ins16_h << 16) | ins16_l;
            result = riscv_ins32_get_push_lr_framesize(ins32, &offset);
            i += 2;
        }
        else
        {
            ins16 = ins16_h;
            result = riscv_ins16_get_push_lr_framesize(ins16, &offset);
        }

        if (offset >= 0)
        {
            break;
        }
    }

    /* PC maybe in leaf function, we directily use ra */
    backtrace_debug("parse_addr = %p, PC = %p, offset = %d\n", parse_addr, PC, offset);
#endif

    framesize = 0;
    for (i = 0; parse_addr + i < PC; i += 2)
    {
        temp = 0;
        if (IS_VALID_TEXT_ADDRESS(parse_addr + i) == 0)
        {
            if (print_func != NULL)
                print_func("backtrace fail!\r\n");
            return -1;
        }
        ins16_l = *(unsigned short *)(parse_addr + i);

        if (IS_VALID_TEXT_ADDRESS(parse_addr + i + 2) == 0)
        {
            if (print_func != NULL)
                print_func("backtrace fail!\r\n");
            return -1;
        }
        ins16_h = *(unsigned short *)(parse_addr + i + 2);

        if (insn_length(ins16_l) == 4 || ins16_l == 0)
        {
            ins32 = (ins16_h << 16) | ins16_l;
            temp = riscv_ins32_backtrace_stask_push(ins32);
            i += 2;
        }
        else
        {
            ins16 = ins16_l;
            temp = riscv_ins16_backtrace_stask_push(ins16);
        }

        if (temp < 0)
            framesize += -temp;
    }

    backtrace_debug("framesize = %d, SP = %p\n", framesize, SP);

#ifdef CONFIG_COMPONENTS_KALLSYMS
    if (offset == -1) {
        LR  = *pLR;
        offset = 0;
        goto leaf_func;
    }

#endif
    if (!offset)
        return -1;

    if (IS_VALID_TEXT_ADDRESS(SP + offset) == 0)
    {
        if (print_func != NULL)
            print_func("backtrace : invalid sp(%p)\r\n",SP + offset);
        return -1;
    }

    LR  = (char *) * (SP + offset);
#ifdef CONFIG_COMPONENTS_KALLSYMS
leaf_func:
#endif
    if (IS_VALID_TEXT_ADDRESS(LR) == 0)
    {
        if (print_func != NULL)
            print_func("backtrace : invalid lr(%p)\r\n", LR);
        return -1;
    }

    *pSP   = SP + framesize;
    offset = find_lr_offset(LR, print_func);
    *pPC   = LR - offset;

    backtrace_debug("new SP = %p, offset = %d, new PC = %p\n\n\n", *pSP, offset, *pPC);

#ifndef CONFIG_COMPONENTS_KALLSYMS
    if (!offset)
    {
        //invalid instruction offset
        return 1;
    }

    if (!is_top_level)
    {
        return 0;
    }

    if (!IS_VALID_TEXT_ADDRESS(*pLR))
        return 0;

    if (LR != *pLR)
    {
        //saved LR register value in stack is different from LR register value when thread is interrupted,
        //but the two address is both valid, so we need to provide two possible backtrace information.
        //This situation generally occur in emebedded system when the top level function does't has frame.
        return USE_LR_REG_DATA_TO_BACKTRACE;
    }

    return 0;
#else
    return offset == 0 ? 1 : 0;
#endif
}

static int riscv_backtrace_from_lr(long **pSP, char **pPC, char *LR,
                                   print_function print_func)
{
    long *SP = *pSP;
    char *PC = *pPC;
    char *parse_addr = NULL;
    int i;
    int temp;
    int framesize = 0;
    int offset;
    int result = 0;
    unsigned int ins32 = 0;
    unsigned short ins16 = 0;
    unsigned short ins16_h = 0;
    unsigned short ins16_l = 0;

    if (IS_VALID_TEXT_ADDRESS(PC) == 0)
    {
        if (IS_VALID_TEXT_ADDRESS(LR) == 0)
        {
            if (print_func != NULL)
            {
                print_func("backtrace : invalid lr\r\n");
            }
            return -1;
        }
        offset = find_lr_offset(LR, print_func);
        PC     = LR - offset;
        *pPC   = PC;
        return offset == 0 ? 1 : 0;
    }

    for (i = 0; i < BT_SCAN_MAX_LIMIT; i += 2)
    {
        parse_addr = PC + i;

        if (IS_VALID_TEXT_ADDRESS(parse_addr) == 0)
        {
            if (print_func)
            {
                print_func("backtrace fail!\r\n");
            }
            return -1;
        }

        if (IS_VALID_TEXT_ADDRESS(parse_addr + 2) == 0)
        {
            if (print_func)
            {
                print_func("backtrace fail!\r\n");
            }
            return -1;
        }

        ins16_l = *(unsigned short *)parse_addr;
        ins16_h = *(unsigned short *)(parse_addr + 2);

        if (insn_length(ins16_l) == 4 || ins16_l == 0)
        {
            ins32 = (ins16_h << 16) | ins16_l;
            result = riscv_ins32_backtrace_return_pop(ins32);
            i += 2;
            parse_addr -= 4;
        }
        else
        {
            ins16 = ins16_l;
            result = riscv_ins16_backtrace_return_pop(ins16);
            parse_addr -= 2;
        }

        if (result >= 0)
        {
            break;
        }
    }

    backtrace_debug("i = %d, parse_addr = %p, PC = %p, framesize = %d\n", i, parse_addr, PC, framesize);

    framesize = result;

    if (i == BT_SCAN_MAX_LIMIT)
    {
        if (print_func != NULL)
        {
            print_func("Backtrace fail!\r\n");
        }
        return -1;
    }

    for (i = 0; parse_addr - i >= PC; i += 2)
    {
        if (IS_VALID_TEXT_ADDRESS(parse_addr - i) == 0)
        {
            if (print_func != NULL)
            {
                print_func("Backtrace fail!\r\n");
            }
            return -1;
        }

        if (IS_VALID_TEXT_ADDRESS(parse_addr - i - 2) == 0)
        {
            if (print_func != NULL)
            {
                print_func("Backtrace fail!\r\n");
            }
            return -1;
        }

        ins16_l = *(unsigned short *)(parse_addr - i - 2);
        ins16_h = *(unsigned short *)(parse_addr - i);

        if (insn_length(ins16_l) == 4)
        {
            ins32 = (ins16_h << 16) | ins16_l;
            temp = riscv_ins32_backtrace_stack_pop(ins32);
            i += 2;
        }
        else
        {
            ins16 = ins16_h;
            temp = riscv_ins16_backtrace_stack_pop(ins16);
        }

        if (temp >= 0)
        {
            backtrace_debug("framesize add %d\n", temp);
            framesize += temp;
        }
    }

    backtrace_debug("i = %d, parse_addr = %p, PC = %p, SP = %p, framesize = %d\n", i, parse_addr, PC, SP, framesize);

    if (IS_VALID_TEXT_ADDRESS(LR) == 0)
    {
        if (print_func != NULL)
        {
            print_func("backtrace : invalid lr\r\n");
        }
        return -1;
    }
    *pSP   = SP + framesize;
    offset = find_lr_offset(LR, print_func);
    *pPC   = LR - offset;

    backtrace_debug("*pSP = %p, offset = %d, *pPC = %p\n", *pSP, offset, *pPC);

    return offset == 0 ? 1 : 0;
}

static int backtrace_from_stack(long **pSP, char **pPC, char **pLR,
                                print_function print_func, int is_top_level)
{
    if (IS_VALID_TEXT_ADDRESS(*pPC) == 0)
        return -1;

    return riscv_backtrace_from_stack(pSP, pPC, pLR, print_func, is_top_level);
}

static int backtrace_from_lr(long **pSP, char **pPC, char *LR,
                             print_function print_func)
{
    return riscv_backtrace_from_lr(pSP, pPC, LR, print_func);
}

/*
 * _backtrace function
 * @taskname: the task need to get call stack
 * @output:the call stack output buffer
 * @size:  the size of output
 * @offset:
 * @print_func: print function
 * @arg_cpsr: cpsr register value when use in exception mode
 * @arg_sp: sp register value when use in exception mode
 * @arg_pc: pc register value when use in exception mode
 * @arg_lr: lr register value when use in exception mode
 * @exception_mode: 0/1; 0:normal mode, 1:exception mode
 */
static int _backtrace(char *taskname, void *output[], int size, int offset, print_function print_func,
                      unsigned long arg_cpsr,
                      unsigned long arg_sp,
                      unsigned long arg_pc,
                      unsigned long arg_lr,
                      unsigned long exception_mode)
{
    char *PC = NULL;
    long  *SP = NULL;
    unsigned long CPSR = 0;
    int   level;
    int   ret;
    TaskStatus_t *task = NULL;
    char    *LR = NULL;
    int check_self = 0;
    char *top_level_pc = NULL;
    char *top_level_lr = NULL;
    char *top_level_sp = NULL;
    int use_lr_to_bt = 0;

    if (output && size > 0)
        memset(output, 0, size * sizeof(void *));

    if (taskname)
    {
        task = (TaskStatus_t *)find_task(taskname);
        if (task == NULL)
        {
            if (print_func)
                print_func("Task not found : %s\n", taskname);
            return 0;
        }
        get_register_from_task_stack((void *)task->pxTopOfStack, &PC, &LR, &SP);
        if (check_task_is_running(task) == TASK_RUNNING ||
            check_task_is_running(task) == TASK_INTERRUPTED)
            check_self = 1;
        if(strcmp( pcTaskGetName( NULL ), taskname ) == 0) {
            check_self = 1;
        }
    }

    if ((taskname == NULL || check_self == 1) && exception_mode != 1)
    {
        __asm__ volatile("mv %0, sp\n" : "=r"(SP));
        __asm__ volatile("auipc %0, 0\n" : "=r"(PC));
        __asm__ volatile("mv %0, ra\n" : "=r"(LR));
        __asm__ volatile("csrr %0, mstatus\n" : "=r"(CPSR));

        check_self = 1;
    }
    else if (exception_mode == 1)
    {
        LR = (char *)arg_lr;
        SP = (long *)arg_sp;
        PC = (char *)arg_pc;
        CPSR = arg_cpsr;
    }

    if (SP == NULL)
        return 0;

    backtrace_debug("Reg Sta :SP=%p PC=%p LR=%p, CPSR=0x%lx\r\n", SP, PC, LR, CPSR);
    ret = -1;
    level = 0;
    if (output)
    {
        if (level >= offset && level - offset < size)
            output[level - offset] = PC;
        if (level - offset >= size)
            goto out;
    }

    print_backtrace(print_func, (unsigned long)PC);

    backtrace_debug("try to backtrace from stack\r\n");
    top_level_pc = PC;
    top_level_lr = LR;
    top_level_sp = (char *)SP;
    for (level = 1; level < BT_LEVEL_LIMIT; level++)
    {
        if (level == 1)
            ret = backtrace_from_stack(&SP, &PC, &LR, print_func, 1);
        else
            ret = backtrace_from_stack(&SP, &PC, &LR, print_func, 0);

        if (ret == USE_LR_REG_DATA_TO_BACKTRACE)
        {
            use_lr_to_bt = 1;
            ret = 0;
        }

        if (ret != 0)
        {
            backtrace_debug("backtrace_from_stack failed, level: %d, ret: %d\n", level, ret);
            break;
        }
        if (output)
        {
            if (level >= offset && level - offset < size)
                output[level - offset] = PC;
            if (level - offset >= size)
                break;
        }
    }

    if (ret < 0 && (check_self == 0 || exception_mode == 1) && level == 1)
    {
        backtrace_debug("try to backtrace from lr\r\n");
        ret = backtrace_from_lr(&SP, &PC, LR, print_func);
        if (ret == 0)
        {
            for (; level < BT_LEVEL_LIMIT; level++)
            {
                ret = backtrace_from_stack(&SP, &PC, &LR, print_func, 0);
                if (ret != 0)
                    break;
                if (output)
                {
                    if (level >= offset && level - offset < size)
                        output[level - offset] = PC;
                    if (level - offset >= size)
                        break;
                }
            }
        }
    }

    if (use_lr_to_bt)
    {
        PC = top_level_pc;
        LR = top_level_lr;
        SP = (long *)top_level_sp;
        backtrace_debug("provide another backtrace info when we can't judge whether the frame of top level function exist!\n");

        if (print_func)
            print_func("Another possible backtrace(Use register value to get the caller of top level function):\n", taskname);

        if (output)
        {
            if (level >= offset && level - offset < size)
                output[level - offset] = PC;
            if (level - offset >= size)
                goto out;
        }

        print_backtrace(print_func, (unsigned long)PC);

        ret = backtrace_from_lr(&SP, &PC, LR, print_func);
        if (ret == 0)
        {
            for (; level < BT_LEVEL_LIMIT; level++)
            {
                ret = backtrace_from_stack(&SP, &PC, &LR, print_func, 0);
                if (ret != 0)
                    break;
                if (output)
                {
                    if (level >= offset && level - offset < size)
                        output[level - offset] = PC;
                    if (level - offset >= size)
                        break;
                }
            }
        }
    }
out:
    return level - offset < 0 ? 0 : level - offset;
}

int arch_backtrace(char *taskname, void *trace[], int size, int offset, print_function print_func)
{
    return _backtrace(taskname, trace, size, offset, print_func, 0, 0, 0, 0, 0);
}

int arch_backtrace_exception(print_function print_func,
                             unsigned long arg_cpsr,
                             unsigned long arg_sp,
                             unsigned long arg_pc,
                             unsigned long arg_lr)
{
    return _backtrace(NULL, NULL, 0, 0, print_func, arg_cpsr, arg_sp, arg_pc, arg_lr, 1);
}
