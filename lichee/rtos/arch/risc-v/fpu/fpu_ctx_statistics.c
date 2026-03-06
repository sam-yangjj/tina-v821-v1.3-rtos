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
#include <hal_cmd.h>
#include <fpu.h>

#if CPU_WORD_LEN == 8
#define FPU_CNT_FMT "%llu"
#elif CPU_WORD_LEN == 4
#define FPU_CNT_FMT "%u"
#else
#error unsupported CPU word length!
#endif

__attribute__((aligned (CPU_WORD_LEN))) fpu_cnt_t g_skip_fpu_ctx_save_cnt;
__attribute__((aligned (CPU_WORD_LEN))) fpu_cnt_t g_skip_fpu_off_ctx_restore_cnt;
__attribute__((aligned (CPU_WORD_LEN))) fpu_cnt_t g_skip_fpu_init_ctx_restore_cnt;
__attribute__((aligned (CPU_WORD_LEN))) fpu_cnt_t g_skip_fpu_clean_ctx_restore_cnt;

int cmd_show_fpu_statistics(int argc, char **argv)
{
	printf("RISC-V FPU context save/restore operation statistics info:\n");
	printf("skip save operation count(FPU is not dirty): "FPU_CNT_FMT"\n", g_skip_fpu_ctx_save_cnt);
	printf("skip restore operation count(FS field in thread stack is off): "FPU_CNT_FMT"\n", g_skip_fpu_off_ctx_restore_cnt);
	printf("skip restore operation count(FS field in thread stack is init): "FPU_CNT_FMT"\n", g_skip_fpu_init_ctx_restore_cnt);
	printf("skip restore operation count(FPU is clean and no thread switch): "FPU_CNT_FMT"\n", g_skip_fpu_clean_ctx_restore_cnt);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_show_fpu_statistics, show_fpu_stat, Show RISC-V FPU context save/restore operation statistics info);
