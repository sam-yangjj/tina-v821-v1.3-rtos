#ifndef _AW_BYTEORDER_SWAB_H
#define _AW_BYTEORDER_SWAB_H

#define __swab16(x) __builtin_bswap16(x)
#define __swab32(x) __builtin_bswap32(x)
#define __swab64(x) __builtin_bswap64(x)


#define ___constant_swab16(x) __swab16(x)
#define ___constant_swab32(x) __swab32(x)
#define ___constant_swab64(x) __swab64(x)

#define swab16 __swab16
#define swab32 __swab32
#define swab64 __swab64

static inline uint16_t __swab16p(const uint16_t *p)
{
	return __swab16(*p);
}

static inline uint32_t __swab32p(const uint32_t *p)
{
	return __swab32(*p);
}

static inline uint64_t __swab64p(const uint64_t *p)
{
	return __swab64(*p);
}

static inline void __swab16s(uint16_t *p)
{
	*p = __swab16p(p);
}

static inline void __swab32s(uint32_t *p)
{
	*p = __swab32p(p);
}

static inline void __swab64s(uint64_t *p)
{
	*p = __swab64p(p);
}

#define swab16p __swab16p
#define swab32p __swab32p
#define swab64p __swab64p
#define swab16s __swab16s
#define swab32s __swab32s
#define swab64s __swab64s

#endif /* _AW_BYTEORDER_SWAB_H */
