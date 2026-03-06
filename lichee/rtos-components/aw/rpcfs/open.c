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

static DECLARE_SRPC_FUNC(rpcfs_open, RPCFS_FUNC_OPEN_ID,
	SRPC_PROTO(const char *path, int flags, uint32_t mode, int *ret),
	SRPC_ARGS(path, flags, mode, ret),
	SRPC_INPUT__entry(
		_srpc_field(unsigned long, path_pa),
		_srpc_field(int , flags),
		_srpc_field(uint32_t, mode),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->path_pa = local_va2pa((void *)path);
		__entry->flags = flags;
		__entry->mode = mode;
		__entry->ret = 0;
		rpcfs_dbg("rpcfs_open: %s, flags:0x%x\n", path, flags);
	),
	SRPC_OUTPUT(
		*ret = __entry->ret;
		rpcfs_dbg( "rpcfs_open: ret = %d\n", *ret);
	)
);

int rpcfs_open(const char *path, int flags, int mode)
{
	int fd = 0;
	int rpc_ret;
	char str_buf[RPCFS_MAX_PATH_LEN] __attribute__ ((aligned (RPCFS_ARCH_CACHELINE)));

	memcpy(str_buf, path, strlen(path) > RPCFS_MAX_PATH_LEN - 1 ? RPCFS_MAX_PATH_LEN - 1 : strlen(path) + 1);
	str_buf[RPCFS_MAX_PATH_LEN - 1] = '\0';
	rpcfs_cache_clean(str_buf, RPCFS_MAX_PATH_LEN);
	rpc_ret = srpc_rpcfs_open(str_buf, flags, mode, &fd);
	if (rpc_ret < 0) {
		printf("rpc call: rpcfs_open failed, ret = %d!", rpc_ret);
		return -1;
	}
	return fd;
}
