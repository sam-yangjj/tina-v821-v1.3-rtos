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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <console.h>

#include "trigger_module.h"

static void show_help(void)
{
//    debug_dump_all_breaks_info();
    printf("Usage: \n"
	"List hardware supportted triggers - watchpoint trigger \n"
	"List valid watchpoints - watchpoint list \n"
	"Set write watchpoints - watchpoint write addr \n"
	"Set read watchpoints - watchpoint read addr \n"
	"Set access watchpoints - watchpoint access addr \n"
	"Remove watchpoints - watchpoint remove addr \n");
}

int cmd_watchpoint(int argc, char *argv[])
{
	unsigned long addr = 0;
	//unsigned long length = 4;
	char *err = NULL;
	//int core_mask;

	if (argc == 2 && !strcmp(argv[1], "trigger")){
		printf("List support triggers.\n");
		return gdb_list_triggers();

	} else if (argc == 2 && !strcmp(argv[1], "list")) {
		dump_all_watchpoint_info();
		return 0;
	}

	if (argc < 3)
	{
		show_help();
		return -1;
	}

	addr = strtoul(argv[2], &err, 0);
	if (*err != 0)
	{
		printf("addr error\n");
		return -1;
	}

	if (!strcmp(argv[1], "write"))
	{
		printf("Set write watchpoint.\n");
		return gdb_set_hw_watch(addr, WRITE_WATCHPOINT);
	}
	else if (!strcmp(argv[1], "read"))
	{
		printf("Set read watchpoint.\n");
		 return gdb_set_hw_watch(addr, READ_WATCHPOINT);
	}
	else if (!strcmp(argv[1], "access"))
	{
		printf("Set access watchpoint.\n");
		return gdb_set_hw_watch(addr, ACCESS_WATCHPOINT);
	}
	else if (!strcmp(argv[1], "remove"))
	{
		printf("remove watchpoint.\n");
		return gdb_remove_hw_watch(addr);
	}
	else
	{
		show_help();
	}
	return -1;

}
FINSH_FUNCTION_EXPORT_CMD(cmd_watchpoint, watchpoint, Watchpoint Command);
