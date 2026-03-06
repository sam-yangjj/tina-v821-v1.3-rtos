/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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
#include <stdio.h>
#include <errno.h>

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
#include <fenv.h>
#endif

#include <os_test_log.h>
#include "fpu_test.h"
#include "fpu_ctx_test.h"

#include <../e90x/platform/platform.h>
#include <../e90x/irq_core.h>
#include <../e90x/clic.h>

#include <osal/hal_interrupt.h>

//#define DEBUG_E90X_IRQ_FPU_CTX_TEST

//#define DUMP_E90X_IRQ_FPU_CTX_TEST_INFO

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
#define PREEMPT_PRIORITY_BITS CLICINTCTL_NLBITS
#else
#define PREEMPT_PRIORITY_BITS 0
#endif

#define PREEMPT_PRIORITY_CNT (1 << PREEMPT_PRIORITY_BITS)

#define START_IRQ_ID CONFIG_E90X_START_INTERRUPT_ID

#define DEFAULT_INIT_INTEGET_PART 2

#define TEST_NAME_ARRAY_LEN 8
typedef struct
{
	char name[TEST_NAME_ARRAY_LEN];
	uint32_t irq_id;
	hal_irq_prio_t irq_priority;
	hal_irq_handler_t irq_handler;
	int is_executed;

	int result;
	uint32_t integer_part;
#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	int rounding_mode;
#endif

	void *prev;
	void *next;
} e90x_irq_fpu_ctx_test_t;

static e90x_irq_fpu_ctx_test_t g_irq_fpu_test[PREEMPT_PRIORITY_CNT];

#ifdef DUMP_E90X_IRQ_FPU_CTX_TEST_INFO
static void dump_e90x_irq_fpu_ctx_test(const e90x_irq_fpu_ctx_test_t *fpu_test)
{
	uint32_t reg_addr;

	os_test_info("e90x_irq_fpu_ctx_test: %p", fpu_test);
	os_test_info("name: '%s'", fpu_test->name);
	os_test_info("irq_id: %u", fpu_test->irq_id);
	os_test_info("irq_priority: %u", fpu_test->irq_priority);
	os_test_info("irq_handler: %p", fpu_test->irq_handler);
	os_test_info("integer_part: %u", fpu_test->integer_part);
#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	os_test_info("rounding_mode: %d", fpu_test->rounding_mode);
#endif
	os_test_info("prev: %p", fpu_test->prev);
	os_test_info("next: %p", fpu_test->next);

	reg_addr = PLAT_CLIC_BASE_ADDR + CLIC_INX_X_32BIT_REG_OFF(fpu_test->irq_id);
	os_test_info("CLIC reg(0x%08x): 0x%08x", reg_addr, hal_readl(reg_addr));
}
#endif

static void e90x_trigger_irq(int irq_id)
{
	hal_writeb(1, PLAT_CLIC_BASE_ADDR + CLIC_INT_X_IP_REG_OFF(irq_id));
	dsb();
}

hal_irqreturn_t low_priority_irq_handler(void *arg)
{
	int ret, is_middle_priority;
	uint32_t integer_part, fpu_reg_cnt;
	fpu_reg_t reg_buf_before_test[MAX_FPU_GENERAL_REG_NUM];
	fpu_reg_t reg_buf_after_test[MAX_FPU_GENERAL_REG_NUM];

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	int rounding_mode_before_test, rounding_mode_after_test;
#endif

	e90x_irq_fpu_ctx_test_t *irq_fpu_test = arg;
	e90x_irq_fpu_ctx_test_t *next_irq_fpu_test = irq_fpu_test->next;

	const char *thread_name = irq_fpu_test->name;
#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("enter interrupt handler, irq_id: %u, priority: %u",
		irq_fpu_test->irq_id, irq_fpu_test->irq_priority);
	fpu_test_info("interger_part=%u", irq_fpu_test->integer_part);
#endif

	if (irq_fpu_test->prev)
	{
		is_middle_priority = 1;
	}

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("target rounding mode: %d", irq_fpu_test->rounding_mode);
#endif
	ret = fesetround(irq_fpu_test->rounding_mode);
	if (ret)
	{
		fpu_test_err("set rounding mode failed, ret: %d", ret);
		irq_fpu_test->result = FPU_TEST_RET_UNSUPPORTED_ROUNDING_MODE;
		goto exit;
	}

	rounding_mode_before_test = fegetround();
#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("current rounding mode: %d", rounding_mode_before_test);
#endif
#endif

	integer_part = irq_fpu_test->integer_part;;
	fpu_reg_cnt = FPU_GENERAL_REG_CNT;

	fill_fpu_reg_data(fpu_reg_cnt, reg_buf_before_test, integer_part);
	fpu_ctx_test_write_reg(reg_buf_before_test);

#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("try to trigger higher priority interrupt, irq_id: %u, priority: %u",
		next_irq_fpu_test->irq_id, next_irq_fpu_test->irq_priority);
#endif

	next_irq_fpu_test->is_executed = 0;
	e90x_trigger_irq(next_irq_fpu_test->irq_id);

#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("Wait higher priority interrupt handler executed");
#endif
	while (!next_irq_fpu_test->is_executed);

	fpu_ctx_test_read_reg(reg_buf_after_test);

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	rounding_mode_after_test = fegetround();

	if (rounding_mode_after_test != rounding_mode_before_test)
	{
		fpu_test_err("FPU rounding mode error! current integer part: %u", integer_part);
		fpu_test_err("rounding_mode_before_test: %d, rounding_mode_after_test: %d",
					 rounding_mode_before_test, rounding_mode_after_test);
		irq_fpu_test->result = -FPU_TEST_RET_ROUNDING_MODE_ERROR;
		goto exit;
	}
#endif

	ret = check_fpu_reg_data(irq_fpu_test->name, fpu_reg_cnt, reg_buf_before_test, reg_buf_after_test, integer_part);
	if (ret)
	{
		irq_fpu_test->result = FPU_TEST_RET_FPU_REG_ERROR_IN_IRQ_ENV;
		goto exit;
	}

	irq_fpu_test->integer_part++;

exit:
	if (is_middle_priority)
	{
		irq_fpu_test->is_executed = 1;
	}

#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("exit interrupt handler, irq_id: %u, priority: %u",
		irq_fpu_test->irq_id, irq_fpu_test->irq_priority);
#endif

	return 0;
}

hal_irqreturn_t highest_priority_irq_handler(void *arg)
{
	uint32_t integer_part, fpu_reg_cnt;
	fpu_reg_t reg_buf[MAX_FPU_GENERAL_REG_NUM];

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	int ret;
#endif

	e90x_irq_fpu_ctx_test_t *irq_fpu_test = (e90x_irq_fpu_ctx_test_t *)arg;
	const char *thread_name = irq_fpu_test->name;

#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("enter interrupt handler, irq_id: %u, priority: %u",
		irq_fpu_test->irq_id, irq_fpu_test->irq_priority);
	fpu_test_info("interger_part=%u", irq_fpu_test->integer_part);
#endif

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("target rounding mode: %d", irq_fpu_test->rounding_mode);
#endif
	ret = fesetround(irq_fpu_test->rounding_mode);
	if (ret)
	{
		fpu_test_err("set rounding mode failed, ret: %d", ret);
		irq_fpu_test->result = FPU_TEST_RET_UNSUPPORTED_ROUNDING_MODE;
		goto exit;
	}

#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("current rounding mode: %d", fegetround());
#endif
#endif
	integer_part = irq_fpu_test->integer_part;
	fpu_reg_cnt = FPU_GENERAL_REG_CNT;
	fill_fpu_reg_data(fpu_reg_cnt, reg_buf, integer_part);
	fpu_ctx_test_write_reg(reg_buf);

	irq_fpu_test->integer_part++;

exit:
	irq_fpu_test->is_executed = 1;

#ifdef DEBUG_E90X_IRQ_FPU_CTX_TEST
	fpu_test_info("exit interrupt handler, irq_id: %u, priority: %u",
		irq_fpu_test->irq_id, irq_fpu_test->irq_priority);
#endif
	return 0;
}

static int e90x_irq_init(e90x_irq_fpu_ctx_test_t *info)
{
	int ret, irq_num;
	irq_num = MAKE_IRQn(info->irq_id, 0);

	ret = hal_request_irq(irq_num, info->irq_handler, NULL, info);
	if (ret)
	{
		os_test_err("request irq %d failed, ret: %d", irq_num, ret);
		return FPU_TEST_RET_REQUEST_IRQ_FAILED;
	}

	ret = hal_irq_set_priority(irq_num, info->irq_priority);
	if (ret)
	{
		os_test_err("set irq %d priority failed, ret: %d", irq_num, ret);
		return FPU_TEST_RET_SET_IRQ_PRIORITY_FAILED;
	}

	ret = irq_core_set_irq_trigger_type(irq_num, IRQ_TRIGGER_TYPE_EDGE_RISING);
	if (ret)
	{
		os_test_err("set irq %d trigger type failed, ret: %d", irq_num, ret);
		return FPU_TEST_RET_SET_IRQ_TRIGGER_TYPE_FAILED;
	}

	ret = hal_enable_irq(irq_num);
	if (ret)
	{
		os_test_err("enable irq %d failed, ret: %d", irq_num, ret);
		return FPU_TEST_RET_ENABLE_IRQ_FAILED;
	}

	return 0;
}

int init_fpu_ctx_test_interrupt(void)
{
	int ret, i;

	e90x_irq_fpu_ctx_test_t *irq_fpu_test, *prev_irq_fpu_test, *next_irq_fpu_test;
	uint32_t irq_id = START_IRQ_ID, integer_part = DEFAULT_INIT_INTEGET_PART;
	hal_irq_prio_t preempt_priority = HAL_IRQ_PRIO_LOW;
	hal_irq_handler_t irq_handler = low_priority_irq_handler;
	int rounding_mode;

	for (i = 0; i < PREEMPT_PRIORITY_CNT; i++)
	{
		irq_fpu_test = &g_irq_fpu_test[i];
		if (i == 0)
		{
			prev_irq_fpu_test = NULL;
			next_irq_fpu_test = &g_irq_fpu_test[i + 1];
#if (PREEMPT_PRIORITY_CNT == 1)
			irq_handler = highest_priority_irq_handler;
#endif
		}
		else if (i == (PREEMPT_PRIORITY_CNT - 1))
		{
			prev_irq_fpu_test = &g_irq_fpu_test[i - 1];
			next_irq_fpu_test = NULL;
			irq_handler = highest_priority_irq_handler;
		}
		else
		{
			prev_irq_fpu_test = &g_irq_fpu_test[i - 1];
			next_irq_fpu_test = &g_irq_fpu_test[i + 1];
		}

		if (i % 2)
		{
			rounding_mode = FE_UPWARD;
		}
		else
		{
			rounding_mode = FE_DOWNWARD;
		}

		snprintf(irq_fpu_test->name, TEST_NAME_ARRAY_LEN, "irq%d", preempt_priority);
		irq_fpu_test->irq_id = irq_id;
		irq_fpu_test->irq_priority = preempt_priority;
		irq_fpu_test->irq_handler = irq_handler;
		irq_fpu_test->is_executed = 0;
		irq_fpu_test->integer_part = integer_part;
		irq_fpu_test->rounding_mode = rounding_mode;
		irq_fpu_test->result = 0;
		irq_fpu_test->prev = prev_irq_fpu_test;
		irq_fpu_test->next = next_irq_fpu_test;
		ret = e90x_irq_init(irq_fpu_test);
		if (ret)
		{
			os_test_err("e90x_irq_init failed, irq_id: %d, ret: %d", irq_id, ret);
			return ret;
		}

		irq_id++;
		preempt_priority++;
		integer_part++;

#ifdef DUMP_E90X_IRQ_FPU_CTX_TEST_INFO
		dump_e90x_irq_fpu_ctx_test(irq_fpu_test);
#endif
	}

	return 0;
}

int fpu_ctx_test_in_interrupt(void)
{
	int i;
	e90x_irq_fpu_ctx_test_t *irq_fpu_test;

	e90x_trigger_irq(START_IRQ_ID);

	for (i = 0; i < PREEMPT_PRIORITY_CNT; i++)
	{
		irq_fpu_test = &g_irq_fpu_test[i];
		if (irq_fpu_test->result)
		{
			return irq_fpu_test->result;
		}
	}
	return 0;
}
