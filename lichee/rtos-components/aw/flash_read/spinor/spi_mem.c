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

#include "spi_mem.h"
#include "spif.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_PM_STANDBY_MEMORY)
#include <pm_mem.h>
#endif

#define CONFIG_SUNXI_SPIF_V0   0x10000
#define CONFIG_SUNXI_SPIF_V1   0x10001
#define CONFIG_SUNXI_SPIF_V2   0x10002

#define SPIF_DATA_LEN_1M 1024 * 1024

#define SPIF_DATA_LEN_ALIGN    ROUND_UP(512, CONFIG_SYS_CACHELINE_SIZE)
#define SPIF_OP_LEN_ALIGN      ROUND_UP(544, CONFIG_SYS_CACHELINE_SIZE)

#if (defined CONFIG_ARCH_SUN300IW1) && (defined CONFIG_PM_STANDBY_MEMORY)
__standby_unsaved_bss char spif_align_buffer[SPIF_DATA_LEN_ALIGN + CONFIG_SYS_CACHELINE_SIZE - 1];
__standby_unsaved_bss uint8_t spif_op_buffer[SPIF_OP_LEN_ALIGN + CONFIG_SYS_CACHELINE_SIZE - 1] __aligned(CONFIG_SYS_CACHELINE_SIZE);
#else
char spif_align_buffer[SPIF_DATA_LEN_ALIGN + CONFIG_SYS_CACHELINE_SIZE - 1];
uint8_t spif_op_buffer[SPIF_OP_LEN_ALIGN + CONFIG_SYS_CACHELINE_SIZE - 1] __aligned(CONFIG_SYS_CACHELINE_SIZE);
#endif

//extern void spif_print_descriptor(struct spif_descriptor_op *spif_op);

static int spif_select_buswidth(uint32_t buswidth)
{
	int width = 0;

	switch (buswidth) {
	case SPIF_SINGLE_MODE:
		width = 0;
		break;
	case SPIF_DUEL_MODE:
		width = 1;
		break;
	case SPIF_QUAD_MODE:
		width = 2;
		break;
	case SPIF_OCTAL_MODE:
		width = 3;
		break;
	default:
		SPIF_INFO("Parameter error with buswidth:%d\n", buswidth);
	}
	return width;
}


int spif_mem_exec_op_v0(const struct spi_mem_op *op)
{
	int ret;
	struct spif_descriptor_op *spif_op;
	uint cache_buf[CONFIG_SYS_CACHELINE_SIZE] __aligned(CONFIG_SYS_CACHELINE_SIZE);
	uint desc_count = ((op->data.nbytes + SPIF_MAX_TRANS_NUM - 1) / SPIF_MAX_TRANS_NUM) + 1;
	uint desc_size = desc_count * sizeof(struct spif_descriptor_op);

	uint32_t desc_size_align = 0;

	if (CHECK_IS_NOALIGN(desc_size))
		desc_size_align = ROUND_UP(desc_size, CONFIG_SYS_CACHELINE_SIZE);
	else
		desc_size_align = desc_size;

	uint8_t spif_op_buffer[desc_size_align + CONFIG_SYS_CACHELINE_SIZE - 1];
	spif_op = (struct spif_descriptor_op *)((uint32_t)(spif_op_buffer + CONFIG_SYS_CACHELINE_SIZE - 1) & ~(CONFIG_SYS_CACHELINE_SIZE - 1));

	memset(spif_op, 0, desc_size);
	memset(cache_buf, 0, CONFIG_SYS_CACHELINE_SIZE * sizeof(uint));

	/* set hburst type */
	spif_op->hburst_rw_flag &= ~HBURST_TYPE;
	spif_op->hburst_rw_flag |= HBURST_INCR16_TYPE;

	/* the last one descriptor */
	spif_op->hburst_rw_flag |= DMA_FINISH_FLASG;

	/* set DMA block len mode */
	spif_op->block_data_len &= ~DMA_BLK_LEN;
	spif_op->block_data_len |= DMA_BLK_LEN_64B;

	spif_op->addr_dummy_data_count |= SPIF_DES_NORMAL_EN;

	/* dispose cmd */
	if (op->cmd.opcode) {
		spif_op->trans_phase |= SPIF_CMD_TRANS_EN;
		spif_op->cmd_mode_buswidth |= op->cmd.opcode << SPIF_CMD_OPCODE_POS;
		/* set cmd buswidth */
		spif_op->cmd_mode_buswidth |=
			spif_select_buswidth(op->cmd.buswidth) << SPIF_CMD_TRANS_POS;
		if (op->cmd.buswidth != 1)
			spif_op->cmd_mode_buswidth |=
				spif_select_buswidth(op->cmd.buswidth) <<
				SPIF_DATA_TRANS_POS;
	}

	/* dispose addr */
	if (op->addr.nbytes) {
		spif_op->trans_phase |= SPIF_ADDR_TRANS_EN;
		spif_op->flash_addr = op->addr.val;
		if (op->addr.nbytes == 4) {//set 4byte addr mode
		    spif_op->addr_dummy_data_count &= SPIF_ADDR_SIZE_MASK;
		    spif_op->addr_dummy_data_count |= SPIF_ADDR_SIZE_32BIT;
		}
		/* set addr buswidth */
		spif_op->cmd_mode_buswidth |=
			spif_select_buswidth(op->addr.buswidth) <<
			SPIF_ADDR_TRANS_POS;
	}

	/* dispose mode */
	if (op->mode.val) {
		spif_op->trans_phase |= SPIF_MODE_TRANS_EN;
		spif_op->cmd_mode_buswidth |=
				*(uint8_t *)op->mode.val << SPIF_MODE_OPCODE_POS;
		/* set addr buswidth */
		spif_op->cmd_mode_buswidth |=
			spif_select_buswidth(op->mode.buswidth) <<
			SPIF_MODE_TRANS_POS;
	}

	/* dispose dummy */
	if (op->dummy.cycle) {
		spif_op->trans_phase |= SPIF_DUMMY_TRANS_EN;
		spif_op->addr_dummy_data_count |=
			(op->dummy.cycle << SPIF_DUMMY_NUM_POS);
	}

	/* dispose data */
	if (op->data.nbytes) {
		/* set data buswidth */
		spif_op->cmd_mode_buswidth |=
			spif_select_buswidth(op->data.buswidth) << SPIF_DATA_TRANS_POS;

		if (op->data.dir == SPI_MEM_DATA_IN) {
			spif_op->trans_phase |= SPIF_RX_TRANS_EN;
			if (op->data.nbytes < SPIF_MIN_TRANS_NUM)
				spif_op->data_addr = (uint32_t)cache_buf;
			else
				spif_op->data_addr = (uint32_t)op->data.buf.in;
			/* Write:1 DMA Write to dram */
			spif_op->hburst_rw_flag |= DMA_RW_PROCESS;
		} else {
			spif_op->trans_phase |= SPIF_TX_TRANS_EN;
			spif_op->data_addr = (uint32_t)op->data.buf.out;
			/* Read:0 DMA read for dram */
			spif_op->hburst_rw_flag &= ~DMA_RW_PROCESS;
		}
		if (op->data.nbytes < SPIF_MIN_TRANS_NUM &&
				op->data.dir == SPI_MEM_DATA_IN) {
			spif_op->addr_dummy_data_count |=
				SPIF_MIN_TRANS_NUM << SPIF_DATA_NUM_POS;
			spif_op->block_data_len |=
				SPIF_MIN_TRANS_NUM << SPIF_DATA_NUM_POS;
		} else if (op->data.nbytes <= SPIF_MAX_TRANS_NUM) {
			spif_op->addr_dummy_data_count |=
				op->data.nbytes << SPIF_DATA_NUM_POS;
			spif_op->block_data_len |=
				op->data.nbytes << SPIF_DATA_NUM_POS;
		} else {
			unsigned int total_len = op->data.nbytes;
			struct spif_descriptor_op *current_op = spif_op;

			spif_op->addr_dummy_data_count |=
				SPIF_MAX_TRANS_NUM << SPIF_DATA_NUM_POS;
			spif_op->block_data_len |=
				SPIF_MAX_TRANS_NUM << SPIF_DATA_NUM_POS;
			spif_op->hburst_rw_flag &= ~DMA_FINISH_FLASG;
			total_len -= SPIF_MAX_TRANS_NUM;

			while (total_len) {
				struct spif_descriptor_op *next_op = current_op + 1;

				memcpy(next_op, current_op, sizeof(struct spif_descriptor_op));

				current_op->next_des_addr = (unsigned int)next_op;
				//spif_print_descriptor(current_op);

				next_op->addr_dummy_data_count &= ~DMA_DATA_LEN;
				next_op->block_data_len &= ~DMA_DATA_LEN;

				if (total_len > SPIF_MAX_TRANS_NUM) {
					next_op->addr_dummy_data_count |=
						SPIF_MAX_TRANS_NUM << SPIF_DATA_NUM_POS;
					next_op->block_data_len |=
						SPIF_MAX_TRANS_NUM << SPIF_DATA_NUM_POS;
					total_len -= SPIF_MAX_TRANS_NUM;
				} else {
					next_op->addr_dummy_data_count |=
						total_len << SPIF_DATA_NUM_POS;
					next_op->block_data_len |=
						total_len << SPIF_DATA_NUM_POS;
					next_op->hburst_rw_flag |= DMA_FINISH_FLASG;
					next_op->next_des_addr = 0;
					total_len = 0;
				}

				next_op->data_addr =
					current_op->data_addr + SPIF_MAX_TRANS_NUM;
				next_op->flash_addr =
					current_op->flash_addr + SPIF_MAX_TRANS_NUM;
				current_op = next_op;
			}
			//spif_print_descriptor(current_op);
		}
	}

	ret = spif_xfer(spif_op, op->data.nbytes);

	if (op->data.nbytes < SPIF_MIN_TRANS_NUM &&
			op->data.dir == SPI_MEM_DATA_IN)
		memcpy((void *)op->data.buf.in,	(const void *)cache_buf,
					op->data.nbytes);
	spif_op = NULL;
	return ret;
}

int spif_mem_exec_op_v1(const struct spi_mem_op *op)
{
	struct spif_descriptor_op *spif_op = NULL;
	struct spif_descriptor_op *current_op = NULL;
	uint data_nbytes_once, data_nbytes;
	void *buf_in_once = op->data.buf.in;
	const void *buf_out_once = op->data.buf.out;
	uint64_t flash_addr = op->addr.val;
	data_nbytes = op->data.nbytes;
	uint8_t is_data_noalign = 0;
	char *cache_buf_align = NULL;
	size_t remain_len = 0, handle_len = 0;
	int ret;

	if (op->data.dir == SPI_MEM_DATA_IN) {
		if (CHECK_IS_NOALIGN((uint32_t)op->data.buf.in)  ||
			CHECK_IS_NOALIGN(op->data.nbytes))
			is_data_noalign = 1;
	} else {
		if (CHECK_IS_NOALIGN((uint32_t)op->data.buf.out) ||
			CHECK_IS_NOALIGN(op->data.nbytes))
			is_data_noalign = 1;
	}

spif_transfer:
	data_nbytes_once = data_nbytes > SPIF_DATA_LEN_1M ? SPIF_DATA_LEN_1M : data_nbytes;
	remain_len = data_nbytes_once;
	handle_len = 0;

	memset(spif_op_buffer, 0, sizeof(spif_op_buffer));
	spif_op = (struct spif_descriptor_op *)((uint32_t)(spif_op_buffer + CONFIG_SYS_CACHELINE_SIZE - 1) & ~(CONFIG_SYS_CACHELINE_SIZE - 1));
	current_op = spif_op;

	/* set hburst type */
	spif_op->hburst_rw_flag &= ~HBURST_TYPE;
	spif_op->hburst_rw_flag |= HBURST_INCR16_TYPE;

	/* set DMA block len mode */
	spif_op->block_data_len &= ~DMA_BLK_LEN;
	spif_op->block_data_len |= DMA_BLK_LEN_64B;

	spif_op->addr_dummy_data_count |= SPIF_DES_NORMAL_EN;

	/* dispose cmd */
	if (op->cmd.opcode) {
		spif_op->trans_phase |= SPIF_CMD_TRANS_EN;
		spif_op->cmd_mode_buswidth |= op->cmd.opcode << SPIF_CMD_OPCODE_POS;
		/* set cmd buswidth */
		spif_op->cmd_mode_buswidth |=
			spif_select_buswidth(op->cmd.buswidth) << SPIF_CMD_TRANS_POS;
		if (op->cmd.buswidth != 1)
			spif_op->cmd_mode_buswidth |=
				spif_select_buswidth(op->cmd.buswidth) <<
				SPIF_DATA_TRANS_POS;
	}

	/* dispose addr */
	if (op->addr.nbytes) {
		spif_op->trans_phase |= SPIF_ADDR_TRANS_EN;
		spif_op->flash_addr = flash_addr;
		if (op->addr.nbytes == 4) {//set 4byte addr mode
		    if (sunxi_spif_get_version() >= CONFIG_SUNXI_SPIF_V2) {
			    spif_op->addr_dummy_data_count &= ~SPIF_ADDR_SIZE_MASK_V2;
			    spif_op->addr_dummy_data_count |= SPIF_ADDR_SIZE_32BIT_V2;
			} else {
				spif_op->addr_dummy_data_count &= ~SPIF_ADDR_SIZE_MASK;
				spif_op->addr_dummy_data_count |= SPIF_ADDR_SIZE_32BIT;
			}
		} else {
			if (sunxi_spif_get_version() >= CONFIG_SUNXI_SPIF_V2) {
			    spif_op->addr_dummy_data_count &= ~SPIF_ADDR_SIZE_MASK_V2;
			    spif_op->addr_dummy_data_count |= SPIF_ADDR_SIZE_24BIT_V2;
			} else {
			    spif_op->addr_dummy_data_count &= ~SPIF_ADDR_SIZE_MASK;
			    spif_op->addr_dummy_data_count |= SPIF_ADDR_SIZE_24BIT;
			}
		}
		/* set addr buswidth */
		spif_op->cmd_mode_buswidth |=
			spif_select_buswidth(op->addr.buswidth) <<
			SPIF_ADDR_TRANS_POS;
	}

	/* dispose mode */
	if (op->mode.val) {
		spif_op->trans_phase |= SPIF_MODE_TRANS_EN;
		spif_op->cmd_mode_buswidth |=
				*(uint8_t *)op->mode.val << SPIF_MODE_OPCODE_POS;
		/* set addr buswidth */
		spif_op->cmd_mode_buswidth |=
			spif_select_buswidth(op->mode.buswidth) <<
			SPIF_MODE_TRANS_POS;
	}

	/* dispose dummy */
	if (op->dummy.cycle) {
		spif_op->trans_phase |= SPIF_DUMMY_TRANS_EN;
		spif_op->addr_dummy_data_count |=
			(op->dummy.cycle << SPIF_DUMMY_NUM_POS);
	}


	/* dispose data */
	if (data_nbytes_once) {
		if (op->data.dir == SPI_MEM_DATA_IN) {
			spif_op->trans_phase |= SPIF_RX_TRANS_EN;
			if (is_data_noalign) {
				cache_buf_align = (char *)((uint32_t)(spif_align_buffer + CONFIG_SYS_CACHELINE_SIZE - 1) & ~(CONFIG_SYS_CACHELINE_SIZE - 1));
				spif_op->data_addr = (uint32_t)cache_buf_align;
			} else
				spif_op->data_addr = (uint32_t)buf_in_once;
			/* Read Flash:1 DMA Write to dram */
			spif_op->hburst_rw_flag |= DMA_RW_PROCESS;
		} else {
			spif_op->trans_phase |= SPIF_TX_TRANS_EN;
			if (is_data_noalign) {
				cache_buf_align = (char *)((uint32_t)(spif_align_buffer + CONFIG_SYS_CACHELINE_SIZE - 1) & ~(CONFIG_SYS_CACHELINE_SIZE - 1));
				memcpy(cache_buf_align, buf_out_once, data_nbytes_once);
				spif_op->data_addr = (uint32_t)cache_buf_align;

			} else
				spif_op->data_addr = (uint32_t)buf_out_once;
			/* Write Flash:0 DMA read for dram */
			spif_op->hburst_rw_flag &= ~DMA_RW_PROCESS;
		}

		/* addr word alignment */
		spif_op->data_addr = spif_op->data_addr >> 2;

		/* set data buswidth */
		spif_op->cmd_mode_buswidth |=
			spif_select_buswidth(op->data.buswidth) << SPIF_DATA_TRANS_POS;

		handle_len = min_t(size_t, SPIF_MAX_TRANS_NUM, remain_len);

		spif_op->block_data_len |=
			handle_len << SPIF_DATA_NUM_POS;
		spif_op->addr_dummy_data_count |= handle_len == SPIF_MAX_TRANS_NUM ?
			DMA_TRANS_NUM_16BIT : handle_len << SPIF_DATA_NUM_POS;

		remain_len = data_nbytes_once - handle_len;
		while (remain_len) {
			struct spif_descriptor_op *next_op = current_op + 1;

			memcpy(next_op, current_op, sizeof(struct spif_descriptor_op));

			next_op->flash_addr += SPIF_MAX_TRANS_NUM;
			next_op->data_addr += (SPIF_MAX_TRANS_NUM >> 2);

			handle_len = min_t(size_t, SPIF_MAX_TRANS_NUM, remain_len);

			next_op->block_data_len &= ~DMA_DATA_LEN;
			next_op->addr_dummy_data_count &=
				~(DMA_TRANS_NUM_16BIT | DMA_TRANS_NUM);

			next_op->block_data_len |=
				handle_len << SPIF_DATA_NUM_POS;
			next_op->addr_dummy_data_count |=
				handle_len == SPIF_MAX_TRANS_NUM ?
				DMA_TRANS_NUM_16BIT : (handle_len << SPIF_DATA_NUM_POS);

			/* set next des addr */
			current_op->next_des_addr = (uint32_t)next_op >> 2;

			remain_len -= handle_len;

			current_op = next_op;
		}
		current_op->next_des_addr = 0;
		current_op->hburst_rw_flag |= DMA_FINISH_FLASG;
	}

	ret = spif_xfer(spif_op, data_nbytes_once);

	if (op->data.dir == SPI_MEM_DATA_IN) {
		if (cache_buf_align)
			memcpy((void *)buf_in_once,	(const void *)cache_buf_align,
						data_nbytes_once);
		buf_in_once += data_nbytes_once;
	} else {
		buf_out_once += data_nbytes_once;
	}

	flash_addr += data_nbytes_once;
	data_nbytes -= data_nbytes_once;

	cache_buf_align = NULL;
	if(data_nbytes)
		goto spif_transfer;

	return ret;
}

int spi_mem_exec_op(const struct spi_mem_op *op)
{
	if (sunxi_spif_get_version() == CONFIG_SUNXI_SPIF_V0)
		return spif_mem_exec_op_v0(op);
	else
		return spif_mem_exec_op_v1(op);
}

