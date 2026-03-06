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
#include <string.h>

#include <ktimer.h>
#include <hal_time.h>
#include <hal_status.h>
#include <hw_timestamp.h>

extern void sleep(int seconds);
extern int usleep(int usecs);
extern void udelay(unsigned int us);

int msleep(unsigned int msecs)
{
    usleep(msecs * 1000);

    return HAL_OK;
}

int hal_sleep(unsigned int secs)
{
    sleep(secs);
    return HAL_OK;
}

int hal_usleep(unsigned int usecs)
{
    usleep(usecs);
    return HAL_OK;
}

int hal_msleep(unsigned int msecs)
{
    msleep(msecs);
    return HAL_OK;
}

void hal_udelay(unsigned int us)
{
    udelay(us);
}

void hal_mdelay(unsigned int ms)
{
    hal_udelay(ms * 1000);
}

void hal_sdelay(unsigned int s)
{
    hal_mdelay(s * 1000);
}

uint64_t hal_get_timestamp(void)
{
    return hw_get_timestamp_us();
}

uint64_t hal_get_timestamp_ns(void)
{
    return hw_get_timestamp_ns();
}

uint64_t hal_get_count_value(void)
{
    return hw_get_count_value();
}

uint32_t hal_get_count_freq(void)
{
    return hw_get_count_freq();
}

/* legacy API, it will be deprecated and removed in the future */
uint64_t hal_gettime_ns(void)
{
    return hw_get_timestamp_ns();
}
