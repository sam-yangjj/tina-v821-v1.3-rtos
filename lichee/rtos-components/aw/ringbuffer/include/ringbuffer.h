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
#ifndef HAL_RINGBUFFER_H
#define HAL_RINGBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_mem.h>
#include <hal_thread.h>
#include <hal_mutex.h>
#include <hal_sem.h>
#include <hal_event.h>
#include <hal_status.h>

struct hal_ringbuffer;
typedef struct hal_ringbuffer *hal_ringbuffer_t;

hal_ringbuffer_t hal_ringbuffer_init(int size);
void hal_ringbuffer_release(hal_ringbuffer_t rb);
int hal_ringbuffer_resize(hal_ringbuffer_t rb, int size);
uint32_t hal_ringbuffer_length(hal_ringbuffer_t rb);
uint32_t hal_ringbuffer_valid(hal_ringbuffer_t rb);
bool hal_ringbuffer_is_full(hal_ringbuffer_t rb);
bool hal_ringbuffer_is_empty(hal_ringbuffer_t rb);
int hal_ringbuffer_get(hal_ringbuffer_t rb, void *buf, int size, unsigned int timeout);
int hal_ringbuffer_put(hal_ringbuffer_t rb, const void *buf, int size);
int hal_ringbuffer_force_put(hal_ringbuffer_t rb, const void *buf, int size);
int hal_ringbuffer_wait_put(hal_ringbuffer_t rb, const void *buf, int size, int timeout);

#ifdef __cplusplus
}
#endif
#endif
