#include <stdlib.h>
#include <sunxi_hal_common.h>
#include <flash_read.h>
#include <console.h>
#include <squashfs.h>
#include <hal_time.h>
#include <md5.h>

static int cmd_sqfs_mount(int argc, char **argv)
{
	int ret;
	uint32_t sector, sector_num;
	char *partition_name = argv[1];

	if (argc != 2) {
		printf("Useage: sqfs_mount partition_name\n");
		return 0;
	}

	flash_init();
	ret = get_partition_by_name(partition_name, &sector, &sector_num);
	if (ret) {
		printf("get %s partition fail\n", partition_name);
		return -1;
	}
	printf("%s sector: %u num: %u\n", partition_name, sector, sector_num);

	ret = sqfs_probe(sector, sector_num, 512);
	if (ret) {
		printf("squashfs probe failed, ret = %d\n", ret);
		return -1;
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_sqfs_mount, sqfs_mount, mount a squash fs);

static int sqfs_ls_generic(const char *dirname)
{
	int ret;
	struct squashfs_dir_stream *dirs = NULL;
	struct squashfs_dirent *dent = NULL;
	int nfiles = 0, ndirs = 0;

	ret = sqfs_opendir(dirname, &dirs);
	if (!dirs) {
		printf("sqfs_opendir %s failed, ret=%d\n", dirname, ret);
		return ret;
	}

	while (!sqfs_readdir(dirs, &dent)) {
		if (!dent) {
			printf("sqfs_readdir failed!\n");
			break;
		}

		if (dent->type == 0) { /* TODO: FS_DT_DIR */
			printf("            %s/\n", dent->name);
			ndirs++;
		} else if (dent->type == 3) { /* TODO: FS_DT_LNK */
			printf("    <SYM>   %s\n", dent->name);
			nfiles++;
		} else {
			printf(" %8lld   %s\n", dent->size, dent->name);
			nfiles++;
		}
	}

	sqfs_closedir(dirs);

	printf("\n%d file(s), %d dir(s)\n\n", nfiles, ndirs);

	return 0;
}

static int cmd_sqfs_ls(int argc, char **argv)
{
	if (argc != 2) {
		printf("Useage: sqfs_ls dir\n");
		return 0;
	}

	return sqfs_ls_generic(argv[1]);
}
FINSH_FUNCTION_EXPORT_CMD(cmd_sqfs_ls, sqfs_ls, ls squash dir);

static int cmd_sqfs_read(int argc, char **argv)
{
	unsigned long addr;
	int ret;
	uint64_t t;
	u32 read = 0;
	uint32_t output[4] = { 0 };

	if (argc != 3) {
		printf("Useage: sqfs_read path addr\n");
		return 0;
	}

	addr = strtoul(argv[2], NULL, 0);
	printf("read %s to 0x%08lx\n", argv[1], addr);
	t = hal_get_timestamp_ns();
	ret = sqfs_read(argv[1], (void *)addr, 0, 0, &read);
	t = hal_get_timestamp_ns() - t;
	if (ret) {
		printf("sqfs_read %s failed, ret = %d\n", argv[1], ret);
		return ret;
	}
	printf("read %s bytes: %d, cost: %llu us\n", argv[1], read, t / 1000);
	md5((unsigned char *)(addr), read, (unsigned char *)output);
	printf("md5: %08lx%08lx%08lx%08lx\n", __builtin_bswap32(output[0]), __builtin_bswap32(output[1]),
					__builtin_bswap32(output[2]), __builtin_bswap32(output[3]));

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_sqfs_read, sqfs_read, read a squashfs file);

static int cmd_sqfs_unmount(int argc, char **argv)
{
	sqfs_close();
	flash_deinit();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_sqfs_unmount, sqfs_unmount, unmount a squash fs);
