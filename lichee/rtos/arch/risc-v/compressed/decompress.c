/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#include <compiler.h>	/* for inline */
#include <stdint.h>		/* for size_t */
#include <stddef.h>		/* for NULL */
#include <string.h>

extern unsigned long free_mem_ptr;
extern unsigned long free_mem_end_ptr;
extern void error(char *);

#define STATIC static

#ifdef CONFIG_KERNEL_COMPRESS_GZIP
#include "../../../components/common/thirdparty/decompress/decompress_inflate.c"
#endif

#ifdef CONFIG_KERNEL_COMPRESS_LZO
#include "../../../components/common/thirdparty/decompress/decompress_unlzo.c"
#endif

#ifdef CONFIG_KERNEL_COMPRESS_LZMA
#include "../../../components/common/thirdparty/decompress/decompress_unlzma.c"
#endif

#ifdef CONFIG_KERNEL_COMPRESS_XZ
#define memmove memmove
#define memcpy memcpy
#include "../../../components/common/thirdparty/decompress/decompress_unxz.c"
#endif

#ifdef CONFIG_KERNEL_COMPRESS_LZ4
#include "../../../components/common/thirdparty/decompress/decompress_unlz4.c"
#endif

int do_decompress(uint8_t *input, int len, uint8_t *output, void (*error)(char *x))
{
	return __decompress(input, len, NULL, NULL, output, 0, NULL, error);
}
