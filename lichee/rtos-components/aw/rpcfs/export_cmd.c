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
#include "rpcfs_internal.h"
#include <console.h>
#include <stdlib.h>
#include <string.h>

static int cmd_rpcfs_open(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 2) {
		printf("need ./rpcfs_open path\n");
		return 0;
	}

	ret = rpcfs_open(argv[1], 0, 0);
	if (ret < 0) {
		printf("rpc call rpcfs_open failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_open successed! ret = %d\n", ret);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_open, rpcfs_open, test rpcfs_open);

static int cmd_rpcfs_read(int argc, char *argv[])
{
	int ret = 0;
	uint32_t buf[8];

	if (argc != 2) {
		printf("need ./rpcfs_read fd\n");
		return 0;
	}

	ret = rpcfs_read(atoi(argv[1]), buf, sizeof(buf));
	if (ret < 0) {
		printf("rpc call rpcfs_read failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_read successed! ret = %d\n", ret);
	printf("data: 0x%08x 0x%08x 0x%08x 0x%08x\n", buf[0], buf[1], buf[2], buf[3]);
	printf("data: 0x%08x 0x%08x 0x%08x 0x%08x\n", buf[4], buf[5], buf[6], buf[7]);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_read, rpcfs_read, test rpcfs_read);

static int cmd_rpcfs_write(int argc, char *argv[])
{
	int ret = 0;
	uint32_t buf[8];

	if (argc != 3) {
		printf("need ./rpcfs_write fd data\n");
		return 0;
	}

	printf("memset data 32bytes: 0x%x\n", atoi(argv[2]));
	memset(buf, atoi(argv[2]), sizeof(buf));

	ret = rpcfs_write(atoi(argv[1]), buf, sizeof(buf));
	if (ret < 0) {
		printf("rpc call rpcfs_write failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_write successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_write, rpcfs_write, test rpcfs_write);

static int cmd_rpcfs_close(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 2) {
		printf("need ./rpcfs_close fd\n");
		return 0;
	}

	ret = rpcfs_close(atoi(argv[1]));
	if (ret < 0) {
		printf("rpc call rpcfs_close failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_close successed! ret = %d\n", ret);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_close, rpcfs_close, test rpcfs_close);

static int cmd_rpcfs_link(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 3) {
		printf("need ./rpcfs_link old new\n");
		return 0;
	}

	ret = rpcfs_link(argv[1], argv[2]);
	if (ret < 0) {
		printf("rpc call rpcfs_link failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_link successed! ret = %d\n", ret);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_link, rpcfs_link, test rpcfs_link);

static int cmd_rpcfs_unlink(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 2) {
		printf("need ./rpcfs_unlink path\n");
		return 0;
	}

	ret = rpcfs_unlink(argv[1]);
	if (ret < 0) {
		printf("rpc call rpcfs_unlink failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_unlink successed! ret = %d\n", ret);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_unlink, rpcfs_unlink, test rpcfs_unlink);

static int cmd_rpcfs_dir(int argc, char *argv[])
{
	int ret = 0;
	const char *path;
	DIR *pdir;
	struct dirent *d;

	if (argc != 2) {
		printf("need ./rpcfs_dir dir\n");
		return 0;
	}

	path = argv[1];

	printf("opendir: %s\n", path);
	pdir = rpcfs_opendir(path);
	if (!pdir) {
		printf("rpcfs_opendir failed\n");
		return 0;
	}

	while ((d = rpcfs_readdir(pdir)) != NULL) {
		printf("%8d  %-10s %s\n", d->d_ino, (d->d_type == DT_REG) ?  "regular" :
											(d->d_type == DT_DIR) ?  "directory" :
											(d->d_type == DT_FIFO) ?  "FIFO" :
											(d->d_type == DT_SOCK) ?  "socket" :
											(d->d_type == DT_LNK) ?  "symlink" :
											(d->d_type == DT_BLK) ?  "block dev" :
											(d->d_type == DT_CHR) ?  "char dev" :
											"Unknown", d->d_name);
	}

	printf("opendir: %s\n", path);
	ret = rpcfs_closedir(pdir);
	if (ret) {
		printf("rpcfs_close failed\n");
		return 0;
	}

	printf("rpc call rpcfs_dir successed!\n");

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_dir, rpcfs_dir, test rpcfs_dir);

static inline void print_statfs(struct statfs *st)
{
	printf("struct statfs:\n");
	printf("\tf_type = 0x%lx\n", (unsigned long)st->f_type);
	printf("\tf_bsize = 0x%lx\n", (unsigned long)st->f_bsize);
	printf("\tf_blocks = 0x%lx \n", (unsigned long)st->f_blocks);
	printf("\tf_bfree = 0x%lx\n", (unsigned long)st->f_bfree);
	printf("\tf_bavail = 0x%lx\n", (unsigned long)st->f_bavail);
	printf("\tf_files = 0x%lx\n", (unsigned long)st->f_files);
	printf("\tf_ffree = 0x%lx\n", (unsigned long)st->f_ffree);
}

static inline void print_stat(struct stat *st)
{
	printf("struct stat:\n");
	printf("\tst_dev = 0x%lx\n", (unsigned long)st->st_dev);
	printf("\tst_ino = 0x%lx\n", (unsigned long)st->st_ino);
	printf("\tst_mode = 0x%lx \n", (unsigned long)st->st_mode);
	printf("\tst_nlink = 0x%lx\n", (unsigned long)st->st_nlink);
	printf("\tst_uid = 0x%lx\n", (unsigned long)st->st_uid);
	printf("\tst_gid = 0x%lx\n", (unsigned long)st->st_gid);
	printf("\tst_rdev = 0x%lx\n", (unsigned long)st->st_rdev);
	printf("\tst_size = %lu\n", (unsigned long)st->st_size);
	printf("\tst_atime = %lu\n", (unsigned long)(st->st_atim.tv_sec + st->st_atim.tv_nsec));
	printf("\tst_mtime = %lu\n", (unsigned long)(st->st_mtim.tv_sec + st->st_mtim.tv_nsec));
	printf("\tst_ctime = %lu\n", (unsigned long)(st->st_ctim.tv_sec + st->st_ctim.tv_nsec));
}

static int cmd_rpcfs_statfs(int argc, char *argv[])
{
	int ret = 0;
	struct statfs st;

	if (argc != 2) {
		printf("need ./rpcfs_statfs path\n");
		return 0;
	}

	ret = rpcfs_statfs(argv[1], &st);
	if (ret < 0) {
		printf("rpc call rpcfs_statfs failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_statfs successed! ret = %d\n", ret);
	print_statfs(&st);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_statfs, rpcfs_statfs, test rpcfs_statfs);

static int cmd_rpcfs_fstatfs(int argc, char *argv[])
{
	int ret = 0;
	struct statfs st;

	if (argc != 2) {
		printf("need ./rpcfs_fstatfs fd\n");
		return 0;
	}

	ret = rpcfs_fstatfs(atoi(argv[1]), &st);
	if (ret < 0) {
		printf("rpc call rpcfs_fstatfs failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_fstatfs successed! ret = %d\n", ret);
	print_statfs(&st);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_fstatfs, rpcfs_fstatfs, test rpcfs_fstatfs);

static int cmd_rpcfs_access(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 2) {
		printf("need ./rpcfs_access path\n");
		return 0;
	}

	ret = rpcfs_access(argv[1], F_OK);
	if (ret < 0) {
		printf("rpc call rpcfs_access failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_access successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_access, rpcfs_access, test rpcfs_access);

static int cmd_rpcfs_fsync(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 2) {
		printf("need ./rpcfs_fsync fd\n");
		return 0;
	}

	ret = rpcfs_fsync(atoi(argv[1]));
	if (ret < 0) {
		printf("rpc call rpcfs_fsync failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_fsync successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_fsync, rpcfs_fsync, test rpcfs_fsync);

static int cmd_rpcfs_truncate(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 3) {
		printf("need ./rpcfs_truncate path len\n");
		return 0;
	}

	ret = rpcfs_truncate(argv[1], atoi(argv[2]));
	if (ret < 0) {
		printf("rpc call rpcfs_truncate failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_truncate successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_truncate, rpcfs_truncate, test rpcfs_truncate);

static int cmd_rpcfs_ftruncate(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 3) {
		printf("need ./rpcfs_ftruncate fd len\n");
		return 0;
	}

	ret = rpcfs_ftruncate(atoi(argv[1]), atoi(argv[2]));
	if (ret < 0) {
		printf("rpc call rpcfs_ftruncate failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_ftruncate successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_ftruncate, rpcfs_ftruncate, test rpcfs_ftruncate);

static int cmd_rpcfs_fstat(int argc, char *argv[])
{
	int ret = 0;
	struct stat st;

	if (argc != 2) {
		printf("need ./rpcfs_fstat fd\n");
		return 0;
	}

	ret = rpcfs_fstat(atoi(argv[1]), &st);
	if (ret < 0) {
		printf("rpc call rpcfs_fstat failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_fstat successed! ret = %d\n", ret);
	print_stat(&st);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_fstat, rpcfs_fstat, test rpcfs_fstat);

static int cmd_rpcfs_stat(int argc, char *argv[])
{
	int ret = 0;
	struct stat st;

	if (argc != 2) {
		printf("need ./rpcfs_stat path\n");
		return 0;
	}

	ret = rpcfs_stat(argv[1], &st);
	if (ret < 0) {
		printf("rpc call rpcfs_stat failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_stat successed! ret = %d\n", ret);
	print_stat(&st);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_stat, rpcfs_stat, test rpcfs_stat);

static int cmd_rpcfs_lseek(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 4) {
		printf("need ./rpcfs_lseek fd offset whence\n");
		return 0;
	}

	ret = rpcfs_lseek(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
	if (ret < 0) {
		printf("rpc call rpcfs_lseek failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_lseek successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_lseek, rpcfs_lseek, test rpcfs_lseek);

static int cmd_rpcfs_rename(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 3) {
		printf("need ./rpcfs_rename old new\n");
		return 0;
	}

	ret = rpcfs_rename(argv[1], argv[2]);
	if (ret < 0) {
		printf("rpc call rpcfs_rename failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_rename successed! ret = %d\n", ret);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_rename, rpcfs_rename, test rpcfs_rename);

static int cmd_rpcfs_mkdir(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 2) {
		printf("need ./rpcfs_mkdir path\n");
		return 0;
	}

	ret = rpcfs_mkdir(argv[1], 0);
	if (ret < 0) {
		printf("rpc call rpcfs_mkdir failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_mkdir successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_mkdir, rpcfs_mkdir, test rpcfs_mkdir);

static int cmd_rpcfs_rmdir(int argc, char *argv[])
{
	int ret = 0;

	if (argc != 2) {
		printf("need ./rpcfs_rmdir path\n");
		return 0;
	}

	ret = rpcfs_rmdir(argv[1]);
	if (ret < 0) {
		printf("rpc call rpcfs_rmdir failed, ret = %d!\n", ret);
		return 0;
	}

	printf("rpc call rpcfs_rmdir successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcfs_rmdir, rpcfs_rmdir, test rpcfs_rmdir);
