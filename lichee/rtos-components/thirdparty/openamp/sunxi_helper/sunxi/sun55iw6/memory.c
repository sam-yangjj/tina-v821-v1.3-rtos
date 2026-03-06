#include <errno.h>
#include <metal/sys.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/openamp_log.h>

const struct mem_mapping mem_mappings[] = {
	/* SRAM A2 */
	{ .va = 0x40000, .len = 0x2bfff, .pa = 0x40000, .attr = MEM_NONCACHEABLE },
	/* SHARED SRAM */
	{ .va = 0x1000000, .len = 0x7ffff, .pa = 0x1000000, .attr = MEM_NONCACHEABLE },
	/* DRAM SPACE1 */
	{ .va = 0x40000000, .len = 0x3f000000, .pa = 0x40000000, .attr = MEM_CACHEABLE },
	/* DRAM SPACE2 */
	{ .va = 0x80000000, .len = 0x3f000000, .pa = 0x80000000, .attr = MEM_CACHEABLE },
	/* DRAM SPACE3 */
	{ .va = 0xC0000000, .len = 0x3f000000, .pa = 0xC0000000, .attr = MEM_CACHEABLE },
};
REGISTER_MEM_MAPPINGS(mem_mappings);
