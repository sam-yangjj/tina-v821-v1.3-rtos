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
#include <errno.h>
#include <aw_list.h>
#include "rpbuf_internal.h"

#ifdef CONFIG_AW_RPBUF_PERF_TRACE
#include <amp_timestamp.h>
#endif

LIST_HEAD(__rpbuf_controllers);
static rpbuf_mutex_t __rpbuf_controllers_lock;

__attribute__((constructor)) static void rpbuf_controllers_lock_create(void)
{
	__rpbuf_controllers_lock = rpbuf_mutex_create();
	if (!__rpbuf_controllers_lock) {
		printf("rpbuf_controllers_lock_create error\n");
	}
}

__attribute__((destructor)) static void rpbuf_controllers_lock_delete(void)
{

	if (rpbuf_mutex_delete(__rpbuf_controllers_lock)) {
		printf("rpbuf_links_lock_delete error\n");
	}
	__rpbuf_controllers_lock = NULL;
}

struct rpbuf_controller *rpbuf_init_controller(int id, void *token, enum rpbuf_role role,
					       const struct rpbuf_controller_ops *ops,
					       void *priv)
{
	int ret;
	struct rpbuf_controller *controller;

	if (role != RPBUF_ROLE_MASTER && role != RPBUF_ROLE_SLAVE) {
		rpbuf_err("invalid rpbuf role\n");
		goto err_out;
	}

	if (!__rpbuf_controllers_lock)
		rpbuf_controllers_lock_create();

#ifdef CONFIG_AW_RPBUF_PERF_TRACE
	ret = amp_ts_get_dev(CONFIG_RPBUF_AMP_TS_DEV_ID, &g_rpbuf_amp_ts_dev);
	if (ret) {
		rpbuf_info("get AMP timestamp device failed, rpbuf performance data is invalid!\n");
	}
#endif

	controller = rpbuf_create_controller(id, ops, priv);
	if (!controller) {
		rpbuf_err("rpbuf_create_controller failed\n");
		goto err_out;
	}

	ret = rpbuf_register_controller(controller, token, role);
	if (ret < 0) {
		rpbuf_err("rpbuf_register_controller failed\n");
		goto err_destroy_controller;
	}

	rpbuf_mutex_lock(__rpbuf_controllers_lock);
	list_add_tail(&controller->list, &__rpbuf_controllers);
	rpbuf_mutex_unlock(__rpbuf_controllers_lock);

	return controller;

err_destroy_controller:
	rpbuf_destroy_controller(controller);
err_out:
	return NULL;
}

void rpbuf_deinit_controller(struct rpbuf_controller* controller)
{
	if (!controller) {
		rpbuf_err("invalid rpbuf_controller ptr\n");
		return;
	}

	rpbuf_mutex_lock(__rpbuf_controllers_lock);
	list_del(&controller->list);
	rpbuf_mutex_unlock(__rpbuf_controllers_lock);

	rpbuf_unregister_controller(controller);
	rpbuf_destroy_controller(controller);
	rpbuf_controllers_lock_delete();
}

struct rpbuf_controller *rpbuf_get_controller_by_id(int id)
{
	struct rpbuf_controller *controller;

	rpbuf_mutex_lock(__rpbuf_controllers_lock);
	list_for_each_entry(controller, &__rpbuf_controllers, list) {
		if (controller->id == id) {
			rpbuf_mutex_unlock(__rpbuf_controllers_lock);
			return controller;
		}
	}
	rpbuf_mutex_unlock(__rpbuf_controllers_lock);

	return NULL;
}

