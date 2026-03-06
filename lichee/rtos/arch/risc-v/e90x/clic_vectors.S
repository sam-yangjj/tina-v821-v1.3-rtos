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

#include "platform/platform_irq.h"
#include "platform/platform.h"

#ifdef CONFIG_ARCH_RISCV_FPU
#include <fpu.S>
#endif

#if __riscv_xlen == 64
	#define portWORD_SIZE 8
	#define store_x sd
	#define load_x ld
#elif __riscv_xlen == 32
	#define store_x sw
	#define load_x lw
	#define portWORD_SIZE 4
#else
	#error The toolchain did not define __riscv_xlen
#endif

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
#define CLIC_MINTTHRESH_REG_OFF (0x08)
#define CLIC_MINTTHRESH_REG (NEST_PLAT_ROOT_IC_REG_BASE_ADDR + CLIC_MINTTHRESH_REG_OFF)
#endif

#if defined(CONFIG_ARCH_RISCV_FPU) && !defined(CONFIG_SAVE_RISCV_FPU_CONTEXT_IN_TCB) && !defined(CONFIG_ARCH_RISCV_INTERRUPT_NEST)
#define GENERAL_CONTEXT_SIZE (WORD_SUM * portWORD_SIZE)
#define portCONTEXT_SIZE (GENERAL_CONTEXT_SIZE + FPU_CONTEXT_SIZE)
#else
#define portCONTEXT_SIZE (WORD_SUM * portWORD_SIZE)
#endif

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
#ifdef CONFIG_ARCH_RISCV_FPU
#define INTERRUPT_LEVEL_SUM	(0x1 << CLICINTCTL_NLBITS)
/* The value of fpu_use_rec_flag is placed at the end */
#define FPU_SAVE_PROTSIZE	(FPU_CONTEXT_SIZE + portWORD_SIZE)
#define FPU_SAVE_NEST_SIZE 	(INTERRUPT_LEVEL_SUM * FPU_SAVE_PROTSIZE)

.section .bss
	.balign 16
	.global fpu_save_nest
fpu_save_nest:
	.space FPU_SAVE_NEST_SIZE

/* The maxmum priority number of e90x is 32.
 * Dirty value of MSTATUS.FS will not be overwritten before and after nesting.
 * The bit corresponding to the nest_cnt is 1 indicates this nest layer uses FPU.
 * if (fpu_use_rec_flag EXIT > fpu_use_rec_flag ENTRY), means FPU has been used after interrupt ENTRY.
 */
.section .bss
	.balign 16
	.global fpu_use_rec_flag
fpu_use_rec_flag:
	.space 4
#endif /* CONFIG_ARCH_RISCV_FPU */

.section .bss
	.balign 16
	.global interrupt_nest_cnt
interrupt_nest_cnt:
	.space 4

.section .bss
	.balign 16
	.global clic_thresh_save
clic_thresh_save:
	.space 4

.section .bss
	.balign 16
	.global mcause_save
mcause_save:
	.space portWORD_SIZE

.extern __freertos_irq_stack_bottom
#ifdef CONFIG_IRQNEST_OSTEST_GENERAL_CTX_CMP
.extern g_irqnest_store_addr
.extern irqnest_ostest_ctx_cmp_entry
.extern irqnest_ostest_ctx_cmp_exit
#endif
#endif /* CONFIG_ARCH_RISCV_INTERRUPT_NEST */

.section .vectors, "aw", @progbits
	.balign 64
	.globl  clic_interrupt_vectors
	.type   clic_interrupt_vectors, @object
	clic_interrupt_vectors:
	.rept   (PLAT_CLIC_IRQ_CNT)
	.long   clic_interrupt_handler
	.endr

.text

	.global clic_interrupt_handler
	.type   clic_interrupt_handler, %function
clic_interrupt_handler:
	addi sp, sp, -portCONTEXT_SIZE
	store_x x1,  1  * portWORD_SIZE( sp )
	store_x x5,  2  * portWORD_SIZE( sp )
	store_x x6,  3  * portWORD_SIZE( sp )
	store_x x7,  4  * portWORD_SIZE( sp )
	store_x x8,  5  * portWORD_SIZE( sp )
	store_x x9,  6  * portWORD_SIZE( sp )
	store_x x10, 7  * portWORD_SIZE( sp )
	store_x x11, 8  * portWORD_SIZE( sp )
	store_x x12, 9  * portWORD_SIZE( sp )
	store_x x13, 10 * portWORD_SIZE( sp )
	store_x x14, 11 * portWORD_SIZE( sp )
	store_x x15, 12 * portWORD_SIZE( sp )
	store_x x16, 13 * portWORD_SIZE( sp )
	store_x x17, 14 * portWORD_SIZE( sp )
	store_x x18, 15 * portWORD_SIZE( sp )
	store_x x19, 16 * portWORD_SIZE( sp )
	store_x x20, 17 * portWORD_SIZE( sp )
	store_x x21, 18 * portWORD_SIZE( sp )
	store_x x22, 19 * portWORD_SIZE( sp )
	store_x x23, 20 * portWORD_SIZE( sp )
	store_x x24, 21 * portWORD_SIZE( sp )
	store_x x25, 22 * portWORD_SIZE( sp )
	store_x x26, 23 * portWORD_SIZE( sp )
	store_x x27, 24 * portWORD_SIZE( sp )
	store_x x28, 25 * portWORD_SIZE( sp )
	store_x x29, 26 * portWORD_SIZE( sp )
	store_x x30, 27 * portWORD_SIZE( sp )
	store_x x31, 28 * portWORD_SIZE( sp )
	store_x x3,  31 * portWORD_SIZE( sp )
	store_x x4,  32 * portWORD_SIZE( sp )

	addi a0, sp, portCONTEXT_SIZE
	store_x a0,  30 * portWORD_SIZE( sp )

	csrr a0, mepc
	store_x a0,  0 * portWORD_SIZE( sp )
	csrr t0, mscratch
	store_x t0, 33 * portWORD_SIZE( sp )

	csrr t0, mstatus
	store_x t0, 29 * portWORD_SIZE( sp )

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	load_x a0, interrupt_nest_cnt
	bnez a0, nest_save_to_sp
	/* store thresh and mcause to label if nest_cnt is 0 */
	la t0, mcause_save
	csrr t1, mcause
	store_x t1, 0(t0)

	la t0, clic_thresh_save
	li t1, CLIC_MINTTHRESH_REG
	lw t2, 0(t1)
	sw t2, 0(t0)
	j nest_save_end
nest_save_to_sp:
	/* There is no reserved space in thread stack,
	 * store mcause and clic t0 sp when nest_cnt is not zero.
	 */
	csrr t0, mcause
	store_x t0, 34 * portWORD_SIZE( sp )

	li t0, CLIC_MINTTHRESH_REG
	lw t1, 0(t0)
	sw t1, 35 * portWORD_SIZE( sp )
#ifdef CONFIG_IRQNEST_OSTEST_GENERAL_CTX_CMP
	/* use irqstack if nest_cnt > 0 */
	store_x sp, g_irqnest_store_addr, t0
	mv a1, a0	/* a0 is nest_cnt here */
	csrr a0, mcause
	andi a0, a0, 0x7FF
	call irqnest_ostest_ctx_cmp_entry
#endif
nest_save_end:
#endif

#ifdef CONFIG_ARCH_RISCV_FPU
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	load_x s3, interrupt_nest_cnt
	beqz s3, nest_fpu_save_act

	li t0, FPU_SAVE_PROTSIZE
	mul t0, t0, s3
	la t1, fpu_save_nest
	add s2, t1, t0
	/* store the fpu_use flag */
	li t0, 31
	sub t0, t0, s3
	li t1, 0x7fffffff
	srl t1, t1, t0
	lw t0, fpu_use_rec_flag
	and t1, t1, t0
	sw t1, FPU_CONTEXT_SIZE(s2)

nest_fpu_save_act:
	SAVE_FPU_CONTEXT_WITH_SKIP pxCurrentTCB,sp,29 * portWORD_SIZE,s0,s1,t0,t1,s2,s3
#else
#ifdef CONFIG_SAVE_RISCV_FPU_CONTEXT_IN_TCB
	SAVE_FPU_CONTEXT_WITH_SKIP pxCurrentTCB,sp,29 * portWORD_SIZE,s0,s1,t0,t1
#else
	addi a0, sp, GENERAL_CONTEXT_SIZE
	SAVE_FPU_CONTEXT a0 t0
#endif
#endif /* CONFIG_ARCH_RISCV_INTERRUPT_NEST */
#endif /* CONFIG_ARCH_RISCV_FPU */

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	/* nest_cnt++ */
	load_x a0, interrupt_nest_cnt
	addi a0, a0, 1
	store_x a0, interrupt_nest_cnt, t0
	addi a0, a0, -1
	bnez a0, enter_sp_switch_end
#endif
	lw      t0, pxCurrentTCB		/* Load pxCurrentTCB. */
	sw      sp, 0( t0 )				/* Write sp to first TCB member. */

	load_x sp, xISRStackTop
enter_sp_switch_end:
	call enter_interrupt_handler

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	/* Change interrupt thresh to current interrupt level */
	li t1, CLIC_MINTTHRESH_REG
	csrr t0, mintstatus
	sw t0, 0(t1)
	/* Enable global interrupt */
	li t0, 8
	csrrs t0, mstatus, t0
#endif
	csrr a0, mcause
	andi a0, a0, 0x7FF           /* If the CLIC support more than 2048 interrupts, we need to use the mask value saved in register */
	call irq_core_handle_root_ic_irq

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	/* Disable global interrupt */
	li t0, 8
	csrrc t0, mstatus, t0
#endif
	call exit_interrupt_handler
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	/* nest_cnt-- */
	load_x a0, interrupt_nest_cnt
	addi a0, a0, -1
	store_x a0, interrupt_nest_cnt, t0
	bnez a0, exit_sp_switch_end
#endif
	lw      t1, pxCurrentTCB			/* Load pxCurrentTCB. */
	lw      sp, 0( t1 )				 	/* Read sp from first TCB member. */
exit_sp_switch_end:

#ifdef CONFIG_ARCH_RISCV_FPU
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	mv s7, a0
	beqz a0, restore_fpu_context	/* a0 is nest_cnt here */
	li t0, FPU_SAVE_PROTSIZE
	mul t0, t0, a0
	la t1, fpu_save_nest
	add s6, t1, t0
	/* record FPU has changed if FS Dirty */
	csrr t2, mstatus
	li a0, SR_FS
	and t0, t2, a0
	li a1, SR_FS_DIRTY
	bne t0, a1, restore_fpu_record_use_end
	li t0, 1
	sll t0, t0, s7
	lw t1, fpu_use_rec_flag
	or t0, t0, t1
	sw t0, fpu_use_rec_flag, t1
restore_fpu_record_use_end:
	load_x t0, 29 * portWORD_SIZE(sp)
	and t0, t0, a0 /* a0 is SR_FS here */
	bne t0, a1, restore_fpu_context /* a1 is SR_FS_DIRTY here */
	/* if (FS in stack is Dirty) && (FPU has changed), change FS to Dirty */
	lw t0, fpu_use_rec_flag
	lw t1, FPU_CONTEXT_SIZE(s6)
	bgeu t1, t0, restore_fpu_context
	not t1, a0 /* a0 is SR_FS here */
	and t2, t2, t1 /* t2 is MSTATUS here */
	or t2, t2, a1 /* a1 is SR_FS_DIRTY here */
	csrw mstatus, t2

restore_fpu_context:
	RESTORE_FPU_CONTEXT_WITH_SKIP pxCurrentTCB,sp,29 * portWORD_SIZE,s0,s1,s2,s3,s4,s5,t0,t1,s6,s7

	/* if nest, overwrites the MSTATUS.FS value of the lowest priority nested interrupt to Dirty.
	 * make sure to restore FPU context for thread.
	 */
	li t0, 1
	bne s7, t0, nest_fpu_restore_end
	load_x t2, 29 * portWORD_SIZE(sp)
	not t1, a0 /* a0 is SR_FS here */
	and t2, t2, t1 /* t2 is MSTATUS in stack here */
	or t2, t2, a1 /* a1 is SR_FS_DIRTY here */
	store_x t2, 29 * portWORD_SIZE(sp)
nest_fpu_restore_end:
#else
#ifdef CONFIG_SAVE_RISCV_FPU_CONTEXT_IN_TCB
	RESTORE_FPU_CONTEXT_WITH_SKIP pxCurrentTCB,sp,29 * portWORD_SIZE,s0,s1,s2,s3,s4,s5,t0,t1
#else
	addi a0, sp, GENERAL_CONTEXT_SIZE
	RESTORE_FPU_CONTEXT a0 t0
#endif
#endif /* CONFIG_ARCH_RISCV_INTERRUPT_NEST */
#endif /* CONFIG_ARCH_RISCV_FPU */

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	load_x a0, interrupt_nest_cnt
	beqz a0, nest_restore_clic
	/* restore mcause and clic regs when nested */
	li t0, CLIC_MINTTHRESH_REG
	lw t1, 35 * portWORD_SIZE( sp )
	sw t1, 0(t0)

	load_x  t0, 34 * portWORD_SIZE( sp )
	csrw mcause, t0
	j nest_restore_end

nest_restore_clic:
	la t0, clic_thresh_save
	li t1, CLIC_MINTTHRESH_REG
	lw t2, 0(t0)
	sw t2, 0(t1)
	la t0, mcause_save
	load_x t1, 0(t0)
	csrw mcause, t1
nest_restore_end:
#endif

	load_x  t0, 0  * portWORD_SIZE( sp )
	csrw mepc, t0

	/* Load mstatus with the interrupt enable bits used by the task. */
	load_x  t0, 29 * portWORD_SIZE( sp )
	csrw mstatus, t0

	load_x  t0, 33 * portWORD_SIZE( sp )
	csrw mscratch, t0

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
#ifdef CONFIG_IRQNEST_OSTEST_GENERAL_CTX_CMP
	load_x a1, interrupt_nest_cnt
	beqz a1, ostest_exit_cmp_end
	/* use irqstack if nest_cnt > 0 */
	la t0, g_irqnest_store_addr
	store_x sp, 0(t0)
	csrr a0, mcause
	andi a0, a0, 0x7FF
	call irqnest_ostest_ctx_cmp_exit
ostest_exit_cmp_end:
#endif

	load_x t0, __freertos_irq_stack_bottom
	li t1, IRQSTACK_BOTTOM_MAGIC
	beq t0, t1, irqstack_overflow_check_end
	ebreak
irqstack_overflow_check_end:
#endif

	load_x  x1 , 1  * portWORD_SIZE( sp )
	load_x  x5 , 2  * portWORD_SIZE( sp )		/* t0 */
	load_x  x6 , 3  * portWORD_SIZE( sp )		/* t1 */
	load_x  x7 , 4  * portWORD_SIZE( sp )		/* t2 */
	load_x  x8 , 5  * portWORD_SIZE( sp )		/* s0/fp */
	load_x  x9 , 6  * portWORD_SIZE( sp )		/* s1 */
	load_x  x10, 7  * portWORD_SIZE( sp )	/* a0 */
	load_x  x11, 8  * portWORD_SIZE( sp )	/* a1 */
	load_x  x12, 9  * portWORD_SIZE( sp )	/* a2 */
	load_x  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	load_x  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	load_x  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	load_x  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	load_x  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	load_x  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	load_x  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	load_x  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	load_x  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	load_x  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	load_x  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	load_x  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	load_x  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	load_x  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	load_x  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	load_x  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	load_x  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	load_x  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	load_x  x31, 28 * portWORD_SIZE( sp )	/* t6 */
	addi sp, sp, portCONTEXT_SIZE

	mret
