/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <hal_def.h>
#include <hal_cache.h>

#define __ASM_STR(x)   #x
#define CSR_MHCR       0x7c1
#define csr_set(csr, val)                   \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrs " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })
#define csr_clear(csr, val)                 \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrc " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#define SYSMAP_BASE                (0x3FFFF000UL)                     /*!< SYSMAP Base Address */
#define SYSMAP_REG_GROUP_GAP       (0x8UL)                            /*!< gap between 2 address group cfg register */
#define L1_CACHE_BYTES             (32)


void HAL_CACHE_CleanInvalidAllDcache(void)
{
	/* clean all dache */
	asm volatile(".word 0x0010000b":::"memory");
	/* invalid all dache */
	asm volatile(".word 0x0020000b":::"memory");
	asm volatile("fence.i");
}

void HAL_CACHE_CleanAllDcache(void)
{
	/* clean all dache */
	asm volatile(".word 0x0010000b":::"memory");
	asm volatile("fence.i");
}

void HAL_CACHE_IvalidAllDcache(void)
{
	/* invalid all dache */
	asm volatile(".word 0x0020000b":::"memory");
	asm volatile("fence.i");
}

void HAL_CACHE_CleanDcache(unsigned long addr, unsigned long size)
{
	register unsigned long i asm("a5");

	i = addr & ~(L1_CACHE_BYTES - 1);

	for (; i < (addr + size); i += L1_CACHE_BYTES)
	{
		asm volatile(".word 0x0297800b\n":::"memory");
	}
	asm volatile(".word 0x0000000f":::"memory");
	asm volatile("fence.i");
}

void HAL_CACHE_InvalidDcache(unsigned long addr, unsigned long size)
{
	register unsigned long i asm("a5");

    i = addr & ~(L1_CACHE_BYTES - 1);

    for (; i < (addr + size); i += L1_CACHE_BYTES)
    {
        asm volatile(".word 0x02a7800b\n":::"memory");
    }
    asm volatile(".word 0x0000000f":::"memory");
	asm volatile("fence.i");
}

void HAL_CACHE_EnableDcache(void)
{
	csr_set(CSR_MHCR, 0x1 << 1);
}

void HAL_CACHE_DisableDcache(void)
{
	csr_clear(CSR_MHCR, 0x1 << 1);
}

void HAL_CACHE_EnableIcache(void)
{
	csr_set(CSR_MHCR, 0x1 << 0);
}

void HAL_SYSMAP_Backup(sysmap_backup_t *cfg)
{
	uint32_t *backup_cfg = (uint32_t *)cfg;
	uint32_t attr_val;

	for (int i = 0; i < 8; i++) {
		/* backup addr0~7 address value */
		*backup_cfg++ = HAL_REG_32BIT(SYSMAP_BASE + SYSMAP_REG_GROUP_GAP * i);
		/* put addr0~7 attribute value to a 32bit value */
		attr_val = ((HAL_REG_32BIT(SYSMAP_BASE + SYSMAP_REG_GROUP_GAP * i + 4) >> 2) & 0x7UL);
		cfg->attr_set |= (attr_val << (3 * i));
	}
}

void HAL_SYSMAP_Restore(sysmap_backup_t *cfg)
{
	uint32_t *backup_cfg = (uint32_t *)cfg;
	uint32_t attr_val;

	for (int i = 0; i < 8; i++) {
		/* restore addr0~7 address value */
		HAL_REG_32BIT(SYSMAP_BASE + SYSMAP_REG_GROUP_GAP * i) = *backup_cfg++;
		/* restore addr0~7 attribute value */
		attr_val = ((cfg->attr_set >> (3 * i)) & 0x7UL) << 2;
		HAL_REG_32BIT(SYSMAP_BASE + SYSMAP_REG_GROUP_GAP * i + 4) = attr_val;
	}
}
