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

#include <barrier.h>
extern void data_sync_barrier(void);

#include <squashfs.h>

#define CALC_RAMDISK_MD5 (0)
#if CALC_RAMDISK_MD5
#include <md5.h>
#endif

#define SECTOR_SIZE (512)

static int _squashfs_mount(char *partition_name)
{
	int ret;
	uint32_t sector, sector_num;

	ret = get_partition_by_name(partition_name, &sector, &sector_num);
	if (ret) {
		printf("get %s partition fail\n", partition_name);
		return ret;
	}
	printf("%s sector: %u num: %u\n", partition_name, sector, sector_num);

	ret = sqfs_probe(sector, sector_num, 512);
	if (ret) {
		printf("squashfs probe failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static int _squashfs_read(const char *path, void *buf, u32 max)
{
	int ret;
	u32 read = 0;
#ifdef CONFIG_PRINT_ROOTFS_LOADTIME
	uint64_t t;

	t = hal_get_timestamp_ns();
#endif
	ret = sqfs_read(path, buf, 0, max, &read);
#ifdef CONFIG_PRINT_ROOTFS_LOADTIME
	t = hal_get_timestamp_ns() - t;
#endif
	if (ret) {
		printf("sqfs_read %s failed, ret = %d\n", path, ret);
		return ret;
	}
#ifdef CONFIG_PRINT_ROOTFS_LOADTIME
	printf("read %s bytes: %d, cost: %llu us\n", path, read, t / 1000);
#endif
#if CALC_RAMDISK_MD5
	{
		uint32_t output[4] = { 0 };
		md5((unsigned char *)(buf), read, (unsigned char *)output);
		printf("md5: %08lx%08lx%08lx%08lx\n",
		       __builtin_bswap32(output[0]),
		       __builtin_bswap32(output[1]),
		       __builtin_bswap32(output[2]),
		       __builtin_bswap32(output[3]));
	}
#endif
	return read;
}

#define _rootfs_mount _squashfs_mount
#define _rootfs_read _squashfs_read
#define _rootfs_unmount sqfs_close

#define N_ALIGN(len) ((((len) + 1) & ~3) + 2)

/* copy from include/uapi/linux/stat.h */
#undef S_IFREG
#define S_IFREG 0100000

static u32 get_hdr_offset(const char *name)
{
	int namesize;

	if (name[0] == '/')
		name++;
	namesize = strlen(name) + 1;

	return 6 + 104 + N_ALIGN(namesize);
}

static void push_hdr(const char *name, char *buf)
{
	char s[16];
	int namesize;
	char *p = buf;

	if (name[0] == '/')
		name++;
	namesize = strlen(name) + 1;

	memcpy(p, "070701", 6);
	p += 6;

	memset(p, 0, 104);
	sprintf(s, "%08X", S_IFREG);
	memcpy(&p[1 * 8], s, 8); /* mode */
	sprintf(s, "%08X", namesize);
	memcpy(&p[11 * 8], s, 8); /* name_len */

	p += 104;
	memcpy(p, name, namesize);
}

static void *push_done(const char *name, char *out, int data_size)
{
	char s[16];
	char *p = out;
	int namesize;

	if (name[0] == '/')
		name++;
	namesize = strlen(name) + 1;

	/* update body_len */
	sprintf(s, "%08X", data_size);
	memcpy(&p[6 * 8 + 6], s, 8); /* body_len */

	if (likely(data_size > (128 * 32)))
		hal_dcache_clean((unsigned long)p, 110 + N_ALIGN(namesize));
	else
		hal_dcache_clean((unsigned long)p,
				 110 + N_ALIGN(namesize) + data_size);
	data_sync_barrier();

	/* next header */
	return (void *)ALIGN_UP(
		(unsigned long)p + 110 + N_ALIGN(namesize) + data_size, 4);
}

static void push_trailer(void *buf)
{
	char s[256];
	const char name[] = "TRAILER!!!";
	char *p = buf;
	int namesize;

	namesize = strlen(name) + 1;

	/* magic */
	memcpy(p, "070701", 6);
	p += 6;

	memset(p, 0, 104);
	sprintf(s, "%08X", namesize);
	memcpy(&p[11 * 8], s, 8); /* name_len */

	p += 104;
	memcpy(p, name, namesize);

	p += namesize;

	memset(p, 0, 512 - ((unsigned long)p) % 512);
	hal_dcache_clean((unsigned long)p, 512);
	data_sync_barrier();
}

static void free_paths(char *paths[], int start_index, int count)
{
	int i;
	for (i = start_index; i < count; i++) {
		if (paths[i]) {
			hal_free(paths[i]);
			paths[i] = NULL;
		}
	}
}

static int parse_filelist(const char *lists, char *item[], int max)
{
	int i = 0;
	const char *p, *lp;

	lp = lists;
	while ((p = strchr(lp, ' '))) {
		if (p == lp) {
			lp++;
			continue;
		}
		item[i] = hal_malloc((int)(p - lp) + 1);
		if (!item[i]) {
			free_paths(item, 0, i);
			return -ENOMEM;
		}
		memcpy(item[i], lp, (int)(p - lp));
		item[i][(int)(p - lp)] = '\0';
		i++;
		if (i == max)
			return i;
		lp = p;
	}

	if (lp[1] != '\0') {
		item[i] = hal_malloc(strlen(lp) + 1);
		if (!item[i]) {
			free_paths(item, 0, i);
			return -ENOMEM;
		}
		memcpy(item[i], lp, strlen(lp) + 1);
		i++;
	}

	return i;
}

static int load_rootfs_file(char *partition_name, void *buf, unsigned long len)
{
	int ret, i, n;
	char *p = buf;
	u32 offset, size, hdr_count = 0;
	char *paths[32];

	if (len < 512) /* no space */
		return 0;

	n = parse_filelist(CONFIG_ROOTFS_LOAD_FILES, paths, ARRAY_SIZE(paths));
	if (n <= 0) {
		printf("parse_filelist: CONFIG_ROOTFS_LOAD_FILES failed\n");
		return 0;
	}

	len -= 512;
	ret = _rootfs_mount(partition_name);
	if (ret) {
		free_paths(paths, 0, n);
		return ret;
	}

	for (i = 0; i < n; i++) {
		const char *path = paths[i];
		if (sqfs_size(path, &size)) {
			printf("%s not exist.\n", path);
			hal_free((void *)path);
			continue;
		}
		if (size > len) {
			printf("output buf too small, cut..\n");
			hal_free((void *)path);
			free_paths(paths, i + 1, n);
			break;
		}
		offset = get_hdr_offset(path);
		ret = _rootfs_read(path, p + offset, 0);
		if (ret < 0) {
			if (ret == -ENOENT) {
				hal_free((void *)path);
				continue;
			}
			hal_free((void *)path);
			free_paths(paths, i + 1, n);
			break;
		}
		push_hdr(path, p);
		p = push_done(path, p, ret);
		hdr_count += 1;
		len -= size;
		hal_free((void *)path);
	}
	if (hdr_count)
		push_trailer(p);
	_rootfs_unmount();
	return ret;
}

void load_rootfs(void)
{
	int ret;

#ifdef CONFIG_PRINT_ROOTFS_LOADTIME
	printf("rootfs_read_thread start...\r\n");
#endif
	ret = load_rootfs_file("rootfs", (void *)CONFIG_ROOTFS_LOADADDR,
			       CONFIG_ROOTFS_PARTITION_SIZE);
	if (ret < 0)
		printf("loak_rootfs_file failed!\n");
#ifdef CONFIG_PRINT_ROOTFS_LOADTIME
	printf("rootfs_read_thread done...\r\n");
#endif
}
