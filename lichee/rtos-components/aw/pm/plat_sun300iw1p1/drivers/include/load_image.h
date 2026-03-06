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

#ifndef _LOAD_IMAGE_H_
#define _LOAD_IMAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define LOAD_INF_EN      (0)
#define LOAD_LOG_EN      (0)
#define LOAD_ERR_EN      (1)
#if LOAD_INF_EN
#define load_inf(fmt,...)   printf("[LOAD_INF]" fmt,##__VA_ARGS__)
#else
#define load_inf(fmt,...)
#endif
#if LOAD_LOG_EN
#define load_log(fmt,...)   printf("[LOAD_LOG]" fmt,##__VA_ARGS__)
#else
#define load_log(fmt,...)
#endif
#if LOAD_ERR_EN
#define load_err(fmt,...)   printf("[LOAD_ERR]" fmt,##__VA_ARGS__)
#else
#define load_err(fmt,...)
#endif

/* bin info struct */
typedef struct {
	uint32_t src_addr;
	uint32_t dst_addr;
	uint32_t size;
}bin_info;

/* elf info struct */
typedef struct {
	uint32_t src_addr;
	uint32_t dst_addr;
	uint32_t file_size;
	uint32_t mem_size;
}section_info;

#define LOAD_SECTOR_SIZE        (512)
#define RTOS_SECTION_MAX_NUM    (2)
typedef struct {
	section_info section[RTOS_SECTION_MAX_NUM];
	uint32_t section_num;
}elf_info;

typedef struct {
	uint32_t flash_rtos_addr;      /* rtos.elf address in flash */
	uint32_t rsvmem_rtos_addr;     /* rtos reserve memory address */
	uint32_t runmem_rtos_addr;     /* rtos running memory address */
	uint32_t ehdr_offset;          /* elf header offset */
	uint32_t ehdr_size;            /* elf header size */
	uint32_t shdr_offset;          /* section header offset */
	uint32_t shdr_size;            /* section header size */
	uint32_t strtab_offset;        /* string table section offset */
	uint32_t strtab_size;          /* string table section size */
	uint32_t rsctab_offset;        /* resource table section offset */
	uint32_t rsctab_size;          /* resource table section size */
	uint32_t runmem_rsctab_offset; /* resource table offset base running memory */
}rproc_info;

int parse_rtos_fw(uint32_t sector, uint32_t sector_num, elf_info *elf, rproc_info *rproc);
int load_rtos_fw(rproc_info *rproc);
int parse_resource_table(uint32_t elf_addr, bin_info *info);
void bootpkg_set_start_sector(uint32_t sector);
uint32_t bootpkg_get_start_sector(void);
int bootpkg_init(void);
int bootpkg_deinit(void);
int get_bootpkg_by_name(char *name, uint32_t *addr, uint32_t *size);
int parse_standby_pmboot_info(uint32_t addr, uint32_t size, bin_info *info);
int load_bin(bin_info *info);
void reload_dram_size(uint32_t start_addr, uint32_t dram_size);

#ifdef __cplusplus
}
#endif
#endif /* _LOAD_IMAGE_H_ */

