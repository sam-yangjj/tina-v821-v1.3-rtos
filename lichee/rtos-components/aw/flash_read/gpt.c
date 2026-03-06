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
#include <hal_mem.h>
#include <hal_time.h>
#include <flash_read.h>

/* gpt partition data struct */
typedef struct {
    uint8_t b[16];
} efi_guid_t;

typedef struct _gpt_header {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved1;
    uint64_t my_lba;
    uint64_t alternate_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    efi_guid_t disk_guid;
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries;
    uint32_t sizeof_partition_entry;
    uint32_t partition_entry_array_crc32;
} __packed gpt_header;

typedef union _gpt_entry_attributes {
    struct {
        uint64_t required_to_function:1;
        uint64_t no_block_io_protocol:1;
        uint64_t legacy_bios_bootable:1;
        /* u64 reserved:45; */
        uint64_t reserved:27;
        uint64_t user_type:16;
        uint64_t ro:1;
        uint64_t keydata:1;
        uint64_t type_guid_specific:16;
    } fields;
    unsigned long long raw;
} __packed gpt_entry_attributes;

typedef uint16_t efi_char16_t;

#define PARTNAME_SZ   (72 / sizeof(efi_char16_t))
typedef struct _gpt_entry {
    efi_guid_t partition_type_guid;
    efi_guid_t unique_partition_guid;
    uint64_t starting_lba;
    uint64_t ending_lba;
    gpt_entry_attributes attributes;
    efi_char16_t partition_name[PARTNAME_SZ];
} __packed gpt_entry;

#define GPT_HEADER_SIGNATURE    (0x5452415020494645ULL)
#define GPT_SECTOR_NUM          (8U)
/*
#ifdef CONFIG_COMPONENTS_AW_FLASH_GPT_OFFSET
#define GPT_START_SECTOR        (CONFIG_COMPONENTS_AW_FLASH_GPT_OFFSET)
#else
#error "should not hardcode gpt start sector"
#endif
*/

#ifndef LOAD_SECTOR_SIZE
#define LOAD_SECTOR_SIZE        (512)
#endif
#define GPT_HEAD_OFFSET         (512U)
#define GPT_ENTRY_OFFSET        (1024U)

static uint8_t *gpt_buf = NULL;
static uint32_t gpt_start_sector = CONFIG_COMPONENTS_AW_FLASH_GPT_OFFSET;

void gpt_set_start_sector(uint32_t sector)
{
	gpt_inf("Sec GPT Sector Start: %d\n", sector);
	gpt_start_sector = sector;
}

int gpt_init(void)
{
	int ret = 0;

	gpt_buf = (uint8_t *)hal_malloc_align(GPT_SECTOR_NUM * LOAD_SECTOR_SIZE, 64);
	if (gpt_buf == NULL) {
		gpt_err("gpt malloc fail\n");
		return -1;
	}

	gpt_inf("GPT Sector Offset: %d\n", gpt_start_sector);
	ret = spl_flash_read(gpt_start_sector, GPT_SECTOR_NUM, gpt_buf);
	if (ret) {
		gpt_err("gpt get info fail\n");
		hal_free_align(gpt_buf);
		gpt_buf = NULL;
		return -1;
	}
	return 0;
}

int gpt_deinit(void)
{
	if (gpt_buf) {
		hal_free_align(gpt_buf);
		gpt_buf = NULL;
	}
	return 0;
}

int get_partition_by_name(char *name, uint32_t *sector, uint32_t *sector_num)
{
	char   char8_name[PARTNAME_SZ] = {0};
	gpt_header *head;
	gpt_entry *entry;
	uint32_t i, j;

	if (gpt_buf == NULL) {
		gpt_err("gpt has not init\n");
		return -1;
	}

	head = (gpt_header *)(gpt_buf + GPT_HEAD_OFFSET);
	entry = (gpt_entry *)(gpt_buf + GPT_ENTRY_OFFSET);

	if (head->signature != GPT_HEADER_SIGNATURE) {
		gpt_inf("gpt magic error, %llx != %llx\n", head->signature, GPT_HEADER_SIGNATURE);
		return -1;
	}

	for (i = 0; i < head->num_partition_entries; i++) {
		for (j = 0; j < PARTNAME_SZ; j++) {
			char8_name[j] = (char)(entry[i].partition_name[j]);
		}
		if (!strcmp(name, char8_name)) {
			*sector = (uint32_t)entry[i].starting_lba;
#ifndef CONFIG_PM_LOAD_STORAGE_TYPE_MMC
			*sector += gpt_start_sector;
#endif
			*sector_num = (uint32_t)(entry[i].ending_lba + 1 - entry[i].starting_lba);
			gpt_inf("gpt get partition success: %s\n", name);
			gpt_inf("sector: %u sector_num: %u\n", *sector, *sector_num);
			return 0;
		}
	}
	gpt_err("gpt get partition fail name: %s\n", name);
	return -1;
}

