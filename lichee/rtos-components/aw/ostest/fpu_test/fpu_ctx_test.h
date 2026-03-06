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

#ifndef __FPU_CTX_TEST_H__
#define __FPU_CTX_TEST_H__

#include <inttypes.h>

#define MAX_FPU_GENERAL_REG_NUM 32

#define FPU_GENERAL_REG_CNT (CONFIG_FPU_GENERAL_REG_CNT)
#define FPU_GENERAL_REG_SIZE (CONFIG_FPU_GENERAL_REG_SIZE)

#if (FPU_GENERAL_REG_CNT > MAX_FPU_GENERAL_REG_NUM) || (FPU_GENERAL_REG_CNT == 0)
#error Unsupported FPU general register count!
#endif

#if (FPU_GENERAL_REG_SIZE != 4) && (FPU_GENERAL_REG_SIZE != 8)
#error Unsupported FPU general register size!
#endif

#if (FPU_GENERAL_REG_SIZE == 4)
typedef uint32_t fpu_reg_t;
typedef float arch_float_t;
#define FPU_REG_TYPE_FMT_STR "08x"
#endif

#if (FPU_GENERAL_REG_SIZE == 8)
typedef uint64_t fpu_reg_t;
typedef double arch_float_t;
#define FPU_REG_TYPE_FMT_STR PRIu64
#endif

extern void fpu_ctx_test_read_reg(fpu_reg_t *reg_buf);
extern void fpu_ctx_test_write_reg(const fpu_reg_t *reg_buf);

void fill_fpu_reg_data(uint32_t fpu_reg_cnt, fpu_reg_t *reg_buf, uint32_t integer_part);
int check_fpu_reg_data(const char *thread_name, uint32_t fpu_reg_cnt, const fpu_reg_t *before, const fpu_reg_t *after, uint32_t integer_part);
int dump_fpu_reg_data(const char *name, uint32_t fpu_reg_cnt, const fpu_reg_t *reg_buf);


int init_fpu_ctx_test_interrupt(void);
int fpu_ctx_test_in_interrupt(void);

#endif /* __FPU_CTX_TEST_H__ */