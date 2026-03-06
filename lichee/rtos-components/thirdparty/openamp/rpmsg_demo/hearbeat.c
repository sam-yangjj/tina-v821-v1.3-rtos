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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <hal_osal.h>
#include <hal_sem.h>
#include <hal_cache.h>
#include <hal_msgbox.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/msgbox_ipi.h>
#include <pm_state.h>

#define RPMSG_SERVICE_NAME "sunxi,rpmsg_heartbeat"
#define TICK_INTERVAL		100

#if 1
#define debug(fmt, args...)		printf(fmt, ##args)
#else
#define debug(fmt, args...)
#endif

struct rpmsg_heartbeat {
	const char *name;
	uint32_t src;
	uint32_t dst;
	struct rpmsg_endpoint *ept;
	osal_timer_t timer;
	int unbound;
	uint32_t tick;
};

struct hearbeat_packet {
	char name[32];
	uint32_t tick;
};

static struct hearbeat_packet packet = {
	.name = CONFIG_RPMSG_REMOTE_NAME,
};

#ifdef CONFIG_SUPPORT_RPROC_SUSPEND_ONLY
static int g_is_rproc_suspend;
#endif

static struct rpmsg_heartbeat *heart = NULL;
static hal_thread_t heart_thread = NULL;

static void time_out_handler(void *arg)
{
	struct rpmsg_heartbeat *ddata = arg;

#ifdef CONFIG_COMPONENTS_PM
	if (pm_state_get() != PM_STATUS_RUNNING) {
		return ;
	}
#endif

	if (!ddata->unbound) {

#ifdef CONFIG_SUPPORT_RPROC_SUSPEND_ONLY
		uint32_t reg_data = hal_readl(CONFIG_RPROC_PM_INFO_REG_ADDR);
		if ((reg_data & CONFIG_RPROC_PM_INFO_MAGIC_BIT_MASK) == CONFIG_RPROC_PM_INFO_MAGIC) {
#ifdef CONFIG_RPROC_SUSPEND_ONLY_BIT_ACTIVE_LOW
			if (!(reg_data & CONFIG_RPROC_SUSPEND_ONLY_BIT_MASK)) {
#else
			if (reg_data & CONFIG_RPROC_SUSPEND_ONLY_BIT_MASK) {
#endif
				if (!g_is_rproc_suspend)
					printf("remoteproc suspend! stop sending rpmsg heartbeat\n");

				g_is_rproc_suspend = 1;
				return;
			} else {
				if (g_is_rproc_suspend)
					printf("remoteproc resume! start sending rpmsg heartbeat\n");

				g_is_rproc_suspend = 0;
			}
		}
#endif

		packet.tick = ddata->tick;
		openamp_rpmsg_send(ddata->ept, &packet, sizeof(packet));
		ddata->tick++;
		ddata->tick %= TICK_INTERVAL;
	}
}

static int rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	return 0;
}

static void rpmsg_unbind_callback(struct rpmsg_endpoint *ept)
{
	struct rpmsg_heartbeat *heart = ept->priv;

	printf("%s is destroyed\n", ept->name);
	heart->unbound = 1;
	osal_timer_stop(heart->timer);
	osal_timer_delete(heart->timer);
	printf("Stop Rpmsg Hearbeat Timer\r\n");
}

static void cmd_rpmsg_demo_thread(void *arg)
{
	heart = hal_malloc(sizeof(*heart));
	if (!heart) {
		printf("Failed to malloc memory\r\n");
		goto out;
	}

	if (openamp_init() != 0) {
		printf("Failed to init openamp framework\r\n");
		goto out;
	}

	heart->name = RPMSG_SERVICE_NAME;
	heart->unbound = 0;
	heart->tick = 0;
	heart->src = RPMSG_ADDR_ANY;
	heart->dst = RPMSG_ADDR_ANY;

	/* Create Timer */
	heart->timer = osal_timer_create("rpmsg-heart", time_out_handler,
					heart, 5 * CONFIG_HZ - 10,
					OSAL_TIMER_FLAG_SOFT_TIMER |
					OSAL_TIMER_FLAG_PERIODIC);

	if (heart->timer == NULL) {
		printf("Failed to Create Timer\r\n");
		goto out;
	}

	heart->ept = openamp_ept_open(heart->name, 0, heart->src, heart->dst,
					heart, rpmsg_ept_callback, rpmsg_unbind_callback);
	if (heart->ept == NULL) {
		printf("Failed to Create Endpoint\r\n");
		goto out;
	}

	/* we need to send a tick right away to tell the remote our rproc name */
	time_out_handler(heart);
	osal_timer_start(heart->timer);
	printf("Start Rpmsg Hearbeat Timer\r\n");

out:
	heart_thread = NULL;
	hal_thread_stop(NULL);
}

int rpmsg_heart_init(void)
{
	heart_thread = hal_thread_create(cmd_rpmsg_demo_thread, NULL, "rpmsg_heart", 2048, 8);
	if(heart_thread != NULL)
		hal_thread_start(heart_thread);
	else
		printf("fail to create rpmsg hearbeat thread\r\n");

	return 0;
}

int rpmsg_heart_deinit(void)
{
	if (heart_thread) {
		hal_thread_stop(heart_thread);
		heart_thread = NULL;
	}
	if (heart) {
		if (heart->ept) {
			openamp_ept_close(heart->ept);
		}
		if (heart->timer) {
			osal_timer_stop(heart->timer);
			osal_timer_delete(heart->timer);
		}
		hal_free(heart);
		heart = NULL;
	}

	return 0;
}
#ifdef CONFIG_OS_MELIS
subsys_initcall(rpmsg_heart_init);
#endif
