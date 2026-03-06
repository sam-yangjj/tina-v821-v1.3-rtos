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

#define PRCM_RESET_CTRL             (0x4A00001C)
#define PRCM_BOOT_FLAG_REG          (0x4a0001C0)
#define PRCM_BOOT_ADDR_REG          (0x4a0001C4)
#define PRCM_BOOT_ARG_REG           (0x4a0001C8)
#define PRCM_UPGRADE_FLAG_REG       (0x4a0001D0)
#define PWRCTRL_WAKEUP_STATUS_REG   (0x4A000880)
#define PWRCTRL_SLEEP_MODE_REG      (0x4A011060)
#define PWRCTRL_RES_VLD_REG         (0x4304500C)
#define RTC_WDG_SRST_REG            (0x4a001008)

#define PRCM_SYS_PRIV0              (0x4a000200)
#define PRCM_SYS_PRIV1              (0x4a000204)
#define PRCM_SYS_PRIV2              (0x4a000208)
#define PRCM_SYS_PRIV3              (0x4a00020C)
#define PRCM_SYS_PRIV4              (0x4a000210)
#define PRCM_SYS_PRIV5              (0x4a000214)
#define PRCM_SYS_PRIV6              (0x4a000218)
#define PRCM_SYS_PRIV7              (0x4a00021C)

.section .bss
	.align 2
	.local reg_space
	.globl g_sleep_flag

/* 227 registers backup space */
reg_space:
	.space 760

g_sleep_flag:
.long 0

.macro backup_csr reg offset
	csrr	t1, \reg
	sw	t1, \offset
.endm

.macro resoter_csr reg offset
	lw	t1, \offset
	csrw	\reg, t1
.endm

.macro backup_cpu_register

	la		t0, reg_space

	/* backup General Registers */
	sw		x0,		0*4(t0)
	sw		ra,		1*4(t0)
	sw		sp,		2*4(t0)
	sw		gp,		3*4(t0)
	sw		tp,		4*4(t0)
//	sw		t0, 	5*4(t0)
//	sw		t1, 	6*4(t0)
	sw		t2,		7*4(t0)
	sw		s0,		8*4(t0)
	sw		s1,		9*4(t0)
	sw		a0,		10*4(t0)
	sw		a1,		11*4(t0)
	sw		a2,		12*4(t0)
	sw		a3,		13*4(t0)
	sw		a4,		14*4(t0)
	sw		a5,		15*4(t0)
	sw		a6,		16*4(t0)
	sw		a7,		17*4(t0)
	sw		s2,		18*4(t0)
	sw		s3,		19*4(t0)
	sw		s4,		20*4(t0)
	sw		s5,		21*4(t0)
	sw		s6,		22*4(t0)
	sw		s7,		23*4(t0)
	sw		s8,		24*4(t0)
	sw		s9,		25*4(t0)
	sw		s10,	26*4(t0)
	sw		s11,	27*4(t0)
	sw		t3,  	28*4(t0)
	sw		t4,  	29*4(t0)
	sw		t5,  	30*4(t0)
	sw		t6,  	31*4(t0)
	addi	t0, t0, 32*4

	backup_csr	mstatus			0*4(t0)
	backup_csr	mie				1*4(t0)
	backup_csr	mtvec			2*4(t0)
	backup_csr	mtvt			3*4(t0)
	addi	t0,	t0,	4*4

	backup_csr	mscratch		0*4(t0)
	backup_csr	mepc			1*4(t0)
	backup_csr	mcause			2*4(t0)
	backup_csr	mtval			3*4(t0)
	backup_csr	mip				4*4(t0)
	backup_csr	mnxti			5*4(t0)
	backup_csr	mscratchcsw		6*4(t0)
	backup_csr	mscratchcswl	7*4(t0)
	addi	t0,	t0,	8*4

#ifdef CONFIG_ARCH_RISCV_FPU
	fsw		ft0,	0*8(t0)
	fsw		ft1,	1*8(t0)
	fsw		ft2,	2*8(t0)
	fsw		ft3,	3*8(t0)
	fsw		ft4,	4*8(t0)
	fsw		ft5,	5*8(t0)
	fsw		ft6,	6*8(t0)
	fsw		ft7,	7*8(t0)
	fsw		fs0,	8*8(t0)
	fsw		fs1,	9*8(t0)
	fsw		fa0,	10*8(t0)
	fsw		fa1,	11*8(t0)
	fsw		fa2,	12*8(t0)
	fsw		fa3,	13*8(t0)
	fsw		fa4,	14*8(t0)
	fsw		fa5,	15*8(t0)
	fsw		fa6,	16*8(t0)
	fsw		fa7,	17*8(t0)
	fsw		fs2,	18*8(t0)
	fsw		fs3,	19*8(t0)
	fsw		fs4,	20*8(t0)
	fsw		fs5,	21*8(t0)
	fsw		fs6,	22*8(t0)
	fsw		fs7,	23*8(t0)
	fsw		fs8,	24*8(t0)
	fsw		fs9,	25*8(t0)
	fsw		fs10,	26*8(t0)
	fsw		fs11,	27*8(t0)
	fsw		ft8,	28*8(t0)
	fsw		ft9,	29*8(t0)
	fsw		ft10,	30*8(t0)
	fsw		ft11,	31*8(t0)
	addi	t0,	t0,	32*8

	backup_csr	fflags			0*4(t0)
	backup_csr	frm				1*4(t0)
	backup_csr	fcsr			2*4(t0)
	backup_csr	fxcr			3*4(t0)
	addi	t0,	t0,	4*4
#endif

	backup_csr	pmpcfg0			0*4(t0)
	backup_csr	pmpcfg1			1*4(t0)
	backup_csr	pmpcfg2			2*4(t0)
	backup_csr	pmpcfg3			3*4(t0)
	backup_csr	pmpaddr0		4*4(t0)
	backup_csr	pmpaddr1		5*4(t0)
	backup_csr	pmpaddr2		6*4(t0)
	backup_csr	pmpaddr3		7*4(t0)
	backup_csr	pmpaddr4		8*4(t0)
	backup_csr	pmpaddr5		9*4(t0)
	backup_csr	pmpaddr6		10*4(t0)
	backup_csr	pmpaddr7		11*4(t0)
	backup_csr	pmpaddr8		12*4(t0)
	backup_csr	pmpaddr9		13*4(t0)
	backup_csr	pmpaddr10 		14*4(t0)
	backup_csr	pmpaddr11 		15*4(t0)
	backup_csr	pmpaddr12 		16*4(t0)
	backup_csr	pmpaddr13 		17*4(t0)
	backup_csr	pmpaddr14 		18*4(t0)
	backup_csr	pmpaddr15 		19*4(t0)
	addi	t0,	t0,	20*4

	backup_csr	mcounteren		0*4(t0)
	backup_csr	mcountinhibit	1*4(t0)
	backup_csr	mhpmevent3		2*4(t0)
	backup_csr	mhpmevent4		3*4(t0)
	backup_csr	mhpmevent5		4*4(t0)
	backup_csr	mhpmevent6		5*4(t0)
	backup_csr	mhpmevent7		6*4(t0)
	backup_csr	mhpmevent8		7*4(t0)
	backup_csr	mhpmevent9		8*4(t0)
	backup_csr	mhpmevent10		9*4(t0)
	backup_csr	mhpmevent11		10*4(t0)
	backup_csr	mhpmevent12		11*4(t0)
	backup_csr	mhpmevent13		12*4(t0)
	backup_csr	mhpmevent14		13*4(t0)
	backup_csr	mhpmevent15		14*4(t0)
	backup_csr	mhpmevent16		15*4(t0)
	backup_csr	mhpmevent17		16*4(t0)
	addi	t0,	t0,	17*4

	backup_csr	mcycle			0*4(t0)
	backup_csr	minstret		1*4(t0)
	backup_csr	mcycleh			2*4(t0)
	backup_csr	minstreth		3*4(t0)
	backup_csr	mhpmcounter3	4*4(t0)
	backup_csr	mhpmcounter4	5*4(t0)
	backup_csr	mhpmcounter5	6*4(t0)
	backup_csr	mhpmcounter6	7*4(t0)
	backup_csr	mhpmcounter7	8*4(t0)
	backup_csr	mhpmcounter8	9*4(t0)
	backup_csr	mhpmcounter9	10*4(t0)
	backup_csr	mhpmcounter10	11*4(t0)
	backup_csr	mhpmcounter11	12*4(t0)
	backup_csr	mhpmcounter12	13*4(t0)
	backup_csr	mhpmcounter13	14*4(t0)
	backup_csr	mhpmcounter14	15*4(t0)
	backup_csr	mhpmcounter15	16*4(t0)
	backup_csr	mhpmcounter16	17*4(t0)
	backup_csr	mhpmcounter17	18*4(t0)
	backup_csr	mhpmcounter3h	19*4(t0)
	backup_csr	mhpmcounter4h	20*4(t0)
	backup_csr	mhpmcounter5h	21*4(t0)
	backup_csr	mhpmcounter6h	22*4(t0)
	backup_csr	mhpmcounter7h	23*4(t0)
	backup_csr	mhpmcounter8h	24*4(t0)
	backup_csr	mhpmcounter9h	25*4(t0)
	backup_csr	mhpmcounter10h	26*4(t0)
	backup_csr	mhpmcounter11h	27*4(t0)
	backup_csr	mhpmcounter12h	28*4(t0)
	backup_csr	mhpmcounter13h	29*4(t0)
	backup_csr	mhpmcounter14h	30*4(t0)
	backup_csr	mhpmcounter15h	31*4(t0)
	backup_csr	mhpmcounter16h	32*4(t0)
	backup_csr	mhpmcounter17h	33*4(t0)
	addi	t0,	t0,	34*4

	backup_csr	mxstatus		0*4(t0)
	backup_csr	mhcr			1*4(t0)
	backup_csr	mhint			2*4(t0)
	backup_csr	mraddr			3*4(t0)
	backup_csr	mexstatus		4*4(t0)
	backup_csr	mnmicause		5*4(t0)
	backup_csr	mnmipc			6*4(t0)
	addi	t0, t0, 7*4

.endm

.macro restore_cpu_register

	la		t0, reg_space

	/* restore General Registers */
	lw		x0,		0*4(t0)
	lw		ra,		1*4(t0)
	lw		sp,		2*4(t0)
	lw		gp,		3*4(t0)
	lw		tp,		4*4(t0)
//	lw		t0, 	5*4(t0)
//	lw		t1, 	6*4(t0)
	lw		t2,		7*4(t0)
	lw		s0,		8*4(t0)
	lw		s1,		9*4(t0)
	lw		a0,		10*4(t0)
	lw		a1,		11*4(t0)
	lw		a2,		12*4(t0)
	lw		a3,		13*4(t0)
	lw		a4,		14*4(t0)
	lw		a5,		15*4(t0)
	lw		a6,		16*4(t0)
	lw		a7,		17*4(t0)
	lw		s2,		18*4(t0)
	lw		s3,		19*4(t0)
	lw		s4,		20*4(t0)
	lw		s5,		21*4(t0)
	lw		s6,		22*4(t0)
	lw		s7,		23*4(t0)
	lw		s8,		24*4(t0)
	lw		s9,		25*4(t0)
	lw		s10,	26*4(t0)
	lw		s11,	27*4(t0)
	lw		t3,  	28*4(t0)
	lw		t4,  	29*4(t0)
	lw		t5,  	30*4(t0)
	lw		t6,  	31*4(t0)
	addi	t0, t0, 32*4

	resoter_csr	mstatus			0*4(t0)
	resoter_csr	mie				1*4(t0)
	resoter_csr	mtvec			2*4(t0)
	resoter_csr	mtvt			3*4(t0)
	addi	t0,	t0,	4*4

	resoter_csr	mscratch		0*4(t0)
	resoter_csr	mepc			1*4(t0)
	resoter_csr	mcause			2*4(t0)
	resoter_csr	mtval			3*4(t0)
	resoter_csr	mip				4*4(t0)
//	resoter_csr	mnxti			5*4(t0)
	resoter_csr	mscratchcsw		6*4(t0)
	resoter_csr	mscratchcswl	7*4(t0)
	addi	t0,	t0,	8*4

#ifdef CONFIG_ARCH_RISCV_FPU
	flw		ft0,	0*8(t0)
	flw		ft1,	1*8(t0)
	flw		ft2,	2*8(t0)
	flw		ft3,	3*8(t0)
	flw		ft4,	4*8(t0)
	flw		ft5,	5*8(t0)
	flw		ft6,	6*8(t0)
	flw		ft7,	7*8(t0)
	flw		fs0,	8*8(t0)
	flw		fs1,	9*8(t0)
	flw		fa0,	10*8(t0)
	flw		fa1,	11*8(t0)
	flw		fa2,	12*8(t0)
	flw		fa3,	13*8(t0)
	flw		fa4,	14*8(t0)
	flw		fa5,	15*8(t0)
	flw		fa6,	16*8(t0)
	flw		fa7,	17*8(t0)
	flw		fs2,	18*8(t0)
	flw		fs3,	19*8(t0)
	flw		fs4,	20*8(t0)
	flw		fs5,	21*8(t0)
	flw		fs6,	22*8(t0)
	flw		fs7,	23*8(t0)
	flw		fs8,	24*8(t0)
	flw		fs9,	25*8(t0)
	flw		fs10,	26*8(t0)
	flw		fs11,	27*8(t0)
	flw		ft8,	28*8(t0)
	flw		ft9,	29*8(t0)
	flw		ft10,	30*8(t0)
	flw		ft11,	31*8(t0)
	addi	t0,	t0,	32*8

	resoter_csr	fflags			0*4(t0)
	resoter_csr	frm				1*4(t0)
	resoter_csr	fcsr			2*4(t0)
	resoter_csr	fxcr			3*4(t0)
	addi	t0,	t0,	4*4
#endif

	resoter_csr	pmpaddr0		4*4(t0)
	resoter_csr	pmpaddr1		5*4(t0)
	resoter_csr	pmpaddr2		6*4(t0)
	resoter_csr	pmpaddr3		7*4(t0)
	resoter_csr	pmpaddr4		8*4(t0)
	resoter_csr	pmpaddr5		9*4(t0)
	resoter_csr	pmpaddr6		10*4(t0)
	resoter_csr	pmpaddr7		11*4(t0)
	resoter_csr	pmpaddr8		12*4(t0)
	resoter_csr	pmpaddr9		13*4(t0)
	resoter_csr	pmpaddr10 		14*4(t0)
	resoter_csr	pmpaddr11 		15*4(t0)
	resoter_csr	pmpaddr12 		16*4(t0)
	resoter_csr	pmpaddr13 		17*4(t0)
	resoter_csr	pmpaddr14 		18*4(t0)
	resoter_csr	pmpaddr15 		19*4(t0)
	resoter_csr	pmpcfg0			0*4(t0)
	resoter_csr	pmpcfg1			1*4(t0)
	resoter_csr	pmpcfg2			2*4(t0)
	resoter_csr	pmpcfg3			3*4(t0)
	addi	t0,	t0,	20*4

	resoter_csr	mcounteren		0*4(t0)
	resoter_csr	mcountinhibit	1*4(t0)
	resoter_csr	mhpmevent3		2*4(t0)
	resoter_csr	mhpmevent4		3*4(t0)
	resoter_csr	mhpmevent5		4*4(t0)
	resoter_csr	mhpmevent6		5*4(t0)
	resoter_csr	mhpmevent7		6*4(t0)
	resoter_csr	mhpmevent8		7*4(t0)
	resoter_csr	mhpmevent9		8*4(t0)
	resoter_csr	mhpmevent10		9*4(t0)
	resoter_csr	mhpmevent11		10*4(t0)
	resoter_csr	mhpmevent12		11*4(t0)
	resoter_csr	mhpmevent13		12*4(t0)
	resoter_csr	mhpmevent14		13*4(t0)
	resoter_csr	mhpmevent15		14*4(t0)
	resoter_csr	mhpmevent16		15*4(t0)
	resoter_csr	mhpmevent17		16*4(t0)
	addi	t0,	t0,	17*4

	resoter_csr	mcycle			0*4(t0)
	resoter_csr	minstret		1*4(t0)
	resoter_csr	mcycleh			2*4(t0)
	resoter_csr	minstreth		3*4(t0)
	resoter_csr	mhpmcounter3	4*4(t0)
	resoter_csr	mhpmcounter4	5*4(t0)
	resoter_csr	mhpmcounter5	6*4(t0)
	resoter_csr	mhpmcounter6	7*4(t0)
	resoter_csr	mhpmcounter7	8*4(t0)
	resoter_csr	mhpmcounter8	9*4(t0)
	resoter_csr	mhpmcounter9	10*4(t0)
	resoter_csr	mhpmcounter10	11*4(t0)
	resoter_csr	mhpmcounter11	12*4(t0)
	resoter_csr	mhpmcounter12	13*4(t0)
	resoter_csr	mhpmcounter13	14*4(t0)
	resoter_csr	mhpmcounter14	15*4(t0)
	resoter_csr	mhpmcounter15	16*4(t0)
	resoter_csr	mhpmcounter16	17*4(t0)
	resoter_csr	mhpmcounter17	18*4(t0)
	resoter_csr mhpmcounter3h	19*4(t0)
	resoter_csr mhpmcounter4h	20*4(t0)
	resoter_csr mhpmcounter5h	21*4(t0)
	resoter_csr mhpmcounter6h	22*4(t0)
	resoter_csr mhpmcounter7h	23*4(t0)
	resoter_csr mhpmcounter8h	24*4(t0)
	resoter_csr mhpmcounter9h	25*4(t0)
	resoter_csr mhpmcounter10h	26*4(t0)
	resoter_csr mhpmcounter11h	27*4(t0)
	resoter_csr mhpmcounter12h	28*4(t0)
	resoter_csr mhpmcounter13h	29*4(t0)
	resoter_csr mhpmcounter14h	30*4(t0)
	resoter_csr mhpmcounter15h	31*4(t0)
	resoter_csr mhpmcounter16h	32*4(t0)
	resoter_csr mhpmcounter17h	33*4(t0)
	addi	t0,	t0,	34*4

	resoter_csr	mxstatus		0*4(t0)
	resoter_csr	mhcr			1*4(t0)
	resoter_csr	mhint			2*4(t0)
	resoter_csr	mraddr			3*4(t0)
	resoter_csr	mexstatus		4*4(t0)
	resoter_csr	mnmicause		5*4(t0)
	resoter_csr	mnmipc			6*4(t0)
	addi	t0, t0, 7*4
.endm

.section .text
	.align 2
	.globl reboot_soc
	.type  reboot_soc, %function
reboot_soc:
	/* rtc wdg enable reset */
	la	t0, PRCM_RESET_CTRL
	lw	t1, 0(t0)
	ori t1, t1, 0x8
	sw t1, 0(t0)
	/* rtc wdg soft reset */
	la t0, RTC_WDG_SRST_REG
	la t1, 0x16AA0001
	sw t1, 0(t0)
	/* wait reboot */
wait_reboot:
	jal wait_reboot
	ret

.section .text
	.align 2
	.globl cpu_suspend
	.type  cpu_suspend, %function
cpu_suspend:
	/* backup t0 and t1 to tack */
	sw	t0,	-4(sp)
	sw	t1,	-8(sp)

	/* backup all cpu registers */
	backup_cpu_register

	/* set boot flag */
	la t0, PRCM_BOOT_FLAG_REG
	la t1, 0x429B0001
	sw t1, 0(t0)

	/* set boot addr */
	la t0, PRCM_BOOT_ADDR_REG
	la t1, cpu_resume
	sw t1, 0(t0)

	/* set wfi mode */
	la t0, 0x0000000C
	csrc mexstatus, t0

	/* enter wfi */
	wfi
 	/* jump to cpu_resume if wfi fail */
	jal cpu_resume

	.align 2
	.globl cpu_resume
	.type  cpu_resume, %function
cpu_resume:

	/* clr boot flag */
	li t0, PRCM_BOOT_FLAG_REG
	li t1, 0x429b0000
	sw t1, 0(t0)

	/* restore all cpu registers */
	restore_cpu_register

	/* backup t0 and t1 to stack */
	lw t1, -8(sp)
	lw t0, -4(sp)
	ret

	.align 2
	.globl hibernation_suspend
	.type  hibernation_suspend, %function
hibernation_suspend:
	/* check wakesrc */
	la	t0, PWRCTRL_WAKEUP_STATUS_REG
	lw	t1, 0(t0)
	andi	t1, t1, 0x00000007
	la	t0,	0x00000000
	bne	t0, t1,	reboot_soc

	/* set boot flag */
	la t0, PRCM_BOOT_FLAG_REG
	la t1, 0x429B0000
	sw t1, 0(t0)

	/* set boot addr */
	la t0, PRCM_BOOT_ADDR_REG
	la t1, 0x00000000
	sw t1, 0(t0)

	/* set upgrade flag */
	la t0, PRCM_UPGRADE_FLAG_REG
	la t1, 0x429B0000
	sw t1, 0(t0)

	/* set wfi mode */
	la t0, 0x0000000C
	csrc mexstatus, t0

	/* enter wfi */
	wfi
	ret

