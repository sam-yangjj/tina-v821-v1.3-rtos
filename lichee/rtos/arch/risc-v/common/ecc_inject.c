/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.
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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <hal_cmd.h>
#include <FreeRTOS.h>
#include <aw_version.h>
#include <irqs.h>
#include <platform.h>
#include <memory.h>

#include <serial.h>
#include <interrupt.h>
#include <compiler.h>


#define RVE907_SYS_CFG_BASE	(0x01A00000)

#define E907_CACHE_INIT_CTRL_REG	(RVE907_SYS_CFG_BASE + 0x340)
#define BIT_ICACHE_RAM_INIT_EN      (1 < 0)
#define BIT_DCACHE_RAM_INIT_EN      (1 < 1)

#define E907_CACHE_INIT_STA_REG		(RVE907_SYS_CFG_BASE + 0x350)
#define BIT_ICACHE_RAM_INIT_STA     (1 < 0)
#define BIT_DCACHE_RAM_INIT_STA     (1 < 1)

//icache
#define ICACHE_ECC_CTRL_REG	(RVE907_SYS_CFG_BASE + 0x400)
#define BIT_ICACHE_RAM0_INJECT_MODE 4
#define BIT_ICACHE_RAM1_INJECT_MODE 6
#define BIT_ICACHE_RAM2_INJECT_MODE 8
#define BIT_ICACHE_RAM3_INJECT_MODE 10
#define BIT_ICACHE_RAM0_INJECT_TYPE 12
#define BIT_ICACHE_RAM1_INJECT_TYPE 14
#define BIT_ICACHE_RAM2_INJECT_TYPE 16
#define BIT_ICACHE_RAM3_INJECT_TYPE 18


#define ICACHE_ECC_IRQ_CLR_REG	(RVE907_SYS_CFG_BASE + 0x404)
//dache
#define DCACHE_ECC_CTRL_REG	(RVE907_SYS_CFG_BASE + 0x408)
#define DCACHE_ECC_IRQ_CLR_REG	(RVE907_SYS_CFG_BASE + 0x40c)

#define CACHE_IRQ_EN	(1<<24)
#define CACHE_ECC_EN	(1<<20)

unsigned int status = 0;
int cnt = 0;
#define TEST_DATA_SIZE	(33*1024)
unsigned char dcache_test_data_A[TEST_DATA_SIZE];
unsigned char dcache_test_data_B[TEST_DATA_SIZE];
int ecc_debug = 0;

extern void udelay(uint32_t us);

void icache_ecc_test(unsigned int ecc_bit)
{
	unsigned int value = 0;
	unsigned int reg_ddr = 0;
	int offset = 0;

	//check cache init status
	reg_ddr = E907_CACHE_INIT_CTRL_REG;
	value = hal_readl(reg_ddr);
	printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__,reg_ddr, value);
	reg_ddr = E907_CACHE_INIT_STA_REG;
	value = hal_readl(reg_ddr);
	printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, reg_ddr, value);

	//icache ecc error inject
	reg_ddr = ICACHE_ECC_CTRL_REG;
	value = hal_readl(reg_ddr);
	value |= (ecc_bit << BIT_ICACHE_RAM3_INJECT_TYPE) | (ecc_bit << BIT_ICACHE_RAM2_INJECT_TYPE) | \
			(ecc_bit << BIT_ICACHE_RAM1_INJECT_TYPE) | (ecc_bit << BIT_ICACHE_RAM0_INJECT_TYPE);
	hal_writel(value, reg_ddr);

	//icache ram inject type=1 bit
	value |= (0x01 << BIT_ICACHE_RAM3_INJECT_MODE) | (0x01 << BIT_ICACHE_RAM2_INJECT_MODE) | \
		(0x01 << BIT_ICACHE_RAM1_INJECT_MODE) | (0x01 << BIT_ICACHE_RAM0_INJECT_MODE);
	hal_writel(value, reg_ddr);

	//set ecc inject enable
	reg_ddr = RVE907_SYS_CFG_BASE + 0x240;
	value = hal_readl(reg_ddr);
	value |= (0x1 << 31);
	hal_writel(value, reg_ddr);

	//irq
	reg_ddr = ICACHE_ECC_CTRL_REG;
	value = hal_readl(reg_ddr);
	printf("%s->%d, reg_ddr = 0x%x, value = 0x%x\n", __func__, __LINE__, reg_ddr, value);
	value |= (1 << 24);
	hal_writel(value, reg_ddr);
	printf("%s->%d, reg_ddr = 0x%x, value = 0x%x\n", __func__, __LINE__, reg_ddr, hal_readl(reg_ddr));

	// open ecc
	reg_ddr = ICACHE_ECC_CTRL_REG;
	value = hal_readl(reg_ddr);
	printf("%s->%d, reg_ddr = 0x%x, value = 0x%x\n", __func__, __LINE__, reg_ddr, value);
	//value |= 0xf;
	value |= 0x0f;
	hal_writel(value, reg_ddr);

	while(1) {
		memset(dcache_test_data_A, 0xaa, TEST_DATA_SIZE);
		memset(dcache_test_data_A, 0x55, TEST_DATA_SIZE);
		memset(dcache_test_data_A, 0xcc, TEST_DATA_SIZE);
		memcpy(dcache_test_data_B, dcache_test_data_A, TEST_DATA_SIZE);
		nop();
		memset(dcache_test_data_A, 0xaa, TEST_DATA_SIZE);
		memset(dcache_test_data_A, 0x55, TEST_DATA_SIZE);
		memset(dcache_test_data_A, 0xcc, TEST_DATA_SIZE);
		memcpy(dcache_test_data_B, dcache_test_data_A, TEST_DATA_SIZE);
		nop();
		memset(dcache_test_data_A, 0xaa, TEST_DATA_SIZE);
		memset(dcache_test_data_A, 0x55, TEST_DATA_SIZE);
		memset(dcache_test_data_A, 0xcc, TEST_DATA_SIZE);
		memcpy(dcache_test_data_B, dcache_test_data_A, TEST_DATA_SIZE);
		nop();
		cnt++;
		if (cnt >= 6) {
			break;
		}
		printf("---check reg&status [%d]---\n", cnt);
		printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, (RVE907_SYS_CFG_BASE + 0x40c),
									hal_readl((RVE907_SYS_CFG_BASE + 0x40c)));
			printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, (RVE907_SYS_CFG_BASE + 0x3f0),
									hal_readl((RVE907_SYS_CFG_BASE + 0x3f0)));
		for (offset = 0x410; offset <= 0x43c; offset += 4) {
			printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__,
					(RVE907_SYS_CFG_BASE + offset),hal_readl(RVE907_SYS_CFG_BASE + offset));
		}
		udelay(100);

		hal_writel(0x1, (RVE907_SYS_CFG_BASE + 0x40c));
			printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, (RVE907_SYS_CFG_BASE + 0x40c),
									hal_readl((RVE907_SYS_CFG_BASE + 0x40c)));
		hal_writel(0, (RVE907_SYS_CFG_BASE + 0x40c));
		printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, (RVE907_SYS_CFG_BASE + 0x40c),
								hal_readl((RVE907_SYS_CFG_BASE + 0x40c)));
	}

	//set ecc inject disable
	reg_ddr = RVE907_SYS_CFG_BASE + 0x240;
	value = hal_readl(reg_ddr);
	value &= ~(0x1 << 31);
	hal_writel(value, reg_ddr);
}

static int icache_ecc_1bit_test(int argc, char **argv)
{
	icache_ecc_test(1);
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(icache_ecc_1bit_test, icache_ecc_1bit_test, Command);

static int icache_ecc_2bit_test(int argc, char **argv)
{
	icache_ecc_test(2);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(icache_ecc_2bit_test, icache_ecc_2bit_test, Command);


void data_cache_ecc_test(unsigned int ecc_bit)
{
	unsigned int value = 0;
	unsigned int reg_ddr = 0;
	int offset = 0;

	//check cache init status
	reg_ddr = E907_CACHE_INIT_CTRL_REG;
	value = hal_readl(reg_ddr);
	printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__,reg_ddr, value);
	reg_ddr = E907_CACHE_INIT_STA_REG;
	value = hal_readl(reg_ddr);
	printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, reg_ddr, value);

	//dcache ecc enable
	reg_ddr = DCACHE_ECC_CTRL_REG;
	value = hal_readl(reg_ddr);

	//set ecc_bit bit err
	value |= (ecc_bit << 18) | (ecc_bit << 16) | (ecc_bit << 14) | (ecc_bit << 12);

	hal_writel(value, reg_ddr);

	value |= (0x01 << 10) | (0x01 << 8) | (0x01 << 6) | (0x01 << 4);
	hal_writel(value, reg_ddr);

	//set ecc inject enable
	reg_ddr = RVE907_SYS_CFG_BASE + 0x240;
	value = hal_readl(reg_ddr);
	printf("%s->%d, reg_ddr = 0x%x, value = 0x%x\n", __func__, __LINE__, reg_ddr, value);
	value |= (0x1 << 31);
	hal_writel(value, reg_ddr);
	printf("%s->%d, reg_ddr = 0x%x, value = 0x%x\n", __func__, __LINE__, reg_ddr, hal_readl(reg_ddr));

	//irq
	reg_ddr = DCACHE_ECC_CTRL_REG;
	value = hal_readl(reg_ddr);
	value |= (1 << 24);
	hal_writel(value, reg_ddr);

	// open ecc
	reg_ddr = DCACHE_ECC_CTRL_REG;
	value = hal_readl(reg_ddr);
	value |= 0x0f;
	hal_writel(value, reg_ddr);
	printf("%s->%d, reg_ddr = 0x%x, value = 0x%x\n", __func__, __LINE__, reg_ddr, hal_readl(reg_ddr));

	while(1) {
		memset(dcache_test_data_A, 0xaa, TEST_DATA_SIZE);
		memset(dcache_test_data_A, 0x55, TEST_DATA_SIZE);
		memset(dcache_test_data_A, 0xcc, TEST_DATA_SIZE);
		memcpy(dcache_test_data_B, dcache_test_data_A, TEST_DATA_SIZE);
		cnt++;
		if (cnt >= 6) {
			break;
		}
		printf("---check reg&status [%d]---\n", cnt);
		printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, (RVE907_SYS_CFG_BASE + 0x40c),
									hal_readl((RVE907_SYS_CFG_BASE + 0x40c)));
			printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, (RVE907_SYS_CFG_BASE + 0x3f0),
									hal_readl((RVE907_SYS_CFG_BASE + 0x3f0)));
		for (offset = 0x440; offset <= 0x49c; offset += 4) {
			printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__,
					(RVE907_SYS_CFG_BASE + offset),hal_readl(RVE907_SYS_CFG_BASE + offset));
		}
		udelay(100);

		hal_writel(0x1, (RVE907_SYS_CFG_BASE + 0x40c));
			printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, (RVE907_SYS_CFG_BASE + 0x40c),
									hal_readl((RVE907_SYS_CFG_BASE + 0x40c)));
		hal_writel(0, (RVE907_SYS_CFG_BASE + 0x40c));
		printf("%s->%d, reg = 0x%x, value = 0x%x\n", __func__, __LINE__, (RVE907_SYS_CFG_BASE + 0x40c),
								hal_readl((RVE907_SYS_CFG_BASE + 0x40c)));
	}

	//set ecc inject disable
	reg_ddr = RVE907_SYS_CFG_BASE + 0x240;
	value = hal_readl(reg_ddr);
	value &= ~(0x1 << 31);
	hal_writel(value, reg_ddr);

}

static int dcache_ecc_1bit_test(int argc, char **argv)
{
	data_cache_ecc_test(1);
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(dcache_ecc_1bit_test, dcache_ecc_1bit_test, Command);

static int dcache_ecc_2bit_test(int argc, char **argv)
{
	data_cache_ecc_test(2);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dcache_ecc_2bit_test, dcache_ecc_2bit_test, Command);
