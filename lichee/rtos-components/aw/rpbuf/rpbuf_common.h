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
#ifndef __RPBUF_COMMON_H__
#define __RPBUF_COMMON_H__

#include <stdio.h>
#include <string.h>
#include <hal_mutex.h>
#include <hal_atomic.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_waitqueue.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define RPBUF_LOG_COLOR_NONE		"\e[0m"
#define RPBUF_LOG_COLOR_RED		"\e[31m"
#define RPBUF_LOG_COLOR_GREEN		"\e[32m"
#define RPBUF_LOG_COLOR_YELLOW	"\e[33m"
#define RPBUF_LOG_COLOR_BLUE		"\e[34m"

// TODO: remove debug
// #define RPBUF_DBG

#ifdef RPBUF_DBG
#define rpbuf_dbg(fmt, args...) \
	printf(RPBUF_LOG_COLOR_GREEN "[RPBUF_DBG][%s:%d]" fmt \
		RPBUF_LOG_COLOR_NONE, __func__, __LINE__, ##args)
#define rpbuf_dbgFromISR(fmt, args...) \
	printfFromISR(RPBUF_LOG_COLOR_GREEN "[RPBUF_DBG][%s:%d]" fmt \
		RPBUF_LOG_COLOR_NONE, __func__, __LINE__, ##args)
#else
#define rpbuf_dbg(fmt, args...)
#define rpbuf_dbgFromISR(fmt, args...)
#endif /* RPBUF_DBG */

#define rpbuf_info(fmt, args...) \
	printf(RPBUF_LOG_COLOR_BLUE "[RPBUF_INFO][%s:%d]" fmt \
		RPBUF_LOG_COLOR_NONE, __func__, __LINE__, ##args)
#define rpbuf_infoFromISR(fmt, args...) \
	printfFromISR(RPBUF_LOG_COLOR_BLUE "[RPBUF_INFO][%s:%d]" fmt \
		RPBUF_LOG_COLOR_NONE, __func__, __LINE__, ##args)

#define rpbuf_err(fmt, args...) \
	printf(RPBUF_LOG_COLOR_RED "[RPBUF_ERR][%s:%d]" fmt \
		RPBUF_LOG_COLOR_NONE, __func__, __LINE__, ##args)
#define rpbuf_errFromISR(fmt, args...) \
	printfFromISR(RPBUF_LOG_COLOR_RED "[RPBUF_ERR][%s:%d]" fmt \
		RPBUF_LOG_COLOR_NONE, __func__, __LINE__, ##args)

typedef hal_mutex_t rpbuf_mutex_t;
typedef hal_spinlock_t rpbuf_spinlock_t;

/* Mutex */
static inline rpbuf_mutex_t rpbuf_mutex_create(void)
{
	hal_mutex_t mutex = hal_mutex_create();
	if (!mutex) {
		printf("rpbuf_mutex_create failed\n");
	}
	return mutex;
}

static inline int rpbuf_mutex_delete(rpbuf_mutex_t mutex)
{
	int ret = hal_mutex_delete(mutex);
	if (ret) {
		printf("rpbuf_mutex_delete failed ret = %d\n", ret);
	}
	return ret;
}

static inline int rpbuf_mutex_lock(rpbuf_mutex_t mutex)
{
	int ret = hal_mutex_lock(mutex);
	if (ret) {
		printf("rpbuf_mutex_lock error ret=%d\n", ret);
	}
	return ret;
}

static inline int rpbuf_mutex_unlock(rpbuf_mutex_t mutex)
{
	int ret = hal_mutex_unlock(mutex);
	if (ret) {
		printf("rpbuf_mutex_unlock error  ret=%d\n", ret);
	}
	return ret;
}

/* Spin lock */
static inline void rpbuf_spin_lock(rpbuf_spinlock_t *lock)
{
	hal_spin_lock(lock);
}

static inline void rpbuf_spin_unlock(rpbuf_spinlock_t *lock)
{
	hal_spin_unlock(lock);
}

static inline uint32_t rpbuf_spin_lock_irqsave(rpbuf_spinlock_t *lock)
{
	return hal_spin_lock_irqsave(lock);
}

static inline void rpbuf_spin_unlock_irqrestore(rpbuf_spinlock_t *lock, uint32_t __cpsr)
{
	hal_spin_unlock_irqrestore(lock, __cpsr);
}

/* Dynamically allocate/free memory */
static inline void *rpbuf_malloc(unsigned int size)
{
	return hal_malloc(size);
}

static inline void *rpbuf_zalloc(unsigned int size)
{
	void *p = rpbuf_malloc(size);
	if (!p)
		return NULL;
	memset(p, 0, size);
	return p;
}

static inline void rpbuf_free(void *p)
{
	hal_free(p);
}

/* Cache */
static inline void rpbuf_dcache_clean(void *addr, unsigned long size)
{
	hal_dcache_clean((unsigned long)addr, size);
}

static inline void rpbuf_dcache_invalidate(void *addr, unsigned long size)
{
	hal_dcache_invalidate((unsigned long)addr, size);
}

#ifdef __cplusplus
}
#endif

#endif /* ifndef __RPBUF_COMMON_H__ */

