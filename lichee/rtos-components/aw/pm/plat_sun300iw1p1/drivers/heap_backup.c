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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <heap_backup.h>
#ifndef STANDBY_FIRMWARE
#include <hal_cmd.h>
#endif

#ifdef STANDBY_FIRMWARE
#define printf(fmt,...)
#endif

//#define HEAP_BACKUP_DEBUG
#define get_real_size(size)   (size & 0x7FFFFFFF)

typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
} BlockLink_t;

#ifndef STANDBY_FIRMWARE
extern BlockLink_t *pxStart;
extern BlockLink_t *pxEnd;

void *heap_get_start(void)
{
	return pxStart;
}

void *heap_get_end(void)
{
	return pxEnd;
}
#endif

int heap_backup(struct heap_backup_info *info, uint32_t heap_start, uint32_t heap_end, uint32_t backup_start, uint32_t backup_end)
{
	int ret= 0;
	uint32_t size = 0;
	void *dst = (void *)backup_start;
	BlockLink_t *block = (BlockLink_t *)heap_start;

	if (info == NULL) {
		return -1;
	}
	memset(info, 0, sizeof(*info));
	while (block < (BlockLink_t *)heap_end) {
		if (block->pxNextFreeBlock == NULL) {
			/* block is used, backup whole block */
			size = get_real_size(block->xBlockSize);
			info->backup_size += get_real_size(block->xBlockSize);
			info->payload_num++;
		} else {
			/* block is free, backup block header and abandon payload*/
			size = sizeof(*block);
			info->backup_size += sizeof(*block);
		}
		info->header_num ++;
		if ((uint32_t)dst + size <= backup_end) {
			/* backup heap size is larger than mem size */
			memcpy((void *)dst, (void *)block, size);
			dst = (void *)((uint32_t)dst + size);
		} else {
			ret = -1;
		}
		block = (BlockLink_t *)((uint32_t)block + get_real_size(block->xBlockSize));
	}
	return ret;
}

void heap_restore(uint32_t heap_start, uint32_t heap_end, uint32_t backup_start, uint32_t backup_end)
{
	uint32_t size = 0;
	void *dst = (void *)heap_start;
	BlockLink_t *block = (BlockLink_t *)backup_start;

	while (block < (BlockLink_t *)backup_end) {
		if (block->pxNextFreeBlock == NULL) {
			/* block is used, restore whole block */
			size = get_real_size(block->xBlockSize);
		} else {
			/* block is free, restore block header and reserved payload */
			size = sizeof(BlockLink_t);
		}

		memcpy((void *)dst, (void *)block, size);
		dst = (void *)((uint32_t)dst + get_real_size(block->xBlockSize));
		block = (BlockLink_t *)((uint32_t)block + size);
	}
	/* restore heap end initial value */
	block = (BlockLink_t *)heap_end;
	block->pxNextFreeBlock = NULL;
	block->xBlockSize = 0;
}

#ifndef STANDBY_FIRMWARE
#ifdef HEAP_BACKUP_DEBUG

static uint8_t copy_heap[0x15200];
static uint8_t backup_ram[0x15200];
static uint8_t restore_heap[0x15200];

static int cmd_heap_test_backup(int argc, char **argv)
{
	uint32_t backup_heap_size;

	memcpy(copy_heap, (uint8_t *)heap_get_start(), (uint32_t)heap_get_end() - (uint32_t)heap_get_start());
	backup_heap_size = heap_backup((uint32_t)copy_heap, (uint32_t)(copy_heap + ((uint32_t)heap_get_end() - (uint32_t)heap_get_start())), (uint32_t)backup_ram, (uint32_t)(backup_ram + sizeof(backup_ram)));
	heap_restore((uint32_t)restore_heap , (uint32_t)(restore_heap + ((uint32_t)heap_get_end() - (uint32_t)heap_get_start())), (uint32_t)backup_ram, (uint32_t)(backup_ram + backup_heap_size));


	printf("source heap size: %u\n", (uint32_t)heap_get_end() - (uint32_t)heap_get_start());
	printf("backup ram size: %u\n", backup_heap_size);
	printf("copy_heap start: 0x%08x end: 0x%08x\n", (uint32_t)copy_heap, (uint32_t)(copy_heap + ((uint32_t)heap_get_end() - (uint32_t)heap_get_start())));
	printf("restore_heap start: 0x%08x end: 0x%08x\n", (uint32_t)restore_heap, (uint32_t)(restore_heap + ((uint32_t)heap_get_end() - (uint32_t)heap_get_start())));

	printf("heap_show 0 0x%08x 0x%08x\n", (uint32_t)copy_heap, (uint32_t)(copy_heap + ((uint32_t)heap_get_end() - (uint32_t)heap_get_start())));
	printf("heap_show 0 0x%08x 0x%08x\n", (uint32_t)restore_heap, (uint32_t)(restore_heap + ((uint32_t)heap_get_end() - (uint32_t)heap_get_start())));
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_heap_test_backup, heap_test_backup, heap_test_backup);

static int cmd_heap_show(int argc, char **argv)
{
	uint32_t i;
	uint8_t *ptr;

	uint32_t start_addr = 0;
	uint32_t end_addr = 0;
	uint32_t printf_flag = 0;
	uint32_t used_size = 0;
	uint32_t free_size = 0;
	uint32_t cnt = 0;
	BlockLink_t *block;

	if (argc != 4) {
		printf("param err\n");
		return -1;
	}
	printf_flag = strtoul(argv[1], NULL, 0);
	start_addr = strtoul(argv[2], NULL, 0);
	end_addr = strtoul(argv[3], NULL, 0);
	printf("printf_flag: %u\n", printf_flag);
	printf("start_addr: 0x%08x\n", start_addr);
	printf("end_addr: 0x%08x\n", end_addr);
	block = (BlockLink_t *)start_addr;

	while (block < (BlockLink_t *)end_addr) {
		if ((printf_flag == 0) ||
			(printf_flag == 1 && block->pxNextFreeBlock == NULL) ||
			(printf_flag == 2 && block->pxNextFreeBlock)) {
			printf("entry[%u] next: 0x%08x size: %u\n", \
				   cnt++, (uint32_t)block->pxNextFreeBlock, get_real_size(block->xBlockSize));

			ptr = (uint8_t *)block;
			for (i = 0; i < get_real_size(block->xBlockSize); i++) {
				if (i != 0 && i % 32 ==0) {
					printf("\n");
				}
				printf("%02x ", ptr[i]);
			}
			printf("\n");
		}
		if (block->pxNextFreeBlock == NULL) {
			used_size += get_real_size(block->xBlockSize);
		} else {
			free_size += get_real_size(block->xBlockSize);
		}
		block = (BlockLink_t *)((uint8_t *)block + get_real_size(block->xBlockSize));
	}
	printf("pxend\n");
	ptr = (uint8_t *)end_addr;
	for (i = 0; i < 16; i++) {
		printf("%02x ", ptr[i]);
	}
	printf("\n");

	printf("used size: %u\n", used_size);
	printf("free size: %u\n", free_size);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_heap_show, heap_show, heap_show);
#endif
#endif

