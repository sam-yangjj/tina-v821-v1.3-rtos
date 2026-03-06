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
#include <csr.h>

.global _start
.extern freertos_risc_v_trap_handler

#ifdef CONFIG_COMPONENTS_PM
.globl cpu_resume
.globl g_sleep_flag
#endif

#ifdef CONFIG_KASAN
.extern kasan_early_init
#endif

    .section ".start", "ax", @progbits
    .align  2
    .globl  _start
    .type   _start, %function
_start:
    rdtime s1
    /*
     * setup non-deep sleep mode.
     * note: need to switch to deep sleep mode
     * before entering standby/hibernation if
     * platforms using e90x as power control core
     */
    li      t0, (1 << 2)
    csrs    mexstatus, t0

#ifdef CONFIG_COMPONENTS_PM
    /* check sleep flag */
    la      a0, g_sleep_flag
    lw      a1, 0(a0)
    li      a0, 1
    bne     a1, a0, .normal
    j       cpu_resume
.normal:
#endif

    /* disable interrupt */
    li t0, MR_MIE
    csrc mstatus, t0
    csrw mie, zero

    /* enable T-Head Extension ISA */
    li t1, (1 << 22)
    csrs mxstatus, t1

#ifdef CONFIG_ARCH_SUN300IW1P1
#define RTC_DATA_REGS(n)			(0x4a000000 + 0x200 + (n) * 4)
    li t1, RTC_DATA_REGS(0)
    lw t0, 0(t1)
    andi t0, t0, ~(1 << 5)
    sw t0, 0(t1)
#endif

.option push
.option norelax
    la      gp, __global_pointer$
.option pop

    /* setup the address of exception handler */
    la      a0, freertos_risc_v_trap_handler
    ori     a0, a0, 3
    csrw    mtvec, a0

    /* setup the address of vector interrupt handler */
    la      a0, clic_interrupt_vectors
    csrw    mtvt, a0

#ifdef CONFIG_ARCH_RISCV_FPU
    # clear FS field.
    li    t0, SR_FS
    csrc  mstatus, t0

    # initial status to set the fpu registers.
    la    t0, SR_FS_INITIAL
    csrs  mstatus, t0

    # follwing change of fpu registers
    # would set sr_sd again.

    # clear the floating-point swap control and status register.
    # abandon the original value.
    fscsr   x0, x0

    # clear the GPRS of FPU.
    fmv.w.x f0, x0
    fmv.w.x f1, x0
    fmv.w.x f2, x0
    fmv.w.x f3, x0
    fmv.w.x f4, x0
    fmv.w.x f5, x0
    fmv.w.x f6, x0
    fmv.w.x f7, x0
    fmv.w.x f8, x0
    fmv.w.x f9, x0
    fmv.w.x f10,x0
    fmv.w.x f11,x0
    fmv.w.x f12,x0
    fmv.w.x f13,x0
    fmv.w.x f14,x0
    fmv.w.x f15,x0
    fmv.w.x f16,x0
    fmv.w.x f17,x0
    fmv.w.x f18,x0
    fmv.w.x f19,x0
    fmv.w.x f20,x0
    fmv.w.x f21,x0
    fmv.w.x f22,x0
    fmv.w.x f23,x0
    fmv.w.x f24,x0
    fmv.w.x f25,x0
    fmv.w.x f26,x0
    fmv.w.x f27,x0
    fmv.w.x f28,x0
    fmv.w.x f29,x0
    fmv.w.x f30,x0
    fmv.w.x f31,x0

    # clear SD temporaly. and disable interrupt.
    li    t0, SR_FS_DIRTY
    csrc  mstatus, t0

    # re-initial status to initial status.
    li    t0, SR_FS_INITIAL
    csrs  mstatus, t0
#endif

    # clear mscratch register.
    csrw   mscratch,zero

    /* Clear bss section */
    la      a0, __bss_start__
    la      a1, __bss_end__
    bgeu    a0, a1, 2f

    /* boot0 alread memset, skip and save 12ms(200K bss) */
    lb      a2, (a0)
    bnez    a2, 1f
    lb      a3, -1(a1)
    beqz    a3, 2f

1:
    sub     a2, a1, a0
    li      a1, 0
    jal     memset

2:

#ifdef CONFIG_KASAN
    la    t0, kasan_early_init
    jalr  t0
#endif

    /* setup stack pointer for C runtime environment */
    la      sp, __init_process_stack_end__

    jal     system_init_early

    mv      a0, s1
    jal     start_kernel

    .size   _start, . - _start

__exit:
    j      __exit
