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
#include <hal_waitqueue.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/msgbox_ipi.h>

#define RPMSG_NOTIFY_MODULE_EPT_NAME "sunxi,notify"
#define RPMSG_NOTIFY_MAX_LEN			32

#define RPMSG_NOTIFY_MODULE_THREAD_NAME "rpmsg_notify"

#if 1
#define debug(fmt, args...)		printf(fmt, ##args)
#else
#define debug(fmt, args...)
#endif

static struct rpmsg_notify_priv {
	struct rpmsg_endpoint *srm_ept;
	hal_waitqueue_head_t wq;
	int inited;
} notify_priv = { 0 };

static int rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	return 0;
}

static void rpmsg_unbind_callback(struct rpmsg_endpoint *ept)
{
	debug("%s is destroyed\n", ept->name);
	notify_priv.inited = 0;
	notify_priv.srm_ept = NULL;
}

int rpmsg_notify(char *name, void *data, int len)
{
	uint8_t buf[RPMSG_MAX_LEN];

	if (!notify_priv.srm_ept && !notify_priv.inited) {
		debug("rpmsg notify module not init or init failed previously!\n");
		return -ENXIO;
	}

	if (!notify_priv.srm_ept && notify_priv.inited)
		hal_wait_event(notify_priv.wq, notify_priv.srm_ept);

	memcpy(buf, name, strlen(name));
	memset(buf + strlen(name), 0, RPMSG_NOTIFY_MAX_LEN - strlen(name));
	if (data)
		memcpy(buf + RPMSG_NOTIFY_MAX_LEN, data, len);
	else
		len = 0;

	openamp_rpmsg_send(notify_priv.srm_ept, buf, RPMSG_NOTIFY_MAX_LEN + len);

	return 0;
}

int rpmsg_notify_init(void)
{
	int ret;
	struct rpmsg_endpoint *ept;
	const char *ept_name;

	if (!notify_priv.srm_ept && notify_priv.inited) {
		debug("waiting previous rpmsg notify module init done.\n");
		hal_wait_event(notify_priv.wq, notify_priv.srm_ept);
		return 0;
	}

	if (notify_priv.srm_ept && notify_priv.inited) {
		debug("previous rpmsg notify module already init done.\n");
		return 0;
	}

	hal_waitqueue_head_init(&notify_priv.wq);
	notify_priv.inited = 1;

	ret = openamp_init();
	if (ret != 0) {
		printf("init openamp framework in rpmsg notify module failed, ret: %d\r\n", ret);
		notify_priv.inited = 0;
		return -1;
	}

	ept_name = RPMSG_NOTIFY_MODULE_EPT_NAME;
	ept = openamp_ept_open(ept_name, 0, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
					NULL, rpmsg_ept_callback, rpmsg_unbind_callback);
	if (ept == NULL) {
		printf("create rpmsg endpoint('%s') failed\r\n", ept_name);
		notify_priv.inited = 0;
		return -2;
	}

	notify_priv.srm_ept = ept;
	hal_wake_up_all(&notify_priv.wq);
	return 0;
}

int rpmsg_notify_deinit(void)
{
	if (!notify_priv.srm_ept && !notify_priv.inited) {
		debug("rpmsg notify module already deinit done.\n");
		return 0;
	}

	if (!notify_priv.srm_ept && notify_priv.inited) {
		debug("waiting previous rpmsg notify module init done.\n");
		hal_wait_event(notify_priv.wq, notify_priv.srm_ept);
	}

	if (notify_priv.srm_ept) {
		openamp_ept_close(notify_priv.srm_ept);
		notify_priv.srm_ept = NULL;
	}
	hal_waitqueue_head_deinit(&notify_priv.wq);
	notify_priv.inited = 0;

	return 0;
}

static void cmd_rpmsg_demo_thread(void *arg)
{
	rpmsg_notify_init();

	hal_thread_stop(NULL);
}

int rpmsg_notify_init_async(void)
{
	void *thread;
	const char *thread_name = RPMSG_NOTIFY_MODULE_THREAD_NAME;

	thread = hal_thread_create(cmd_rpmsg_demo_thread, NULL, thread_name, 2048, 8);
	if(thread != NULL)
		hal_thread_start(thread);
	else
		printf("create thread('%s') failed\r\n", thread_name);

	return 0;
}
