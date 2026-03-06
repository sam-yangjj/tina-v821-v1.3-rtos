/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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

#ifndef _MMC_DEF_H_
#define _MMC_DEF_H_

#include <stdio.h>
#include "mmc_platform.h"

#define SUNXI_MMC0_BASE				(SUNXI_SMHC0_BASE)
#define SUNXI_MMC1_BASE				(SUNXI_SMHC1_BASE)
#define SUNXI_MMC2_BASE				(SUNXI_SMHC2_BASE)

#define MAX_MMC_NUM		2
#define MMC_TRANS_BY_DMA

//#define MMC_DEBUG
#define MMC_REG_FIFO_OS		(0x200)

#define MMC_GPIO_BASE		SUNXI_GPIO_BASE
#define MMC_REG_BASE		SUNXI_MMC0_BASE
#if (!defined(CONFIG_ARCH_SUN300IW1))
#define CCMU_REG_SMHC0_BGR  	CCMU_SMHC0_BGR_REG
#else
#define CCMU_REG_SMHC0_BGR_GATING  	CCMU_BUS_CLK_GATING1_REG
#define CCMU_REG_SMHC0_BGR_RESET  	CCMU_BUS_RESET1_REG
#endif
#define CCMU_MMC0_CLK_BASE 	CCMU_SDMMC0_CLK_REG
#if (!defined(CONFIG_ARCH_SUN300IW1))
#define CCMU_MMC2_CLK_BASE 	CCMU_SDMMC2_CLK_REG
#endif
/*#define CCMU_PLL5_CLK_BASE    0x01c20020*/
#define __mmc_be32_to_cpu(x)	((0x000000ff&((x)>>24)) | (0x0000ff00&((x)>>8)) | 			\
							 (0x00ff0000&((x)<<8)) | (0xff000000&((x)<<24)))

#ifdef MMC_TRANS_BY_DMA
/*change address to iomem pointer*/
typedef unsigned int phys_addr_t;
#define IOMEM_ADDR(addr) ((volatile void __iomem *)((phys_addr_t)(addr)))
#define PT_TO_U32(p)   ((u32)((phys_addr_t)(p)))
#define PT_TO_U(p)   ((phys_addr_t)(p))
#define WR_MB() 	__asm__ __volatile__ ("fence iorw, iorw" : : : "memory")
#endif

typedef signed char s8;
typedef unsigned char u8;
typedef signed int s32;
typedef unsigned int u32;
typedef unsigned long long uint64_t;

#ifndef NULL
#define NULL (void *)0
#endif

#ifdef MMC_DEBUG
#define mmcinfo(fmt, ...)	printf("[mmc]: "fmt, ##__VA_ARGS__)
#define mmcdbg(fmt, ...)	printf("[mmc]: "fmt, ##__VA_ARGS__)
#define mmcmsg(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define mmcinfo(fmt, ...) do { } while(0)
#define mmcdbg(fmt, ...)	do { } while(0)
#define mmcmsg(fmt, ...)	do { } while(0)
#endif

#ifndef readb
#define readb(addr)         (*((volatile u8  *)(addr)))
#endif
#ifndef readl
#define readl(addr)         (*((volatile u32  *)(addr)))
#endif
#ifndef writeb
#define writeb(v, addr)     (*((volatile u8  *)(addr)) = (u8)(v))
#endif
#ifndef writel
#define writel(v, addr)     (*((volatile u32  *)(addr)) = (u32)(v))
#endif

#define DMAC_DES_BASE_IN_SRAM		(0x20000 + 0xC000)
#ifdef CONFIG_ARCH_SUN300IW1
#define DMAC_DES_BASE_IN_SDRAM		(0x82000000)
#else
#define DMAC_DES_BASE_IN_SDRAM		(0x42000000)
#endif
#define DRAM_START_ADDR				(0x40000000)

#define DRIVER_VER  "2024-11-11 20:50"

#endif	/* _MMC_DEF_H_ */

