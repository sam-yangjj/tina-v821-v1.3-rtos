#ifndef  __EXPRISCV_INC__
#define  __EXPRISCV_INC__

#ifndef __ASSEMBLY__

#include <stdint.h>

// zero, x2(sp), x3(tp) need not backup.
typedef struct
{
    unsigned long mepc;            // 0 * __SIZEOF_LONG__
    unsigned long x1;              // 1 * __SIZEOF_LONG__
    unsigned long x5;              // 5 * __SIZEOF_LONG__
    unsigned long x6;              // 6 * __SIZEOF_LONG__
    unsigned long x7;              // 7 * __SIZEOF_LONG__
    unsigned long x8;              // 8 * __SIZEOF_LONG__
    unsigned long x9;              // 9 * __SIZEOF_LONG__
    unsigned long x10;             //10 * __SIZEOF_LONG__
    unsigned long x11;             //11 * __SIZEOF_LONG__
    unsigned long x12;             //12 * __SIZEOF_LONG__
    unsigned long x13;             //13 * __SIZEOF_LONG__
    unsigned long x14;             //14 * __SIZEOF_LONG__
    unsigned long x15;             //15 * __SIZEOF_LONG__
    unsigned long x16;             //16 * __SIZEOF_LONG__
    unsigned long x17;             //17 * __SIZEOF_LONG__
    unsigned long x18;             //18 * __SIZEOF_LONG__
    unsigned long x19;             //19 * __SIZEOF_LONG__
    unsigned long x20;             //20 * __SIZEOF_LONG__
    unsigned long x21;             //21 * __SIZEOF_LONG__
    unsigned long x22;             //22 * __SIZEOF_LONG__
    unsigned long x23;             //23 * __SIZEOF_LONG__
    unsigned long x24;             //24 * __SIZEOF_LONG__
    unsigned long x25;             //25 * __SIZEOF_LONG__
    unsigned long x26;             //26 * __SIZEOF_LONG__
    unsigned long x27;             //27 * __SIZEOF_LONG__
    unsigned long x28;             //28 * __SIZEOF_LONG__
    unsigned long x29;             //29 * __SIZEOF_LONG__
    unsigned long x30;             //30 * __SIZEOF_LONG__
    unsigned long x31;             //31 * __SIZEOF_LONG__
    unsigned long mstatus;         //32 * __SIZEOF_LONG__
    unsigned long x2;              // 2 * __SIZEOF_LONG__
    unsigned long x3;              // 3 * __SIZEOF_LONG__
    unsigned long x4;              // 4 * __SIZEOF_LONG__
    unsigned long mscratch;        //33 * __SIZEOF_LONG__
} irq_regs_t;

#endif

#endif
