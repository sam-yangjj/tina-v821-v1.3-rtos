/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#ifndef _ASM_RISCV_BARRIER_H
#define _ASM_RISCV_BARRIER_H

#ifndef __ASSEMBLY__

#define nop()           __asm__ __volatile__ ("nop")

#define RISCV_FENCE(p, s) \
    __asm__ __volatile__ ("fence " #p "," #s : : : "memory")

/* These barriers need to enforce ordering on both devices or memory. */
#define mb()            RISCV_FENCE(iorw,iorw)
#define rmb()           RISCV_FENCE(ir,ir)
#define wmb()           RISCV_FENCE(ow,ow)

/* These barriers do not need to enforce ordering on devices, just memory. */
#define __smp_mb()      RISCV_FENCE(rw,rw)
#define __smp_rmb()     RISCV_FENCE(r,r)
#define __smp_wmb()     RISCV_FENCE(w,w)

#if defined(CONFIG_ARCH_RISCV_C906)
#define dmb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
} while(0)

#define dsb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
	__asm__ __volatile__ ("sync");\
} while(0)

#define isb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
	__asm__ __volatile__ ("sync.i");\
} while(0)
#elif defined(CONFIG_ARCH_RISCV_E906) || defined(CONFIG_ARCH_RISCV_E907)
#define dmb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
} while(0)

#define dsb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
	__asm__ __volatile__ (".insn r 0xb, 0, 0, x0, x0, x24");\
} while(0)

#define isb(...)        do {\
	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory");\
	__asm__ __volatile__ (".insn r 0xb, 0, 0, x0, x0, x26");\
} while(0)
#endif


#define soft_break(...) do { __asm__ __volatile__("ebreak" ::: "memory", "cc"); } while(0)
#endif

#endif
