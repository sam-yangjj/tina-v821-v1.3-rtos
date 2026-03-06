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

#if defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINOR)
#include <spinor.h>
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINAND)
#include <spinand.h>
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_EMMC)
#include <mmc_bsp.h>
#endif

#if defined(CONFIG_COMPONENTS_AW_FLASH_READ_EMMC)
static inline int get_card_num(void)
{
	return 0;
}
#endif

int spl_flash_config(void *pirv)
{
	int ret = 0;

	if (pirv == NULL) {
		return -1;
	}
#if defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINOR)
	ret = spinor_set_boot_param_start(*(uint32_t *)pirv);
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINAND)
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_EMMC)
#endif
	return ret;
}

int spl_flash_init(void)
{
	int ret = -1;
#if defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINOR)
	ret = spinor_init(0);
	if (ret) {
		return ret;
	}
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINAND)
	ret = spinand_init();
	if (ret) {
		return ret;
	}
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_EMMC)
	ret = sunxi_mmc_init(get_card_num(), 4);
	if (ret == -1) {
		return ret;
	}
#endif
	return ret;
}

int spl_flash_deinit(void)
{
#if defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINOR)
	spinor_exit(0);
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINAND)
	spinand_deinit();
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_EMMC)
	sunxi_mmc_deinit(get_card_num());
#endif
	return 0;
}

int spl_flash_read(uint32_t sector, uint32_t sector_num, void *buff)
{
#if defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINOR)
	if (spinor_read(sector, sector_num, buff) != 0) {
		return -1;
	}
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINAND)
	uint32_t i;
	uint32_t cnt = 0;
	uint32_t start = sector;
	uint32_t num = sector_num;

	while (num) {
			start += cnt * LOAD_SECTOR_SIZE;
			if (spinand_read(start , buff, 1) != NAND_OP_TRUE) {
				return -1;
			}
			cnt++;
			num--;
	}
#elif defined(CONFIG_COMPONENTS_AW_FLASH_READ_EMMC)
	if (mmc_bread(get_card_num(), sector , sector_num , buff)  == 0) {
		return -1;
	}
#endif
	return 0;
}

#ifdef CONFIG_COMPONENTS_AW_FLASH_WRITE
int spl_flash_erase(uint32_t sector, uint32_t sector_num)
{
#if defined(CONFIG_COMPONENTS_AW_FLASH_READ_SPINOR)
	if (spinor_erase(sector, sector_num) != 0)
		return -1;
	return 0;
#endif
	printf("unsupport spl_flash_erase function\n");
	return -1;
}

int spl_flash_write(uint32_t sector, uint32_t sector_num, void *buff)
{
#if defined(CONFIG_PM_LOAD_STORAGE_TYPE_SPINOR)
	if (spinor_write(sector, sector_num, buff) != 0) {
		return -1;
	}
	return 0;
#endif
	printf("unsupport spl_flash_write function\n");
	return -1;
}
#endif
