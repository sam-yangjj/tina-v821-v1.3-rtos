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

#include <metal/alloc.h>
#include <openamp/open_amp.h>
#include <hal_msgbox.h>

#include <openamp/sunxi_helper/msgbox_ipi.h>
#include <openamp/sunxi_helper/openamp_log.h>

#include <pm_state.h>

#define REMOTEPROC_STOP_REQUEST		(0xf3f30301)

#ifdef CONFIG_RPMSG_SPEEDTEST
static msgbox_sta_callback full_func = NULL;
static msgbox_sta_callback empty_func = NULL;
#endif

__attribute__((weak))
void platform_remoteproc_stop_eve(void) {}

/**
 * msgbox recive data callback
 * it will wakeup recv task
 * */
static void msgbox_recv_callback(uint32_t data, void *priv)
{
	int ret = 0;
	struct msgbox_ipi *ipi = (struct msgbox_ipi *)priv;

	openamp_dbg("%s: receive kick:%d\r\n", ipi->info.name, data);

	if (data == REMOTEPROC_STOP_REQUEST) {
		platform_remoteproc_stop_eve();
	}

	ret = hal_queue_send(ipi->recv_queue, &data);
	if (ret != 0) {
#ifdef CONFIG_RPMSG_SPEEDTEST
		if (full_func)
			full_func();
#else
		openamp_errFromISR("%s:recv_queue is full\r\n", ipi->info.name);
#endif
	}
}

static void msgbox_recv_handler_task(void *params)
{
	struct msgbox_ipi *ipi = (struct msgbox_ipi *)params;
	uint32_t data = 0;
	uint32_t ret;

	openamp_dbg("%s: recv task start\r\n",ipi->info.name);
	while(1) {
#ifdef CONFIG_RPMSG_SPEEDTEST
		ret = hal_is_queue_empty(ipi->recv_queue);
		if (ret && empty_func)
			empty_func();
#endif
		/* wait forever */
		ret = hal_queue_recv(ipi->recv_queue, &data, -1);
		if(ret != 0)
			continue;
		if(ipi->is_init != 1)
			break;
		if(ipi->callback)
			ipi->callback(ipi->priv, data);
	}
	ipi->msgbox_task_handler = NULL;
	hal_thread_stop(NULL);
}

struct msgbox_ipi *msgbox_ipi_create(struct msgbox_ipi_info *info,
				msgbox_ipi_callback callback, void *priv)
{
	int ret = 0;
	struct msgbox_ipi *ipi = NULL;
	struct msg_endpoint *ept = NULL;

	ipi = metal_allocate_memory(sizeof(*ipi));
	if(!ipi) {
		openamp_err("%s: Failed to allocate memory for ipi handle\n", info->name);
		ret = -ENOMEM;
		goto out;
	}
	memcpy(&ipi->info, info, sizeof(*info));
	ipi->callback = callback;
	ipi->priv = priv;

	ipi->recv_queue = hal_queue_create(info->name, sizeof(uint32_t), info->queue_size);
	if(!ipi->recv_queue) {
		openamp_err("%s: Failed to allocate memory for queue\n", info->name);
		ret  = -ENOMEM;
		goto free_ipi;
	}
	/* hal_msgbox init */
	ept = metal_allocate_memory(sizeof(*ept));
	if(!ept) {
		openamp_err("%s: Failed to allocate memory for mgs endpoint\n", info->name);
		ret  = -ENOMEM;
		goto free_queue;
	}
	/* config ept recv callback */
	ept->rec = msgbox_recv_callback;
	ept->private = (void *)ipi;
	ipi->ept = ept;
	/* alloc msgbox channel */
	ret = hal_msgbox_alloc_channel(ept, info->remote_id, info->read_ch, info->write_ch);
	if(ret != 0) {
		openamp_err("Failed to allocate msgbox channel\n");
		goto free_ept;
	}

	ipi->is_init = 1;
	/* create recv task */
#ifdef CONFIG_ARCH_SUN300IW1
	ipi->msgbox_task_handler = hal_thread_create(msgbox_recv_handler_task, (void *)ipi, info->name,
			HAL_THREAD_STACK_SIZE, HAL_THREAD_PRIORITY_BELOW_HIGH);
#else
	ipi->msgbox_task_handler = hal_thread_create(msgbox_recv_handler_task, (void *)ipi, info->name,
			HAL_THREAD_STACK_SIZE, HAL_THREAD_PRIORITY_HIGHEST);
#endif
	if(ipi->msgbox_task_handler == NULL)
		goto free_channel;
	hal_thread_start(ipi->msgbox_task_handler);

	return ipi;

free_channel:
	hal_msgbox_free_channel(ept);
free_ept:
	metal_free_memory(ept);
free_queue:
	hal_queue_delete(ipi->recv_queue);
free_ipi:
	metal_free_memory(ipi);
out:
	ipi->is_init = -1;
	return NULL;
}

void msgbox_ipi_release(struct msgbox_ipi *ipi)
{
	if(!ipi) {
		openamp_err("Invalid msgbox_ini input\r\n");
		return;
	}

	ipi->is_init = -1;
	/* wakeup thread */
	uint32_t data;
	hal_queue_send(ipi->recv_queue, &data);
	while(ipi->msgbox_task_handler) {
		hal_msleep(1);
	}
	hal_msgbox_free_channel(ipi->ept);
	metal_free_memory(ipi->ept);
	ipi->ept = NULL;
	hal_queue_delete(ipi->recv_queue);
	ipi->recv_queue = NULL;
	metal_free_memory(ipi);
}

/* it will be called when remote connected */
int msgbox_ipi_notify(struct msgbox_ipi *ipi, uint32_t data)
{
	int ret;

#if defined(CONFIG_ARCH_SUN300IW1) && defined(CONFIG_COMPONENTS_PM)
	if (pm_state_get() != PM_STATUS_RUNNING && pm_state_get() != PM_STATUS_PRERUNNING) {
		/* only if openamp is inited or ready to init can send message to remote */
		return 0;
	}
#endif

	ret = hal_msgbox_channel_send(ipi->ept, (uint8_t *)&data, sizeof(data));
	if(ret != 0) {
		openamp_err("%s: Failed to send message to remote\r\n",ipi->info.name);
		return -1;
	}
	return 0;
}

#ifdef CONFIG_RPMSG_SPEEDTEST
void msgbox_set_full_callback(msgbox_sta_callback call)
{
	full_func = call;
}

void msgbox_set_empty_callback(msgbox_sta_callback call)
{
	empty_func = call;
}

void msgbox_reset_full_callback(void)
{
	full_func = NULL;
}

void msgbox_reset_empty_callback(void)
{
	empty_func = NULL;
}
#endif

