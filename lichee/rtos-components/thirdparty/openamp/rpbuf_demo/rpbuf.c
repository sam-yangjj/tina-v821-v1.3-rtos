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

#include <errno.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/openamp_platform.h>
#include <rpbuf.h>
#include <hal_thread.h>
#include <hal_mutex.h>
#include <hal_event.h>

static int init_stat[NR_RPROC] = { 0 };
static hal_event_t lock = NULL;

static int rpbuf_lock(uint32_t controller_id)
{
	int ret;

	if (controller_id > NR_RPROC) {
		openamp_err("invalid controller_id = %d\r\n", controller_id);
		return -EINVAL;
	}

	if (lock == NULL) {
		lock = hal_event_create();
		hal_event_set_bits(lock, (1 << NR_RPROC) - 1);
	}

__wait:
	ret = hal_ev_wait_any(lock, 1 << controller_id, HAL_WAIT_FOREVER);
	if (ret < 0) {
		openamp_err("wait event failed, try again.\r\n");
		goto __wait;
	}

	return 0;
}

static int rpbuf_unlock(uint32_t controller_id)
{
	if (controller_id > NR_RPROC) {
		openamp_err("invalid controller_id = %d\r\n", controller_id);
		return -EINVAL;
	}

	hal_event_set_bits(lock, 1 << controller_id);

	return 0;
}

static int rpbuf_lock_delete(void)
{
	if (lock) {
		hal_event_delete(lock);
		lock = NULL;
	}

	return 0;
}

int rpbuf_platform_init(int rproc_id, enum rpbuf_role role)
{
	int ret = 0;
	struct rpmsg_device *rpmsg_dev;
	struct rpbuf_controller *controller;
	struct rpbuf_service *service;

	rpbuf_lock(rproc_id);

	if (init_stat[rproc_id] == 1) {
		printf("rpbuf%d already init.\r\n", rproc_id);
		rpbuf_unlock(rproc_id);
		return 0;
	}

	rpmsg_dev = openamp_sunxi_get_rpmsg_vdev(rproc_id);
	if (!rpmsg_dev) {
		printf("(%s:%d) Failed to get rpmsg device 0\n", __func__, __LINE__);
		ret = -ENOENT;
		goto err_out;
	}

	controller = rpbuf_init_controller(rproc_id, (void *)rpmsg_dev, role, NULL, NULL);
	if (!controller) {
		printf("(%s:%d) rpbuf_init_controller failed\n", __func__, __LINE__);
		goto err_out;
	}

	service = rpbuf_init_service(rproc_id, (void *)rpmsg_dev, role);
	if (!service) {
		printf("(%s:%d) rpbuf_init_service failed\n", __func__, __LINE__);
		goto err_deinit_controller;
	}

	init_stat[rproc_id] = 1;
	rpbuf_unlock(rproc_id);

	return 0;

err_deinit_controller:
	if (role == RPBUF_ROLE_SLAVE)
		rpbuf_deinit_controller(controller);
err_out:
	rpbuf_unlock(rproc_id);
	return ret;
}

void rpbuf_platform_deinit(int rproc_id)
{
	struct rpbuf_controller *controller = rpbuf_get_controller_by_id(rproc_id);
	struct rpbuf_service *service = rpbuf_get_service_by_id(rproc_id);

	rpbuf_deinit_service(service);
	rpbuf_deinit_controller(controller);

	rpbuf_lock(rproc_id);
	init_stat[rproc_id] = 0;
	rpbuf_unlock(rproc_id);
	rpbuf_lock_delete();
}

int rpbuf_init(void)
{
	int ret;

	openamp_dbg("init tocpu rpbuf.\r\n");
	ret = rpbuf_platform_init(RPROC_CPU_ID, RPBUF_ROLE_SLAVE);
	if (ret < 0)
		openamp_err("init tocpu rpbuf failed.\r\n");

#ifdef CONFIG_MULTI_OPENAMP_SUPPORT_DSP
	openamp_dbg("init todsp rpbuf.\r\n");
#ifdef CONFIG_OPENAMP_TODSP_RPROC_MASTER
	ret = rpbuf_platform_init(RPROC_DSP_ID, RPBUF_ROLE_MASTER);
#else
	ret = rpbuf_platform_init(RPROC_DSP_ID, RPBUF_ROLE_SLAVE);
#endif
	if (ret < 0)
		openamp_err("init todsp rpbuf failed.\r\n");
#endif

#ifdef CONFIG_MULTI_OPENAMP_SUPPORT_RV
	openamp_dbg("init torv rpbuf.\r\n");
#ifdef CONFIG_OPENAMP_TORV_RPROC_MASTER
	ret = rpbuf_platform_init(RPROC_RV_ID, RPBUF_ROLE_MASTER);
#else
	ret = rpbuf_platform_init(RPROC_RV_ID, RPBUF_ROLE_SLAVE);
#endif
	if (ret < 0)
		openamp_err("init torv rpbuf failed.\r\n");
#endif

	return 0;
}

static void rpbuf_init_thread(void *arg)
{
	rpbuf_init();
	hal_thread_stop(NULL);
}

int rpbuf_init_async(void)
{
	void *thread;

	thread = hal_thread_create(rpbuf_init_thread, NULL,
					"rpbuf_init", 4 * 1024, 8);
	if (thread)
		hal_thread_start(thread);

	return 0;
}

void rpbuf_deinit(int rproc_id)
{
	openamp_dbg("deinit tocpu rpbuf.\r\n");
	rpbuf_platform_deinit(RPROC_CPU_ID);

#ifdef CONFIG_MULTI_OPENAMP_SUPPORT_DSP
	openamp_dbg("deinit todsp rpbuf.\r\n");
	rpbuf_platform_deinit(RPROC_DSP_ID);
#endif

#ifdef CONFIG_MULTI_OPENAMP_SUPPORT_RV
	openamp_dbg("deinit torv rpbuf.\r\n");
	rpbuf_platform_deinit(RPROC_RV_ID);
#endif
}

