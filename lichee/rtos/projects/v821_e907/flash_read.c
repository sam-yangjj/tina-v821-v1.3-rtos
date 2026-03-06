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
#include <openamp/sunxi_helper/rpmsg_master.h>

#define RTC_DATA_REGS(n)			(0x4a000000 + 0x200 + (n) * 4)
#define FLASH_FLAGS_INDEX			(CONFIG_SPIF_WAIT_INDEX)
#define FLASH_FLAGS_BIT				(CONFIG_SPIF_WAIT_BIT)

void do_flash_read(void)
{
	int i = 0, timeout_cnt = 2000;

	printf("flash_read_thread start...\r\n");
	/* waiting for controller release */
	for (i = 0; i < timeout_cnt; i++) {
		/* wait for boot0 sign out*/
		if (hal_readl(RTC_DATA_REGS(FLASH_FLAGS_INDEX)) & (1 << FLASH_FLAGS_BIT))
			break;
		hal_msleep(1);
	}
	printf("wait flash : %d ms\n", i);

	flash_init();

#ifdef CONFIG_PRELOAD_ISP_PARAM
	extern void load_isp_param(void);
	load_isp_param();
#endif
#ifdef CONFIG_PRELOAD_RAMDISK
	extern void load_ramdisk(void);
	load_ramdisk();
#endif
#ifdef CONFIG_PRELOAD_ROOTFS
	extern void load_rootfs(void);
	load_rootfs();
#endif

	flash_deinit();
}

static void flash_read_thread(void *param)
{
	(void)param;
	do_flash_read();
	rpmsg_notify_init();
#ifdef CONFIG_COMPONENTS_AW_FLASH_READ_SPINOR
	rpmsg_notify("spif", NULL, 0);
#endif
#ifdef CONFIG_COMPONENTS_AW_FLASH_READ_EMMC
	rpmsg_notify("mmc", "44020000.sdmmc", sizeof("44020000.sdmmc"));
#endif
	hal_thread_stop(NULL);
}

void load_ramdisk_sync(void)
{
	do_flash_read();
}

void flash_load_data_async(void)
{
	void *flash_thread;
	flash_thread = hal_thread_create(flash_read_thread, NULL,
			"flash_read", 8 * 1024, HAL_THREAD_PRIORITY_SYS);

	if (flash_thread != NULL)
		hal_thread_start(flash_thread);
}
