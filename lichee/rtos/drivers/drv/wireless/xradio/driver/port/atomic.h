/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __WIRELESS_ATOMIC_H
#define __WIRELESS_ATOMIC_H

#include "xr_types.h"
#include "irqflags.h"

#include "sys/atomic.h"

#define ATOMIC_INIT(i)	{ (i) }

static XR_INLINE u8 __atomic_swap_u8(u8 x, volatile void *ptr)
{
    u8 ret;
    unsigned long flags;

    local_irq_save(flags);
    ret = *(volatile u8 *)ptr;
    *(volatile u8 *)ptr = x;
    local_irq_restore(flags);

    return ret;
}

static XR_INLINE u16 __atomic_swap_u16(u16 x, volatile void *ptr)
{
    u16 ret;
    unsigned long flags;

    local_irq_save(flags);
    ret = *(volatile u16 *)ptr;
    *(volatile u16 *)ptr = x;
    local_irq_restore(flags);

    return ret;
}

static XR_INLINE u32 __atomic_swap_u32(u32 x, volatile void *ptr)
{
    u32 ret;
    unsigned long flags;

    local_irq_save(flags);
    ret = *(volatile u32 *)ptr;
    *(volatile u32 *)ptr = x;
    local_irq_restore(flags);

    return ret;
}

#ifdef CONFIG_64BIT
static XR_INLINE u64 __atomic_swap_u64(u64 x, volatile void *ptr)
{
    u64 ret;
    unsigned long flags;

    local_irq_save(flags);
    ret = *(volatile u64 *)ptr;
    *(volatile u64 *)ptr = x;
    local_irq_restore(flags);

    return ret;
}
#endif /* CONFIG_64BIT */

static XR_INLINE
unsigned long __atomic_swap(unsigned long x, volatile void *ptr, int size)
{
    switch (size) {
    case 1:
        return __atomic_swap_u8((u8)x, ptr);
    case 2:
        return __atomic_swap_u16((u16)x, ptr);
    case 4:
        return __atomic_swap_u32((u32)x, ptr);
#ifdef CONFIG_64BIT
    case 8:
        return __atomic_swap_u64((u64)x, ptr);
#endif
    default:
        return x;
    }
}

#define xchg(ptr, x) \
	(__atomic_swap((unsigned long)(x), (ptr), sizeof(*(ptr))))

#define atomic_xchg(ptr, v)    (xchg(&(ptr)->counter, (v)))

#endif
