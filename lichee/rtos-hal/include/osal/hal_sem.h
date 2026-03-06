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
#ifndef SUNXI_HAL_SEM_H
#define SUNXI_HAL_SEM_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stddef.h>
#include <stdint.h>

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#elif defined(CONFIG_RTTKERNEL)
#include <rtthread.h>
#else
#error "can not support the RTOS!!"
#endif

typedef struct hal_sem {
#ifdef CONFIG_KERNEL_FREERTOS
	StaticSemaphore_t entry;
	SemaphoreHandle_t ptr;
#elif defined(CONFIG_RTTKERNEL)
	struct rt_semaphore entry;
	rt_sem_t ptr;
#endif
} *hal_sem_t;

/**
 * hal_sem_init - Init a semaphore
 *
 * @sem: a point that point to struct hal_sem entry
 * @cnt: sem init val
 *
 */
void hal_sem_init(hal_sem_t sem, unsigned int cnt);

/**
 * hal_sem_deinit - Deinit a semaphore
 *
 * @sem: a point that point to struct hal_sem entry
 *
 */
void hal_sem_deinit(hal_sem_t sem);

/**
 * hal_sem_create - Create a semaphore
 *
 * @cnt: sem init val
 *
 * Return pointer to rpmsg buffer on success, or NULL on failure.
 */
hal_sem_t hal_sem_create(unsigned int cnt);

/**
 * hal_sem_delete - delete a semaphore
 *
 * @sem: a point that create by hal_sem_create
 *
 * Return HAL_OK on success
 */
int hal_sem_delete(hal_sem_t sem);

/**
 * hal_sem_getvalue - get current semaphore value
 *
 * @sem: a semaphore pointer
 * @val: [out] current semaphore value
 *
 * Return HAL_OK on success
 */
int hal_sem_getvalue(hal_sem_t sem, int *val);

/**
 * hal_sem_post - post semaphore
 *
 * @sem: a semaphore pointer
 * @val: [out] current semaphore value
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_post(hal_sem_t sem);

/**
 * hal_sem_timedwait - wait semaphore until acquire semaphore or timeout
 *
 * @sem: a semaphore pointer
 * @ticks: timeout ticks, can use MS_TO_OSTICK to
 *         convert ms to ticks
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_timedwait(hal_sem_t sem, unsigned long ticks);

/**
 * hal_sem_trywait - try to acquire semaphore
 *
 * @sem: a semaphore pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_trywait(hal_sem_t sem);

/**
 * hal_sem_wait - try to acquire semaphore forever
 *
 * @sem: a semaphore pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_wait(hal_sem_t sem);

/**
 * hal_sem_clear - reset semaphore
 *
 * @sem: a semaphore pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_clear(hal_sem_t sem);

#ifdef __cplusplus
}
#endif
#endif
