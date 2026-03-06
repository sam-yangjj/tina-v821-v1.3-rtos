/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
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
#include "capacity_read.h"

static twi_status_t twi_byte_init(void)
{
	twi_status_t ret;
	if (!(HAL_REG_32BIT(CCU_APP_BUS_CLK_REG) & 1<<TWI0_CLK_BIT))
		HAL_REG_32BIT(CCU_APP_BUS_CLK_REG) |= 1<<TWI0_CLK_BIT;
	ret = hal_twi_init(PORT_TWI);
	return ret;
}

static twi_status_t twi_byte_deinit(void)
{
	twi_status_t ret;
	ret = hal_twi_uninit(PORT_TWI);
	if (HAL_REG_32BIT(CCU_APP_BUS_CLK_REG) & 1<<TWI0_CLK_BIT)
		HAL_REG_32BIT(CCU_APP_BUS_CLK_REG) &= ~(1<<TWI0_CLK_BIT);
	return ret;
}

static twi_status_t twi_byte_read(uint8_t *reg_val)
{
	twi_status_t ret;
	ret = hal_twi_read(PORT_TWI, I2C_SLAVE, SLAVE_ADDR, SOC_REG_ADDR, reg_val, 1);
	return ret;
}

static twi_status_t sleep_bit_set(uint8_t st)
{
	twi_msg_t msg;
	twi_status_t ret;
	uint8_t buf[2];
	uint8_t reg_val;

	ret = hal_twi_read(PORT_TWI, I2C_SLAVE, SLAVE_ADDR, MODE_REG_ADDR, &reg_val, 1);
	if (ret != TWI_STATUS_OK)
		return ret;
	if (st) {
		if (reg_val & 1<<BIT_SLEEP)
			return TWI_STATUS_OK;
		else
			reg_val |= (1<<BIT_SLEEP);
	} else {
		if (!(reg_val & 1<<BIT_SLEEP))
			return TWI_STATUS_OK;
		else
			reg_val &= ~(1<<BIT_SLEEP);
	}

	buf[0] = MODE_REG_ADDR;
	buf[1] = reg_val;
	msg.flags = 0;
	msg.addr = SLAVE_ADDR;
	msg.len = 2;
	msg.buf = buf;
	ret = hal_twi_write(PORT_TWI, &msg, 1);
	return ret;
}

int capacity_read_init(void) {
	twi_status_t ret;

	ret = twi_byte_init();
	if (ret != TWI_STATUS_OK) {
		printf("[%s:%d]twi_byte_init failed, ret=%d.\n", __FILE__, __LINE__, ret);
		return TWI_STATUS_ERROR;
	}

	ret = sleep_bit_set(0);
	if (ret != TWI_STATUS_OK) {
		printf("[%s:%d]sleep_bit_set failed, ret=%d.\n", __FILE__, __LINE__, ret);
		return TWI_STATUS_ERROR;
	}

	ret = twi_byte_deinit();
	if (ret != TWI_STATUS_OK) {
		printf("[%s:%d]twi_byte_deinit failed, ret=%d.\n", __FILE__, __LINE__, ret);
		return TWI_STATUS_ERROR;
	}

	return TWI_STATUS_OK;
}

int capacity_read(uint8_t *cap) {
	twi_status_t ret;

	ret = twi_byte_init();
	if (ret != TWI_STATUS_OK) {
		printf("[%s:%d]twi_byte_init failed, ret=%d.\n", __FILE__, __LINE__, ret);
		return TWI_STATUS_ERROR;
	}

	ret = twi_byte_read(cap);
	if (ret != TWI_STATUS_OK) {
		printf("[%s:%d]twi_byte_read failed, ret=%d.\n", __FILE__, __LINE__, ret);
		return TWI_STATUS_ERROR;
	}

	ret = twi_byte_deinit();
	if (ret != TWI_STATUS_OK) {
		printf("[%s:%d]twi_byte_deinit failed, ret=%d.\n", __FILE__, __LINE__, ret);
		return TWI_STATUS_ERROR;
	}

	return TWI_STATUS_OK;
}