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

#include <gunzip.h>
#include <bunzip2.h>
#include <unlzma.h>
#include <unxz.h>
#include <unlzo.h>
#include <unlz4.h>

#define CALC_RAMDISK_MD5					(0)
#if CALC_RAMDISK_MD5
#include <md5.h>
#endif

typedef int (*decompress_fn) (unsigned char *inbuf, long len,
				long (*fill)(void*, unsigned long),
				long (*flush)(void*, unsigned long),
				unsigned char *outbuf, size_t out_len,
				long *posp,
				void(*error)(char *x));

#ifndef CONFIG_COMPONENTS_COMPRESS_GZIP
# define gunzip NULL
#endif
#ifndef CONFIG_COMPONENTS_COMPRESS_BZIP2
# define bunzip2 NULL
#endif
#ifndef CONFIG_COMPONENTS_COMPRESS_LZMA
# define unlzma NULL
#endif
#ifndef CONFIG_COMPONENTS_COMPRESS_XZ
# define unxz NULL
#endif
#ifndef CONFIG_COMPONENTS_COMPRESS_LZO
# define unlzo NULL
#endif
#ifndef CONFIG_COMPONENTS_COMPRESS_LZ4
# define unlz4 NULL
#endif

struct compress_format {
	unsigned char magic[2];
	const char *name;
	decompress_fn decompressor;
};

static const struct compress_format compressed_formats[] = {
	{ {0x1f, 0x8b}, "gzip", gunzip },
	{ {0x1f, 0x9e}, "gzip", gunzip },
	{ {0x42, 0x5a}, "bzip2", bunzip2 },
	{ {0x5d, 0x00}, "lzma", unlzma },
	{ {0xfd, 0x37}, "xz", unxz },
	{ {0x89, 0x4c}, "lzo", unlzo },
	{ {0x02, 0x21}, "lz4", unlz4 },
	{ {0, 0}, NULL, NULL }
};

static decompress_fn decompress_method(const char *inbuf, int len, const char **name)
{
	const struct compress_format *cf;

	if (len < 2) {
		if (name)
			*name = NULL;
	return NULL;    /* Need at least this much... */
	}

	pr_debug("Compressed data magic: %02x %02x\n", inbuf[0], inbuf[1]);

	for (cf = compressed_formats; cf->name; cf++) {
		if (!memcmp(inbuf, cf->magic, 2))
			break;
	}

	if (name)
		*name = cf->name;
	return cf->decompressor;
}

static void error(char *x)
{
	printf("Decompress error: %s\n", x);
}

static int loak_ramdisk(const char *partition_name, void *buf, unsigned long len)
{
#define SECTOR_SIZE					(512)
	int ret;
	unsigned long actual_len;
	uint32_t sector, sector_num;

	ret = get_partition_by_name("ramdisk", &sector, &sector_num);
	if (ret) {
		printf("get ramdisk partition fail\n");
		return -1;
	}
	printf("ramdisk sector: %u num: %u\n", sector, sector_num);
	if (sector_num * SECTOR_SIZE > len) {
		printf("ramdisk partition is too large, config size: %lu, actual size: %d\n",
						len, sector_num * SECTOR_SIZE);
		return -1;
	}

	flash_read(sector, 1, buf);

	if ((*(uint32_t *)buf) == 0x44525741) { /* MAGIC: 'AWRD' str */
		actual_len = *(uint32_t *)(buf + 4) + 8;
		printf("use actual_len: %lu instead of partition len: %lu\n", actual_len, len);
	} else
		actual_len = sector_num * SECTOR_SIZE;

	flash_read(sector + 1, (actual_len + SECTOR_SIZE - 1) / SECTOR_SIZE, buf + SECTOR_SIZE);

	return actual_len;
}

//#define DEBUG_COMPRESSED_DATA_MD5
static int unpack_to_rootfs(char *buf, unsigned long len, void *outbuf, size_t out_len)
{
	decompress_fn decompress;
	const char *compress_name;

	if ((*(uint32_t *)buf) == 0x44525741) { /* MAGIC: 'AWRD' str */
#if CALC_RAMDISK_MD5
		uint32_t output[4] = { 0 };

		md5((unsigned char *)(buf), len, (unsigned char *)output);
		printf("md5: %08lx%08lx%08lx%08lx\n", __builtin_bswap32(output[0]), __builtin_bswap32(output[1]),
						__builtin_bswap32(output[2]), __builtin_bswap32(output[3]));
#endif
		len -= 8;
		buf += 8;
	}

	decompress = decompress_method(buf, len, &compress_name);
	printf("Detected %s compressed data, len:%lu\n", compress_name ? compress_name : "null", len);
	if (decompress) {
		int ret = decompress((unsigned char *)buf, len, NULL, NULL, outbuf, out_len, NULL, error);
		if (ret)
			printf("decompressor failed\n");
		else
			printf("decompress successed\n");
	} else if (compress_name) {
		printf("compress method %s not configured\n", compress_name);
	} else
		printf("invalid magic at start of compressed archive\n");
	return 0;
}

void load_ramdisk(void)
{
	uint32_t ramdisk_len;

	ramdisk_len = loak_ramdisk("ramdisk", (void *)CONFIG_RAMDISK_LOADADDR, CONFIG_RAMDISK_PARTITION_SIZE);
	if (ramdisk_len < 0) {
		printf("loak_ramdisk failed!\n");
		return;
	}
	unpack_to_rootfs((char *)CONFIG_RAMDISK_LOADADDR, ramdisk_len,
					(void *)CONFIG_RAMDISK_DECOMPRESS_ADDR, CONFIG_RAMDISK_DECOMPRESS_SIZE);
	hal_dcache_clean(CONFIG_RAMDISK_DECOMPRESS_ADDR, CONFIG_RAMDISK_DECOMPRESS_SIZE);
	/* notify kernel */
	rpmsg_notify_init();
	rpmsg_notify("ramdisk", NULL, 0);
}
