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
#include <time.h>
#include <inttypes.h>

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
#include <fenv.h>
#endif

#include <hal_cmd.h>
#include <hal_time.h>
#include <hal_thread.h>

#include <os_test_log.h>
#include "fpu_test.h"

#include "fpu_ctx_test.h"

#define FPU_CTX_TEST_THREAD_STACK_DEPTH 4096
#define FPU_CTX_TEST_THREAD_PRIORITY 31

#define DEFAULT_FPU_CTX_TEST_CNT 1000

typedef struct
{
	int result;
	int is_need_exit;
	int is_complete;

	uint32_t max_test_cnt;
	uint32_t busy_waiting_cnt;
	uint32_t integer_part;
#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	int rounding_mode;
#endif

	const char *thread_name;
	void *thread_handle;
} fpu_ctx_test_t;

extern void fpu_ctx_test_read_reg(fpu_reg_t *reg_buf);
extern void fpu_ctx_test_write_reg(const fpu_reg_t *reg_buf);

static void fpu_ctx_test_thread_busy_waiting(uint32_t busy_waiting_cnt)
{
	uint32_t i = 0;
	while (1)
	{
		i++;
		if (i > busy_waiting_cnt)
			break;
	}
}

static void fpu_ctx_test_thread_sleep(void)
{
	__attribute__((unused)) const char *thread_name = hal_thread_get_name(NULL);

	int time = rand()%100 + 1;
	fpu_test_dbg("sleep %dms", time);
	hal_msleep(time);
}

void fill_fpu_reg_data(uint32_t fpu_reg_cnt, fpu_reg_t *reg_buf, uint32_t integer_part)
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

int dump_fpu_reg_data(const char *thread_name, uint32_t fpu_reg_cnt, const fpu_reg_t *reg_buf)
{
	int i;

	for (i = 0; i < fpu_reg_cnt; i++)
	{
		fpu_test_info("reg%02d: 0x%" FPU_REG_TYPE_FMT_STR "(%.6f)",
					  i, reg_buf[i], *((arch_float_t *)&reg_buf[i]));
	}

	return 0;
}

static inline int dump_fpu_reg_data_in_thread(uint32_t fpu_reg_cnt, const fpu_reg_t *reg_buf)
{
	dump_fpu_reg_data(hal_thread_get_name(NULL), fpu_reg_cnt, reg_buf);

	return 0;
}

int check_fpu_reg_data(const char *thread_name, uint32_t fpu_reg_cnt, const fpu_reg_t *before, const fpu_reg_t *after, uint32_t integer_part)
{
	int i;

	for (i = 0; i < fpu_reg_cnt; i++)
	{
		if (!memcmp(&before[i], &after[i], sizeof(fpu_reg_t)))
		{
			continue;
		}

		fpu_test_err("FPU register %d data error! current integer part: %u", i, integer_part);

		fpu_test_info("FPU register data before test:");
		dump_fpu_reg_data(thread_name, fpu_reg_cnt, before);

		fpu_test_info("FPU register data after test:");
		dump_fpu_reg_data(thread_name, fpu_reg_cnt, after);

		return -1;
	}

	return 0;
}

static int check_fpu_reg_data_in_thread(uint32_t fpu_reg_cnt, const fpu_reg_t *before, const fpu_reg_t *after, uint32_t integer_part)
{
	return check_fpu_reg_data(hal_thread_get_name(NULL), fpu_reg_cnt, before, after, integer_part);
}

static void fpu_ctx_test_thread_func(void *param)
{
	int ret = 0;
	fpu_ctx_test_t *fpu_test = (fpu_ctx_test_t *)param;
	const char *thread_name = hal_thread_get_name(NULL);

	fpu_reg_t reg_buf_before_test[MAX_FPU_GENERAL_REG_NUM];
	fpu_reg_t reg_buf_after_test[MAX_FPU_GENERAL_REG_NUM];

	uint32_t busy_waiting_cnt, integer_part, fpu_reg_cnt;
	uint32_t test_cnt = 0, success_cnt = 0;

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	int rounding_mode_before_test, rounding_mode_after_test;
#endif

	fpu_test_info("-------now begin test---------");

	busy_waiting_cnt = fpu_test->busy_waiting_cnt;
	integer_part = fpu_test->integer_part;
	fpu_reg_cnt = FPU_GENERAL_REG_CNT;

	fpu_test_info("busy_waiting_cnt=%u, init_interger_part=%u",
				  busy_waiting_cnt, integer_part);

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	fpu_test_info("target rounding mode: %d", fpu_test->rounding_mode);

	ret = fesetround(fpu_test->rounding_mode);
	if (ret)
	{
		fpu_test_err("set rounding mode failed, ret: %d", ret);
		fpu_test->result = FPU_TEST_RET_UNSUPPORTED_ROUNDING_MODE;
		goto exit;
	}

	rounding_mode_before_test = fegetround();
	rounding_mode_after_test = 0;

	fpu_test_info("current rounding mode: %d", rounding_mode_before_test);
#endif

	while (1)
	{
		if (fpu_test->is_need_exit)
			break;

		if (test_cnt >= fpu_test->max_test_cnt)
			break;

		fill_fpu_reg_data(fpu_reg_cnt, reg_buf_before_test, integer_part);

		fpu_ctx_test_write_reg(reg_buf_before_test);
		ret = fpu_ctx_test_in_interrupt();
		if (ret)
		{
			fpu_test->result = ret;
			goto exit;
		}
		fpu_ctx_test_thread_busy_waiting(busy_waiting_cnt);
		fpu_ctx_test_thread_sleep();
		fpu_ctx_test_read_reg(reg_buf_after_test);

		test_cnt++;

#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
		rounding_mode_after_test = fegetround();

		if (rounding_mode_after_test != rounding_mode_before_test)
		{
			fpu_test_err("FPU rounding mode error! current integer part: %u", integer_part);
			fpu_test_err("rounding_mode_before_test: %d, rounding_mode_after_test: %d",
						 rounding_mode_before_test, rounding_mode_after_test);
			fpu_test->result = FPU_TEST_RET_ROUNDING_MODE_ERROR;
			goto exit;
		}
#endif

		ret = check_fpu_reg_data_in_thread(fpu_reg_cnt, reg_buf_before_test, reg_buf_after_test, integer_part);
		if (ret)
		{
			fpu_test->result = FPU_TEST_RET_FPU_REG_ERROR;
			goto exit;
		}

		integer_part++;
		success_cnt++;
	}
exit:
	fpu_test_info("test_cnt: %u, success_cnt: %u", test_cnt, success_cnt);
	fpu_test->is_complete = 1;
	hal_thread_stop(NULL);
}

static int init_fpu_ctx_test_thread(fpu_ctx_test_t *fpu_test)
{
	int ret;
	const char *thread_name = NULL;
	void *thread_handle = NULL;

	thread_name = fpu_test->thread_name;
	thread_handle = hal_thread_create(fpu_ctx_test_thread_func, (void *)fpu_test,
									  thread_name, FPU_CTX_TEST_THREAD_STACK_DEPTH, FPU_CTX_TEST_THREAD_PRIORITY);

	if (!thread_handle)
	{
		os_test_err("create thread('%s') failed", thread_name);
		return FPU_TEST_RET_CREATE_THREAD_FAILED;
	}

	ret = hal_thread_start(thread_handle);
	if (ret)
	{
		os_test_err("start thread('%s') failed, ret: %d", thread_name, ret);
		return FPU_TEST_RET_START_THREAD_FAILED;
	}

	fpu_test->thread_handle = thread_handle;
	return 0;
}

/* TODO: General FPU context test in interrupt environment, need to use one peripheral interrupt */
__attribute__((weak)) int init_fpu_ctx_test_interrupt(void)
{
	return 0;
}
__attribute__((weak)) int fpu_ctx_test_in_interrupt(void)
{
	return 0;
}

int fpu_ctx_test(uint32_t test_cnt)
{
	int ret;
	srand((unsigned int)(time(NULL)));

	os_test_info("type size info, float: %d, double: %d, arch_float_t: %d",
				 sizeof(float), sizeof(double), sizeof(arch_float_t));

	os_test_info("FPU general register info, size: %d, count: %d", sizeof(fpu_reg_t), FPU_GENERAL_REG_CNT);

#if defined(CONFIG_ARCH_RISCV) && defined(__riscv_flen)
	if (FPU_GENERAL_REG_SIZE != (__riscv_flen / 8))
	{
		os_test_warn("Warning: FPU register size(%d) is not same with the size(%d) toolchain provided",
					 FPU_GENERAL_REG_SIZE, (__riscv_flen / 8));
	}
#endif

	ret = init_fpu_ctx_test_interrupt();
	if (ret)
	{
		return ret;
	}

	fpu_ctx_test_t s_fpu_test1, s_fpu_test2;
	memset(&s_fpu_test1, 0, sizeof(s_fpu_test1));
	memset(&s_fpu_test2, 0, sizeof(s_fpu_test2));

	s_fpu_test1.max_test_cnt = test_cnt;
	s_fpu_test1.busy_waiting_cnt = 1;
#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	s_fpu_test1.rounding_mode = FE_TONEAREST;
#endif
	s_fpu_test1.integer_part = 0;
	s_fpu_test1.thread_name = "fpu-test-1";
	s_fpu_test1.result = 0;
	ret = init_fpu_ctx_test_thread(&s_fpu_test1);
	if (ret)
		return ret;

	s_fpu_test2.max_test_cnt = test_cnt;
	s_fpu_test2.busy_waiting_cnt = 240000;
#ifdef CONFIG_FPU_CTX_TEST_WITH_ROUNDING_MODE
	s_fpu_test2.rounding_mode = FE_TOWARDZERO;
#endif
	s_fpu_test2.integer_part = 1;
	s_fpu_test2.thread_name = "fpu-test-2";
	s_fpu_test2.result = 0;
	ret = init_fpu_ctx_test_thread(&s_fpu_test2);
	if (ret)
	{
		s_fpu_test1.is_need_exit = 1;
		return ret;
	}

	while (1)
	{
		hal_msleep(10);
		if (!s_fpu_test1.is_complete)
			continue;

		if (!s_fpu_test2.is_complete)
			continue;

		break;
	}

	if (s_fpu_test1.result)
		return s_fpu_test1.result;

	if (s_fpu_test2.result)
		return s_fpu_test2.result;

	return FPU_TEST_RET_OK;
}

int cmd_fpu_ctx_test(int argc, char **argv)
{
	int ret = 0;
	unsigned long test_cnt = DEFAULT_FPU_CTX_TEST_CNT;
	char *ptr = NULL;
	errno = 0;

	if (argc >= 2)
	{
		test_cnt = strtoul(argv[1], &ptr, 10);
		if (errno || (ptr && *ptr != '\0'))
		{
			os_test_err("invalid input parameter('%s')!", argv[1]);
			return 0;
		}
	}

	ret = fpu_ctx_test(test_cnt);
	if (!ret)
		os_test_info("FPU context save/restore test success!");
	else
		os_test_err("FPU context save/restore test failed, ret: %d", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_fpu_ctx_test, fpu_ctx_test, FPU context save/restore test);
