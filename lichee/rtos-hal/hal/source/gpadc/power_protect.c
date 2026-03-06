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
#include <FreeRTOS.h>
#include <stdio.h>
#include <string.h>
#include <hal_interrupt.h>
#include "sunxi_hal_gpadc.h"
#include "sunxi_hal_power_protect.h"
#include "platform/gpadc_sun20iw2.h"

#define ADC_CHANNEL                     GP_CH_8

/* #define POWER_PROTECT_DEBUG */
#ifdef POWER_PROTECT_DEBUG
#define pp_info(fmt, args...)  printf("%s()%d - "fmt, __func__, __LINE__, ##args)
#else
#define pp_info(fmt, args...)
#endif

static int flash_ret = 0;

static int gpadc_power_protect_irq_callback(uint32_t data_type, uint32_t data)
{
	static int cnt = 0;

#ifdef POWER_PROTECT_DEBUG
	uint32_t vol_data;
	data = ((VOL_RANGE / 4096) * data); /* 12bits sample rate */
	vol_data = data / 1000;
#endif

	if (data_type == GPADC_DOWN) {
		pp_info("channel %d vol data: %u\n", ADC_CHANNEL, vol_data);
		gpadc_channel_enable_highirq(ADC_CHANNEL);
		flash_ret = 1;
	} else if (data_type == GPADC_UP) {
		pp_info("channel %d vol data: %u\n", ADC_CHANNEL, vol_data);
		flash_ret = 0;
		cnt++;
		if (cnt > 2) {
			gpadc_channel_disable_highirq(ADC_CHANNEL);
			cnt = 0;
		}
	}
	return 0;
}

int get_flash_stat()
{
	return flash_ret;
}

int sunxi_power_protect_init(void)
{
	int ret = -1;
	hal_gpadc_init();
	hal_gpadc_channel_init(ADC_CHANNEL);
	ret = hal_gpadc_register_callback(ADC_CHANNEL, gpadc_power_protect_irq_callback);
	gpadc_channel_enable_lowirq(ADC_CHANNEL);

	return ret;
}
