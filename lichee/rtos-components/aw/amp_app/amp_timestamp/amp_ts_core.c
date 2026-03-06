/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <aw_list.h>
#include "amp_timestamp.h"

#include "amp_ts_core.h"

#define EXPORT_SYMBOL(x)

#define ARM_ARCH_COUNTER_TYPE 0
#define THEAD_RISCV_ARCH_COUNTER_TYPE 1

typedef struct amp_timestamp_dev {
	uint32_t id;
	uint32_t div_us;
	amp_counter_t counter;
	struct list_head list;
} amp_timestamp_dev_t;

static amp_counter_ops_t *g_counter_ops[] = {
	&g_arm_arch_counter_ops,
	&g_thead_riscv_arch_counter_ops,
};

static LIST_HEAD(g_amp_ts_dev_list);

static amp_timestamp_dev_t *get_amp_ts_dev(uint32_t id)
{
	amp_timestamp_dev_t *pos;

	list_for_each_entry(pos, &g_amp_ts_dev_list, list) {
		if (id == pos->id) {
			return pos;
		}
	}

	return NULL;
}

int amp_ts_get_dev(uint32_t id, amp_ts_dev_t *dev)
{
	amp_timestamp_dev_t *ts_dev = get_amp_ts_dev(id);
	if (!ts_dev)
		return -ENODEV;

	*dev = ts_dev;
	return 0;
}
EXPORT_SYMBOL(amp_ts_get_dev);

int amp_ts_get_timestamp(amp_ts_dev_t dev, uint64_t *timestamp)
{
	amp_timestamp_dev_t *ts_dev = (amp_timestamp_dev_t *)dev;
	amp_counter_t *counter = &ts_dev->counter;
	uint64_t count_value;

	count_value = counter->ops->read_counter(counter);

	*timestamp = count_value / ts_dev->div_us;
	return 0;
}
EXPORT_SYMBOL(amp_ts_get_timestamp);

int amp_ts_get_count_value(amp_ts_dev_t dev, uint64_t *count_value)
{
	amp_timestamp_dev_t *ts_dev = (amp_timestamp_dev_t *)dev;
	amp_counter_t *counter = &ts_dev->counter;

	*count_value = counter->ops->read_counter(counter);
	return 0;
}
EXPORT_SYMBOL(amp_ts_get_count_value);

int amp_ts_get_count_freq(amp_ts_dev_t dev, uint32_t *freq)
{
	amp_timestamp_dev_t *ts_dev = (amp_timestamp_dev_t *)dev;

	*freq = ts_dev->counter.count_freq;
	return 0;
}
EXPORT_SYMBOL(amp_ts_get_count_freq);

static int parse_dev_cfg(uint32_t dev_id, amp_timestamp_dev_t *ts_dev)
{
	uint32_t id, counter_type, count_freq;
	amp_counter_t *counter;

	counter = &ts_dev->counter;

	counter_type = CONFIG_AMP_TS_DEV_0_COUNTER_TYPE;

	if (counter_type != ARM_ARCH_COUNTER_TYPE &&
		counter_type != THEAD_RISCV_ARCH_COUNTER_TYPE) {
		printf("Unsupported counter type(%u)!", counter_type);
		return -EINVAL;
	}

	counter->type = counter_type;

	count_freq = CONFIG_AMP_TS_DEV_0_COUNT_FREQ;

	if (count_freq < 1000000) {
		printf("count freq(%u) is little than 1MHz!", count_freq);
		return -EINVAL;
	}
	counter->count_freq = count_freq;

	id = dev_id;
	if (get_amp_ts_dev(id)) {
		printf("invalid id(%u)", id);
		return -EINVAL;
	}

	counter->reg_base_addr = CONFIG_AMP_TS_DEV_0_COUNTER_REG_ADDR;
	ts_dev->id = id;
	return 0;
}

static int amp_ts_dev_init(amp_timestamp_dev_t *ts_dev)
{
	int ret;
	amp_counter_t *counter;

	ret = parse_dev_cfg(0, ts_dev);
	if (ret) {
		printf("parse_dev_cfg failed, ret: %d\n", ret);
		return -EFAULT;
	}

	counter = &ts_dev->counter;
	counter->ops = g_counter_ops[counter->type];

	ret = counter->ops->init(counter);
	if (ret) {
		printf("amp counter %u init failed, ret: %d\n", ts_dev->id, ret);
		return -1;
	}

	ts_dev->div_us = counter->count_freq / 1000000;

	list_add(&ts_dev->list, &g_amp_ts_dev_list);

	printf("AMP timestamp device %u probe success, count_freq: %uHz\n", ts_dev->id, counter->count_freq);

	return 0;
}

static int g_has_init;

int amp_timestamp_init(void)
{
	int ret = 0;
	amp_timestamp_dev_t *ts_dev;

	if (g_has_init) {
		printf("amp timestamp already init!\n");
		return 0;
	}

	ts_dev = malloc(sizeof(*ts_dev));
	if (!ts_dev) {
		printf("amp_timestamp_dev_t memory allocation failed\n");
		return -ENOMEM;
	}

	ret = amp_ts_dev_init(ts_dev);
	if (ret) {
		printf("amp_ts_dev_init failed, ret: %d\n", ret);
		goto exit_with_free;
	}

	g_has_init = 1;
	return 0;

exit_with_free:
	free(ts_dev);
	return ret;
}

#ifdef CONFIG_AMP_TS_TEST_CMD
#include <console.h>
static int cmd_amp_ts(int argc, char **argv)
{
	int ret;
	amp_ts_dev_t ts_dev = NULL;
	uint64_t ts = 0, count = 0;
	uint32_t freq = 0;

	ret = amp_ts_get_dev(0, &ts_dev);
	if (ret < 0)
		return ret;
	amp_ts_get_count_freq(ts_dev, &freq);
	amp_ts_get_timestamp(ts_dev, &ts);
	amp_ts_get_count_value(ts_dev, &count);

	printf("timestamp:%llu, count:%llu, freq:%u\n", ts, count, freq);

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_amp_ts, amp_ts, AMP TS test);
#endif
