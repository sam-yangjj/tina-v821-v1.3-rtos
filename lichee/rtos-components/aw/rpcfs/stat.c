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
#include <stdlib.h>
#include <string.h>

static DECLARE_SRPC_FUNC(rpcfs_statfs64, RPCFS_FUNC_STATFS_ID,
	SRPC_PROTO(const char *pathname, struct statfs *st, int *ret),
	SRPC_ARGS(pathname, st, ret),
	SRPC_INPUT__entry(
		_srpc_field(uint32_t, path_pa),
		_srpc_field(uint64_t, f_type),
		_srpc_field(uint64_t, f_bsize),
		_srpc_field(uint64_t, f_bfree),
		_srpc_field(uint64_t, f_blocks),
		_srpc_field(uint64_t, f_bavail),
		_srpc_field(uint64_t, f_files),
		_srpc_field(uint64_t, f_ffree),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->path_pa = local_va2pa((void *)pathname);
		rpcfs_dbg("rpcfs_statfs64: %s\n", pathname);
	),
	SRPC_OUTPUT(
		st->f_type = __entry->f_type;
		st->f_bsize = __entry->f_bsize;
		st->f_blocks = __entry->f_blocks;
		st->f_bfree = __entry->f_bfree;
		st->f_bavail = __entry->f_bavail;
		st->f_files = __entry->f_files;
		st->f_ffree = __entry->f_ffree;
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_statfs64: ret = %d\n", *ret);
	)
);

int rpcfs_statfs(const char *pathname, struct statfs *st)
{
	int ret = 0, rpc_ret;
	char str_buf[RPCFS_MAX_PATH_LEN] __attribute__ ((aligned (RPCFS_ARCH_CACHELINE)));

	memcpy(str_buf, pathname, strlen(pathname) > RPCFS_MAX_PATH_LEN - 1 ? RPCFS_MAX_PATH_LEN - 1 : strlen(pathname) + 1);
	str_buf[RPCFS_MAX_PATH_LEN - 1] = '\0';
	rpcfs_cache_clean(str_buf, RPCFS_MAX_PATH_LEN);

	rpc_ret = srpc_rpcfs_statfs64(str_buf, st, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_statfs64 failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}

static DECLARE_SRPC_FUNC(rpcfs_fstatfs64, RPCFS_FUNC_FSTATFS_ID,
	SRPC_PROTO(int fd, struct statfs *st, int *ret),
	SRPC_ARGS(fd, st, ret),
	SRPC_INPUT__entry(
		_srpc_field(int, fd),
		_srpc_field(uint64_t, f_type),
		_srpc_field(uint64_t, f_bsize),
		_srpc_field(uint64_t, f_bfree),
		_srpc_field(uint64_t, f_blocks),
		_srpc_field(uint64_t, f_bavail),
		_srpc_field(uint64_t, f_files),
		_srpc_field(uint64_t, f_ffree),
		_srpc_field(int, ret)

	),
	SRPC_INPUT(
		rpcfs_dbg("rpcfs_fstatfs64: fd=%d\n", fd);
		__entry->fd = fd;
	),
	SRPC_OUTPUT(
		st->f_type = __entry->f_type;
		st->f_bsize = __entry->f_bsize;
		st->f_blocks = __entry->f_blocks;
		st->f_bfree = __entry->f_bfree;
		st->f_bavail = __entry->f_bavail;
		st->f_files = __entry->f_files;
		st->f_ffree = __entry->f_ffree;
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_fstatfs64: ret = %d\n", *ret);
	)
);

int rpcfs_fstatfs(int fd, struct statfs *st)
{
	int ret = 0, rpc_ret;

	rpc_ret = srpc_rpcfs_fstatfs64(fd, st, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_fstatfs64 failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}

static DECLARE_SRPC_FUNC(rpcfs_access, RPCFS_FUNC_ACCESS_ID,
	SRPC_PROTO(const char *pathname, uint32_t mode, int *ret),
	SRPC_ARGS(pathname, mode, ret),
	SRPC_INPUT__entry(
		_srpc_field(uint32_t, path_pa),
		_srpc_field(uint32_t, mode),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->path_pa = local_va2pa((void *)pathname);
		__entry->mode = mode;
		rpcfs_dbg("rpcfs_access: %s mode:0x%x\n", pathname, mode);
	),
	SRPC_OUTPUT(
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_access: ret = %d\n", *ret);
	)
);

int rpcfs_access(const char *pathname, int mode)
{
	int ret = 0, rpc_ret;
	char str_buf[RPCFS_MAX_PATH_LEN] __attribute__ ((aligned (RPCFS_ARCH_CACHELINE)));

	memcpy(str_buf, pathname, strlen(pathname) > RPCFS_MAX_PATH_LEN - 1 ? RPCFS_MAX_PATH_LEN - 1 : strlen(pathname) + 1);
	str_buf[RPCFS_MAX_PATH_LEN - 1] = '\0';
	rpcfs_cache_clean(str_buf, RPCFS_MAX_PATH_LEN);

	rpc_ret = srpc_rpcfs_access(str_buf, mode, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_access failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}

static DECLARE_SRPC_FUNC(rpcfs_fsync, RPCFS_FUNC_FSYNC_ID,
	SRPC_PROTO(int fd, int *ret),
	SRPC_ARGS(fd, ret),
	SRPC_INPUT__entry(
		_srpc_field(int, fd),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->fd = fd;
		rpcfs_dbg("rpcfs_fsync: fd=%d\n", __entry->fd);
	),
	SRPC_OUTPUT(
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_fsync: ret = %d\n", *ret);
	)
);

int rpcfs_fsync(int fd)
{
	int ret = 0, rpc_ret;

	rpc_ret = srpc_rpcfs_fsync(fd, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_fsync failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}

static DECLARE_SRPC_FUNC(rpcfs_truncate, RPCFS_FUNC_TRUNCATE_ID,
	SRPC_PROTO(const char *pathname, uint64_t len, int *ret),
	SRPC_ARGS(pathname, len, ret),
	SRPC_INPUT__entry(
		_srpc_field(uint64_t, len),
		_srpc_field(uint32_t, path_pa),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->path_pa = local_va2pa((void *)pathname);
		__entry->len = len;
		rpcfs_dbg("rpcfs_truncate: %s len:%llu\n", pathname, __entry->len);
	),
	SRPC_PROTO(
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_truncate: ret = %d\n", *ret);
	)
);

int rpcfs_truncate(const char *pathname, off_t len)
{
	int ret = 0, rpc_ret;
	char str_buf[RPCFS_MAX_PATH_LEN] __attribute__ ((aligned (RPCFS_ARCH_CACHELINE)));

	memcpy(str_buf, pathname, strlen(pathname) > RPCFS_MAX_PATH_LEN - 1 ? RPCFS_MAX_PATH_LEN - 1 : strlen(pathname) + 1);
	str_buf[RPCFS_MAX_PATH_LEN - 1] = '\0';
	rpcfs_cache_clean(str_buf, RPCFS_MAX_PATH_LEN);

	rpc_ret = srpc_rpcfs_truncate(str_buf, len, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_truncate failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}

static DECLARE_SRPC_FUNC(rpcfs_ftruncate, RPCFS_FUNC_FTRUNCATE_ID,
	SRPC_PROTO(int fd, uint64_t len, int *ret),
	SRPC_ARGS(fd, len, ret),
	SRPC_INPUT__entry(
		_srpc_field(uint64_t, len),
		_srpc_field(int, fd),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->fd = fd;
		__entry->len = len;
		rpcfs_dbg("rpcfs_ftruncate: fd=%d len:%llu\n", __entry->fd, __entry->len);
	),
	SRPC_OUTPUT(
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_ftruncate: ret = %d\n", *ret);
	)
);

int rpcfs_ftruncate(int fd, off_t len)
{
	int ret = 0, rpc_ret;

	rpc_ret = srpc_rpcfs_ftruncate(fd, len, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_ftruncate failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}

static DECLARE_SRPC_FUNC(rpcfs_lseek, RPCFS_FUNC_LSEEK_ID,
	SRPC_PROTO(unsigned int fd, int64_t offset, int whence, int *ret),
	SRPC_ARGS(fd, offset, whence, ret),
	SRPC_INPUT__entry(
		_srpc_field(int64_t, offset),
		_srpc_field(unsigned int, fd),
		_srpc_field(unsigned int, whence),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->offset = offset;
		__entry->fd = fd;
		__entry->whence = whence;
		rpcfs_dbg("rpcfs_lseek: fd=%d ofs:%lld whence:%d\n", fd, offset, whence);
	),
	SRPC_OUTPUT(
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_lseek: ret = %d\n", *ret);
	)
);

off_t rpcfs_lseek(int fd, off_t offset, int whence)
{
	int ret = 0, rpc_ret;

	rpc_ret = srpc_rpcfs_lseek(fd, offset, whence, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_lseek failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}

static DECLARE_SRPC_FUNC(rpcfs_stat64, RPCFS_FUNC_STAT64_ID,
	SRPC_PROTO(const char *pathname, struct stat *st, int *ret),
	SRPC_ARGS(pathname, st, ret),
	SRPC_INPUT__entry(
		_srpc_field(uint32_t, path_pa),
		_srpc_field(uint64_t, st_dev),
		_srpc_field(uint64_t, st_ino),
		_srpc_field(unsigned int, st_mode),
		_srpc_field(unsigned int, st_nlink),
		_srpc_field(unsigned int, st_uid),
		_srpc_field(unsigned int, st_gid),
		_srpc_field(uint64_t, st_rdev),
		_srpc_field(int64_t, st_size),
		_srpc_field(int, st_blksize),
		_srpc_field(uint64_t, st_blocks),
		_srpc_field(unsigned int, st_time[6]), /* atime, atine_nsec, mtime, mtime_nesc, ctime, ctime_nsec */
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->path_pa = local_va2pa((void *)pathname);
		rpcfs_dbg("rpcfs_stat64: %s\n", pathname);
	),
	SRPC_OUTPUT(
		st->st_dev = __entry->st_dev;
		st->st_ino = __entry->st_ino;
		st->st_mode = __entry->st_mode;
		st->st_nlink = __entry->st_nlink;
		st->st_uid = __entry->st_uid;
		st->st_gid = __entry->st_gid;
		st->st_rdev = __entry->st_rdev;
		st->st_size = __entry->st_size;
		st->st_blksize = __entry->st_blksize;
		st->st_blocks = __entry->st_blocks;
		st->st_atim.tv_sec = __entry->st_time[0];
		st->st_atim.tv_nsec = __entry->st_time[1];
		st->st_mtim.tv_sec = __entry->st_time[2];
		st->st_mtim.tv_nsec = __entry->st_time[3];
		st->st_ctim.tv_sec = __entry->st_time[4];
		st->st_ctim.tv_nsec = __entry->st_time[5];
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_stat64: ret = %d\n", *ret);
	)
);

int rpcfs_stat(const char *pathname, struct stat *st)
{
	int ret = 0, rpc_ret;
	char str_buf[RPCFS_MAX_PATH_LEN] __attribute__ ((aligned (RPCFS_ARCH_CACHELINE)));

	memcpy(str_buf, pathname, strlen(pathname) > RPCFS_MAX_PATH_LEN - 1 ? RPCFS_MAX_PATH_LEN - 1 : strlen(pathname) + 1);
	str_buf[RPCFS_MAX_PATH_LEN - 1] = '\0';
	rpcfs_cache_clean(str_buf, RPCFS_MAX_PATH_LEN);

	rpc_ret = srpc_rpcfs_stat64(str_buf, st, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_stat64 failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}

static DECLARE_SRPC_FUNC(rpcfs_fstat64, RPCFS_FUNC_FSTAT64_ID,
	SRPC_PROTO(unsigned int fd, struct stat *st, int *ret),
	SRPC_ARGS(fd, st, ret),
	SRPC_INPUT__entry(
		_srpc_field(unsigned int, fd),
		_srpc_field(uint64_t, st_dev),
		_srpc_field(uint64_t, st_ino),
		_srpc_field(unsigned int, st_mode),
		_srpc_field(unsigned int, st_nlink),
		_srpc_field(unsigned int, st_uid),
		_srpc_field(unsigned int, st_gid),
		_srpc_field(uint64_t, st_rdev),
		_srpc_field(int64_t, st_size),
		_srpc_field(int, st_blksize),
		_srpc_field(uint64_t, st_blocks),
		_srpc_field(unsigned int, st_time[6]), /* atime, atine_nsec, mtime, mtime_nesc, ctime, ctime_nsec */
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->fd = fd;
		rpcfs_dbg("rpcfs_fstat64: fd=%d\n", fd);
	),
	SRPC_OUTPUT(
		st->st_dev = __entry->st_dev;
		st->st_ino = __entry->st_ino;
		st->st_mode = __entry->st_mode;
		st->st_nlink = __entry->st_nlink;
		st->st_uid = __entry->st_uid;
		st->st_gid = __entry->st_gid;
		st->st_rdev = __entry->st_rdev;
		st->st_size = __entry->st_size;
		st->st_blksize = __entry->st_blksize;
		st->st_blocks = __entry->st_blocks;
		st->st_atim.tv_sec = __entry->st_time[0];
		st->st_atim.tv_nsec = __entry->st_time[1];
		st->st_mtim.tv_sec = __entry->st_time[2];
		st->st_mtim.tv_nsec = __entry->st_time[3];
		st->st_ctim.tv_sec = __entry->st_time[4];
		st->st_ctim.tv_nsec = __entry->st_time[5];
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_fstat64: ret = %d\n", *ret);
	)
);

int rpcfs_fstat(int fd, struct stat *st)
{
	int ret = 0, rpc_ret;

	rpc_ret = srpc_rpcfs_fstat64(fd, st, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: rpcfs_fstat64 failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}
