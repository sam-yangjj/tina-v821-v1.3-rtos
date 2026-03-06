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

#include <stdio.h>
#include <string.h>
#include <hal_mem.h>
#include <console.h>
#include <hal_atomic.h>
#include <sunxi_hal_common.h>

#ifndef LOAD_SECTOR_SIZE
#define LOAD_SECTOR_SIZE (512)
#endif

#include "spl_flash.c"

#if defined(CONFIG_WAIT_SPIF_CONTROLLER)

#define RTC_DATA_REGS(n)			(0x4a000000 + 0x200 + (n) * 4)
#define FLASH_FLAGS_INDEX			(CONFIG_SPIF_WAIT_INDEX)
#define FLASH_FLAGS_BIT				(CONFIG_SPIF_WAIT_BIT)

static void flash_read_timeout_wait()
{
	int i, timeout_cnt = 5000;
	if (!(hal_readl(RTC_DATA_REGS(FLASH_FLAGS_INDEX)) & (1 << FLASH_FLAGS_BIT))) {
		/* waiting for controller release */
		for (i = 0; i < timeout_cnt; i++) {
			/* wait for boot0 sign out*/
			if (hal_readl(RTC_DATA_REGS(FLASH_FLAGS_INDEX)) & (1 << FLASH_FLAGS_BIT))
				break;
			hal_msleep(1);
		}
		printf("wait flash : %d ms\n", i);
	}
}

#endif /* CONFIG_WAIT_SPIF_CONTROLLER */


static int flash_inited = 0;

int flash_init(void)
{
	int ret;
	unsigned long flags;

#if defined(CONFIG_WAIT_SPIF_CONTROLLER)
        flash_read_timeout_wait();
#endif /* CONFIG_WAIT_SPIF_CONTROLLER */

	flags = hal_enter_critical();
	ret = flash_inited;
	flash_inited++;
	hal_exit_critical(flags);

	if (!ret) {
		ret = spl_flash_init();
		if (ret)
			return ret;
		return gpt_init();
		if (ret)
			return ret;
	}

	ret = 0;
	return ret;
}

int flash_deinit(void)
{
	int ret;

	if (--flash_inited)
		return 0;

	ret = gpt_deinit();
	if (ret)
		return ret;

	return spl_flash_deinit();
}

int flash_read(uint32_t sector, uint32_t sector_num, void *buff)
{
	return spl_flash_read(sector, sector_num, buff);
}

#ifdef CONFIG_COMPONENTS_AW_FLASH_WRITE
int flash_erase(uint32_t sector, uint32_t sector_num)
{
	return spl_flash_erase(sector, sector_num);
}

int flash_write(uint32_t sector, uint32_t sector_num, void *buff)
{
	return spl_flash_write(sector, sector_num, buff);
}
#endif /* CONFIG_COMPONENTS_AW_FLASH_WRITE */

#ifdef CONFIG_COMPONENTS_AW_FLASH_DEBUG
static void _hexdump(const char *name, uint32_t base, int len, void *p)
{
	int i = 0, ofs = 0, loop;
	char buf[64];

	if (name)
		printf("%s: \n", name);

	while (len > 0) {
		ofs = snprintf(buf, sizeof(buf), "0x%08x:", base + i);
		if (len >= 16)
			loop = 4;
		else if (len >= 12)
			loop = 3;
		else if (len >= 8)
			loop = 2;
		else
			loop = 1;

		while (loop--) {
			ofs += snprintf(buf + ofs, sizeof(buf) - ofs, " %08x", *(uint32_t *)(p + i));
			i += 4;
			len -= 4;
		}
		printf("%s\n", buf);
	}
}

static int cmd_flash_read(int argc, char **argv)
{
	int ret;
	uint32_t sector, sector_num;
	char *partition_name;
	uint8_t buf[LOAD_SECTOR_SIZE];

	if (argc != 4) {
		printf("Useage: flas_read partition_name sector_start sector_num\n");
		return 0;
	}

	partition_name = argv[1];

	flash_init();
	ret = get_partition_by_name(partition_name, &sector, &sector_num);
	if (ret) {
		printf("get rtos partition fail\n");
		return -1;
	}
	printf("%s sector: %u num: %u\n", partition_name, sector, sector_num);

	sector += atoi(argv[2]);
	sector_num = atoi(argv[3]);

	printf("Read: %s Sector: %d + %d\n", partition_name, sector, sector_num);

	while (sector_num--) {
		if (flash_read(sector, 1, buf)) {
			printf("Read Sector: %d failed\n", sector);
			break;
		}
		_hexdump(NULL, sector * LOAD_SECTOR_SIZE, LOAD_SECTOR_SIZE, buf);
		sector++;
	}

	flash_deinit();
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_flash_read, fr, Tina flash read test);
#ifdef CONFIG_COMPONENTS_AW_FLASH_WRITE
static int cmd_flash_write(int argc, char **argv)
{
	int ret;
	uint32_t sector, sector_num;
	char *partition_name;
	uint8_t buf[LOAD_SECTOR_SIZE];
	unsigned long data;
	int bytes;

	if (argc != 5) {
		printf("Useage: fw partition_name sector_start data bytes\n");
		return 0;
	}

	partition_name = argv[1];

	flash_init();
	ret = get_partition_by_name(partition_name, &sector, &sector_num);
	if (ret) {
		printf("get rtos partition fail\n");
		return -1;
	}
	printf("%s sector: %u num: %u\n", partition_name, sector, sector_num);

	sector += atoi(argv[2]);
	data = strtoul(argv[3], NULL, 0);
	bytes = strtoul(argv[4], NULL, 0);

	printf("Write: %s Sector: %d data: 0x%x + %d\n", partition_name, sector, (uint8_t)data, bytes);

	while (bytes) {
		if (flash_read(sector, 1, buf)) {
			printf("Read Sector: %d failed\n", sector);
			break;
		}
		memset(buf, (uint8_t)data, bytes > LOAD_SECTOR_SIZE ? LOAD_SECTOR_SIZE : bytes);
		flash_write(sector, 1, buf);
		bytes -= bytes > LOAD_SECTOR_SIZE ? LOAD_SECTOR_SIZE : bytes;
		sector++;
	}

	flash_deinit();
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_flash_write, fw, Tina flash write test);
#endif /* CONFIG_COMPONENTS_AW_FLASH_WRITE */
#endif
