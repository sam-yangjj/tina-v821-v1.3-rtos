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
#include <openamp/rpmsg.h>
#include <openamp/sunxi_helper/openamp.h>
#include "rpbuf_internal.h"

#define RPMSG_RPBUF_SERVICE_NAME "rpbuf-service"
#define RPMSG_RPBUF_SERVICE_SRC_ADDR RPMSG_ADDR_ANY
#define RPMSG_RPBUF_SERVICE_DST_ADDR RPMSG_ADDR_ANY

struct rpmsg_rpbuf_service_instance {
	struct rpmsg_endpoint *ept;
	hal_waitqueue_head_t wq;
};

LIST_HEAD(__rpbuf_services);
static rpbuf_mutex_t __rpbuf_services_lock;

__attribute__((constructor)) static void rpbuf_services_lock_create(void)
{
	__rpbuf_services_lock = rpbuf_mutex_create();
	if (!__rpbuf_services_lock) {
		printf("rpbuf_services_lock_create error\n");
	}
}

__attribute__((destructor)) static void rpbuf_services_lock_delete(void)
{
	if (rpbuf_mutex_delete(__rpbuf_services_lock)) {
		printf("rpbuf_services_lock_delete error\n");
	}
	__rpbuf_services_lock = NULL;
}

static int rpmsg_rpbuf_service_ept_cb(struct rpmsg_endpoint *ept, void *data,
				      size_t len, uint32_t src, void *priv)
{
	struct rpbuf_service *service = priv;
	int ret;

	ret = rpbuf_service_get_notification(service, data, len);
	if (ret < 0) {
		rpbuf_err("rpbuf_service_get_notification failed\n");
	}

	/*
	 * Return 0 anyway because openamp checks the returned value of endpoint
	 * callback. We cannot return a negative value otherwise it will trigger
	 * the assertion failure.
	 */
	return 0;
}

static int rpmsg_rpbuf_service_notify(void *msg, int msg_len, void *priv)
{
	struct rpmsg_rpbuf_service_instance *inst = priv;
	struct rpmsg_endpoint *ept = inst->ept;

	if (!is_rpmsg_ept_ready(ept)) {
		rpbuf_err("endpoint not ready\n");
		return -EACCES;
	}

	return rpmsg_trysend(ept, msg, msg_len);
}

static const struct rpbuf_service_ops rpmsg_rpbuf_service_ops = {
	.notify = rpmsg_rpbuf_service_notify,
};

int rpbuf_driver_probe(struct rpmsg_device *rdev, struct rpmsg_endpoint *ept,
				uint32_t src, uint32_t dest)
{
	struct rpmsg_rpbuf_service_instance *inst = ept->priv;

	rpbuf_info("rpbuf ept('%s') register.\r\n", ept->name);

	if (!ept->priv) {
		rpbuf_err("invalid rpbuf ept.\r\n");
		return 0;
	}

	inst->ept = ept;
	hal_wake_up(&inst->wq);

	return 0;
}

void rpbuf_driver_remove(struct rpmsg_endpoint *ept)
{
	struct rpbuf_service *service = ept->priv;

	rpbuf_deinit_service(service);
}

static struct rpmsg_driver rpbuf_driver = {
	.name = RPMSG_RPBUF_SERVICE_NAME,
	.probe = rpbuf_driver_probe,
	.remove = rpbuf_driver_remove,
	.callback = rpmsg_rpbuf_service_ept_cb,
};

/* 'token' must be a rpmsg_device */
struct rpbuf_service *rpbuf_init_service(int id, void *token, enum rpbuf_role role)
{
	int ret;
	struct rpmsg_device *rpmsg_dev = token;
	struct rpmsg_rpbuf_service_instance *inst;
	struct rpbuf_service *service;

	if (!token) {
		rpbuf_err("invalid token\n");
		goto err_out;
	}

	if (!__rpbuf_services_lock) {
		rpbuf_services_lock_create();
	}

	inst = rpbuf_zalloc(sizeof(struct rpmsg_rpbuf_service_instance));
	if (!inst) {
		rpbuf_err("rpbuf_zalloc failed\n");
		goto err_out;
	}

	inst->ept = rpbuf_zalloc(sizeof(*inst->ept));
	if (!inst) {
		rpbuf_err("rpbuf_zalloc failed\n");
		goto err_free_inst;
	}

	service = rpbuf_create_service(id, &rpmsg_rpbuf_service_ops, (void *)inst);
	if (!service) {
		rpbuf_err("rpbuf_create_service failed\n");
		goto err_free_ept;
	}

	inst->ept->priv = (void *)service;

	ret = rpbuf_register_service(service, (void *)rpmsg_dev);
	if (ret < 0) {
		rpbuf_err("rpbuf_register_service failed\n");
		goto err_destroy_service;
	}

	if (role == RPBUF_ROLE_SLAVE) {
		ret = rpmsg_create_ept(inst->ept, rpmsg_dev, RPMSG_RPBUF_SERVICE_NAME,
				       RPMSG_RPBUF_SERVICE_SRC_ADDR, RPMSG_RPBUF_SERVICE_DST_ADDR,
				       rpmsg_rpbuf_service_ept_cb, NULL);
		if (ret != 0) {
			rpbuf_err("rpmsg_create_ept failed: %d\n", ret);
			goto err_unregister_service;
		}
	} else {
		rpbuf_info("Waiting for rpbuf ept('%s') register.\r\n", rpbuf_driver.name);
		rpbuf_driver.priv = inst;
		inst->ept = NULL;
 		hal_waitqueue_head_init(&inst->wq);

		rpmsg_register_driver(&rpbuf_driver);
		hal_wait_event(inst->wq, inst->ept != NULL);
	}

	rpbuf_info("Waiting for rpbuf ept('%s') ready.\r\n", inst->ept->name);

	while (!is_rpmsg_ept_ready(inst->ept))
		hal_msleep(1);

	rpbuf_info("rpbuf ept('%s') ready! src: 0x%x, dst: 0x%x\r\n", inst->ept->name,
					inst->ept->addr, inst->ept->dest_addr);

	rpbuf_mutex_lock(__rpbuf_services_lock);
	list_add_tail(&service->list, &__rpbuf_services);
	rpbuf_mutex_unlock(__rpbuf_services_lock);

	return service;

err_unregister_service:
	rpbuf_unregister_service(service);
err_destroy_service:
	rpbuf_destroy_service(service);
err_free_ept:
	rpbuf_free(inst->ept);
err_free_inst:
	rpbuf_free(inst);
err_out:
	return NULL;
}

void rpbuf_deinit_service(struct rpbuf_service *service)
{
	struct rpmsg_rpbuf_service_instance *inst = service->priv;

	if (!service) {
		rpbuf_err("invalid rpbuf_service ptr\n");
		return;
	}

	rpmsg_unregister_driver(&rpbuf_driver);
 	hal_waitqueue_head_deinit(&inst->wq);

	rpbuf_mutex_lock(__rpbuf_services_lock);
	list_del(&service->list);
	rpbuf_mutex_unlock(__rpbuf_services_lock);

	rpbuf_unregister_service(service);
	rpbuf_destroy_service(service);

	rpmsg_destroy_ept(inst->ept);
	rpbuf_free(inst->ept);
	rpbuf_free(inst);
	rpbuf_services_lock_delete();
}

struct rpbuf_service *rpbuf_get_service_by_id(int id)
{
	struct rpbuf_service *service;

	rpbuf_mutex_lock(__rpbuf_services_lock);
	list_for_each_entry(service, &__rpbuf_services, list) {
		if (service->id == id) {
			rpbuf_mutex_unlock(__rpbuf_services_lock);
			return service;
		}
	}
	rpbuf_mutex_unlock(__rpbuf_services_lock);

	return NULL;
}

