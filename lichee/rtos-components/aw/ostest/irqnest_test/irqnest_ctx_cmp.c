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
#include <os_test_log.h>

#include "irqnest_test.h"

#ifdef CONFIG_IRQNEST_FPU_TEST
void irqnest_fill_fpu_reg_data(uint32_t fpu_reg_cnt, fpu_reg_t *reg_buf, uint32_t integer_part)
{
	arch_float_t *fpu_reg = (arch_float_t *)reg_buf;

	fpu_reg[0] = integer_part + 0.0f;
	fpu_reg[1] = integer_part + 1.0f;
	fpu_reg[2] = integer_part + 2.0f;
	fpu_reg[3] = integer_part + 3.0f;
	fpu_reg[4] = integer_part + 4.0f;
	fpu_reg[5] = integer_part + 5.0f;
	fpu_reg[6] = integer_part + 6.0f;
	fpu_reg[7] = integer_part + 7.0f;
	fpu_reg[8] = integer_part + 8.0f;
	fpu_reg[9] = integer_part + 9.0f;
	fpu_reg[10] = integer_part + 10.0f;
	fpu_reg[11] = integer_part + 11.0f;
	fpu_reg[12] = integer_part + 12.0f;
	fpu_reg[13] = integer_part + 13.0f;
	fpu_reg[14] = integer_part + 14.0f;
	fpu_reg[15] = integer_part + 15.0f;

	if (fpu_reg_cnt <= 16)
		return;

	fpu_reg[16] = integer_part + 16.0f;
	fpu_reg[17] = integer_part + 17.0f;
	fpu_reg[18] = integer_part + 18.0f;
	fpu_reg[19] = integer_part + 19.0f;
	fpu_reg[20] = integer_part + 20.0f;
	fpu_reg[21] = integer_part + 21.0f;
	fpu_reg[22] = integer_part + 22.0f;
	fpu_reg[23] = integer_part + 23.0f;
	fpu_reg[24] = integer_part + 24.0f;
	fpu_reg[25] = integer_part + 25.0f;
	fpu_reg[26] = integer_part + 26.0f;
	fpu_reg[27] = integer_part + 27.0f;
	fpu_reg[28] = integer_part + 28.0f;
	fpu_reg[29] = integer_part + 29.0f;
	fpu_reg[30] = integer_part + 30.0f;
	fpu_reg[31] = integer_part + 31.0f;
}

void irqnest_dump_fpu_reg_data(int hw_irqid, uint32_t fpu_reg_cnt, const fpu_reg_t *reg_buf)
{
	int i;

	for (i = 0; i < fpu_reg_cnt; i++)
	{
		os_test_err("FPU reg%02d value: 0x%" FPU_REG_TYPE_FMT_STR "(%.6f)",
					  i, reg_buf[i], *((arch_float_t *)&reg_buf[i]));
	}
}

int irqnest_check_fpu_reg_data(int hw_irqid, uint32_t fpu_reg_cnt, const fpu_reg_t *before, const fpu_reg_t *after, uint32_t integer_part)
{
	int i;
	int error = 0;

	for (i = 0; i < fpu_reg_cnt; i++)
	{
		if (!memcmp(&before[i], &after[i], sizeof(fpu_reg_t)))
			continue;

		os_test_err("FPU register %d data error! current integer part: %u", i, integer_part);
		error = 1;
	}

	if (error) {
		os_test_err("--- FPU register data before test:");
		irqnest_dump_fpu_reg_data(hw_irqid, fpu_reg_cnt, before);
		os_test_err("--- FPU register data after test:");
		irqnest_dump_fpu_reg_data(hw_irqid, fpu_reg_cnt, after);
		return -1;
	}

	return 0;
}

#endif /* CONFIG_IRQNEST_FPU_TEST */

#ifdef CONFIG_IRQNEST_OSTEST_GENERAL_CTX_CMP
__attribute__((weak)) void irqnest_platform_ctx_extra_check_entry(int hw_irqid, uint32_t nest, uint32_t seq_num)
{

}

__attribute__((weak)) int irqnest_platform_ctx_extra_check_exit(int hw_irqid, uint32_t nest, uint32_t seq_num)
{
	return 0;
}



static int irqnest_ctx_track_irqid[TEST_NUM_MAX] = {INVALID_IRQNUM};
static general_reg_t irqnest_ctx_entry[GENERAL_REG_CNT * TEST_NUM_MAX] = {0};
static general_reg_t irqnest_ctx_exit[GENERAL_REG_CNT * TEST_NUM_MAX] = {0};

/* The global variable is used by assembly code in interrupt entry for sp recording */
void *g_irqnest_store_addr = NULL;

void irqnest_ctx_track_irq_init(void)
{
	int i;

	os_test_info("ctx compare track irqid:");
	for (i = 0; i < TEST_NUM_MAX; i++) {
		irqnest_ctx_track_irqid[i] = CONFIG_IRQNEST_TEST_HW_IRQID_START + i;
		os_test_info("irqnest_ctx_track_irqid[%d] = %d", i, irqnest_ctx_track_irqid[i]);
	}
}

/* call by assembly code in interrupt entry/exit(global interrupt closed) */
void irqnest_ostest_ctx_cmp_entry(int hw_irqid, uint32_t nest)
{
	int i;

	if ((nest < 1) || !g_irqnest_store_addr)
		/* irq may exit to another thread stack with different context */
		return;

	for (i = 0; i < TEST_NUM_MAX; i++) {
		if (hw_irqid == irqnest_ctx_track_irqid[i]) {
			irqnest_test_dbg("=== irq_id ( %d ) ENTRY! nest ( %d ) ===\n", hw_irqid, nest);
			irqnest_test_dbg("entry sp: 0x%lx, memcpy dest: 0x%lx\n",
				(unsigned long)g_irqnest_store_addr, (unsigned long)&irqnest_ctx_entry[GENERAL_REG_CNT * i]);

			memcpy(&irqnest_ctx_entry[GENERAL_REG_CNT * i], g_irqnest_store_addr, GENERAL_REG_CNT * sizeof(general_reg_t));

#ifdef CONFIG_IRQNEST_TEST_CTX_CONTENT_DEBUG
			int j;
			irqnest_test_dbg("entry general save context: ");
			for(j = 0; j < GENERAL_REG_CNT; j++) {
				irqnest_test_dbg("0x%" GENERAL_REG_TYPE_FMT_STR " ", irqnest_ctx_entry[GENERAL_REG_CNT * i + j]);
			}
#endif

			irqnest_platform_ctx_extra_check_entry(hw_irqid, nest, i);

			return;
		}
	}
}

void irqnest_ostest_ctx_cmp_exit(int hw_irqid, uint32_t nest)
{
	int i, j;
	int error = 0;

	if ((nest < 1) || !g_irqnest_store_addr)
		/* irq may exit to another thread stack with different context */
		return;

	for (i = 0; i < TEST_NUM_MAX; i++) {
		if (hw_irqid == irqnest_ctx_track_irqid[i]) {
			irqnest_test_dbg("=== irq_id ( %d ) EXIT! nest ( %d ) ===\n", hw_irqid, nest);
			irqnest_test_dbg("exit sp: 0x%lx, memcpy dest: 0x%lx\n",
				(unsigned long)g_irqnest_store_addr, (unsigned long)&irqnest_ctx_exit[GENERAL_REG_CNT * i]);

			memcpy(&irqnest_ctx_exit[GENERAL_REG_CNT * i], g_irqnest_store_addr, GENERAL_REG_CNT * sizeof(general_reg_t));

#ifdef CONFIG_IRQNEST_TEST_CTX_CONTENT_DEBUG
			irqnest_test_dbg("exit general save context: ");
			for(j = 0; j < GENERAL_REG_CNT; j++)
				irqnest_test_dbg("0x%" GENERAL_REG_TYPE_FMT_STR " ", irqnest_ctx_exit[GENERAL_REG_CNT * i + j]);
#endif

			/* compare general regs data */
			if (memcmp(&irqnest_ctx_entry[GENERAL_REG_CNT * i], &irqnest_ctx_exit[GENERAL_REG_CNT * i], GENERAL_REG_CNT)) {
				error = 1;
				os_test_err("general regs restore data error, current irqid( %d ), nest( %d )", hw_irqid, nest);
				os_test_err("--- irqid( %d ) ENTRY general data:", hw_irqid);
				for(j = 0; j < GENERAL_REG_CNT; j++)
					os_test_err("0x%" GENERAL_REG_TYPE_FMT_STR " ", irqnest_ctx_entry[GENERAL_REG_CNT * i + j]);
				os_test_err("--- irqid( %d ) EXIT general data:", hw_irqid);
				for(j = 0; j < GENERAL_REG_CNT; j++)
					os_test_err("0x%" GENERAL_REG_TYPE_FMT_STR " ", irqnest_ctx_exit[GENERAL_REG_CNT * i + j]);
			}

			if (irqnest_platform_ctx_extra_check_exit(hw_irqid, nest, i))
				error = 1;

			if (error) {
				os_test_err("irqid( %d ) exit enter stuck", hw_irqid);
				/* irq restore may fail */
				while(1);
			}

			return;
		}
	}
}

#endif /* CONFIG_IRQNEST_OSTEST_GENERAL_CTX_CMP */
