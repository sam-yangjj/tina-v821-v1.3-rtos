#include <errno.h>
#include <metal/sys.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/openamp_log.h>

const struct mem_mapping mem_mappings[] = {
	/* DSP RAM */
	{ .va = 0x20000, .len = 0x20000, .pa = 0x20000, .attr = MEM_NONCACHEABLE },
	/* SRAM A2 */
	{ .va = 0x44000, .len = 0x28000, .pa = 0x44000, .attr = MEM_NONCACHEABLE },
	/* DDR */
	{ .va = 0x8000000, .len = 0x37f00000, .pa = 0x8000000, .attr = MEM_NONCACHEABLE },
	/* SRAM SPACE 1 */
	{ .va = 0x3ff80000, .len = 0x80000, .pa = 0x07200000, .attr = MEM_CACHEABLE },
	/* SRAM SPACE 2 && DRAM SPACE 1 */
	{ .va = 0x40000000, .len = 0x80000, .pa = 0x07280000, .attr = MEM_CACHEABLE },
	/* DRAM SPACE 2 */
	{ .va = 0x40080000, .len = 0x3ff80000, .pa = 0x40080000, .attr = MEM_CACHEABLE },
};
REGISTER_MEM_MAPPINGS(mem_mappings);
