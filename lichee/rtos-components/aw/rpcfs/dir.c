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

//struct linux_dirent {
//	unsigned long  d_ino;     /* Inode number */
//	unsigned long  d_off;     /* Offset to next linux_dirent */
//	unsigned short d_reclen;  /* Length of this linux_dirent */
//	char           d_type;
//	char           d_name[];  /* Filename (null-terminated) */
//};
static int linux_dirent_d_off_ofs;
static int linux_dirent_d_reclen_ofs;
static int linux_dirent_type_ofs;
static int linux_dirent_name_ofs;

struct rpcfs_dir {
	DIR dir;
	struct dirent e; /* use for readdir */
	int fd;
	unsigned int filepos;
	unsigned int size;
	unsigned int allocation;
	unsigned int offset;
	uint8_t *data;
};

static inline int __rpcfs_dirent_to_local(void *remote, struct dirent *local)
{
	unsigned short d_reclen;

	local->d_ino = (int)*((uint32_t *)(remote + 0));
	d_reclen = *((unsigned short *)(remote + linux_dirent_d_reclen_ofs));
	local->d_type = *((char *)(remote + linux_dirent_type_ofs));

	if ((d_reclen - linux_dirent_name_ofs) >= sizeof(local->d_name))
		memcpy(local->d_name, remote + linux_dirent_name_ofs, 256);
	else
		memcpy(local->d_name, remote + linux_dirent_name_ofs, (int)d_reclen - linux_dirent_name_ofs);

	return d_reclen;
}

static DECLARE_SRPC_FUNC(rpcfs_getdentsofs, RPCFS_FUNC_GETDENTOFS_ID,
	SRPC_PROTO(),
	SRPC_ARGS(),
	SRPC_INPUT__entry(
		_srpc_field(int, reclen_ofs),
		_srpc_field(int, type_ofs),
		_srpc_field(int, name_ofs)
	),
	SRPC_INPUT(
		__entry->reclen_ofs = 0;
		__entry->type_ofs = 0;
		__entry->name_ofs = 0;
		rpcfs_dbg("rpcfs_getdentsofs\n");
	),
	SRPC_OUTPUT(
		linux_dirent_d_off_ofs    = __entry->reclen_ofs / 2;
		linux_dirent_d_reclen_ofs = __entry->reclen_ofs;
		linux_dirent_type_ofs = __entry->type_ofs;
		linux_dirent_name_ofs = __entry->name_ofs;
		rpcfs_dbg("linux_dirent_d_off_ofs: %d\n", linux_dirent_d_off_ofs);
		rpcfs_dbg("linux_dirent_d_reclen_ofs: %d\n", linux_dirent_d_reclen_ofs);
		rpcfs_dbg("linux_dirent_type_ofs: %d\n", linux_dirent_type_ofs);
		rpcfs_dbg("linux_dirent_name_ofs: %d\n", linux_dirent_name_ofs);
	)
);

static DECLARE_SRPC_FUNC(rpcfs_getdents64, RPCFS_FUNC_GETDENTS_ID,
	SRPC_PROTO(int fd, void *buf, uint32_t count, int *ret),
	SRPC_ARGS(pathname, st, ret),
	SRPC_INPUT__entry(
		_srpc_field(int, fd),
		_srpc_field(uint32_t, buf_pa),
		_srpc_field(uint32_t, buf_size),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->fd = fd;
		__entry->buf_size = count;
		__entry->buf_pa = local_va2pa((void *)buf);

		rpcfs_dbg("rpcfs_getdents64: fd = %d\n", fd);
	),
	SRPC_OUTPUT(
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_getdents64: ret = %d\n", *ret);
	)
);

static int rpcfs_getdents64(int fd, void *buf, uint32_t count)
{
	int ret = 0, rpc_ret;

	rpcfs_cache_clean(buf, count);

	rpc_ret = srpc_rpcfs_getdents64(fd, buf, count, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_statfs64 failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	rpcfs_cache_inval(buf, count);

	return ret;
}

DIR *rpcfs_opendir(const char *path)
{
	int fd;
	struct stat st = { 0 };
	enum { allocation_size = 16384 };
	struct rpcfs_dir *dirp = NULL;
	static const uint32_t opendir_oflags = RPCFS_O_RDONLY | \
										   RPCFS_O_NDELAY | \
										   RPCFS_O_DIRECTORY | \
										   RPCFS_O_LARGEFILE | \
										   RPCFS_O_CLOEXEC;

	if (!linux_dirent_d_off_ofs)
		srpc_rpcfs_getdentsofs();

	fd = rpcfs_open(path, opendir_oflags, 0);
	if (fd < 0) {
		rpcfs_err("open: %s failed, ret=%d\n", path, fd);
		return NULL;
	}

	rpcfs_fstat(fd, &st);
	if (!S_ISDIR(st.st_mode)) {
		rpcfs_err("%s is not a dir\n", path);
		goto close_fd;
	}

	dirp = (typeof(dirp))rpcfs_malloc(sizeof(*dirp));
	if (!dirp) {
		rpcfs_err("alloc DIR for %s failed\n", path);
		goto close_fd;
	}
	memset(dirp, 0, sizeof(*dirp));

	dirp->data = (typeof(dirp->data))rpcfs_malloc_align(allocation_size);
	if (!dirp) {
		rpcfs_err("alloc DIR for %s failed\n", path);
		goto free_dirp;
	}

	rpcfs_dbg("rpcfs_opendir: %s fd = %d\n", path, fd);

	dirp->fd = fd;
	dirp->allocation = allocation_size;

	return (DIR *)dirp;

free_dirp:
	rpcfs_free(dirp);
close_fd:
	rpcfs_close(fd);
	return NULL;
}

int rpcfs_closedir(DIR *pdir)
{
	struct rpcfs_dir *dirp = (struct rpcfs_dir *)pdir;
	int fd;

	fd = dirp->fd;

	rpcfs_dbg("rpcfs_closedir: fd = %d\n", fd);

	rpcfs_free_align(dirp->data);
	rpcfs_free(dirp);

	return rpcfs_close(fd);
}

struct dirent *rpcfs_readdir(DIR *pdir)
{
	struct rpcfs_dir *dirp = (struct rpcfs_dir *)pdir;
	int bytes;

	if (dirp->offset >= dirp->size) {
		bytes = rpcfs_getdents64(dirp->fd, dirp->data, dirp->allocation);
		if (bytes <= 0) {
			if (bytes < 0)
				rpcfs_err("rpcfs_getdents64 failed, ret = %d\n", bytes);
			return NULL;
		}
		dirp->size = bytes;
		dirp->offset = 0;
	}

	dirp->offset += __rpcfs_dirent_to_local(&dirp->data[dirp->offset], &dirp->e);
	dirp->filepos = *((unsigned long *)(dirp->data + linux_dirent_d_off_ofs));

	return &dirp->e;
}

void rpcfs_seekdir(DIR *pdir, long offset)
{
	struct rpcfs_dir *dirp = (struct rpcfs_dir *)pdir;

	rpcfs_lseek(dirp->fd, offset, SEEK_SET);
}

long rpcfs_telldir(DIR *pdir)
{
	struct rpcfs_dir *dirp = (struct rpcfs_dir *)pdir;

	return dirp->filepos;
}

static DECLARE_SRPC_FUNC(rpcfs_mkdir, RPCFS_FUNC_MKDIR_ID,
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
		rpcfs_dbg("rpcfs_mkdir: %s mode: %08x\n", pathname, __entry->mode);
	),
	SRPC_OUTPUT(
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_mkdir: ret = %d\n", *ret);
	)
);

int rpcfs_mkdir(const char *pathname, mode_t mode)
{
	int ret = 0, rpc_ret;
	char str_buf[RPCFS_MAX_PATH_LEN] __attribute__ ((aligned (RPCFS_ARCH_CACHELINE)));

	memcpy(str_buf, pathname, strlen(pathname) > RPCFS_MAX_PATH_LEN - 1 ? RPCFS_MAX_PATH_LEN - 1 : strlen(pathname) + 1);
	str_buf[RPCFS_MAX_PATH_LEN - 1] = '\0';
	rpcfs_cache_clean(str_buf, RPCFS_MAX_PATH_LEN);

	rpc_ret = srpc_rpcfs_mkdir(str_buf, mode, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_statfs64 failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}

static DECLARE_SRPC_FUNC(rpcfs_rmdir, RPCFS_FUNC_RMDIR_ID,
	SRPC_PROTO(const char *pathname, int *ret),
	SRPC_ARGS(pathname, ret),
	SRPC_INPUT__entry(
		_srpc_field(uint32_t, path_pa),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->path_pa = local_va2pa((void *)pathname);
		rpcfs_dbg("rpcfs_rmdir: %s\n", pathname);
	),
	SRPC_OUTPUT(
		*ret = __entry->ret;
		rpcfs_dbg("rpcfs_rmdir: ret = %d\n", *ret);
	)
);

int rpcfs_rmdir(const char *pathname)
{
	int ret = 0, rpc_ret;
	char str_buf[RPCFS_MAX_PATH_LEN] __attribute__ ((aligned (RPCFS_ARCH_CACHELINE)));

	memcpy(str_buf, pathname, strlen(pathname) > RPCFS_MAX_PATH_LEN - 1 ? RPCFS_MAX_PATH_LEN - 1 : strlen(pathname) + 1);
	str_buf[RPCFS_MAX_PATH_LEN - 1] = '\0';
	rpcfs_cache_clean(str_buf, RPCFS_MAX_PATH_LEN);

	rpc_ret = srpc_rpcfs_rmdir(str_buf, &ret);
	if (rpc_ret < 0) {
		rpcfs_err("rpc call: srpc_rpcfs_rmdir failed, ret = %d!", rpc_ret);
		return rpc_ret;
	}

	return ret;
}
