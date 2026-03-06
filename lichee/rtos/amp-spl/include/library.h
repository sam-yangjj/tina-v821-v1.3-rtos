#ifndef __LIBRARY_H__
#define __LIBRARY_H__

/* basic string functions */
extern size_t strlen(const char *s);
extern size_t strnlen(const char *s, size_t n);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern char *strcat(char *dest, const char *src);
extern char *strncat(char *dest, const char *src, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);

/* basic mem functions */
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memmove(void *dest, const void *src, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);
extern void *memchr(const void *s, int c, size_t n);
extern void *memset(void *d, int c, size_t n);

/* integer to string */
extern char *itoa(int value, char *string, int radix);
extern char *utoa(unsigned int value, char *string, int radix);
void hexdump(char *name, char *base, int len);

extern unsigned int hstr2int(const char *str, unsigned int len);
extern unsigned int dstr2int(const char *str, unsigned int len);

extern int pr_emerg(const char *fmt, ...);
extern int printf(const char *fmt, ...);
extern int puts(const char *s);
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, size_t size, const char *fmt, ...);
#define printk(...)    pr_emerg(__VA_ARGS__)

#endif
