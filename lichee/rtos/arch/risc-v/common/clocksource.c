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
#include <stdint.h>
#include <stddef.h>
#include <ktimer.h>
#include <stdio.h>
#include <timex.h>
#include <inttypes.h>
#include <csr.h>
#include <sunxi_hal_common.h>

#ifdef CONFIG_DRIVERS_CCMU
#include <hal_clk.h>
#endif

#include <hw_timestamp.h>

#define HW_TS_DEV_DEFAULT_COUNT_FREQ (CONFIG_ARCH_TIMER_HZ)
#define HW_TS_DEV_MIN_COUNT_FREQ 1000000

static u64 arch_counter_read(struct clock_source *cs)
{
	(void)cs;

	return get_cycles64();
}

static struct clock_source riscv_arch_timer = {
	.name = "arch_timer",
	.read = arch_counter_read,
	.mask = CLOCKSOURCE_MASK(63),
};

int arch_timestamp_init(void)
{
	int ret = 0;
	uint32_t count_freq = 0;

#ifdef CONFIG_DRIVERS_CCMU
	count_freq = hal_clock_get_hosc_freq();
#endif

	if (!count_freq)
	{
		count_freq = HW_TS_DEV_DEFAULT_COUNT_FREQ;
	}

	if (count_freq < HW_TS_DEV_MIN_COUNT_FREQ)
	{
		ret = -1;
		count_freq = HW_TS_DEV_MIN_COUNT_FREQ;
	}

	riscv_arch_timer.freq = count_freq;

	return ret;
}

void timekeeping_init(void)
{
	arch_timestamp_init();
	hw_timestamp_init(&riscv_arch_timer);
}

/* legacy API, it will be deprecated and removed in the future */
#ifdef CONFIG_ARCH_SUN20IW2P1
uint32_t arch_timer_get_cntfrq(void)
{
	return riscv_arch_timer.freq;
}
#endif

void timestamp(char *tag)
{
	struct timespec64 ts;
	do_gettimeofday(&ts);

	printf("[TSM]: %*.*s]:sec %" PRId64 ", nsec %d.\n", 12, 12, tag, ts.tv_sec, ts.tv_nsec);
}
