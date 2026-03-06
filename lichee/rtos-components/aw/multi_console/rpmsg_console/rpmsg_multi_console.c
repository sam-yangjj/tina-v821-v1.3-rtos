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
#include <stdlib.h>
#include <stdint.h>
#include <hal_queue.h>

#include "rpmsg_console.h"

#define RPMSG_BIND_NAME				"console"
#define RPMSG_CONSOLE_MAX			100

#define log					printf

static void rpmsg_create_console(void *arg)
{
	int ret;
	struct rpmsg_service *ser;
	struct rpmsg_ept_client *client = arg;

	ser = client->priv;
	if (!ser) {
		rpmsg_err("no rpmsg service found! client: %p", client);
		goto exit;
	}

	log("create rpmsg%u console.\r\n", client->id);
	ser->shell = rpmsg_console_create(client->ept, client->id);
	if (!ser->shell) {
		rpmsg_err("rpmsg_console_create failed!\r\n");
		ret = -1;
		goto exit_with_release_sem;
	}

	log("create rpmsg%u success.\r\n", client->id);

exit_with_release_sem:
	ret = hal_sem_post(&ser->ready_sem);
	if (ret)
		rpmsg_err("release sem failed, ret: %d\n", ret);

exit:
	hal_thread_stop(NULL);
}

static void rpmsg_delete_console(void *arg)
{
	struct rpmsg_service *ser;
	struct rpmsg_ept_client *client = arg;

	ser = client->priv;
	if (!ser) {
		rpmsg_err("no rpmsg service found! client: %p", client);
		return ;
	}
	if (ser->shell) {
		rpmsg_console_delete(ser->shell);
		ser->shell = NULL;
	}
}

static int rpmsg_bind_cb(struct rpmsg_ept_client *client)
{
	int ret = 0;
	void *thread;
	struct rpmsg_service *ser;

	log("rpmsg%u : binding, name: '%s', client: %p\n",
		client->id, client->name, client);

	ser = hal_malloc(sizeof(*ser));
	if (!ser) {
		rpmsg_err("failed to alloc rpmsg service\n");
		ret = -1;
		goto exit;
	}
	memset(ser, 0, sizeof(*ser));

	hal_sem_init(&ser->ready_sem, 0);
	client->priv = ser;

	thread = hal_thread_create(rpmsg_create_console, client,
					"rp_con_init", 8 * 1024, 5);

	if (thread != NULL) {
		hal_thread_start(thread);
		ret = 0;
	} else
		ret = -EFAULT;

	if (ret) {
		client->priv = NULL;
		hal_free(ser);
	}
exit:
	return ret;
}

static int rpmsg_unbind_cb(struct rpmsg_ept_client *client)
{
	int ret;
	struct rpmsg_service *ser = client->priv;

	log("rpmsg%u: unbinding, name: '%s', client: %p\n",
		client->id, client->name, client);

	if (!ser) {
		rpmsg_err("no rpmsg service found! client: %p\n", client);
		return -ENODEV;
	}

	ret = hal_sem_timedwait(&ser->ready_sem, MS_TO_OSTICK(5000));
	if (ret)
		rpmsg_err("wait rpmsg service ready failed! ret: %d, client: %p\n", ret, client);

	rpmsg_delete_console(client);
	hal_sem_deinit(&ser->ready_sem);
	client->priv = NULL;
	hal_free(ser);
	return 0;
}

static int rpmsg_tmp_ept_cb(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	return 0;
}

int rpmsg_multi_console_init(void)
{
	rpmsg_client_bind(RPMSG_BIND_NAME, rpmsg_tmp_ept_cb, rpmsg_bind_cb,
					rpmsg_unbind_cb, RPMSG_CONSOLE_MAX, NULL);

	return 0;
}

static void rpmsg_multi_console_init_thread(void *arg)
{
	rpmsg_multi_console_init();
	hal_thread_stop(NULL);
}

int rpmsg_multi_console_init_async(void)
{
	void *thread;

	thread = hal_thread_create(rpmsg_multi_console_init_thread, NULL,
					"init", 8 * 1024, 5);
	if (thread != NULL)
		hal_thread_start(thread);

	return 0;
}

void rpmsg_multi_console_close(void)
{
	rpmsg_client_unbind(RPMSG_BIND_NAME);
}

int rpmsg_multi_console_register(void)
{
	return rpmsg_multi_console_init_async();
}
void rpmsg_multi_console_unregister(void)
{
	rpmsg_multi_console_close();
}
