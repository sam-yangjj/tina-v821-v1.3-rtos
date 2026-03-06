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
#ifdef CONFIG_ARCH_RISCV_FPU

#include <csr.h>
#include <fpu.h>

#if __riscv_xlen == 64
	#define STORE_REG sd
	#define LOAD_REG ld
#elif __riscv_xlen == 32
	#define STORE_REG sw
	#define LOAD_REG lw
#else
	#error The toolchain did not define __riscv_xlen
#endif

#if __riscv_flen == 64
	#define FSTORE fsd
	#define FLOAD fld
#elif __riscv_flen == 32
	#define FSTORE fsw
	#define FLOAD flw
#else
	#error toolchain did not define __riscv_flen
#endif

.macro SAVE_FPU_CONTEXT mem_addr_reg:req tmp_reg:req
	FSTORE f0, FPU_CTX_F0_OFFSET(\mem_addr_reg)
	FSTORE f1, FPU_CTX_F1_OFFSET(\mem_addr_reg)
	FSTORE f2, FPU_CTX_F2_OFFSET(\mem_addr_reg)
	FSTORE f3, FPU_CTX_F3_OFFSET(\mem_addr_reg)
	FSTORE f4, FPU_CTX_F4_OFFSET(\mem_addr_reg)
	FSTORE f5, FPU_CTX_F5_OFFSET(\mem_addr_reg)
	FSTORE f6, FPU_CTX_F6_OFFSET(\mem_addr_reg)
	FSTORE f7, FPU_CTX_F7_OFFSET(\mem_addr_reg)
	FSTORE f8, FPU_CTX_F8_OFFSET(\mem_addr_reg)
	FSTORE f9, FPU_CTX_F9_OFFSET(\mem_addr_reg)
	FSTORE f10, FPU_CTX_F10_OFFSET(\mem_addr_reg)
	FSTORE f11, FPU_CTX_F11_OFFSET(\mem_addr_reg)
	FSTORE f12, FPU_CTX_F12_OFFSET(\mem_addr_reg)
	FSTORE f13, FPU_CTX_F13_OFFSET(\mem_addr_reg)
	FSTORE f14, FPU_CTX_F14_OFFSET(\mem_addr_reg)
	FSTORE f15, FPU_CTX_F15_OFFSET(\mem_addr_reg)
	FSTORE f16, FPU_CTX_F16_OFFSET(\mem_addr_reg)
	FSTORE f17, FPU_CTX_F17_OFFSET(\mem_addr_reg)
	FSTORE f18, FPU_CTX_F18_OFFSET(\mem_addr_reg)
	FSTORE f19, FPU_CTX_F19_OFFSET(\mem_addr_reg)
	FSTORE f20, FPU_CTX_F20_OFFSET(\mem_addr_reg)
	FSTORE f21, FPU_CTX_F21_OFFSET(\mem_addr_reg)
	FSTORE f22, FPU_CTX_F22_OFFSET(\mem_addr_reg)
	FSTORE f23, FPU_CTX_F23_OFFSET(\mem_addr_reg)
	FSTORE f24, FPU_CTX_F24_OFFSET(\mem_addr_reg)
	FSTORE f25, FPU_CTX_F25_OFFSET(\mem_addr_reg)
	FSTORE f26, FPU_CTX_F26_OFFSET(\mem_addr_reg)
	FSTORE f27, FPU_CTX_F27_OFFSET(\mem_addr_reg)
	FSTORE f28, FPU_CTX_F28_OFFSET(\mem_addr_reg)
	FSTORE f29, FPU_CTX_F29_OFFSET(\mem_addr_reg)
	FSTORE f30, FPU_CTX_F30_OFFSET(\mem_addr_reg)
	FSTORE f31, FPU_CTX_F31_OFFSET(\mem_addr_reg)
	frcsr \tmp_reg
	STORE_REG \tmp_reg, FPU_CTX_FCSR_OFFSET(\mem_addr_reg)
.endm

.macro RESTORE_FPU_CONTEXT mem_addr_reg:req tmp_reg:req
	FLOAD f0, FPU_CTX_F0_OFFSET(\mem_addr_reg)
	FLOAD f1, FPU_CTX_F1_OFFSET(\mem_addr_reg)
	FLOAD f2, FPU_CTX_F2_OFFSET(\mem_addr_reg)
	FLOAD f3, FPU_CTX_F3_OFFSET(\mem_addr_reg)
	FLOAD f4, FPU_CTX_F4_OFFSET(\mem_addr_reg)
	FLOAD f5, FPU_CTX_F5_OFFSET(\mem_addr_reg)
	FLOAD f6, FPU_CTX_F6_OFFSET(\mem_addr_reg)
	FLOAD f7, FPU_CTX_F7_OFFSET(\mem_addr_reg)
	FLOAD f8, FPU_CTX_F8_OFFSET(\mem_addr_reg)
	FLOAD f9, FPU_CTX_F9_OFFSET(\mem_addr_reg)
	FLOAD f10, FPU_CTX_F10_OFFSET(\mem_addr_reg)
	FLOAD f11, FPU_CTX_F11_OFFSET(\mem_addr_reg)
	FLOAD f12, FPU_CTX_F12_OFFSET(\mem_addr_reg)
	FLOAD f13, FPU_CTX_F13_OFFSET(\mem_addr_reg)
	FLOAD f14, FPU_CTX_F14_OFFSET(\mem_addr_reg)
	FLOAD f15, FPU_CTX_F15_OFFSET(\mem_addr_reg)
	FLOAD f16, FPU_CTX_F16_OFFSET(\mem_addr_reg)
	FLOAD f17, FPU_CTX_F17_OFFSET(\mem_addr_reg)
	FLOAD f18, FPU_CTX_F18_OFFSET(\mem_addr_reg)
	FLOAD f19, FPU_CTX_F19_OFFSET(\mem_addr_reg)
	FLOAD f20, FPU_CTX_F20_OFFSET(\mem_addr_reg)
	FLOAD f21, FPU_CTX_F21_OFFSET(\mem_addr_reg)
	FLOAD f22, FPU_CTX_F22_OFFSET(\mem_addr_reg)
	FLOAD f23, FPU_CTX_F23_OFFSET(\mem_addr_reg)
	FLOAD f24, FPU_CTX_F24_OFFSET(\mem_addr_reg)
	FLOAD f25, FPU_CTX_F25_OFFSET(\mem_addr_reg)
	FLOAD f26, FPU_CTX_F26_OFFSET(\mem_addr_reg)
	FLOAD f27, FPU_CTX_F27_OFFSET(\mem_addr_reg)
	FLOAD f28, FPU_CTX_F28_OFFSET(\mem_addr_reg)
	FLOAD f29, FPU_CTX_F29_OFFSET(\mem_addr_reg)
	FLOAD f30, FPU_CTX_F30_OFFSET(\mem_addr_reg)
	FLOAD f31, FPU_CTX_F31_OFFSET(\mem_addr_reg)
	LOAD_REG \tmp_reg, FPU_CTX_FCSR_OFFSET(\mem_addr_reg)
	fscsr \tmp_reg
.endm

#ifdef CONFIG_SAVE_RISCV_FPU_CONTEXT_IN_TCB

/* the size of g_fpu_ctx_member_offset is fixed to 4 bytes whether the CPU word length */
.global g_fpu_ctx_member_offset

/*
 * Save FPU context with one skipped situation: skip save operation when FPU is not dirty
 * when exit:
 * saved_reg1 is the current TCB.
 * saved_reg2 is the modified mstatus register content or origin register value.
 * saved_reg3 is the base addr for FPU saving/restore in interrupt nest
 * saved_reg4 is the interrupt nest count
 */
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
.macro SAVE_FPU_CONTEXT_WITH_SKIP current_tcb:req stack_pointer:req mstatus_offset_in_stack:req\
 saved_reg1:req saved_reg2:req tmp_reg1:req tmp_reg2:req saved_reg3:req saved_reg4:req

	bnez \saved_reg4, fpu_save_check_fs
#else
.macro SAVE_FPU_CONTEXT_WITH_SKIP current_tcb:req stack_pointer:req mstatus_offset_in_stack:req\
 saved_reg1:req saved_reg2:req tmp_reg1:req tmp_reg2:req
#endif
	/* save the RTOS TCB when saving the FPU context */
	LOAD_REG \saved_reg1, \current_tcb
	STORE_REG \saved_reg1, g_rtos_tcb_when_save_fpu_ctx, \tmp_reg1

fpu_save_check_fs:
	csrr \tmp_reg1, mstatus
	li \saved_reg2, SR_FS
	and \tmp_reg1, \tmp_reg1, \saved_reg2

#ifdef CONFIG_RECORD_RISCV_FPU_CTX_STATISTICS
	beq \tmp_reg1, \saved_reg2, fpu_context_save

	LOAD_REG \tmp_reg1, g_skip_fpu_ctx_save_cnt
	add \tmp_reg1, \tmp_reg1, 1
	STORE_REG \tmp_reg1, g_skip_fpu_ctx_save_cnt, \tmp_reg2
	j skip_fpu_context_save

fpu_context_save:
#else
	bne \tmp_reg1, \saved_reg2, skip_fpu_context_save /* FPU is not dirty state, don't need to save FPU context */
#endif

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	beqz \saved_reg4, fpu_save_get_tcb_member

	/* get the offset for fpu context saving */
	mv \tmp_reg1, \saved_reg3
	j fpu_save_action
#endif
fpu_save_get_tcb_member:
	/* get the offset of arch_fpu_context_t member in thread TCB */
	lw \tmp_reg2, g_fpu_ctx_member_offset
	add \tmp_reg1, \saved_reg1, \tmp_reg2

fpu_save_action:
	SAVE_FPU_CONTEXT \tmp_reg1 \tmp_reg2

	/* set FS filed(in mstatus reg) to clean state */
	csrc mstatus, \saved_reg2
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	beqz \saved_reg4, fpu_save_change_fs_to_clean
	li \tmp_reg1, SR_FS_INITIAL
	j fpu_save_change_fs
fpu_save_change_fs_to_clean:
	li \tmp_reg1, SR_FS_CLEAN
fpu_save_change_fs:
	csrs mstatus, \tmp_reg1
#else
	li \tmp_reg1, SR_FS_CLEAN
	csrs mstatus, \tmp_reg1
#endif

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	/* FS should restore to its original if nest_cnt > 0.
	 * Irq handler may be interrupted again,
	 * or exit without FPU restore even if it changed FPU because FS is set to clean and it doesn't change FPU after restore.
	 */
	bnez \saved_reg4, skip_fpu_context_save
	li \tmp_reg1, SR_FS_CLEAN
#endif

	/* set FS filed(in thread stack) to clean state */
	not \tmp_reg2, \saved_reg2 /* saved_reg2 is SR_FS here */
	LOAD_REG  \saved_reg2, \mstatus_offset_in_stack(\stack_pointer)
	and \saved_reg2, \saved_reg2, \tmp_reg2
	or \saved_reg2, \saved_reg2, \tmp_reg1 /* tmp_reg1 is SR_FS_CLEAN here */
	STORE_REG \saved_reg2, \mstatus_offset_in_stack(\stack_pointer)

skip_fpu_context_save:
.endm

/*
 * Restore FPU context with three skipped situation:
 * 1. skip restore operation when FS field in thread stack is off(fatal error if happened)
 * 2. skip restore operation when FS field in thread stack is init
 * 3. skip restore operation when FPU status is clean and no thread switch since the last FPU context save operation
 * when exit:
 * saved_reg1 is the modified or unmodified mstatus register content in thread stack.
 * saved_reg2 is the value of SR_FS.
 * saved_reg3 is the value of FS field in thread stack when enter this logic
 * saved_reg4 is the current TCB.
 * saved_reg5 is the value of SR_FS_CLEAN.
 * saved_reg6 is the value of SR_FS_DIRTY.
 * saved_reg7 is the base addr for FPU saving/restore in interrupt nest
 * saved_reg8 is the interrupt nest count
 */
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
.macro RESTORE_FPU_CONTEXT_WITH_SKIP current_tcb:req stack_pointer:req mstatus_offset_in_stack:req\
 saved_reg1:req saved_reg2:req saved_reg3:req saved_reg4:req saved_reg5:req saved_reg6:req\
 tmp_reg1:req tmp_reg2:req saved_reg7:req saved_reg8:req
#else
.macro RESTORE_FPU_CONTEXT_WITH_SKIP current_tcb:req stack_pointer:req mstatus_offset_in_stack:req\
 saved_reg1:req saved_reg2:req saved_reg3:req saved_reg4:req saved_reg5:req saved_reg6:req\
 tmp_reg1:req tmp_reg2:req
#endif

	/* get the saved mstatus reg value in thread stack and extract the FS field */
	LOAD_REG \saved_reg1, \mstatus_offset_in_stack(\stack_pointer)
	li \saved_reg2, SR_FS
	and \saved_reg3, \saved_reg1, \saved_reg2

	/* confirm whether the thread need to use FPU */
	li \tmp_reg1, SR_FS_OFF
#ifdef CONFIG_RECORD_RISCV_FPU_CTX_STATISTICS
	bne \saved_reg3, \tmp_reg1, fpu_is_not_off_state

	LOAD_REG \tmp_reg1, g_skip_fpu_off_ctx_restore_cnt
	add \tmp_reg1, \tmp_reg1, 1
	STORE_REG \tmp_reg1, g_skip_fpu_off_ctx_restore_cnt, \tmp_reg2
	j skip_fpu_context_restore

fpu_is_not_off_state:
#else
	beq \saved_reg3, \tmp_reg1, skip_fpu_context_restore /* the value of FS field is OFF, does't need to restore FPU context */
#endif

	/* confirm whether the thread has used FPU previously */
	li \tmp_reg1, SR_FS_INITIAL
#ifdef CONFIG_RECORD_RISCV_FPU_CTX_STATISTICS
	bne \saved_reg3, \tmp_reg1, fpu_is_not_init_state

	LOAD_REG \tmp_reg1, g_skip_fpu_init_ctx_restore_cnt
	add \tmp_reg1, \tmp_reg1, 1
	STORE_REG \tmp_reg1, g_skip_fpu_init_ctx_restore_cnt, \tmp_reg2
	j skip_fpu_context_restore

fpu_is_not_init_state:
#else
	beq \saved_reg3, \tmp_reg1, skip_fpu_context_restore /* the value of FS field is initial, does't need to restore FPU context */
#endif

	LOAD_REG \saved_reg4, \current_tcb
	li \saved_reg5, SR_FS_CLEAN

	/* handle abnormal situation. If not in interrupt nesting, the value of FS field should not be dirte value in here!
	 * maybe the TCB of this thread has been modified accidentally.
	 */
	li \saved_reg6, SR_FS_DIRTY
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	bnez \saved_reg8, fpu_resotre_skip_stack_dirty_check
	beq \saved_reg3, \saved_reg6, fpu_context_restore /* the value of FS field is dirty, force to restore the FPU context in this situation. */
fpu_resotre_skip_stack_dirty_check:
#else
	beq \saved_reg3, \saved_reg6, fpu_context_restore /* the value of FS field is dirty, force to restore the FPU context in this situation. */
#endif

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	bnez \saved_reg8, fpu_resotre_check_fs
#endif

	/* confirm whether there is a thread switch operation after execute vTaskSwitchContext function */
	LOAD_REG \tmp_reg1, g_rtos_tcb_when_save_fpu_ctx
	bne \saved_reg4, \tmp_reg1, fpu_context_restore /* thread switch has been happened, need to restore FPU context*/

fpu_resotre_check_fs:
	/* the value of FS field(in saved stack) is clean and no thread switch, confirm whether the current FPU is clean state */
	csrr \tmp_reg1, mstatus
	and \tmp_reg2, \tmp_reg1, \saved_reg2

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	/* If support interrupt nest, the INITIAL state means FPU is not used since the last FPU context save operation */
	beqz \saved_reg8, fpu_restore_nest_check_fs
	li \saved_reg5, SR_FS_INITIAL
fpu_restore_nest_check_fs:
#endif

#ifdef CONFIG_RECORD_RISCV_FPU_CTX_STATISTICS
	bne \tmp_reg2, \saved_reg5, fpu_is_not_clean_state

	LOAD_REG \tmp_reg1, g_skip_fpu_clean_ctx_restore_cnt
	add \tmp_reg1, \tmp_reg1, 1
	STORE_REG \tmp_reg1, g_skip_fpu_clean_ctx_restore_cnt, \tmp_reg2
	j skip_fpu_context_restore

fpu_is_not_clean_state:
#else
	/* FPU is not used(clean state) since the last FPU context save operation, does't need to restore FPU context */
	beq \tmp_reg2, \saved_reg5, skip_fpu_context_restore
#endif

fpu_context_restore:
#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	beqz \saved_reg8, fpu_restore_get_tcb_member

	li \saved_reg5, SR_FS_CLEAN
	/* get the offset for fpu context saving */
	mv \tmp_reg1, \saved_reg7
	j fpu_restore_action
fpu_restore_get_tcb_member:
#endif
	/* get the offset of arch_fpu_context_t member */
	lw \tmp_reg2, g_fpu_ctx_member_offset
	add \tmp_reg1, \saved_reg4, \tmp_reg2

fpu_restore_action:
	RESTORE_FPU_CONTEXT \tmp_reg1 \tmp_reg2

#ifdef CONFIG_ARCH_RISCV_INTERRUPT_NEST
	/* FS should restore to its original if nest_cnt > 0.
	 * Irq handler may be interrupted again,
	 * or exit without FPU restore even if it changed FPU because FS is set to clean and it doesn't change FPU after restore.
	 */
	bnez \saved_reg8, skip_fpu_state_update
#endif

	bne \saved_reg3, \saved_reg6, skip_fpu_state_update
	/* set FS filed(in thread stack) to clean state when the value of FS field is dirty. */
	not \tmp_reg2, \saved_reg2
	and \saved_reg1, \saved_reg1, \tmp_reg2
	or \saved_reg1, \saved_reg1, \saved_reg5
	STORE_REG \saved_reg1, \mstatus_offset_in_stack(\stack_pointer)

skip_fpu_state_update:

skip_fpu_context_restore:
	/* no special operation here, the FS field of mstatus register will be correctly restored later */
.endm

#endif /* CONFIG_SAVE_RISCV_FPU_CONTEXT_IN_TCB */

#endif /* CONFIG_ARCH_RISCV_FPU */
