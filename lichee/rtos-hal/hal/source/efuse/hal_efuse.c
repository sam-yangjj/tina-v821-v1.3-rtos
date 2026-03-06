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
#include <stdio.h>
#include <string.h>

#include <sunxi_hal_efuse.h>
#include <sunxi_hal_common.h>
#include <hal_mem.h>
#include "platform_efuse.h"

uint32_t efuse_dram_read_key(uint key_index)
{
	uint32_t *dram_data = (uint32_t *)EFUSE_DRAM;
	return dram_data[key_index / 4];
}

int hal_efuse_read_ext(uint32_t start_bit, uint32_t bit_num, uint8_t *data)
{
	printf("efuse base dram addr:0x%x\n", EFUSE_DRAM);

	if ((data == NULL) ||
		(start_bit >= EFUSE_BIT_NUM) ||
		(bit_num == 0) ||
		(bit_num > EFUSE_BIT_NUM) ||
		(start_bit + bit_num > EFUSE_BIT_NUM)) {
		printf("arg error: start bit %u, bit num %u, data 0x%p\n", start_bit, bit_num, data);
		return EFUSE_ERR_ARG;
	}

	uint32_t start_word = start_bit / 32;
	uint8_t word_shift = start_bit % 32;
	uint8_t word_cnt = (start_bit + bit_num) / 32 - start_word;
	if ((start_bit + bit_num) % 32)
		word_cnt ++;

	uint32_t *sid_data = hal_malloc(word_cnt * 4);
	if (!sid_data) {
		printf("efuse malloc %d bytes error\n", word_cnt * 4);
		return EFUSE_ERR_ARG;
	}
	int i;
	for (i = 0; i < word_cnt; i++) {
		sid_data[i] = efuse_dram_read_key((start_word + i) * 4);
	}
	if (word_shift) {
		for (i = 0; i < word_cnt - 1; i++) {
			sid_data[i] = (sid_data[i] >> word_shift) | (sid_data[i + 1] << (32 - word_shift));
		}
		sid_data[i] = (sid_data[i] >> word_shift);
	}

	((uint8_t*)sid_data)[(bit_num - 1) / 8] &= ((1 << (bit_num % 8 == 0 ? 8 : bit_num % 8)) - 1);
	memcpy(data, (uint8_t*)sid_data, (bit_num + 7) / 8);

	hal_free(sid_data);

	return 0;
}

int hal_efuse_get_chipid(unsigned char *buffer)
{
	return hal_efuse_read_ext(CHIPID_OFFSET, CHIPID_BIT_LEN, buffer);
}

#if CONFIG_ARCH_SUN300IW1
int hal_efuse_get_pmc_enable(unsigned char *buffer)
{
	return hal_efuse_read_ext(PMC_ENABLE_OFFSET, PMC_ENABLE_BIT_LEN, buffer);
}
#endif
