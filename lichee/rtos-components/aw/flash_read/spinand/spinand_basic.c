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
#include "spinand_basic.h"
#include "spinand_common.h"

s32 spi_nand_getsr(u32 spi_no, u8 *reg)
{
	s32 ret = NAND_OP_TRUE;
	u8 sdata[2] ;
	u32 txnum;
	u32 rxnum;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	/*status adr:0xc0;feature adr:0xb0;protection adr:0xa0*/
	sdata[1] = 0xc0;
	spi_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spi_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0);

	return ret;
}

s32 spi_wait_status(u32 spi_no, u8 *status)
{
	s32 timeout = 0xfffff;
	s32 ret = NAND_OP_TRUE;

	while (1) {
		ret = spi_nand_getsr(spi_no, status);
		if (ret != NAND_OP_TRUE) {
			printf("m0_spi_wait_status getsr fail!\n");
			return ret;
		}
		if (!(*(u8 *)status & SPI_NAND_READY))
			break;
		if (timeout < 0) {
			printf("m0_spi_wait_status timeout!\n");
			return -ERR_TIMEOUT;
		}
		timeout--;
	}
	return NAND_OP_TRUE;
}

/*mode=0:check ecc status  mode=1:check operation status*/
s32 spi_nand_read_status(u32 spi_no, u32 chip, u8 status, u32 mode)
{
	s32 ret = NAND_OP_TRUE;

	spi_sel_ss(spi_no, chip);

	if (mode) {
		ret = spi_wait_status(spi_no, &status);
		if (ret != NAND_OP_TRUE)
			return ret;

		if (status & SPI_NAND_ERASE_FAIL) {
			printf("spi_nand_read_status : erase fail,\
					status = 0x%x\n", status);
			ret = NAND_OP_FALSE;
		}
		if (status & SPI_NAND_WRITE_FAIL) {
			printf("spi_nand_read_status : write fail,\
					status = 0x%x\n", status);
			ret = NAND_OP_FALSE;
		}
	} else {
		ret = spi_wait_status(spi_no, &status);
		if (ret != NAND_OP_TRUE)
			return ret;

		if (((status >> SPI_NAND_ECC_FIRST_BIT) & SPI_NAND_ECC_BITMAP)
				== 0x0) {
			ret = NAND_OP_TRUE;
		} else if (((status >> SPI_NAND_ECC_FIRST_BIT) & \
					SPI_NAND_ECC_BITMAP) == 0x1) {
			ret = NAND_OP_TRUE;
		} else if (((status >> SPI_NAND_ECC_FIRST_BIT) & \
					SPI_NAND_ECC_BITMAP) == 0x2) {
			printf("ecc error 0x%x\n", status);
			ret = -ERR_ECC;
		}
	}

	return ret;
}

s32 spi_nand_setecc(u32 spi_no, u32 chip, u8 reg)
{
	s32 ret = NAND_OP_TRUE;
	u8 sdata[3];
	u32 txnum;
	u32 rxnum;
	u8 status = 0;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	sdata[1] = 0x90;       /*feature adr:0x90 for XTX spinand*/
	sdata[2] = reg;

	spi_sel_ss(spi_no, chip);

	spi_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spi_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = spi_wait_status(spi_no, &status);
	if (ret != NAND_OP_TRUE)
		return ret;

	return ret;
}

 s32 spi_nand_setblocklock(u32 spi_no, u32 chip, u8 reg)
{
	s32 ret = NAND_OP_TRUE;
	u8 sdata[3];
	u32 txnum;
	u32 rxnum;
	u8 status = 0;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	/*status adr:0xc0;feature adr:0xb0;protection adr:0xa0*/
	sdata[1] = 0xa0;
	sdata[2] = reg;

	spi_sel_ss(spi_no, chip);

	spi_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spi_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = spi_wait_status(spi_no, &status);
	if (ret != NAND_OP_TRUE)
		return ret;

	return ret;
}

 s32 spi_nand_setotp(u32 spi_no, u32 chip, u8 reg)
{
	s32 ret = NAND_OP_TRUE;
	u8 sdata[3];
	u32 txnum;
	u32 rxnum;
	u8 status = 0;

	txnum = 3;
	rxnum = 0;

	sdata[0] = SPI_NAND_SETSR;
	/*status adr:0xc0;feature adr:0xb0;protection adr:0xa0*/
	sdata[1] = 0xb0;
	sdata[2] = reg;

	spi_sel_ss(spi_no, chip);

	spi_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spi_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = spi_wait_status(spi_no, &status);
	if (ret != NAND_OP_TRUE)
		return ret;

	return ret;
}

 s32 spi_nand_getotp(u32 spi_no, u32 chip, u8 *reg)
{
	s32 ret = NAND_OP_TRUE;
	u8 sdata[3];
	u32 txnum;
	u32 rxnum;
	u8 status = 0;

	txnum = 2;
	rxnum = 1;

	sdata[0] = SPI_NAND_GETSR;
	/*status adr:0xc0;feature adr:0xb0;protection adr:0xa0*/
	sdata[1] = 0xb0;

	spi_sel_ss(spi_no, chip);

	spi_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spi_rw(spi_no, txnum, (void *)sdata, rxnum, reg, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = spi_wait_status(spi_no, &status);
	if (ret != NAND_OP_TRUE)
		return ret;

	return ret;
}

s32 spi_nand_reset(u32 spi_no, u32 chip)
{
	u8 sdata = SPI_NAND_RESET;
	s32 ret = NAND_OP_TRUE;
	u32 txnum;
	u32 rxnum;
	u8  status = 0;

	txnum = 1;
	rxnum = 0;

	spi_sel_ss(spi_no, chip);

	spi_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spi_rw(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = spi_wait_status(spi_no, &status);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = NAND_OP_TRUE;

	return ret;
}

s32 spi_nand_read_id(u32 spi_no, u32 chip, u8 *id, u8 len, int dummy)
{
	u8 txbuf[2] = {SPI_NAND_RDID, 0x00};
	s32 ret = NAND_OP_TRUE;
	u32 txnum;
	u32 rxnum;

	if (dummy)
		txnum = 2;
	else
		txnum = 1;

	rxnum = len;

	spi_sel_ss(spi_no, chip);

	spi_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spi_rw(spi_no, txnum, (void *)&txbuf, rxnum, (void *)id, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = NAND_OP_TRUE;

	return ret;
}

s32 spi_nand_read_x1(u32 spi_no, u32 page_num, u32 mbyte_cnt,
		u32 sbyte_cnt, void *mbuf, void *sbuf, u32 column)
{
	u32 txnum;
	u32 rxnum;
	u32 page_addr = page_num ;
	u8  sdata[8] = {0};
	s32 ret = NAND_OP_TRUE;
	u8 status = 0;
	s32 ecc_status = 0;
	u8 bad_flag;
	u8 plane_select;

	plane_select = (page_addr / (SPN_BLOCK_SIZE / SPN_PAGE_SIZE)) & 0x1;

	txnum = 4;
	rxnum = 0;

	sdata[0] = SPI_NAND_PAGE_READ;
	sdata[1] = (page_addr>>16)&0xff; /*9dummy+15bit row adr*/
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;

	spi_config_dual_mode(spi_no, 0, 0, txnum);
	ret = spi_rw(spi_no, txnum, (void *)sdata, rxnum, NULL, 0);
	if (ret != NAND_OP_TRUE)
		return ret;

	ret = spi_wait_status(spi_no, &status);
	if (ret != NAND_OP_TRUE)
		return ret;

	ecc_status = spi_nand_read_status(spi_no, 0, status, 0);
	if (ecc_status == -ERR_ECC)
		printf("ecc err\n");

	if (mbuf) {
		if (OperationOpt & SPINAND_ONEDUMMY_AFTER_RANDOMREAD) {
			txnum = 5;
			rxnum = mbyte_cnt;

			if (OperationOpt & SPINAND_QUAD_READ)
				sdata[0] = SPI_NAND_READ_X4;
			else if (OperationOpt & SPINAND_DUAL_READ)
				sdata[0] = SPI_NAND_READ_X2;
			else
				sdata[0] = SPI_NAND_FAST_READ_X1;

			sdata[1] = 0x0; /*1byte dummy*/
			/*4bit dummy,12bit column adr*/
			sdata[2] = ((column >> 8) & 0xff);
			sdata[3] = column & 0xff;
			sdata[4] = 0x0; /*1byte dummy*/
		} else {
			/*read main data*/
			txnum = 4;
			rxnum = mbyte_cnt;

			if (OperationOpt & SPINAND_QUAD_READ)
				sdata[0] = SPI_NAND_READ_X4;
			else if (OperationOpt & SPINAND_DUAL_READ)
				sdata[0] = SPI_NAND_READ_X2;
			else
				sdata[0] = SPI_NAND_FAST_READ_X1;

			if (OperationOpt & SPINAND_TWO_PLANE_SELECT) {
				/*3bit dummy,1bit plane,12bit column adr*/
				if (plane_select)
					sdata[1] = ((column >> 8) & 0x0f)|0x10;
				else
					sdata[1] = ((column >> 8) & 0x0f);
			} else {
				/*4bit dummy,12bit column adr*/
				sdata[1] = ((column >> 8) & 0xff);
			}
				sdata[2] = column & 0xff;
				sdata[3] = 0x0; /*1byte dummy*/
		}
		/*signal read, dummy:1byte, signal tx:3*/
		if (OperationOpt & SPINAND_QUAD_READ)
			spi_config_dual_mode(spi_no, 2, 0, txnum);
		else if (OperationOpt & SPINAND_DUAL_READ)
			spi_config_dual_mode(spi_no, 1, 0, txnum);
		else
			spi_config_dual_mode(spi_no, 0, 0, txnum);
		ret = spi_rw(spi_no, txnum,
				(void *)sdata, rxnum, mbuf, 0);
		if (ret != NAND_OP_TRUE)
			return ret;
	}

	if (sbuf) {
		if (OperationOpt & SPINAND_ONEDUMMY_AFTER_RANDOMREAD) {
			txnum = 5;
			rxnum = 1;

			if (OperationOpt & SPINAND_QUAD_READ)
				sdata[0] = SPI_NAND_READ_X4;
			else if (OperationOpt & SPINAND_DUAL_READ)
				sdata[0] = SPI_NAND_READ_X2;
			else
				sdata[0] = SPI_NAND_FAST_READ_X1;

			sdata[1] = 0x0; /*1byte dummy*/
			/*4bit dummy,12bit column adr*/
			sdata[2] = (((512 * (SPN_PAGE_SIZE >> \
				SCT_SZ_WIDTH)) >> 8) & 0xff);
			sdata[3] = (512 * (SPN_PAGE_SIZE >> SCT_SZ_WIDTH)) & 0xff;
			sdata[4] = 0x0; /*1byte dummy*/
		} else {
			/*read bad mark area*/
			txnum = 4;
			rxnum = sbyte_cnt;

			if (OperationOpt & SPINAND_QUAD_READ)
				sdata[0] = SPI_NAND_READ_X4;
			else if (OperationOpt & SPINAND_DUAL_READ)
				sdata[0] = SPI_NAND_READ_X2;
			else
				sdata[0] = SPI_NAND_FAST_READ_X1;

			if (OperationOpt & SPINAND_TWO_PLANE_SELECT) {
			/*3bit dummy,1bit plane,12bit column adr*/
				if (plane_select)
					sdata[1] = (((512 * (SPN_PAGE_SIZE >>\
					SCT_SZ_WIDTH)) >> 8) & 0x0f) | 0x10;
				else
					sdata[1] = (((512 * (SPN_PAGE_SIZE >>\
						SCT_SZ_WIDTH)) >> 8) & 0x0f);
			} else {
				/*4bit dummy,12bit column adr*/
				sdata[1] = (((512 * (SPN_PAGE_SIZE >>\
						SCT_SZ_WIDTH)) >> 8) & 0xff);
			}
			sdata[2] = (512 * (SPN_PAGE_SIZE >> \
						SCT_SZ_WIDTH)) & 0xff;
			sdata[3] = 0x0; /*1byte dummy*/
		}
		/*signal read, dummy:1byte, signal tx:3*/
		if (OperationOpt & SPINAND_QUAD_READ)
			spi_config_dual_mode(spi_no, 2, 0, txnum);
		else if (OperationOpt & SPINAND_DUAL_READ)
			spi_config_dual_mode(spi_no, 1, 0, txnum);
		else
			spi_config_dual_mode(spi_no, 0, 0, txnum);

		ret = spi_rw(spi_no, txnum,
				(void *)sdata, rxnum, &bad_flag, 0);
		if (ret != NAND_OP_TRUE)
			return ret;

		if (bad_flag != 0xff)
			*(u8 *)sbuf = 0;
		else
			*(u8 *)sbuf = 0xff;

	}

	return 0;
}

s32 spi_nand_read_single_page(struct spinand_physical_param *readop,
		u32 data_size, u32 spare_only_flag)
{
	s32 ret = NAND_OP_TRUE;
	u32 addr;

	addr = readop->block * SPN_BLOCK_SIZE / SPN_PAGE_SIZE + readop->page;

	spi_sel_ss(0, readop->chip);

	if (spare_only_flag)
		ret = spi_nand_read_x1(0, addr, 0, 16, NULL, readop->oobbuf, 0);
	else
		ret = spi_nand_read_x1(0, addr, data_size, 16,
				(u8 *)readop->mainbuf, readop->oobbuf, 0);

	return ret;
}

