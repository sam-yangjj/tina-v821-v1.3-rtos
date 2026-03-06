/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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

#include <stdlib.h>
#include <string.h>
#include <osal/hal_interrupt.h>
#include <errno.h>
#include <pm_adapt.h>
#include <pm_debug.h>
#include <pm_suspend.h>
#include <pm_wakesrc.h>
#include <pm_wakelock.h>
#include <pm_testlevel.h>
#include <pm_devops.h>
#include <pm_syscore.h>
#include <pm_notify.h>
#include <pm_task.h>
#include <pm_platops.h>
#include <pm_subsys.h>
#include <pm_init.h>
#include <pm_rpcfunc.h>
#include <pm_plat.h>
#ifdef CONFIG_COMPONENTS_VIRT_LOG
#include <virt_log.h>
#endif

int pm_trigger_suspend(suspend_mode_t mode)
{
	if (!pm_suspend_mode_valid(mode))
		return -EINVAL;

	return pm_suspend_request(mode);
}

int pm_init(int argc, char **argv)
{
	/* init wakesrc*/
	pm_wakesrc_init();

	/* init wakelock */
	pm_wakelocks_init();

	/* creat mutex*/
	pm_notify_init();

	/* protect Idle/Timer task */
	pm_task_init();

	/* register ops  */
	pm_plat_platops_init();

	/* creat pm_task and register it, then create queue*/
	pm_suspend_init();

	pm_client_init();

#ifdef CONFIG_COMPONENTS_VIRT_LOG
	virt_log_init();
#endif

	return 0;
}

