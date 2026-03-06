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

#include <typedef.h>
#include <string.h>
#include "elf.h"

#ifdef CFG_ELF_64BIT
#define Elf_Ehdr				Elf64_Ehdr
#define Elf_Phdr				Elf64_Phdr
#define Elf_Shdr				Elf64_Shdr
#else
#define Elf_Ehdr				Elf32_Ehdr
#define Elf_Phdr				Elf32_Phdr
#define Elf_Shdr				Elf32_Shdr
#endif

static Elf_Shdr *elf_find_segment(unsigned long elf_addr, const char *seg_name)
{
	int i;
	Elf_Shdr *shdr;
	Elf_Ehdr *ehdr;
	const char *name_table;
	const uint8_t *elf_data = (void *)elf_addr;

	ehdr = (Elf_Ehdr *)elf_data;
	shdr = (Elf_Shdr *)(elf_data + ehdr->e_shoff);
	name_table = (const char *)(elf_data + shdr[ehdr->e_shstrndx].sh_offset);

	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		if (strcmp(name_table + shdr->sh_name, seg_name))
			continue;

		return shdr;
	}

	return NULL;
}

void *elf_find_segment_offset(unsigned long elf_addr, const char *seg_name)
{
	Elf_Shdr *shdr;

	shdr = elf_find_segment(elf_addr, seg_name);
	if (!shdr)
		return NULL;

	return (void *)elf_addr + shdr->sh_offset;
}

#if 0
static void *elf_find_segment_addr(unsigned long elf_addr, const char *seg_name)
{
	Elf_Shdr *shdr;

	shdr = elf_find_segment(elf_addr, seg_name);
	if (!shdr)
		return NULL;

	return (void *)shdr->sh_addr;
}
#endif

//#ifdef CONFIG_GEN_DIGEST
/* format: 4bytes image size + 16bytes digest */
__attribute__((section(".digest"))) const unsigned char digest_data[20];
//#endif

#ifdef CONFIG_COMPONENTS_AMP
/* reserved 256byte for resource_table */
static unsigned char __attribute__((__section__(".resource_table")))
		resource_table[256];

static void update_resource_table(unsigned long elf_addr)
{
	Elf_Shdr *shdr;
	void *dst, *src;

	shdr = elf_find_segment(elf_addr, ".resource_table");
	if (!shdr)
		return;

	dst = (void *)elf_addr + shdr->sh_offset;
	src = (void *)(resource_table);

	memcpy(dst, src, shdr->sh_size);
}
#endif
#ifdef CONFIG_AMP_SHARE_IRQ
static unsigned char __attribute__((__section__(".share_irq_table")))
		share_irq_table_info[8];

static void update_share_irq_table(unsigned long elf_addr)
{
	Elf_Shdr *shdr;
	void *dst, *src;

	shdr = elf_find_segment(elf_addr, ".share_irq_table");
	if (!shdr)
		return;

	dst = (void *)elf_addr + shdr->sh_offset;
	src = (void *)(share_irq_table_info);

	memcpy(dst, src, shdr->sh_size);
}
#endif

unsigned long elf_get_entry_addr(unsigned long base)
{
	Elf_Ehdr *ehdr;

	ehdr = (Elf_Ehdr *)base;

	return ehdr->e_entry;
}

int load_elf_image(unsigned long img_addr)
{
	int i;
	Elf_Ehdr *ehdr;
	Elf_Phdr *phdr;

	void *dst = NULL;
	void *src = NULL;

	ehdr = (Elf_Ehdr *)img_addr;
	phdr = (Elf_Phdr *)(img_addr + ehdr->e_phoff);

	/* mark decompress complete */
	ehdr->e_machine &= 0x00ff;
	asm volatile("dcache.cpa %0"::"r"((unsigned long)(&ehdr->e_machine)):"memory");
	asm volatile("fence":::"memory");

#ifdef CONFIG_COMPONENTS_AMP
	update_resource_table(img_addr);
#endif
#ifdef CONFIG_AMP_SHARE_IRQ
	update_share_irq_table(img_addr);
#endif
	/* load elf program segment */
	for (i = 0; i < ehdr->e_phnum; ++i, ++phdr) {
		dst = (void *)((unsigned long)phdr->p_paddr);
		src = (void *)(img_addr + phdr->p_offset);

		if (phdr->p_type != PT_LOAD)
			continue;

		if ((phdr->p_memsz == 0) || (phdr->p_filesz == 0))
			continue;

		if (phdr->p_filesz)
			memcpy(dst, src, phdr->p_filesz);

		if (phdr->p_filesz != phdr->p_memsz)
			memset((u8 *)dst + phdr->p_filesz, 0x00,
						phdr->p_memsz - phdr->p_filesz);
	}

	return 0;
}
