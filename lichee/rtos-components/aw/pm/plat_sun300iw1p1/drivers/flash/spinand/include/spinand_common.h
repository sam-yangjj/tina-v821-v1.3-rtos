/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.
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

#ifndef __SPINAND_COMMON_H
#define __SPINAND_COMMON_H

#define FPGA_PLATFORM 1

typedef signed char s8;
typedef unsigned char u8;
typedef signed int s32;
typedef unsigned int u32;

#define readb(addr)         (*((volatile unsigned char  *)(addr)))
#define readl(addr)         (*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)     (*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writel(v, addr)     (*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

extern u32 SPN_BLOCK_SIZE;
extern u32 SPN_BLK_SZ_WIDTH;
extern u32 SPN_PAGE_SIZE;
extern u32 SPN_PG_SZ_WIDTH;
extern u32 page_for_bad_block;
extern u32 OperationOpt;

#define BIT(nr)                 (1UL << (nr))

#define SECTOR_SIZE             512U
#define SCT_SZ_WIDTH            9U

#define NAND_OP_TRUE            (0)				/*define the successful return value*/
#define NAND_OP_FALSE           (-1)			/*define the failed return value*/
#define ERR_TIMEOUT             14				/*hardware timeout*/
#define ERR_ECC                 12				/*too much ecc error*/
#define ERR_NANDFAIL            13				/*nand flash program or erase fail*/
#define SPINAND_BAD_BLOCK       1
#define SPINAND_GOOD_BLOCK	0

#define CFG_BUF_MODE		BIT(3)
#define CFG_ECC_ENABLE		BIT(4)
#define CFG_QUAD_ENABLE		BIT(0)

#define SPINAND_DUAL_READ			BIT(0)
#define SPINAND_QUAD_READ			BIT(1)
#define SPINAND_QUAD_PROGRAM			BIT(2)
#define SPINAND_QUAD_NO_NEED_ENABLE		BIT(3)

#define SPINAND_TWO_PLANE_SELECT	 			(1<<7)			/*nand flash need plane select for addr*/
#define SPINAND_ONEDUMMY_AFTER_RANDOMREAD			(1<<8)			/*nand flash need a dummy Byte after random fast read*/

struct aw_spinand_phy_info {
#define MAX_ID_LEN 8
	unsigned char NandID[MAX_ID_LEN];
	unsigned int PageCntPerBlk;
	unsigned int SectCntPerPage;
	unsigned int OperationOpt;
	unsigned int Freq; 	/*MHz*/
};

typedef struct spiflash_info {
	u32 chip_cnt;
	u32 blk_cnt_per_chip;
	u32 blocksize;
	u32 pagesize;
	u32 pagewithbadflag; /*bad block flag was written at the first byte of spare area of this page*/
} spiflash_info_t;

struct spinand_physical_param {
	u32  chip; /*chip no*/
	u32  block; /* block no within chip*/
	u32  page; /* apge no within block*/
	u32  sectorbitmap;
	void   *mainbuf; /*data buf*/
	void   *oobbuf; /*oob buf*/
};

#endif
