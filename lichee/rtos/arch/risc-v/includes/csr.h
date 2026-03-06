/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef _RISCV_CSR_H
#define _RISCV_CSR_H

#include <consts.h>

/* Status register flags */
#define SR_SIE        0x00000002UL /* Supervisor Interrupt Enable */
#define SR_MIE        0x00000008UL /* Machine Interrupt Enable */
#define SR_SPIE       0x00000020UL /* Previous Supervisor IE */
#define SR_MPIE       0x00000080UL /* Previous Machine IE */
#define SR_SPP        0x00000100UL /* Previously Supervisor */
#define SR_MPP        0x00001800UL /* Previously Machine */
#define SR_SUM        0x00040000UL /* Supervisor User Memory Access */

#define MR_MPP        0x00001800UL /* Previously Machine */
#define MR_MPIE       0x00000080UL /* Previously Machine */
#define MR_MIE        0x00000008UL  /* Previously Machine */

#define SR_FS         0x00006000UL /* Floating-point Status */
#define SR_FS_OFF     0x00000000UL
#define SR_FS_INITIAL 0x00002000UL
#define SR_FS_CLEAN   0x00004000UL
#define SR_FS_DIRTY   0x00006000UL

#define SR_XS         0x00018000UL /* Extension Status */
#define SR_XS_OFF     0x00000000UL
#define SR_XS_INITIAL 0x00008000UL
#define SR_XS_CLEAN   0x00010000UL
#define SR_XS_DIRTY   0x00018000UL

#ifndef CONFIG_64BIT
#define SR_SD         0x80000000UL /* FS/XS dirty */
#else
#define SR_SD         0x8000000000000000UL /* FS/XS dirty */
#endif

/* SATP flags */
#ifndef CONFIG_64BIT
#define SATP_PPN        0x003FFFFFUL
#define SATP_MODE_32    0x80000000UL
#define SATP_MODE       SATP_MODE_32
#define SATP_ASID_BITS  9
#define SATP_ASID_SHIFT 22
#define SATP_ASID_MASK  0x1FFUL
#else
#define SATP_PPN        0x00000FFFFFFFFFFFUL
#define SATP_MODE_39    0x8000000000000000UL
#define SATP_MODE       SATP_MODE_39
#define SATP_ASID_BITS  16
#define SATP_ASID_SHIFT 44
#define SATP_ASID_MASK  0xFFFFUL
#endif

/* SCAUSE */
#define SCAUSE_IRQ_FLAG     (1UL << (__riscv_xlen - 1))

#define MCAUSE_IRQ_MASK     0xfffUL
#define IRQ_U_SOFT      0
#define IRQ_S_SOFT      1
#define IRQ_M_SOFT      3
#define IRQ_U_TIMER     4
#define IRQ_S_TIMER     5
#define IRQ_M_TIMER     7
#define IRQ_U_EXT       8
#define IRQ_S_EXT       9
#define IRQ_M_EXT       11
#define IRQ_S_PMU       17

#define EXC_INST_MISALIGNED     0
#define EXC_INST_ACCESS         1
#define EXC_INST_ILLEGAL        2
#define EXC_BREAKPOINT          3
#define EXC_LOAD_MISALIGN       4
#define EXC_LOAD_ACCESS         5
#define EXC_STORE_MISALIGN      6
#define EXC_STORE_ACCESS        7
#define EXC_SYSCALL_FRM_U       8
#define EXC_SYSCALL_FRM_S       9
#define EXC_SYSCALL_FRM_M       11
#define EXC_INST_PAGE_FAULT     12
#define EXC_LOAD_PAGE_FAULT     13
#define EXC_STORE_PAGE_FAULT    15

/* symbolic CSR names: */
#define CSR_CYCLE       0xc00
#define CSR_TIME        0xc01
#define CSR_INSTRET     0xc02
#define CSR_CYCLEH      0xc80
#define CSR_TIMEH       0xc81
#define CSR_INSTRETH    0xc82

#define CSR_SSTATUS     0x100
#define CSR_SIE         0x104
#define CSR_STVEC       0x105
#define CSR_SCOUNTEREN  0x106
#define CSR_SSCRATCH    0x140
#define CSR_SEPC        0x141
#define CSR_SCAUSE      0x142
#define CSR_STVAL       0x143
#define CSR_SIP         0x144
#define CSR_SATP        0x180

/* M-Mode System Registers */
#define MIE_MSIE        (1UL << IRQ_M_SOFT)
#define MIE_MTIE        (1UL << IRQ_M_TIMER)
#define MIE_MEIE        (1UL << IRQ_M_EXT)

#define CSR_MSTATUS     0x300
#define CSR_MISA        0x301
#define CSR_MEDELEG     0x302
#define CSR_MIDELEG     0x303
#define CSR_MIE         0x304
#define CSR_MTVEC       0x305
#define CSR_MCOUNTEREN  0x306

#define CSR_MSCRATCH    0x340
#define CSR_MEPC        0x341
#define CSR_MCAUSE      0x342
#define CSR_MTVAL       0x343
#define CSR_MIP         0x344

#define CSR_MCOR         0x7c2
#define CSR_MHCR         0x7c1
#define CSR_MCCR2        0x7c3
#define CSR_MHINT        0x7c5
#define CSR_MXSTATUS     0x7c0
#define CSR_PLIC_BASE    0xfc1
#define CSR_MRMR         0x7c6
#define CSR_MRVBR        0x7c7

// REF - RISC-V External Debug Support Version 0.13-DRAFT
#define CSR_TSELECT         0x7a0
#define CSR_TDATA1          0x7a1
#define CSR_MCONTROL        0x7a1
#define CSR_ICOUNT          0x7a1
#define CSR_ITRIGGER        0x7a1
#define CSR_ETRIGGER        0x7a1
#define CSR_TDATA2          0x7a2
#define CSR_TDATA3          0x7a3
#define CSR_TEXTRA32        0x7a3
#define CSR_TEXTRA64        0x7a3
#define CSR_TINFO           0x7a4
#define CSR_TCONCTRL        0x7a5
#define CSR_MCONTEXT        0x7a8
#define CSR_SCONTEXT        0x7aa

#ifdef __ASSEMBLY__
#define __ASM_STR(x)    x
#else
#define __ASM_STR(x)    #x
#endif

#ifndef __ASSEMBLY__
#define csr_swap(csr, val)                  \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrrw %0, " __ASM_STR(csr) ", %1"\
                              : "=r" (__v) : "rK" (__v)     \
                              : "memory");          \
        __v;                            \
    })

#define csr_read(csr)                       \
    ({                              \
        register unsigned long __v;             \
        __asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)    \
                              : "=r" (__v) :            \
                              : "memory");          \
        __v;                            \
    })

#define csr_write(csr, val)                 \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrw " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#define csr_read_set(csr, val)                  \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrrs %0, " __ASM_STR(csr) ", %1"\
                              : "=r" (__v) : "rK" (__v)     \
                              : "memory");          \
        __v;                            \
    })

#define csr_set(csr, val)                   \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrs " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#define csr_read_clear(csr, val)                \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrrc %0, " __ASM_STR(csr) ", %1"\
                              : "=r" (__v) : "rK" (__v)     \
                              : "memory");          \
        __v;                            \
    })

#define csr_clear(csr, val)                 \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrc " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#endif

#endif /* _RISCV_CSR_H */
