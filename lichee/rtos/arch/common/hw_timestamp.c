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
#include <hw_timestamp.h>
#include <ktimer.h>
#include <sunxi_hal_common.h>
#include <compiler.h>
#include <limits.h>
#include <awlog.h>
#include <console.h>

//#define CONFIG_TIMESTAMP_DEBUG

static clock_source_t *g_cs;

static inline uint64_t clocksource_cyc2ns(clock_source_t *cs, uint64_t cycles)
{
	return ((uint64_t) cycles * cs->cyc2ns.mult) >> cs->cyc2ns.shift;
}

static inline uint64_t clocksource_cyc2us(clock_source_t *cs, uint64_t cycles)
{
	return ((uint64_t) cycles * cs->cyc2us.mult) >> cs->cyc2us.shift;
}

static inline uint64_t clocksource_ns2cyc(clock_source_t *cs, uint64_t ns)
{
	return ((uint64_t) ns * cs->ns2cyc.mult) >> cs->ns2cyc.shift;
}

static inline uint64_t clocksource_us2cyc(clock_source_t *cs, uint64_t us)
{
	return ((uint64_t) us * cs->us2cyc.mult) >> cs->us2cyc.shift;
}

static void clocks_calc_mult_shift(uint32_t *mult, uint32_t *shift, uint32_t from, uint32_t to, uint32_t maxsec)
{
	uint64_t tmp;
	uint32_t sft, sftacc= 32;

	/*
	 * Calculate the shift factor which is limiting the conversion
	 * range:
	 */
	tmp = ((uint64_t)maxsec * from) >> 32;
	while (tmp) {
		tmp >>=1;
		sftacc--;
	}

	/*
	 * Find the conversion shift/mult pair which has the best
	 * accuracy and fits the maxsec conversion range:
	 */
	for (sft = 32; sft > 0; sft--) {
		tmp = (uint64_t) to << sft;
		tmp += from / 2;
		tmp /= from;
		if ((tmp >> sftacc) == 0)
			break;
	}
	*mult = tmp;
	*shift = sft;
}

static int clocksource_init_scale(clock_source_t *cs)
{
	uint64_t sec;
	uint32_t freq = cs->freq;

	if (g_cs) {
		pr_info("current clocksource is %s, ignore %s\n", g_cs->name, cs->name);
		return 0;
	}

	if (freq) {
		sec = cs->mask;
		sec /= freq;

		if (!sec)
			sec = 1;
		else if (sec > 600 && cs->mask > UINT_MAX)
			sec = 600;

		clocks_calc_mult_shift(&cs->cyc2ns.mult, &cs->cyc2ns.shift, freq, NSEC_PER_SEC, sec);
		clocks_calc_mult_shift(&cs->cyc2us.mult, &cs->cyc2us.shift, freq, USEC_PER_SEC, sec);

		clocks_calc_mult_shift(&cs->ns2cyc.mult, &cs->ns2cyc.shift, NSEC_PER_SEC, freq, sec);
		clocks_calc_mult_shift(&cs->us2cyc.mult, &cs->us2cyc.shift, USEC_PER_SEC, freq, sec);
	}

	g_cs = cs;

	return 0;
}

uint64_t hw_get_count_value(void)
{
	if (unlikely(!g_cs)) {
		return 0;
	}

	return g_cs->read(g_cs);
}

uint64_t hw_get_timestamp_us(void)
{
	uint64_t cyc;

	if (unlikely(!g_cs)) {
		return 0;
	}

	cyc = g_cs->read(g_cs);
	return clocksource_cyc2us(g_cs, cyc);
}

uint64_t hw_get_timestamp_ns(void)
{
	uint64_t cyc;

	if (unlikely(!g_cs)) {
		return 0;
	}

	cyc = g_cs->read(g_cs);
	return clocksource_cyc2ns(g_cs, cyc);
}

uint32_t hw_get_count_freq(void)
{
	if (unlikely(!g_cs)) {
		return 0;
	}

	return g_cs->freq;
}

void udelay(unsigned int us)
{
	uint64_t cyc = 0;

	if (unlikely(!g_cs)) {
		pr_err("no available clock_source\n");
		return;
	}

	cyc = g_cs->read(g_cs);
	cyc += clocksource_us2cyc(g_cs, us);

	while (hw_get_count_value() <= cyc);
}

void mdelay(uint32_t ms)
{
	udelay(ms * 1000);
}

void sdelay(uint32_t sec)
{
	mdelay(sec * 1000);
}

int64_t ktime_get(void)
{
	return hw_get_timestamp_ns();
}

int do_gettimeofday(struct timespec64 *ts)
{
	hal_assert(ts != NULL);

	int64_t nsecs = hw_get_timestamp_ns();

	ts->tv_sec  = nsecs / NSEC_PER_SEC;
	ts->tv_nsec = nsecs % NSEC_PER_SEC;

	return 0;
}

int hw_timestamp_init(clock_source_t *cs)
{
	return clocksource_init_scale(cs);
}

#ifdef CONFIG_TIMESTAMP_DEBUG
int cmd_timestamp(int argc, char *argv[])
{
	clock_source_t *cs = g_cs;

	(void)cs;
	(void)argc;
	(void)argv;

	pr_info("clock_source name : %s\n", cs->name);
	pr_info("    freq : %u mask: 0x%llx\n", cs->freq, (unsigned long long)cs->mask);
	pr_info("    cyc2ns: mult: %u, shift: %u\n", cs->cyc2ns.mult, cs->cyc2ns.shift);
	pr_info("    cyc2us: mult: %u, shift: %u\n", cs->cyc2us.mult, cs->cyc2us.shift);
	pr_info("    ns2cyc: mult: %u, shift: %u\n", cs->ns2cyc.mult, cs->ns2cyc.shift);
	pr_info("    us2cyc: mult: %u, shift: %u\n", cs->us2cyc.mult, cs->us2cyc.shift);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_timestamp, timestamp_info, print timestamp info)
#endif