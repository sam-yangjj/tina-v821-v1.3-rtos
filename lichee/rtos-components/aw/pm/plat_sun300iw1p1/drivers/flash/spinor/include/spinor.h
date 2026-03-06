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

#ifndef __SPINOR_H
#define __SPINOR_H

#include "spi_mem.h"

typedef struct {
	uint8_t			magic[8];
	int32_t			readcmd;
	int32_t			read_mode;
	int32_t			write_mode;
	int32_t			flash_size;
	int32_t			addr4b_opcodes;
	int32_t			erase_size;
	int32_t			delay_cycle;    //When the frequency is greater than 60MHZ configured as 1;when the frequency is less than 24MHZ configured as 2;and other 3
	int32_t			lock_flag;
	int32_t			frequency;
	unsigned int		sample_delay;
	unsigned int		sample_mode;
	enum spi_nor_protocol	read_proto;
	enum spi_nor_protocol	write_proto;
	uint8_t			read_dummy;
} spinor_info_t;

/* Erase commands */
#define CMD_ERASE_4K			0x20
#define CMD_ERASE_CHIP			0xc7
#define CMD_ERASE_64K			0xd8

/* Write commands */
#define CMD_WRITE_STATUS		0x01
#define CMD_WRITE_STATUS1		0x31
#define CMD_WRITE_CONFIG1		0xb1
#define CMD_PAGE_PROGRAM		0x02
#define CMD_WRITE_DISABLE		0x04
#define CMD_WRITE_ENABLE		0x06
#define CMD_QUAD_PAGE_PROGRAM		0x32

/* Used for Macronix and Winbond flashes. */
#define SPINOR_OP_EN4B			0xb7	/* Enter 4-byte mode */
#define SPINOR_OP_EX4B			0xe9	/* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define SPINOR_OP_BRWR			0x17	/* Bank register write */
#define SPINOR_OP_BRRD			0x16	/* Bank register read */
#define SPINOR_OP_CLSR			0x30	/* Clear status register 1 */

/* Read commands */
#define CMD_READ_ARRAY_SLOW		0x03
#define CMD_READ_ARRAY_SLOW_4B		0x13
#define CMD_READ_ARRAY_FAST		0x0b
#define CMD_READ_ARRAY_FAST_4B		0x0c
#define CMD_READ_DUAL_OUTPUT_FAST	0x3b
#define CMD_READ_DUAL_IO_FAST		0xbb
#define CMD_READ_QUAD_OUTPUT_FAST	0x6b
#define CMD_READ_QUAD_IO_FAST		0xeb
#define CMD_READ_ID			0x9f
#define CMD_READ_STATUS			0x05
#define CMD_READ_STATUS1		0x35
#define CMD_READ_CONFIG			0x35
#define CMD_READ_CONFIG1		0xb5
#define CMD_FLAG_STATUS			0x70

/*work mode*/
#define SPINOR_QUAD_MODE		4
#define SPINOR_DUAL_MODE		2
#define SPINOR_SINGLE_MODE		1

#define SPINOR_OP_READ_1_1_2    	0x3b    /* Read data bytes (Dual SPI) */
#define SPINOR_OP_READ_1_1_4    	0x6b    /* Read data bytes (Quad SPI) */
#define SPINOR_OP_READ_1_4_4		0xeb	/* Read data bytes (Quad I/O SPI) */
#define SPINOR_OP_READ_1_4_4_DTR	0xed
/* 4-byte address opcodes - used on Spansion and some Macronix flashes. */
#define SPINOR_OP_READ4_1_1_2   	0x3c    /* Read data bytes (Dual SPI) */
#define SPINOR_OP_READ4_1_1_4   	0x6c    /* Read data bytes (Quad SPI) */
#define SPINOR_OP_READ4_1_4_4		0xec	/* Read data bytes (Quad I/O SPI) */
#define SPINOR_OP_READ4_1_4_4_DTR	0xee

/* CFI Manufacture ID's */
#define SPI_FLASH_CFI_MFR_SPANSION      0x01
//#define SPI_FLASH_CFI_MFR_STMICRO       0x20
#define SPI_FLASH_CFI_MFR_XMC	        0x20
#define SPI_FLASH_CFI_MFR_MACRONIX      0xc2
#define SPI_FLASH_CFI_MFR_SST           0xbf
#define SPI_FLASH_CFI_MFR_WINBOND       0xef
#define SPI_FLASH_CFI_MFR_ATMEL         0x1f
#define SPI_FLASH_CFI_MFR_GIGADEVICE    0xc8
#define SPI_FLASH_CFI_MFR_ADESTO        0x1f
#define SPI_FLASH_CFI_MFR_ESMT	        0x1c
#define SPI_FLASH_CFI_MFR_PUYA	        0x85
#define SPI_FLASH_CFI_MFR_ZETTA	        0xba
#define SPI_FLASH_CFI_MFR_BOYA	        0x68
#define SPI_FLASH_CFI_MFR_XTX	        0x0b
#define SPI_FLASH_CFI_MFR_FM	        0xa1

#define STATUS_WIP			(1 << 0)
#define STATUS_QEB_WINSPAN		(1 << 1)
#define STATUS_QEB_MXIC			(1 << 6)
#define STATUS_QEB_GIGA			(1 << 1)
#define STATUS_QEB_XMC			(1 << 1)
//#define STATUS_QEB_STMICRO		(1 << 3)


int spinor_init(int stage);
int spinor_exit(int force);
int spinor_read(uint32_t start, uint32_t sector_cnt, void *buffer);
void set_delay_para(void);
int spinor_set_boot_param_start(uint32_t start);


#endif
