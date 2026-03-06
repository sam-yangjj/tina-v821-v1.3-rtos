#ifndef DECOMPRESS_GUNZIP_H
#define DECOMPRESS_GUNZIP_H

int __gunzip(unsigned char *buf, long len,
	long (*fill)(void*, unsigned long),
	long (*flush)(void*, unsigned long),
	unsigned char *out_buf, long out_len,
	long *pos,
	void(*error)(char *x));

int gunzip(unsigned char *buf, long len,
	long (*fill)(void*, unsigned long),
	long (*flush)(void*, unsigned long),
	unsigned char *out_buf, size_t out_len,
	long *pos,
	void(*error)(char *x));

#endif
