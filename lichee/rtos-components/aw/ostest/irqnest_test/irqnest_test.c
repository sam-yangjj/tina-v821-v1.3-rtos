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
/*
* Copyright (c) 2024-2030 Allwinner Technology Co., Ltd. ALL rights reserved.
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
#include <hal_cmd.h>
#include <hal_time.h>
#include <hal_interrupt.h>
#include <os_test_log.h>

#include "irqnest_test.h"

#define ORDER_SIZE	(2 * 2)

__attribute__((weak)) int irqnest_platform_get_irq_num(int hw_irqid)
{
	return INVALID_IRQNUM;
}

__attribute__((weak)) int irqnest_platform_irq_request_config(int hw_irqid)
{
	return 0;
}

__attribute__((weak)) int irqnest_platform_trigger_irq(int hw_irqid)
{
	return -1;
}

__attribute__((weak)) void irqnest_platform_clean_pending(int hw_irqid)
{

}

struct irqnest_test_irq {
	int hw_irqid;
	int irq_num;
	uint32_t integer_part;
	uint32_t preemptprio;
	uint32_t subprio;
	hal_irq_prio_t hal_prio;
	hal_irq_handler_t irq_handler;
	uint32_t id;
	/* need to modify orderly */
	unsigned char waiting;
	unsigned char testing;
	void *prev;
	void *next;
};

struct irqnest_test_irq irq_info[TEST_NUM_MAX];
int hw_irqid_order[ORDER_SIZE] = {0};
int hw_irqid_order_cmp[ORDER_SIZE] = {0};

/* Bit N is set to 1 means the id N test irq handler use FPU */
#ifdef CONFIG_IRQNEST_FPU_TEST
volatile uint64_t irqnest_fpu_run_flag = 0;
extern void irqnest_fpu_ctx_test_read_reg(fpu_reg_t *reg_buf);
extern void irqnest_fpu_ctx_test_write_reg(const fpu_reg_t *reg_buf);
#endif

/* context save/restore test */
static hal_irqreturn_t irqnest_ctx_test_handler(void *arg)
{
	int i;
	struct irqnest_test_irq *info = arg;
	struct irqnest_test_irq *next = info->next;
	struct irqnest_test_irq *prev = info->prev;

	os_test_info("== irqid( %d ) ENTER handler, hal_prio: %d ==", info->hw_irqid, info->hal_prio);

	irqnest_platform_clean_pending(info->hw_irqid);

#ifdef CONFIG_IRQNEST_FPU_TEST
	uint32_t integer_part, fpu_reg_cnt;
	fpu_reg_t reg_buf_before_test[MAX_FPU_GENERAL_REG_NUM];
	fpu_reg_t reg_buf_after_test[MAX_FPU_GENERAL_REG_NUM];

	integer_part = info->integer_part;
	fpu_reg_cnt = FPU_GENERAL_REG_CNT;
	if (irqnest_fpu_run_flag & (0x1 << info->id)) {
		irqnest_fill_fpu_reg_data(fpu_reg_cnt, reg_buf_before_test, integer_part);
		irqnest_fpu_ctx_test_write_reg(reg_buf_before_test);
		os_test_info("changed fpu");
	}
#endif

	if (next) {
		info->waiting = 1;
		irqnest_platform_trigger_irq(next->hw_irqid);
		if (info->hal_prio >= next->hal_prio) {
			i = 0xffffffff;
			os_test_info("enter while 0x%x --...", i);
			while(1) {
				i -= 1;
				if ((i <= 0) || (info->waiting == 0))
					break;
				dsb();
			}
		} else {
			while (info->waiting)
				dsb();
		}
	}

#ifdef CONFIG_IRQNEST_FPU_TEST
	if (irqnest_fpu_run_flag & (0x1 << info->id)) {
		irqnest_fpu_ctx_test_read_reg(reg_buf_after_test);
		if (irqnest_check_fpu_reg_data(info->hw_irqid, fpu_reg_cnt, reg_buf_before_test, reg_buf_after_test, integer_part)) {
			os_test_err("irqid( %d ) FPU context Save/Restore test Fail, irqnest_fpu_run_flag: %llx, enter while...",
					info->hw_irqid, irqnest_fpu_run_flag);
			/* irq restore may fail */
			while(1);
		}
	}
#endif

	if (prev) {
		prev->waiting = 0;
	}

	os_test_info("== irqid( %d ) EXIT handler, hal_prio: %d ==", info->hw_irqid, info->hal_prio);
	info->testing = 0;
	return 0;
}

static int irqnest_ctx_test(int test_cnt)
{
	int i;
	int ret;
	uint32_t cnt = 0;
	uint64_t init_rec = 0;
#ifdef CONFIG_IRQNEST_FPU_TEST
	int thread_fpu_run_flag = 0;
	uint32_t integer_part, fpu_reg_cnt;
	fpu_reg_t reg_buf_before_test[MAX_FPU_GENERAL_REG_NUM];
	fpu_reg_t reg_buf_after_test[MAX_FPU_GENERAL_REG_NUM];

	integer_part = TEST_NUM_MAX;
	fpu_reg_cnt = FPU_GENERAL_REG_CNT;
#endif

	os_test_info("Test IRQ List:");
	for (i = 0; i < TEST_NUM_MAX; i++) {
		/* init priority, irq handler, integer_part*/
		irq_info[i].integer_part = i;
		irq_info[i].hal_prio = HAL_IRQ_PRIO_BASE + i;
		if (irq_info[i].hal_prio >= HAL_IRQ_PRIO_MAX)
			irq_info[i].hal_prio = HAL_IRQ_PRIO_MAX - 1;
		irq_info[i].irq_handler = irqnest_ctx_test_handler;
		if (i > 0)
			irq_info[i].prev = &irq_info[i - 1];
		if (i < (TEST_NUM_MAX - 1))
			irq_info[i].next = &irq_info[i + 1];

		os_test_info("irq_info[%d]:  hw_irqid(%d),  irq_num(%d),  hal_prio(%d) ",
			i, irq_info[i].hw_irqid, irq_info[i].irq_num, irq_info[i].hal_prio);
	}

	for (i = 0; i < TEST_NUM_MAX; i++) {
		ret = hal_request_irq(irq_info[i].irq_num, irqnest_ctx_test_handler, NULL, &irq_info[i]);
		if (ret) {
			os_test_err("request irq %d failed, ret: %d", irq_info[i].irq_num, ret);
			goto ctx_test_fail;
		}
		ret = irqnest_platform_irq_request_config(irq_info[i].hw_irqid);
		if (ret) {
			os_test_err("platform request hw_irqid %d config failed, ret: %d", irq_info[i].hw_irqid, ret);
			goto ctx_test_fail2;
		}
		ret = hal_irq_set_priority(irq_info[i].irq_num, irq_info[i].hal_prio);
		if (ret) {
			os_test_err("set irq %d priority failed, ret: %d", irq_info[i].irq_num, ret);
			goto ctx_test_fail2;
		}
		ret = hal_enable_irq(irq_info[i].irq_num);
		if (ret) {
			os_test_err("enable irq %d failed, ret: %d", irq_info[i].irq_num, ret);
			goto ctx_test_fail3;
		}

		init_rec |= (0x1 << i);
	}


	while (1) {
		os_test_info("------ irqnest ctx test cnt: %d ------", cnt + 1);

#ifdef CONFIG_IRQNEST_FPU_TEST
		/* thread change fpu */
		if (thread_fpu_run_flag) {
			irqnest_fill_fpu_reg_data(fpu_reg_cnt, reg_buf_before_test, integer_part);
			irqnest_fpu_ctx_test_write_reg(reg_buf_before_test);
			os_test_info("thread changed fpu");
		}
#endif

		irq_info[0].testing = 1;
		irqnest_platform_trigger_irq(irq_info[0].hw_irqid);
		while(1) {
			hal_msleep(10);
			if (!irq_info[0].testing)
				break;
		}

#ifdef CONFIG_IRQNEST_FPU_TEST
		if (thread_fpu_run_flag) {
			irqnest_fpu_ctx_test_read_reg(reg_buf_after_test);
			if (irqnest_check_fpu_reg_data(INVALID_IRQNUM, fpu_reg_cnt, reg_buf_before_test, reg_buf_after_test, integer_part)) {
				os_test_err("THREAD FPU context Save/Restore test Fail, thread_fpu_run_flag: %d, enter while...", thread_fpu_run_flag);
				break;
			}
		}

		irqnest_fpu_run_flag += 1;
		if (irqnest_fpu_run_flag >= (0x1 << TEST_NUM_MAX)) {
			irqnest_fpu_run_flag = 0;
			thread_fpu_run_flag = !(!!thread_fpu_run_flag);
		}
#endif

		cnt += 1;
		if (cnt >= test_cnt)
			break;
	}

	for (i = 0; i < TEST_NUM_MAX; i++) {
		if (init_rec & (0x1 << i)) {
			hal_disable_irq(irq_info[i].irq_num);
			ret = hal_irq_set_priority(irq_info[i].irq_num, HAL_IRQ_PRIO_BASE);
			if (ret)
				os_test_err("fail handler set irq %d hal_prio(%d) failed, ret: %d",
					irq_info[i].irq_num, HAL_IRQ_PRIO_BASE, ret);
			hal_free_irq(irq_info[i].irq_num);
		}
	}

	return 0;

ctx_test_fail3:
	ret = hal_irq_set_priority(irq_info[i].irq_num, HAL_IRQ_PRIO_BASE);
	if (ret)
		os_test_err("fail handler set irq %d hal_prio(%d) failed, ret: %d",
				irq_info[i].irq_num, HAL_IRQ_PRIO_BASE, ret);
ctx_test_fail2:
	hal_free_irq(irq_info[i].irq_num);

ctx_test_fail:
	for (i = 0; i < TEST_NUM_MAX; i++) {
		if (init_rec & (0x1 << i)) {
			hal_disable_irq(irq_info[i].irq_num);
			ret = hal_irq_set_priority(irq_info[i].irq_num, HAL_IRQ_PRIO_BASE);
			if (ret)
				os_test_err("fail handler set irq %d hal_prio(%d) failed, ret: %d",
					irq_info[i].irq_num, HAL_IRQ_PRIO_BASE, ret);
			hal_free_irq(irq_info[i].irq_num);
		}
	}

	return -1;
}

/* preempt test */
static hal_irqreturn_t irqnest_preempt_test_handler(void *arg)
{
	int i;
	struct irqnest_test_irq *info = arg;
	struct irqnest_test_irq *next = info->next;
	struct irqnest_test_irq *prev = info->prev;

	os_test_info("== irqid( %d ) ENTER handler, hal_prio: %d ==", info->hw_irqid, info->hal_prio);

	for (i = 0; i < ORDER_SIZE; i++) {
		if(hw_irqid_order[i] == 0) {
			hw_irqid_order[i] = info->hw_irqid;
			break;
		}
	}

	if (next) {
		info->waiting = 1;
		irqnest_platform_trigger_irq(next->hw_irqid);
		if (info->hal_prio >= next->hal_prio) {
			i = 0xffffffff;
			os_test_info("enter while 0x%x --...", i);
			while(1) {
				i -= 1;
				if ((i <= 0) || (info->waiting == 0))
					break;
				dsb();
			}
		} else {
			while (info->waiting)
				dsb();
		}
	}

	if (prev) {
		prev->waiting = 0;
	}

	for (i = 0; i < ORDER_SIZE; i++) {
		if (hw_irqid_order[i] == 0) {
			hw_irqid_order[i] = (-1 * info->hw_irqid);
			break;
		}
	}

	os_test_info("== irqid( %d ) EXIT handler, hal_prio: %d ==", info->hw_irqid, info->hal_prio);
	info->testing = 0;
	return 0;
}

static int irqnest_preempt_test(void)
{
	int i;
	int ret;
	uint64_t init_rec = 0;

	os_test_info("--- same priority preempt test... ---");
	memset(hw_irqid_order, 0, ORDER_SIZE * sizeof(int));
	memset(hw_irqid_order_cmp, 0, ORDER_SIZE * sizeof(int));
	for (i = 0; i < 2; i++) {
		ret = hal_request_irq(irq_info[i].irq_num, irqnest_preempt_test_handler, NULL, &irq_info[i]);
		if (ret) {
			os_test_err("request irq %d failed, ret: %d", irq_info[i].irq_num, ret);
			goto preempt_test_fail;
		}

		ret = irqnest_platform_irq_request_config(irq_info[i].hw_irqid);
		if (ret) {
			os_test_err("platform request hw_irqid %d config failed, ret: %d", irq_info[i].hw_irqid, ret);
			goto preempt_test_fail2;
		}

		/* Top level */
		irq_info[i].hal_prio = HAL_IRQ_PRIO_MAX - 1;
		ret = hal_irq_set_priority(irq_info[i].irq_num, irq_info[i].hal_prio);
		if (ret) {
			os_test_err("set irq %d priority failed, ret: %d", irq_info[i].irq_num, ret);
			goto preempt_test_fail2;
		}

		ret = hal_enable_irq(irq_info[i].irq_num);
		if (ret) {
			os_test_err("enable irq %d failed, ret: %d", irq_info[i].irq_num, ret);
			goto preempt_test_fail3;
		}

		init_rec |= (0x1 << i);
		irq_info[i].testing = 1;
	}

	irq_info[0].prev = NULL;
	irq_info[0].next = &irq_info[1];
	irq_info[1].prev = &irq_info[0];
	irq_info[1].next = NULL;
	hw_irqid_order_cmp[0] = irq_info[0].hw_irqid;
	hw_irqid_order_cmp[1] = (-1 * irq_info[0].hw_irqid);
	hw_irqid_order_cmp[2] = irq_info[1].hw_irqid;
	hw_irqid_order_cmp[3] = (-1 * irq_info[1].hw_irqid);
	os_test_info("same priority test trigger irq...");
	irqnest_platform_trigger_irq(irq_info[0].hw_irqid);
	while(1) {
		hal_msleep(10);

		if (!irq_info[0].testing && !irq_info[1].testing)
			break;
	}
	if (memcmp(hw_irqid_order_cmp, hw_irqid_order, ORDER_SIZE * sizeof(int))) {
		os_test_err("--- same priority preempt test fail ---");
		os_test_err("hw_irqid_order_cmp:");
		for (i = 0; i < ORDER_SIZE; i++) {
			os_test_err("%d", hw_irqid_order_cmp[i]);
		}
		os_test_err("hw_irqid_order:");
		for (i = 0; i < ORDER_SIZE; i++) {
			os_test_err("%d", hw_irqid_order[i]);
		}
		goto preempt_test_fail;
	}
	os_test_info("--- same priority preempt test Success ---");

	os_test_info("--- low preempt high priority test... ---");
	memset(hw_irqid_order, 0, ORDER_SIZE * sizeof(int));
	memset(hw_irqid_order_cmp, 0, ORDER_SIZE * sizeof(int));
	for (i = 0; i < 2; i++) {
		hal_disable_irq(irq_info[i].irq_num);

		irq_info[i].hal_prio = HAL_IRQ_PRIO_MAX - 1 - i;
		ret = hal_irq_set_priority(irq_info[i].irq_num, irq_info[i].hal_prio);
		if (ret) {
			os_test_err("set irq %d priority failed, ret: %d", irq_info[i].irq_num, ret);
			goto preempt_test_fail;
		}

		ret = hal_enable_irq(irq_info[i].irq_num);
		if (ret) {
			os_test_err("enable irq %d failed, ret: %d", irq_info[i].irq_num, ret);
			goto preempt_test_fail;
		}

		irq_info[i].testing = 1;
	}

	irq_info[0].prev = NULL;
	irq_info[0].next = &irq_info[1];
	irq_info[1].prev = &irq_info[0];
	irq_info[1].next = NULL;
	hw_irqid_order_cmp[0] = irq_info[0].hw_irqid;
	hw_irqid_order_cmp[1] = (-1 * irq_info[0].hw_irqid);
	hw_irqid_order_cmp[2] = irq_info[1].hw_irqid;
	hw_irqid_order_cmp[3] = (-1 * irq_info[1].hw_irqid);
	os_test_info("low preempt high priority test trigger irq...");
	irqnest_platform_trigger_irq(irq_info[0].hw_irqid);
	while(1) {
		hal_msleep(10);

		if (!irq_info[0].testing && !irq_info[1].testing)
			break;
	}
	if (memcmp(hw_irqid_order_cmp, hw_irqid_order, ORDER_SIZE * sizeof(int))) {
		os_test_info("--- low preempt high priority test trigger irq fail ---");
		os_test_err("hw_irqid_order_cmp:");
		for (i = 0; i < ORDER_SIZE; i++) {
			os_test_err("%d", hw_irqid_order_cmp[i]);
		}
		os_test_err("hw_irqid_order:");
		for (i = 0; i < ORDER_SIZE; i++) {
			os_test_err("%d", hw_irqid_order[i]);
		}
		goto preempt_test_fail;
	}
	os_test_info("--- low preempt high priority test trigger irq Success---");

	os_test_info("--- high preempt low priority test... ---");
	memset(hw_irqid_order, 0, ORDER_SIZE * sizeof(int));
	memset(hw_irqid_order_cmp, 0, ORDER_SIZE * sizeof(int));
	for (i = 0; i < 2; i++) {
		irq_info[i].testing = 1;
	}

	irq_info[1].prev = NULL;
	irq_info[1].next = &irq_info[0];
	irq_info[0].prev = &irq_info[1];
	irq_info[0].next = NULL;
	hw_irqid_order_cmp[0] = irq_info[1].hw_irqid;
	hw_irqid_order_cmp[1] = irq_info[0].hw_irqid;
	hw_irqid_order_cmp[2] = (-1 * irq_info[0].hw_irqid);
	hw_irqid_order_cmp[3] = (-1 * irq_info[1].hw_irqid);
	os_test_info("high preempt low priority test trigger irq...");
	irqnest_platform_trigger_irq(irq_info[1].hw_irqid);
	while(1) {
		hal_msleep(10);

		if (!irq_info[0].testing && !irq_info[1].testing)
			break;
	}
	if (memcmp(hw_irqid_order_cmp, hw_irqid_order, ORDER_SIZE * sizeof(int))) {
		os_test_info("--- high preempt low priority test trigger irq fail ---");
		os_test_err("hw_irqid_order_cmp:");
		for (i = 0; i < ORDER_SIZE; i++) {
			os_test_err("%d", hw_irqid_order_cmp[i]);
		}
		os_test_err("hw_irqid_order:");
		for (i = 0; i < ORDER_SIZE; i++) {
			os_test_err("%d", hw_irqid_order[i]);
		}
		goto preempt_test_fail;
	}
	os_test_info("--- high preempt low priority test trigger irq Success---");

	for (i = 0; i < 2; i++) {
		hal_disable_irq(irq_info[i].irq_num);
		ret = hal_irq_set_priority(irq_info[i].irq_num, HAL_IRQ_PRIO_BASE);
		if (ret)
			os_test_err("set irq %d hal_prio(%d) failed, ret: %d",
				irq_info[i].irq_num, HAL_IRQ_PRIO_BASE, ret);
		hal_free_irq(irq_info[i].irq_num);
	}

	memset(hw_irqid_order, 0, ORDER_SIZE * sizeof(int));
	memset(hw_irqid_order_cmp, 0, ORDER_SIZE * sizeof(int));
	return 0;

preempt_test_fail3:
	ret = hal_irq_set_priority(irq_info[i].irq_num, HAL_IRQ_PRIO_BASE);
	if (ret)
		os_test_err("fail handler set irq %d hal_prio(%d) failed, ret: %d",
				irq_info[i].irq_num, HAL_IRQ_PRIO_BASE, ret);
preempt_test_fail2:
	hal_free_irq(irq_info[i].irq_num);
	
preempt_test_fail:
	for (i = 0; i < 2; i++) {
		if (init_rec & (0x1 << i)) {
			hal_disable_irq(irq_info[i].irq_num);
			ret = hal_irq_set_priority(irq_info[i].irq_num, HAL_IRQ_PRIO_BASE);
			if (ret)
				os_test_err("fail handler set irq %d hal_prio(%d) failed, ret: %d",
					irq_info[i].irq_num, HAL_IRQ_PRIO_BASE, ret);
			hal_free_irq(irq_info[i].irq_num);
		}
	}
	memset(hw_irqid_order, 0, ORDER_SIZE * sizeof(int));
	memset(hw_irqid_order_cmp, 0, ORDER_SIZE * sizeof(int));

	return -1;
}

static int irqnest_priority_set_and_get_test(void)
{
	int i;
	int ret;
	uint32_t preemptprio;
	uint32_t subprio;
	hal_irq_prio_t hal_prio;
	uint64_t init_rec = 0;

	/* Test action:
	 * irq request -> set hal prio -> get hal prio and compare
	 * -> get specific prio -> set specific prio -> get specific prio and compare
	 * -> get hal prio and compare -> enable irq.
	 */

	for (i = 0; i < TEST_NUM_MAX; i++) {
		ret = hal_irq_set_priority(irq_info[i].irq_num, irq_info[i].hal_prio);
		if (ret) {
			os_test_err("set irq %d priority failed, ret: %d", irq_info[i].irq_num, ret);
			goto prio_set_get_fail;
		}

		ret = hal_irq_get_priority(irq_info[i].irq_num, &hal_prio);
		if (ret) {
			os_test_err("get irq %d hal priority failed, ret: %d", irq_info[i].irq_num, ret);
			goto prio_set_get_fail2;
		} else {
			if (irq_info[i].hal_prio != hal_prio) {
				os_test_err("irq %d hal priority: get (%d) != set (%d)",
						irq_info[i].irq_num, hal_prio, irq_info[i].hal_prio);
				goto prio_set_get_fail2;
			}
		}

		ret = hal_irq_get_specific_priority(irq_info[i].irq_num, &preemptprio, &subprio);
		if (ret) {
			os_test_err("get irq %d specific priority failed, ret: %d", irq_info[i].irq_num, ret);
			goto prio_set_get_fail2;
		}

		ret = hal_irq_set_specific_priority(irq_info[i].irq_num, preemptprio, subprio);
		if (ret) {
			os_test_err("set irq %d specific priority failed, ret: %d", irq_info[i].irq_num, ret);
			goto prio_set_get_fail2;
		}

		ret = hal_irq_get_specific_priority(irq_info[i].irq_num, &irq_info[i].preemptprio, &irq_info[i].subprio);
		if (ret) {
			os_test_err("2nd get irq %d specific priority failed, ret: %d", irq_info[i].irq_num, ret);
			goto prio_set_get_fail2;
		} else {
			if (irq_info[i].preemptprio != preemptprio) {
				os_test_err("irq %d specific priority preemptprio: get (%d) != set (%d)",
					irq_info[i].irq_num, irq_info[i].preemptprio, preemptprio);
				goto prio_set_get_fail2;
			}

			if ((irq_info[i].subprio != subprio)) {
				os_test_err("irq %d specific priority subprio: get (%d) != set (%d)",
					irq_info[i].irq_num, irq_info[i].subprio, subprio);
				goto prio_set_get_fail2;
			}
		}

		ret = hal_irq_get_priority(irq_info[i].irq_num, &hal_prio);
		if (ret) {
			os_test_err("2nd get irq %d hal priority failed, ret: %d", irq_info[i].irq_num, ret);
			goto prio_set_get_fail2;
		} else {
			if (irq_info[i].hal_prio != hal_prio) {
				os_test_err("irq %d hal priority map specific priority error: get (%d) != want to set (%d)",
						irq_info[i].irq_num, hal_prio, irq_info[i].hal_prio);
				goto prio_set_get_fail2;
			}
		}

		init_rec |= (0x1 << i);
		os_test_info("Testirq%d: irqnum(%d), hw_irqid(%d), hal_prio(%d), preemptprio(%d), subprio(%d)\n",
			i, irq_info[i].irq_num, irq_info[i].hw_irqid, hal_prio, preemptprio, subprio);
	}

	for (i = 0; i < TEST_NUM_MAX; i++) {
		if (init_rec & (0x1 << i)) {
			ret = hal_irq_set_priority(irq_info[i].irq_num, HAL_IRQ_PRIO_BASE);
			if (ret)
				os_test_err("fail handler set irq %d hal_prio(%d) failed, ret: %d",
						irq_info[i].irq_num, HAL_IRQ_PRIO_BASE, ret);
		}
	}

	return 0;

prio_set_get_fail2:
	ret = hal_irq_set_priority(irq_info[i].irq_num, HAL_IRQ_PRIO_BASE);
	if (ret)
		os_test_err("fail handler set irq %d hal_prio(%d) failed, ret: %d",
				irq_info[i].irq_num, HAL_IRQ_PRIO_BASE, ret);

prio_set_get_fail:
	for (i = 0; i < TEST_NUM_MAX; i++) {
		if (init_rec & (0x1 << i)) {
			ret = hal_irq_set_priority(irq_info[i].irq_num, HAL_IRQ_PRIO_BASE);
			if (ret)
				os_test_err("fail handler set irq %d hal_prio(%d) failed, ret: %d",
						irq_info[i].irq_num, HAL_IRQ_PRIO_BASE, ret);
		}
	}
	return -1;
}

static int irqnest_test_init(int ctx_test_cnt)
{
	int i;
	int ret;

	if (TEST_NUM_MAX < 2) {
		os_test_err("TEST_NUM_MAX(%d) invalid", TEST_NUM_MAX);
		return -1;
	}

	if (CONFIG_IRQNEST_TEST_HW_IRQID_START == INVALID_IRQNUM) {
		os_test_err("IRQNEST_TEST_HW_IRQID_START(%d) invalid", CONFIG_IRQNEST_TEST_HW_IRQID_START);
		return -1;
		
	}

	if (irqnest_platform_get_irq_num(CONFIG_IRQNEST_TEST_HW_IRQID_START) == INVALID_IRQNUM) {
			os_test_err("no platform irq num define");
			return -1;
	}

	for (i = 0; i < TEST_NUM_MAX; i++) {
		irq_info[i].hw_irqid = CONFIG_IRQNEST_TEST_HW_IRQID_START + i;
		irq_info[i].irq_num = irqnest_platform_get_irq_num(CONFIG_IRQNEST_TEST_HW_IRQID_START + i);

		irq_info[i].integer_part = 0;
		irq_info[i].hal_prio = HAL_IRQ_PRIO_BASE + i;
		if (irq_info[i].hal_prio >= HAL_IRQ_PRIO_MAX)
			irq_info[i].hal_prio = HAL_IRQ_PRIO_MAX - 1;
		irq_info[i].preemptprio = 0;
		irq_info[i].subprio = 0;
		irq_info[i].irq_handler = irqnest_ctx_test_handler;

		irq_info[i].id = i;
		irq_info[i].prev = NULL;
		irq_info[i].next = NULL;
	}

	os_test_info("--- hal priority API set and get test start... ---");
	ret = irqnest_priority_set_and_get_test();
	if (ret) {
		os_test_err("--- hal priority API set and get test End, Fail ---");
		return -1;
	}
	os_test_info("--- hal priority API set and get test End, Success ---");

	os_test_info("--- preempt test start... ---");
	ret = irqnest_preempt_test();
	if (ret) {
		os_test_err("--- preempt test End, Fail ---");
		return -1;
	}
	os_test_info("--- preempt test End, Success ---");

	os_test_info("--- context save/resotre stress test start... ---");
#ifdef CONFIG_IRQNEST_OSTEST_GENERAL_CTX_CMP
	irqnest_ctx_track_irq_init();
#endif
#ifdef CONFIG_IRQNEST_FPU_TEST
	irqnest_fpu_run_flag = 0;
#endif
	ret = irqnest_ctx_test(ctx_test_cnt);
	if (ret) {
		os_test_err("--- context save/restore stress test End, Fail ---");
		return -1;
	}
	os_test_info("--- context save/restore stress test End, Success, cnt: %d ---", ctx_test_cnt);


	return 0;
}


int cmd_irqnest_test(int argc, char **argv)
{
	int ret;
	int ctx_test_cnt = 0;

	if (argc != 2) {
		os_test_err("invalid param, useage: irqnest_test <test_cnt>");
		return -1;
	}

	ctx_test_cnt = atoi(argv[1]);

	os_test_info("irqnest ctx_test_cnt: %d", ctx_test_cnt);
	ret = irqnest_test_init(ctx_test_cnt);

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_irqnest_test, irqnest_test, irq_nest_testcase);
