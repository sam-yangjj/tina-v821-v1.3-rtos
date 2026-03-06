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
#ifndef AW_BREAKPOINT_H
#define AW_BREAKPOINT_H

#include <FreeRTOSConfig.h>

#define BREAK_INSTR_SIZE 4

enum gdb_bptype
{
    BP_BREAKPOINT = 0,
    BP_HARDWARE_BREAKPOINT,
    BP_WRITE_WATCHPOINT,
    BP_READ_WATCHPOINT,
    BP_ACCESS_WATCHPOINT,
    BP_POKE_BREAKPOINT,
};

enum gdb_bpstate
{
    BP_UNDEFINED = 0,
    BP_REMOVED,
    BP_SET,
    BP_ACTIVE
};

struct gdb_bkpt
{
    unsigned long       bpt_addr;
    unsigned char       saved_instr[BREAK_INSTR_SIZE];
    enum gdb_bptype type;
    enum gdb_bpstate    state;
    unsigned long       length;
};

struct gdb_arch
{
    unsigned char gdb_bpt_instr[BREAK_INSTR_SIZE];
    unsigned long flags;

    int (*set_sw_breakpoint)(unsigned long addr, char *saved_instr);
    int (*remove_sw_breakpoint)(unsigned long addr, char *bundle);
    int (*set_hw_breakpoint)(int, unsigned long);
    int (*remove_hw_breakpoint)(int, unsigned long);
    int (*set_hw_watchpoint)(enum gdb_bptype, int, unsigned long);
    int (*remove_hw_watchpoint)(enum gdb_bptype, int, unsigned long);
};

enum gdb_cmdtype
{
    GDBCMD_NOP = 0,
    GDBCMD_INIT,
    GDBCMD_RMALL_BREAK_WATCH,
	
    GDBCMD_IS_RM_HW_BREAK,
	GDBCMD_RM_HW_BREAK,
    GDBCMD_SET_HW_BREAK,
	
    GDBCMD_IS_RM_HW_WATCH,
	GDBCMD_RM_HW_WATCH,
    GDBCMD_SET_HW_WATCH,
};

struct gdb_cmd_arg
{
	volatile int status[configNR_CPUS];
	int reply[configNR_CPUS];
	enum gdb_cmdtype cmd;
	unsigned long addr;
	enum gdb_bptype bp;
	int core;
};

extern struct gdb_arch arch_gdb_ops;

void debug_dump_all_breaks_info(void);

// current cpu only
int remove_all_break_watch_points(void);
int debug_watchpoint_init(void);

int gdb_set_hw_break(unsigned long addr);
int gdb_remove_hw_break(unsigned long addr);
int gdb_isremoved_hw_break(unsigned long addr);

int gdb_set_hw_watch(unsigned long addr, enum gdb_bptype type);
int gdb_remove_hw_watch(unsigned long addr);
int gdb_isremoved_hw_watch(unsigned long addr);

// specify cpu
int debug_watchpoint_init_on_core(int core_mask);
int remove_all_break_watch_points_on_core(int core_mask);

int gdb_isremoved_hw_break_on_core(int core_mask, unsigned long addr);
int gdb_remove_hw_break_on_core(int core_mask, unsigned long addr);
int gdb_set_hw_break_on_core(int core_mask, unsigned long addr);

int gdb_isremoved_hw_watch_on_core(int core_mask, unsigned long addr);
int gdb_remove_hw_watch_on_core(int core_mask, unsigned long addr);
int gdb_set_hw_watch_on_core(int core_mask, unsigned long addr, enum gdb_bptype type);

// all cpu
int debug_watchpoint_init_all_core(void);
int remove_all_break_watch_points_all_core(int core_mask);

int gdb_isremoved_hw_break_all_core(unsigned long addr);
int gdb_remove_hw_break_all_core(unsigned long addr);
int gdb_set_hw_break_all_core(unsigned long addr);

int gdb_isremoved_hw_watch_all_core(unsigned long addr);
int gdb_remove_hw_watch_all_core(unsigned long addr);
int gdb_set_hw_watch_all_core(unsigned long addr, enum gdb_bptype type);

#endif  /*AW_BREAKPOINT_H*/
