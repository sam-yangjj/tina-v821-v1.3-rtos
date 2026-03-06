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
#include <flash_read.h>
#include <string.h>
#include <flash_read.h>
#include <sunxi_hal_common.h>
#include <hal_time.h>
#include <hal_thread.h>
#include <hal_cache.h>

#include <gunzip.h>
#include <bunzip2.h>
#include <unlzma.h>
#include <unxz.h>
#include <unlzo.h>
#include <unlz4.h>

#define VIN_SENSOR0_RESERVE_ADDR (0x81CEE000)

static int load_isp_param_from_flash(void *buf, unsigned long len)
{
#define SECTOR_SIZE					(512)
	int ret;
	uint32_t sector, sector_num;

	ret = get_partition_by_name("isp_param", &sector, &sector_num);
	if (ret) {
		printf("get ramdisk partition fail\n");
		return -1;
	}
	printf("isp_param sector: %u num: %u\n", sector, sector_num);

	flash_read(sector, len * SECTOR_SIZE, buf);

	return sector_num;
}

void load_isp_param(void)
{
	uint32_t isp_param_len;

	isp_param_len = load_isp_param_from_flash((void *)VIN_SENSOR0_RESERVE_ADDR, 16);
	if (isp_param_len < 0) {
		printf("load_isp_param_from_flash failed!\n");
		return;
	}
}
