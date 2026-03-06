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
#ifndef SUNXI_HAL_MUTEX_H
#define SUNXI_HAL_MUTEX_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#elif defined(CONFIG_RTTKERNEL)
#include <rtthread.h>
#else
#error "can not support the RTOS!!"
#endif

#include <stdint.h>
#include <stddef.h>

typedef struct hal_mutex {
#ifdef CONFIG_KERNEL_FREERTOS
    //struct QueueDefinition entry;
    StaticSemaphore_t entry;
    SemaphoreHandle_t ptr;
#elif defined(CONFIG_RTTKERNEL)
    struct rt_mutex entry;
    rt_mutex_t ptr;
#endif
} *hal_mutex_t;

/**
 * hal_mutex_init - Init a mutex
 *
 * @mutex: a pointer that point to struct hal_mutex entry
 *
 */
int hal_mutex_init(hal_mutex_t mutex);

/**
 * hal_mutex_detach - Detach a mutex
 *
 * @mutex: a pointer that point to struct hal_mutex entry
 *
 */
int hal_mutex_detach(hal_mutex_t mutex);

/**
 * hal_mutex_create - Create a mutex
 *
 * @cnt: mutex init val
 *
 * Return pointer to hal_mutex_t on success, or NULL on failure.
 */
hal_mutex_t hal_mutex_create(void);

/**
 * hal_mutex_delete - delete a semaphore
 *
 * @mutex: a point that create by hal_mutex_create
 *
 * Return HAL_OK on success
 */
int hal_mutex_delete(hal_mutex_t mutex);

/**
 * hal_mutex_lock - acquire a mutex lock
 *
 * @mutex: a mutex pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_mutex_lock(hal_mutex_t  mutex);

/**
 * hal_mutex_lock - release a mutex lock
 *
 * @mutex: a mutex pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_mutex_unlock(hal_mutex_t  mutex);

/**
 * hal_mutex_trylock - try to acquire a mutex lock
 *
 * @mutex: a mutex pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_mutex_trylock(hal_mutex_t  mutex);

/**
 * hal_mutex_lock - waiting for acquiring a mutex lock until acquiring it sucessfully or timeout
 *
 * @mutex: a mutex pointer
 * @ticks: timeout ticks, can use MS_TO_OSTICK to convert ms to ticks
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_mutex_timedwait(hal_mutex_t mutex, int ticks);

#ifdef __cplusplus
}
#endif

#endif
