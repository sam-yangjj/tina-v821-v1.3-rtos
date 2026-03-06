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

#ifndef _PM_RPCFUNC_H_
#define _PM_RPCFUNC_H_

#include <stdio.h>
#include "pm_base.h"
#include "pm_notify.h"
#include "pm_debug.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_AMPRPC

/*
 * When AMP-RPC is used, stubs and services function both are required.
 * So, these macros have to drop,
 * otherwise there will be a warning
 */

//#ifdef CONFIG_AMP_PMOFM33_SERVICE
int rpc_pm_set_wakesrc(int wakesrc_id, int core, int status);
int rpc_pm_trigger_suspend(int mode);
int rpc_pm_report_subsys_action(int subsys_id, int action);
int rpc_pm_subsys_soft_wakeup(int affinity, int irq, int action);
//#endif

//#ifdef CONFIG_AMP_PMOFRV_SERVICE
int rpc_pm_wakelocks_getcnt_riscv(int stash);
int rpc_pm_msgtorv_trigger_notify(int mode, int event);
int rpc_pm_msgtorv_trigger_suspend(int mode);
int rpc_pm_msgtorv_check_subsys_assert(int mode);
int rpc_pm_msgtorv_check_wakesrc_num(int type);
//#endif

//#ifdef CONFIG_AMP_PMOFDSP_SERVICE
int rpc_pm_wakelocks_getcnt_dsp(int stash);
int rpc_pm_msgtodsp_trigger_notify(int mode, int event);
int rpc_pm_msgtodsp_trigger_suspend(int mode);
int rpc_pm_msgtodsp_check_subsys_assert(int mode);
int rpc_pm_msgtodsp_check_wakesrc_num(int type);
//#endif

//#ifdef CONFIG_AMP_PMOFM33_STUB
int pm_subsys_soft_wakeup(int affinity, int irq, int action);
int pm_set_wakesrc(int wakesrc_id, int core, int status);
int pm_trigger_suspend(suspend_mode_t mode);
int pm_report_subsys_action(int subsys_id, int action);
//#endif

//#ifdef CONFIG_AMP_PMOFRV_STUB
uint32_t pm_wakelocks_getcnt_riscv(int stash);
int pm_msgtorv_trigger_notify(int mode, int event);
int pm_msgtorv_trigger_suspend(int mode);
int pm_msgtorv_check_subsys_assert(int mode);
int pm_msgtorv_check_wakesrc_num(int type);
//#endif

//#ifdef CONFIG_AMP_PMOFDSP_STUB
uint32_t pm_wakelocks_getcnt_dsp(int stash);
int pm_msgtodsp_trigger_notify(int mode, int event);
int pm_msgtodsp_trigger_suspend(int mode);
int pm_msgtodsp_check_subsys_assert(int mode);
int pm_msgtodsp_check_wakesrc_num(int type);
//#endif

#endif

