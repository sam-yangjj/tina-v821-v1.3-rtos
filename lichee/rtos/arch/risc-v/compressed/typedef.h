/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
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
#ifndef __TYPEDEF__
#define __TYPEDEF__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>

#ifndef EBASE_TRUE
#define EBASE_TRUE      0
#endif

#ifndef EBASE_FALSE
#define EBASE_FALSE     (-1)
#endif

#ifndef EBSP_TRUE
#define EBSP_TRUE       EBASE_TRUE
#endif

#ifndef EBSP_FALSE
#define EBSP_FALSE      EBASE_FALSE
#endif

#ifndef EPDK_OK
#define EPDK_OK         0
#endif

#ifndef EPDK_FAIL
#define EPDK_FAIL       (-1)
#endif

#ifndef EPDK_TRUE
#define EPDK_TRUE       1
#endif

#ifndef EPDK_FALSE
#define EPDK_FALSE      0
#endif

#ifndef EPDK_DISABLED
#define EPDK_DISABLED   0
#endif

#ifndef EPDK_ENABLED
#define EPDK_ENABLED    1
#endif

#ifndef EPDK_NO
#define EPDK_NO         0
#endif

#ifndef EPDK_YES
#define EPDK_YES        1
#endif

#ifndef EPDK_OFF
#define EPDK_OFF        0
#endif

#ifndef EPDK_ON
#define EPDK_ON         1
#endif

#ifndef EPDK_CLR
#define EPDK_CLR        0
#endif

#ifndef EPDK_SET
#define EPDK_SET        1
#endif

#ifndef NULL
#define NULL            (void*)(0)
#endif

#ifndef DATA_TYPE_X___u64
#define DATA_TYPE_X___u64
typedef uint64_t    __u64;
#endif

#ifndef DATA_TYPE_X___u32
#define DATA_TYPE_X___u32
typedef uint32_t    __u32;
#endif

#ifndef DATA_TYPE_X___u16
#define DATA_TYPE_X___u16
typedef uint16_t    __u16;
#endif

#ifndef DATA_TYPE_X___u8
#define DATA_TYPE_X___u8
typedef uint8_t     __u8;
#endif

#ifndef DATA_TYPE_X___s64
#define DATA_TYPE_X___s64
typedef int64_t     __s64;
#endif

#ifndef DATA_TYPE_X___s32
#define DATA_TYPE_X___s32
typedef int32_t     __s32;
#endif

#ifndef DATA_TYPE_X___s16
#define DATA_TYPE_X___s16
typedef int16_t     __s16;
#endif

#ifndef DATA_TYPE_X___s8
#define DATA_TYPE_X___s8
typedef int8_t      __s8;
#endif

#ifndef DATA_TYPE_X_u64
#define DATA_TYPE_X_u64
typedef uint64_t    u64;
#endif

#ifndef DATA_TYPE_X_u32
#define DATA_TYPE_X_u32
typedef uint32_t    u32;
#endif

#ifndef DATA_TYPE_X_u16
#define DATA_TYPE_X_u16
typedef uint16_t    u16;
#endif

#ifndef DATA_TYPE_X_u8
#define DATA_TYPE_X_u8
typedef uint8_t     u8;
#endif

#ifndef DATA_TYPE_X_s64
#define DATA_TYPE_X_s64
typedef int64_t     s64;
#endif

#ifndef DATA_TYPE_X_s32
#define DATA_TYPE_X_s32
typedef int32_t     s32;
#endif

#ifndef DATA_TYPE_X_s16
#define DATA_TYPE_X_s16
typedef int16_t     s16;
#endif

#ifndef DATA_TYPE_X_s8
#define DATA_TYPE_X_s8
typedef int8_t      s8;
#endif


#ifndef DATA_TYPE_X___hdle
#define DATA_TYPE_X___hdle
typedef void       *__hdle;
#endif

#ifndef DATA_TYPE_X___bool
#define DATA_TYPE_X___bool
typedef signed char __bool;
#endif

#ifndef DATA_TYPE_X___size
#define DATA_TYPE_X___size
typedef unsigned int __size;
#endif

#ifndef DATA_TYPE_X___fp32
#define DATA_TYPE_X___fp32
typedef float        __fp32;
#endif

#ifndef DATA_TYPE_X___wchar_t
#define DATA_TYPE_X___wchar_t
typedef uint16_t   __wchar_t;
#endif

#ifndef DATA_TYPE_X_uint
#define DATA_TYPE_X_uint
typedef unsigned int uint;
#endif

#ifdef __cplusplus
}
#endif

#endif

