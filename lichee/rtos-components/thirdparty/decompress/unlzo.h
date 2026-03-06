#ifndef DECOMPRESS_UNLZO_H
#define DECOMPRESS_UNLZO_H

int unlzo(unsigned char *in, long in_size,
		long (*fill)(void *dest, unsigned long size),
		long (*flush)(void *src, unsigned long size),
		unsigned char *out, size_t out_len, long *in_used,
		void (*error)(char *x));

int lzo1x_decompress_safe(const unsigned char *src, size_t src_len,
			  unsigned char *dst, size_t *dst_len);

#endif
