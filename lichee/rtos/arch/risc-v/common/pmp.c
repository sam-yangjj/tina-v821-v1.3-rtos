/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.
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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <irqflags.h>
#include <excep.h>
#include <csr.h>
#include <console.h>
#include <limits.h>
#include <pmp.h>
#include <hal_interrupt.h>
#ifdef CONFIG_COMPONENTS_PM
#include "pm_devops.h"
#endif

#define PMP_COUNT			(CONFIG_PMP_REGIONS)
#define PMP_ADDR_SHIFT		2

#define MIN_ALIGN			(128)

#ifndef PMP_ALIGN
#define PMP_ALIGN(x)			(((x) + (CONFIG_PMP_ADDR_ALIGN) - 1) &\
										~((CONFIG_PMP_ADDR_ALIGN) - 1))
#endif

#define CSR_PMPCFG0			0x3a0
#define CSR_PMPCFG1			0x3a1
#define CSR_PMPCFG2			0x3a2
#define CSR_PMPCFG3			0x3a3
#define CSR_PMPADDR0			0x3b0
#define CSR_PMPADDR1			0x3b1
#define CSR_PMPADDR2			0x3b2
#define CSR_PMPADDR3			0x3b3
#define CSR_PMPADDR4			0x3b4
#define CSR_PMPADDR5			0x3b5
#define CSR_PMPADDR6			0x3b6
#define CSR_PMPADDR7			0x3b7
#define CSR_PMPADDR8			0x3b8
#define CSR_PMPADDR9			0x3b9
#define CSR_PMPADDR10			0x3ba
#define CSR_PMPADDR11			0x3bb
#define CSR_PMPADDR12			0x3bc
#define CSR_PMPADDR13			0x3bd
#define CSR_PMPADDR14			0x3be
#define CSR_PMPADDR15			0x3bf

#define NAPOT_MASK(x)			(unsigned long)(((uint64_t)x) - 1)
#define NAPOT_SIZE(x)			(unsigned long)((~((uint64_t)(x) >> 3)) & (NAPOT_MASK(x) >> 2))

#define PMP_A					0x18
#define PMP_A_OFF				0x00	/* 2b'00 */
#define PMP_A_TOR				0x08	/* 2b'01 */
#define PMP_A_NA4				0x10	/* 2b'10 not Support */
#define PMP_A_NAPOT				0x18	/* 2b'11 */
#define PMP_L					0x80

#ifdef CONFIG_COMPONENTS_PM
static int hal_pmp_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	csr_clear(CSR_MSTATUS, (1 << 17));

	return 0;
}

static int hal_pmp_resume(struct pm_device *dev, suspend_mode_t mode)
{
	csr_set(CSR_MSTATUS, (1 << 17));

	return 0;
}

static struct pm_devops pmp_devops = {
	.suspend = hal_pmp_suspend,
	.resume = hal_pmp_resume,
};

static struct pm_device pmp_dev = {
	.name = "pmp",
	.ops = &pmp_devops,
};
#endif

#ifdef CONFIG_PMP_DEBUG_CONFIG_SEQUENCE
typedef struct
{
	unsigned long start;
	unsigned long end;
	unsigned long prot;
	int result;
} pmp_config_record_t;

int s_record_index = 0;
int s_record_index_overflow_cnt = 0;
pmp_config_record_t s_pmp_config_records[CONFIG_PMP_CONFIG_RECORD_NUM];

static void record_pmp_config(unsigned long start, unsigned long end,
				unsigned long prot, int result)
{
	s_pmp_config_records[s_record_index].start = start;
	s_pmp_config_records[s_record_index].end = end;
	s_pmp_config_records[s_record_index].prot = prot;
	s_pmp_config_records[s_record_index].result = result;

	s_record_index++;
	if (s_record_index == CONFIG_PMP_CONFIG_RECORD_NUM)
	{
		s_record_index_overflow_cnt++;
		s_record_index = 0;
	}
}

void dump_pmp_config_record(void)
{
	unsigned long start, end, prot, result;
	int i;

	printf("PMP Configuration Record:\n");
	printf("current record num: %d\n", s_record_index);
	printf("record overflow count: %d\n", s_record_index_overflow_cnt);
	for (i = 0; i < s_record_index; i++)
	{
		start = s_pmp_config_records[i].start;
		end = s_pmp_config_records[i].end;
		prot = s_pmp_config_records[i].prot;
		result = s_pmp_config_records[i].result;
		printf("PMP config record %4d: [0x%p", i, (void *)start);
#ifdef CONFIG_PMP_USE_TOR_MODE
		printf("(0x%p)", (void *)PMP_ALIGN(start));
#endif
		printf(" - 0x%p] : %c%c%c",
						(void *)end,
						(prot & PMP_R) ? 'R' : ' ',
						(prot & PMP_W) ? 'W' : ' ',
						(prot & PMP_X) ? 'X' : ' ');
		printf(" : prot:0x%02lx : %s(%ld)\r\n",  prot, result ? "failed" : "success", result);
	}
}
#endif

#ifdef CONFIG_PMP_USE_NAPOT_MODE
static int __ffz(unsigned long val)
{
	int num = 0;
	unsigned long vval = val;

#if CONFIG_BITS_PER_LONG == 64
	if ((val & 0xffffffff) == 0xffffffff) {
		num += 32;
		val >>= 32;
	}
#endif
	if ((val & 0xffff) == 0xffff) {
		num += 16;
		val >>= 16;
	}
	if ((val & 0xff) == 0xff) {
		num += 8;
		val >>= 8;
	}
	if ((val & 0xf) == 0xf) {
		num += 4;
		val >>= 4;
	}
	if ((val & 0x3) == 0x3) {
		num += 2;
		val >>= 2;
	}
	if ((val & 0x1) == 0x1)
		num += 1;

	if ((vval & (1 << num)) != 0)
		return -ENODEV;
	return num;
}
#endif

// fix unused-function
__attribute__((__unused__)) static int get_free_pmp(void)
{
#if PMP_COUNT > 12
	if (((csr_read(CSR_PMPCFG3) >> 24) & PMP_A) == PMP_A_OFF)
		return 15;
	if (((csr_read(CSR_PMPCFG3) >> 16) & PMP_A) == PMP_A_OFF)
		return 14;
	if (((csr_read(CSR_PMPCFG3) >> 8 ) & PMP_A) == PMP_A_OFF)
		return 13;
	if (((csr_read(CSR_PMPCFG3) >> 0 ) & PMP_A) == PMP_A_OFF)
		return 12;
#endif
#if PMP_COUNT > 8
	if (((csr_read(CSR_PMPCFG2) >> 24) & PMP_A) == PMP_A_OFF)
		return 11;
	if (((csr_read(CSR_PMPCFG2) >> 16) & PMP_A) == PMP_A_OFF)
		return 10;
	if (((csr_read(CSR_PMPCFG2) >> 8 ) & PMP_A) == PMP_A_OFF)
		return  9;
	if (((csr_read(CSR_PMPCFG2) >> 0 ) & PMP_A) == PMP_A_OFF)
		return  8;
#endif
#if PMP_COUNT > 4
	if (((csr_read(CSR_PMPCFG1) >> 24) & PMP_A) == PMP_A_OFF)
		return  7;
	if (((csr_read(CSR_PMPCFG1) >> 16) & PMP_A) == PMP_A_OFF)
		return  6;
	if (((csr_read(CSR_PMPCFG1) >> 8 ) & PMP_A) == PMP_A_OFF)
		return  5;
	if (((csr_read(CSR_PMPCFG1) >> 0 ) & PMP_A) == PMP_A_OFF)
		return  4;
#endif
	if (((csr_read(CSR_PMPCFG0) >> 24) & PMP_A) == PMP_A_OFF)
		return  3;
	if (((csr_read(CSR_PMPCFG0) >> 16) & PMP_A) == PMP_A_OFF)
		return  2;
	if (((csr_read(CSR_PMPCFG0) >> 8 ) & PMP_A) == PMP_A_OFF)
		return  1;
	if (((csr_read(CSR_PMPCFG0) >> 0 ) & PMP_A) == PMP_A_OFF)
		return  0;
	return -ENODEV;
}

static void set_pmpcfg(int index, uint8_t val)
{
	switch (index) {
	case 0:
		csr_write(CSR_PMPCFG0, (csr_read(CSR_PMPCFG0) & ~(0xff << 0 )) | (val << 0 ));
	case 1:
		csr_write(CSR_PMPCFG0, (csr_read(CSR_PMPCFG0) & ~(0xff << 8 )) | (val << 8 ));
	case 2:
		csr_write(CSR_PMPCFG0, (csr_read(CSR_PMPCFG0) & ~(0xff << 16)) | (val << 16));
	case 3:
		csr_write(CSR_PMPCFG0, (csr_read(CSR_PMPCFG0) & ~(0xff << 24)) | (val << 24));
	case 4:
		csr_write(CSR_PMPCFG1, (csr_read(CSR_PMPCFG1) & ~(0xff << 0 )) | (val << 0 ));
	case 5:
		csr_write(CSR_PMPCFG1, (csr_read(CSR_PMPCFG1) & ~(0xff << 8 )) | (val << 8 ));
	case 6:
		csr_write(CSR_PMPCFG1, (csr_read(CSR_PMPCFG1) & ~(0xff << 16)) | (val << 16));
	case 7:
		csr_write(CSR_PMPCFG1, (csr_read(CSR_PMPCFG1) & ~(0xff << 24)) | (val << 24));
	case 8:
		csr_write(CSR_PMPCFG2, (csr_read(CSR_PMPCFG2) & ~(0xff << 0 )) | (val << 0 ));
	case 9:
		csr_write(CSR_PMPCFG2, (csr_read(CSR_PMPCFG2) & ~(0xff << 8 )) | (val << 8 ));
	case 10:
		csr_write(CSR_PMPCFG2, (csr_read(CSR_PMPCFG2) & ~(0xff << 16)) | (val << 16));
	case 11:
		csr_write(CSR_PMPCFG2, (csr_read(CSR_PMPCFG2) & ~(0xff << 24)) | (val << 24));
	case 12:
		csr_write(CSR_PMPCFG3, (csr_read(CSR_PMPCFG3) & ~(0xff << 0 )) | (val << 0 ));
	case 13:
		csr_write(CSR_PMPCFG3, (csr_read(CSR_PMPCFG3) & ~(0xff << 8 )) | (val << 8 ));
	case 14:
		csr_write(CSR_PMPCFG3, (csr_read(CSR_PMPCFG3) & ~(0xff << 16)) | (val << 16));
	case 15:
		csr_write(CSR_PMPCFG3, (csr_read(CSR_PMPCFG3) & ~(0xff << 24)) | (val << 24));
	}
}

static uint8_t get_pmpcfg(int index)
{
	switch (index) {
	case 0:
		return (csr_read(CSR_PMPCFG0) >> 0 ) & 0xff;
	case 1:
		return (csr_read(CSR_PMPCFG0) >> 8 ) & 0xff;
	case 2:
		return (csr_read(CSR_PMPCFG0) >> 16) & 0xff;
	case 3:
		return (csr_read(CSR_PMPCFG0) >> 24) & 0xff;
	case 4:
		return (csr_read(CSR_PMPCFG1) >> 0 ) & 0xff;
	case 5:
		return (csr_read(CSR_PMPCFG1) >> 8 ) & 0xff;
	case 6:
		return (csr_read(CSR_PMPCFG1) >> 16) & 0xff;
	case 7:
		return (csr_read(CSR_PMPCFG1) >> 24) & 0xff;
	case 8:
		return (csr_read(CSR_PMPCFG2) >> 0 ) & 0xff;
	case 9:
		return (csr_read(CSR_PMPCFG2) >> 8 ) & 0xff;
	case 10:
		return (csr_read(CSR_PMPCFG2) >> 16) & 0xff;
	case 11:
		return (csr_read(CSR_PMPCFG2) >> 24) & 0xff;
	case 12:
		return (csr_read(CSR_PMPCFG3) >> 0 ) & 0xff;
	case 13:
		return (csr_read(CSR_PMPCFG3) >> 8 ) & 0xff;
	case 14:
		return (csr_read(CSR_PMPCFG3) >> 16) & 0xff;
	case 15:
		return (csr_read(CSR_PMPCFG3) >> 24) & 0xff;
	default:
		return 0;
	}
}

static unsigned long get_pmpaddr(int index)
{
	switch (index) {
	case 0:
		return csr_read(CSR_PMPADDR0);
	case 1:
		return csr_read(CSR_PMPADDR1);
	case 2:
		return csr_read(CSR_PMPADDR2);
	case 3:
		return csr_read(CSR_PMPADDR3);
	case 4:
		return csr_read(CSR_PMPADDR4);
	case 5:
		return csr_read(CSR_PMPADDR5);
	case 6:
		return csr_read(CSR_PMPADDR6);
	case 7:
		return csr_read(CSR_PMPADDR7);
	case 8:
		return csr_read(CSR_PMPADDR8);
	case 9:
		return csr_read(CSR_PMPADDR9);
	case 10:
		return csr_read(CSR_PMPADDR10);
	case 11:
		return csr_read(CSR_PMPADDR11);
	case 12:
		return csr_read(CSR_PMPADDR12);
	case 13:
		return csr_read(CSR_PMPADDR13);
	case 14:
		return csr_read(CSR_PMPADDR14);
	case 15:
		return csr_read(CSR_PMPADDR15);
	default:
		return 0;
	}
}

static void set_pmpaddr(int index, unsigned long val)
{
	switch (index) {
	case 0:
		csr_write(CSR_PMPADDR0, val);
	case 1:
		csr_write(CSR_PMPADDR1, val);
	case 2:
		csr_write(CSR_PMPADDR2, val);
	case 3:
		csr_write(CSR_PMPADDR3, val);
	case 4:
		csr_write(CSR_PMPADDR4, val);
	case 5:
		csr_write(CSR_PMPADDR5, val);
	case 6:
		csr_write(CSR_PMPADDR6, val);
	case 7:
		csr_write(CSR_PMPADDR7, val);
	case 8:
		csr_write(CSR_PMPADDR8, val);
	case 9:
		csr_write(CSR_PMPADDR9, val);
	case 10:
		csr_write(CSR_PMPADDR10, val);
	case 11:
		csr_write(CSR_PMPADDR11, val);
	case 12:
		csr_write(CSR_PMPADDR12, val);
	case 13:
		csr_write(CSR_PMPADDR13, val);
	case 14:
		csr_write(CSR_PMPADDR14, val);
	case 15:
		csr_write(CSR_PMPADDR15, val);
	}
}

static unsigned long get_addr(int idx)
{
#ifdef CONFIG_PMP_USE_NAPOT_MODE
	unsigned long addr;
	int bit;

	addr = get_pmpaddr(idx);

	bit = __ffz(addr);
	if (bit < 0)
		return 0;
	return (addr & (~((1 << (bit + 1)) - 1))) << PMP_ADDR_SHIFT;
#endif
#ifdef CONFIG_PMP_USE_TOR_MODE
	if (idx == 0 || idx >= PMP_COUNT)
		return 0;
	return get_pmpaddr(idx - 1) << PMP_ADDR_SHIFT;
#endif
}

static uint64_t get_size(int idx)
{
#ifdef CONFIG_PMP_USE_NAPOT_MODE
	unsigned long addr;
	uint64_t size = 1;
	int bit;

	addr = get_pmpaddr(idx);

	bit = __ffz(addr);
	if (bit < 0)
		return 0;
	bit += 3;
	size <<= bit;
	return size;
#endif
#ifdef CONFIG_PMP_USE_TOR_MODE
	if (idx >= PMP_COUNT)
		return 0;
	if (idx == 0)
		return get_pmpaddr(idx) << PMP_ADDR_SHIFT;
	else
		return (get_pmpaddr(idx) - get_pmpaddr(idx - 1)) << PMP_ADDR_SHIFT;
#endif
}

#ifdef CONFIG_PMP_USE_NAPOT_MODE
static unsigned long napot_size2mask(unsigned long size)
{
#define SIZE_CASE(x)	if ((uint64_t)size == ((uint64_t)x - 1)) return NAPOT_MASK(x)
	SIZE_CASE(128);
	SIZE_CASE(256);
	SIZE_CASE(512);
	SIZE_CASE(1024);
	SIZE_CASE(1024 * 2);
	SIZE_CASE(1024 * 4);
	SIZE_CASE(1024 * 8);
	SIZE_CASE(1024 * 16);
	SIZE_CASE(1024 * 32);
	SIZE_CASE(1024 * 64);
	SIZE_CASE(1024 * 128);
	SIZE_CASE(1024 * 256);
	SIZE_CASE(1024 * 512);
	SIZE_CASE(1024 * 1024);
	SIZE_CASE(1024 * 1024 * 2);
	SIZE_CASE(1024 * 1024 * 4);
	SIZE_CASE(1024 * 1024 * 8);
	SIZE_CASE(1024 * 1024 * 16);
	SIZE_CASE(1024 * 1024 * 32);
	SIZE_CASE(1024 * 1024 * 64);
	SIZE_CASE(1024 * 1024 * 128);
	SIZE_CASE(1024 * 1024 * 256);
	SIZE_CASE(1024 * 1024 * 512);
	SIZE_CASE(1024 * 1024 * 1024);
	SIZE_CASE(1024 * 1024 * 1024 * 2);
	SIZE_CASE(1024 * 1024 * 1024 * 4);
#if CONFIG_BITS_PER_LONG == 64
	SIZE_CASE(1024 * 1024 * 1024 * 8);
	SIZE_CASE(1024 * 1024 * 1024 * 16);
#endif
	return 0;
}

int pmp_add_region(unsigned long start, unsigned long end,
				unsigned long prot)
{
	int idx = get_free_pmp();
	unsigned long mask;
	uint64_t size;
	int ret = 0;

#ifdef CONFIG_PMP_DEBUG_CONFIG_SEQUENCE
	unsigned long start_bak, end_bak, prot_bak;
	start_bak = start;
	end_bak = end;
	prot_bak = prot;
#endif

	if (idx < 0) {
		ret = -ENOMEM;
		goto exit;
	}

	mask = napot_size2mask(end - start);
	if (!mask) {
		printf("don't support 0x%lx\r\n", end - start);
		ret = -EINVAL;
		goto exit;
	}
	if (start & mask) {
		printf("addr need to align 0x%lx\r\n", mask);
		ret = -EINVAL;
		goto exit;
	}


	size = (uint64_t)end - (uint64_t)start + 1;
	printf("0x%llx mask = 0x%x\r\n", size, mask);

	start >>= PMP_ADDR_SHIFT;
	start |= NAPOT_SIZE(size);
	prot &= 0x7;
	prot |= PMP_A_NAPOT;
	prot |= PMP_L;

	printf("write 0x%lx -> pmpaddr%d\r\n", start, idx);
	printf("write 0x%lx -> pmpcfg%d\r\n", prot, idx);
	set_pmpaddr(idx, start);
	set_pmpcfg(idx, prot);

	ret = 0;

exit:

#ifdef CONFIG_PMP_DEBUG_CONFIG_SEQUENCE
	record_pmp_config(start_bak, end_bak, prot_bak, ret);
#endif

	return ret;
}
#endif /* CONFIG_PMP_USE_NAPOT_MODE */

#ifdef CONFIG_PMP_USE_TOR_MODE
static uint32_t pmpaddr_tmp[PMP_COUNT];
static uint8_t pmpcfg_tmp[PMP_COUNT];

static void write_pmp_prepare(void)
{
	int i;

	memset(pmpaddr_tmp, 0, sizeof(pmpaddr_tmp));
	memset(pmpcfg_tmp, 0, sizeof(pmpcfg_tmp));

	for (i = 0; i < PMP_COUNT; i++) {
		pmpaddr_tmp[i] = get_pmpaddr(i);
		pmpcfg_tmp[i] = get_pmpcfg(i);
	}
}

static int write_pmp_region(int idx, uint32_t addr, uint8_t cfg)
{
	if (idx < 0 || idx > PMP_COUNT)
		return -EINVAL;

	pmpaddr_tmp[idx] = addr;
	if (cfg != 0xff)
		pmpcfg_tmp[idx] = cfg;

	return 0;
}

static inline uint32_t read_pmpaddr(int idx)
{
	return pmpaddr_tmp[idx];
}

static inline uint32_t read_pmpcfg(int idx)
{
	return pmpcfg_tmp[idx];
}

static void write_pmp_finish(void)
{
	int i;

	for (i = 0; i < PMP_COUNT; i++) {
		set_pmpaddr(i, pmpaddr_tmp[i]);
		set_pmpcfg(i, pmpcfg_tmp[i]);
	}
}

static int pmp_move_down(int start, int n)
{
	int i;
	int free = 0;
	int s = 0;

	for (i = PMP_COUNT - 1; i >= 0; i--) {
		if ((read_pmpcfg(i) & PMP_A) != PMP_A_OFF)
			break;
		free++;
	}

	if (free < n)
		return -ENOMEM;

	for (i = PMP_COUNT - 1; i >= 0; i--) {
		if ((read_pmpcfg(i) & PMP_A) != PMP_A_OFF) {
			s = 1;
		}
		if (s && (i + n) < PMP_COUNT) {
			write_pmp_region(i + n, read_pmpaddr(i), read_pmpcfg(i));
			write_pmp_region(i, 0, 0);
		}
		if (i == start)
			break;
	}

	return 0;
}

int pmp_add_region(unsigned long start, unsigned long end,
				unsigned long prot)
{
	int i;
	unsigned long region_s, region_e;
	unsigned long tmp_prot;
	int type = -1;
	int ret = 0;

#ifdef CONFIG_PMP_DEBUG_CONFIG_SEQUENCE
	unsigned long start_bak, end_bak, prot_bak;
	start_bak = start;
	end_bak = end;
	prot_bak = prot;
#endif

	if (get_pmpcfg(0) & PMP_L) {
		printf("PMP already lock, Can't change\r\n");
		ret = -EBUSY;
		goto exit;
	}

	start = PMP_ALIGN(start);

	/*
	 * type 0: new
	 * type 1: contain on
	 * type 2: insert
	 * type 3: expand
	 * */
	for (i = 0; i < PMP_COUNT; i++) {
		tmp_prot = get_pmpcfg(i);
		region_s = get_addr(i);
		region_e = region_s + get_size(i);

		if ((tmp_prot & PMP_A) == PMP_A_OFF || region_e == 0) {
			if (i == 0 || ((get_addr(i - 1) + get_size(i - 1)) == start))
				type = 0;
			break;
		}
		if (start >= region_s && end <= region_e) {
			type = 1;
			break;
		}
		if (start > region_s && start < region_e &&
						end > region_e) {
			type = 2;

			if ((i + 2) < PMP_COUNT &&
						((get_pmpcfg(i + 2) & PMP_A) != PMP_A_OFF) &&
						end > get_addr(i + 2))
				type = 3;
			break;
		}
		if (start <= region_s && end >= region_e) {
			type = 3;
			break;
		}
	}

	if (type == -1) {
		printf("Can't add region [0x%lx,0x%lx)\r\n", start, end);
		ret = -EINVAL;
		goto exit;
	}

	prot &= 0x7;
	prot |= PMP_A_TOR;

	write_pmp_prepare();
	if (type == 0) {
		write_pmp_region(i, end >> PMP_ADDR_SHIFT, prot);
	} else if (type == 1) {
		if (prot != tmp_prot) {
			if (start == region_s && end == region_e) {
				write_pmp_region(i, read_pmpaddr(i), prot);
			} else if (start == region_s || end == region_e) {
				if (!((i && region_s == start && read_pmpcfg(i - 1) == prot) ||
						(region_e == end && read_pmpcfg(i + 1) == prot))) {
					if (pmp_move_down(i + 1, 1) != 0) {
						printf("Can't split region %d\r\n", i + 1);
						ret = -ENOMEM;
						goto exit;
					}
				}
				if (start == region_s) {
					if (i && read_pmpcfg(i - 1) == prot) {
						write_pmp_region(i - 1, end >> PMP_ADDR_SHIFT, 0xff);
					} else {
						write_pmp_region(i + 1, read_pmpaddr(i), read_pmpcfg(i));
						write_pmp_region(i, end >> PMP_ADDR_SHIFT, prot);
					}
				} else {
					write_pmp_region(i, start >> PMP_ADDR_SHIFT, 0xff);
					if (i < PMP_COUNT && read_pmpcfg(i + 1) != prot)
						write_pmp_region(i + 1, end >> PMP_ADDR_SHIFT, prot);
				}
			} else {
				if (pmp_move_down(i + 1, 2) != 0) {
					printf("Can't split region %d\r\n", i + 1);
					ret = -ENOMEM;
					goto exit;
				}

				write_pmp_region(i + 2, read_pmpaddr(i), read_pmpcfg(i));
				write_pmp_region(i + 1, end >> PMP_ADDR_SHIFT, prot);
				write_pmp_region(i, start >> PMP_ADDR_SHIFT, 0xff);
			}
		}
	} else if (type == 2) {
		if (read_pmpcfg(i) == prot) {
			write_pmp_region(i, end >> PMP_ADDR_SHIFT, 0xff);
		} else if (read_pmpcfg(i + 1) == prot) {
			write_pmp_region(i, start >> PMP_ADDR_SHIFT, 0xff);
		} else {
			if (pmp_move_down(i + 1, 1) != 0) {
				printf("Can't split region %d\r\n", i + 1);
				ret = -ENOMEM;
				goto exit;
			}
			write_pmp_region(i, start >> PMP_ADDR_SHIFT, 0xff);
			write_pmp_region(i + 1, end >> PMP_ADDR_SHIFT, prot);
		}
	} else if (type == 3) {
		write_pmp_region(i - 1, start >> PMP_ADDR_SHIFT, 0xff);
		write_pmp_region(i, end >> PMP_ADDR_SHIFT, prot);
	}

	/*
	 * FIXME:
	 *  For the setup to be successful,
	 *  we need to close the remaining PMP_A_OFF
	 **/
	write_pmp_finish();
	ret = 0;

exit:

#ifdef CONFIG_PMP_DEBUG_CONFIG_SEQUENCE
	record_pmp_config(start_bak, end_bak, prot_bak, ret);
#endif

	return ret;
}
#endif /* CONFIG_PMP_USE_TOR_MODE */

void pmp_enable(void)
{
	int i;

#ifdef CONFIG_PMP_USE_TOR_MODE
	uint8_t cfgback[PMP_COUNT];
	uint32_t addrback[PMP_COUNT];
	unsigned long flags;

	for (i = 0; i < PMP_COUNT; i++) {
		cfgback[i] = get_pmpcfg(i);
		addrback[i] = get_pmpaddr(i);
	}

	flags = hal_interrupt_disable_irqsave();
	for (i = PMP_COUNT - 1; i >= 0; i--) {
		unsigned long prot = cfgback[i];

		prot |= PMP_L;
		set_pmpaddr(i, addrback[i]);
		set_pmpcfg(i, prot);
	}
	hal_interrupt_enable_irqrestore(flags);
#endif
	csr_set(CSR_MSTATUS, (1 << 17));

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_register(&pmp_dev);
#endif
}

int pmp_dump_region(int idx)
{
	unsigned long addr, prot;
	uint64_t size;
	const char *mode = "OFF  ";

	prot = get_pmpcfg(idx);

	if ((prot & PMP_A) == PMP_A_OFF || get_pmpaddr(idx) == 0) {
		mode = "OFF  ";
	} else if ((prot & PMP_A) == PMP_A_TOR)
		mode = "TOR  ";
	else if ((prot & PMP_A) == PMP_A_NA4)
		mode = "NA4  ";
	else if ((prot & PMP_A) == PMP_A_NAPOT)
		mode = "NAPOT";

	size = get_size(idx);
	addr = get_addr(idx);

	printf("PMP%2d: %s [0x%08lx - 0x%08lx) : %c%c%c",
					idx, mode, addr,
					(unsigned long)(size + addr),
					(prot & PMP_R) ? 'R' : ' ',
					(prot & PMP_W) ? 'W' : ' ',
					(prot & PMP_X) ? 'X' : ' ');
	printf(" : pmpaddr:0x%08lx prot:0x%02lx\r\n",
					get_pmpaddr(idx), prot);

	return 0;
}

/*
 * return value: -1: addr not match,
 *               >=0: addr match
 *                   0: no permisson
 *                   1: has permission
 */
static int pmp_entry_check_addr_permission(int idx, unsigned long addr, int pmp_permission)
{
	//TODO: currently only support TOR mode
#ifdef CONFIG_PMP_USE_TOR_MODE
	unsigned long start_addr, end_addr, prot;
	uint64_t size;

	prot = get_pmpcfg(idx);

	if ((prot & PMP_A) == PMP_A_OFF || get_pmpaddr(idx) == 0) {
		return -1;
	} else if ((prot & PMP_A) != PMP_A_TOR) {
		printf("invalid PMP entry %d, cfg: 0x%lx\n", idx, prot);
		return -1;
	}

	size = get_size(idx);
	start_addr = get_addr(idx);
	end_addr = start_addr + size - 1;

	if ((addr >= start_addr) && (addr <= end_addr)) {
		if (prot & pmp_permission) {
			return 1;
		} else {
			return 0;
		}
	}
#endif
	return -1;
}

int pmp_check_addr_permission(unsigned long addr, int pmp_permission)
{
	int i, ret;
	char permission_char;

	if (pmp_permission == PMP_R) {
		permission_char = 'R';
	} else if (pmp_permission == PMP_W) {
		permission_char = 'W';
	} else if (pmp_permission == PMP_X) {
		permission_char = 'X';
	} else {
		printf("Invalid PMP permission(%d)!\n", pmp_permission);
		return -1;
	}

	for (i = 0; i < PMP_COUNT; i++) {
		ret = pmp_entry_check_addr_permission(i, addr, pmp_permission);
		if (ret < 0)
			continue;

		printf("Target addr(0x%p) match PMP entry %d, ", (void *)addr, i);
		if (ret) {
			printf("and it has permission(%c)\n", permission_char);
			return 1;
		} else {
			printf("but it does't have permission(%c)\n", permission_char);
			return 0;
		}
	}

	return -2;
}

int cmd_pmpset(int argc, const char **argv)
{
	unsigned long addr, end;
	unsigned prot = 0;

	if (argc != 4) {
		printf ("Useage:\r\n");
		printf ("\t: pmp_set start end RWX\r\n");
		printf ("e.g.\r\n");
		printf ("\t: pmp_set 0xc0000000 0x3fffffff R\r\n");
		return 0;
	}

	addr = strtoul(argv[1], NULL, 0);
	if (addr == ULONG_MAX) {
		printf("invalid addr : %s\r\n", argv[1]);
		return 0;
	}

	end = strtoul(argv[2], NULL, 0);

	if (strchr(argv[3], 'R') != NULL)
		prot |= PMP_R;
	if (strchr(argv[3], 'W') != NULL)
		prot |= PMP_W;
	if (strchr(argv[3], 'X') != NULL)
		prot |= PMP_X;

#ifdef CONFIG_PMP_USE_NAPOT_MODE
	pmp_add_region(addr, end - 1, prot);
#endif
#ifdef CONFIG_PMP_USE_TOR_MODE
	pmp_add_region(addr, end, prot);
#endif

	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_pmpset, pmp_set, set pmp info);

int cmd_pmpdump(int argc, const char **argv)
{
	int i;

	for (i = 0; i < PMP_COUNT; i++) {
		pmp_dump_region(i);
	}
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_pmpdump, pmp_dump, dump pmp info);

int cmd_pmp_enable(int argc, const char **argv)
{
	pmp_enable();
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_pmp_enable, pmp_enable, enable pmp);

#ifdef CONFIG_PMP_DEBUG_CONFIG_SEQUENCE
int cmd_dump_pmp_config_record(int argc, const char **argv)
{
	dump_pmp_config_record();
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_dump_pmp_config_record, pmp_dump_record, dump pmp configuration records);
#endif
