.section ".start", "ax", @progbits
.global _start
.type   _start, @function

.global boot_time

_start:
	li t0, CONFIG_TIMESTAMP_BASE_ADDR
	lw t1, 0(t0)
	la t2, boot_time
	sw t1, (t2)
	# li  x1, 0 # ra
	# li  x2, 0 # sp
	# li  x3, 0 # gp
	li  x4, 0
	li  x5, 0
	li  x6, 0
	li  x7, 0
	li  x8, 0
	li  x9, 0
	li  x10,0
	li  x11,0
	li  x12,0
	li  x13,0
	li  x14,0
	li  x15,0
	li  x16,0
	li  x17,0
	li  x18,0
	li  x19,0
	li  x20,0
	li  x21,0
	li  x22,0
	li  x23,0
	li  x24,0
	li  x25,0
	li  x26,0
	li  x27,0
	li  x28,0
	li  x29,0
	li  x30,0
	li  x31,0

.option push
.option norelax
	la gp, __global_pointer$
.option pop

	# enable xthead extension, unaligned access
	li  a3, 0x408000
	csrs mxstatus, a3

	# disable interrupt
	li a3, 0x8
	csrc mstatus, a3

	# disable fpu
	li a3, 0x6000
	csrc mstatus,a3

	# enable fpu
	li a3, 0x2000
	csrs mstatus,a3

	fscsr x0, x0
	fmv.w.x f0,x0
	fmv.w.x f1,x0
	fmv.w.x f2,x0
	fmv.w.x f3,x0
	fmv.w.x f4,x0
	fmv.w.x f5,x0
	fmv.w.x f6,x0
	fmv.w.x f7,x0
	fmv.w.x f8,x0
	fmv.w.x f9,x0
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

	li a3, 0x2000
	csrs mstatus,a3

	# init stack
	la sp, __stack_end

	# init trap
	la a3, __trap_handler
	ori a3, a3, 3
	csrw mtvec, a3
#ifdef CONFIG_CLIC_USED
	# init interrupt
	la a3, clic_interrupt_vectors
#endif
	csrw mtvt, a3

	/* Zero bss */
	la a0, __bss_start
	la a1, __bss_end
	lb a2, (a1)
	beqz a2, 2f
	bgeu a0, a1, 2f
1:
	sw zero, 0(a0)
	addi a0, a0, 4
	bltu a0, a1, 1b
2:
	jal arch_early_init

	# enable ICACHE,DCACHE,BHT,BTB,RAS,WA
	li a3, 0x107f  // I/DCACHE ENABLE, WIRTE ALLOCATE, WB MODE
	csrs 0x7c1, a3  #mhcr

	# enable data_cache_prefetch, amr
	li a3, 0x410c
	csrs 0x7c5, a3   #mhint

	jal	main
	j	exit

  .global __exit
__exit:
	j .

  .global __fail
__fail:
	j .

.align 6
__trap_handler:
	addi sp,sp,-33*4
	sw   x1, 0 *4(sp)
	sw   x2, 1 *4(sp)
	sw   x3, 2 *4(sp)
	sw   x4, 3 *4(sp)
	sw   x5, 4 *4(sp)
	sw   x6, 5 *4(sp)
	sw   x7, 6 *4(sp)
	sw   x8, 7 *4(sp)
	sw   x9, 8 *4(sp)
	sw   x10,9 *4(sp)
	sw   x11,10*4(sp)
	sw   x12,11*4(sp)
	sw   x13,12*4(sp)
	sw   x14,13*4(sp)
	sw   x15,14*4(sp)
	sw   x16,15*4(sp)
	sw   x17,16*4(sp)
	sw   x18,17*4(sp)
	sw   x19,18*4(sp)
	sw   x20,19*4(sp)
	sw   x21,20*4(sp)
	sw   x22,21*4(sp)
	sw   x23,22*4(sp)
	sw   x24,23*4(sp)
	sw   x25,24*4(sp)
	sw   x26,25*4(sp)
	sw   x27,26*4(sp)
	sw   x28,27*4(sp)
	sw   x29,28*4(sp)
	sw   x30,29*4(sp)
	sw   x31,30*4(sp)
	csrr a0, mepc
	sw   a0, 31*4(sp)
	csrr a0, mtval
	sw   a0, 32*4(sp)

	csrr a0, mcause
	mv   a1, sp
	call hadle_trap
	csrw mepc,a0

	lw   x1, 0 *4(sp)
	lw   x3, 2 *4(sp)
	lw   x4, 3 *4(sp)
	lw   x5, 4 *4(sp)
	lw   x6, 5 *4(sp)
	lw   x7, 6 *4(sp)
	lw   x8, 7 *4(sp)
	lw   x9, 8 *4(sp)
	lw   x10,9 *4(sp)
	lw   x11,10*4(sp)
	lw   x12,11*4(sp)
	lw   x13,12*4(sp)
	lw   x14,13*4(sp)
	lw   x15,14*4(sp)
	lw   x16,15*4(sp)
	lw   x17,16*4(sp)
	lw   x18,17*4(sp)
	lw   x19,18*4(sp)
	lw   x20,19*4(sp)
	lw   x21,20*4(sp)
	lw   x22,21*4(sp)
	lw   x23,22*4(sp)
	lw   x24,23*4(sp)
	lw   x25,24*4(sp)
	lw   x26,25*4(sp)
	lw   x27,26*4(sp)
	lw   x28,27*4(sp)
	lw   x29,28*4(sp)
	lw   x30,29*4(sp)
	lw   x31,30*4(sp)
	addi sp, sp, 33*4
	mret

boot_time:
	.word 4
