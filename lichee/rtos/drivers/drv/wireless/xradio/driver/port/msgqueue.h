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
#ifndef __XRADIO_MSGQUEUE_H
#define __XRADIO_MSGQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __MELIS__

#elif (defined(CONFIG_ARCH_STM32))

#include "xr_rtos/xr_msgqueue.h"

typedef struct xr_msgqueue xr_drv_msgqueue_t;

#define xr_drv_msgqueue_init(queue, n) XR_OS_MsgQueueCreate(queue, n)
#define xr_drv_msgqueue_destroy(queue) XR_OS_MsgQueueDelete(queue)
#define xr_drv_msgqueue_send(queue, msg) XR_OS_MsgQueueSend(queue, msg, 0)
#define xr_drv_msgqueue_receive(queue, msg) XR_OS_MsgQueueReceive(queue, msg, XR_DRV_WAIT_FOREVER)

#elif (defined(CONFIG_CHIP_XRADIO))

#include "kernel/os/os_queue.h"

typedef OS_Queue_t xr_drv_msgqueue_t;

#define xr_drv_msgqueue_init(queue, n) XR_OS_MsgQueueCreate(queue, n)
#define xr_drv_msgqueue_destroy(queue) XR_OS_MsgQueueDelete(queue)
#define xr_drv_msgqueue_send(queue, msg) XR_OS_MsgQueueSend(queue, msg, 0)
#define xr_drv_msgqueue_receive(queue, msg, timeout) XR_OS_MsgQueueReceive(queue, msg, timeout)

#elif (defined(CONFIG_OS_RTTHREAD) || defined(CONFIG_OS_YUNOS) || (defined(CONFIG_OS_NUTTX)) || \
       defined(CONFIG_OS_TINA))

#include "kernel/os/os_queue.h"

typedef XR_OS_Queue_t xr_drv_msgqueue_t;

#define xr_drv_msgqueue_init(queue, n) XR_OS_MsgQueueCreate(queue, n)
#define xr_drv_msgqueue_destroy(queue) XR_OS_MsgQueueDelete(queue)
#define xr_drv_msgqueue_send(queue, msg) XR_OS_MsgQueueSend(queue, msg, 0)
#define xr_drv_msgqueue_receive(queue, msg, timeout) XR_OS_MsgQueueReceive(queue, msg, timeout)
#define xr_drv_msgqueue_messages_waiting(queue) XR_OS_MsgQueueMessagesWaiting(queue)
/*
#elif (defined(CONFIG_OS_NUTTX))

#include <mqueue.h>

typedef struct nuttx_msg_queue {
	char mqname[16];
	mqd_t queue;
}xr_drv_msgqueue_t;

int nuttx_xr_drv_msgqueue_init(xr_drv_msgqueue_t *queue_hd,uint32_t queueLen);
int nuttx_xr_drv_msgqueue_send(xr_drv_msgqueue_t *queue_hd,void *msg);
int nuttx_xr_drv_msgqueue_receive(xr_drv_msgqueue_t *queue_hd,void *msg,uint32_t timeout);
int nuttx_xr_drv_msgqueue_destroy(xr_drv_msgqueue_t *queue_hd);

#define xr_drv_msgqueue_init      nuttx_xr_drv_msgqueue_init
#define xr_drv_msgqueue_destroy   nuttx_xr_drv_msgqueue_destroy
#define xr_drv_msgqueue_send      nuttx_xr_drv_msgqueue_send
#define xr_drv_msgqueue_receive   nuttx_xr_drv_msgqueue_receive
*/
#endif

#ifdef __cplusplus
}
#endif

#endif /* __XRADIO_MSGQUEUE_H */
