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

#include <string.h>
#include <load_image.h>
#include <pm_debug.h>
#include <hal_mem.h>
#include <hal_cmd.h>
#include <pm_mem.h>
#include <flash_read.h>

//#define LOAD_IMAGE_DEBUG

/*
#ifdef CONFIG_COMPONENTS_AW_FLASH_BOOTPKG_OFFSET
#define BOOTPKG_START_SECTOR    (CONFIG_COMPONENTS_AW_FLASH_BOOTPKG_OFFSET)
#else
#error "should not hardcode bootpkg start sector"
#endif
*/

/* elf data struct */
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

#define EI_NIDENT   (16)
typedef struct elf32_hdr {
	unsigned char	e_ident[EI_NIDENT]; /* ELF Identification */

	Elf32_Half	e_type;		/* object file type */
	Elf32_Half	e_machine;	/* machine */
	Elf32_Word	e_version;	/* object file version */
	Elf32_Addr	e_entry;	/* virtual entry point */

	Elf32_Off	e_phoff;	/* program header table offset */
	Elf32_Off	e_shoff;	/* section header table offset */
	Elf32_Word	e_flags;	/* processor-specific flags */
	Elf32_Half	e_ehsize;	/* ELF header size */

	Elf32_Half	e_phentsize;	/* program header entry size */
	Elf32_Half	e_phnum;	/* number of program header entries */
	Elf32_Half	e_shentsize;	/* section header entry size */
	Elf32_Half	e_shnum;	/* number of section header entries */

	Elf32_Half	e_shstrndx;	/* section header table's "section
					   header string table" entry offset */
} Elf32_Ehdr;

typedef struct  elf32_phdr {
	Elf32_Word	p_type;		/* segment type */
	Elf32_Off	p_offset;	/* segment offset */
	Elf32_Addr	p_vaddr;	/* virtual address of segment */
	Elf32_Addr	p_paddr;	/* physical address of segment */
	Elf32_Word	p_filesz;	/* number of bytes in file for seg */
	Elf32_Word	p_memsz;	/* number of bytes in mem. for seg */
	Elf32_Word	p_flags;	/* flags */
	Elf32_Word	p_align;	/* memory alignment */
} Elf32_Phdr;

typedef struct elf32_shdr {
  Elf32_Word    sh_name;
  Elf32_Word    sh_type;
  Elf32_Word    sh_flags;
  Elf32_Addr    sh_addr;
  Elf32_Off sh_offset;
  Elf32_Word    sh_size;
  Elf32_Word    sh_link;
  Elf32_Word    sh_info;
  Elf32_Word    sh_addralign;
  Elf32_Word    sh_entsize;
} Elf32_Shdr;

/* boot package data struct */
struct sbrom_toc1_head_info
{
    char name[16]   ;   //user can modify
    uint32_t  magic  ;   //must equal TOC_U32_MAGIC
    uint32_t  add_sum    ;

    uint32_t  serial_num ;   //user can modify
    uint32_t  status     ;   //user can modify,such as TOC_MAIN_INFO_STATUS_ENCRYP_NOT_USED

    uint32_t  items_nr;  //total entry number
    uint32_t  valid_len;
    uint32_t  version_main;  //only one byte
    uint32_t  version_sub;   //two bytes
    uint32_t  reserved[3];   //reserved for future

    uint32_t  end;
};

struct sbrom_toc1_item_info
{
    char name[64];          //such as ITEM_NAME_SBROMSW_CERTIF
    uint32_t  data_offset;
    uint32_t  data_len;
    uint32_t  encrypt;           //0: no aes   //1: aes
    uint32_t  type;              //0: normal file, dont care  1: key certif  2: sign certif 3: bin file
    uint32_t  run_addr;          //if it is a bin file, then run on this address; if not, it should be 0
    uint32_t  index;             //if it is a bin file, this value shows the index to run; if not
                           //if it is a certif file, it should equal to the bin file index
                           //that they are in the same group
                           //it should be 0 when it anyother data type
    uint32_t  reserved[69];     //reserved for future;
    uint32_t  end;
};

/* pm boot data struct */
#define MAGIC_SIZE   8
typedef struct _Boot_file_head
{
	uint32_t  jump_instruction;   /* one intruction jumping to real code */
	uint8_t   magic[MAGIC_SIZE];  /* ="eGON.BT0" */
	uint32_t  check_sum;          /* generated by PC */
	uint32_t  length;             /* generated by PC */
	uint32_t  pub_head_size;      /* the size of boot_file_head_t */
	uint8_t   pub_head_vsn[4];    /* the version of boot_file_head_t */
	uint32_t ret_addr;           /* the return value */
	uint32_t  run_addr;           /* run addr */
	uint32_t  boot_cpu;           /* eGON version */
	uint8_t   platform[8];        /* platform information */
}boot_file_head_t;

/* common define */
#define TOC_MAIN_INFO_MAGIC             (0x89119800)
#define DRAM_SIZE_OFFSET                (792U)
#define ALIGN_SECTOR(x)                 (((x) + (LOAD_SECTOR_SIZE - 1)) & ~(LOAD_SECTOR_SIZE - 1))
#define FIND_SECTOR(addr)               (addr / LOAD_SECTOR_SIZE * LOAD_SECTOR_SIZE)
#define GET_MAX(a, b)                   ((a) > (b) ? (a) : (b))

#define START_SECTOR(addr)              ((addr) / LOAD_SECTOR_SIZE)
#define SECTOR_OFFSET(addr)             ((addr) % LOAD_SECTOR_SIZE)
#define START_ADDRESS_OF_SECTOR(addr)   ((addr) & ~(LOAD_SECTOR_SIZE - 1))
#define END_ADDRESS_OF_SECTOR(addr)     (((addr) + (LOAD_SECTOR_SIZE - 1)) & ~(LOAD_SECTOR_SIZE - 1))
#define LOAD_SECTOR_NUM(addr, size)     ((END_ADDRESS_OF_SECTOR((addr) + (size)) - START_ADDRESS_OF_SECTOR(addr)) / LOAD_SECTOR_SIZE)

static __standby_unsaved_bss uint8_t *bootpkg_buf = NULL;
static uint32_t bootpkg_start_sector = 0;

void bootpkg_set_start_sector(uint32_t sector)
{
	bootpkg_start_sector = sector;
}

uint32_t bootpkg_get_start_sector(void)
{
#ifdef CONFIG_COMPONENTS_AW_FLASH_BOOTPKG_OFFSET
	if (bootpkg_start_sector == 0) {
		bootpkg_start_sector = CONFIG_COMPONENTS_AW_FLASH_BOOTPKG_OFFSET;
	}
#endif
	return bootpkg_start_sector;
}

int bootpkg_init(void)
{
	int ret = 0;
	struct sbrom_toc1_head_info	*toc1_head;
	uint32_t len;

	load_inf("bootpackage start sector: %u\n", bootpkg_start_sector);

	bootpkg_buf = (uint8_t *)hal_malloc(LOAD_SECTOR_SIZE);
	if (bootpkg_buf == NULL) {
		load_err("bootpkg init malloc fail_1\n");
		ret = -1;
		goto exit;
	}
	load_inf("bootpkg init malloc_1: 0x%08x\n", LOAD_SECTOR_SIZE);
	ret = spl_flash_read(bootpkg_start_sector , 1, bootpkg_buf);
	if (ret) {
		load_err("bootpkg init flash read fail_1\n");
		hal_free(bootpkg_buf);
		bootpkg_buf = NULL;
		goto exit;
	}
	toc1_head = (struct sbrom_toc1_head_info *)bootpkg_buf;
	if (toc1_head->magic != TOC_MAIN_INFO_MAGIC) {
		load_err("bootpkg init magic fail\n");
		hal_free(bootpkg_buf);
		bootpkg_buf = NULL;
		goto exit;
	}
	/* calculate boot package head and all entry total size */
	len = sizeof(struct sbrom_toc1_head_info) + sizeof(struct sbrom_toc1_item_info) * toc1_head->items_nr;
	load_inf("source len: 0x%08x\n", len);
	len = ALIGN_SECTOR(len);
	load_inf("align len: 0x%08x\n", len);
	if (len > LOAD_SECTOR_SIZE) {
		hal_free(bootpkg_buf);
		bootpkg_buf = NULL;
		bootpkg_buf = (uint8_t *)hal_malloc(len);
		if (bootpkg_buf == NULL) {
			load_err("bootpkg init malloc fail_2\n");
			goto exit;
		}
		load_inf("bootpkg init malloc_2: 0x%08x\n", len);
		ret = spl_flash_read(bootpkg_start_sector , len / LOAD_SECTOR_SIZE, bootpkg_buf);
		if (ret) {
			load_err("bootpkg init flash read fail_2\n");
			hal_free(bootpkg_buf);
			bootpkg_buf = NULL;
			goto exit;
		}
	}
exit:
	return ret;
}

int bootpkg_deinit(void)
{
	if (bootpkg_buf) {
		hal_free(bootpkg_buf);
		bootpkg_buf = NULL;
	}
	return 0;
}

int get_bootpkg_by_name(char *name, uint32_t *addr, uint32_t *size)
{
	uint32_t i;
	struct sbrom_toc1_head_info  *toc1_head = NULL;
	struct sbrom_toc1_item_info  *item_head = NULL;
	struct sbrom_toc1_item_info  *toc1_item = NULL;

	if (bootpkg_buf == NULL) {
		return -1;
	}

	toc1_head = (struct sbrom_toc1_head_info *)bootpkg_buf;
	item_head = (struct sbrom_toc1_item_info *)(bootpkg_buf + sizeof(struct sbrom_toc1_head_info));

	toc1_item = item_head;
	for(i = 0; i < toc1_head->items_nr; i++,toc1_item++) {
		if (toc1_item->type != 3) {
			continue;
		}
		if (strncmp(toc1_item->name, name, strlen(toc1_item->name)) == 0) {
			*addr = bootpkg_start_sector * LOAD_SECTOR_SIZE + toc1_item->data_offset;
			*size = toc1_item->data_len;
			load_inf("find bootpkt success: %s\n", name);
			load_inf("data_offset: 0x%08x\n", (uint32_t)toc1_item->data_offset);
			load_inf("data_len: 0x%08x\n", (uint32_t)toc1_item->data_len);
			return 0;
		}
	}
	load_err("find bootpkt fail: %s\n", name);
	return -1;
}

int load_bin(bin_info *info)
{
	int ret = 0;

	if (!info->size) {
		load_err("bin parse size is 0\n");
		ret = -1;
		goto exit;
	}

	ret = spl_flash_read(info->src_addr / LOAD_SECTOR_SIZE,
			                 (info->size + (LOAD_SECTOR_SIZE - 1)) / LOAD_SECTOR_SIZE,
			                 (void *)info->dst_addr);
	if (ret) {
		load_err("bin load fail sector: 0x%08x\n", info->src_addr / LOAD_SECTOR_SIZE);
	}
exit:
	return ret;
}

int parse_rtos_fw(uint32_t sector, uint32_t sector_num, elf_info *elf, rproc_info *rproc)
{
	int ret = 0;
	uint32_t i;
	uint32_t rsvmem_addr = CONFIG_RTOS_FW_RESERVED_MEM_ADDR;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr_entry;
	Elf32_Shdr *shdr_entry;
	uint32_t load_sector;
	uint32_t load_num;
	void *load_buff;
	char *p_rsctab;

	load_sector = sector;
	load_num = 1;
	load_buff = (void *)START_ADDRESS_OF_SECTOR(rsvmem_addr);
	ret = spl_flash_read(load_sector, load_num, load_buff);
	if (ret) {
		load_err("ehdr read fail\n");
		ret = -1;
		goto exit;
	}
	ehdr = (Elf32_Ehdr *)load_buff;
	load_sector = sector + START_SECTOR(ehdr->e_phoff);
	load_num = LOAD_SECTOR_NUM(ehdr->e_phoff, ehdr->e_phentsize * ehdr->e_phnum);
	load_buff = (void *)START_ADDRESS_OF_SECTOR(rsvmem_addr + ehdr->e_phoff);
	ret = spl_flash_read(load_sector, load_num, load_buff);
	if (ret) {
		load_err("phdr read fail\n");
		ret = -1;
		goto exit;
	}

	load_sector = sector + START_SECTOR(ehdr->e_shoff);
	load_num = LOAD_SECTOR_NUM(ehdr->e_shoff, ehdr->e_shentsize * ehdr->e_shnum);
	load_buff = (void *)START_ADDRESS_OF_SECTOR(rsvmem_addr + ehdr->e_shoff);
	ret = spl_flash_read(load_sector, load_num, load_buff);
	if (ret) {
		load_err("shdr read fail\n");
		ret = -1;
		goto exit;
	}

	shdr_entry =  (Elf32_Shdr *)(rsvmem_addr + ehdr->e_shoff + ehdr->e_shstrndx * ehdr->e_shentsize);
	load_sector = sector + START_SECTOR(shdr_entry->sh_offset);
	load_num = LOAD_SECTOR_NUM(shdr_entry->sh_offset, shdr_entry->sh_size);
	load_buff = (void *)START_ADDRESS_OF_SECTOR(rsvmem_addr + shdr_entry->sh_offset);
	ret = spl_flash_read(load_sector, load_num, load_buff);
	if (ret) {
		load_err("strtab read fail\n");
		ret = -1;
		goto exit;
	}

	/* pheader handle */
	if (ehdr->e_phnum >= RTOS_SECTION_MAX_NUM) {
		load_err("elf over section max num: %u\n", ehdr->e_phnum);
		ret = -1;
		goto exit;
	}
	elf->section_num = ehdr->e_phnum;
	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr_entry =  (Elf32_Phdr *)(rsvmem_addr + ehdr->e_phoff + i * ehdr->e_phentsize);
		elf->section[i].dst_addr = phdr_entry->p_paddr;
		elf->section[i].src_addr = sector * LOAD_SECTOR_SIZE + phdr_entry->p_offset;
		elf->section[i].file_size = phdr_entry->p_filesz;
		elf->section[i].mem_size = phdr_entry->p_memsz;
	}
	/* sheader handle */
	shdr_entry =  (Elf32_Shdr *)(rsvmem_addr + ehdr->e_shoff + ehdr->e_shstrndx * ehdr->e_shentsize);
	if (shdr_entry->sh_type == 3) {
		rproc->strtab_offset = (uint32_t)shdr_entry->sh_offset;
		rproc->strtab_size = shdr_entry->sh_size;
	} else {
		load_err("can not find string table\n");
		ret = -1;
		goto exit;
	}
	/* resource table handle */
	p_rsctab = (char *)rsvmem_addr + shdr_entry->sh_offset;
	for (i = 0; i < ehdr->e_shnum; i++) {
		shdr_entry = (Elf32_Shdr *)(rsvmem_addr + ehdr->e_shoff + i * ehdr->e_shentsize);
		if (!strcmp(&p_rsctab[shdr_entry->sh_name], ".resource_table")) {
			rproc->runmem_rsctab_offset = shdr_entry->sh_addr - CONFIG_ARCH_START_ADDRESS;
			rproc->rsctab_offset = shdr_entry->sh_offset;
			rproc->rsctab_size = shdr_entry->sh_size;
			break;
		}
	}
	if (i == ehdr->e_shnum) {
		load_err("can not find resource table in rtos fw\n");
		ret = -1;
		goto exit;
	}
	/* other handle */
	rproc->flash_rtos_addr = sector * LOAD_SECTOR_SIZE;
	rproc->runmem_rtos_addr = CONFIG_ARCH_START_ADDRESS;
	rproc->rsvmem_rtos_addr = rsvmem_addr;
	rproc->ehdr_offset = 0;
	rproc->ehdr_size = ehdr->e_ehsize;
	rproc->shdr_offset = ehdr->e_shoff;
	rproc->shdr_size = ehdr->e_shentsize * ehdr->e_shnum;
exit:
	return ret;
}

int load_rtos_fw(rproc_info *rproc)
{
	uint32_t sector;
	uint32_t sector_num;
	uint8_t *buff;

	/* load rtos eheader */
	sector = START_SECTOR(rproc->flash_rtos_addr + rproc->ehdr_offset);
	sector_num = LOAD_SECTOR_NUM(rproc->ehdr_offset, rproc->ehdr_size);
	buff = (uint8_t *)START_ADDRESS_OF_SECTOR(rproc->rsvmem_rtos_addr + rproc->ehdr_offset);
	spl_flash_read(sector, sector_num, buff);

	/* load rtos sheader */
	sector = START_SECTOR(rproc->flash_rtos_addr + rproc->shdr_offset);
	sector_num = LOAD_SECTOR_NUM(rproc->shdr_offset, rproc->shdr_size);
	buff = (uint8_t *)START_ADDRESS_OF_SECTOR(rproc->rsvmem_rtos_addr + rproc->shdr_offset);
	spl_flash_read(sector, sector_num, buff);

	/* load rtos strtab */
	sector = START_SECTOR(rproc->flash_rtos_addr + rproc->strtab_offset);
	sector_num = LOAD_SECTOR_NUM(rproc->strtab_offset, rproc->strtab_size);
	buff = (uint8_t *)START_ADDRESS_OF_SECTOR(rproc->rsvmem_rtos_addr + rproc->strtab_offset);
	spl_flash_read(sector, sector_num, buff);

	/* load rtos rsctab */
	sector = START_SECTOR(rproc->flash_rtos_addr + rproc->rsctab_offset);
	sector_num = LOAD_SECTOR_NUM(rproc->rsctab_offset, rproc->rsctab_size);
	buff = (uint8_t *)START_ADDRESS_OF_SECTOR(rproc->rsvmem_rtos_addr + rproc->rsctab_offset);
	spl_flash_read(sector, sector_num, buff);
	return 0;
}

int parse_standby_pmboot_info(uint32_t addr, uint32_t size, bin_info *info)
{
	int ret = 0;
	uint8_t *boot0_buf;
	boot_file_head_t *boot0_head;

	boot0_buf = (uint8_t *)hal_malloc(LOAD_SECTOR_SIZE);
	if (boot0_buf == NULL) {
		load_err("standby_b oot0 parse malloc fail\n");
		ret = -1;
		goto exit;
	}

	ret = spl_flash_read(addr / LOAD_SECTOR_SIZE, 1, boot0_buf);
	if (ret) {
		load_err("standby_boot0 parse flash read fail\n");
		goto error;
	}
	boot0_head = (boot_file_head_t *)boot0_buf;
	info->src_addr = addr;
	info->dst_addr = boot0_head->run_addr;
	info->size = boot0_head->length;
error:
	hal_free(boot0_buf);
exit:
	return ret;
}

void reload_dram_size(uint32_t start_addr, uint32_t dram_size)
{
	*((uint32_t *)(start_addr + DRAM_SIZE_OFFSET)) = dram_size;
}

#if (defined(LOAD_IMAGE_DEBUG))
int cmd_flash_dump(int argc, char **argv)
{
	char *err = NULL;
	uint32_t sector, sector_num;
	uint8_t *buf;
	uint32_t i;

	if (argc < 3) {
		printf("arg erro\n");
		return -1;
	}

	sector = strtoul(argv[1], &err, 0);
	sector_num = strtoul(argv[2], &err, 0);

	buf = (uint8_t *)hal_malloc(LOAD_SECTOR_SIZE * sector_num);
	if (buf == NULL) {
		printf("malloc erro\n");
		return -1;
	}
	spl_flash_init();
	spl_flash_read(sector, sector_num, buf);
	printf("dump\n");
	for (i = 0; i < sector_num *512; i++) {
		if (i != 0 && i % 16 == 0) {
			printf("\n");
		}
		if (i % 512 == 0) {
			printf("sector[%d]\n", i / 512);
		}
		printf("%02x ", buf[i]);
	}
	printf("\n");

	spl_flash_deinit();
	hal_free(buf);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_flash_dump, flash_dump, flash_dump);

int cmd_find_bootpkg(int argc, char **argv)
{
	int ret = 0;
	char *name;
	uint32_t addr, size;

	if (argc < 2) {
		printf("arg erro\n");
		return -1;
	}

	name = argv[1];
	ret = spl_flash_init();
	if (ret) {
		printf("flash init fail\n");
		goto flash_fail;
	}
	ret = bootpkg_init();
	if (ret) {
		printf("bootpkg init fail\n");
		goto bootpkg_fail;
	}
	ret = get_bootpkg_by_name(name, &addr, &size);
	if (ret) {
		printf("bootpkg find fail: %s\n", name);
		goto find_fail;
	}
	printf("bootpkg find success: %s\n", name);
	printf("addr: 0x%08x size: 0x%08x\n", addr, size);
find_fail:
	bootpkg_deinit();
bootpkg_fail:
	spl_flash_deinit();
flash_fail:
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_find_bootpkg, find_bootpkg, find_bootpkg);

int cmd_find_gpt(int argc, char **argv)
{
	int ret = 0;
	char *name;
	uint32_t sector, sector_num;

	if (argc < 2) {
		printf("arg erro\n");
		return -1;
	}

	name = argv[1];

	ret = spl_flash_init();
	if (ret) {
		printf("flash init fail\n");
		goto flash_fail;
	}
	ret = gpt_init();
	if (ret) {
		printf("gpt init fail\n");
		goto bootpkg_fail;
	}
	ret = get_partition_by_name(name, &sector, &sector_num);
	if (ret) {
		printf("gpt find fail: %s\n", name);
		goto find_fail;
	}
	printf("gpt find success: %s\n", name);
	printf("sector: 0x%08x sector_num: 0x%08x\n", sector, sector_num);
	find_fail:
	gpt_deinit();
	bootpkg_fail:
	spl_flash_deinit();
	flash_fail:
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_find_gpt, find_gpt, find_gpt);

#endif

