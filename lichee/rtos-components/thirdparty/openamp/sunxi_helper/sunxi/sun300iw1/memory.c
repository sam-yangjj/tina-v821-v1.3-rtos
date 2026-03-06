#include <errno.h>
#include <metal/sys.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/openamp_log.h>

const struct mem_mapping mem_mappings[] = {
	/* SRAM ISP */
	{ .va = 0x2000000, .len = 0xa800, .pa = 0x2000000, .attr = MEM_NONCACHEABLE },
	/* SRAM VE */
	{ .va = 0x200a800, .len = 0x17400, .pa = 0x200a800, .attr = MEM_NONCACHEABLE },
	/* SRAM WIFI */
	{ .va = 0x68000000, .len = 0x1000000, .pa = 0x68000000, .attr = MEM_NONCACHEABLE },
	/* DRAM */
	{ .va = 0x80000000, .len = 0x10000000, .pa = 0x80000000, .attr = MEM_NONCACHEABLE },
};
REGISTER_MEM_MAPPINGS(mem_mappings);
