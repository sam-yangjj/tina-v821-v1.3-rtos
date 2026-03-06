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

#include <stdio.h>
#include "spi.h"
#include "spinand.h"
#include "spinand_id.h"
#include "spinand_common.h"
#include "spinand_basic.h"

unsigned int def_freq = 100; // 100MHz

u32 SPN_BLOCK_SIZE = 0;
u32 SPN_BLK_SZ_WIDTH = 0;
u32 SPN_PAGE_SIZE = 0;
u32 SPN_PG_SZ_WIDTH = 0;
u32 page_for_bad_block = 0;
u32 OperationOpt = 0;

struct aw_spinand_phy_info *spinand_phyinfo;

static s32 analyze_spinand_system(void)
{
	s32 result;
	unsigned char reg = 0;
	int dummy = 1;
	u8 id[8];

	/*reset the nand flash chip on boot chip select*/
	result = spi_nand_reset(0, 0);
	if (result) {
		printf("spi nand reset fail!\n");
		return -1;
	}

retry_nodummy:
		/*read id*/
		spi_nand_read_id(0, 0, id, 8, dummy);
		printf("Read SPI Nand ID: %02x %02x %02x %02x %02x %02x %02x %02x\n",
				id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
		/*match id table*/
		spinand_phyinfo = spinand_match_id(id);
		if (spinand_phyinfo == NULL) {
			if (dummy == 0) {
				printf("read the spinand id is err\n");
				printf("the read id: %02x %02x %02x %02x %02x %02x %02x %02x\n",
						id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
				return -1;
			}
			dummy = 0;
			goto retry_nodummy;
		}

	OperationOpt = spinand_phyinfo->OperationOpt;

	reg |= CFG_ECC_ENABLE;
	if (OperationOpt & SPINAND_QUAD_READ)
		reg |= CFG_QUAD_ENABLE;
	if (OperationOpt & SPINAND_QUAD_NO_NEED_ENABLE)
		reg &= ~CFG_QUAD_ENABLE;

	if (spinand_phyinfo->NandID[0] == 0xef) {
		/*winbond: 0x18,bit3 is BUF mode;*/
		reg |= CFG_BUF_MODE;
		spi_nand_setotp(0, 0, reg);
	} else if (spinand_phyinfo->NandID[0] == 0x98) {
		/* toshiba: bit2 is BBI,bit1 is HSE;*/
		reg |= 0x06;
		spi_nand_setotp(0, 0, reg);
	} else if (spinand_phyinfo->NandID[0] == 0xa1) {
		/* XTX spinand */
		spi_nand_setotp(0, 0, 0x0);
		spi_nand_setecc(0, 0, 0x10);
	} else {
		spi_nand_setotp(0, 0, reg);  /*other:0x10,bit3~1 don't care*/
	}

	if ((OperationOpt & SPINAND_QUAD_READ) &&
			!(OperationOpt & SPINAND_QUAD_NO_NEED_ENABLE)) {
		reg = 0;
		spi_nand_getotp(0, 0, &reg);
		if (!(reg & CFG_QUAD_ENABLE)) {
			OperationOpt &= ~SPINAND_QUAD_READ;
			printf("spinand quad mode error\n");
		}
	}

	spi_nand_setblocklock(0, 0, 0);

	return 0;
}

s32 spinand_get_flashinfo(spiflash_info_t *param)
{

//	param->chip_cnt = ;
//	param->blk_cnt_per_chip = ;
	param->pagesize = spinand_phyinfo->SectCntPerPage;
	param->blocksize = param->pagesize * spinand_phyinfo->PageCntPerBlk;
	param->pagewithbadflag  = 0 ;   /* fix page 0 as bad flag page index*/

	return 0;
}

s32 spinand_init(void)
{
	s32 ret;
	spiflash_info_t param;

	ret = spi_init(0);
	if (ret) {
		printf("spi_init fail\n");
		goto error;
	}

	ret = analyze_spinand_system();
	if (ret) {
		printf("spi nand scan fail\n");
		goto error;
	}

	if (spinand_get_flashinfo(&param) == -1)
		goto error;

	page_for_bad_block = param.pagewithbadflag;
	SPN_BLOCK_SIZE = param.blocksize * SECTOR_SIZE;
	SPN_PAGE_SIZE = param.pagesize * SECTOR_SIZE;

	if (spinand_phyinfo && spinand_phyinfo->Freq != def_freq) {
		spi_set_clk(0, spinand_phyinfo->Freq);
		spi_sample_point_delay_set_legacy(spinand_phyinfo->Freq);
	}

	return 0;

error:
	spinand_deinit();
	return -1;

}

s32 spinand_deinit(void)
{
	spi_deinit(0);

	/* close nand flash bus clock gate */
	/*NAND_CloseAHBClock();*/

	return 0;
}

s32 spinand_check_badblock(u32 block_num)
{
	struct spinand_physical_param  para;
	u8  oob_buf[16];
	s32 ret;

	para.chip = 0;
	para.block = block_num;
	para.page = 0;
	para.mainbuf = NULL;
	para.oobbuf = oob_buf;
	ret = spi_nand_read_single_page(&para, 0, 1);
	if (ret != NAND_OP_TRUE) {
		printf("Check_BadBlock: read_single_page FAIL\n");
		return NAND_OP_FALSE;
	}
	if (oob_buf[0] != 0xFF) {
		printf("oob_buf[0] = %x\n", oob_buf[0]);
		return SPINAND_BAD_BLOCK;
	} else
		return SPINAND_GOOD_BLOCK;
}

s32 spinand_read(u32 sector_num, void *buffer, u32 N)
{
	struct spinand_physical_param  para;
	u8  oob_buf[16];
	u32 page_nr;
	u32 scts_per_page = SPN_PAGE_SIZE >> SCT_SZ_WIDTH;
	u32 start_page;
	u32 i;
	u32 blk_num;
	u32 not_full_page_flag = 0;
	u32 data_size;

	para.chip = 0;
	blk_num = sector_num / (SPN_BLOCK_SIZE >> SCT_SZ_WIDTH);
	para.block = blk_num;
	printf("block:%d\n", para.block);
	start_page = (sector_num % (SPN_BLOCK_SIZE >> \
				SCT_SZ_WIDTH)) / scts_per_page;
	para.oobbuf = oob_buf;
	page_nr = N / scts_per_page;
	if (N % scts_per_page) {
		page_nr++;
		not_full_page_flag = 1;
	}
	for (i = 0; i < page_nr; i++) {
		para.mainbuf = (u8 *)buffer + SPN_PAGE_SIZE * i;
		para.page = start_page + i;
		printf("page:%d \n", para.page);
		data_size = SPN_PAGE_SIZE;
		if ((i == (page_nr - 1)) && not_full_page_flag)
			data_size = (N << SCT_SZ_WIDTH) - (SPN_PAGE_SIZE * i);
		if (spi_nand_read_single_page(&para, data_size, 0) != NAND_OP_TRUE)
			return NAND_OP_FALSE;
	}

	return NAND_OP_TRUE;
}