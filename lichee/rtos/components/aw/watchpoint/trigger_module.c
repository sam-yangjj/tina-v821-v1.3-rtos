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
// REF - RISC-V External Debug Support Version 0.13-DRAFT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spinlock.h>
#include <csr.h>

#include "trigger_module.h"

/*
                             23 22    21   20     19       18    17    16 15    12    11   10    7  6   5   4   3      2        1       0
  | type | dmode | maskmax | 0 | sizehi | hit | select | timing | sizelo | action | chain | match | m | 0 | s | u | execute | store | load |
     4       1        6             2      1      1        1        2        4        1       4     1   1   1   1      1        1       1

*/

#define TRIGGER_ADDRDATA_BEHAVIOR_SHIFT		0
#define TRIGGER_ADDRDATA_BEHAVIOR_MASK			(_AC(0x7, UL) << TRIGGER_ADDRDATA_BEHAVIOR_SHIFT)
#define TRIGGER_ADDRDATA_BEHAVIOR_LOAD			(_AC(0x1, UL) << TRIGGER_ADDRDATA_BEHAVIOR_SHIFT)
#define TRIGGER_ADDRDATA_BEHAVIOR_STORE			(_AC(0x2, UL) << TRIGGER_ADDRDATA_BEHAVIOR_SHIFT)
#define TRIGGER_ADDRDATA_BEHAVIOR_EXCU			(_AC(0x4, UL) << TRIGGER_ADDRDATA_BEHAVIOR_SHIFT)

#define TRIGGER_ADDRDATA_MODE_SHIFT		3
#define TRIGGER_ADDRDATA_MODE_MASK			(_AC(0xf, UL) << TRIGGER_ADDRDATA_MODE_SHIFT)
#define TRIGGER_ADDRDATA_MODE_U				(_AC(0x1, UL) << TRIGGER_ADDRDATA_MODE_SHIFT)
#define TRIGGER_ADDRDATA_MODE_S				(_AC(0x2, UL) << TRIGGER_ADDRDATA_MODE_SHIFT)
#define TRIGGER_ADDRDATA_MODE_M				(_AC(0x8, UL) << TRIGGER_ADDRDATA_MODE_SHIFT)

#define TRIGGER_ADDRDATA_MATCH_SHIFT		7
#define TRIGGER_ADDRDATA_MATCH_MASK			(_AC(0xf, UL) << TRIGGER_ADDRDATA_MATCH_SHIFT)
#define TRIGGER_ADDRDATA_MATCH0				(_AC(0x0, UL) << TRIGGER_ADDRDATA_MATCH_SHIFT)
#define TRIGGER_ADDRDATA_MATCH1				(_AC(0x1, UL) << TRIGGER_ADDRDATA_MATCH_SHIFT)
#define TRIGGER_ADDRDATA_MATCH2				(_AC(0x2, UL) << TRIGGER_ADDRDATA_MATCH_SHIFT)
#define TRIGGER_ADDRDATA_MATCH3				(_AC(0x3, UL) << TRIGGER_ADDRDATA_MATCH_SHIFT)
#define TRIGGER_ADDRDATA_MATCH4				(_AC(0x4, UL) << TRIGGER_ADDRDATA_MATCH_SHIFT)
#define TRIGGER_ADDRDATA_MATCH5				(_AC(0x5, UL) << TRIGGER_ADDRDATA_MATCH_SHIFT)

#define TRIGGER_ADDRDATA_CHAIN_SHIFT		11
#define TRIGGER_ADDRDATA_CHAIN_MASK			(_AC(0x1, UL) << TRIGGER_ADDRDATA_CHAIN_SHIFT)
#define TRIGGER_ADDRDATA_CHAIN				(_AC(0x0, UL) << TRIGGER_ADDRDATA_CHAIN_SHIFT)

#define TRIGGER_ADDRDATA_ACTION_SHIFT		12
#define TRIGGER_ADDRDATA_ACTION_MASK			(_AC(0xf, UL) << TRIGGER_ADDRDATA_ACTION_SHIFT)
#define TRIGGER_ADDRDATA_ACTION_BREAKPOINT		(_AC(0x0, UL) << TRIGGER_ADDRDATA_ACTION_SHIFT)
#define TRIGGER_ADDRDATA_ACTION_DEBUG			(_AC(0x1, UL) << TRIGGER_ADDRDATA_ACTION_SHIFT)

#define TRIGGER_ADDRDATA_SIZELO_SHIFT		16
#define TRIGGER_ADDRDATA_SIZELO_MASK			(_AC(0x3, UL) << TRIGGER_ADDRDATA_SIZELO_SHIFT)

#define TRIGGER_ADDRDATA_TIMING_SHIFT		18
#define TRIGGER_ADDRDATA_TIMING_MASK			(_AC(0x1, UL) << TRIGGER_ADDRDATA_TIMING_SHIFT)
#define TRIGGER_ADDRDATA_TIMING_TRIGGER_FIRST		(_AC(0x0, UL) << TRIGGER_ADDRDATA_TIMING_SHIFT)
#define TRIGGER_ADDRDATA_TIMING_INSTR_FIRST		(_AC(0x1, UL) << TRIGGER_ADDRDATA_TIMING_SHIFT)

#define TRIGGER_ADDRDATA_SELECT_SHIFT		19
#define TRIGGER_ADDRDATA_SELECT_MASK			(_AC(0x1, UL) << TRIGGER_ADDRDATA_SELECT_SHIFT)
#define TRIGGER_ADDRDATA_SELECT_ADDR			(_AC(0x0, UL) << TRIGGER_ADDRDATA_SELECT_SHIFT)
#define TRIGGER_ADDRDATA_SELECT_DATA			(_AC(0x1, UL) << TRIGGER_ADDRDATA_SELECT_SHIFT)

#define TRIGGER_ADDRDATA_HIT_SHIFT		20
#define TRIGGER_ADDRDATA_HIT_MASK			(_AC(0x1, UL) << TRIGGER_ADDRDATA_HIT_SHIFT)

#define TRIGGER_ADDRDATA_SIZEHI_SHIFT		21
#define TRIGGER_ADDRDATA_SIZEHI_MASK			(_AC(0x3, UL) << TRIGGER_ADDRDATA_SIZEHI_SHIFT)

#define TRIGGER_TCONTROL_MMODE_ENABLE_SHIFT	3
#define TRIGGER_TCONTROL_MMODE_ENABLE			(_AC(0x1, UL) << TRIGGER_TCONTROL_MMODE_ENABLE_SHIFT)

/**
  * watch expr: Set a watchpoint for an expression. Trigger will break when expr is written
  * into by the program and its value changes.
  *
  *   | type | dmode | maskmax | 0 | sizehi | hit | select | timing | sizelo | action | chain | match | m | 0 | s | u | execute | store | load |
  *                                                   0        1                0        0       0     1   0   1   1      0        1       0
  *
  */

int set_watch_expr_hardware( unsigned long addr, unsigned int trigger)
{
	unsigned long taddr = addr;
	unsigned long tmconctrl_value  = 0;
	unsigned long tcontrol_value  = 0;

	if (trigger >= ARCH_SUPPORT_WATCHPOINT_MAX) {
		printf("Invalid trigger number.\n");
		return -1;
	}

	csr_write(CSR_TSELECT, trigger);
	csr_write(CSR_TDATA2, taddr);

	tmconctrl_value = csr_read(CSR_MCONTROL);
	tmconctrl_value &= ~TRIGGER_ADDRDATA_BEHAVIOR_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_BEHAVIOR_STORE;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_MODE_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_MODE_U|TRIGGER_ADDRDATA_MODE_S|TRIGGER_ADDRDATA_MODE_M;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_MATCH_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_MATCH0;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_CHAIN_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_CHAIN;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_ACTION_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_ACTION_BREAKPOINT;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_TIMING_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_TIMING_INSTR_FIRST;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_SELECT_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_SELECT_ADDR;
	csr_write(CSR_MCONTROL, tmconctrl_value);

	printf("Trigger %d addr:0x%lx, mconctrl:0x%lx.\n", trigger, taddr, tmconctrl_value);

	tcontrol_value = csr_read(CSR_TCONCTRL);
	tcontrol_value |= TRIGGER_TCONTROL_MMODE_ENABLE;
	csr_write(CSR_TCONCTRL, tcontrol_value);
	return 0;
}
/**
  *  rwatch expr: Set a watchpoint that will break when watch expr is read by the program
  *
  *
  *   | type | dmode | maskmax | 0 | sizehi | hit | select | timing | sizelo | action | chain | match | m | 0 | s | u | execute | store | load |
  *                                                    0        1                0        0       0     1   0   1   1      0        0       1
  *
  */

int set_rwatch_expr_hardware( unsigned long addr, unsigned int trigger)
{
	unsigned long taddr = addr;
	unsigned long tmconctrl_value  = 0;
	unsigned long tcontrol_value  = 0;

	if (trigger >= ARCH_SUPPORT_WATCHPOINT_MAX) {
		printf("Invalid trigger number.\n");
		return -1;
	}

	csr_write(CSR_TSELECT, trigger);

	csr_write(CSR_TDATA2, taddr);

	tmconctrl_value = csr_read(CSR_MCONTROL);
	tmconctrl_value &= ~TRIGGER_ADDRDATA_BEHAVIOR_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_BEHAVIOR_LOAD;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_MODE_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_MODE_U|TRIGGER_ADDRDATA_MODE_S|TRIGGER_ADDRDATA_MODE_M;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_MATCH_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_MATCH0;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_CHAIN_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_CHAIN;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_ACTION_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_ACTION_BREAKPOINT;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_TIMING_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_TIMING_INSTR_FIRST;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_SELECT_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_SELECT_ADDR;
	csr_write(CSR_MCONTROL, tmconctrl_value);

	printf("Trigger addr:0x%lx, mconctrl:0x%lx.\n", taddr, tmconctrl_value);

	tcontrol_value = csr_read(CSR_TCONCTRL);
	tcontrol_value |= TRIGGER_TCONTROL_MMODE_ENABLE;
	csr_write(CSR_TCONCTRL, tcontrol_value);
	return 0;
}

/**
  * awatch expr: Set a watchpoint that will break when expr is either read or written into by the program
  *
  *
  *   | type | dmode | maskmax | 0 | sizehi | hit | select | timing | sizelo | action | chain | match | m | 0 | s | u | execute | store | load |
  *                                                    0        1                0        0       0     1   0   1   1      0        1       1
  *
  */

int set_awatch_expr_hardware(unsigned long addr, unsigned int trigger)
{
	unsigned long taddr = addr;
	unsigned long tmconctrl_value  = 0;
	unsigned long tcontrol_value  = 0;

	if (trigger >= ARCH_SUPPORT_WATCHPOINT_MAX) {
		printf("Invalid trigger number.\n");
		return -1;
	}

	csr_write(CSR_TSELECT, trigger);

	csr_write(CSR_TDATA2, taddr);

	tmconctrl_value = csr_read(CSR_MCONTROL);
	tmconctrl_value &= ~TRIGGER_ADDRDATA_BEHAVIOR_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_BEHAVIOR_LOAD|TRIGGER_ADDRDATA_BEHAVIOR_STORE;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_MODE_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_MODE_U|TRIGGER_ADDRDATA_MODE_S|TRIGGER_ADDRDATA_MODE_M;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_MATCH_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_MATCH0;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_CHAIN_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_CHAIN;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_ACTION_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_ACTION_BREAKPOINT;

	tmconctrl_value &= ~TRIGGER_ADDRDATA_TIMING_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_TIMING_INSTR_FIRST;



	tmconctrl_value &= ~TRIGGER_ADDRDATA_SELECT_MASK;
	tmconctrl_value |= TRIGGER_ADDRDATA_SELECT_ADDR;
	csr_write(CSR_MCONTROL, tmconctrl_value);

	printf("Trigger addr:0x%lx, mconctrl:0x%lx.\n", taddr, tmconctrl_value);

	tcontrol_value = csr_read(CSR_TCONCTRL);
	tcontrol_value |= TRIGGER_TCONTROL_MMODE_ENABLE;
	csr_write(CSR_TCONCTRL, tcontrol_value);
	return 0;
}

int remove_hardware(unsigned long addr, unsigned int trigger)
{
	unsigned long taddr = addr;
	unsigned long tmconctrl_value  = 0;

	if (trigger >= ARCH_SUPPORT_WATCHPOINT_MAX) {
		printf("Invalid trigger number.\n");
		return -1;
	}

	csr_write(CSR_TSELECT, trigger);

	csr_write(CSR_TDATA2, 0);

	tmconctrl_value = csr_read(CSR_MCONTROL);
	tmconctrl_value &= ~TRIGGER_ADDRDATA_BEHAVIOR_MASK;
	tmconctrl_value &= ~TRIGGER_ADDRDATA_MODE_MASK;

	csr_write(CSR_MCONTROL, tmconctrl_value);

	printf("Trigger addr:0x%lx, mconctrl:0x%lx.\n", taddr, tmconctrl_value);
	return 0;
}

struct percpu_watchpoint cpu_watchpoint[configNR_CPUS];

int gdb_set_hw_watch(unsigned long addr, enum wp_type type)
{
    int cpu_id = cur_cpu_id();
    int i = 0;

    if (cpu_watchpoint[cpu_id].initialize != WATCHPOINT_INTIALIZE) {
	    memset(&cpu_watchpoint[cpu_id], 0, sizeof(struct percpu_watchpoint));
	    cpu_watchpoint[cpu_id].hw_watchpoint = ARCH_SUPPORT_WATCHPOINT_MAX;
	    cpu_watchpoint[cpu_id].initialize = WATCHPOINT_INTIALIZE;
    }

    if (cpu_watchpoint[cpu_id].hw_watchpoint == 0) {
	    printf("No free trigger, set watchpoint failed, addr:0x%lx, type:%d, \
			please delete unuse watchpoint.\n", addr, type);
	return -1;
    }


    for (i = 0; i < ARCH_SUPPORT_WATCHPOINT_MAX; i++)
    {
	    if (cpu_watchpoint[cpu_id].wp_sta[i].state == WP_BUSY &&
			    cpu_watchpoint[cpu_id].wp_sta[i].wp_addr == addr) {
		    printf("WARN: watchpoint addr 0x%lx already vaild.\n", addr);
		    return 0;
	    }
	    if (cpu_watchpoint[cpu_id].wp_sta[i].state == WP_FREE)
		    break;
    }

    if (i >= ARCH_SUPPORT_WATCHPOINT_MAX) {
	  /** Useless code for Static code check
	    * Checking components/aw/watchpoint/trigger_module.c ...
	    * components/aw/watchpoint/trigger_module.c,290,error,arrayIndexOutOfBounds,Array 'cpu_watchpoint[cpu_id].wp_sta[2]' accessed at index 2, which is out of bounds.
	    * components/aw/watchpoint/trigger_module.c,291,error,arrayIndexOutOfBounds,Array 'cpu_watchpoint[cpu_id].wp_sta[2]' accessed at index 2, which is out of bounds.
	    * components/aw/watchpoint/trigger_module.c,292,error,arrayIndexOutOfBounds,Array 'cpu_watchpoint[cpu_id].wp_sta[2]' accessed at index 2, which is out of bounds.
	    */
	    printf("Failed, No free trigger.\n");
	    return -1;

    }

    cpu_watchpoint[cpu_id].wp_sta[i].wp_addr = addr;
    cpu_watchpoint[cpu_id].wp_sta[i].type = type;
    cpu_watchpoint[cpu_id].wp_sta[i].state = WP_BUSY;
    cpu_watchpoint[cpu_id].hw_watchpoint--;

    switch (type) {
	    case WRITE_WATCHPOINT:
		    set_watch_expr_hardware(addr, i);
		    break;
	    case READ_WATCHPOINT:
		    set_rwatch_expr_hardware(addr, i);
		    break;
	    case ACCESS_WATCHPOINT:
		    set_awatch_expr_hardware(addr, i);
		    break;
	    default:
		    printf("ERROR: watchpoint type is err.");
		    break;
    }


    return 0;
}

int gdb_remove_hw_watch(unsigned long addr)
{
	int cpu_id = cur_cpu_id();
	int i;

	for (i = 0; i < ARCH_SUPPORT_WATCHPOINT_MAX; i++)
	{
		if (cpu_watchpoint[cpu_id].wp_sta[i].wp_addr == addr) {
			break;
		}
	}

	if (i >= ARCH_SUPPORT_WATCHPOINT_MAX || cpu_watchpoint[cpu_id].wp_sta[i].state == WP_FREE) {
		printf("Failed, watchpoint with address 0x%lx not be found.\n", addr);
		return -1;
	}

	remove_hardware(addr, i);
	cpu_watchpoint[cpu_id].wp_sta[i].state = WP_FREE;
	cpu_watchpoint[cpu_id].hw_watchpoint++;
	printf("Successfully deleted watchpoint of addr 0x%lx.\n", addr);

	return 0;
}

void dump_all_watchpoint_info(void)
{
	int i, j;

	for(i = 0; i < configNR_CPUS; i++) {
		printf("cpu%d:\n", i);

		if (cpu_watchpoint[i].initialize != WATCHPOINT_INTIALIZE) {
			memset(&cpu_watchpoint[i], 0, sizeof(struct percpu_watchpoint));
			cpu_watchpoint[i].hw_watchpoint = ARCH_SUPPORT_WATCHPOINT_MAX;
			cpu_watchpoint[i].initialize = WATCHPOINT_INTIALIZE;
		}

		printf("watchpoint num = %d:\n",
				ARCH_SUPPORT_WATCHPOINT_MAX - cpu_watchpoint[i].hw_watchpoint);
		printf("Id    Addr    State      Type\n");
		for (j = 0; j < ARCH_SUPPORT_WATCHPOINT_MAX; j++) {
			if (cpu_watchpoint[i].wp_sta[j].state == WP_BUSY)

				printf("%d  0x%lx  %d         %d\n", j,
						cpu_watchpoint[i].wp_sta[j].wp_addr,
						cpu_watchpoint[i].wp_sta[j].state,
						cpu_watchpoint[i].wp_sta[j].type);
		}
	}
}


/**
  * Each trigger may support a variety of features. A debugger can build a list of all triggers and their
  * features as follows:
  * 1. Write 0 to tselect.
  * 2. Read back tselect and check that it contains the written value. If not, exit the loop.
  * 3. Read tinfo.
  * 4. If that caused an exception, the debugger must read tdata1 to discover the type. (If type is
  * 0, this trigger doesn’t exist. Exit the loop.)
  * 5. If info is 1, this trigger doesn’t exist. Exit the loop.
  * 6. Otherwise, the selected trigger supports the types discovered in info.
  * 7. Repeat, incrementing the value in tselect.
  */

int gdb_list_triggers()
{
	int i;
	unsigned long tselect_value = 0;
	unsigned long tinfo_value = 0;

	for (i = 0; i < 16; i++) {
		csr_write(CSR_TSELECT, i);
		tselect_value = csr_read(CSR_TSELECT);
		if (tselect_value != i) {
			printf("Trigger %d not support. Exit the loop.\n", i);
			break;
		}
		tinfo_value = csr_read(CSR_TINFO);
		if (tinfo_value == 1) {
			printf("This trigger doesn’t exist. Exit the loop.\n" );
			break;
		}
		printf("Trigger %d tinfo: 0x%lx.\n", i, tinfo_value);
	}

	return 0;
}
