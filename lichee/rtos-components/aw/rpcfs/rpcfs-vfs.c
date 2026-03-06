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
#include <awlog.h>
#include <sunxi_hal_common.h>

static inline int rpcfs_vfs_open(const char *path, int flags, int mode)
{
	return rpcfs_open(path, rpcfs_convert_o_flags(flags), mode);
}

#define rpcfs_vfs_write				rpcfs_write
#define rpcfs_vfs_read				rpcfs_read
#define rpcfs_vfs_close				rpcfs_close
#define rpcfs_vfs_lseek				rpcfs_lseek
#define rpcfs_vfs_stat				rpcfs_stat
#define rpcfs_vfs_ftruncate			rpcfs_ftruncate
#define rpcfs_vfs_truncate			rpcfs_truncate
#define rpcfs_vfs_fstat				rpcfs_fstat
#define rpcfs_vfs_link				rpcfs_link
#define rpcfs_vfs_unlink			rpcfs_unlink
#define rpcfs_vfs_rename			rpcfs_rename
#define rpcfs_vfs_opendir			rpcfs_opendir
#define rpcfs_vfs_closedir			rpcfs_closedir
#define rpcfs_vfs_readdir			rpcfs_readdir
#define rpcfs_vfs_seekdir			rpcfs_seekdir
#define rpcfs_vfs_telldir			rpcfs_telldir
#define rpcfs_vfs_mkdir				rpcfs_mkdir
#define rpcfs_vfs_rmdir				rpcfs_rmdir
#define rpcfs_vfs_statfs			rpcfs_statfs
#define rpcfs_vfs_fstatfs			rpcfs_fstatfs
#define rpcfs_vfs_access			rpcfs_access
#define rpcfs_vfs_fsync				rpcfs_fsync

static int rpcfs_vfs_readdir_r(void *_ctx, DIR *pdir, struct dirent *entry,
        struct dirent **out_dirent)
{
    return -ENOSYS;
}

int rpcfs_register_vfs(const char *mnt_path)
{
	const vfs_t vfs = {
		.flags = VFS_FLAG_DEFAULT,
		.open = rpcfs_vfs_open,
		.write = &rpcfs_vfs_write,
		.read = &rpcfs_vfs_read,
		.close = &rpcfs_vfs_close,
		.lseek = &rpcfs_vfs_lseek,
		.fstat = &rpcfs_vfs_fstat,
		.stat = &rpcfs_vfs_stat,
		.link = &rpcfs_vfs_link,
		.unlink = &rpcfs_vfs_unlink,
		.rename = &rpcfs_vfs_rename,
		.opendir = &rpcfs_vfs_opendir,
		.closedir = &rpcfs_vfs_closedir,
		.readdir = &rpcfs_vfs_readdir,
		.readdir_r_p = &rpcfs_vfs_readdir_r,
		.seekdir = &rpcfs_vfs_seekdir,
		.telldir = &rpcfs_vfs_telldir,
		.mkdir = &rpcfs_vfs_mkdir,
		.rmdir = &rpcfs_vfs_rmdir,
		.statfs = &rpcfs_vfs_statfs,
		.fstatfs = &rpcfs_vfs_fstatfs,
		.access = &rpcfs_vfs_access,
		.fsync = &rpcfs_vfs_fsync,
		.truncate = &rpcfs_vfs_truncate,
		.ftruncate = &rpcfs_vfs_ftruncate,
	};

	return vfs_register(mnt_path, &vfs, NULL);
}

int rpcfs_unregister_vfs(const char *mnt_path)
{
    return vfs_unregister(mnt_path);
}

