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

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(n, a) (((n)) & ~((a) - 1))
#endif
#ifndef ALIGN
#define ALIGN(n, a) (((n) + (a) - 1) & ~((a) - 1))
#endif

static DECLARE_SRPC_FUNC(rpcfs_rw, RPCFS_FUNC_RW_ID,
	SRPC_PROTO(int fd, void *buf, uint32_t count, int is_write, int *ret),
	SRPC_ARGS(fd, buf, count, is_write, ret),
	SRPC_INPUT__entry(
		_srpc_field(int, fd),
		_srpc_field(uint32_t, buf_pa),
		_srpc_field(uint32_t, count),
		_srpc_field(int, is_write),
		_srpc_field(int, ret)
	),
	SRPC_INPUT(
		__entry->fd = fd;
		__entry->buf_pa = (uint32_t)local_va2pa(buf);
		__entry->count = count;
		__entry->is_write = is_write;
		__entry->ret = 0;
		rpcfs_dbg("rpcfs_rw: %d -> %px+0x%x [%s]\n", fd, buf, count, is_write ? "write" : "read");
	),
	SRPC_OUTPUT(
		*ret = __entry->ret;
		rpcfs_dbg( "rpcfs_rw: %d ret = %d\n", fd, *ret);
	)
);

ssize_t rpcfs_read(int fd, void *buf, size_t count)
{
	int rpc_ret, ret = 0;
	void *buf_aligned, *buf_end_aligned;
	void *tmp_buf;
	size_t bytes, ofs;

	rpcfs_dbg("fd=%d, buf:%p, count:%d\n", fd, buf, count);
	buf_aligned = (void *)ALIGN_DOWN((unsigned long)buf, RPCFS_ARCH_CACHELINE);
	buf_end_aligned = (void *)ALIGN_DOWN((unsigned long)(buf + count), RPCFS_ARCH_CACHELINE);

	if (buf == buf_aligned && (buf + count) == buf_end_aligned)
		goto direct_read;

	ofs = 0;
	if (buf == buf_aligned) {
		bytes = (unsigned long)buf_end_aligned - (unsigned long)buf_aligned;
		rpcfs_cache_clean(buf, bytes);
		rpc_ret = srpc_rpcfs_rw(fd, (void *)buf, bytes, 0, &ret);
		if (rpc_ret < 0) {
			printf("rpc call: rpcfs_read failed, ret = %d!", rpc_ret);
			return -1;
		}
		rpcfs_cache_inval(buf, bytes);
		ofs += ret;
		rpcfs_dbg("ofs:%d, count:%d, rpc_ret:%d, ret:%d\n", ofs, count, rpc_ret, ret);
	}

	tmp_buf = rpcfs_malloc_align(RPCFS_CACHE_BUF_SIZE);
	if (!tmp_buf) {
		printf("rpc call: rpcfs_read Out of memory!");
		return -ENOMEM;
	}

	while (ofs < count) {
		bytes = count - ofs > RPCFS_CACHE_BUF_SIZE ? RPCFS_CACHE_BUF_SIZE : count - ofs;
		rpcfs_cache_clean(tmp_buf, bytes);
		rpc_ret = srpc_rpcfs_rw(fd, (void *)tmp_buf, bytes, 0, &ret);
		if (rpc_ret < 0) {
			printf("rpc call: rpcfs_read failed, ret = %d!", rpc_ret);
			rpcfs_free_align(tmp_buf);
			return -1;
		}
		if (!ret) {
			rpcfs_dbg("fd:%d, EOF\n", fd);
			break;
		}
		rpcfs_cache_inval(tmp_buf, bytes);
		memcpy(buf + ofs, tmp_buf, bytes);
		ofs += ret;
		rpcfs_dbg("ofs:%d, count:%d, bytes:%d, rpc_ret:%d, ret:%d\n",
			ofs, count, bytes, rpc_ret, ret);
	}
	count = ofs;

	rpcfs_free_align(tmp_buf);

	rpcfs_dbg("count:%d\n", count);
	return count;

direct_read:
	rpcfs_cache_clean(buf, count);
	rpc_ret = srpc_rpcfs_rw(fd, (void *)buf, count, 0, &ret);
	if (rpc_ret < 0) {
		printf("rpc call: rpcfs_write failed, ret = %d!", rpc_ret);
		return -1;
	}
	rpcfs_cache_inval(buf, count);
	rpcfs_dbg("buf:%p, count:%d, rpc_ret:%d, ret:%d\n", buf, count, rpc_ret, ret);

	return ret;

}

ssize_t rpcfs_write(int fd, const void *buf, size_t count)
{
	int rpc_ret, ret = 0;
	void *buf_aligned, *buf_end_aligned;
	void *tmp_buf;
	size_t bytes, ofs;

	buf_aligned = (void *)ALIGN_DOWN((unsigned long)buf, RPCFS_ARCH_CACHELINE);
	buf_end_aligned = (void *)ALIGN_DOWN((unsigned long)(buf + count), RPCFS_ARCH_CACHELINE);

	if (buf == buf_aligned && (buf + count) == buf_end_aligned) 
		goto direct_write;

	ofs = 0;
	if (buf == buf_aligned) {
		bytes = (unsigned long)buf_end_aligned - (unsigned long)buf_aligned;
		rpcfs_cache_clean(buf, bytes);
		rpc_ret = srpc_rpcfs_rw(fd, (void *)buf, bytes, 1, &ret);
		if (rpc_ret < 0) {
			printf("rpc call: rpcfs_write failed, ret = %d!", rpc_ret);
			return -1;
		}
		ofs += ret;
	}

	tmp_buf = rpcfs_malloc_align(RPCFS_CACHE_BUF_SIZE);
	if (!tmp_buf) {
		printf("rpc call: rpcfs_write Out of memory!");
		return -ENOMEM;
	}

	while (ofs < count) {
		bytes = count - ofs > RPCFS_CACHE_BUF_SIZE ? RPCFS_CACHE_BUF_SIZE : count - ofs;
		memcpy(tmp_buf, buf + ofs, bytes);
		rpcfs_cache_clean(tmp_buf, bytes);
		rpc_ret = srpc_rpcfs_rw(fd, (void *)tmp_buf, bytes, 1, &ret);
		if (rpc_ret < 0) {
			printf("rpc call: rpcfs_write failed, ret = %d!", rpc_ret);
			rpcfs_free_align(tmp_buf);
			return -1;
		}
		if (ret <= 0)
			break;
		ofs += ret;
	}
	count = ofs;

	rpcfs_free_align(tmp_buf);

	return ret < 0 ? ret : count;

direct_write:
	rpcfs_cache_clean(buf, count);
	rpc_ret = srpc_rpcfs_rw(fd, (void *)buf, count, 1, &ret);
	if (rpc_ret < 0) {
		printf("rpc call: rpcfs_write failed, ret = %d!", rpc_ret);
		return -1;
	}

	return ret;
}
