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

#include <stdint.h>

unsigned int __machine_arch_type;
void putstr(const char *ptr);
extern void error(char *x);
extern int backtrace(void);

#define PREBOOT

#include "uncompress.h"
#include "compiler.h" /* for inline */
#include "barrier.h" /* for  isb()*/
#include "../../../components/common/thirdparty/decompress/unaligned.h"

#ifdef CONFIG_KERNEL_COMPRESS_DEBUG
void putstr(const char *ptr)
{
	char c;

	while ((c = *ptr++) != '\0') {
		if (c == '\n')
			putc('\r');
		putc(c);
	}

	flush();
}
#else
void putstr(const char *ptr)
{
	(void)ptr;
}
#endif

#ifdef CONFIG_KERNEL_COMPRESS_DEBUG
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

    str[8] = '\0';

    return str;
}

static char *num2str(long num, char *str)
{
    char index[] = "0123456789";
    unsigned long usnum = (num < 0) ? -num : num;

    int i = 0;
    do {
        str[i++] = index[usnum % 10];
        usnum /= 10;
    } while (usnum > 0);

    if (num < 0)
        str[i++] = '-';

    str[i] = '\0';

    // Reverse the string
    int j = 0;
    if (num < 0)
        j = 1;

    i--;
    while (j < i) {
        char temp = str[j];
        str[j] = str[i];
        str[i] = temp;
        j++;
        i--;
    }

    return str;
}

void printf_val(char *string, long val)
{
	char str[64];
	putstr(string);
	putstr(":");
	putstr(num2str(val, str));
	putstr("\r\n");
}

extern void putstr(const char *ptr);
extern void printf_val(char *string, long val);
#endif
/*
 * gzip declarations
 */
extern char input_data[];
extern char input_data_end[];

unsigned char *output_data;

extern unsigned long free_mem_ptr;
extern unsigned long free_mem_end_ptr;

void error(char *x)
{
	putstr("\n\n");
#ifdef CONFIG_KERNEL_COMPRESS_DEBUG
	backtrace();
#endif
	putstr(x);
	putstr("\n\n -- System halted");

	while(1);	/* Halt */
}

void __div0(void)
{
	error("Attempting division by 0!");
}

unsigned long __stack_chk_guard;

void __stack_chk_guard_setup(void)
{
	__stack_chk_guard = 0x000a0dff;
}

void __stack_chk_fail(void)
{
	error("stack-protector: Kernel stack is corrupted\n");
}

extern int do_decompress(uint8_t *input, int len, uint8_t *output, void (*error)(char *x));
int load_elf_image(unsigned long img_addr);
unsigned long elf_get_entry_addr(unsigned long base);

#ifdef CONFIG_KERNEL_COMPRESS_DEBUG
static inline uint64_t arch_counter_get_cntvct(void)
{
#ifdef CONFIG_ARCH_ARM
	uint64_t cval;

	isb();
	asm volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (cval));
#else
	uint64_t cval;
	uint32_t hi, lo;

	asm volatile("rdtimeh %0" : "=r" (hi):: "memory");
	asm volatile("rdtime  %0" : "=r" (lo):: "memory");
	cval = hi;
	cval = (cval << 32) | lo;
#endif
	return cval;
}
#endif

void decompress_kernel(unsigned long output_start, unsigned long free_mem_ptr_p,
		unsigned long free_mem_ptr_end_p,
		int arch_id)
{
	int ret;
	char str[16];
	__stack_chk_guard_setup();

	output_data	= (unsigned char *)output_start;
	free_mem_ptr = free_mem_ptr_p;
	free_mem_end_ptr = free_mem_ptr_end_p;
	__machine_arch_type	= arch_id;

	arch_decomp_setup();

#ifdef CONFIG_KERNEL_COMPRESS_DEBUG
	uint64_t decompress_start_ms = arch_counter_get_cntvct();
	putstr("\r\nUncompressing rtos: addr 0x");
	putstr(long2str(output_start, str));
	putstr("\r\n");
#endif

	ret = do_decompress((uint8_t *)input_data, ((int)input_data_end - (int)input_data),
			    (uint8_t *)output_data, error);

#ifdef CONFIG_KERNEL_COMPRESS_DEBUG
	uint32_t decompress_done_ms = (uint32_t)(arch_counter_get_cntvct() - decompress_start_ms)/40;
	str[0] = decompress_done_ms/1000000%10 + '0';
	str[1] = decompress_done_ms/100000%10 + '0';
	str[2] = decompress_done_ms/10000%10 + '0';
	str[3] = decompress_done_ms/1000%10 + '0';
	str[4] = '.';
	str[5] = decompress_done_ms/100%10 + '0';
	str[6] = decompress_done_ms/10%10 + '0';
	str[7] = decompress_done_ms%10 + '0';
	str[8] = 'm';
	str[9] = 's';
	str[10] = '\r';
	str[11] = '\n';
	str[12] = 0;
#endif
	putstr("Uncompressing rtos...\r\n");
	putstr("Total time:");
	putstr(str);
	if (ret)
		error("decompressor returned an error");
	else
		putstr("Done, booting the rtos.\n");
}
