/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *	  1. Redistributions of source code must retain the above copyright
 *		 notice, this list of conditions and the following disclaimer.
 *	  2. Redistributions in binary form must reproduce the above copyright
 *		 notice, this list of conditions and the following disclaimer in the
 *		 documentation and/or other materials provided with the
 *		 distribution.
 *	  3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *		 its contributors may be used to endorse or promote products derived
 *		 from this software without specific prior written permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *	OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <flash_base.h>
#include <flash_head.h>
#include <string.h>

#define L1_CACHE_BYTES   (32)
#define __ASM_STR(x)     #x
#define CSR_TIME         0xc01
#define csr_read(csr)                       \
    ({                              \
        register unsigned long __v;             \
        __asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)    \
                              : "=r" (__v) :            \
                              : "memory");          \
        __v;                            \
    })


extern struct flash_head g_flash_head;
static flash_func_t g_flash_func;

int flash_base_register_function(flash_func_t *func)
{
	if (func == NULL) {
		return -1;
	}
	memcpy(&g_flash_func, func, sizeof(g_flash_func));
	return 0;
}

uint32_t flash_base_get_hosc_freq(void)
{
	if (*(volatile uint32_t *)(0x4A010404) & (1 << 31)) {
		return 24000000;
	}
	return 40000000;
}

uint32_t flash_base_get_us(void)
{
	return (csr_read(CSR_TIME) / (flash_base_get_hosc_freq() / 1000000));
}

void flash_base_udelay(uint32_t us)
{
	uint32_t mark;
	uint32_t timeout;

	if (us >= 1000 && g_flash_func.usleep) {
		g_flash_func.usleep(us);
		return;
	}
	mark = csr_read(CSR_TIME);
	timeout = us * (flash_base_get_hosc_freq() / 1000000);
	while (csr_read(CSR_TIME) - mark <= timeout);
}

void flash_base_flush_dcache(unsigned long addr, unsigned long size)
{
	register unsigned long i asm("a5");

	i = addr & ~(L1_CACHE_BYTES - 1);

	for (; i < (addr + size); i += L1_CACHE_BYTES)
	{
		/* clean && invalid */
		asm volatile(".word 0x02b7800b\n":::"memory");
	}
	asm volatile("fence w,w":::"memory");
}

void flash_base_invalid_dcache(unsigned long addr, unsigned long size)
{
	register unsigned long i asm("a5");

	i = addr & ~(L1_CACHE_BYTES - 1);

	for (; i < (addr + size); i += L1_CACHE_BYTES)
	{
		asm volatile(".word 0x02a7800b\n":::"memory");
	}
	asm volatile("fence w,w":::"memory");
}

int flash_base_ptinrf(const char *fmt, ...)
{
	va_list arg;
	int ret;

	if (g_flash_func.vprintf == NULL) {
		return -1;
	}
	va_start(arg, fmt);
	ret = g_flash_func.vprintf(fmt, arg);
	va_end(arg);
	return ret;
}

