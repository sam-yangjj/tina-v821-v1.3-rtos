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
/// Copyright 2018-2023 Piotr Grygorczuk <grygorek@gmail.com>
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.

#ifndef _GTHR_FREERTOS_X__H_
#define _GTHR_FREERTOS_X__H_

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <sys/time.h>

namespace free_rtos_std
{
  struct Once
  {
    bool v = false;
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    ~Once() { vSemaphoreDelete(m); }
  };
}

extern "C"
{

#define __GTHREAD_COND_INIT_FUNCTION
#define __GTHREADS 1

// returns: 1 - thread system is active; 0 - thread system is not active
static int __gthread_active_p() { return 0; }

typedef free_rtos_std::Once __gthread_once_t;
typedef SemaphoreHandle_t __gthread_mutex_t;
typedef SemaphoreHandle_t __gthread_recursive_mutex_t;
typedef int __gthread_key_t;
typedef int __gthread_cond_t;

#define _GLIBCXX_UNUSED __attribute__((__unused__))

#define __GTHREAD_ONCE_INIT free_rtos_std::Once()

  static inline void __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION(
      __gthread_recursive_mutex_t *mutex)
  {
    *mutex = xSemaphoreCreateRecursiveMutex();
  }
  static inline void __GTHREAD_MUTEX_INIT_FUNCTION(__gthread_mutex_t *mutex)
  {
    *mutex = xSemaphoreCreateMutex();
  }

  static int __gthread_once(__gthread_once_t *once, void (*func)(void))
  {
    if (!once->m)
      return 12; // POSIX error: ENOMEM

    bool flag{true};
    xSemaphoreTake(once->m, portMAX_DELAY);
    std::swap(once->v, flag);
    if (flag == false)
      func();
    xSemaphoreGive(once->m);

    return 0;
  }

  static int __gthread_key_create(__gthread_key_t *keyp _GLIBCXX_UNUSED, void (*dtor)(void *) _GLIBCXX_UNUSED)
  {
    return 0;
  }

  static int __gthread_key_delete(__gthread_key_t key _GLIBCXX_UNUSED)
  {
    return 0;
  }

  static void *__gthread_getspecific(__gthread_key_t key _GLIBCXX_UNUSED)
  {
    return 0;
  }

  static int __gthread_setspecific(__gthread_key_t key _GLIBCXX_UNUSED, const void *ptr _GLIBCXX_UNUSED)
  {
    return 0;
  }

  static inline int __gthread_mutex_destroy(__gthread_mutex_t *mutex)
  {
    vSemaphoreDelete(*mutex);
    return 0;
  }
  static inline int __gthread_recursive_mutex_destroy(
      __gthread_recursive_mutex_t *mutex)
  {
    vSemaphoreDelete(*mutex);
    return 0;
  }

  static inline int __gthread_mutex_lock(__gthread_mutex_t *mutex)
  {
    return (xSemaphoreTake(*mutex, portMAX_DELAY) == pdTRUE) ? 0 : 1;
  }
  static inline int __gthread_mutex_trylock(__gthread_mutex_t *mutex)
  {
    return (xSemaphoreTake(*mutex, 0) == pdTRUE) ? 0 : 1;
  }
  static inline int __gthread_mutex_unlock(__gthread_mutex_t *mutex)
  {
    return (xSemaphoreGive(*mutex) == pdTRUE) ? 0 : 1;
  }

  static inline int __gthread_recursive_mutex_lock(
      __gthread_recursive_mutex_t *mutex)
  {
    return (xSemaphoreTakeRecursive(*mutex, portMAX_DELAY) == pdTRUE) ? 0 : 1;
  }
  static inline int __gthread_recursive_mutex_trylock(
      __gthread_recursive_mutex_t *mutex)
  {
    return (xSemaphoreTakeRecursive(*mutex, 0) == pdTRUE) ? 0 : 1;
  }
  static inline int __gthread_recursive_mutex_unlock(
      __gthread_recursive_mutex_t *mutex)
  {
    return (xSemaphoreGiveRecursive(*mutex) == pdTRUE) ? 0 : 1;
  }

} // extern "C"

#endif // _GTHR_FREERTOS_X__H_
