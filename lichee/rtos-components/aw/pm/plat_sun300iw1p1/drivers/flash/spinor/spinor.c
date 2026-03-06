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

#include "spif.h"
#include "spinor.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <flash_base.h>

/* porting defination */
//#define printf(fmt,...)                 flash_base_ptinrf(fmt, ##__VA_ARGS__)
#define printf(fmt,...)

#define SPINOR_DEBUG 0

#if SPINOR_DEBUG
#define spinor_debug(fmt, arg...)    printf("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
#define spinor_debug(fmt, arg...)
#endif

#define SYSTEM_PAGE_SIZE 512
#define READ_LEN (128*512)
#define JEDEC_MFR(id)		(id&0xff)

/*
 * Clock Frequency for all commands (except Read), depend on flash, maybe 104M or 133M
 * Clock Frequency for READ(0x03) instructions is special, depend on flash, maybe only 50M or 80M
 * Default use FASTREAD(0x0b) which can work with sunxi spi clk(max 100M)
 */
static uint8_t read_cmd = CMD_READ_ARRAY_SLOW;
static enum spi_nor_protocol read_proto = SNOR_PROTO_1_1_1;
static enum spi_nor_protocol write_proto = SNOR_PROTO_1_1_1;
static uint8_t read_dummy;
static uint8_t spinor_4bytes_addr_mode;
static uint32_t boot_param_start;
static ssize_t (*spi_nor_wr_func)(off_t from, size_t len, u_char *buf);
static int spi_flash_write_en(void);

#define BOOT_PARAM_SIZE (2048)
#define BOOT_PARAM_MAGIC		"bootpara"
#define SPINOR_BOOT_PARAM_MAGIC	"NORPARAM"

struct sunxi_boot_parameter_header {
	uint8_t magic[8]; //bootpara
	uint32_t version; // describe the region version
	uint32_t check_sum;
	uint32_t length;
	uint32_t boot0_checksum;
	uint32_t transfer_flag;
	uint8_t reserved[4];
};

typedef struct sunxi_boot_param_region{
	struct sunxi_boot_parameter_header header;
	char sdmmc_info[256 -32];
	char nand_info[256];
	char spiflash_info[256];
	char ufs[256];
	char ddr_info[512];
	uint8_t reserved[BOOT_PARAM_SIZE - 512*3];// = 2048 - 32 - sdmmc_size - nand_size - spi_size - ddr_size
} typedef_sunxi_boot_param;

static uint8_t param_buff[512];

spinor_info_t *spinor_info = NULL;

void dump_spinor_info(spinor_info_t *spinor_info)
{
	spinor_debug("\n"
		"----------------------\n"
		"readcmd:%x\n"
		"read_mode:%d\n"
		"write_mode:%d\n"
		"flash_size:%dM\n"
		"addr4b_opcodes:%d\n"
		"erase_size:%d\n"
		"frequency:%d\n"
		"sample_mode:%x\n"
		"sample_delay:%x\n"
		"read_proto:%x\n"
		"write_proto:%x\n"
		"read_dummy:%d\n"
		"----------------------\n",
		spinor_info->readcmd, spinor_info->read_mode,
		spinor_info->write_mode, spinor_info->flash_size,
		spinor_info->addr4b_opcodes, spinor_info->erase_size,
		spinor_info->frequency, spinor_info->sample_mode,
		spinor_info->sample_delay, spinor_info->read_proto,
		spinor_info->write_proto, spinor_info->read_dummy);
}

static int spi_nor_read_write_reg(struct spi_mem_op *op, void *buf)
{
	if (op->data.dir == SPI_MEM_DATA_IN)
		op->data.buf.in = buf;
	else
		op->data.buf.out = buf;
	return spi_mem_exec_op(op);
}

static int spi_nor_read_reg(uint8_t code, uint8_t *val, int len)
{
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(code, 1),
					  SPI_MEM_OP_NO_ADDR,
					  SPI_MEM_OP_NO_MODE,
					  SPI_MEM_OP_NO_DUMMY,
					  SPI_MEM_OP_DATA_IN(len, NULL, 1));
	int ret;

	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(read_proto);
	op.dummy.buswidth = 1;
	op.data.buswidth = op.cmd.buswidth;

	if (op.cmd.buswidth == 8)
		op.dummy.cycle = 8;

	ret = spi_nor_read_write_reg(&op, val);
	if (ret < 0)
		SPIF_ERR("error %d reading %x\n", ret, code);

	return ret;
}

static int spi_nor_write_reg(uint8_t opcode, uint8_t *buf, int len)
{
	struct spi_mem_op op =
			SPI_MEM_OP(SPI_MEM_OP_CMD(opcode, 1),
				   SPI_MEM_OP_NO_ADDR,
				   SPI_MEM_OP_MODE(buf, 1),
				   SPI_MEM_OP_NO_DUMMY,
				   SPI_MEM_OP_NO_DATA);

	/* get transfer protocols. */
	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(write_proto);
	op.mode.buswidth = spi_nor_get_protocol_data_nbits(read_proto);

	return spi_mem_exec_op(&op);
}

static int spi_nor_write_reg_2byte(uint8_t opcode, uint8_t *buf)
{
	off_t cmd_sr_cr = (opcode << 16) | (buf[0] << 8) | buf[1];
	struct spi_mem_op op =
			SPI_MEM_OP(SPI_MEM_OP_NO_CMD,
				   SPI_MEM_OP_ADDR(3, cmd_sr_cr, 1),
				   SPI_MEM_OP_NO_MODE,
				   SPI_MEM_OP_NO_DUMMY,
				   SPI_MEM_OP_NO_DATA);

	/* The command line width is the same as the address line width. */
	op.addr.buswidth = spi_nor_get_protocol_inst_nbits(write_proto);

	return spi_mem_exec_op(&op);
}

static ssize_t spi_nor_read_data(off_t from, size_t len, u_char *buf)
{
	uint8_t addr_width = spinor_4bytes_addr_mode ? 4 : 3;

	struct spi_mem_op op =
			SPI_MEM_OP(SPI_MEM_OP_CMD(read_cmd, 1),
				   SPI_MEM_OP_ADDR(addr_width, from, 1),
				   SPI_MEM_OP_NO_MODE,
				   SPI_MEM_OP_DUMMY(read_dummy, 1),
				   SPI_MEM_OP_DATA_IN(len, buf, 1));
	size_t remaining = len;
	int ret;

	/* get transfer protocols. */
	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(read_proto);
	op.addr.buswidth = spi_nor_get_protocol_addr_nbits(read_proto);
	op.dummy.buswidth = op.addr.buswidth;
	op.data.buswidth = spi_nor_get_protocol_data_nbits(read_proto);

	while (remaining) {
		ret = spi_mem_exec_op(&op);
		if (ret)
			return ret;

		op.addr.val += op.data.nbytes;
		remaining -= op.data.nbytes;
		op.data.buf.in += op.data.nbytes;
	}

	return len;
}

static ssize_t spi_nor_io_read_data(off_t from, size_t len, u_char *buf)
{
	uint8_t addr_width = spinor_4bytes_addr_mode ? 4 : 3;
	uint8_t mode = 0;

	struct spi_mem_op op =
			SPI_MEM_OP(SPI_MEM_OP_CMD(read_cmd, 1),
				   SPI_MEM_OP_ADDR(addr_width, from, 1),
				   SPI_MEM_OP_MODE(&mode, 1),
				   SPI_MEM_OP_DUMMY(read_dummy, 1),
				   SPI_MEM_OP_DATA_IN(len, buf, 1));
	size_t remaining = len;
	int ret;

	/* get transfer protocols. */
	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(read_proto);
	op.addr.buswidth = spi_nor_get_protocol_addr_nbits(read_proto);
	op.mode.buswidth = op.addr.buswidth;
	op.dummy.buswidth = op.addr.buswidth;
	op.data.buswidth = spi_nor_get_protocol_data_nbits(read_proto);

	while (remaining) {
		ret = spi_mem_exec_op(&op);
		if (ret)
			return ret;

		op.addr.val += op.data.nbytes;
		remaining -= op.data.nbytes;
		op.data.buf.in += op.data.nbytes;
	}

	return len;
}

static int is_valid_read_cmd(uint8_t cmd)
{
	if (SPINOR_OP_READ_1_1_2 == cmd
		|| SPINOR_OP_READ_1_1_4 == cmd
		|| SPINOR_OP_READ4_1_1_2 == cmd
		|| SPINOR_OP_READ4_1_1_4 == cmd
		|| CMD_READ_ARRAY_FAST == cmd
		|| CMD_READ_ARRAY_FAST_4B == cmd
		|| CMD_READ_ARRAY_SLOW == cmd
		|| CMD_READ_ARRAY_SLOW_4B == cmd
		|| SPINOR_OP_READ_1_4_4 == cmd
		|| SPINOR_OP_READ_1_4_4_DTR == cmd
		|| SPINOR_OP_READ4_1_4_4 == cmd
		|| SPINOR_OP_READ4_1_4_4_DTR == cmd)
		return 1;
	return 0;
}

static int is_quad_read_cmd(uint8_t cmd)
{
	if (SPINOR_OP_READ_1_1_4 == cmd
		|| SPINOR_OP_READ4_1_1_4 == cmd
		|| SPINOR_OP_READ_1_4_4 == cmd
		|| SPINOR_OP_READ4_1_4_4 == cmd
		|| SPINOR_OP_READ_1_4_4_DTR == cmd
		|| SPINOR_OP_READ4_1_4_4_DTR == cmd)
		return 1;
	return 0;
}

static int read_sr(uint8_t *status)
{
	return spi_nor_read_reg(CMD_READ_STATUS, status, 1);
}

static int read_sr1(uint8_t *status)
{
	return spi_nor_read_reg(CMD_READ_STATUS1, status, 1);
}

static int spi_flash_ready(void)
{
	uint8_t sr;
	int ret;

	ret = read_sr(&sr);
	if (ret < 0)
		return ret;

	return !(sr & STATUS_WIP);
}

static int spi_flash_wait_till_ready(void)
{
	uint timeout = 0x40;
	int ret;

	while (timeout --) {
		ret = spi_flash_ready();
		if (ret < 0)
			return ret;
		if (ret)
			return 0;
	}

	SPIF_WARN("SF: Timeout!\n");
	return -1;
}

static int spi_flash_write_en(void)
{
	return spi_nor_write_reg(CMD_WRITE_ENABLE, NULL, 0);
}

static int write_sr(uint8_t status)
{
	spi_flash_write_en();

	if (spi_nor_write_reg(CMD_WRITE_STATUS, &status, 1))
		return -1;

	return spi_flash_wait_till_ready();

}

static int write_sr1(uint8_t status)
{
	spi_flash_write_en();

	if (spi_nor_write_reg(CMD_WRITE_STATUS1, &status, 1))
		return -1;

	return spi_flash_wait_till_ready();
}

static int read_cr(uint8_t *status)
{
	return spi_nor_read_reg(CMD_READ_CONFIG, status, 1);
}

static int write_cr(uint8_t wc)
{
	uint8_t dout[4];
	uint8_t status;
	int ret;

	ret = read_sr(&status);
	if (ret < 0)
		return ret;

	spi_flash_write_en();

	dout[0] = status;
	dout[1] = wc;

	if (spi_nor_write_reg_2byte(CMD_WRITE_STATUS, dout))
		return -1;

	return spi_flash_wait_till_ready();

}

/*Order code“05H”/“35H”/“15H”,The corresponding status registerS7~S0 / S15~S8 / S16~S23.*/
static int GigaDevice_quad_enable(void)
{
	uint8_t qeb_status;
	int ret;

	ret = read_sr1(&qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_GIGA) {
		SPIF_WARN("already enable\n");
		return 0;
	}

	ret = write_sr1(qeb_status | STATUS_QEB_GIGA);
	if (ret < 0)
		return ret;

	/* read SR and check it */
	ret = read_sr1(&qeb_status);
	if (!(ret >= 0 && (qeb_status & STATUS_QEB_GIGA))) {
		SPIF_ERR("SF: Macronix SR Quad bit not clear\n");
		return -1;
	}
	return ret;
}

static int macronix_quad_enable(void)
{
	uint8_t qeb_status;
	int ret;

	ret = read_sr(&qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_MXIC) {
		SPIF_INFO("already enable\n");
		return 0;
	}

	ret = write_sr(qeb_status | STATUS_QEB_MXIC);
	if (ret < 0)
		return ret;

	/* read SR and check it */
	ret = read_sr(&qeb_status);
	if (!(ret >= 0 && (qeb_status & STATUS_QEB_MXIC))) {
		SPIF_ERR("SF: Macronix SR Quad bit not clear\n");
		return -1;
	}
	return ret;
}

static int xmcwh_quad_enable(void)
{
	uint8_t qeb_status;
	int ret;

	ret = read_sr1(&qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_XMC) {
		SPIF_INFO("already enable\n");
		return 0;
	}

	ret = write_sr1(qeb_status | STATUS_QEB_XMC);
	if (ret < 0)
		return ret;

	/* read SR and check it */
	ret = read_sr1(&qeb_status);
	if (!(ret >= 0 && (qeb_status & STATUS_QEB_XMC))) {
		SPIF_ERR("SF: Xmcwh SR Quad bit not clear\n");
		return -1;
	}
	return ret;
}

static int spansion_quad_enable(void)
{
	uint8_t qeb_status;
	int ret;

	ret = read_cr(&qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_WINSPAN)
		return 0;

	ret = write_cr(qeb_status | STATUS_QEB_WINSPAN);
	if (ret < 0)
		return ret;

	/* read CR and check it */
	ret = read_cr(&qeb_status);
	if (!(ret >= 0 && (qeb_status & STATUS_QEB_WINSPAN))) {
		SPIF_ERR("SF: Spansion CR Quad bit not clear\n");
		return -1;
	}

	return ret;
}

static int set_quad_mode(uint8_t id_0, uint8_t id_1)
{
	switch (JEDEC_MFR(id_0)) {

	case SPI_FLASH_CFI_MFR_MACRONIX:
		if (JEDEC_MFR(id_1) >> 4 == 'b') {
			SPIF_INFO("SF: QEB is volatile for %02xb flash\n", JEDEC_MFR(id_0));
			return 0;
		}
		return macronix_quad_enable();
	case SPI_FLASH_CFI_MFR_XMC:
		return xmcwh_quad_enable();
	case SPI_FLASH_CFI_MFR_GIGADEVICE:
	case SPI_FLASH_CFI_MFR_ADESTO:
	case SPI_FLASH_CFI_MFR_PUYA:
	case SPI_FLASH_CFI_MFR_ZETTA:
	case SPI_FLASH_CFI_MFR_BOYA:
	case SPI_FLASH_CFI_MFR_ESMT:
		return GigaDevice_quad_enable();
	case SPI_FLASH_CFI_MFR_SPANSION:
	case SPI_FLASH_CFI_MFR_WINBOND:
	case SPI_FLASH_CFI_MFR_XTX:
	case SPI_FLASH_CFI_MFR_FM:
		return spansion_quad_enable();
	default:
		SPIF_ERR("SF: Need set QEB func for %02x flash\n",
		       JEDEC_MFR(id_0));
		return -1;
	}
}


static int spinor_read_id(uint8_t *id)
{
	return spi_nor_read_reg(CMD_READ_ID, id, 3);
}

static int spinor_enter_4bytes_addr(int enable)
{
	uint8_t command = 0;
	uint8_t buf = 0;

	if (enable)
		command = 0xB7;
	else
		command = 0xE9;

	spi_nor_write_reg(command, NULL, 0);

	read_cr(&buf);
	if ((buf >> 5)  & 0x1)
		SPIF_INFO("4byte mode ok\n");
	else {
		SPIF_INFO("4byte mode error\n");
		return 0;
	}
	return 1;
}

void spi_nor_set_4byte_opcodes(void)
{
	if (!spinor_info->addr4b_opcodes)
		spinor_enter_4bytes_addr(1);
	else {
		if (read_cmd == CMD_READ_ARRAY_FAST)
			read_cmd = CMD_READ_ARRAY_FAST_4B;
		else if (read_cmd == SPINOR_OP_READ_1_1_2)
			read_cmd = SPINOR_OP_READ4_1_1_2;
		else if (read_cmd == SPINOR_OP_READ_1_1_4)
			read_cmd = SPINOR_OP_READ4_1_1_4;
		else if (read_cmd == SPINOR_OP_READ_1_4_4)
			read_cmd = SPINOR_OP_READ4_1_4_4;
		else if (read_cmd == SPINOR_OP_READ_1_4_4_DTR)
			read_cmd = SPINOR_OP_READ_1_4_4_DTR;
	}
	spinor_4bytes_addr_mode = 1;/* set 4byte opcodes*/
	return;
}

static int spinor_set_readcmd(void)
{
	if (is_valid_read_cmd(spinor_info->readcmd)) {
		read_cmd = spinor_info->readcmd;
		read_proto = spinor_info->read_proto;
		write_proto = spinor_info->write_proto;
		read_dummy = spinor_info->read_dummy;
	} else
		return -1;

	if (spinor_info->flash_size > 16)
		spi_nor_set_4byte_opcodes();

	return 0;
}

int get_boot_param(void)
{
	struct sunxi_boot_parameter_header *header;
	spinor_info_t *info = NULL;
	int ret = 0;

	/* read boot param header */
	ret = spinor_read(boot_param_start, 1, (void *)(&param_buff));
	if (ret)
		goto err;

	header = (struct sunxi_boot_parameter_header *)param_buff;
	if (strncmp((const char *)header->magic, (const char *)BOOT_PARAM_MAGIC, sizeof(header->magic))) {
		ret = -1;
		goto err;
	}

	/* read spinor info header */
	ret = spinor_read(boot_param_start + 1, 1, (void *)(&param_buff));
	if (ret)
		goto err;

	info = (spinor_info_t *)param_buff;
	if (strncmp((const char *)info->magic, (const char *)SPINOR_BOOT_PARAM_MAGIC, sizeof(info->magic))) {
		ret = -1;
		goto err;
	}
	spinor_info = info;
	ret = 0;
err:
	return ret;
}

void set_delay_para(void)
{
	if (spinor_info->frequency &&
		spinor_info->sample_delay != SAMP_MODE_DL_DEFAULT &&
		(spinor_info->sample_delay || spinor_info->sample_mode)) {
		SPIF_INFO("spi sample_mode:%x sample_delay:%x\n",
			spinor_info->sample_mode, spinor_info->sample_delay);
		spif_samp_mode((void *)SUNXI_SPIF_BASE, 1);
		spif_samp_dl_sw_rx_status((void *)SUNXI_SPIF_BASE, 1);
		spif_set_sample_mode((void *)SUNXI_SPIF_BASE,
				spinor_info->sample_mode);
		spif_set_sample_delay((void *)SUNXI_SPIF_BASE,
				spinor_info->sample_delay);
	}
	return ;
}

int spinor_init(int stage)
{
	uint8_t id[4] = {0};
	int ret = 0;

	if (spif_init())
		return -1;

	ret = spinor_read_id(id);
	if (ret || id[0] == 0x0) {
		SPIF_ERR("read id error, spinor id is: %02x %02x %02x\n",
				id[0], id[1], id[2]);
		return -1;
	}
	ret = get_boot_param();

	if (!ret && spinor_info && !spinor_set_readcmd()) {
		struct sunxi_spif_slave *sspif = get_sspif();
		sspif->max_hz = spinor_info->frequency;
		if (spi_nor_protocol_is_dtr(read_proto))
			sspif->rx_dtr_en = 1;
		if (spi_nor_protocol_is_dtr(write_proto))
			sspif->tx_dtr_en = 1;
		if (set_spif_clk(spinor_info->frequency)) {
			SPIF_ERR("spi set clk error\n");
			return -1;
		} else {
			set_delay_para();
		}
	}
	dump_spinor_info(spinor_info);

	SPIF_INFO("spinor id is: %02x %02x %02x, read cmd: %02x\n",
		id[0], id[1], id[2], read_cmd);

	/*
	 * Flash powers up read-only, so clear BP# bits.
	 *
	 * Note on some flash (like Macronix), QE (quad enable) bit is in the
	 * same status register as BP# bits, and we need preserve its original
	 * value during a reboot cycle as this is required by some platforms
	 * (like Intel ICH SPI controller working under descriptor mode).
	 */
	if (JEDEC_MFR(id[0]) == SPI_FLASH_CFI_MFR_ATMEL ||
	   (JEDEC_MFR(id[0]) == SPI_FLASH_CFI_MFR_SST) ||
	   (JEDEC_MFR(id[0]) == SPI_FLASH_CFI_MFR_MACRONIX)) {
		uint8_t sr = 0;

		read_sr(&sr);
		if (JEDEC_MFR(id[0]) == SPI_FLASH_CFI_MFR_MACRONIX) {
			sr &= STATUS_QEB_MXIC;
		}
		write_sr(sr);
	}

	if (is_quad_read_cmd(read_cmd))
		set_quad_mode(id[0], id[1]);

	return 0;
}

int spinor_exit(int stage)
{
	struct sunxi_spif_slave *sspif = get_sspif();

	spif_exit();

	spinor_info = NULL;
	spi_nor_wr_func = NULL;
	read_cmd = CMD_READ_ARRAY_SLOW;
	read_proto = SNOR_PROTO_1_1_1;
	write_proto = SNOR_PROTO_1_1_1;
	read_dummy = 0;
	spinor_4bytes_addr_mode = 0;
	memset(param_buff, 0x00, sizeof(param_buff));
	memset(sspif, 0x00, sizeof(*sspif));

	return 0;
}

int spinor_read(uint32_t start, uint32_t sector_cnt, void *buffer)
{
	uint32_t addr;
	uint8_t *buf = (uint8_t *)buffer;
	uint32_t len = sector_cnt * 512;
	uint32_t from = start * 512;
	uint32_t rxnum_offset;
	uint8_t i = 0;
	uint32_t flash_size = spinor_info->flash_size * 1024 * 1024;
	int ret;
	size_t rxnum[3];

	if (from + len > flash_size)
		len = flash_size - from;

	if ((uint32_t)buf % CONFIG_SYS_CACHELINE_SIZE) {
		/*If the addresses are not aligned, the transmission is divided into three parts:
		 * 1. In the middle, the address and length are aligned.
		 * 2. The head and tail are small sections and do not need to be aligned,
		 * The spif transmission interface will align it.*/
		rxnum[0] = ROUND_UP((uint32_t)buf, CONFIG_SYS_CACHELINE_SIZE) - (uint32_t)buf;
		rxnum[1] = len - rxnum[0];
		if (CHECK_IS_NOALIGN(rxnum[1])) {
			rxnum[1] = ROUND_DOWN(rxnum[1], CONFIG_SYS_CACHELINE_SIZE);
			rxnum[2] = len - rxnum[0] - rxnum[1];
			rxnum_offset = ROUND_UP(rxnum[2], CONFIG_SYS_CACHELINE_SIZE) - rxnum[2];
			rxnum[2] = ROUND_UP(rxnum[2], CONFIG_SYS_CACHELINE_SIZE);
		}
	} else {
		rxnum[0] = len;
	}

	if (spi_nor_get_protocol_addr_nbits(read_proto) != 1)
		spi_nor_wr_func = spi_nor_io_read_data;
	else
		spi_nor_wr_func = spi_nor_read_data;

	while (len) {
		/*read 128*512 bytes per time*/
		addr = from;
		/*In order not to exceed the boundary,
		 * when the length of the tail is aligned,
		 * the excess part should be retracted a little.*/
		if (i == 2) {
			addr -= rxnum_offset;
			buf -= rxnum_offset;
		}

		ret = spi_nor_wr_func(addr, rxnum[i], buf);
		if (ret == 0) {
			/* We shouldn't see 0-length reads */
			ret = -1;
			goto read_err;
		}
		if (ret < 0)
			goto read_err;

		if (i == 2) {
			ret -= rxnum_offset;
		}

		buf += ret;
		from += ret;
		len -= ret;
		i++;
	}
	ret = 0;

read_err:
	return ret;
}

int spinor_set_boot_param_start(uint32_t start)
{
	boot_param_start = start - 8;
	return 0;
}

