/*
 * Allwinnertech rtos interrupt header file.
 * Copyright (C) 2019  Allwinnertech Co., Ltd. or its affiliates.  All Rights Reserved.
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
 */

#ifndef __INTERRUPT_GIC600_H
#define __INTERRUPT_GIC600_H

typedef enum {
    IntGroupG0  = 0,// GRPMODR == 0 && GROUP == 0
    IntGroupG1S = 1,// GRPMODR == 1 && GROUP == 0
    IntGroupG1NS= 2,// GRPMODR == 0 && GROUP == 1
} GICv3IntGroup;

/*GIC*/
//#define GIC_BASE            (0x03020000)
#define GIC_DIST_BASE       (0x03400000)
#define GIC_BASE            GIC_DIST_BASE
#define GICA_BASE           (0x03410000)
#define GICT_BASE           (0x03420000)
#define GICP_BASE           (0x03430000)
#define GITS_BASE           (0x03440000)
#define GITT_BASE           (0x03450000)
#define GICR_LPI_BASE(n)    (0x03460000 + n * 0x20000)
#define GICR_SGI_BASE(n)    (0x03470000 + n * 0x20000)
#define GICR0_LPI_BASE      (0x03460000)
#define GICR0_SGI_BASE      (0x03470000)
#define GICR1_LPI_BASE      (0x03480000)
#define GICR1_SGI_BASE      (0x03490000)
#define GICR2_LPI_BASE      (0x034a0000)
#define GICR2_SGI_BASE      (0x034b0000)
#define GICR3_LPI_BASE      (0x034c0000)
#define GICR4_SGI_BASE      (0x034d0000)
#define GICDA_BASE          (0x034e0000)
#define GIC_CPUIF_BASE      (0x03022000)


/* GIC registers */
#define GIC_DIST_CON        (GIC_DIST_BASE + 0x0000)
#define GIC_CON_TYPE        (GIC_DIST_BASE + 0x0004)
#define GIC_CON_IIDR        (GIC_DIST_BASE + 0x0008)

#define GIC_CON_IGRP        (GIC_DIST_BASE + 0x0080)

#define GIC_SET_EN(_n)      (GIC_DIST_BASE + 0x100 + 4 * (_n))
#define GIC_SET_EN0         GIC_SET_EN(0)   // 0x100
#define GIC_SET_EN1         GIC_SET_EN(1)   // 0x104
#define GIC_SET_EN2         GIC_SET_EN(2)   // 0x108
#define GIC_SET_EN3         GIC_SET_EN(3)   // 0x10c
#define GIC_SET_EN4         GIC_SET_EN(4)   // 0x110

#define GIC_CLR_EN(_n)      (GIC_DIST_BASE + 0x180 + 4 * (_n))
#define GIC_CLR_EN0         GIC_CLR_EN(0)   // 0x180
#define GIC_CLR_EN1         GIC_CLR_EN(1)   // 0x184
#define GIC_CLR_EN2         GIC_CLR_EN(2)   // 0x188
#define GIC_CLR_EN3         GIC_CLR_EN(3)   // 0x18c
#define GIC_CLR_EN4         GIC_CLR_EN(4)   // 0x190

#define GIC_PEND_SET(_n)    (GIC_DIST_BASE + 0x200 + 4 * (_n))
#define GIC_PEND_SET0       GIC_PEND_SET(0) // 0x200
#define GIC_PEND_SET1       GIC_PEND_SET(1) // 0x204
#define GIC_PEND_SET2       GIC_PEND_SET(2) // 0x208
#define GIC_PEND_SET3       GIC_PEND_SET(3) // 0x20c
#define GIC_PEND_SET4       GIC_PEND_SET(4) // 0x210

#define GIC_PEND_CLR(_n)    (GIC_DIST_BASE + 0x280 + 4 * (_n))
#define GIC_PEND_CLR0       GIC_PEND_CLR(0) // 0x280
#define GIC_PEND_CLR1       GIC_PEND_CLR(1) // 0x284
#define GIC_PEND_CLR2       GIC_PEND_CLR(2) // 0x288
#define GIC_PEND_CLR3       GIC_PEND_CLR(3) // 0x28c
#define GIC_PEND_CLR4       GIC_PEND_CLR(4) // 0x290

#define GIC_ACT_SET(_n)     (GIC_DIST_BASE + 0x300 + 4 * (_n))
#define GIC_ACT_SET0        GIC_ACT_SET(0)  // 0x300
#define GIC_ACT_SET1        GIC_ACT_SET(1)  // 0x304
#define GIC_ACT_SET2        GIC_ACT_SET(2)  // 0x308
#define GIC_ACT_SET3        GIC_ACT_SET(3)  // 0x30c
#define GIC_ACT_SET4        GIC_ACT_SET(4)  // 0x310

#define GIC_ACT_CLR(_n)     (GIC_DIST_BASE + 0x380 + 4 * (_n))
#define GIC_ACT_CLR0        GIC_ACT_CLR(0)  // 0x380
#define GIC_ACT_CLR1        GIC_ACT_CLR(1)  // 0x384
#define GIC_ACT_CLR2        GIC_ACT_CLR(2)  // 0x388
#define GIC_ACT_CLR3        GIC_ACT_CLR(3)  // 0x38c
#define GIC_ACT_CLR4        GIC_ACT_CLR(4)  // 0x390

#define GIC_SGI_PRIO(_n)    (GIC_DIST_BASE + 0x400 + 4 * (_n))
#define GIC_SGI_PRIO0       GIC_SGI_PRIO(0) // 0x400
#define GIC_SGI_PRIO1       GIC_SGI_PRIO(1) // 0x404
#define GIC_SGI_PRIO2       GIC_SGI_PRIO(2) // 0x408
#define GIC_SGI_PRIO3       GIC_SGI_PRIO(3) // 0x40C

#define GIC_PPI_PRIO(_n)    (GIC_DIST_BASE + 0x410 + 4 * (_n))
#define GIC_PPI_PRIO0       GIC_PPI_PRIO(0) // 0x410
#define GIC_PPI_PRIO1       GIC_PPI_PRIO(1) // 0x414
#define GIC_PPI_PRIO2       GIC_PPI_PRIO(2) // 0x418
#define GIC_PPI_PRIO3       GIC_PPI_PRIO(3) // 0x41C

#define GIC_SPI_PRIO(_n)    (GIC_DIST_BASE + 0x420 + 4 * (_n))
#define GIC_SPI_PRIO0       GIC_SPI_PRIO(0 )    // 0x420
#define GIC_SPI_PRIO1       GIC_SPI_PRIO(1 )    // 0x424
#define GIC_SPI_PRIO2       GIC_SPI_PRIO(2 )    // 0x428
#define GIC_SPI_PRIO3       GIC_SPI_PRIO(3 )    // 0x42C
#define GIC_SPI_PRIO4       GIC_SPI_PRIO(4 )    // 0x430
#define GIC_SPI_PRIO5       GIC_SPI_PRIO(5 )    // 0x434
#define GIC_SPI_PRIO6       GIC_SPI_PRIO(6 )    // 0x438
#define GIC_SPI_PRIO7       GIC_SPI_PRIO(7 )    // 0x43C
#define GIC_SPI_PRIO8       GIC_SPI_PRIO(8 )    // 0x440
#define GIC_SPI_PRIO9       GIC_SPI_PRIO(9 )    // 0x444
#define GIC_SPI_PRIO10      GIC_SPI_PRIO(10)    // 0x448
#define GIC_SPI_PRIO11      GIC_SPI_PRIO(11)    // 0x44C
#define GIC_SPI_PRIO12      GIC_SPI_PRIO(12)    // 0x450
#define GIC_SPI_PRIO13      GIC_SPI_PRIO(13)    // 0x454
#define GIC_SPI_PRIO14      GIC_SPI_PRIO(14)    // 0x458
#define GIC_SPI_PRIO15      GIC_SPI_PRIO(15)    // 0x45C
#define GIC_SPI_PRIO16      GIC_SPI_PRIO(16)    // 0x460
#define GIC_SPI_PRIO17      GIC_SPI_PRIO(17)    // 0x464
#define GIC_SPI_PRIO18      GIC_SPI_PRIO(18)    // 0x468
#define GIC_SPI_PRIO19      GIC_SPI_PRIO(19)    // 0x46C
#define GIC_SPI_PRIO20      GIC_SPI_PRIO(20)    // 0x470
#define GIC_SPI_PRIO21      GIC_SPI_PRIO(21)    // 0x474
#define GIC_SPI_PRIO22      GIC_SPI_PRIO(22)    // 0x478
#define GIC_SPI_PRIO23      GIC_SPI_PRIO(23)    // 0x47C
#define GIC_SPI_PRIO24      GIC_SPI_PRIO(24)    // 0x480
#define GIC_SPI_PRIO25      GIC_SPI_PRIO(25)    // 0x484
#define GIC_SPI_PRIO26      GIC_SPI_PRIO(26)    // 0x488
#define GIC_SPI_PRIO27      GIC_SPI_PRIO(27)    // 0x48C
#define GIC_SPI_PRIO28      GIC_SPI_PRIO(28)    // 0x490
#define GIC_SPI_PRIO29      GIC_SPI_PRIO(29)    // 0x494
#define GIC_SPI_PRIO30      GIC_SPI_PRIO(30)    // 0x498
#define GIC_SPI_PRIO31      GIC_SPI_PRIO(31)    // 0x49C

#define GIC_SGI_PROC_TARG(_n)   (GIC_DIST_BASE + 0x800 + 4 * (_n))
#define GIC_SGI_PROC_TARG0      GIC_SGI_PROC_TARG(0)    // 0x800
#define GIC_SGI_PROC_TARG1      GIC_SGI_PROC_TARG(1)    // 0x804
#define GIC_SGI_PROC_TARG2      GIC_SGI_PROC_TARG(2)    // 0x808
#define GIC_SGI_PROC_TARG3      GIC_SGI_PROC_TARG(3)    // 0x80C

#define GIC_PPI_PROC_TARG(_n)   (GIC_DIST_BASE + 0x810 + 4 * (_n))
#define GIC_PPI_PROC_TARG0      GIC_PPI_PROC_TARG(0)    // 0x810
#define GIC_PPI_PROC_TARG1      GIC_PPI_PROC_TARG(1)    // 0x814
#define GIC_PPI_PROC_TARG2      GIC_PPI_PROC_TARG(2)    // 0x818
#define GIC_PPI_PROC_TARG3      GIC_PPI_PROC_TARG(3)    // 0x81C

#define GIC_SPI_PROC_TARG(_n)   (GIC_DIST_BASE + 0x820 + 4 * (_n))
#define GIC_SPI_PROC_TARG0      GIC_SPI_PROC_TARG(0 )   // 0x820
#define GIC_SPI_PROC_TARG1      GIC_SPI_PROC_TARG(1 )   // 0x824
#define GIC_SPI_PROC_TARG2      GIC_SPI_PROC_TARG(2 )   // 0x828
#define GIC_SPI_PROC_TARG3      GIC_SPI_PROC_TARG(3 )   // 0x82C
#define GIC_SPI_PROC_TARG4      GIC_SPI_PROC_TARG(4 )   // 0x830
#define GIC_SPI_PROC_TARG5      GIC_SPI_PROC_TARG(5 )   // 0x834
#define GIC_SPI_PROC_TARG6      GIC_SPI_PROC_TARG(6 )   // 0x838
#define GIC_SPI_PROC_TARG7      GIC_SPI_PROC_TARG(7 )   // 0x83C
#define GIC_SPI_PROC_TARG8      GIC_SPI_PROC_TARG(8 )   // 0x840
#define GIC_SPI_PROC_TARG9      GIC_SPI_PROC_TARG(9 )   // 0x844
#define GIC_SPI_PROC_TARG10     GIC_SPI_PROC_TARG(10)   // 0x848
#define GIC_SPI_PROC_TARG11     GIC_SPI_PROC_TARG(11)   // 0x84C
#define GIC_SPI_PROC_TARG12     GIC_SPI_PROC_TARG(12)   // 0x850
#define GIC_SPI_PROC_TARG13     GIC_SPI_PROC_TARG(13)   // 0x854
#define GIC_SPI_PROC_TARG14     GIC_SPI_PROC_TARG(14)   // 0x858
#define GIC_SPI_PROC_TARG15     GIC_SPI_PROC_TARG(15)   // 0x85C
#define GIC_SPI_PROC_TARG16     GIC_SPI_PROC_TARG(16)   // 0x860
#define GIC_SPI_PROC_TARG17     GIC_SPI_PROC_TARG(17)   // 0x864
#define GIC_SPI_PROC_TARG18     GIC_SPI_PROC_TARG(18)   // 0x868
#define GIC_SPI_PROC_TARG19     GIC_SPI_PROC_TARG(19)   // 0x86C
#define GIC_SPI_PROC_TARG20     GIC_SPI_PROC_TARG(20)   // 0x870
#define GIC_SPI_PROC_TARG21     GIC_SPI_PROC_TARG(21)   // 0x874
#define GIC_SPI_PROC_TARG22     GIC_SPI_PROC_TARG(22)   // 0x878
#define GIC_SPI_PROC_TARG23     GIC_SPI_PROC_TARG(23)   // 0x87C
#define GIC_SPI_PROC_TARG24     GIC_SPI_PROC_TARG(24)   // 0x880
#define GIC_SPI_PROC_TARG25     GIC_SPI_PROC_TARG(25)   // 0x884
#define GIC_SPI_PROC_TARG26     GIC_SPI_PROC_TARG(26)   // 0x888
#define GIC_SPI_PROC_TARG27     GIC_SPI_PROC_TARG(27)   // 0x88C
#define GIC_SPI_PROC_TARG28     GIC_SPI_PROC_TARG(28)   // 0x890
#define GIC_SPI_PROC_TARG29     GIC_SPI_PROC_TARG(29)   // 0x894
#define GIC_SPI_PROC_TARG30     GIC_SPI_PROC_TARG(30)   // 0x898
#define GIC_SPI_PROC_TARG31     GIC_SPI_PROC_TARG(31)   // 0x89C

#define GIC_IRQ_TYPE_CFG(_n)     (GIC_DIST_BASE + 0xC00 + 4 * (_n))

#define GIC_IRQ_MOD_CFG(_n)     (GIC_DIST_BASE + 0xD00 + 4 * (_n))
#define GIC_IRQ_MOD_CFG0        GIC_IRQ_MOD_CFG(0)      // 0xc00 - SGI
#define GIC_IRQ_MOD_CFG1        GIC_IRQ_MOD_CFG(1)      // 0xc04 - PPI
#define GIC_IRQ_MOD_CFG2        GIC_IRQ_MOD_CFG(2)      // 0xc08 - SPI0 ~ 15
#define GIC_IRQ_MOD_CFG3        GIC_IRQ_MOD_CFG(3)      // 0xc0C - SPI16 ~ 31
#define GIC_IRQ_MOD_CFG4        GIC_IRQ_MOD_CFG(4)      // 0xc10 - SPI32 ~ 47
#define GIC_IRQ_MOD_CFG5        GIC_IRQ_MOD_CFG(5)      // 0xc14 - SPI48 ~ 63
#define GIC_IRQ_MOD_CFG6        GIC_IRQ_MOD_CFG(6)      // 0xc18 - SPI64 ~ 79
#define GIC_IRQ_MOD_CFG7        GIC_IRQ_MOD_CFG(7)      // 0xc1C - SPI80 ~ 95
#define GIC_IRQ_MOD_CFG8        GIC_IRQ_MOD_CFG(8)      // 0xc20 - SPI96 ~ 111
#define GIC_IRQ_MOD_CFG9        GIC_IRQ_MOD_CFG(9)      // 0xc24 - SPI112 ~ 127

#define GIC_SOFT_IRQ_GEN        (GIC_DIST_BASE + 0xf00) // 0xf00
#define GIC_SGI_PEND_CLR(_n)    (GIC_DIST_BASE + 0xf10 + 4 * (_n))
#define GIC_SGI_PEND_CLR0       GIC_SGI_PEND_CLR(0)     // 0xf10
#define GIC_SGI_PEND_CLR1       GIC_SGI_PEND_CLR(1)     // 0xf14
#define GIC_SGI_PEND_CLR2       GIC_SGI_PEND_CLR(2)     // 0xf18
#define GIC_SGI_PEND_CLR3       GIC_SGI_PEND_CLR(3)     // 0xf1C
#define GIC_SGI_PEND_SET(_n)    (GIC_DIST_BASE + 0xf20 + 4 * (_n))
#define GIC_SGI_PEND_SET0       GIC_SGI_PEND_SET(0)     // 0xf20
#define GIC_SGI_PEND_SET1       GIC_SGI_PEND_SET(1)     // 0xf24
#define GIC_SGI_PEND_SET2       GIC_SGI_PEND_SET(2)     // 0xf28
#define GIC_SGI_PEND_SET3       GIC_SGI_PEND_SET(3)     // 0xf2C


/***************************Added for GICv3*****************************/
#define GIC_IROUTR(_n)           (GIC_DIST_BASE + 0x6000 + 8 * (_n))            //64bit
#define GIC_CHIPSR               (GIC_DIST_BASE + 0xC000)
#define GIC_DCHIPR               (GIC_DIST_BASE + 0xC004)
#define GIC_CHIPR(_n)            (GIC_DIST_BASE + 0xC008 + 8 * (_n))            //64bit
#define GIC_ICLAR(_n)            (GIC_DIST_BASE + 0xE008 + 4 * (_n))
#define GIC_IERRR(_n)            (GIC_DIST_BASE + 0xE108 + 4 * (_n))
#define GIC_CFGID                (GIC_DIST_BASE + 0xF000)                    //64bit
#define GIC_PIDR4                (GIC_DIST_BASE + 0xFFD0)
#define GIC_PIDR5                (GIC_DIST_BASE + 0xFFD4)
#define GIC_PIDR6                (GIC_DIST_BASE + 0xFFD8)
#define GIC_PIDR7                (GIC_DIST_BASE + 0xFFDC)
#define GIC_PIDR0                (GIC_DIST_BASE + 0xFFE0)
#define GIC_PIDR1                (GIC_DIST_BASE + 0xFFE4)
#define GIC_PIDR2                (GIC_DIST_BASE + 0xFFE8)
#define GIC_PIDR3                (GIC_DIST_BASE + 0xFFEC)

#define GIC_CIDR0                (GIC_DIST_BASE + 0xFFF0)
#define GIC_CIDR1                (GIC_DIST_BASE + 0xFFF4)
#define GIC_CIDR2                (GIC_DIST_BASE + 0xFFF8)
#define GIC_CIDR3                (GIC_DIST_BASE + 0xFFFC)

/***************************Added for GICv3, GICA message-based SPIs*****************************/
#define GICA_SETSPI_NSR          (GICA_BASE + 0x40)
#define GICA_CLRSPI_NSR          (GICA_BASE + 0x48)
#define GICA_SETSPI_SR           (GICA_BASE + 0x50)
#define GICA_CLRSPI_SR           (GICA_BASE + 0x58)


/***************************Added for GICv3, control and physical LPIs*****************************/
#define GICR_CTRL(m)             (GICR_LPI_BASE(m) + 0x0000)
#define GICR_IIDR(m)             (GICR_LPI_BASE(m) + 0x0004)
#define GICR_TYPER(m)            (GICR_LPI_BASE(m) + 0x0008)
#define GICR_WAKER(m)            (GICR_LPI_BASE(m) + 0x0014)
#define GICR_FCTLR(m)            (GICR_LPI_BASE(m) + 0x0020)
#define GICR_PWRR(m)             (GICR_LPI_BASE(m) + 0x0024)
#define GICR_CLASS(m)            (GICR_LPI_BASE(m) + 0x0028)
#define GICR_SETLPIR(m)          (GICR_LPI_BASE(m) + 0x0040)
#define GICR_CLRLPIR(m)          (GICR_LPI_BASE(m) + 0x0048)
#define GICR_PROPBASER(m)        (GICR_LPI_BASE(m) + 0x0070)
#define GICR_PENDBASER(m)        (GICR_LPI_BASE(m) + 0x0078)
#define GICR_INVLPIR(m)          (GICR_LPI_BASE(m) + 0x00A0)
#define GICR_INVAKKR(m)          (GICR_LPI_BASE(m) + 0x00B0)
#define GICR_SYBCR(m)            (GICR_LPI_BASE(m) + 0x00C0)


/***************************Added for GICv3, redistributor SGIs and PPIs*****************************/
#define GICR_IGROUPR(m, n)       (GICR_SGI_BASE(m) + 0x0080 + 4 * (n))
#define GICR_ISENABLER(m, n)     (GICR_SGI_BASE(m) + 0x0100 + 4 * (n))
#define GICR_ICENABLER(m, n)     (GICR_SGI_BASE(m) + 0x0100 + 4 * (n))
#define GICR_ISPENDR(m)          (GICR_SGI_BASE(m) + 0x0200)
#define GICR_ICPENDR(m)          (GICR_SGI_BASE(m) + 0x0280)
#define GICR_ISACTIVER(m)        (GICR_SGI_BASE(m) + 0x0300)
#define GICR_ICACTIVER(m)        (GICR_SGI_BASE(m) + 0x0380)
#define GICR_IPRIORITYR(m, n)    (GICR_SGI_BASE(m) + 0x0400 + 4 * (n))
#define GICR_ICFGR_SGI(m, n)     (GICR_SGI_BASE(m) + 0x0C00 + 4 * (n))
#define GICR_ICFGR_PPI(m, n)     (GICR_SGI_BASE(m) + 0x0C04 + 4 * (n))
#define GICR_IGRPMODR0(m, n)     (GICR_SGI_BASE(m) + 0x0D00 + 4 * (n))
#define GICR_IGNSACR(m)          (GICR_SGI_BASE(m) + 0x0E00)
#define GICR_MISCSTATUSR(m)      (GICR_SGI_BASE(m) + 0xC000)
#define GICR_IERRVR(m)           (GICR_SGI_BASE(m) + 0xC008)
#define GICR_SGIDR(m)            (GICR_SGI_BASE(m) + 0xC010)
#define GICR_CFGID0(m)           (GICR_SGI_BASE(m) + 0xF000)
#define GICR_CFGID1(m)           (GICR_SGI_BASE(m) + 0xF004)


/***************************Added for GICv3, ITS control*****************************/
#define GITS_CTLR                (GITS_BASE + 0x0000)
#define GITS_IIDR                (GITS_BASE + 0x0004)
#define GITS_TYPER                (GITS_BASE + 0x0008)        //64bit
#define GITS_FCTLR                (GITS_BASE + 0x0020)
#define GITS_OPR                (GITS_BASE + 0x0028)        //64bit
#define GITS_OPSR                (GITS_BASE + 0x0030)        //64bit
#define GITS_CBASER                (GITS_BASE + 0x0080)        //64bit
#define GITS_CWRITER            (GITS_BASE + 0x0088)        //64bit
#define GITS_CREADR                (GITS_BASE + 0x0090)        //64bit
#define GITS_BASER0                (GITS_BASE + 0x0100)        //64bit
#define GITS_BASER1                (GITS_BASE + 0x0108)        //64bit
#define GITS_CFGID                (GITS_BASE + 0xF000)
#define GITS_PIDR4                (GITS_BASE + 0xFFD0)
#define GITS_PIDR5                (GITS_BASE + 0xFFD4)
#define GITS_PIDR6                (GITS_BASE + 0xFFD8)
#define GITS_PIDR7                (GITS_BASE + 0xFFDC)
#define GITS_PIDR0                (GITS_BASE + 0xFFE0)
#define GITS_PIDR1                (GITS_BASE + 0xFFE4)
#define GITS_PIDR2                (GITS_BASE + 0xFFE8)
#define GITS_PIDR3                (GITS_BASE + 0xFFEC)
#define GITS_CIDR0                (GITS_BASE + 0xFFF0)
#define GITS_CIDR1                (GITS_BASE + 0xFFF4)
#define GITS_CIDR2                (GITS_BASE + 0xFFF8)
#define GITS_CIDR3                (GITS_BASE + 0xFFFC)

/***************************Added for GICv3, ITS translation*****************************/
#define GITS_TRANSR                (GITT_BASE + 0x0040)

#define GIC_CPU_IF_CTRL         (GIC_CPUIF_BASE + 0x000)    // 0x000
#define GIC_INT_PRIO_MASK       (GIC_CPUIF_BASE + 0x004) // 0x004
#define GIC_BINARY_POINT        (GIC_CPUIF_BASE + 0x008) // 0x008
#define GIC_INT_ACK_REG         (GIC_CPUIF_BASE + 0x00c) // 0x00c
#define GIC_END_INT_REG         (GIC_CPUIF_BASE + 0x010) // 0x010
#define GIC_RUNNING_PRIO        (GIC_CPUIF_BASE + 0x014) // 0x014
#define GIC_HIGHEST_PENDINT     (GIC_CPUIF_BASE + 0x018) // 0x018

#define GIC_DEACT_INT_REG       (GIC_CPUIF_BASE + 0x1000)// 0x1000

/*Added by gaojiangtao, for LPI, 2021-9-27*/
#define REDISTRIBUTOR_PRO_BASE_ADDR         (u64)0x0000000050000000                     //align 4KB
#define LPI_PENDING_TABLE_BASE_ADDR(m)      (u64)(0x0000000050010000 + m*0x10000)         //align 64KB
#define GITS_BASE_ADDR0                     (u64)0x0000000050100000                    //align 4KB/16KB/64KB
#define GITS_BASE_ADDR1                     (u64)0x0000000050200000                    //align 4KB/16KB/64KB
#define ITS_CMD_QUEUE_BASE_ADDR             (u64)0x0000000050300000                     //align 4KB
#define ITT_BASE_ADDR                       (u64)0x0000000050400000


/***********************gic source list*********************************/
/* software generated interrupt */
#define GIC_SRC_SGI(_n)         (_n)
#define GIC_SRC_SGI0            GIC_SRC_SGI(0 )  // (0 )
#define GIC_SRC_SGI1            GIC_SRC_SGI(1 )  // (1 )
#define GIC_SRC_SGI2            GIC_SRC_SGI(2 )  // (2 )
#define GIC_SRC_SGI3            GIC_SRC_SGI(3 )  // (3 )
#define GIC_SRC_SGI4            GIC_SRC_SGI(4 )  // (4 )
#define GIC_SRC_SGI5            GIC_SRC_SGI(5 )  // (5 )
#define GIC_SRC_SGI6            GIC_SRC_SGI(6 )  // (6 )
#define GIC_SRC_SGI7            GIC_SRC_SGI(7 )  // (7 )
#define GIC_SRC_SGI8            GIC_SRC_SGI(8 )  // (8 )
#define GIC_SRC_SGI9            GIC_SRC_SGI(9 )  // (9 )
#define GIC_SRC_SGI10           GIC_SRC_SGI(10)  // (10)
#define GIC_SRC_SGI11           GIC_SRC_SGI(11)  // (11)
#define GIC_SRC_SGI12           GIC_SRC_SGI(12)  // (12)
#define GIC_SRC_SGI13           GIC_SRC_SGI(13)  // (13)
#define GIC_SRC_SGI14           GIC_SRC_SGI(14)  // (14)
#define GIC_SRC_SGI15           GIC_SRC_SGI(15)  // (15)
/* private peripheral interrupt */
#define GIC_SRC_PPI(_n)         (16 + (_n))
#define GIC_SRC_PPI0            GIC_SRC_PPI(0 )  // (16)
#define GIC_SRC_PPI1            GIC_SRC_PPI(1 )  // (17)
#define GIC_SRC_PPI2            GIC_SRC_PPI(2 )  // (18)
#define GIC_SRC_PPI3            GIC_SRC_PPI(3 )  // (19)
#define GIC_SRC_PPI4            GIC_SRC_PPI(4 )  // (20)
#define GIC_SRC_PPI5            GIC_SRC_PPI(5 )  // (21)
#define GIC_SRC_PPI6            GIC_SRC_PPI(6 )  // (22)
#define GIC_SRC_PPI7            GIC_SRC_PPI(7 )  // (23)
#define GIC_SRC_PPI8            GIC_SRC_PPI(8 )  // (24)
#define GIC_SRC_PPI9            GIC_SRC_PPI(9 )  // (25)
#define GIC_SRC_PPI10           GIC_SRC_PPI(10)  // (26)
#define GIC_SRC_PPI11           GIC_SRC_PPI(11)  // (27)
#define GIC_SRC_PPI12           GIC_SRC_PPI(12)  // (28)
#define GIC_SRC_PPI13           GIC_SRC_PPI(13)  // (29)
#define GIC_SRC_PPI14           GIC_SRC_PPI(14)  // (30)
#define GIC_SRC_PPI15           GIC_SRC_PPI(15)  // (31)
/* external peripheral interrupt */
#define GIC_SRC_SPI(_n)         (32 + (_n))

//Interrupt Source Begin

#define GIC_SRC_CPUX_MBOX_R                     GIC_SRC_SPI(32-32)
#define GIC_SRC_UART0                           GIC_SRC_SPI(34-32)
#define GIC_SRC_UART1                           GIC_SRC_SPI(35-32)
#define GIC_SRC_UART2                           GIC_SRC_SPI(36-32)
#define GIC_SRC_UART3                           GIC_SRC_SPI(37-32)
#define GIC_SRC_UART4                           GIC_SRC_SPI(38-32)
#define GIC_SRC_UART5                           GIC_SRC_SPI(39-32)
#define GIC_SRC_UART6                           GIC_SRC_SPI(40-32)
#define GIC_SRC_UART7                           GIC_SRC_SPI(41-32)
#define GIC_SRC_TWI0                            GIC_SRC_SPI(42-32)
#define GIC_SRC_TWI1                            GIC_SRC_SPI(43-32)
#define GIC_SRC_TWI2                            GIC_SRC_SPI(44-32)
#define GIC_SRC_TWI3                            GIC_SRC_SPI(45-32)
#define GIC_SRC_TWI4                            GIC_SRC_SPI(46-32)
#define GIC_SRC_TWI5                            GIC_SRC_SPI(47-32)
#define GIC_SRC_SPI0                            GIC_SRC_SPI(48-32)
#define GIC_SRC_SPI1                            GIC_SRC_SPI(49-32)
#define GIC_SRC_SPI2                            GIC_SRC_SPI(50-32)
#define GIC_SRC_PWM0                            GIC_SRC_SPI(51-32)
#define GIC_SRC_SPIF                            GIC_SRC_SPI(52-32)
#define GIC_SRC_SPI3                            GIC_SRC_SPI(53-32)
#define GIC_SRC_SPI4                            GIC_SRC_SPI(54-32)
#define GIC_SRC_SPDIF                           GIC_SRC_SPI(55-32)
#define GIC_SRC_DMIC                            GIC_SRC_SPI(56-32)
#define GIC_SRC_ADDA                            GIC_SRC_SPI(57-32)
#define GIC_SRC_IRTX                            GIC_SRC_SPI(58-32)
#define GIC_SRC_IRRX0                           GIC_SRC_SPI(59-32)
#define GIC_SRC_IRRX                            GIC_SRC_IRRX0
#define GIC_SRC_LEDC                            GIC_SRC_SPI(60-32)
#define GIC_SRC_USB0_DEVICE                     GIC_SRC_SPI(61-32)
#define GIC_SRC_USB0_EHCI                       GIC_SRC_SPI(62-32)
#define GIC_SRC_USB0_OHCI                       GIC_SRC_SPI(63-32)
#define GIC_SRC_USB1_EHCI                       GIC_SRC_SPI(64-32)
#define GIC_SRC_USB1_OHCI                       GIC_SRC_SPI(65-32)
#define GIC_SRC_USB2                            GIC_SRC_SPI(66-32)
#define GIC_SRC_CAN0_TOP                        GIC_SRC_SPI(67-32)
#define GIC_SRC_CAN0_0                          GIC_SRC_SPI(68-32)
#define GIC_SRC_CAN0_1                          GIC_SRC_SPI(69-32)
#define GIC_SRC_NAND                            GIC_SRC_SPI(70-32)
#define GIC_SRC_THS0                            GIC_SRC_SPI(71-32)
#define GIC_SRC_SMHC0                           GIC_SRC_SPI(72-32)
#define GIC_SRC_SMHC1                           GIC_SRC_SPI(73-32)
#define GIC_SRC_SMHC2                           GIC_SRC_SPI(74-32)
#define GIC_SRC_NSI                             GIC_SRC_SPI(75-32)
#define GIC_SRC_SMC                             GIC_SRC_SPI(76-32)
#define GIC_SRC_PWM2                            GIC_SRC_SPI(77-32)
#define GIC_SRC_GMAC0                           GIC_SRC_SPI(78-32)
#define GIC_SRC_GMAC1                           GIC_SRC_SPI(79-32)
#define GIC_SRC_LBC                             GIC_SRC_SPI(80-32)
#define GIC_SRC_HRDY_TOUT                       GIC_SRC_SPI(81-32)
#define GIC_SRC_DMA0_NS                         GIC_SRC_SPI(82-32)
#define GIC_SRC_DMA0_S                          GIC_SRC_SPI(83-32)
#define GIC_SRC_DMA                             GIC_SRC_DMA0_NS
#define GIC_SRC_CE_NS                           GIC_SRC_SPI(84-32)
#define GIC_SRC_CE_S                            GIC_SRC_SPI(85-32)
#define GIC_SRC_SPINLOCK                        GIC_SRC_SPI(86-32)
#define GIC_SRC_TIMER0                          GIC_SRC_SPI(87-32)
#define GIC_SRC_TIMER1                          GIC_SRC_SPI(88-32)
#define GIC_SRC_TIMER2                          GIC_SRC_SPI(89-32)
#define GIC_SRC_TIMER3                          GIC_SRC_SPI(90-32)
#define GIC_SRC_TIMER4                          GIC_SRC_SPI(91-32)
#define GIC_SRC_TIMER5                          GIC_SRC_SPI(92-32)
#define GIC_SRC_GPADC0                          GIC_SRC_SPI(93-32)
#define GIC_SRC_GPADC                           GIC_SRC_GPADC0
#define GIC_SRC_GPADC2                          GIC_SRC_SPI(94-32)
#define GIC_SRC_WDOG0                           GIC_SRC_SPI(95-32)
#define GIC_SRC_GPADC1                          GIC_SRC_SPI(96-32)
#define GIC_SRC_IOMMU                           GIC_SRC_SPI(97-32)
#define GIC_SRC_LRADC                           GIC_SRC_SPI(98-32)
#define GIC_SRC_GPIOA_NS                        GIC_SRC_SPI(99-32)
#define GIC_SRC_GPIOA_S                         GIC_SRC_SPI(100-32)
#define GIC_SRC_GPIOA                           GIC_SRC_GPIOA_NS
#define GIC_SRC_GPIOB_NS                        GIC_SRC_SPI(101-32)
#define GIC_SRC_GPIOB_S                         GIC_SRC_SPI(102-32)
#define GIC_SRC_GPIOB                           GIC_SRC_GPIOB_NS
#define GIC_SRC_GPIOC_NS                        GIC_SRC_SPI(103-32)
#define GIC_SRC_GPIOC_S                         GIC_SRC_SPI(104-32)
#define GIC_SRC_GPIOC                           GIC_SRC_GPIOC_NS
#define GIC_SRC_GPIOD_NS                        GIC_SRC_SPI(105-32)
#define GIC_SRC_GPIOD_S                         GIC_SRC_SPI(106-32)
#define GIC_SRC_GPIOD                           GIC_SRC_GPIOD_NS
#define GIC_SRC_GPIOE_NS                        GIC_SRC_SPI(107-32)
#define GIC_SRC_GPIOE_S                         GIC_SRC_SPI(108-32)
#define GIC_SRC_GPIOE                           GIC_SRC_GPIOE_NS
#define GIC_SRC_GPIOF_NS                        GIC_SRC_SPI(109-32)
#define GIC_SRC_GPIOF_S                         GIC_SRC_SPI(110-32)
#define GIC_SRC_GPIOF                           GIC_SRC_GPIOF_NS
#define GIC_SRC_GPIOG_NS                        GIC_SRC_SPI(111-32)
#define GIC_SRC_GPIOG_S                         GIC_SRC_SPI(112-32)
#define GIC_SRC_GPIOG                           GIC_SRC_GPIOG_NS
#define GIC_SRC_GPIOH_NS                        GIC_SRC_SPI(113-32)
#define GIC_SRC_GPIOH_S                         GIC_SRC_SPI(114-32)
#define GIC_SRC_GPIOH                           GIC_SRC_GPIOH_NS
#define GIC_SRC_GPIOI_NS                        GIC_SRC_SPI(115-32)
#define GIC_SRC_GPIOI_S                         GIC_SRC_SPI(116-32)
#define GIC_SRC_GPIOI                           GIC_SRC_GPIOI_NS
#define GIC_SRC_GPIOJ_NS                        GIC_SRC_SPI(117-32)
#define GIC_SRC_GPIOJ_S                         GIC_SRC_SPI(118-32)
#define GIC_SRC_GPIOJ                           GIC_SRC_GPIOJ_NS
#define GIC_SRC_DE0                             GIC_SRC_SPI(119-32)
#define GIC_SRC_DE                              GIC_SRC_DE0
#define GIC_SRC_DE0_CRC                         GIC_SRC_SPI(120-32)
//#define GIC_SRC_TIMER0                        GIC_SRC_SPI(121-32)
#define GIC_SRC_TCON0_LCD0                      GIC_SRC_SPI(122-32)
#define GIC_SRC_DSI0                            GIC_SRC_SPI(123-32)
#define GIC_SRC_GMAC0_PWR_CLK_CTRL              GIC_SRC_SPI(124-32)
#define GIC_SRC_GMAC0_PERCH_TX                  GIC_SRC_SPI(125-32)
#define GIC_SRC_GMAC0_PERCH_RX                  GIC_SRC_SPI(126-32)
#define GIC_SRC_GMAC1_PWR_CLK_CTRL              GIC_SRC_SPI(127-32)
#define GIC_SRC_GMAC1_PERCH_TX                  GIC_SRC_SPI(128-32)
#define GIC_SRC_GMAC1_PERCH_RX                  GIC_SRC_SPI(129-32)
#define GIC_SRC_PCIE_EDMA0                      GIC_SRC_SPI(130-32)
#define GIC_SRC_PCIE_EDMA1                      GIC_SRC_SPI(131-32)
#define GIC_SRC_PCIE_EDMA2                      GIC_SRC_SPI(132-32)
#define GIC_SRC_PCIE_EDMA3                      GIC_SRC_SPI(133-32)
#define GIC_SRC_PCIE_EDMA4                      GIC_SRC_SPI(134-32)
#define GIC_SRC_PCIE_EDMA5                      GIC_SRC_SPI(135-32)
#define GIC_SRC_PCIE_EDMA6                      GIC_SRC_SPI(136-32)
#define GIC_SRC_PCIE_EDMA7                      GIC_SRC_SPI(137-32)
#define GIC_SRC_PCIE_SII                        GIC_SRC_SPI(138-32)
#define GIC_SRC_PCIE_MSI                        GIC_SRC_SPI(139-32)
#define GIC_SRC_CAN2_TOP                        GIC_SRC_SPI(140-32)
#define GIC_SRC_CAN2_0                          GIC_SRC_SPI(141-32)
#define GIC_SRC_CAN2_1                          GIC_SRC_SPI(142-32)
#define GIC_SRC_CAN3_TOP                        GIC_SRC_SPI(143-32)
#define GIC_SRC_CAN3_0                          GIC_SRC_SPI(144-32)
#define GIC_SRC_CAN3_1                          GIC_SRC_SPI(145-32)
#define GIC_SRC_NPU_SRAM_TZMA                   GIC_SRC_SPI(146-32)
#define GIC_SRC_NPU                             GIC_SRC_SPI(148-32)
#define GIC_SRC_IRRX1                           GIC_SRC_SPI(149-32)
#define GIC_SRC_IRRX2                           GIC_SRC_SPI(150-32)
#define GIC_SRC_IRRX3                           GIC_SRC_SPI(151-32)
#define GIC_SRC_VE                              GIC_SRC_SPI(152-32)
#define GIC_SRC_MEMC_DFS                        GIC_SRC_SPI(153-32)
#define GIC_SRC_CSI_DMA0                        GIC_SRC_SPI(154-32)
#define GIC_SRC_CSI_DMA1                        GIC_SRC_SPI(155-32)
#define GIC_SRC_CSI_DMA2                        GIC_SRC_SPI(156-32)
#define GIC_SRC_CSI_DMA3                        GIC_SRC_SPI(157-32)
#define GIC_SRC_CSI_VIPP0                       GIC_SRC_SPI(158-32)
#define GIC_SRC_CSI_VIPP1                       GIC_SRC_SPI(159-32)
#define GIC_SRC_CSI_VIPP2                       GIC_SRC_SPI(160-32)
#define GIC_SRC_CSI_VIPP3                       GIC_SRC_SPI(161-32)
#define GIC_SRC_CSI_PARSER0                     GIC_SRC_SPI(162-32)
#define GIC_SRC_CSI_PARSER1                     GIC_SRC_SPI(163-32)
#define GIC_SRC_CSI_PARSER2                     GIC_SRC_SPI(164-32)
#define GIC_SRC_CSI_ISP0                        GIC_SRC_SPI(165-32)
#define GIC_SRC_CSI_ISP1                        GIC_SRC_SPI(166-32)
#define GIC_SRC_CSI_ISP2                        GIC_SRC_SPI(167-32)
#define GIC_SRC_CSI_ISP3                        GIC_SRC_SPI(168-32)
#define GIC_SRC_CSI_CMB                         GIC_SRC_SPI(169-32)
#define GIC_SRC_CSI_TDM                         GIC_SRC_SPI(170-32)
#define GIC_SRC_CSI_TOP_PKT                     GIC_SRC_SPI(171-32)
#define GIC_SRC_GPIOK_NS                        GIC_SRC_SPI(172-32)
#define GIC_SRC_GPIOK_S                         GIC_SRC_SPI(173-32)
#define GIC_SRC_GPIOK                           GIC_SRC_GPIOK_NS
#define GIC_SRC_PWM1                            GIC_SRC_SPI(174-32)
#define GIC_SRC_G2D                             GIC_SRC_SPI(175-32)
#define GIC_SRC_CAN1_0                          GIC_SRC_SPI(176-32)
#define GIC_SRC_CAN1_1                          GIC_SRC_SPI(177-32)
#define GIC_SRC_CAN1_TOP                        GIC_SRC_SPI(178-32)
#define GIC_SRC_CSI_PARSER3                     GIC_SRC_SPI(179-32)
#define GIC_SRC_UART8                           GIC_SRC_SPI(180-32)
#define GIC_SRC_UART9                           GIC_SRC_SPI(181-32)
#define GIC_SRC_UART10                          GIC_SRC_SPI(182-32)
#define GIC_SRC_UART11                          GIC_SRC_SPI(183-32)
#define GIC_SRC_UART12                          GIC_SRC_SPI(184-32)
#define GIC_SRC_UART13                          GIC_SRC_SPI(185-32)
#define GIC_SRC_UART14                          GIC_SRC_SPI(186-32)
#define GIC_SRC_TWI6                            GIC_SRC_SPI(187-32)
#define GIC_SRC_TIMER6                          GIC_SRC_SPI(188-32)
#define GIC_SRC_TIMER7                          GIC_SRC_SPI(189-32)
#define GIC_SRC_GPADC3                          GIC_SRC_SPI(190-32)
#define GIC_SRC_TPADC                           GIC_SRC_SPI(191-32)
#define GIC_SRC_I2S0                            GIC_SRC_SPI(192-32)
#define GIC_SRC_I2S1                            GIC_SRC_SPI(193-32)
#define GIC_SRC_I2S2                            GIC_SRC_SPI(194-32)
#define GIC_SRC_I2S3                            GIC_SRC_SPI(195-32)
#define GIC_SRC_DMA1_NS                         GIC_SRC_SPI(196-32)
#define GIC_SRC_DMA1_S                          GIC_SRC_SPI(197-32)
#define GIC_SRC_DMA1                            GIC_SRC_DMA1_NS
#define GIC_SRC_CORE0_MBOX_R                    GIC_SRC_SPI(198-32)
#define GIC_SRC_CORE1_MBOX_R                    GIC_SRC_SPI(199-32)
#define GIC_SRC_CORE2_MBOX_R                    GIC_SRC_SPI(200-32)
#define GIC_SRC_CORE3_MBOX_R                    GIC_SRC_SPI(201-32)
#define GIC_SRC_CORE0_MBOX_CORE1_W              GIC_SRC_SPI(202-32)
#define GIC_SRC_CORE0_MBOX_CORE2_W              GIC_SRC_SPI(203-32)
#define GIC_SRC_CORE0_MBOX_CORE3_W              GIC_SRC_SPI(204-32)
#define GIC_SRC_CORE1_MBOX_CORE0_W              GIC_SRC_SPI(205-32)
#define GIC_SRC_CORE1_MBOX_CORE2_W              GIC_SRC_SPI(206-32)
#define GIC_SRC_CORE1_MBOX_CORE3_W              GIC_SRC_SPI(207-32)
#define GIC_SRC_CORE2_MBOX_CORE0_W              GIC_SRC_SPI(208-32)
#define GIC_SRC_CORE2_MBOX_CORE1_W              GIC_SRC_SPI(209-32)
#define GIC_SRC_CORE2_MBOX_CORE3_W              GIC_SRC_SPI(210-32)
#define GIC_SRC_CORE3_MBOX_CORE0_W              GIC_SRC_SPI(211-32)
#define GIC_SRC_CORE3_MBOX_CORE1_W              GIC_SRC_SPI(212-32)
#define GIC_SRC_CORE3_MBOX_CORE2_W              GIC_SRC_SPI(213-32)
//#define GIC_SRC_MEMC0_SMC1                    GIC_SRC_SPI(214-32)
#define GIC_SRC_RV_MBOX_CPUX_W                  GIC_SRC_SPI(215-32)
//#define GIC_SRC_MEMC_DDRPHY                   GIC_SRC_SPI(216-32)
#define GIC_SRC_RV_WDG                          GIC_SRC_SPI(217-32)
#define GIC_SRC_RV_TIMER0                       GIC_SRC_SPI(218-32)
#define GIC_SRC_RV_TIMER1                       GIC_SRC_SPI(219-32)
#define GIC_SRC_RV_TIMER2                       GIC_SRC_SPI(220-32)
#define GIC_SRC_RV_TIMER3                       GIC_SRC_SPI(221-32)
#define GIC_SRC_RV_ECC                          GIC_SRC_SPI(222-32)
//#define GIC_SRC_R_WDT                         GIC_SRC_SPI(223-32)
#define GIC_SRC_NMI                             GIC_SRC_SPI(224-32)
#define GIC_SRC_PCK600_QCHANNEL                 GIC_SRC_SPI(225-32)
#define GIC_SRC_R_TWD                           GIC_SRC_SPI(226-32)
#define GIC_SRC_R_WDT                           GIC_SRC_SPI(227-32)
#define GIC_SRC_R_TIMER0                        GIC_SRC_SPI(228-32)
#define GIC_SRC_R_TIMER1                        GIC_SRC_SPI(229-32)
#define GIC_SRC_R_TIMER2                        GIC_SRC_SPI(230-32)
#define GIC_SRC_R_TIMER3                        GIC_SRC_SPI(231-32)
#define GIC_SRC_R_ALARM0                        GIC_SRC_SPI(232-32)
#define GIC_SRC_ALARM                           GIC_SRC_R_ALARM0
#define GIC_SRC_GPIOL_S                         GIC_SRC_SPI(233-32)
#define GIC_SRC_GPIOL_NS                        GIC_SRC_SPI(234-32)
#define GIC_SRC_GPIOL                           GIC_SRC_GPIOL_S
#define GIC_SRC_GPIOM_S                         GIC_SRC_SPI(235-32)
#define GIC_SRC_GPIOM_NS                        GIC_SRC_SPI(236-32)
#define GIC_SRC_GPIOM                           GIC_SRC_GPIOM_S
#define GIC_SRC_R_UART0                         GIC_SRC_SPI(237-32)
#define GIC_SRC_R_UART1                         GIC_SRC_SPI(238-32)
#define GIC_SRC_R_TWI0                          GIC_SRC_SPI(239-32)
#define GIC_SRC_R_TWI1                          GIC_SRC_SPI(240-32)
#define GIC_SRC_R_TWI2                          GIC_SRC_SPI(241-32)
#define GIC_SRC_R_IRRX                          GIC_SRC_SPI(242-32)
#define GIC_SRC_R_PWM                           GIC_SRC_SPI(243-32)
#define GIC_SRC_SRAM_A2_TZMA                    GIC_SRC_SPI(244-32)
#define GIC_SRC_R_HRDY_TOUT                     GIC_SRC_SPI(245-32)
#define GIC_SRC_R_SPI                           GIC_SRC_SPI(246-32)
#define GIC_SRC_CPUS_MBOX_CPUX_W                GIC_SRC_SPI(247-32)
#define GIC_SRC_PCK600_CPU                      GIC_SRC_SPI(248-32)

#define GIC_SRC_SRAM_A2_ECC                     GIC_SRC_SPI(250-32)

#define GIC_SRC_GMAC0_ECC                       GIC_SRC_SPI(256-32)
#define GIC_SRC_GMAC1_ECC                       GIC_SRC_SPI(257-32)
#define GIC_SRC_SID_CRC                         GIC_SRC_SPI(258-32)
#define GIC_SRC_SD0_ECC                         GIC_SRC_SPI(259-32)
#define GIC_SRC_SD2_ECC                         GIC_SRC_SPI(260-32)
#define GIC_SRC_MEMC_ECC_ERR                    GIC_SRC_SPI(261-32)
#define GIC_SRC_MEMC_ECC_ERR_FAULT              GIC_SRC_SPI(262-32)
#define GIC_SRC_MEMC_ECC_AP_ERR                 GIC_SRC_SPI(263-32)
#define GIC_SRC_MEMC_ECC_AP_ERR_FAULT           GIC_SRC_SPI(264-32)
#define GIC_SRC_MEMC_SRAM_ECC                   GIC_SRC_SPI(265-32)
#define GIC_SRC_NSI_SBR_DONE                    GIC_SRC_SPI(266-32)
#define GIC_SRC_DMA0_ECC                        GIC_SRC_SPI(267-32)
#define GIC_SRC_DMA1_ECC                        GIC_SRC_SPI(268-32)

//#define GIC_SRC_SPI3                            GIC_SRC_SPI(58-32)        // (26 )
//#define GIC_SRC_SPI4                            GIC_SRC_SPI(59-32)        // (27 )
//#define GIC_SRC_SPI5                            GIC_SRC_SPI(60-32)        // (28 )
//#define GIC_SRC_SPI6                            GIC_SRC_SPI(65-32)        // (33 )
//#define GIC_SRC_VE_ENC0                         GIC_SRC_SPI(67-32)        // (35 )
//#define GIC_SRC_VE_ENC1                         GIC_SRC_SPI(68-32)        // (36 )
//#define GIC_SRC_VE_DEC                          GIC_SRC_SPI(69-32)        // (37 )
//#define GIC_SRC_LOCALBUS                        GIC_SRC_SPI(71-32)        // (39 )
//#define GIC_SRC_WDG0                            GIC_SRC_SPI(96-32)        // (64 )
//#define GIC_SRC_PWM1                            GIC_SRC_SPI(97-32)        // (65 )
//#define GIC_SRC_GPIOA_NS                        GIC_SRC_SPI(99-32)        // (67 )
//#define GIC_SRC_GPIOA_S                         GIC_SRC_SPI(100-32)       // (68 )
//#define GIC_SRC_DE0                             GIC_SRC_SPI(121-32)       // (89 )
//#define GIC_SRC_DE1                             GIC_SRC_SPI(122-32)       // (90 )
//#define GIC_SRC_G2D                             GIC_SRC_SPI(123-32)       // (91 )
//#define GIC_SRC_DI                              GIC_SRC_SPI(124-32)       // (92 )
//#define GIC_SRC_DE_FREEZE                       GIC_SRC_SPI(125-32)       // (93 )
//#define GIC_SRC_EDP                             GIC_SRC_SPI(126-32)       // (94 )
//#define GIC_SRC_TCON0_LCD3                      GIC_SRC_SPI(130-32)       // (98 )
//#define GIC_SRC_TV2                             GIC_SRC_SPI(133-32)       // (101)
//#define GIC_SRC_HDMI                            GIC_SRC_SPI(136-32)       // (104)
//#define GIC_SRC_GPU                             GIC_SRC_SPI(137-32)       // (105)
//#define GIC_SRC_GPU_DVFS                        GIC_SRC_SPI(138-32)       // (106)
//#define GIC_SRC_GPU_OS0                         GIC_SRC_SPI(139-32)       // (107)
//#define GIC_SRC_GPU_OS1                         GIC_SRC_SPI(140-32)       // (108)
//#define GIC_SRC_GPU_OS2                         GIC_SRC_SPI(141-32)       // (109)
//#define GIC_SRC_GPU_OS3                         GIC_SRC_SPI(142-32)       // (110)
//#define GIC_SRC_CSI_DMA4                        GIC_SRC_SPI(151-32)       // (119)
//#define GIC_SRC_CSI_DMA5                        GIC_SRC_SPI(152-32)       // (120)
//#define GIC_SRC_CSI_DMA6                        GIC_SRC_SPI(153-32)       // (121)
//#define GIC_SRC_CSI_DMA7                        GIC_SRC_SPI(154-32)       // (122)
//#define GIC_SRC_CSI_DMA8                        GIC_SRC_SPI(155-32)       // (123)
//#define GIC_SRC_CSI_DMA9                        GIC_SRC_SPI(156-32)       // (124)
//#define GIC_SRC_CSI_DMA10                       GIC_SRC_SPI(157-32)       // (125)
//#define GIC_SRC_CSI_DMA11                       GIC_SRC_SPI(158-32)       // (126)
//#define GIC_SRC_CSI_VIPP4                       GIC_SRC_SPI(163-32)       // (131)
//#define GIC_SRC_CSI_VIPP5                       GIC_SRC_SPI(164-32)       // (132)
//#define GIC_SRC_CSI_VIPP6                       GIC_SRC_SPI(165-32)       // (133)
//#define GIC_SRC_CSI_VIPP7                       GIC_SRC_SPI(166-32)       // (134)
//#define GIC_SRC_CSI_VIPP8                       GIC_SRC_SPI(167-32)       // (135)
//#define GIC_SRC_CSI_VIPP9                       GIC_SRC_SPI(168-32)       // (136)
//#define GIC_SRC_CSI_VIPP10                      GIC_SRC_SPI(169-32)       // (137)
//#define GIC_SRC_CSI_VIPP11                      GIC_SRC_SPI(170-32)       // (138)
//#define GIC_SRC_PCIE1_EDMA_0                    GIC_SRC_SPI(200-32)       // (168)
//#define GIC_SRC_PCIE1_EDMA_1                    GIC_SRC_SPI(201-32)       // (169)
//#define GIC_SRC_PCIE1_EDMA_2                    GIC_SRC_SPI(202-32)       // (170)
//#define GIC_SRC_PCIE1_EDMA_3                    GIC_SRC_SPI(203-32)       // (171)
//#define GIC_SRC_PCIE1_EDMA_4                    GIC_SRC_SPI(204-32)       // (172)
//#define GIC_SRC_PCIE1_EDMA_5                    GIC_SRC_SPI(205-32)       // (173)
//#define GIC_SRC_PCIE1_EDMA_6                    GIC_SRC_SPI(206-32)       // (174)
//#define GIC_SRC_PCIE1_EDMA_7                    GIC_SRC_SPI(207-32)       // (175)
//#define GIC_SRC_PCIE1_SII                       GIC_SRC_SPI(208-32)       // (176)
//#define GIC_SRC_PCIE1_MSI                       GIC_SRC_SPI(209-32)       // (177)
//#define GIC_SRC_PCIE1_EDMA_8                    GIC_SRC_SPI(210-32)       // (178)
//#define GIC_SRC_PCIE1_EDMA_9                    GIC_SRC_SPI(211-32)       // (179)
//#define GIC_SRC_PCIE1_EDMA_10                   GIC_SRC_SPI(212-32)       // (180)
//#define GIC_SRC_PCIE1_EDMA_11                   GIC_SRC_SPI(213-32)       // (181)
//#define GIC_SRC_PCIE1_EDMA_12                   GIC_SRC_SPI(214-32)       // (182)
//#define GIC_SRC_PCIE1_EDMA_13                   GIC_SRC_SPI(215-32)       // (183)
//#define GIC_SRC_PCIE1_EDMA_14                   GIC_SRC_SPI(216-32)       // (184)
//#define GIC_SRC_PCIE1_EDMA_15                   GIC_SRC_SPI(217-32)       // (185)
//#define GIC_SRC_COMBO0_PHY1                     GIC_SRC_SPI(219-32)       // (187)
//#define GIC_SRC_COMBO0_PHY2                     GIC_SRC_SPI(220-32)       // (188)
//#define GIC_SRC_COMBO0_PHY3                     GIC_SRC_SPI(221-32)       // (189)
//#define GIC_SRC_COMBO1_PHY0                     GIC_SRC_SPI(222-32)       // (190)
//#define GIC_SRC_COMBO1_PHY1                     GIC_SRC_SPI(223-32)       // (191)
//#define GIC_SRC_USB2_EHCI                       GIC_SRC_SPI(229-32)       // (197)
//#define GIC_SRC_USB2_OHCI                       GIC_SRC_SPI(230-32)       // (198)
//#define GIC_SRC_A76_MBOX_R                      GIC_SRC_SPI(232-32)       // (200)
//#define GIC_SRC_A76_MBOX_R                      GIC_SRC_SPI(233-32)       // (201)
//#define GIC_SRC_A55_MBOX_W                      GIC_SRC_SPI(234-32)       // (202)
//#define GIC_SRC_A55_MBOX_W                      GIC_SRC_SPI(235-32)       // (203)
//#define GIC_SRC_Memc0_smc0                      GIC_SRC_SPI(236-32)       // (204)
//#define GIC_SRC_Memc0_smc1                      GIC_SRC_SPI(237-32)       // (205)
//#define GIC_SRC_Memc0_dfs                       GIC_SRC_SPI(238-32)       // (206)
//#define GIC_SRC_Memc0_ecc0                      GIC_SRC_SPI(239-32)       // (207)
//#define GIC_SRC_Memc0_ecc1                      GIC_SRC_SPI(240-32)       // (208)
//#define GIC_SRC_Memc0_ecc_fault0                GIC_SRC_SPI(241-32)       // (209)
//#define GIC_SRC_Memc0_ecc_fault1                GIC_SRC_SPI(242-32)       // (210)
//#define GIC_SRC_Memc0_ap_ecc0                   GIC_SRC_SPI(243-32)       // (211)
//#define GIC_SRC_Memc0_ap_ecc1                   GIC_SRC_SPI(244-32)       // (212)
//#define GIC_SRC_Memc0_ap_ecc_fault0             GIC_SRC_SPI(245-32)       // (213)
//#define GIC_SRC_Memc0_ap_ecc_fault1             GIC_SRC_SPI(246-32)       // (214)
//#define GIC_SRC_DDRPHY                          GIC_SRC_SPI(247-32)       // (215)
//#define GIC_SRC_TIMER6                          GIC_SRC_SPI(248-32)       // (216)
//#define GIC_SRC_TIMER7                          GIC_SRC_SPI(249-32)       // (217)
//#define GIC_SRC_TIMER8                          GIC_SRC_SPI(250-32)       // (218)
//#define GIC_SRC_TIMER9                          GIC_SRC_SPI(251-32)       // (219)
//#define GIC_SRC_TIMER10                         GIC_SRC_SPI(252-32)       // (220)
//#define GIC_SRC_TIMER11                         GIC_SRC_SPI(253-32)       // (221)
//#define GIC_SRC_TIMER12                         GIC_SRC_SPI(254-32)       // (222)
//#define GIC_SRC_TIMER13                         GIC_SRC_SPI(255-32)       // (223)
//#define GIC_SRC_TIMER14                         GIC_SRC_SPI(256-32)       // (224)
//#define GIC_SRC_TIMER15                         GIC_SRC_SPI(257-32)       // (225)
//#define GIC_SRC_Memc0_sram_ecc0                 GIC_SRC_SPI(258-32)       // (226)
//#define GIC_SRC_Memc0_sram_ecc1                 GIC_SRC_SPI(259-32)       // (227)
//#define GIC_SRC_SMMU_TBU22                      GIC_SRC_SPI(260-32)       // (228)
//#define GIC_SRC_WDG1                            GIC_SRC_SPI(261-32)       // (229)
//#define GIC_SRC_I2S5                            GIC_SRC_SPI(266-32)       // (234)
//#define GIC_SRC_SPDIF                           GIC_SRC_SPI(267-32)       // (235)
//#define GIC_SRC_SMMU_TCU                        GIC_SRC_SPI(268-32)       // (236)
//#define GIC_SRC_SMMU_TBU_ISP                    GIC_SRC_SPI(269-32)       // (237)
//#define GIC_SRC_SMMU_TBU_CSI0                   GIC_SRC_SPI(270-32)       // (238)
//#define GIC_SRC_SMMU_TBU_CSI1                   GIC_SRC_SPI(271-32)       // (239)
//#define GIC_SRC_SMMU_TBU_DI                     GIC_SRC_SPI(272-32)       // (240)
//#define GIC_SRC_SMMU_TBU_G2D                    GIC_SRC_SPI(273-32)       // (241)
//#define GIC_SRC_SMMU_TBU_VE_ENC0                GIC_SRC_SPI(274-32)       // (242)
//#define GIC_SRC_SMMU_TBU_VE_ENC1                GIC_SRC_SPI(275-32)       // (243)
//#define GIC_SRC_SMMU_TBU_VE_DEC0                GIC_SRC_SPI(276-32)       // (244)
//#define GIC_SRC_SMMU_TBU_VE_DEC1                GIC_SRC_SPI(277-32)       // (245)
//#define GIC_SRC_SMMU_TBU_DE0                    GIC_SRC_SPI(278-32)       // (246)
//#define GIC_SRC_SMMU_TBU_DE1                    GIC_SRC_SPI(279-32)       // (247)
//#define GIC_SRC_SMMU_TBU_GMAC0                  GIC_SRC_SPI(280-32)       // (248)
//#define GIC_SRC_SMMU_TBU_GMAC1                  GIC_SRC_SPI(281-32)       // (249)
//#define GIC_SRC_SMMU_TBU_USB3                   GIC_SRC_SPI(282-32)       // (250)
//#define GIC_SRC_SMMU_TBU_MSI0                   GIC_SRC_SPI(283-32)       // (251)
//#define GIC_SRC_SMMU_TBU_MSI1                   GIC_SRC_SPI(284-32)       // (252)
//#define GIC_SRC_SMMU_TBU_PCIE0                  GIC_SRC_SPI(285-32)       // (253)
//#define GIC_SRC_SMMU_TBU_PCIE1                  GIC_SRC_SPI(286-32)       // (254)
//#define GIC_SRC_SMMU_TBU_GPU0                   GIC_SRC_SPI(287-32)       // (255)
//#define GIC_SRC_SMMU_TBU_GPU1                   GIC_SRC_SPI(288-32)       // (256)
//#define GIC_SRC_SMMU_TBU_AIPU                   GIC_SRC_SPI(289-32)       // (257)
//#define GIC_SRC_SMMU_TBU_NPU                    GIC_SRC_SPI(290-32)       // (258)
//#define GIC_SRC_AIPU                            GIC_SRC_SPI(291-32)       // (259)
//#define GIC_SRC_NPU                             GIC_SRC_SPI(292-32)       // (260)
//#define GIC_SRC_NPU_TZMA_ERR                    GIC_SRC_SPI(293-32)       // (261)
//#define GIC_SRC_CAN0_0                          GIC_SRC_SPI(294-32)       // (262)
//#define GIC_SRC_CAN0_1                          GIC_SRC_SPI(295-32)       // (263)
//#define GIC_SRC_CAN0_TOP                        GIC_SRC_SPI(296-32)       // (264)
//#define GIC_SRC_CAN1_0                          GIC_SRC_SPI(297-32)       // (265)
//#define GIC_SRC_CAN1_1                          GIC_SRC_SPI(298-32)       // (266)
//#define GIC_SRC_CAN1_TOP                        GIC_SRC_SPI(299-32)       // (267)
//#define GIC_SRC_CAN2_0                          GIC_SRC_SPI(300-32)       // (268)
//#define GIC_SRC_CAN2_1                          GIC_SRC_SPI(301-32)       // (269)
//#define GIC_SRC_CAN2_TOP                        GIC_SRC_SPI(302-32)       // (270)
//#define GIC_SRC_GMAC0_SFTY                      GIC_SRC_SPI(304-32)       // (272)
//#define GIC_SRC_GMAC0_PWR_CLK_CTRL              GIC_SRC_SPI(305-32)       // (273)
//#define GIC_SRC_GMAC0_PERCH_TX_0                GIC_SRC_SPI(306-32)       // (274)
//#define GIC_SRC_GMAC0_PERCH_TX_1                GIC_SRC_SPI(307-32)       // (275)
//#define GIC_SRC_GMAC0_PERCH_TX_2                GIC_SRC_SPI(308-32)       // (276)
//#define GIC_SRC_GMAC0_PERCH_TX_3                GIC_SRC_SPI(309-32)       // (277)
//#define GIC_SRC_GMAC0_PERCH_TX_4                GIC_SRC_SPI(310-32)       // (278)
//#define GIC_SRC_GMAC0_PERCH_TX_5                GIC_SRC_SPI(311-32)       // (279)
//#define GIC_SRC_GMAC0_PERCH_TX_6                GIC_SRC_SPI(312-32)       // (280)
//#define GIC_SRC_GMAC0_PERCH_TX_7                GIC_SRC_SPI(313-32)       // (281)
//#define GIC_SRC_GMAC0_PERCH_RX_0                GIC_SRC_SPI(314-32)       // (282)
//#define GIC_SRC_GMAC0_PERCH_RX_1                GIC_SRC_SPI(315-32)       // (283)
//#define GIC_SRC_GMAC0_PERCH_RX_2                GIC_SRC_SPI(316-32)       // (284)
//#define GIC_SRC_GMAC0_PERCH_RX_3                GIC_SRC_SPI(317-32)       // (285)
//#define GIC_SRC_GMAC0_PERCH_RX_4                GIC_SRC_SPI(318-32)       // (286)
//#define GIC_SRC_GMAC0_PERCH_RX_5                GIC_SRC_SPI(319-32)       // (287)
//#define GIC_SRC_GMAC0_PERCH_RX_6                GIC_SRC_SPI(320-32)       // (288)
//#define GIC_SRC_GMAC0_PERCH_RX_7                GIC_SRC_SPI(321-32)       // (289)
//#define GIC_SRC_GMAC1_TOP                       GIC_SRC_SPI(322-32)       // (290)
//#define GIC_SRC_GMAC1_SFTY                      GIC_SRC_SPI(323-32)       // (291)
//#define GIC_SRC_GMAC1_PWR_CLK_CTRL              GIC_SRC_SPI(324-32)       // (292)
//#define GIC_SRC_GMAC1_PERCH_TX_0                GIC_SRC_SPI(325-32)       // (293)
//#define GIC_SRC_GMAC1_PERCH_TX_1                GIC_SRC_SPI(326-32)       // (294)
//#define GIC_SRC_GMAC1_PERCH_TX_2                GIC_SRC_SPI(327-32)       // (295)
//#define GIC_SRC_GMAC1_PERCH_TX_3                GIC_SRC_SPI(328-32)       // (296)
//#define GIC_SRC_GMAC1_PERCH_TX_4                GIC_SRC_SPI(329-32)       // (297)
//#define GIC_SRC_GMAC1_PERCH_TX_5                GIC_SRC_SPI(330-32)       // (298)
//#define GIC_SRC_GMAC1_PERCH_TX_6                GIC_SRC_SPI(331-32)       // (299)
//#define GIC_SRC_GMAC1_PERCH_TX_7                GIC_SRC_SPI(332-32)       // (300)
//#define GIC_SRC_GMAC1_PERCH_RX_0                GIC_SRC_SPI(333-32)       // (301)
//#define GIC_SRC_GMAC1_PERCH_RX_1                GIC_SRC_SPI(334-32)       // (302)
//#define GIC_SRC_GMAC1_PERCH_RX_2                GIC_SRC_SPI(335-32)       // (303)
//#define GIC_SRC_GMAC1_PERCH_RX_3                GIC_SRC_SPI(336-32)       // (304)
//#define GIC_SRC_GMAC1_PERCH_RX_4                GIC_SRC_SPI(337-32)       // (305)
//#define GIC_SRC_GMAC1_PERCH_RX_5                GIC_SRC_SPI(338-32)       // (306)
//#define GIC_SRC_GMAC1_PERCH_RX_6                GIC_SRC_SPI(339-32)       // (307)
//#define GIC_SRC_GMAC1_PERCH_RX_7                GIC_SRC_SPI(340-32)       // (308)
//#define GIC_SRC_GPADC1                          GIC_SRC_SPI(341-32)       // (309)
//#define GIC_SRC_MEMC_SBR                        GIC_SRC_SPI(342-32)       // (310)
//#define GIC_SRC_R_TIMER3                        GIC_SRC_SPI(372-32)       // (340)
//#define GIC_SRC_R_CAN0                          GIC_SRC_SPI(373-32)       // (341)
//#define GIC_SRC_R_CAN1                          GIC_SRC_SPI(374-32)       // (342)
//#define GIC_SRC_RV_CPUS_MBOX_W                  GIC_SRC_SPI(375-32)       // (343)
//#define GIC_SRC_RV_DSP_MBOX_W                   GIC_SRC_SPI(376-32)       // (344)
//#define GIC_SRC_RV_CPUX_MBOX_W                  GIC_SRC_SPI(377-32)       // (345)
//#define GIC_SRC_RV_RV_MBOX_R                    GIC_SRC_SPI(378-32)       // (346)
//#define GIC_SRC_RV_WDG                          GIC_SRC_SPI(379-32)       // (347)
//#define GIC_SRC_RV_TIMER0                       GIC_SRC_SPI(380-32)       // (348)
//#define GIC_SRC_RV_TIMER1                       GIC_SRC_SPI(381-32)       // (349)
//#define GIC_SRC_RV_TIMER2                       GIC_SRC_SPI(382-32)       // (350)
//#define GIC_SRC_RV_TIMER3                       GIC_SRC_SPI(383-32)       // (351)
//#define GIC_SRC_DSP_EXCEPTIONERR                GIC_SRC_SPI(384-32)       // (352)
//#define GIC_SRC_DSP_PFATALERR                   GIC_SRC_SPI(385-32)       // (353)
//#define GIC_SRC_DSP_WDG                         GIC_SRC_SPI(386-32)       // (354)
//#define GIC_SRC_DSP_CPUX_MBOX_W                 GIC_SRC_SPI(387-32)       // (355)
//#define GIC_SRC_DSP_CPUS_MBOX_W                 GIC_SRC_SPI(388-32)       // (356)
//#define GIC_SRC_DSP_RV_MBOX_W                   GIC_SRC_SPI(389-32)       // (357)
//#define GIC_SRC_DSP_TZMA                        GIC_SRC_SPI(390-32)       // (358)
//#define GIC_SRC_DSP_TIMER0                      GIC_SRC_SPI(391-32)       // (359)
//#define GIC_SRC_DSP_TIMER1                      GIC_SRC_SPI(392-32)       // (360)
//#define GIC_SRC_DSP_TIMER2                      GIC_SRC_SPI(393-32)       // (361)
//#define GIC_SRC_DSP_TIMER3                      GIC_SRC_SPI(394-32)       // (362)
//#define GIC_SRC_DMA_MCU_NS                      GIC_SRC_SPI(395-32)       // (363)
//#define GIC_SRC_DMA_MCU_S                       GIC_SRC_SPI(396-32)       // (364)
//#define GIC_SRC_DMA_CPUX_NS                     GIC_SRC_SPI(397-32)       // (365)
//#define GIC_SRC_DMA_CPUX_S                      GIC_SRC_SPI(398-32)       // (366)
//#define GIC_SRC_MCU_TZMA0                       GIC_SRC_SPI(399-32)       // (367)
//#define GIC_SRC_MCU_TZMA1                       GIC_SRC_SPI(400-32)       // (368)
//#define GIC_SRC_MCU_I2S                         GIC_SRC_SPI(401-32)       // (369)
//#define GIC_SRC_MCU_DMIC                        GIC_SRC_SPI(402-32)       // (370)
//#define GIC_SRC_MCU_SPLOCK                      GIC_SRC_SPI(403-32)       // (371)
//#define GIC_SRC_MCU_AHBS_HREADY_TOUT            GIC_SRC_SPI(404-32)       // (372)
//#define GIC_SRC_MCU_PLL_UNLOCK                  GIC_SRC_SPI(405-32)       // (373)
//#define GIC_SRC_PUBSRAM0_ERR_IRQ                GIC_SRC_SPI(407-32)       // (375)
//#define GIC_SRC_PUBSRAM1_ERR_IRQ                GIC_SRC_SPI(408-32)       // (376)
//#define GIC_SRC_DSP_AHBS_HREADY_TOUT            GIC_SRC_SPI(409-32)       // (377)
//#define GIC_SRC_SMMU_TBU_MSI2                   GIC_SRC_SPI(410-32)       // (378)


//Interrupt Source End


/* CPUX */
#define GIC_SRC_C0_nERRIRQ0         GIC_SRC_SPI(274-32)
#define GIC_SRC_C0_nERRIRQ1         GIC_SRC_SPI(275-32)
#define GIC_SRC_C0_nERRIRQ2         GIC_SRC_SPI(276-32)
#define GIC_SRC_C0_nERRIRQ3         GIC_SRC_SPI(277-32)
#define GIC_SRC_C0_nERRIRQ4         GIC_SRC_SPI(278-32)
//#define GIC_SRC_C0_nERRIRQ5       GIC_SRC_SPI(271-32)
//#define GIC_SRC_C0_nERRIRQ6       GIC_SRC_SPI(272-32)
//#define GIC_SRC_C0_nERRIRQ7       GIC_SRC_SPI(273-32)
//#define GIC_SRC_C0_nERRIRQ8       GIC_SRC_SPI(274-32)
#define GIC_SRC_C0_nFAULTIRQ0       GIC_SRC_SPI(279-32)
#define GIC_SRC_C0_nFAULTIRQ1       GIC_SRC_SPI(280-32)
#define GIC_SRC_C0_nFAULTIRQ2       GIC_SRC_SPI(281-32)
#define GIC_SRC_C0_nFAULTIRQ3       GIC_SRC_SPI(282-32)
#define GIC_SRC_C0_nFAULTIRQ4       GIC_SRC_SPI(283-32)
//#define GIC_SRC_C0_nFAULTIRQ5     GIC_SRC_SPI(280-32)
//#define GIC_SRC_C0_nFAULTIRQ6     GIC_SRC_SPI(281-32)
//#define GIC_SRC_C0_nFAULTIRQ7     GIC_SRC_SPI(282-32)
//#define GIC_SRC_C0_nFAULTIRQ8     GIC_SRC_SPI(283-32)
#define GIC_SRC_C0_nCLUSTERPMUIRQ   GIC_SRC_SPI(284-32)
#define GIC_SRC_C0_nGIC_FAULT_INT   GIC_SRC_SPI(285-32)
#define GIC_SRC_C0_nGIC_ERR_INT     GIC_SRC_SPI(286-32)
#define GIC_SRC_C0_nGIC_PMU_INT     GIC_SRC_SPI(287-32)

//#define GIC_SRC_C0_CTI1         GIC_SRC_SPI(161-32) // (161)
//#define GIC_SRC_C0_CTI2         GIC_SRC_SPI(162-32) // (160)
//#define GIC_SRC_C0_CTI3         GIC_SRC_SPI(163-32) // (161)
//#define GIC_SRC_C0_COMMTX0      GIC_SRC_SPI(164-32) // (162)
//#define GIC_SRC_C0_COMMTX1      GIC_SRC_SPI(165-32) // (163)
//#define GIC_SRC_C0_COMMTX2      GIC_SRC_SPI(166-32) // (162)
//#define GIC_SRC_C0_COMMTX3      GIC_SRC_SPI(167-32) // (163)
//#define GIC_SRC_C0_COMMRX0      GIC_SRC_SPI(168-32) // (164)
//#define GIC_SRC_C0_COMMRX1      GIC_SRC_SPI(169-32) // (165)
//#define GIC_SRC_C0_COMMRX2      GIC_SRC_SPI(170-32) // (164)
//#define GIC_SRC_C0_COMMRX3      GIC_SRC_SPI(171-32) // (165)
//
//#define GIC_SRC_C0_PMU0         GIC_SRC_SPI(172-32) // (166)
//#define GIC_SRC_C0_PMU1         GIC_SRC_SPI(173-32) // (167)
//#define GIC_SRC_C0_PMU2         GIC_SRC_SPI(174-32) // (166)
//#define GIC_SRC_C0_PMU3         GIC_SRC_SPI(175-32) // (167)
//#define GIC_SRC_C0_AXI_ERROR    GIC_SRC_SPI(176-32) // (168)
//#define GIC_SRC_C0_AXI_WR       GIC_SRC_SPI(177-32) // (178)
//#define GIC_SRC_C0_AXI_RD       GIC_SRC_SPI(178-32) // (179)
//#define GIC_SRC_DBGRSTREQ0      GIC_SRC_SPI(179-32) // (179)
//#define GIC_SRC_DBGRSTREQ1      GIC_SRC_SPI(180-32) // (179)
//#define GIC_SRC_DBGRSTREQ2      GIC_SRC_SPI(181-32) // (179)
//#define GIC_SRC_DBGRSTREQ3      GIC_SRC_SPI(182-32) // (179)
//#define GIC_SRC_NVCPUMNTIRQ0    GIC_SRC_SPI(183-32) // (179)
//#define GIC_SRC_NVCPUMNTIRQ1    GIC_SRC_SPI(184-32) // (179)
//#define GIC_SRC_NVCPUMNTIRQ2    GIC_SRC_SPI(185-32) // (179)
//#define GIC_SRC_NVCPUMNTIRQ3    GIC_SRC_SPI(186-32) // (179)
//#define GIC_SRC_NCOMMIRQ0       GIC_SRC_SPI(187-32) // (179)
//#define GIC_SRC_NCOMMIRQ1       GIC_SRC_SPI(188-32) // (179)
//#define GIC_SRC_NCOMMIRQ2       GIC_SRC_SPI(189-32) // (179)
//#define GIC_SRC_NCOMMIRQ3       GIC_SRC_SPI(190-32) // (179)
//#define GIC_SRC_DBGPWRUPREQ_OUT   GIC_SRC_SPI(191-32) // (179)

#define GIC_IRQ_NUM             GIC_SRC_C0_nGIC_PMU_INT+1
#define GIC_LPI_NUM             (8192 + 0x100)            //fake
/*legacy defines*/
#define GIC_SRC_CPUIDLE_PCK600_CPU  GIC_SRC_PCK600_CPU

/* processer target */
#define GIC_CPU_TARGET(_n)  (1 << (_n))
#define GIC_CPU_TARGET0     GIC_CPU_TARGET(0)
#define GIC_CPU_TARGET1     GIC_CPU_TARGET(1)
#define GIC_CPU_TARGET2     GIC_CPU_TARGET(2)
#define GIC_CPU_TARGET3     GIC_CPU_TARGET(3)
#define GIC_CPU_TARGET4     GIC_CPU_TARGET(4)
#define GIC_CPU_TARGET5     GIC_CPU_TARGET(5)
#define GIC_CPU_TARGET6     GIC_CPU_TARGET(6)
#define GIC_CPU_TARGET7     GIC_CPU_TARGET(7)
/* trigger mode */
#define GIC_SPI_LEVEL_TRIGGER   (0) //2b'00
#define GIC_SPI_EDGE_TRIGGER    (2) //2b'10

/* processer target */
#define GIC_CPU_TARGET(_n)    (1 << (_n))
#define GIC_CPU_TARGET0        GIC_CPU_TARGET(0)
#define GIC_CPU_TARGET1        GIC_CPU_TARGET(1)
#define GIC_CPU_TARGET2        GIC_CPU_TARGET(2)
#define GIC_CPU_TARGET3        GIC_CPU_TARGET(3)
#define GIC_CPU_TARGET4        GIC_CPU_TARGET(4)
#define GIC_CPU_TARGET5        GIC_CPU_TARGET(5)
#define GIC_CPU_TARGET6        GIC_CPU_TARGET(6)
#define GIC_CPU_TARGET7        GIC_CPU_TARGET(7)
/* trigger mode */
#define GIC_SPI_LEVEL_TRIGGER    (0)    //2b'00
#define GIC_SPI_EDGE_TRIGGER    (2)    //2b'10


void init_gic(void);
s32 set_irq_target(u32 irq_no, u32 target);
#endif
