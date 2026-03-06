/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 Bootlin
 *
 * Author: Joao Marcos Costa <joaomarcos.costa@bootlin.com>
 */

#ifndef SQFS_UTILS_H
#define SQFS_UTILS_H

#include <stdbool.h>
#include <compiler.h>
#include <byteorder.h>
#include <hal_mem.h>
#include <string.h>

#define SQFS_FRAGMENT_INDEX_OFFSET(A) ((A) % SQFS_MAX_ENTRIES)
#define SQFS_FRAGMENT_INDEX(A) ((A) / SQFS_MAX_ENTRIES)
#define SQFS_BLOCK_SIZE(A) ((A) & GENMASK(23, 0))
#define SQFS_CHECK_FLAG(flag, bit) (((flag) >> (bit)) & 1)
/* Useful for both fragment and data blocks */
#define SQFS_COMPRESSED_BLOCK(A) (!((A) & BIT(24)))
/* SQFS_COMPRESSED_DATA strictly used with super block's 'flags' member */
#define SQFS_COMPRESSED_DATA(A) (!((A) & 0x0002))
#define SQFS_IS_FRAGMENTED(A) ((A) != 0xFFFFFFFF)
/*
 * These two macros work as getters for a metada block header, retrieving the
 * data size and if it is compressed/uncompressed
 */
#ifndef GENMASK
#define GENMASK(h, l) (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (CONFIG_BITS_PER_LONG - 1 - (h))))
#endif

#define SQFS_COMPRESSED_METADATA(A) (!((A) & BIT(15)))
#define SQFS_METADATA_SIZE(A) ((A) & GENMASK(14, 0))

struct squashfs_super_block_flags {
	/* check: unused
	 * uncompressed_ids: not supported
	 */
	bool uncompressed_inodes;
	bool uncompressed_data;
	bool check;
	bool uncompressed_frags;
	bool no_frags;
	bool always_frags;
	bool duplicates;
	bool exportable;
	bool uncompressed_xattrs;
	bool no_xattrs;
	bool compressor_options;
	bool uncompressed_ids;
};

static inline void *malloc_cache_aligned(u32 size)
{
	return hal_malloc_align(size, 64);
}

static inline void free_cache_aligned(void *p)
{
	hal_free_align(p);
}

static inline void *sqfs_malloc(u32 size)
{
	return hal_malloc(size);
}

static inline void *sqfs_calloc(int n, u32 size)
{
	void *ptr = hal_malloc(n * size);
	if (!ptr)
		return NULL;
	memset(ptr, 0, n * size);
	return ptr;
}

static inline void sqfs_free(void *p)
{
	return hal_free(p);
}

#ifndef min_t
#define min_t(type, x, y) ({			\
	type __min1 = (x);			\
	type __min2 = (y);			\
	__min1 < __min2 ? __min1: __min2; })
#endif

#endif /* SQFS_UTILS_H  */
