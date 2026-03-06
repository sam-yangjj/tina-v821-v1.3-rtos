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
#ifndef __AW_RPCFS_H__
#define __AW_RPCFS_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <vfs.h>
#include <aw_list.h>
#include <hal_cache.h>
#include <sys/statfs.h>
#include <sys/stat.h>

int rpcfs_open(const char *path, int flags, int mode);
ssize_t rpcfs_read(int fd, void *buf, size_t count);
ssize_t rpcfs_write(int fd, const void *buf, size_t count);
int rpcfs_close(int fd);

int rpcfs_link(const char *oldname, const char *newname);
int rpcfs_unlink(const char *pathname);
int rpcfs_rename(const char *oldname, const char *newname);
int rpcfs_statfs(const char *pathname, struct statfs *st);
int rpcfs_fstatfs(int fd, struct statfs *st);
int rpcfs_access(const char *pathname, int mode);
int rpcfs_fsync(int fd);
int rpcfs_truncate(const char *pathname, off_t len);
int rpcfs_ftruncate(int fd, off_t len);
off_t rpcfs_lseek(int fd, off_t offset, int whence);
int rpcfs_stat(const char *pathname, struct stat *st);
int rpcfs_fstat(int fd, struct stat *st);

DIR *rpcfs_opendir(const char *path);
int rpcfs_closedir(DIR *pdir);
struct dirent *rpcfs_readdir(DIR *pdir);
void rpcfs_seekdir(DIR *pdir, long offset);
long rpcfs_telldir(DIR *pdir);
int rpcfs_mkdir(const char *pathname, mode_t mode);
int rpcfs_rmdir(const char *pathname);

int rpcfs_register_vfs(const char *mnt_path);
int rpcfs_unregister_vfs(const char *mnt_path);

#endif

