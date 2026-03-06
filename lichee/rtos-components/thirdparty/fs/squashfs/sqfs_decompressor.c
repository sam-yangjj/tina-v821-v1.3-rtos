// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Bootlin
 *
 * Author: Joao Marcos Costa <joaomarcos.costa@bootlin.com>
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <gunzip.h>
#include <bunzip2.h>
#include <unlzma.h>
#include <unxz.h>
#include <unlz4.h>
#include <unlzo.h>

#include "sqfs_decompressor.h"
#include "sqfs_utils.h"

int sqfs_decompressor_init(struct squashfs_ctxt *ctxt)
{
	u16 comp_type = get_unaligned_le16(&ctxt->sblk->compression);

	switch (comp_type) {
#ifdef CONFIG_COMPONENTS_COMPRESS_LZO
	case SQFS_COMP_LZO:
		break;
#endif
	default:
		printf("Error: unknown compression type %d.\n", comp_type);
		return -EINVAL;
	}

	return 0;
}

void sqfs_decompressor_cleanup(struct squashfs_ctxt *ctxt)
{
	u16 comp_type = get_unaligned_le16(&ctxt->sblk->compression);

	switch (comp_type) {
#ifdef CONFIG_COMPONENTS_COMPRESS_LZO
	case SQFS_COMP_LZO:
		break;
#endif
	}
}

int sqfs_decompress(struct squashfs_ctxt *ctxt, void *dest,
		    unsigned long *dest_len, void *source, u32 src_len)
{
	u16 comp_type = get_unaligned_le16(&ctxt->sblk->compression);
	int ret = 0;

	switch (comp_type) {
#ifdef CONFIG_COMPONENTS_COMPRESS_LZO
	case SQFS_COMP_LZO: {
		size_t lzo_dest_len = *dest_len;
		ret = lzo1x_decompress_safe(source, src_len, dest, &lzo_dest_len);
		if (ret) {
			printf("LZO decompression failed. Error code: %d\n", ret);
			return -EINVAL;
		}

		break;
	}
#endif
	default:
		printf("Error: unknown compression type.\n");
		return -EINVAL;
	}

	return ret;
}
