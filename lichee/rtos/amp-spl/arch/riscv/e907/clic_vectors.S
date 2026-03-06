.extern interrupt_entry

#if __riscv_flen == 64
	#define FSTORE fsd
	#define FLOAD fld
	#define FREAG_SZ 8
#elif __riscv_flen == 32
	#define FSTORE fsw
	#define FLOAD flw
	#define FREAG_SZ 4
#else
	#error toolchain did not define __riscv_flen
#endif

.section .vectors, "aw", @progbits
	.balign 64
	.globl  clic_interrupt_vectors
	.type   clic_interrupt_vectors, @object
	clic_interrupt_vectors:
	.rept   (CONFIG_IRQ_SOUCE_COUNT)
	.long   clic_interrupt_handler
	.endr

.text
	.global clic_interrupt_handler
	.type   clic_interrupt_handler, %function
clic_interrupt_handler:
	addi sp, sp, -(16 * 4)
	sw ra, 0  * 4( sp )
	sw t0, 1  * 4( sp )
	sw t1, 2  * 4( sp )
	sw t2, 3  * 4( sp )
	sw t3, 4  * 4( sp )
	sw t4, 5  * 4( sp )
	sw t5, 6  * 4( sp )
	sw t6, 7  * 4( sp )
	sw a0, 8  * 4( sp )
	sw a1, 9  * 4( sp )
	sw a2, 10 * 4( sp )
	sw a3, 11 * 4( sp )
	sw a4, 12 * 4( sp )
	sw a5, 13 * 4( sp )
	sw a6, 14 * 4( sp )
	sw a7, 15 * 4( sp )

	addi sp, sp, -(20 * FREAG_SZ)
	FSTORE ft0,  0  * FREAG_SZ( sp )
	FSTORE ft1,  1  * FREAG_SZ( sp )
	FSTORE ft2,  2  * FREAG_SZ( sp )
	FSTORE ft3,  3  * FREAG_SZ( sp )
	FSTORE ft4,  4  * FREAG_SZ( sp )
	FSTORE ft5,  5  * FREAG_SZ( sp )
	FSTORE ft6,  6  * FREAG_SZ( sp )
	FSTORE ft7,  7  * FREAG_SZ( sp )
	FSTORE ft8,  8  * FREAG_SZ( sp )
	FSTORE ft9,  9  * FREAG_SZ( sp )
	FSTORE ft10, 10 * FREAG_SZ( sp )
	FSTORE ft11, 11 * FREAG_SZ( sp )
	FSTORE fa0,  12 * FREAG_SZ( sp )
	FSTORE fa1,  13 * FREAG_SZ( sp )
	FSTORE fa2,  14 * FREAG_SZ( sp )
	FSTORE fa3,  15 * FREAG_SZ( sp )
	FSTORE fa4,  16 * FREAG_SZ( sp )
	FSTORE fa5,  17 * FREAG_SZ( sp )
	FSTORE fa6,  18 * FREAG_SZ( sp )
	FSTORE fa7,  19 * FREAG_SZ( sp )

	call interrupt_entry

	FLOAD ft0,  0  * FREAG_SZ( sp )
	FLOAD ft1,  1  * FREAG_SZ( sp )
	FLOAD ft2,  2  * FREAG_SZ( sp )
	FLOAD ft3,  3  * FREAG_SZ( sp )
	FLOAD ft4,  4  * FREAG_SZ( sp )
	FLOAD ft5,  5  * FREAG_SZ( sp )
	FLOAD ft6,  6  * FREAG_SZ( sp )
	FLOAD ft7,  7  * FREAG_SZ( sp )
	FLOAD ft8,  8  * FREAG_SZ( sp )
	FLOAD ft9,  9  * FREAG_SZ( sp )
	FLOAD ft10, 10 * FREAG_SZ( sp )
	FLOAD ft11, 11 * FREAG_SZ( sp )
	FLOAD fa0,  12 * FREAG_SZ( sp )
	FLOAD fa1,  13 * FREAG_SZ( sp )
	FLOAD fa2,  14 * FREAG_SZ( sp )
	FLOAD fa3,  15 * FREAG_SZ( sp )
	FLOAD fa4,  16 * FREAG_SZ( sp )
	FLOAD fa5,  17 * FREAG_SZ( sp )
	FLOAD fa6,  18 * FREAG_SZ( sp )
	FLOAD fa7,  19 * FREAG_SZ( sp )
	addi sp, sp, (20 * FREAG_SZ)

	lw ra, 0  * 4( sp )
	lw t0, 1  * 4( sp )
	lw t1, 2  * 4( sp )
	lw t2, 3  * 4( sp )
	lw t3, 4  * 4( sp )
	lw t4, 5  * 4( sp )
	lw t5, 6  * 4( sp )
	lw t6, 7  * 4( sp )
	lw a0, 8  * 4( sp )
	lw a1, 9  * 4( sp )
	lw a2, 10 * 4( sp )
	lw a3, 11 * 4( sp )
	lw a4, 12 * 4( sp )
	lw a5, 13 * 4( sp )
	lw a6, 14 * 4( sp )
	lw a7, 15 * 4( sp )
	addi sp, sp, (16 * 4)

	mret
