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
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <reent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/reent.h>
#include <time.h>
#include <sys/times.h>
#include <sys/lock.h>
#include <string.h>
#include <stdio.h>

#include <FreeRTOSConfig.h>
#include <portmacro.h>
#include <portable.h>

#define CYC_PER_TICK   (24000000 / configTICK_RATE_HZ)

int clock_gettime (clockid_t clock_id, struct timespec *tp)
{
    if (tp == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct timeval tv;
    _gettimeofday_r(NULL, &tv, NULL);
    unsigned long monotonic_time_ms = 0;

    switch (clock_id) {
        case CLOCK_REALTIME:
            tp->tv_sec = tv.tv_sec;
            tp->tv_nsec = tv.tv_usec * 1000L;
            break;
        case CLOCK_MONOTONIC:
            monotonic_time_ms = xTaskGetTickCount() / CYC_PER_TICK * (1000 / configTICK_RATE_HZ);
            tp->tv_sec = monotonic_time_ms / 1000;
            tp->tv_nsec = (monotonic_time_ms % 1000) * 1000000L;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    return 0;
}

int clock_settime (clockid_t clock_id, const struct timespec *tp)
{
    if (tp == NULL) {
        errno = EINVAL;
        return -1;
    }
    struct timeval tv;
    switch (clock_id) {
        case CLOCK_REALTIME:
            tv.tv_sec = tp->tv_sec;
            tv.tv_usec = tp->tv_nsec / 1000L;
            settimeofday(&tv, NULL);
            break;
        default:
            printf("Do not support set monotonic time by clock_settime\n");
            errno = EINVAL;
            return -1;
    }
    return 0;
}

#if 0
int usleep(useconds_t us)
{
    const int us_per_tick = portTICK_PERIOD_MS * 1000;
    if (us < us_per_tick) {
        //ets_delay_us((uint32_t) us);
        vTaskDelay(1);
    } else {
        /* since vTaskDelay(1) blocks for anywhere between 0 and portTICK_PERIOD_MS,
         * round up to compensate.
         */
        vTaskDelay((us + us_per_tick - 1) / us_per_tick);
    }
    return 0;
}

unsigned int sleep(unsigned int seconds)
{
    usleep(seconds*1000000UL);
    return 0;
}

int adjtime(const struct timeval *delta, struct timeval *outdelta)
{
  return 0;
}
#endif

int gettimeready(void)
{
    int ret = 0;
    unsigned long long DIFF_YEAR = 60 * 60 * 24 * 365;

    struct timeval sys_time = {0};
    gettimeofday(&sys_time, NULL);

    if(sys_time.tv_sec > 10 * DIFF_YEAR)
    {
        ret = 1;
    }
    return ret;
}
