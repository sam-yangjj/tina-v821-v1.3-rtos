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

#include <string.h>
#include <sunxi_hal_common.h>

#include "platform/platform.h"

#include <e90x_sysmap.h>

static uint32_t g_region_index = 0;

#define SYSMAP_REGION_NUM 8

#define SYSMAP_ADDR_SHIFT 12
#define SYSMAP_ADDR_ALIGN_SIZE (1 << SYSMAP_ADDR_SHIFT)
#define IS_MEM_ADDR_ALIGNED(addr) (!(addr & (SYSMAP_ADDR_ALIGN_SIZE - 1)))

static inline uint32_t sysmap_region_get_upper_limit(uint32_t region_index)
{
	uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8;

	return hal_readl(reg_addr);
}

static inline uint32_t sysmap_region_get_mem_attr(uint32_t region_index)
{
	uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8 + 4;

	return hal_readl(reg_addr);
}

static void sysmap_region_set_upper_limit(uint32_t region_index, uint32_t upper_limit_addr)
{
	uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8;

	hal_writel(upper_limit_addr, reg_addr);
}

static void sysmap_region_set_mem_attr(uint32_t region_index, uint32_t mem_attr)
{
	uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8 + 4;

	hal_writel(mem_attr, reg_addr);
}


static inline uint32_t get_mem_region_upper_limit(uint32_t region_index)
{
	return sysmap_region_get_upper_limit(region_index) << SYSMAP_ADDR_SHIFT;
}

static inline uint32_t get_mem_region_start_addr(uint32_t region_index)
{
	if (region_index == 0)
		return 0;
	else
		return get_mem_region_upper_limit(region_index - 1);
}

static inline uint32_t get_mem_region_end_addr(uint32_t region_index)
{
	return get_mem_region_upper_limit(region_index) - 1;
}

static inline uint32_t get_mem_region_len(uint32_t region_index)
{
	if (region_index == 0)
		return get_mem_region_upper_limit(region_index);
	else
		return get_mem_region_upper_limit(region_index) - get_mem_region_upper_limit(region_index - 1);
}

static inline uint32_t get_mem_region_attr(uint32_t region_index)
{
	return sysmap_region_get_mem_attr(region_index) & SYSMAP_MEM_ATTR_MASK;
}

static inline void set_mem_region_upper_limit(uint32_t region_index, uint32_t upper_limit_addr)
{
	sysmap_region_set_upper_limit(region_index, upper_limit_addr >> SYSMAP_ADDR_SHIFT);
}

static inline void set_mem_region_attr(uint32_t region_index, uint32_t mem_attr)
{
	sysmap_region_set_mem_attr(region_index, mem_attr & SYSMAP_MEM_ATTR_MASK);
}

static inline void sysmap_setup_mem_region(uint32_t region_index, uint32_t upper_limit_addr, uint32_t mem_attr)
{
	set_mem_region_attr(region_index, mem_attr);
	set_mem_region_upper_limit(region_index, upper_limit_addr);
}

static inline void setup_mem_region(uint32_t region_index,
									uint32_t start_addr, uint32_t len, uint32_t mem_attr)
{
	sysmap_setup_mem_region(region_index, start_addr + len, mem_attr);
}

int sysmap_add_mem_region(uint32_t start_addr, uint32_t len, uint32_t mem_attr)
{
	uint32_t current_region_start_addr;

	if (!IS_MEM_ADDR_ALIGNED(start_addr))
		return SYSMAP_RET_INVALID_MEM_ADDR;

	if (!len)
		return SYSMAP_RET_INVALID_MEM_LEN;

	if ((mem_attr & SYSMAP_MEM_ATTR_MASK) != mem_attr)
		return SYSMAP_RET_INVALID_MEM_ATTR;

	if (g_region_index >= SYSMAP_REGION_NUM)
		return SYSMAP_RET_REGION_IS_FULL;

	current_region_start_addr = 0;
	if (g_region_index)
		current_region_start_addr = get_mem_region_start_addr(g_region_index);

	if (start_addr < current_region_start_addr)
		return SYSMAP_RET_INVALID_MEM_ADDR;

	if (start_addr > current_region_start_addr)
	{
		if (g_region_index == (SYSMAP_REGION_NUM - 1))
			return SYSMAP_RET_REGION_NOT_ENOUGH;

		sysmap_setup_mem_region(g_region_index, start_addr, SYSMAP_MEM_ATTR_SO_NC_NB);
		g_region_index++;
	}

	setup_mem_region(g_region_index, start_addr, len, mem_attr);
	g_region_index++;
	return SYSMAP_RET_OK;
}

static int sysmap_mem_attr_to_str(uint32_t mem_attr, char *buf, uint32_t buf_len)
{
	if (!buf)
		return -1;

	buf[0] = '\0';

	if (buf_len < 9)
		return -2;

	if (mem_attr & SYSMAP_MEM_ATTR_SO)
		strncat(buf, "SO", 3);
	else
		strncat(buf, "WO", 3);

	if (mem_attr & SYSMAP_MEM_ATTR_CACHEABLE)
		strncat(buf, "_C_", 4);
	else
		strncat(buf, "_NC_", 5);


	if (mem_attr & SYSMAP_MEM_ATTR_BUFFERABLE)
		strncat(buf, "B", 2);
	else
		strncat(buf, "NB", 3);

	return 0;
}

void sysmap_dump_region_info(void)
{
	uint32_t i, mem_attr;
	char attr_str_buf[16];

	printf("E90x SYSMAP:\n");
	for (i = 0; i < SYSMAP_REGION_NUM; i++)
	{
		mem_attr = get_mem_region_attr(i);
		sysmap_mem_attr_to_str(mem_attr, attr_str_buf, sizeof(attr_str_buf));
		printf("Region %u, start: 0x%08x, end: 0x%08x, len: 0x%08x, attr: %s (0x%x)\n",
			   i, get_mem_region_start_addr(i), get_mem_region_end_addr(i),
			   get_mem_region_len(i), attr_str_buf, mem_attr);
	}
}
__attribute__((weak)) int init_sysmap(void)
{
	return 0;
}
