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

#include <errno.h>
#include <metal/sys.h>
#include <console.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/openamp_log.h>
#include <openamp/sunxi_helper/openamp.h>

extern const struct mem_mapping *_mem_mappings;
extern const int _mem_mappings_size;

int mem_va_to_pa(unsigned long va, metal_phys_addr_t *pa, uint32_t *attr)
{
	const struct mem_mapping *map;
	int i;

	for (i = 0; i < _mem_mappings_size; ++i) {
		map = &_mem_mappings[i];
		if (va >= map->va && va < map->va + map->len) {
			*pa = va - map->va + map->pa;
			if (attr) {
				*attr = map->attr;
			}
			return 0;
		}
	}
	openamp_dbg("Invalid va %lx\n", va);
	return -EINVAL;
}

int mem_pa_to_va(unsigned long pa, metal_phys_addr_t *va, uint32_t *attr)
{
	const struct mem_mapping *map;
	int i;

	/*
	 * TODO:
	 * Maybe there are multiple VAs corresponding to one PA.
	 * Only return the first matching one?
	 */
	for (i = 0; i < _mem_mappings_size; ++i) {
		map = &_mem_mappings[i];
		if (pa >= map->pa && pa < map->pa + map->len) {
			*va = (metal_phys_addr_t)((uintptr_t)(pa - map->pa + map->va));
			if (attr) {
				*attr = map->attr;
			}
			return 0;
		}
	}
	openamp_dbg("Invalid pa 0x%lx\n", pa);
	return -EINVAL;
}

static int dump_mem_mapping(int argc, char *argv[])
{
	const struct mem_mapping *map;
	int i;

	printf("rproc memory mapping:\n");
	printf("va\t\t\t\tpa\t\t\t\tlen\n");
	for (i = 0; i < _mem_mappings_size; ++i) {
		map = &_mem_mappings[i];
		printf("0x%08zx\t\t0x%08zx\t\t0x%08x\t%s\n", (size_t)map->va,
						(size_t)map->pa, map->len,
						map->attr == MEM_CACHEABLE ? "Cacheable" : "Non-Cacheable");
	}
	printf("\n");
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dump_mem_mapping, rproc_dump_mapping, rproc dump mapping);
