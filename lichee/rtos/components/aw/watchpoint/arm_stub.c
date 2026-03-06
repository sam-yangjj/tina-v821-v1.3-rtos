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
#include "hardware_wbp.h"
#include "gdb_stub.h"
#include <stdio.h>
#include <FreeRTOSConfig.h>
static int kgdb_arch_init_flag[configNR_CPUS] = {0};

int create_hw_break_watch(unsigned int hw_break, unsigned int hw_watch);

int arm_arch_set_hw_watchpoint(enum gdb_bptype type, int i, unsigned long addr)
{
    return arm_install_hw_watchpoint(type, i, addr);
}

int arm_arch_remove_hw_watchpoint(enum gdb_bptype type, int i, unsigned long addr)
{
    arm_uninstall_hw_watchpoint(i);
    return 0;
}

int arm_arch_set_hw_breakpoint(int i, unsigned long addr)
{
    return arm_install_hw_breakpoint(i, addr);
}

int arm_arch_remove_hw_breakpoint(int i, unsigned long addr)
{
    arm_uninstall_hw_breakpoint(i);
    return 0;
}

struct gdb_arch arch_gdb_ops =
{
    .set_hw_watchpoint = arm_arch_set_hw_watchpoint,
    .remove_hw_watchpoint = arm_arch_remove_hw_watchpoint,
    .set_hw_breakpoint = arm_arch_set_hw_breakpoint,
    .remove_hw_breakpoint = arm_arch_remove_hw_breakpoint,
};

int kgdb_arch_init(void)
{
    int processor_id = cur_cpu_id();
    int ret = -1;

    unsigned int brp = 0;
    unsigned int wrp = 0;

    if (kgdb_arch_init_flag[processor_id] > 0)
    {
        return 0;
    }

    if (!monitor_mode_enabled())
    {
        ret = enable_monitor_mode();
        if (ret)
        {
            printf("cpu%d: enter monitor mode failed!\n", processor_id);
            return -1;
        }
    }

    brp = get_num_brp_resources();
    wrp = get_num_wrp_resources();

    if (create_hw_break_watch(brp, wrp))
    {
        return -1;
    }

    kgdb_arch_init_flag[processor_id] = 1;
    return ret;
}
