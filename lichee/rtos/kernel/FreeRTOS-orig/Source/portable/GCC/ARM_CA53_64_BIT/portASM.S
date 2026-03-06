/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 * 1 tab == 4 spaces!
 */

#define GUEST

	.text

	/* Variables and functions. */
	.extern ullMaxAPIPriorityMask
	.extern pxCurrentTCB
	.extern vTaskSwitchContext
	.extern vApplicationIRQHandler
	.extern ullPortInterruptNesting
	.extern ullPortTaskHasFPUContext
	.extern ullCriticalNesting
	.extern ullPortYieldRequired
	.extern ullICCEOIR
	.extern ullICCIAR
	.extern test_cnt
	.extern _freertos_vector_table

	.global FreeRTOS_IRQ_Handler
	.global FreeRTOS_SVC_Handler
	.global FreeRTOS_SWI_Handler
	.global vPortRestoreTaskContext

.macro PUSH_ALL_CPU_REGS stackPtr
        stp     x0, x1, [\stackPtr, #-16]!
        stp     x2, x3, [\stackPtr, #-16]!
        stp     x4, x5, [\stackPtr, #-16]!
        stp     x6, x7, [\stackPtr, #-16]!
        stp     x8, x9, [\stackPtr, #-16]!
        stp     x10, x11, [\stackPtr, #-16]!
        stp     x12, x13, [\stackPtr, #-16]!
        stp     x14, x15, [\stackPtr, #-16]!
        stp     x16, x17, [\stackPtr, #-16]!
        stp     x18, x19, [\stackPtr, #-16]!
        stp     x20, x21, [\stackPtr, #-16]!
        stp     x22, x23, [\stackPtr, #-16]!
        stp     x24, x25, [\stackPtr, #-16]!
        stp     x26, x27, [\stackPtr, #-16]!
        stp     x28, x29, [\stackPtr, #-16]!
        stp     x30, xzr, [\stackPtr, #-16]!
.endm

.macro POP_ALL_CPU_REGS stackPtr
        LDP 	X30, XZR, [\stackPtr], #0x10
        LDP 	X28, X29, [\stackPtr], #0x10
        LDP 	X26, X27, [\stackPtr], #0x10
        LDP 	X24, X25, [\stackPtr], #0x10
        LDP 	X22, X23, [\stackPtr], #0x10
        LDP 	X20, X21, [\stackPtr], #0x10
        LDP 	X18, X19, [\stackPtr], #0x10
        LDP 	X16, X17, [\stackPtr], #0x10
        LDP 	X14, X15, [\stackPtr], #0x10
        LDP 	X12, X13, [\stackPtr], #0x10
        LDP 	X10, X11, [\stackPtr], #0x10
        LDP 	X8, X9, [\stackPtr], #0x10
        LDP 	X6, X7, [\stackPtr], #0x10
        LDP 	X4, X5, [\stackPtr], #0x10
        LDP 	X2, X3, [\stackPtr], #0x10
        LDP 	X0, X1, [\stackPtr], #0x10
.endm

.macro PUSH_CALLER_SAVE_CPU_REGS stackPtr
        stp     x0, x1, [\stackPtr, #-16]!
        stp     x2, x3, [\stackPtr, #-16]!
        stp     x4, x5, [\stackPtr, #-16]!
        stp     x6, x7, [\stackPtr, #-16]!
        stp     x8, x9, [\stackPtr, #-16]!
        stp     x10, x11, [\stackPtr, #-16]!
        stp     x12, x13, [\stackPtr, #-16]!
        stp     x14, x15, [\stackPtr, #-16]!
        stp     x16, x17, [\stackPtr, #-16]!
        stp     x18, x19, [\stackPtr, #-16]!
        stp     x29, x30, [\stackPtr, #-16]!
.endm

.macro POP_CALLER_SAVE_CPU_REGS stackPtr
        ldp     x29, x30, [\stackPtr], #16
        ldp     x18, x19, [\stackPtr], #16
        ldp     x16, x17, [\stackPtr], #16
        ldp     x14, x15, [\stackPtr], #16
        ldp     x12, x13, [\stackPtr], #16
        ldp     x10, x11, [\stackPtr], #16
        ldp     x8, x9, [\stackPtr], #16
        ldp     x6, x7, [\stackPtr], #16
        ldp     x4, x5, [\stackPtr], #16
        ldp     x2, x3, [\stackPtr], #16
        ldp     x0, x1, [\stackPtr], #16
.endm

.macro PUSH_CALLER_SAVE_FPU_REGS stackPtr
        stp     q0, q1, [\stackPtr, #-32]!
        stp     q2, q3, [\stackPtr, #-32]!
        stp     q4, q5, [\stackPtr, #-32]!
        stp     q6, q7, [\stackPtr, #-32]!
        stp     q8, q9, [\stackPtr, #-32]!
        stp     q10, q11, [\stackPtr, #-32]!
        stp     q12, q13, [\stackPtr, #-32]!
        stp     q14, q15, [\stackPtr, #-32]!
        stp     q16, q17, [\stackPtr, #-32]!
        stp     q18, q19, [\stackPtr, #-32]!
        stp     q20, q21, [\stackPtr, #-32]!
        stp     q22, q23, [\stackPtr, #-32]!
        stp     q24, q25, [\stackPtr, #-32]!
        stp     q26, q27, [\stackPtr, #-32]!
        stp     q28, q29, [\stackPtr, #-32]!
        stp     q30, q31, [\stackPtr, #-32]!
.endm

.macro POP_CALLER_SAVE_FPU_REGS stackPtr
        ldp     q30, q31, [\stackPtr], #32
        ldp     q28, q29, [\stackPtr], #32
        ldp     q26, q27, [\stackPtr], #32
        ldp     q24, q25, [\stackPtr], #32
        ldp     q22, q23, [\stackPtr], #32
        ldp     q20, q21, [\stackPtr], #32
        ldp     q18, q19, [\stackPtr], #32
        ldp     q16, q17, [\stackPtr], #32
        ldp     q14, q15, [\stackPtr], #32
        ldp     q12, q13, [\stackPtr], #32
        ldp     q10, q11, [\stackPtr], #32
        ldp     q8, q9, [\stackPtr], #32
        ldp     q6, q7, [\stackPtr], #32
        ldp     q4, q5, [\stackPtr], #32
        ldp     q2, q3, [\stackPtr], #32
        ldp     q0, q1, [\stackPtr], #32
.endm

.macro portSAVE_CONTEXT
	/* Switch to use the EL0 stack pointer. */
	MSR 	SPSEL, #0

	/* Save the entire context. */
    PUSH_ALL_CPU_REGS SP

	/* Save the SPSR. */
	MRS		X3, SPSR_EL1
	MRS		X2, ELR_EL1

	STP 	X2, X3, [SP, #-0x10]!

    LDR		X0, ullPortTaskHasFPUContextConst

	/* Save the FPU context indicator. */
	LDR		X2, [X0]

	/* Save the FPU context, if any (32 128-bit registers). */
	CMP		X2, #0
	B.EQ	1f
    PUSH_CALLER_SAVE_FPU_REGS SP

1:
	/* Store the FPU context indicator. */
	STR 	x2, [SP, #-0x10]!

	LDR 	X0, pxCurrentTCBConst

	LDR 	X1, [X0]
	MOV 	X0, SP   /* Move SP into X0 for saving. */
	STR 	X0, [X1]

	/* Switch to use the ELx stack pointer. */
	MSR 	SPSEL, #1

	.endm

; /**********************************************************************/

.macro portRESTORE_CONTEXT
	/* Switch to use the EL0 stack pointer. */
	MSR 	SPSEL, #0

	/* Set the SP to point to the stack of the task being restored. */
	LDR		X0, pxCurrentTCBConst

	LDR		X1, [X0]
	LDR		X0, [X1]
	MOV		SP, X0

	LDR 	X2, [SP], #0x10  /* FPU context. */

	/* Set the PMR register to be correct for the current critical nesting
	depth. */
	MOV		X1, #255					/* X1 holds the unmask value. */

	MSR		s3_0_c4_c6_0, X1 			/* Write the mask value to ICCPMR. s3_0_c4_c6_0 is ICC_PMR_EL1. */
	DSB 	SY							/* _RB_Barriers probably not required here. */
	ISB 	SY

    LDR		X0, ullPortTaskHasFPUContextConst

	/* Restore the FPU context indicator. */
	STR		X2, [X0]

	/* Restore the FPU context, if any. */
	CMP		X2, #0
	B.EQ	1f
    POP_CALLER_SAVE_FPU_REGS SP

1:
	LDP 	X2, X3, [SP], #0x10  /* SPSR and ELR. */

	/* Restore the SPSR. */
	MSR		SPSR_EL1, X3
	/* Restore the ELR. */
	MSR		ELR_EL1, X2

    POP_ALL_CPU_REGS SP

	/* Switch to use the ELx stack pointer.  _RB_ Might not be required. */
	MSR 	SPSEL, #1

	ERET

.endm

/******************************************************************************
 * handles SVC entry and exit.
 *****************************************************************************/
.align 8
.type HwiP_SVC_Handler, %function
FreeRTOS_SVC_Handler:
    /* Save the context of the current task and select a new task to run. */
	portSAVE_CONTEXT
	MRS		X0, ESR_EL1
    MRS     X2, ELR_EL1
	LSR		X1, X0, #26
	CMP		X1, #0x15 	/* 0x15 = SVC instruction. */
	B.NE	FreeRTOS_SVC_Abort
    MRS     x0, mpidr_el1       /* Get CoreID */
    and     x0, x0, #0xff
	BL 		vTaskSwitchContext
	portRESTORE_CONTEXT
FreeRTOS_SVC_Abort:
	/* Full ESR is in X0, exception class code is in X1. */
	B		.

/******************************************************************************
 * FreeRTOS_SWI_Handler handler is used to perform a context switch.
 *****************************************************************************/
.align 8
.type FreeRTOS_SWI_Handler, %function
FreeRTOS_SWI_Handler:
	/* Save the context of the current task and select a new task to run. */
	portSAVE_CONTEXT
#if defined( GUEST )
	MRS		X0, ESR_EL1
#else
	MRS		X0, ESR_EL3
#endif

	LSR		X1, X0, #26

#if defined( GUEST )
	CMP		X1, #0x15 	/* 0x15 = SVC instruction. */
#else
	CMP		X1, #0x17 	/* 0x17 = SMC instruction. */
#endif
	B.NE	FreeRTOS_Abort
	BL 		vTaskSwitchContext

	portRESTORE_CONTEXT

FreeRTOS_Abort:
	/* Full ESR is in X0, exception class code is in X1. */
	B		.

/******************************************************************************
 * vPortRestoreTaskContext is used to start the scheduler.
 *****************************************************************************/
.type vPortRestoreTaskContext, %function
vPortRestoreTaskContext:
	DSB		SY
	ISB		SY
	/* Start the first task. */
	portRESTORE_CONTEXT

/******************************************************************************
 * FreeRTOS_IRQ_Handler handles IRQ entry and exit.
 *****************************************************************************/
.align 8
.type FreeRTOS_IRQ_Handler, %function
FreeRTOS_IRQ_Handler:
	/* save cpu scratch regs */
    PUSH_CALLER_SAVE_CPU_REGS SP

	/* Save the SPSR and ELR. */
	MRS		X3, SPSR_EL1
	MRS		X2, ELR_EL1
	STP 	X2, X3, [SP, #-0x10]!

	/* Increment the interrupt nesting counter. */
	LDR		X5, ullPortInterruptNestingConst
	LDR		X1, [X5]	
	ADD		X6, X1, #1
	STR		X6, [X5]	

	/* Maintain the interrupt nesting information across the function call. */
	STP		X1, X5, [SP, #-0x10]!

	/* Call the C handler. */
	BL vApplicationIRQHandler

	/* Disable interrupts. */
	MSR 	DAIFSET, #2
	DSB		SY
	ISB		SY

	/* Restore the critical nesting count. */
	LDP		X1, X5, [SP], #0x10
	STR		X1, [X5]

	CMP		X1, #0
	B.NE	Exit_IRQ_No_Context_Switch

	LDR		X0, ullPortYieldRequiredConst

	LDR		X1, [X0]
	CMP		X1, #0
	B.EQ	Exit_IRQ_No_Context_Switch

	/* Reset ullPortYieldRequired to 0. */
	MOV		X2, #0
	STR		X2, [X0]

	/* Restore volatile registers. */
	LDP 	X4, X5, [SP], #0x10  /* SPSR and ELR. */
	MSR		SPSR_EL1, X5
	MSR		ELR_EL1, X4
	DSB		SY
	ISB		SY

    POP_CALLER_SAVE_CPU_REGS SP

	/* Save the context of the current task and select a new task to run. */
	portSAVE_CONTEXT
	BL vTaskSwitchContext
	portRESTORE_CONTEXT

Exit_IRQ_No_Context_Switch:
	/* Restore volatile registers. */
	LDP 	X4, X5, [SP], #0x10  /* SPSR and ELR. */
	MSR		SPSR_EL1, X5
	MSR		ELR_EL1, X4
	DSB		SY
	ISB		SY

    POP_CALLER_SAVE_CPU_REGS SP

	ERET

pxCurrentTCBConst: .dword pxCurrentTCB
ullCriticalNestingConst: .dword ullCriticalNesting
ullPortTaskHasFPUContextConst: .dword ullPortTaskHasFPUContext

ullPortYieldRequiredConst: .dword ullPortYieldRequired
ullPortInterruptNestingConst: .dword ullPortInterruptNesting

.end





