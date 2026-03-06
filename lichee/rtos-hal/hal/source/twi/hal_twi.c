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
#include <hal_log.h>
#include <stdlib.h>
#include <hal_gpio.h>
#include <sunxi_hal_twi.h>
#include <hal_dma.h>
#include <hal_cache.h>
#include <sunxi_hal_regulator.h>
#include <interrupt.h>
#include <hal_time.h>
#include "platform_twi.h"

#ifdef CONFIG_DRIVER_SYSCONFIG
#include <hal_cfg.h>
#include <script.h>
#endif

#ifdef CONFIG_COMPONENTS_PM
#include <pm_debug.h>
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define inner_attr	static __attribute__((unused))

#define FIX_DRV_EN_LOSS /* fix DRV_EN bit becomes 0 issues */
#define FIX_ERR_COM_INT /* fix lost or unexpected PD_COM interrupts issues */

#if defined(CONFIG_SUNXI_TWI_ERR_HOOK)
extern void twi_err_report(const char *func, uint32_t line, uint32_t port, uint32_t code);
#define TWI_ERR_REPORT(port, code)	twi_err_report(__func__, __LINE__, port, code)
#else
#define TWI_ERR_REPORT(port, code)
#endif

static const uint32_t hal_twi_address[] =
{
    SUNXI_TWI0_PBASE,
    SUNXI_TWI1_PBASE,
    SUNXI_TWI2_PBASE,
#if !(defined(CONFIG_ARCH_SUN300IW1))
    SUNXI_TWI3_PBASE,
#endif
#if (defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
    SUNXI_TWI4_PBASE,
#endif
#if (defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
    SUNXI_TWI5_PBASE,
#endif
#ifdef CONFIG_ARCH_SUN55IW6
    SUNXI_TWI6_PBASE,
#endif
#if !(defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN300IW1))
    SUNXI_S_TWI0_PBASE,
#endif
#if (defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
    SUNXI_S_TWI1_PBASE,
#endif
#ifdef CONFIG_ARCH_SUN55IW3
    SUNXI_S_TWI2_PBASE,
#endif
};

static const uint32_t hal_twi_irq_num[] =
{
    SUNXI_IRQ_TWI0,
    SUNXI_IRQ_TWI1,
    SUNXI_IRQ_TWI2,
#if !(defined(CONFIG_ARCH_SUN300IW1))
    SUNXI_IRQ_TWI3,
#endif
#if (defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
    SUNXI_IRQ_TWI4,
#endif
#if (defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
    SUNXI_IRQ_TWI5,
#endif
#ifdef CONFIG_ARCH_SUN55IW6
    SUNXI_IRQ_TWI6,
#endif
#if !(defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN300IW1))
    SUNXI_IRQ_S_TWI0,
#endif
#if (defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
    SUNXI_IRQ_S_TWI1,
#endif
#ifdef CONFIG_ARCH_SUN55IW3
    SUNXI_IRQ_S_TWI2,
#endif
};

#if !(defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6) || defined(CONFIG_ARCH_SUN300IW1))
static const hal_clk_id_t hal_twi_pclk[] =
{
    HAL_CLK_BUS_APB2,
    HAL_CLK_BUS_APB2,
    HAL_CLK_BUS_APB2,
    HAL_CLK_BUS_APB2,
    HAL_CLK_BUS_APB2,
};

static const hal_clk_id_t hal_twi_mclk[] =
{
    HAL_CLK_PERIPH_TWI0,
    HAL_CLK_PERIPH_TWI1,
    HAL_CLK_PERIPH_TWI2,
    HAL_CLK_PERIPH_TWI3,
    HAL_CLK_PERIPH_TWI4,
};
#endif

static twi_mode_t twi_mode_table[TWI_MASTER_MAX] = {
#ifdef TWI0_USED_DRV
	[TWI_MASTER_0] = TWI_DRV_XFER,
#endif
#ifdef TWI1_USED_DRV
	[TWI_MASTER_1] = TWI_DRV_XFER,
#endif
#ifdef TWI2_USED_DRV
	[TWI_MASTER_2] = TWI_DRV_XFER,
#endif
#ifdef TWI3_USED_DRV
	[TWI_MASTER_3] = TWI_DRV_XFER,
#endif
#ifdef TWI4_USED_DRV
	[TWI_MASTER_4] = TWI_DRV_XFER,
#endif
#ifdef TWI5_USED_DRV
	[TWI_MASTER_5] = TWI_DRV_XFER,
#endif
#ifdef TWI6_USED_DRV
	[TWI_MASTER_6] = TWI_DRV_XFER,
#endif
#ifdef R_TWI0_USED_DRV
	[S_TWI_MASTER_0] = TWI_DRV_XFER,
#endif
#ifdef R_TWI1_USED_DRV
	[S_TWI_MASTER_1] = TWI_DRV_XFER,
#endif
};

static twi_frequency_t twi_freq_table[TWI_MASTER_MAX] = {
	[TWI_MASTER_0] = TWI_FREQUENCY_400K,
	[TWI_MASTER_1] = TWI_FREQUENCY_400K,
	[TWI_MASTER_2] = TWI_FREQUENCY_400K,
	[TWI_MASTER_3] = TWI_FREQUENCY_400K,
	[TWI_MASTER_4] = TWI_FREQUENCY_400K,
	[TWI_MASTER_5] = TWI_FREQUENCY_400K,
	[TWI_MASTER_6] = TWI_FREQUENCY_400K,
	[S_TWI_MASTER_0] = TWI_FREQUENCY_400K,
	[S_TWI_MASTER_1] = TWI_FREQUENCY_400K,
};

uint32_t hal_twi_get_mode(twi_port_t port)
{
	if (port >= TWI_MASTER_MAX)
		return TWI_MODE_ENGINE;

	if (twi_mode_table[port] == TWI_DRV_XFER)
		return TWI_MODE_DRV;
	else
		return TWI_MODE_ENGINE;
}

twi_status_t hal_twi_set_mode(twi_port_t port, uint32_t mode)
{
	if (port >= TWI_MASTER_MAX)
		return TWI_STATUS_ERROR;

	if (mode & TWI_MODE_DRV)
		twi_mode_table[port] = TWI_DRV_XFER;
	else
		twi_mode_table[port] = ENGINE_XFER;

	return TWI_STATUS_OK;
}

uint32_t hal_twi_get_freq(twi_port_t port)
{
	if (port >= TWI_MASTER_MAX)
		return TWI_FREQUENCY_400K;

	return twi_freq_table[port];
}

twi_status_t hal_twi_set_freq(twi_port_t port, twi_frequency_t freq)
{
	if (port >= TWI_MASTER_MAX)
		return TWI_STATUS_ERROR;

	twi_freq_table[port] = freq;

	return TWI_STATUS_OK;

}
#if 0
static const enum REGULATOR_TYPE_ENUM twi_regulator_type = AXP2101_REGULATOR;

static const enum REGULATOR_ID_ENUM twi_regulator_id[] =
{
    AXP2101_ID_ALDO2,
    AXP2101_ID_ALDO2,
    AXP2101_ID_MAX,
    AXP2101_ID_MAX,
    AXP2101_ID_MAX,
};



static const int twi_vol[] =
{
    3300000,
    3300000,
    -1,
    -1,
    -1,
};

#endif

#ifdef CONFIG_DRIVERS_DMA
static int hal_twi_get_drqsrc(int port)
{
	switch(port) {
#ifdef DRQSRC_TWI0_RX
	case TWI_MASTER_0: return DRQSRC_TWI0_RX;
#endif
#ifdef DRQSRC_TWI1_RX
	case TWI_MASTER_1: return DRQSRC_TWI1_RX;
#endif
#ifdef DRQSRC_TWI2_RX
	case TWI_MASTER_2: return DRQSRC_TWI2_RX;
#endif
#ifdef DRQSRC_TWI3_RX
	case TWI_MASTER_3: return DRQSRC_TWI3_RX;
#endif
#ifdef DRQSRC_TWI4_RX
	case TWI_MASTER_4: return DRQSRC_TWI4_RX;
#endif
#ifdef DRQSRC_TWI5_RX
	case TWI_MASTER_5: return DRQSRC_TWI5_RX;
#endif
#ifdef DRQSRC_TWI6_RX
	case TWI_MASTER_6: return DRQSRC_TWI6_RX;
#endif
#ifdef DRQSRC_R_TWI0_RX
	case S_TWI_MASTER_0: return DRQSRC_R_TWI0_RX;
#endif
#ifdef DRQSRC_R_TWI1_RX
	case S_TWI_MASTER_1: return DRQSRC_R_TWI1_RX;
#endif
	default:
		TWI_ERR("[twi%d] can not get DRQSRC!", port);
		return -1;
	}
}

static int hal_twi_get_drqdst(int port)
{
	switch(port) {
#ifdef DRQDST_TWI0_TX
	case TWI_MASTER_0: return DRQDST_TWI0_TX;
#endif
#ifdef DRQDST_TWI1_TX
	case TWI_MASTER_1: return DRQDST_TWI1_TX;
#endif
#ifdef DRQDST_TWI2_TX
	case TWI_MASTER_2: return DRQDST_TWI2_TX;
#endif
#ifdef DRQDST_TWI3_TX
	case TWI_MASTER_3: return DRQDST_TWI3_TX;
#endif
#ifdef DRQDST_TWI4_TX
	case TWI_MASTER_4: return DRQDST_TWI4_TX;
#endif
#ifdef DRQDST_TWI5_TX
	case TWI_MASTER_5: return DRQDST_TWI5_TX;
#endif
#ifdef DRQDST_TWI6_TX
	case TWI_MASTER_6: return DRQDST_TWI6_TX;
#endif
#ifdef DRQDST_R_TWI0_TX
	case S_TWI_MASTER_0: return DRQDST_R_TWI0_TX;
#endif
#ifdef DRQDST_R_TWI1_TX
	case S_TWI_MASTER_1: return DRQDST_R_TWI1_TX;
#endif
	default:
		TWI_ERR("[twi%d] can not get DRQDST!", port);
		return -1;
	}
}
#endif

static hal_twi_t hal_twi[TWI_MASTER_MAX];

inner_attr void twi_set_bit(unsigned long reg, unsigned int mask, unsigned int offset, unsigned int data)
{
	unsigned int val = readl(reg) & (~mask);

	writel(val | ((data << offset) & mask), reg);
}

inner_attr unsigned int twi_get_bit(unsigned long reg, unsigned int mask, unsigned int offset)
{
	return (readl(reg) & mask) >> offset;
}

typedef struct {
	unsigned int reg_off;
	unsigned int sda_sta;
	unsigned int sda_sta_off;
	unsigned int scl_sta;
	unsigned int scl_sta_off;
	unsigned int sda_mov;
	unsigned int sda_mov_off;
	unsigned int scl_mov;
	unsigned int scl_mov_off;
	unsigned int sda_moe;
	unsigned int sda_moe_off;
	unsigned int scl_moe;
	unsigned int scl_moe_off;
} twi_line_reg;

static const twi_line_reg engine_line_reg = {
	.reg_off	= TWI_LCR_REG,
	.scl_sta	= TWI_LCR_SCL_STA,
	.scl_sta_off	= TWI_LCR_SCL_STA_OFFSET,
	.sda_sta	= TWI_LCR_SDA_STA,
	.sda_sta_off	= TWI_LCR_SDA_STA_OFFSET,
	.scl_mov	= TWI_LCR_SCL_CTL,
	.scl_mov_off	= TWI_LCR_SCL_CTL_OFFSET,
	.sda_mov	= TWI_LCR_SDA_CTL,
	.sda_mov_off	= TWI_LCR_SDA_CTL_OFFSET,
	.scl_moe	= TWI_LCR_SCL_EN,
	.scl_moe_off	= TWI_LCR_SCL_EN_OFFSET,
	.sda_moe	= TWI_LCR_SDA_EN,
	.sda_moe_off	= TWI_LCR_SDA_EN_OFFSET,
};

static const twi_line_reg drv_line_reg = {
	.reg_off	= TWI_DRIVER_BUSC,
	.scl_sta	= TWI_DRV_SCL_STA,
	.scl_sta_off	= TWI_DRV_SCL_STA_OFFSET,
	.sda_sta	= TWI_DRV_SDA_STA,
	.sda_sta_off	= TWI_DRV_SDA_STA_OFFSET,
	.scl_mov	= TWI_DRV_SCL_MOV,
	.scl_mov_off	= TWI_DRV_SCL_MOV_OFFSET,
	.sda_mov	= TWI_DRV_SDA_MOV,
	.sda_mov_off	= TWI_DRV_SDA_MOV_OFFSET,
	.scl_moe	= TWI_DRV_SCL_MOE,
	.scl_moe_off	= TWI_DRV_SCL_MOE_OFFSET,
	.sda_moe	= TWI_DRV_SDA_MOE,
	.sda_moe_off	= TWI_DRV_SDA_MOE_OFFSET,
};

inner_attr uint32_t twi_get_scl(unsigned long base_addr, const twi_line_reg *reg)
{
	return twi_get_bit(base_addr + reg->reg_off, reg->scl_sta, reg->scl_sta_off);
}

inner_attr uint32_t twi_get_sda(unsigned long base_addr, const twi_line_reg *reg)
{
	return twi_get_bit(base_addr + reg->reg_off, reg->sda_sta, reg->sda_sta_off);
}

inner_attr void twi_set_scl(unsigned long base_addr, const twi_line_reg *reg, uint32_t val)
{
	twi_set_bit(base_addr + reg->reg_off, reg->scl_mov, reg->scl_mov_off, val ? 1 : 0);
}

inner_attr void twi_set_sda(unsigned long base_addr, const twi_line_reg *reg, uint32_t val)
{
	twi_set_bit(base_addr + reg->reg_off, reg->sda_mov, reg->sda_mov_off, val ? 1 : 0);
}

inner_attr void twi_scl_control(unsigned long base_addr, const twi_line_reg *reg, uint32_t enable)
{
	twi_set_bit(base_addr + reg->reg_off, reg->scl_moe, reg->scl_moe_off, enable ? 1 : 0);
}

inner_attr void twi_sda_control(unsigned long base_addr, const twi_line_reg *reg, uint32_t enable)
{
	twi_set_bit(base_addr + reg->reg_off, reg->sda_moe, reg->sda_moe_off, enable ? 1 : 0);
}

/* set twi clock
 *
 * clk_n: clock divider factor n
 * clk_m: clock divider factor m
 */
static void twi_clk_write_reg(hal_twi_t *twi, unsigned int reg_clk,
                              unsigned int clk_m, unsigned int clk_n,
                              unsigned int mask_clk_m, unsigned int mask_clk_n)
{
    const unsigned long base_addr = twi->base_addr;
    unsigned int reg_val = readl(base_addr + reg_clk);
    u32 duty;

    if (reg_clk == TWI_DRIVER_BUSC)
    {
        reg_val &= ~(mask_clk_m | mask_clk_n);
        reg_val |= ((clk_m | (clk_n << 4)) << 8);
        if(twi->freq == TWI_FREQUENCY_400K)
            duty= TWI_DRV_CLK_DUTY_30_EN;
        else
            duty = TWI_DRV_CLK_DUTY;
        reg_val |= TWI_DRV_CLK_COUNT_MODE;
    }
    else
    {
        reg_val &= ~(mask_clk_m | mask_clk_n);
        reg_val |= ((clk_m  << 3) | clk_n);
        if(twi->freq == TWI_FREQUENCY_400K)
            duty= TWI_CLK_DUTY_30_EN;
        else
            duty = TWI_CLK_DUTY;
    }
    if (twi->freq > TWI_FREQUENCY_100K)
        reg_val |= duty;
    else
        reg_val &= ~(duty);

    writel(reg_val, base_addr + reg_clk);
}

static void twi_drv_set_timeout(hal_twi_t *twi, unsigned int timeout)
{
	u32 reg_val;
	reg_val = readl(twi->base_addr + TWI_DRIVER_CTRL);
	reg_val &= ~TIMEOUT_N;
	reg_val |= (timeout << TIMEOUT_N_OFFSET);

	writel(reg_val, twi->base_addr + TWI_DRIVER_CTRL);
}

/*
* Fin is APB CLOCK INPUT;
* Fsample = F0 = Fin/2^CLK_N;
* F1 = F0/(CLK_M+1);
* Foscl = F1/10 = Fin/(2^CLK_N * (CLK_M+1)*10);
* Foscl is clock SCL;100KHz or 400KHz
*
* clk_in: apb clk clock
* sclk_req: freqence to set in HZ
*/
static int twi_set_clock(hal_twi_t *twi, unsigned int reg_clk,
                         unsigned int clk_in, unsigned int sclk_req,
                         unsigned int mask_clk_m, unsigned int mask_clk_n)
{
    unsigned int clk_m = 0;
    unsigned int clk_n = 0;
    unsigned int _2_pow_clk_n = 1;
    unsigned int src_clk      = clk_in / 10;
    unsigned int divider      = src_clk / sclk_req; /* 400khz or 100khz */
    unsigned int sclk_real    = 0;      /* the real clock frequency */

    if (divider == 0)
    {
        clk_m = 1;
        goto set_clk;
    }

    /*
     * search clk_n and clk_m,from large to small value so
     * that can quickly find suitable m & n.
     */
    while (clk_n < 8)   /* 3bits max value is 8 */
    {
        /* (m+1)*2^n = divider -->m = divider/2^n -1 */
        clk_m = (divider / _2_pow_clk_n) - 1;
        /* clk_m = (divider >> (_2_pow_clk_n>>1))-1 */
        while (clk_m < 16)   /* 4bits max value is 16 */
        {
            /* src_clk/((m+1)*2^n) */
            sclk_real = src_clk / (clk_m + 1) / _2_pow_clk_n;
            if (sclk_real <= sclk_req)
            {
                goto set_clk;
            }
            else
            {
                clk_m++;
            }
        }
        clk_n++;
        _2_pow_clk_n *= 2; /* mutilple by 2 */
    }

set_clk:
    twi_clk_write_reg(twi, reg_clk, clk_m, clk_n, mask_clk_m, mask_clk_n);
    return 0;
}


/*************************************** TWI ENGINE XFER REG CONTROL begin****************************/

/* clear the interrupt flag */
static inline void twi_clear_irq_flag(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    /* start and stop bit should be 0 */
    reg_val |= TWI_CTL_INTFLG;
    reg_val &= ~(TWI_CTL_STA | TWI_CTL_STP);
    writel(reg_val, base_addr + TWI_CTL_REG);
    /* read two more times to make sure that */
    /* interrupt flag does really be cleared */
    {
        unsigned int temp;

        temp = readl(base_addr + TWI_CTL_REG);
        temp |= readl(base_addr + TWI_CTL_REG);
    }
}

/* get data first, then clear flag */
static inline void twi_get_byte(const unsigned long base_addr, unsigned char  *buffer)
{
    *buffer = (unsigned char)(TWI_DATA_MASK & readl(base_addr + TWI_DATA_REG));
    twi_clear_irq_flag(base_addr);
}

/* only get data, we will clear the flag when stop */
static inline void twi_get_last_byte(const unsigned long base_addr, unsigned char  *buffer)
{
    *buffer = (unsigned char)(TWI_DATA_MASK &
                              readl(base_addr + TWI_DATA_REG));
}

/* write data and clear irq flag to trigger send flow */
static inline void twi_put_byte(const unsigned long base_addr, const unsigned char *buffer)
{
    writel((unsigned int)*buffer, base_addr + TWI_DATA_REG);
    twi_clear_irq_flag(base_addr);
}

static inline void twi_enable_irq(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

    /*
     * 1 when enable irq for next operation, set intflag to 0 to prevent
     * to clear it by a mistake (intflag bit is write-1-to-clear bit)
     * 2 Similarly, mask START bit and STOP bit to prevent to set it
     * twice by a mistake (START bit and STOP bit are self-clear-to-0 bits)
     */
    reg_val |= TWI_CTL_INTEN;
    reg_val &= ~(TWI_CTL_STA | TWI_CTL_STP | TWI_CTL_INTFLG);
    writel(reg_val, base_addr + TWI_CTL_REG);
}

static inline void twi_disable_irq(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

    reg_val &= ~TWI_CTL_INTEN;
    reg_val &= ~(TWI_CTL_STA | TWI_CTL_STP | TWI_CTL_INTFLG);
    writel(reg_val, base_addr + TWI_CTL_REG);
}

static inline void twi_disable(const unsigned long base_addr, unsigned int reg, unsigned int mask)
{
    unsigned int reg_val = readl(base_addr + reg);

    reg_val &= ~mask;
    writel(reg_val, base_addr + reg);
    TWI_INFO("offset: 0x%x value: 0x%x", reg, readl(base_addr + reg));
}

static inline void twi_enable(const unsigned long base_addr, unsigned int reg, unsigned int mask)
{
    unsigned int reg_val = readl(base_addr + reg);

    reg_val |= mask;
    writel(reg_val, base_addr + reg);
    TWI_INFO("offset: 0x%x value: 0x%x", reg,
             readl(base_addr + reg));
}

/* trigger start signal, the start bit will be cleared automatically */
static inline void twi_set_start(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

    reg_val |= TWI_CTL_STA;
    reg_val &= ~TWI_CTL_INTFLG;
    writel(reg_val, base_addr + TWI_CTL_REG);
}

/* get start bit status, poll if start signal is sent */
static inline unsigned int twi_get_start(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

    reg_val >>= 5;
    return reg_val & 1;
}

/* trigger stop signal, the stop bit will be cleared automatically */
static inline void twi_set_stop(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

    reg_val |= TWI_CTL_STP;
    reg_val &= ~TWI_CTL_INTFLG;
    writel(reg_val, base_addr + TWI_CTL_REG);
}

/* get stop bit status, poll if stop signal is sent */
static inline unsigned int twi_get_stop(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

    reg_val >>= 4;
    return reg_val & 1;
}

static inline void twi_disable_ack(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

    reg_val &= ~TWI_CTL_ACK;
    reg_val &= ~TWI_CTL_INTFLG;
    writel(reg_val, base_addr + TWI_CTL_REG);
}

/* when sending ack or nack, it will send ack automatically */
static inline void twi_enable_ack(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

    reg_val |= TWI_CTL_ACK;
    reg_val &= ~TWI_CTL_INTFLG;
    writel(reg_val, base_addr + TWI_CTL_REG);
}

/* get the interrupt flag */
static inline unsigned int twi_query_irq_flag(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

    return (reg_val & TWI_CTL_INTFLG);/* 0x 0000_1000 */
}

/* get interrupt status */
static inline unsigned int twi_query_irq_status(const unsigned long base_addr)
{
    unsigned int reg_val = readl(base_addr + TWI_STAT_REG);

    return (reg_val & TWI_STAT_MASK);
}

/* Enhanced Feature Register */
static inline void twi_set_efr(const unsigned long base_addr, unsigned int efr)
{
    unsigned int reg_val = readl(base_addr + TWI_EFR_REG);

    reg_val &= ~TWI_EFR_MASK;
    efr     &= TWI_EFR_MASK;
    reg_val |= efr;
    writel(reg_val, base_addr + TWI_EFR_REG);
}

/* function  */
static int twi_start(const unsigned long base_addr, int port)
{
    unsigned int timeout = 0xff;

    twi_set_start(base_addr);
    while ((twi_get_start(base_addr) == 1) && (--timeout))
        ;
    if (timeout == 0)
    {
        TWI_ERR("[twi%d] START can't sendout!", port);
        return SUNXI_TWI_FAIL;
    }

    return SUNXI_TWI_OK;
}

static int twi_restart(const unsigned long base_addr, int port)
{
    unsigned int timeout = 0xff;

    twi_set_start(base_addr);
    twi_clear_irq_flag(base_addr);
    while ((twi_get_start(base_addr) == 1) && (--timeout))
        ;
    if (timeout == 0)
    {
        TWI_ERR("[twi%d] Restart can't sendout!", port);
        return SUNXI_TWI_FAIL;
    }

    return SUNXI_TWI_OK;
}

static int twi_stop(const unsigned long base_addr, int port)
{
    unsigned int timeout = 0xff;

    twi_set_stop(base_addr);
    //unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
    twi_clear_irq_flag(base_addr);

    twi_get_stop(base_addr);/* it must delay 1 nop to check stop bit */
    while ((twi_get_stop(base_addr) == 1) && (--timeout))
        ;
    if (timeout == 0)
    {
        TWI_ERR("[twi%d] STOP can't sendout!", port);
        return SUNXI_TWI_TFAIL;
    }

    //twi_clear_irq_flag(base_addr);
    timeout = 0xff;
    while ((readl(base_addr + TWI_STAT_REG) != TWI_STAT_IDLE)
           && (--timeout));

    if (timeout == 0)
    {
        TWI_ERR("[twi%d] twi state(0x%0u) isn't idle(0xf8)",
                port, readl(base_addr + TWI_STAT_REG));
        return SUNXI_TWI_TFAIL;
    }

    timeout = 0xff;
    while ((readl(base_addr + TWI_LCR_REG) != TWI_LCR_IDLE_STATUS
            && readl(base_addr + TWI_LCR_REG) != TWI_LCR_NORM_STATUS)
           && (--timeout))
        ;

    if (timeout == 0)
    {
        TWI_ERR("[twi%d] twi lcr(0x%0u) isn't idle(0x3a)",
                port, readl(base_addr + TWI_LCR_REG));
        return SUNXI_TWI_TFAIL;
    }

    //twi_clear_irq_flag(base_addr);
    TWI_INFO("twi stop end");

    return SUNXI_TWI_OK;
}

/* send 9 clock to release sda */
static twi_status_t twi_send_clk_9pulse(hal_twi_t *twi)
{
	int cycle = 9;
	uint32_t status;
	const twi_line_reg *reg = twi->twi_drv_used ? &drv_line_reg : &engine_line_reg;
	const unsigned long base_addr = twi->base_addr;
	int high_delay_us = 4; // 100Khz * 40% duty
	int low_delay_us = 6; // 100Khz

	/* enable scl control */
	twi_scl_control(base_addr, reg, 1);

	while (cycle--) {
		status = twi_get_sda(base_addr, reg);
		if (status)
			break;

		/* twi_scl -> low */
		twi_set_scl(base_addr, reg, 0);
		hal_udelay(low_delay_us);
		/* twi_scl -> high */
		twi_set_scl(base_addr, reg, 1);
		hal_udelay(high_delay_us);
	}

	status = twi_get_sda(base_addr, reg);

	/* disable scl control */
	twi_scl_control(base_addr, reg, 0);

	if (!status) {
		TWI_ERR("[twi%d] SDA is still Stuck Low, failed.", twi->port);
		return SUNXI_TWI_FAIL;
	}

	return SUNXI_TWI_OK;
}

/*************************************** TWI DRV XFER REG CONTROL begin****************************/

#if 0

/* set twi clock
 *
 * clk_n: clock divider factor n
 * clk_m: clock divider factor m
 */
static void twi_clk_write_reg(const uint32_t base_addr, uint32_t sclk_freq,
                              uint8_t clk_m, uint8_t clk_n)
{
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_BUSC);
#if defined(CONFIG_ARCH_SUN50IW10)
    uint32_t duty;
#endif
    TWI_INFO("reg_clk = 0x%x, clk_m = %u, clk_n = %u,"
             "mask_clk_m = %x, mask_clk_n = %x",
             reg_clk, clk_m, clk_n, mask_clk_m, mask_clk_n);

    reg_val &= ~(TWI_DRV_CLK_M | TWI_DRV_CLK_N);
    reg_val |= ((clk_m | (clk_n << 4)) << 8);
#if defined(CONFIG_ARCH_SUN50IW10)
    duty = TWI_DRV_CLK_DUTY;
    if (sclk_freq > STANDDARD_FREQ)
    {
        reg_val |= duty;
    }
    else
    {
        reg_val &= ~duty;
    }
#endif
    writel(reg_val, base_addr + TWI_DRIVER_BUSC);
}


/*
* Fin is APB CLOCK INPUT;
* Fsample = F0 = Fin/2^CLK_N;
* F1 = F0/(CLK_M+1);
* Foscl = F1/10 = Fin/(2^CLK_N * (CLK_M+1)*10);
* Foscl is clock SCL;100KHz or 400KHz
*
* clk_in: apb clk clock
* sclk_freq: freqence to set in HZ
*/
static int32_t twi_set_clock(twi_port_t twi_port,
                             uint32_t clk_in, uint32_t sclk_freq)
{
    const uint32_t base_addr = g_twi_regbase[twi->port];

    uint8_t clk_m = 0, clk_n = 0, _2_pow_clk_n = 1;
    uint32_t src_clk      = clk_in / 10;
    uint32_t divider      = src_clk / sclk_freq; /* 400khz or 100khz */
    uint32_t sclk_real    = 0;      /* the real clock frequency */

    if (divider == 0)
    {
        clk_m = 1;
        goto set_clk;
    }

    /*
     * search clk_n and clk_m,from large to small value so
     * that can quickly find suitable m & n.
     */
    while (clk_n < 8)   /* 3bits max value is 8 */
    {
        /* (m+1)*2^n = divider -->m = divider/2^n -1 */
        clk_m = (divider / _2_pow_clk_n) - 1;
        /* clk_m = (divider >> (_2_pow_clk_n>>1))-1 */
        while (clk_m < 16)   /* 4bits max value is 16 */
        {
            /* src_clk/((m+1)*2^n) */
            sclk_real = src_clk / (clk_m + 1) / _2_pow_clk_n;
            if (sclk_real <= sclk_freq)
            {
                goto set_clk;
            }
            else
            {
                clk_m++;
            }
        }
        clk_n++;
        _2_pow_clk_n *= 2; /* mutilple by 2 */
    }

set_clk:
    twi_clk_write_reg(base_addr, sclk_freq, clk_m, clk_n);
    return 0;
}
#endif

inner_attr uint32_t twi_drv_query_irq_status(const unsigned long base_addr)
{
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_INTC);

    return (reg_val & TWI_DRV_STAT_MASK);
}

inner_attr uint32_t twi_drv_query_enabled_irq_status(const unsigned long base_addr)
{
	uint32_t reg_val = readl(base_addr + TWI_DRIVER_INTC);
	uint32_t status = reg_val & TWI_DRV_STAT_MASK;
	uint32_t enable = (reg_val & TWI_DRV_INT_MASK) >> 16;

	return (status & enable);
}

static void twi_drv_clear_irq_flag(uint32_t pending_bit, const unsigned long base_addr)
{
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_INTC);

    pending_bit &= TWI_DRV_STAT_MASK;
    reg_val |= pending_bit;
    writel(reg_val, base_addr + TWI_DRIVER_INTC);
}

static void twi_clear_pending(const unsigned long base_addr)
{
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_INTC);

    reg_val |= TWI_DRV_STAT_MASK;
    writel(reg_val, base_addr + TWI_DRIVER_INTC);
}

/* start TWI transfer */
static void twi_start_xfer(const unsigned long base_addr)
{
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_CTRL);

    reg_val |= START_TRAN;
    writel(reg_val, base_addr + TWI_DRIVER_CTRL);
}

/*
 * send DMA RX Req when the data byte number in RECV_FIFO reaches RX_TRIG
 * or Read Packet Tansmission completed with RECV_FIFO not empty
 */
static void twi_set_rx_trig_level(uint32_t val, const unsigned long base_addr)
{
    uint32_t mask = TRIG_MASK;
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_DMAC);

    val = (val & mask) << 16;
    reg_val &= ~(mask << 16);
    reg_val |= val;
    writel(reg_val, base_addr + TWI_DRIVER_DMAC);
}

static void twi_set_tx_trig_level(uint32_t val, const unsigned long base_addr)
{
    uint32_t mask = TRIG_MASK;
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_DMAC);

    val = (val & mask) << 0;
    reg_val &= ~(mask << 0);
    reg_val |= val;
    writel(reg_val, base_addr + TWI_DRIVER_DMAC);
}
/* bytes be send as slave device reg address */
static void twi_set_packet_addr_byte(uint32_t val, const unsigned long base_addr)
{
    uint32_t mask = ADDR_BYTE;
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_FMT);

    reg_val &= ~mask;
    val = (val << 16) & mask;
    reg_val |= val;
    writel(reg_val, base_addr + TWI_DRIVER_FMT);
}

/* bytes be send/received as data */
static void twi_set_packet_data_byte(uint32_t val, const unsigned long base_addr)
{
    uint32_t mask = DATA_BYTE;
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_FMT);

    reg_val &= ~mask;
    val &= mask;
    reg_val |= val;
    writel(reg_val, base_addr + TWI_DRIVER_FMT);
}

#if 0
/* interval between each packet in 32*Fscl cycles */
static void twi_set_packet_interval(uint32_t val, const uint32_t base_addr)
{
    uint32_t mask = INTERVAL_MASK;
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_CFG);

    reg_val &= ~mask;
    val <<= 16;
    val &= mask;
    reg_val |= val;
    writel(reg_val, base_addr + TWI_DRIVER_CFG);
}
#endif

/* FIFO data be transmitted as PACKET_CNT packets in current format */
static void twi_set_packet_cnt(uint32_t val, const unsigned long base_addr)
{
    uint32_t mask = PACKET_MASK;
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_CFG);

    reg_val &= ~mask;
    val &= mask;
    reg_val |= val;
    writel(reg_val, base_addr + TWI_DRIVER_CFG);
}

/* do not send slave_id +W */
static void twi_enable_read_tran_mode(const unsigned long base_addr)
{
    uint32_t mask = READ_TRAN;
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_CTRL);

    reg_val |= mask;
    writel(reg_val, base_addr + TWI_DRIVER_CTRL);
}

/* send slave_id + W */
static void twi_disable_read_tran_mode(const unsigned long base_addr)
{
    uint32_t mask = READ_TRAN;
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_CTRL);

    reg_val &= ~mask;
    writel(reg_val, base_addr + TWI_DRIVER_CTRL);
}

static inline void twi_engine_soft_reset(const unsigned long base_addr)
{
	twi_set_bit(base_addr + TWI_SRST_REG, TWI_SRST_SRST, TWI_SRST_SRST_OFFSET, 1);

	/* Hardware will auto clear this bit when soft reset
	 * Before return, driver must wait reset opertion complete */
	while (twi_get_bit(base_addr + TWI_SRST_REG, TWI_SRST_SRST, TWI_SRST_SRST_OFFSET));
}

static inline void twi_drv_soft_reset(const unsigned long base_addr)
{
	twi_set_bit(base_addr + TWI_DRIVER_CTRL, TWI_DRV_RST, TWI_DRV_RST_OFFSET, 1);

	hal_udelay(5);
	/*
	 * @IP-TODO
	 * drv-mode soft_reset bit will not clear automatically, write 0 to unreset.
	 * The reset only takes one or two CPU clk cycle.
	 */
	twi_set_bit(base_addr + TWI_DRIVER_CTRL, TWI_DRV_RST, TWI_DRV_RST_OFFSET, 0);
}

static inline void twi_soft_reset(hal_twi_t *twi)
{
	if (twi->twi_drv_used)
		twi_drv_soft_reset(twi->base_addr);
	else
		twi_engine_soft_reset(twi->base_addr);
}

static void twi_slave_reset(hal_twi_t *twi)
{
    twi_clear_irq_flag(twi->base_addr);
    twi_disable_ack(twi->base_addr);
    twi_soft_reset(twi);
    twi_enable_ack(twi->base_addr);
    twi->status = TWI_XFER_SLV_IDLE;
}

static void twi_enable_tran_irq(uint32_t bitmap, const unsigned long base_addr)
{
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_INTC);

    reg_val |= bitmap;
    reg_val &= ~TWI_DRV_STAT_MASK;
    writel(reg_val, base_addr + TWI_DRIVER_INTC);
}

static void twi_disable_tran_irq(uint32_t bitmap, const unsigned long base_addr)
{
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_INTC);

    reg_val &= ~bitmap;
    reg_val &= ~TWI_DRV_STAT_MASK;
    writel(reg_val, base_addr + TWI_DRIVER_INTC);
}

#ifdef CONFIG_DRIVERS_DMA
static void twi_enable_dma_irq(uint32_t bitmap, const unsigned long base_addr)
{
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_DMAC);

    bitmap &= TWI_DRQEN_MASK;
    reg_val |= bitmap;
    writel(reg_val, base_addr + TWI_DRIVER_DMAC);
}

static void twi_disable_dma_irq(uint32_t bitmap, const unsigned long base_addr)
{
    uint32_t reg_val = readl(base_addr + TWI_DRIVER_DMAC);

    bitmap &= TWI_DRQEN_MASK;
    reg_val &= ~bitmap;
    writel(reg_val, base_addr + TWI_DRIVER_DMAC);
}
#else
static void twi_disable_dma_irq(uint32_t bitmap, const unsigned long base_addr)
{

}
#endif

static void twi_slave_addr(const unsigned long base_addr, twi_msg_t *msgs)
{
    uint32_t val = 0, cmd = 0;

    /* read, default value is write */
    if (msgs->flags & TWI_M_RD)
    {
        cmd = SLV_RD_CMD;
    }

    if (msgs->flags & TWI_M_TEN)
    {
        /* SLV_ID | CMD | SLV_ID_X */
        val = ((0x78 | ((msgs->addr >> 8) & 0x03)) << 9) | cmd
              | (msgs->addr & 0xff);
    }
    else
    {
        val = ((msgs->addr & 0x7f) << 9) | cmd;
    }

    writel(val, base_addr + TWI_DRIVER_SLV);
}


/* the number of data in SEND_FIFO */
static int32_t twi_query_txfifo(const unsigned long base_addr)
{
    uint32_t reg_val;

    reg_val = readl(base_addr + TWI_DRIVER_FIFOC) & SEND_FIFO_CONT;

    return reg_val;
}

/* the number of data in RECV_FIFO */
static int32_t twi_query_rxfifo(const unsigned long base_addr)
{
    uint32_t reg_val;

    reg_val = readl(base_addr + TWI_DRIVER_FIFOC) & RECV_FIFO_CONT;
    reg_val >>= 16;

    return reg_val;
}

static void twi_clear_txfifo(const unsigned long base_addr)
{
    uint32_t reg_val;

    reg_val = readl(base_addr + TWI_DRIVER_FIFOC);
    reg_val |= SEND_FIFO_CLEAR;
    writel(reg_val, base_addr + TWI_DRIVER_FIFOC);
}

static void twi_clear_rxfifo(const unsigned long base_addr)
{
    uint32_t reg_val;

    reg_val = readl(base_addr + TWI_DRIVER_FIFOC);
    reg_val |= RECV_FIFO_CLEAR;
    writel(reg_val, base_addr + TWI_DRIVER_FIFOC);
}

struct twi_reg_type {
	uint32_t flags;
	const char *name;
	uint32_t offset;
};

#define COM_REG	(0x1 << 0)
#define DRV_REG	(0x1 << 1)
#define R2C_REG	(0x1 << 16)
#define ALL_REG	(COM_REG | DRV_REG | R2C_REG)

struct twi_reg_type dump_reg_table[] = {
	{COM_REG, "VERSION", 0xfc},
	{COM_REG, "ADDR", TWI_ADDR_REG},
	{COM_REG, "XADDR", TWI_XADDR_REG},
	{R2C_REG, "DATA", TWI_DATA_REG},
	{COM_REG, "CTRL", TWI_CTL_REG},
	{COM_REG, "STAT", TWI_STAT_REG},
	{COM_REG, "CLKR", TWI_CLK_REG},
	{COM_REG, "SRST", TWI_SRST_REG},
	{COM_REG, "EFR", TWI_EFR_REG},
	{COM_REG, "LCR", TWI_LCR_REG},
	{COM_REG, "DVFS", TWI_DVFS_REG},
	{COM_REG | DRV_REG, "DRV_CTRL", TWI_DRIVER_CTRL},
	{COM_REG | DRV_REG, "DRV_CFG", TWI_DRIVER_CFG},
	{COM_REG | DRV_REG, "DRV_SLV", TWI_DRIVER_SLV},
	{COM_REG | DRV_REG, "DRV_FMT", TWI_DRIVER_FMT},
	{COM_REG | DRV_REG, "DRV_BUS_CTRL", TWI_DRIVER_BUSC},
	{COM_REG | DRV_REG, "DRV_INT_CTRL", TWI_DRIVER_INTC},
	{COM_REG | DRV_REG, "DRV_DMA_CFG", TWI_DRIVER_DMAC},
	{COM_REG | DRV_REG, "DRV_FIFO_CON", TWI_DRIVER_FIFOC},
	{COM_REG | DRV_REG, "DRV_SEND_FIFO_ACC", TWI_DRIVER_SENDF},
	{R2C_REG, "DRV_RECV_FIFO_ACC", TWI_DRIVER_SENDF},
};

static void twi_dump_reg(hal_twi_t *twi, uint32_t flags)
{
	uint32_t reg_val;
	int i;

	TWI_ERR("[twi%d] base_addr: %lx", twi->port, (unsigned long)twi->base_addr);

	for (i = 0; i < ARRAY_SIZE(dump_reg_table); i++) {
		if (!(dump_reg_table[i].flags & flags)) {
			//TWI_ERR("%s: (ignore)", dump_reg_table[i].name);
			continue;
		}
		reg_val = hal_readl(twi->base_addr + dump_reg_table[i].offset);
		TWI_ERR("%s: 0x%x", dump_reg_table[i].name, reg_val);
	}
}

static int twi_send_msgs(hal_twi_t *twi, twi_msg_t *msgs)
{
    uint16_t i;
    uint8_t time = 0xff;

    TWI_INFO("twi[%d] msgs->len = %d", twi->port, msgs->len);

    for (i = 0; i < msgs->len; i++)
    {
        while ((twi_query_txfifo(twi->base_addr) >= MAX_FIFO) && time--)
            ;
        if (time)
        {
            hal_writeb(msgs->buf[i], twi->base_addr + TWI_DRIVER_SENDF);
        }
        else
        {
            TWI_ERR("[twi%d] SEND FIFO overflow. timeout", twi->port);
            return SUNXI_TWI_FAIL;
        }
    }

    return SUNXI_TWI_OK;
}

static uint32_t twi_recv_msgs(hal_twi_t *twi, twi_msg_t *msgs)
{
    uint16_t i;
    uint8_t time = 0xff;

    TWI_INFO("twi[%d] msgs->len = %d", twi->port, msgs->len);

    for (i = 0; i < msgs->len; i++)
    {
        while (!twi_query_rxfifo(twi->base_addr) && time--)
            ;
        if (time)
        {
            msgs->buf[i] = hal_readb(twi->base_addr + TWI_DRIVER_RECVF);
        }
        else
        {
            return 0;
        }
    }
    return msgs->len;
}
/************************ TWI DRV XFER REG CONTROL end*************************/
#ifdef CONFIG_DRIVERS_DMA
static void twi_dma_callback(void *para)
{
    hal_twi_t *twi = (hal_twi_t *)para;
    int hal_sem_ret;

    hal_sem_ret = hal_sem_post(twi->dma_complete);
    if (hal_sem_ret != 0)
    {
        TWI_ERR("[twi%d] twi dma driver xfer timeout (dev addr:0x%x)\n", twi->port, twi->msgs->addr);
	return;
    }
}

static int twi_dma_xfer(hal_twi_t *twi, char *buf, int len, enum dma_transfer_direction dir)
{
    struct sunxi_dma_chan *dma_chan = twi->dma_chan;
    struct dma_slave_config slave_config;
    int hal_sem_ret;
    int dma_drq;

    hal_dcache_clean((unsigned long)buf, len);
    hal_dma_callback_install(dma_chan, twi_dma_callback, twi);
    if (dir == DMA_MEM_TO_DEV)
    {
        dma_drq = hal_twi_get_drqdst(twi->port);
        slave_config.direction = DMA_MEM_TO_DEV;
	slave_config.src_addr = (unsigned long)buf;
	slave_config.dst_addr = twi->base_addr + TWI_DRIVER_SENDF;
        slave_config.slave_id = sunxi_slave_id(dma_drq, DRQSRC_SDRAM);
    }
    else
    {
        dma_drq = hal_twi_get_drqsrc(twi->port);
        slave_config.direction = DMA_DEV_TO_MEM;
	slave_config.src_addr = twi->base_addr + TWI_DRIVER_RECVF;
	slave_config.dst_addr = (unsigned long)buf;
	slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM, dma_drq);
    }
    if (dma_drq == -1) {
            TWI_ERR("[twi%d] dma_drq error!\n", twi->port);;
            return -1;
    }
    slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
    slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
    slave_config.src_maxburst = DMA_SLAVE_BURST_16;
    slave_config.dst_maxburst = DMA_SLAVE_BURST_16;
    hal_dma_slave_config(dma_chan, &slave_config);
    if (dir == DMA_MEM_TO_DEV)
    {
        hal_dma_prep_device(dma_chan, slave_config.dst_addr, slave_config.src_addr, len, DMA_MEM_TO_DEV);
    }
    else
    {
        hal_dma_prep_device(dma_chan, slave_config.dst_addr, slave_config.src_addr, len, DMA_DEV_TO_MEM);
    }

    //hal_dma_cyclic_callback_install(dma_chan, twi_dma_callback, twi);
    hal_dma_start(dma_chan);

    hal_sem_ret = hal_sem_timedwait(twi->dma_complete, twi->timeout * 100);
    if (hal_sem_ret != 0)
    {
        TWI_ERR("[twi%d] twi dma driver xfer timeout (dev addr:0x%x)\n", twi->port, twi->msgs->addr);
	return -1;
    }
    TWI_INFO("[twi%d] twi driver dma xfer success.\n", twi->port);
    TWI_ERR("[twi%d] twi driver dma xfer success.\n", twi->port);
    return 0;
}
#endif

static int twi_write(hal_twi_t *twi, twi_msg_t *msgs)
{
    twi->msgs = msgs;
	int ret;

    twi_slave_addr(twi->base_addr, msgs);
    if (msgs->len == 1)
    {
        twi_set_packet_addr_byte(0, twi->base_addr);
        twi_set_packet_data_byte(msgs->len, twi->base_addr);
    }
    else
    {
        twi_set_packet_addr_byte(1, twi->base_addr);
        twi_set_packet_data_byte(msgs->len - 1, twi->base_addr);
    }
    twi_set_packet_cnt(1, twi->base_addr);

    twi_clear_pending(twi->base_addr);

	twi_set_tx_trig_level(msgs->len, twi->base_addr);

	ret = twi_send_msgs(twi, msgs);
	if (ret != SUNXI_TWI_OK)
		return ret;

	twi_start_xfer(twi->base_addr);
	twi_enable_tran_irq(TRAN_COM_INT | TRAN_ERR_INT, twi->base_addr);

	return 0;
}

#ifdef CONFIG_DRIVERS_DMA
static int32_t twi_dma_write(hal_twi_t *twi, twi_msg_t *msgs)
{
    int32_t ret = 0;
    const uint32_t base_addr = twi->base_addr;
    twi->msgs = msgs;

    twi_slave_addr(base_addr, msgs);
    twi_set_packet_addr_byte(1, base_addr);
    twi_set_packet_data_byte(msgs->len - 1, base_addr);
    twi_set_packet_cnt(1, base_addr);

    twi_clear_pending(base_addr);
    twi_enable_tran_irq(TRAN_COM_INT | TRAN_ERR_INT, base_addr);
    twi_enable_dma_irq(DMA_TX, base_addr);
    twi_start_xfer(base_addr);

    ret = twi_dma_xfer(twi, (char *)msgs->buf, msgs->len, DMA_MEM_TO_DEV);

    return ret;
}
#else
static int32_t twi_dma_write(hal_twi_t *twi, twi_msg_t *msgs)
{
	return -1;
}
#endif

static int twi_read(hal_twi_t *twi, twi_msg_t *msgs, int32_t num)
{
    twi_msg_t *wmsgs = NULL, *rmsgs = NULL;

    if (num == 1)
    {
        wmsgs = NULL;
        rmsgs = msgs;
    }
    else if (num == 2)
    {
        wmsgs = msgs;
        rmsgs = msgs + 1;
    }
    else
    {
        TWI_ERR("msg num err");
        return -1;
    }
    TWI_INFO("rmsgs->len : %d", rmsgs->len);

    twi->msgs = rmsgs;

    twi_slave_addr(twi->base_addr, rmsgs);
    twi_set_packet_cnt(1, twi->base_addr);
    twi_set_packet_data_byte(rmsgs->len, twi->base_addr);
    if (rmsgs->len > MAX_FIFO)
    {
        twi_set_rx_trig_level(MAX_FIFO, twi->base_addr);
    }
    else
    {
#if defined(FIX_ERR_COM_INT)
        /* When one of the following conditions is met:
         * 1. The number of data (in bytes) in RECV_FIFO reaches RX_TRIG;
         * 2. Packet read done and RECV_FIFO not empty.
         * If RX_REQ is enabled,  the rx-pending-bit will be set to 1 and the interrupt will be triggered;
         * If RX_REQ is disabled, the rx-pending-bit will be set to 1 but the interrupt will NOT be triggered.
         * so set the rx_trigger_level max to avoid the RX_REQ com before COM_REQ
         */
        twi_set_rx_trig_level(MAX_FIFO, twi->base_addr);
#else
        twi_set_rx_trig_level(rmsgs->len, twi->base_addr);
#endif
    }
    if (twi_query_rxfifo(twi->base_addr))
    {
        twi_clear_rxfifo(twi->base_addr);
    }

    twi_clear_pending(twi->base_addr);
#if defined(FIX_ERR_COM_INT)
    twi_enable_tran_irq(TRAN_COM_INT | TRAN_ERR_INT | RX_REQ_INT, twi->base_addr);
#else
    twi_enable_tran_irq(TRAN_COM_INT | TRAN_ERR_INT, twi->base_addr);
#endif
    twi_start_xfer(twi->base_addr);

    if (wmsgs)
    {
        return twi_send_msgs(twi, wmsgs);
    }

    return 0;
}

#ifdef CONFIG_DRIVERS_DMA
static int32_t twi_dma_read(hal_twi_t *twi, twi_msg_t *msgs, int32_t num)
{
    int32_t ret = 0;
    twi_msg_t *wmsgs, *rmsgs;
    const uint32_t base_addr = twi->base_addr;

    if (num == 1)
    {
        wmsgs = NULL;
        rmsgs = msgs;
    }
    else if (num == 2)
    {
        wmsgs = msgs;
        rmsgs = msgs + 1;
    }
    twi->msgs = rmsgs;

    twi_slave_addr(base_addr, rmsgs);
    twi_set_packet_data_byte(rmsgs->len, base_addr);
    twi_set_packet_cnt(1, base_addr);
    twi_set_rx_trig_level(MAX_FIFO / 2, base_addr);
    if (twi_query_rxfifo(base_addr))
    {
        twi_clear_rxfifo(base_addr);
    }

    twi_clear_pending(base_addr);
    twi_enable_tran_irq(TRAN_COM_INT | TRAN_ERR_INT, base_addr);
    twi_enable_dma_irq(DMA_RX, base_addr);
    twi_start_xfer(base_addr);
    if (wmsgs)
    {
        twi_send_msgs(twi, wmsgs);
    }

    ret = twi_dma_xfer(twi, (char *)rmsgs->buf, rmsgs->len, DMA_DEV_TO_MEM);

    return ret;
}
#else
static int32_t twi_dma_read(hal_twi_t *twi, twi_msg_t *msgs, int32_t num)
{
	return -1;
}
#endif
/*************************************** TWI DRV XFER REG CONTROL end****************************/

static int hal_twi_drv_complete(hal_twi_t *twi)
{
    int hal_sem_ret;
#if defined(FIX_ERR_COM_INT)
	uint32_t drv_sta;
	int tx_fifo_cnt;
#endif

#if defined(FIX_ERR_COM_INT)
    hal_sem_ret = hal_sem_timedwait(twi->hal_sem, twi->timeout);
#else
    hal_sem_ret = hal_sem_timedwait(twi->hal_sem, twi->timeout * 100);
#endif
    if (hal_sem_ret < 0)
    {
        twi_disable_tran_irq(TRAN_COM_INT | TRAN_ERR_INT
                             | RX_REQ_INT | TX_REQ_INT, twi->base_addr);
        twi_disable_dma_irq(DMA_TX | DMA_RX, twi->base_addr);
#if defined(FIX_ERR_COM_INT)
		if (!(twi->msgs->flags & TWI_M_RD)) { /* drv write */
			drv_sta = (readl(twi->base_addr + TWI_DRIVER_CTRL) & TWI_DRV_STA) >> 16;
			tx_fifo_cnt = twi_query_txfifo(twi->base_addr);
			if ((drv_sta != 0xf8) || (tx_fifo_cnt != 0)) {
				TWI_ERR_REPORT(twi->port, twi_query_irq_status(twi->base_addr));
				TWI_ERR("[twi%d] twi driver xfer timeout (dev addr:0x%x)", twi->port, twi->msgs->addr);
				twi_dump_reg(twi, ALL_REG);
				return SUNXI_TWI_FAIL;
			}
		}
#endif
		TWI_ERR_REPORT(twi->port, twi_query_irq_status(twi->base_addr));
		TWI_ERR("[twi%d] twi driver xfer timeout (dev addr:0x%x)", twi->port, twi->msgs->addr);
		twi_dump_reg(twi, ALL_REG);
		return SUNXI_TWI_FAIL;
    }
    else if (twi->result == RESULT_ERR)
    {
		TWI_ERR_REPORT(twi->port, twi_query_irq_status(twi->base_addr));
        TWI_ERR("[twi%d]twi drv xfer incomplete xfer"
                "(status: 0x%u, dev addr: 0x%x)",
                twi->port, twi->msgs_idx, twi->msgs->addr);
        twi_disable_tran_irq(TRAN_COM_INT | TRAN_ERR_INT
                             | RX_REQ_INT | TX_REQ_INT, twi->base_addr);
        twi_disable_dma_irq(DMA_TX | DMA_RX, twi->base_addr);
        return SUNXI_TWI_FAIL;
    }

    TWI_INFO("twi drv xfer complete");

    //  twin_lock_irqsave(&twi->lock, flags);
    twi->result = RESULT_COMPLETE;
    //  twin_unlock_irqrestore(&twi->lock, flags);

    return SUNXI_TWI_OK;
}

static int hal_twi_engine_complete(hal_twi_t *twi, int code)
{
    int ret = SUNXI_TWI_OK;
    int hal_sem_ret;

    twi->msgs     = NULL;
    twi->msgs_num = 0;
    twi->msgs_ptr = 0;
    twi->status  = TWI_XFER_IDLE;

    /* twi->msgs_idx  store the information */
    if (code == SUNXI_TWI_FAIL)
    {
        TWI_ERR("[twi%d] Maybe Logic Error, debug it!", twi->port);
        twi->msgs_idx = code;
        ret = SUNXI_TWI_FAIL;
        twi->result = RESULT_ERR;
    }
    else if (code != SUNXI_TWI_OK)
    {
        twi->msgs_idx = code;
        ret = SUNXI_TWI_FAIL;
        twi->result = RESULT_ERR;
    }

    hal_sem_ret = hal_sem_post(twi->hal_sem);
    if (hal_sem_ret != 0)
    {
        ret = SUNXI_TWI_FAIL;
        TWI_ERR(" evdev give hal_semaphore err");
    }

    TWI_INFO("code=%d, complete", twi->msgs_idx);

    return ret;
}

/*
 ****************************************************************************
 *
 *  FunctionName:           hal_i2c_addr_byte
 *
 *  Description:
 *         7bits addr: 7-1bits addr+0 bit r/w
 *         10bits addr: 1111_11xx_xxxx_xxxx-->1111_0xx_rw,xxxx_xxxx
 *         send the 7 bits addr,or the first part of 10 bits addr
 *  Parameters:
 *
 *
 *  Return value:
 *           ??
 *  Notes:
 *
 ****************************************************************************
 */
static void hal_twi_addr_byte(hal_twi_t *twi)
{
    unsigned char addr = 0;
    unsigned char tmp  = 0;

    if (twi->msgs[twi->msgs_idx].flags & TWI_M_TEN)
    {
        /* 0111_10xx,ten bits address--9:8bits */
        tmp = 0x78 | (((twi->msgs[twi->msgs_idx].addr) >> 8) & 0x03);
        addr = tmp << 1;    /*1111_0xx0*/
        /* how about the second part of ten bits addr? */
        /* Answer: deal at twi_core_process() */
    }
    else
    {
        addr = (twi->msgs[twi->msgs_idx].addr & 0x7f) << 1;
    }/* 7-1bits addr, xxxx_xxx0 */

    /* read, default value is write */
    if (twi->msgs[twi->msgs_idx].flags & TWI_M_RD)
    {
        addr |= 1;
    }

    if (twi->msgs[twi->msgs_idx].flags & TWI_M_TEN)
    {
        TWI_INFO("[twi%d] first part of 10bits = 0x%x",
                 twi->port, addr);
    }
    else
    {
        TWI_INFO("[twi%d] 7bits+r/w = 0x%x", twi->port, addr);
    }

    /* send 7bits+r/w or the first part of 10bits */
    twi_put_byte(twi->base_addr, &addr);
}

static int hal_twi_core_process(hal_twi_t *twi)
{
    const uint32_t base_addr = twi->base_addr;
    int  ret        = SUNXI_TWI_OK;
    int  err_code   = 0;
    unsigned char  state = 0;
    unsigned char  tmp   = 0;

    state = twi_query_irq_status(base_addr);

    //twin_lock_irqsave(&twi->lock, flags);
    if (twi->msgs == NULL)
    {
        TWI_ERR("[twi%d] twi message is NULL, err_code = 0xfe",
                twi->port);
        err_code = 0xfe;
        goto msg_null;
    }
    TWI_INFO("[twi%d][slave address = (0x%x), state = (0x%x)]",
             twi->port, twi->msgs->addr, state);

    switch (state)
    {
        case 0xf8:
            /* On reset or stop the bus is idle, use only at poll method */
            err_code = 0xf8;
            goto err_out;
        case 0x08: /* A START condition has been transmitted */
        case 0x10: /* A repeated start condition has been transmitted */
            hal_twi_addr_byte(twi);/* send slave address */
            break;
        case 0xd8: /* second addr has transmitted, ACK not received!    */
        case 0x20: /* SLA+W has been transmitted; NOT ACK has been received */
            err_code = 0x20;
            goto err_out;
        case 0x18: /* SLA+W has been transmitted; ACK has been received */
            /* if any, send second part of 10 bits addr */
            if (twi->msgs[twi->msgs_idx].flags & TWI_M_TEN)
            {
                /* the remaining 8 bits of address */
                tmp = twi->msgs[twi->msgs_idx].addr & 0xff;
                twi_put_byte(base_addr, &tmp); /* case 0xd0: */
                break;
            }
            goto send_data;
        /* for 7 bit addr, then directly send data byte--case 0xd0:  */
        case 0xd0: /* second addr has transmitted,ACK received!     */
        case 0x28: /* Data byte in DATA REG has been transmitted; */
            /*  ACK has been received */
            /* after send register address then START send write data  */
send_data:
            if (twi->msgs_ptr < twi->msgs[twi->msgs_idx].len)
            {
                twi_put_byte(base_addr,
                             &(twi->msgs[twi->msgs_idx].buf[twi->msgs_ptr]));
                twi->msgs_ptr++;
                break;
            }

            twi->msgs_idx++; /* the other msg */
            twi->msgs_ptr = 0;
            if (twi->msgs_idx == twi->msgs_num)
            {
                err_code = SUNXI_TWI_OK;/* Success,wakeup */
                goto ok_out;
            }
            else if (twi->msgs_idx < twi->msgs_num)
            {
                /* for restart pattern, read spec, two msgs */
                ret = twi_restart(base_addr, twi->port);
                if (ret == SUNXI_TWI_FAIL)
                {
                    TWI_ERR("[twi%d] twi restart fail", twi->port);
                    err_code = SUNXI_TWI_FAIL;
                    goto err_out;/* START can't sendout */
                }
            }
            else
            {
                err_code = SUNXI_TWI_FAIL;
                goto err_out;
            }
            break;
        case 0x30: /* Data byte in TWIDAT has been transmitted; */
            /* NOT ACK has been received */
            err_code = 0x30;    /*err,wakeup the thread*/
            goto err_out;
        case 0x38: /* Arbitration lost during SLA+W, SLA+R or data bytes */
            err_code = 0x38;    /*err,wakeup the thread*/
            goto err_out;
        case 0x40: /* SLA+R has been transmitted; ACK has been received */
            /* with Restart,needn't to send second part of 10 bits addr */
            /* refer-"TWI-SPEC v2.1" */
            /* enable A_ACK need it(receive data len) more than 1. */
            if (twi->msgs[twi->msgs_idx].len > 1)
            {
                /* send register addr complete,then enable the A_ACK */
                /* and get ready for receiving data */
                twi_enable_ack(base_addr);
                twi_clear_irq_flag(base_addr);/* jump to case 0x50 */
            }
            else if (twi->msgs[twi->msgs_idx].len == 1)
            {
                twi_clear_irq_flag(base_addr);/* jump to case 0x58 */
            }
            break;
        case 0x48: /* SLA+R has been transmitted; NOT ACK has been received */
            err_code = 0x48;    /*err,wakeup the thread*/
            goto err_out;
        case 0x50: /* Data bytes has been received; ACK has been transmitted */
            /* receive first data byte */
            if (twi->msgs_ptr < twi->msgs[twi->msgs_idx].len)
            {
                /* more than 2 bytes, the last byte need not to send ACK */
                if ((twi->msgs_ptr + 2) == twi->msgs[twi->msgs_idx].len)
                    /* last byte no ACK */
                {
                    twi_disable_ack(base_addr);
                }

                /* get data then clear flag,then next data coming */
                twi_get_byte(base_addr,
                             &twi->msgs[twi->msgs_idx].buf[twi->msgs_ptr]);
                twi->msgs_ptr++;
                break;
            }
            /* err process, the last byte should be @case 0x58 */
            err_code = SUNXI_TWI_FAIL;/* err, wakeup */
            goto err_out;
        case 0x58:
            /* Data byte has been received; NOT ACK has been transmitted */
            /* received the last byte  */
            if (twi->msgs_ptr == twi->msgs[twi->msgs_idx].len - 1)
            {
                twi_get_last_byte(base_addr,
                                  &twi->msgs[twi->msgs_idx].buf[twi->msgs_ptr]);
                twi->msgs_idx++;
                twi->msgs_ptr = 0;
                if (twi->msgs_idx == twi->msgs_num)
                {
                    /* succeed,wakeup the thread */
                    err_code = SUNXI_TWI_OK;
                    goto ok_out;
                }
                else if (twi->msgs_idx < twi->msgs_num)
                {
                    /* repeat start */
                    ret = twi_restart(base_addr, twi->port);
                    if (ret == SUNXI_TWI_FAIL)  /* START fail */
                    {
                        TWI_ERR("[twi%d] twi restart fail", twi->port);
                        err_code = SUNXI_TWI_FAIL;
                        goto err_out;
                    }
                    break;
                }
            }
            else
            {
                err_code = 0x58;
                goto err_out;
            }
            break;
        case 0x00: /* Bus error during master or slave mode due to illegal level condition */
            err_code = 0xff;
            goto err_out;
        default:
            err_code = state;
            goto err_out;
    }
    //twin_unlock_irqrestore(&twi->lock, flags);
    return ret;

ok_out:
err_out:
    if (twi_stop(base_addr, twi->port) == SUNXI_TWI_FAIL)
    {
        TWI_ERR("[twi%d] STOP failed!", twi->port);
    }

msg_null:
    ret = hal_twi_engine_complete(twi, err_code);/* wake up */
    //twin_unlock_irqrestore(&twi->lock, flags);
    return ret;
}

static inline const char *twi_status_to_str(uint32_t code)
{
	switch (code) {
	case 0x00: return "Bus error";
	case 0x08: return "START condition transmitted";
	case 0x10: return "Repeated START condition transmitted";
	case 0x18: return "Address + Write bit transmitted, ACK received";
	case 0x20: return "Address + Write bit transmitted, ACK not received";
	case 0x28: return "Data byte transmitted in master mode, ACK received";
	case 0x30: return "Data byte transmitted in master mode, ACK not received";
	case 0x38: return "Arbitration lost in address or data byte";
	case 0x40: return "Address + Read bit transmitted, ACK received";
	case 0x48: return "Address + Read bit transmitted, ACK not received";
	case 0x50: return "Data byte received in master mode, ACK received";
	case 0x58: return "Data byte received in master mode, ACK not received";
	case 0x60: return "Slave address + Write bit received, ACK transmitted";
	case 0x68: return "Arbitration lost in address as master, slave address + Write bit received, ACK transmitted";
	case 0x70: return "General Call address received, ACK transmitted";
	case 0x78: return "Arbitration lost in address as master, General Call address received, ACK transmitted";
	case 0x80: return "Data byte received after slave addres received, ACK transmitted";
	case 0x88: return "Data byte received after slave addres received, not ACK transmitted";
	case 0x90: return "Data byte received after General Call received, ACK transmitted";
	case 0x98: return "Data byte received after General Call received, not ACK transmitted";
	case 0xa0: return "STOP or Repeated START condition received in slave mode";
	case 0xa8: return "Slave address + Read bit received, ACK transmitted";
	case 0xb0: return "Arbitration lost in address as master, slave address + Read bit received, ACK transmitted";
	case 0xb8: return "Data byte transmitted in slave mode, ACK received";
	case 0xc0: return "Data byte transmitted in slave mode, ACK not received";
	case 0xc8: return "Last byte transmitted in slave mode, ACK received";
	case 0xd0: return "Second address byte + Write bit transmitted, ACK received";
	case 0xd8: return "Second address byte + Write bit transmitted, ACK not received";
	case 0xf8: return "No relevant status infomation, INT_FLAG=0";
	case 0xf9: return "Normal transmission";
	default: return "unknown status";
	}
}

static inline const char *twi_drv_get_code_str(uint32_t code)
{
	switch (code) {
	case 0x00: return "bus error";
	case 0x01: return "timeout when sending 9th SCL clk";
	case 0x20: return "Address + Write bit transmitted, ACK not received";
	case 0x30: return "Data byte transmitted in master mode, ACK not received";
	case 0x38: return "Arbitration lost in address or data byte";
	case 0x48: return "Address + Read bit transmitted, ACK not received";
	case 0x58: return "Data byte received in master mode, ACK not received";
	default: return "unknown error";
	}
}

static twi_status_t twi_bus_reset(hal_twi_t *twi)
{
	const unsigned long base_addr = twi->base_addr;
	twi_status_t ret;
	unsigned int status;

	/* test the bus is free */
	if (twi_query_irq_status(base_addr) == TWI_STAT_IDLE ||
	    twi_query_irq_status(base_addr) == TWI_STAT_BUS_ERR ||
	    twi_query_irq_status(base_addr) == TWI_STAT_ARBLOST_SLAR_ACK)
		return SUNXI_TWI_OK;

	ret = twi_send_clk_9pulse(twi);
	status = twi_query_irq_status(base_addr);

	if (ret != SUNXI_TWI_OK) {
		TWI_ERR("[twi%d] bus is busy, status = %x", twi->port, status);
		return SUNXI_TWI_RETRY;
	}

	TWI_ERR("[twi%d] bus reset, status = %x", twi->port, status);
	return SUNXI_TWI_OK;
}

static int hal_twi_slave_core_process(hal_twi_t *twi)
{
    uint32_t status;
    uint8_t value;
    int timeout = 0x7ffff;
    const twi_line_reg *reg = &engine_line_reg;

    status = twi_query_irq_status(twi->base_addr);

    if (status == TWI_STAT_RXRS_ACK || status == TWI_STAT_SLV_TXD_ACK) {
        /* wait scl into second half cycle otherwise the 9th ack clk may shorter than normal */
        while (twi_get_scl(twi->base_addr, reg) && --timeout)
            ;
        if (timeout <= 0)
            TWI_ERR("[twi%d] slave wait scl low timeout!\n", twi->port);

        twi_set_scl(twi->base_addr, reg, 0);
		twi_scl_control(twi->base_addr, reg, 1);
    }

    switch (status) {
    case 0x60: /* Slave address + Write bit received, ACK transmitted */
        twi->slave_cb(twi->port, I2C_SLAVE_WRITE_REQUESTED, &value);
        twi->status = TWI_XFER_SLV_SADDR;
        break;
    case 0x80: /* Data byte received after slave address received, ACK transmitted */
        if (twi->status == TWI_XFER_SLV_SADDR) {
			twi_get_byte(twi->base_addr, &value);
			twi->slave_cb(twi->port, I2C_SLAVE_WRITE_RECEIVED, &value);
			twi->status = TWI_XFER_SLV_WDATA;
		} else if (twi->status == TWI_XFER_SLV_WDATA) {
			twi_get_byte(twi->base_addr, &value);
			twi->slave_cb(twi->port, I2C_SLAVE_WRITE_RECEIVED, &value);
		} else {
			twi->status = TWI_XFER_SLV_ERROR;
		}
		break;
    case 0xa0: /* STOP or repeated START condition received in slave mode */
        twi->slave_cb(twi->port, I2C_SLAVE_STOP, &value);
		twi->status = TWI_XFER_SLV_IDLE;
        break;
    case 0xa8: /* Slave address + Read bit received, ACK transmitted */
        twi->slave_cb(twi->port, I2C_SLAVE_READ_REQUESTED, &value);
		twi_put_byte(twi->base_addr, &value);
		twi->status = TWI_XFER_SLV_RDATA;
        break;
    case 0xb8: /* Data byte transmitted in slave mode, ACK received */
        twi->slave_cb(twi->port, I2C_SLAVE_READ_PROCESSED, &value);
		twi_put_byte(twi->base_addr, &value);
		twi->status = TWI_XFER_SLV_RDATA;
        break;
    case 0xc0: /* Data byte transmitted in slave mode, ACK not received */
        twi->slave_cb(twi->port, I2C_SLAVE_READ_REQUESTED, &value);
		twi_put_byte(twi->base_addr, &value);
		twi->slave_cb(twi->port, I2C_SLAVE_STOP, &value);
		twi->status = TWI_XFER_SLV_IDLE;
        break;
    default:
        TWI_ERR("[twi%d] slave addr(%#x) error irq(0x%#x) status(%#x), need reset\n", twi->port, twi->slave_addr, status, twi->status);
        twi->status = TWI_XFER_SLV_ERROR;
    }

    if (twi->status == TWI_XFER_SLV_ERROR)
        twi_slave_reset(twi);

    twi_clear_irq_flag(twi->base_addr);

    if (status == TWI_STAT_RXRS_ACK || status == TWI_STAT_SLV_TXD_ACK) {
        /* delay 1us to fix the sda/scl reversal at same time issue */
        hal_udelay(1);
        twi_scl_control(twi->base_addr, reg, 0);
    }

    return 0;
}

static int hal_twi_drv_core_process(hal_twi_t *twi)
{
	int ret = SUNXI_TWI_OK;
	uint32_t status, engine_status = 0;
	int hal_sem_ret;

	// twin_lock_irqsave(&twi->lock, flags);

	status = twi_drv_query_enabled_irq_status(twi->base_addr);
	if (!status) {
		/* Not our interrupt */
		goto ignore_out;
	}

	if (status & TRAN_ERR_PD)
	{
		TWI_ERR_REPORT(twi->port, twi_query_irq_status(twi->base_addr));
		TWI_ERR("[twi%d] twi drv error", twi->port);
		twi_dump_reg(twi, COM_REG | DRV_REG);
		twi_disable_tran_irq(TWI_DRV_INT_MASK, twi->base_addr);
#if 0
		code = readl(twi->base_addr + TWI_DRIVER_CTRL);
		code = (code & TWI_DRV_STA) >> 16;
		TWI_ERR("[twi%d] err code: 0x%x(%s)", twi->port, code, twi_drv_get_code_str(code));
#else
		engine_status = twi_query_irq_status(twi->base_addr);
		TWI_ERR("[twi%d] curent bus real status is: 0x%x(%s)", twi->port, engine_status, twi_status_to_str(engine_status));
		if(engine_status != 0x20)
			twi_dump_reg(twi, COM_REG | DRV_REG);
#endif
		goto err_out;
	}

#if defined(FIX_ERR_COM_INT)
	if (status & TRAN_COM_PD) {
		TWI_INFO("[twi%d] twi drv complete", twi->port);

		/* for read, if only TRAN_COM_PD occur, we ignore it, wait RX_REQ_PD to read msgs */
		if ((twi->msgs->flags & TWI_M_RD) && (twi->msgs->len < DMA_THRESHOLD) && (status & RX_REQ_PD)) {
			twi_disable_tran_irq(TRAN_COM_INT, twi->base_addr); /* only disabled TRAN_COM_INT irq en */
			goto ignore_out;
		}
		/* for write, it is sucess to write */
		twi_disable_tran_irq(TWI_DRV_INT_MASK, twi->base_addr); /* diabled all irq en */
		twi->result = RESULT_COMPLETE;
		goto ok_out;
	}

	/* only trigger after trans end & rRV_FIFO_CNT > 0
	 * (rx_trigger set to be 32, so RX_REQ_PENDING will not PENDING before trans complete)
	 */
	if (status & RX_REQ_PD) {
		TWI_INFO("[twi%d] twi RX_REQ pending", twi->port);
		twi_disable_tran_irq(TWI_DRV_INT_MASK, twi->base_addr); /* diabled all irq en */
#else
	if (status & RX_REQ_PD) {
		// nobody cares yet
		twi_disable_tran_irq(RX_REQ_INT, twi->base_addr);
		goto ignore_out;
	}

	if (status & TRAN_COM_PD) {
		TWI_INFO("[twi%d] twi drv complete", twi->port);
		twi_disable_tran_irq(TRAN_COM_INT, twi->base_addr);
#endif

		/* for recv msgs without dma */
		if ((twi->msgs->flags & TWI_M_RD) && (twi->msgs->len < DMA_THRESHOLD)) {
			ret = twi_recv_msgs(twi, twi->msgs);
			if (twi->msgs->len != ret) {
				TWI_ERR_REPORT(twi->port, twi_query_irq_status(twi->base_addr));
				TWI_ERR("[twi%d] drv recv complete, but recv len error: %d != %d",
					twi->port, twi->msgs->len, ret);
				ret = SUNXI_TWI_FAIL;
				twi->result = RESULT_ERR;
				goto err_out;
			}
		}
		twi->result = RESULT_COMPLETE;
		goto ok_out;
	}

	if (status & TX_REQ_PD) {
		// nobody cares yet
		twi_disable_tran_irq(TX_REQ_INT, twi->base_addr);
		goto ignore_out;
	}

err_out:
	twi->msgs_idx = engine_status;
	twi->result = RESULT_ERR;
	TWI_ERR("[twi%d] packet transmission failed, drv irq pending is: 0x%x", twi->port, status);
ok_out:
	//wake up
	hal_sem_ret = hal_sem_post(twi->hal_sem);
	if (hal_sem_ret != 0) {
		ret = SUNXI_TWI_FAIL;
		TWI_ERR(" evdev give hal_semaphore err");
	}

	/* Cleaning up interrupts will trigger a new round of the transmission state machine */
	twi_drv_clear_irq_flag(status, twi->base_addr);
	return ret;
	//  twin_unlock_irqrestore(&twi->lock, flags);
ignore_out:
	twi_drv_clear_irq_flag(status, twi->base_addr);
	return 0;
}

static hal_irqreturn_t hal_twi_handler(void *dev)
{
    hal_twi_t *twi = (hal_twi_t *)dev;

    if (twi->twi_slave_used)
    {
        hal_twi_slave_core_process(twi);
    }
    else if (twi->twi_drv_used)
    {
        hal_twi_drv_core_process(twi);
    }
    else
    {
        if (!twi_query_irq_flag(twi->base_addr))
        {
            /*
             * Group interrupt will execute all handles, if the interrupt does
             * not belong to this twi, return 1
             */
            return 1;
        }

        /* disable irq */
        twi_disable_irq(twi->base_addr);

        /* twi core process */
        hal_twi_core_process(twi);

        /*
         * enable irq only when twi is transferring,
         * otherwise disable irq
         */
        if (twi->status == TWI_XFER_RUNNING)
        {
            twi_enable_irq(twi->base_addr);
        }
    }
    return 0;
}

/**
 * twi_do_xfer - twi driver transmission control
 */
static int hal_twi_drv_do_xfer(hal_twi_t *twi, struct twi_msg *msgs, int num)
{
    //  uint64_t flags = 0;
    int ret = -1;

    //  twin_lock_irqsave(&twi->lock, flags);
    //  twi->result = 0;
    //  twin_unlock_irqrestore(&twi->lock, flags);

    twi_clear_pending(twi->base_addr);
    twi_disable_tran_irq(TRAN_COM_INT | TRAN_ERR_INT
                         | RX_REQ_INT | TX_REQ_INT, twi->base_addr);
    twi_disable_dma_irq(DMA_TX | DMA_RX, twi->base_addr);
    if (twi_query_txfifo(twi->base_addr))
    {
        twi_clear_txfifo(twi->base_addr);
    }

    if (num == 1)
    {
        if (msgs->flags & TWI_M_RD)
        {
            TWI_INFO("1 msgs read ");
            /* 1 msgs read */
            twi_enable_read_tran_mode(twi->base_addr);
            twi_set_packet_addr_byte(0, twi->base_addr);
            if (twi->dma_chan && (msgs->len >= DMA_THRESHOLD))
            {
                TWI_INFO("twi[%d] master dma read", twi->port);
                ret =  twi_dma_read(twi, msgs, num);
            }
            else
            {
                TWI_INFO("twi[%d] master cpu read", twi->port);
                ret = twi_read(twi, msgs, num);
            }
        }
        else
        {
            /* 1 msgs write */
            twi_disable_read_tran_mode(twi->base_addr);
            if (twi->dma_chan && (msgs->len >= DMA_THRESHOLD))
            {
                TWI_INFO("twi[%d] master dma write\n", twi->port);
                ret = twi_dma_write(twi, msgs);
            }
            else
            {
                TWI_INFO("twi[%d] master cpu write\n", twi->port);
                ret = twi_write(twi, msgs);
            }
        }
    }
    else if ((num == 2) && ((msgs + 1)->flags & TWI_M_RD))
    {
        /* 2 msgs read */
        TWI_INFO("2 msgs read");
        twi_disable_read_tran_mode(twi->base_addr);
        twi_set_packet_addr_byte(msgs->len, twi->base_addr);
        if (twi->dma_chan && ((msgs + 1)->len >= DMA_THRESHOLD))
        {
            TWI_INFO("twi[%d] master dma read\n", twi->port);
            ret =  twi_dma_read(twi, msgs, num);
        }
        else
        {
            TWI_INFO("twi[%d] master cpu read\n", twi->port);
            ret = twi_read(twi, msgs, num);
        }
    }

    if (ret)
    {
		TWI_ERR_REPORT(twi->port, twi_query_irq_status(twi->base_addr));
        TWI_ERR("[twi%d] %s ret: %d", twi->port, __func__, ret);
        return ret;
    }

	ret = hal_twi_drv_complete(twi);
	if (ret != SUNXI_TWI_OK)
		return ret;

    return num;
}

static int hal_twi_engine_do_xfer(hal_twi_t *twi, twi_msg_t *msgs, int num)
{
    int ret;

    const uint32_t base_addr = twi->base_addr;
    int hal_sem_ret;

    /* may conflict with xfer_complete */
    //twin_lock_irqsave(&twi->lock, flags);
    twi->msgs    = msgs;
    twi->msgs_num = num;
    twi->msgs_ptr = 0;
    twi->msgs_idx = 0;
    twi_disable_ack(base_addr); /* disabe ACK */
    /* set the special function register,default:0. */
    twi_set_efr(base_addr, 0);
    //twin_unlock_irqrestore(&twi->lock, flags);

    /* START signal, needn't clear int flag */
    twi->status  = TWI_XFER_START;
    ret = twi_start(base_addr, twi->port);
    if (ret == SUNXI_TWI_FAIL)
    {
        TWI_ERR("[twi%d] twi start fail", twi->port);
        twi_soft_reset(twi);
        twi_disable_irq(base_addr);  /* disable irq */
        ret = SUNXI_TWI_RETRY;
        goto out;
    }

    twi->status  = TWI_XFER_RUNNING;
    /*
     * Turning on the interrupt after sending the start signal is to ensure that
     * the execution of the twi_start function is not interrupted by the interrupt.
     */
    twi_enable_irq(base_addr);  /* enable irq */
    /* sleep and wait,do the transfer at interrupt handler,timeout = 5*HZ */
    hal_sem_ret = hal_sem_timedwait(twi->hal_sem, twi->timeout * 100);
    /* return code,if(msgs_idx == num) succeed */
    ret = twi->msgs_idx;
    if (hal_sem_ret != 0)
    {
        TWI_ERR("[twi%d] xfer timeout (dev addr:0x%x)",
                twi->port, msgs->addr);
        //twin_lock_irqsave(&twi->lock, flags);
        twi->msgs = NULL;
        //twin_unlock_irqrestore(&twi->lock, flags);
        ret = SUNXI_TWI_FAIL;
	goto fail;
    }
    else if (ret != num)
    {
        TWI_ERR("[twi%d] incomplete xfer (status: 0x%x, dev addr: 0x%x)",
                twi->port, ret, msgs->addr);
        ret = SUNXI_TWI_FAIL;
	goto fail;
    }

    TWI_INFO("hal_twi_engine_do_xfer end");
out:
    return ret;
fail:
	twi_soft_reset(twi);
    twi_disable_irq(base_addr);
    return ret;
}

inner_attr void twi_drv_hw_reset(hal_twi_t *twi);
twi_status_t hal_twi_xfer_nolock(twi_port_t port, twi_msg_t *msgs, int32_t num)
{
    hal_twi_t *twi = &hal_twi[port];
    int ret;

    if (twi->twi_slave_used) {
        TWI_ERR("twi bus is in slave mode and shouldn't be using in master anymore\n");
        return TWI_STATUS_ERROR_BUSY;
    }

    if ((msgs == NULL) || (num <= 0))
    {
		TWI_ERR_REPORT(twi->port, 0);
        TWI_ERR("[twi%d] invalid argument", port);
        return TWI_STATUS_INVALID_PARAMETER;
    }

#ifdef CONFIG_COMPONENTS_PM
    pm_wakelocks_acquire(&twi->wl, PM_WL_TYPE_WAIT_INC, OS_WAIT_FOREVER);
#endif

    twi_soft_reset(twi);
    ret = twi_bus_reset(twi);
    if (ret) {
		TWI_ERR_REPORT(twi->port, twi_query_irq_status(twi->base_addr));
        TWI_ERR("[twi%d] bus reset failed", twi->port);
        goto end;
    }

    if (twi->twi_drv_used)
    {
#if defined(FIX_DRV_EN_LOSS)
		twi_drv_hw_reset(twi);
#endif
        TWI_INFO("[twi%d] twi driver xfer", twi->port);
        ret = hal_twi_drv_do_xfer(twi, msgs, num);
        if (ret < 0)
        {
            ret = TWI_STATUS_ERROR;
            goto end;
        }
    }
    else
    {
        TWI_INFO("[twi%d] twi engine xfer", twi->port);
        ret = hal_twi_engine_do_xfer(twi, msgs, num);
        if (ret < 0)
        {
            ret = TWI_STATUS_ERROR;
            goto end;
        }
    }

end:
#ifdef CONFIG_COMPONENTS_PM
    pm_wakelocks_release(&twi->wl);
#endif
    return ret == num ? TWI_STATUS_OK : TWI_STATUS_ERROR;
}

twi_status_t hal_twi_xfer(twi_port_t port, twi_msg_t *msgs, int32_t num)
{
    twi_status_t status;
    hal_twi_t *twi = &hal_twi[port];

    hal_mutex_lock(twi->lock);
    status = hal_twi_xfer_nolock(port, msgs, num);
    hal_mutex_unlock(twi->lock);

    return status;
}

#ifdef CONFIG_DRIVER_SYSCONFIG
#define TWI_MAIN_KEY_PREFIX	"twi"
static twi_status_t hal_twi_get_resource_from_sys_config(hal_twi_t *twi)
{
	int32_t val;
	char mian_key[20] = {0};
	char sub_key[20] = {0};

	snprintf(mian_key, sizeof(mian_key), "%s%u%c", TWI_MAIN_KEY_PREFIX, twi->port, '\0');

	snprintf(sub_key, sizeof(sub_key), "%s%c", "twi_drv_used", '\0');
	if (hal_cfg_get_keyvalue(mian_key, sub_key, (int32_t *)&val, 1)) {
		TWI_ERR("[twi%d] not found in sysconfig: %s-%s\n", twi->port, mian_key, sub_key);
		return TWI_STATUS_ERROR;
	}
	if (val == 1) {
		twi->twi_drv_used = TWI_DRV_XFER;
		TWI_INFO("[twi%d] use drv mode\n", twi->port);
	}

	return TWI_STATUS_OK;
}
#endif

static twi_status_t hal_twi_get_resource_from_build_in_config(hal_twi_t *twi)
{
	twi->twi_drv_used = twi_mode_table[twi->port];

	return TWI_STATUS_OK;
}

static twi_status_t hal_twi_get_resource(hal_twi_t *twi)
{
	twi_status_t ret;

	// default config
        twi->twi_drv_used = ENGINE_XFER;

#ifdef CONFIG_DRIVER_SYSCONFIG
	ret = hal_twi_get_resource_from_sys_config(twi);
	if (ret == TWI_STATUS_OK)
		return TWI_STATUS_OK;
	TWI_ERR("[twi%d] try build in config\n", twi->port);
#endif
	ret = hal_twi_get_resource_from_build_in_config(twi);
	if (ret == TWI_STATUS_OK)
		return TWI_STATUS_OK;

	TWI_ERR("[twi%d] get resource failed!\n", twi->port);
	return ret;
}

static twi_status_t hal_twi_sys_pinctrl_init(hal_twi_t *twi)
{
#ifdef CONFIG_DRIVER_SYSCONFIG
    user_gpio_set_t gpio_cfg[2] = {0};
    int count, i;
    char twi_name[16];
    int ret = TWI_STATUS_OK;

    sprintf(twi_name, "twi%d", twi->port);
    count = hal_cfg_get_gpiosec_keycount(twi_name);
    if (!count)
    {
	TWI_WARN("[twi%d] not support in sys_config\n", twi->port);
        return TWI_STATUS_ERROR;
    }
    hal_cfg_get_gpiosec_data(twi_name, gpio_cfg, count);

    for (i = 0; i < count; i++) {
		twi->pin[i] = (gpio_cfg[i].port - 1) * 32 + gpio_cfg[i].port_num;
		twi->pinmux = gpio_cfg[i].mul_sel;
		ret = hal_gpio_pinmux_set_function(twi->pin[i], twi->pinmux);
		if (ret){
			TWI_ERR("[twi%d] PIN%u set function failed! return %d\n",
					twi->port, twi->pin[i], ret);
			return TWI_STATUS_ERROR;
		}

		ret = hal_gpio_set_driving_level(twi->pin[i], gpio_cfg[i].drv_level);
		if (ret) {
			TWI_ERR("[twi%d] PIN%u set driving level failed! return %d\n",
					twi->port, gpio_cfg[i].drv_level, ret);
			return TWI_STATUS_ERROR;
		}

		if (gpio_cfg[i].pull)
			if (hal_gpio_set_pull(twi->pin[i], gpio_cfg[i].pull))
				return TWI_STATUS_ERROR;
	}

	return TWI_STATUS_OK;
#else
	TWI_ERR("twi[%d] not support sys_config format \n", twi->port);
	return TWI_STATUS_ERROR;
#endif
}

static twi_status_t hal_twi_pinctrl_init(hal_twi_t *twi)
{
    uint8_t i;

    switch (twi->port)
    {
        case TWI_MASTER_0:
            twi->pin[0] = TWI0_SCK;
            twi->pin[1] = TWI0_SDA;
            twi->pinmux = TWI0_PIN_MUXSEL;
            break;
        case TWI_MASTER_1:
            twi->pin[0] = TWI1_SCK;
            twi->pin[1] = TWI1_SDA;
            twi->pinmux = TWI1_PIN_MUXSEL;
            break;
        case TWI_MASTER_2:
            twi->pin[0] = TWI2_SCK;
            twi->pin[1] = TWI2_SDA;
            twi->pinmux = TWI2_PIN_MUXSEL;
            break;
        case TWI_MASTER_3:
#if !(defined(CONFIG_ARCH_SUN300IW1))
            twi->pin[0] = TWI3_SCK;
            twi->pin[1] = TWI3_SDA;
            twi->pinmux = TWI3_PIN_MUXSEL;
#endif
            break;
	case TWI_MASTER_4:
#if (defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
            twi->pin[0] = TWI4_SCK;
            twi->pin[1] = TWI4_SDA;
            twi->pinmux = TWI4_PIN_MUXSEL;
#endif
            break;
	case TWI_MASTER_5:
# if (defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
            twi->pin[0] = TWI5_SCK;
            twi->pin[1] = TWI5_SDA;
            twi->pinmux = TWI5_PIN_MUXSEL;
#endif
            break;
	case TWI_MASTER_6:
# ifdef CONFIG_ARCH_SUN55IW6
            twi->pin[0] = TWI6_SCK;
            twi->pin[1] = TWI6_SDA;
            twi->pinmux = TWI6_PIN_MUXSEL;
#endif
            break;
        case S_TWI_MASTER_0:
#if !(defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN300IW1))
	    twi->pin[0] = S_TWI0_SCK;
	    twi->pin[1] = S_TWI0_SDA;
	    twi->pinmux = S_TWI0_PIN_MUXSEL;
#endif
	    break;
        case S_TWI_MASTER_1:
#if (defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
            twi->pin[0] = S_TWI1_SCK;
            twi->pin[1] = S_TWI1_SDA;
            twi->pinmux = S_TWI1_PIN_MUXSEL;
#endif
            break;
        case S_TWI_MASTER_2:
#ifdef CONFIG_ARCH_SUN55IW3
            twi->pin[0] = S_TWI2_SCK;
            twi->pin[1] = S_TWI2_SDA;
            twi->pinmux = S_TWI2_PIN_MUXSEL;
#endif
            break;
    }

    for (i = 0; i < TWI_PIN_NUM; i++)
    {
        if (hal_gpio_pinmux_set_function(twi->pin[i], twi->pinmux))
        {
            TWI_ERR("[twi%d] PIN set function failed!", twi->port);
            return TWI_STATUS_ERROR;
        }

        if (hal_gpio_set_driving_level(twi->pin[i], TWI_DRIVE_STATE))
        {
            TWI_ERR("[twi%d] PIN set driving failed!", twi->port);
            return TWI_STATUS_ERROR;
        }

        if (hal_gpio_set_pull(twi->pin[i], TWI_PULL_STATE))
        {
            TWI_ERR("[twi%d] PIN set driving failed!", twi->port);
            return TWI_STATUS_ERROR;
        }
    }

    return TWI_STATUS_OK;
}

static void hal_twi_pinctrl_exit(hal_twi_t *twi)
{
    uint8_t i;

    for (i = 0; i < TWI_PIN_NUM; i++)
    {
        hal_gpio_pinmux_set_function(twi->pin[i], GPIO_MUXSEL_DISABLED);
        hal_gpio_set_pull(twi->pin[i], GPIO_PULL_DOWN_DISABLED);
    }
}

// fix unused-function
__attribute__((__unused__)) static twi_status_t hal_twi_regulator_init(hal_twi_t *twi)
{
/*
    enum REGULATOR_TYPE_ENUM regulator_type = twi_regulator_type;
    enum REGULATOR_ID_ENUM regulator_id = twi_regulator_id[twi->port];

    int tar_vol = twi_vol[twi->port];
	int ret;

    if (regulator_id == AXP2101_ID_MAX)
    {
        TWI_INFO("[twi%d] needn't to set regulator", twi->port);
        return TWI_STATUS_OK;
    }

    hal_regulator_get(REGULATOR_GET(regulator_type, regulator_id), &twi->regulator);

    ret = hal_regulator_set_voltage(&twi->regulator, tar_vol);
    if (ret)
    {
        TWI_ERR("twi%d set voltage failed", twi->port);
        return TWI_STATUS_ERROR;
    }

    ret = hal_regulator_enable(&twi->regulator);
    if (ret)
    {
        TWI_ERR("twi%d enabled regulator failed", twi->port);
        return TWI_STATUS_ERROR;
    }
*/
    return TWI_STATUS_OK;
}

#if !defined(CONFIG_ARCH_DSP)
// fix unused-function
__attribute__((__unused__)) static twi_status_t hal_twi_regulator_exit(hal_twi_t *twi)
{
    //int ret;

    //enum REGULATOR_ID_ENUM regulator_id = twi_regulator_id[twi->port];
/*
    if (regulator_id == AXP2101_ID_MAX)
    {
        TWI_INFO("[twi%d] needn't to exit regulator", twi->port);
        return TWI_STATUS_OK;
    }

    ret = hal_regulator_disable(&twi->regulator);

    if (ret)
    {
        TWI_ERR("twi%d disable regulator failed\n", twi->port);
        return TWI_STATUS_ERROR;
    }
*/
    return TWI_STATUS_OK;
}
#endif

static twi_status_t hal_twi_clk_init(hal_twi_t *twi)
{
    unsigned long rate;
    /*
        if (hal_clk_set_parent(twi->mclk, twi->pclk)) {
            TWI_ERR("[twi%d] clk set parent failed!",twi->port);
        return TWI_STATUS_ERROR;
        }
    */
#if !(defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6) || defined(CONFIG_ARCH_SUN300IW1))
    rate = hal_clk_get_rate(twi->pclk);

    if (hal_clock_enable(twi->mclk))
    {
        TWI_ERR("[twi%d] clk enable mclk failed!", twi->port);
        return TWI_STATUS_ERROR;
    }

#else
    //rate =OSC24M; /* use hal_clk_get_rate */
    hal_clk_type_t	clk_type = HAL_SUNXI_CCU;
    hal_clk_id_t	twi_clk_id;
    hal_reset_type_t	reset_type = HAL_SUNXI_RESET;
    hal_reset_id_t	twi_reset_id;

    switch (twi->port)
    {
        case 0:
            twi_clk_id = SUNXI_CLK_TWI(0);
            twi_reset_id = SUNXI_CLK_RST_TWI(0);
            break;
        case 1:
            twi_clk_id = SUNXI_CLK_TWI(1);
            twi_reset_id = SUNXI_CLK_RST_TWI(1);
            break;
#if !defined(CONFIG_ARCH_SUN20IW2)
        case 2:
            twi_clk_id = SUNXI_CLK_TWI(2);
            twi_reset_id = SUNXI_CLK_RST_TWI(2);
            break;
#if !defined(CONFIG_ARCH_SUN300IW1)
        case 3:
            twi_clk_id = SUNXI_CLK_TWI(3);
            twi_reset_id = SUNXI_CLK_RST_TWI(3);
            break;
#endif
#endif
#if (defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6))
        case 4:
            twi_clk_id = SUNXI_CLK_TWI(4);
            twi_reset_id = SUNXI_CLK_RST_TWI(4);
            break;
#endif
#ifdef CONFIG_ARCH_SUN55IW3
        case 5:
            twi_clk_id = SUNXI_CLK_TWI(5);
            twi_reset_id = SUNXI_CLK_RST_TWI(5);
            break;
        case 6:
            twi_clk_id = SUNXI_R_CLK_TWI(0);
            twi_reset_id = SUNXI_R_CLK_RST_TWI(0);
            clk_type = HAL_SUNXI_R_CCU;
            reset_type = HAL_SUNXI_R_RESET;
            break;
        case 7:
            twi_clk_id = SUNXI_R_CLK_TWI(1);
            twi_reset_id = SUNXI_R_CLK_RST_TWI(1);
            clk_type = HAL_SUNXI_R_CCU;
            reset_type = HAL_SUNXI_R_RESET;
            break;
        case 8:
            twi_clk_id = SUNXI_R_CLK_TWI(2);
            twi_reset_id = SUNXI_R_CLK_RST_TWI(2);
            clk_type = HAL_SUNXI_R_CCU;
            reset_type = HAL_SUNXI_R_RESET;
            break;
#endif
#ifdef CONFIG_ARCH_SUN55IW6
        case 5:
            twi_clk_id = SUNXI_CLK_TWI(5);
            twi_reset_id = SUNXI_CLK_RST_TWI(5);
            break;
	case 6:
            twi_clk_id = SUNXI_CLK_TWI(6);
            twi_reset_id = SUNXI_CLK_RST_TWI(6);
            break;
        case 7:
            twi_clk_id = SUNXI_R_CLK_TWI(0);
            twi_reset_id = SUNXI_R_CLK_RST_TWI(0);
            clk_type = HAL_SUNXI_R_CCU;
            reset_type = HAL_SUNXI_R_RESET;
            break;
        case 8:
            twi_clk_id = SUNXI_R_CLK_TWI(1);
            twi_reset_id = SUNXI_R_CLK_RST_TWI(1);
            clk_type = HAL_SUNXI_R_CCU;
            reset_type = HAL_SUNXI_R_RESET;
            break;
#endif
        default:
            TWI_ERR("twi%d is invalid\n", twi->port);
            return TWI_STATUS_ERROR;
    }

    if (twi_reset_id)
    {
        twi->reset = hal_reset_control_get(reset_type, twi_reset_id);
        if (!twi->reset)
        {
            TWI_ERR("twi reset control get error");
            return TWI_STATUS_ERROR;
        }
        hal_reset_control_reset(twi->reset);
    }

    twi->clk = hal_clock_get(clk_type, twi_clk_id);
    if (!twi->clk)
    {
        TWI_ERR("twi clock get error ");
        return TWI_STATUS_ERROR;
    }

    rate = hal_clk_get_rate(twi->clk);
    if (!rate) {
        TWI_INFO("twi%d fail to get twi clk rate, use 24M\n", twi->port);
        rate = OSC24M; /* FIXME: fixed to 24MHz */
    }

    if (hal_clock_enable(twi->clk))
    {
        TWI_ERR("twi clock enable error\n");
        return TWI_STATUS_ERROR;
    }

#endif

    if (twi->twi_drv_used)
    {
        twi_enable(twi->base_addr, TWI_DRIVER_CTRL, TWI_DRV_EN);
        twi_set_clock(twi, TWI_DRIVER_BUSC, rate, twi->freq,
                      TWI_DRV_CLK_M, TWI_DRV_CLK_N);
        twi_drv_set_timeout(twi, 0x1f);
    }
    else
    {
        twi_set_clock(twi, TWI_CLK_REG, rate, twi->freq,
                      TWI_CLK_DIV_M, TWI_CLK_DIV_N);
        twi_enable(twi->base_addr, TWI_CTL_REG, TWI_CTL_BUSEN);
    }

    TWI_INFO("twi clock_reg is %x\n", readl(twi->base_addr + TWI_CLK_REG));

    return TWI_STATUS_OK;
}

static void hal_twi_clk_exit(hal_twi_t *twi)
{
    /* disable twi bus */
    twi_disable(twi->base_addr, TWI_DRIVER_CTRL, TWI_DRV_EN);

#if !(defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW3))
    hal_clock_disable(twi->mclk);
#else
    hal_clock_disable(twi->clk);
    hal_reset_control_assert(twi->reset);
#endif
    hal_reset_control_put(twi->reset);
}

#ifdef CONFIG_COMPONENTS_PM
static int hal_twi_suspend(struct pm_device *dev, suspend_mode_t mode)
{
    hal_twi_t *twi = (hal_twi_t *)dev->data;

    hal_disable_irq(twi->irqnum);
    hal_twi_pinctrl_exit(twi);
    hal_twi_clk_exit(twi);

    pm_inf("[twi%d] suspend\n", twi->port);
    return 0;
}

static int hal_twi_resume(struct pm_device *dev, suspend_mode_t mode)
{
    hal_twi_t *twi = (hal_twi_t *)dev->data;

    hal_twi_clk_init(twi);

    if (hal_twi_sys_pinctrl_init(twi)) {
        if (hal_twi_pinctrl_init(twi)) {
            TWI_ERR("[twi%d] pinctrl init error\n", twi->port);
            return TWI_STATUS_ERROR;
        }
    }

    hal_enable_irq(twi->irqnum);

    pm_inf("[twi%d] resume\n", twi->port);
    return 0;
}

struct pm_devops pm_twi_ops = {
    .suspend = hal_twi_suspend,
    .resume = hal_twi_resume,
};
#endif

twi_status_t hal_twi_init_with_freq(twi_port_t port, twi_frequency_t freq)
{
    hal_twi_t *twi = &hal_twi[port];

    if (twi->already_init) //if twi has been inited, return ok
    {
        twi->already_init++;
        TWI_INFO("twi%d has been inited, return ok\n", port);
        return TWI_STATUS_OK;
    }

    twi->port = port;

    if (TWI_STATUS_OK != hal_twi_get_resource(twi))
        TWI_INFO("[twi%d] get resource failed!\n", twi->port);

    TWI_INFO("[twi%d] mode: %s\n", port, twi->twi_drv_used ? "drv" : "engine");

    twi->base_addr = hal_twi_address[twi->port];
    twi->irqnum = hal_twi_irq_num[twi->port];

#if !(defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6) || defined(CONFIG_ARCH_SUN300IW1))
    twi->pclk = hal_twi_pclk[port];
    twi->mclk = hal_twi_mclk[port];
#endif

    twi->freq = freq;

    twi->status = TWI_XFER_IDLE;
    twi->timeout = 5;
    twi->flags = 0;
    twi->result = RESULT_COMPLETE;

    twi->hal_sem = hal_sem_create(0);
    if (twi->hal_sem == NULL)
    {
        TWI_ERR("[twi%d] creating hal semaphore failed.", port);
        goto err0;
    }

    if (twi->twi_drv_used)
    {
        twi->dma_complete = hal_sem_create(0);
	if (twi->dma_complete == NULL)
	{
            TWI_ERR("[twi%d] creating dma semaphore failed.\n", port);
	    goto errsem;
	}
    }

#if !(defined(CONFIG_ARCH_SUN8IW18P1) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN8IW20))
    if (hal_twi_regulator_init(twi))
    {
        TWI_ERR("[twi%d] regulator init error", port);
        goto errsem_dma;
    }
#endif

    if (hal_twi_clk_init(twi))
    {
        TWI_ERR("[twi%d] clk init error", port);
#if !(defined(CONFIG_ARCH_SUN8IW18P1) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN8IW20))
        goto errregu;
#else
        goto errsem_dma;
#endif
    }

    if (hal_twi_sys_pinctrl_init(twi)) {
        if (hal_twi_pinctrl_init(twi)) {
            TWI_ERR("[twi%d] pinctrl init error", port);
            goto errclk;
        }
    }

    twi->lock = hal_mutex_create();
    if (!twi->lock) {
        TWI_ERR("mutex init failed\n");
        goto errpin;
    }

    snprintf(twi->irqname, 32, "twi%d", port);
    if (hal_request_irq(twi->irqnum, hal_twi_handler, twi->irqname, twi) < 0)
    {
	TWI_ERR("[twi%d] request irq error", twi->port);
	goto errirq;
    }

    hal_enable_irq(twi->irqnum);

    if (twi->twi_drv_used)
    {
#ifdef CONFIG_DRIVERS_DMA
        if (hal_dma_chan_request(&twi->dma_chan) != HAL_DMA_CHAN_STATUS_FREE) {
            TWI_ERR("[twi%d] request dma_chan error", twi->port);
            goto errdma;
        }
#endif
    }

#ifdef CONFIG_COMPONENTS_PM
    twi->wl.name = twi->irqname;
    twi->wl.ref = 0;
    twi->pm.name = twi->irqname;
    twi->pm.ops = &pm_twi_ops;
    twi->pm.data = twi;

    pm_devops_register(&twi->pm);
#endif

    twi->already_init++;

    return TWI_STATUS_OK;
#ifdef CONFIG_DRIVERS_DMA
errdma:
    hal_free_irq(twi->irqnum);
#endif
errirq:
    hal_mutex_delete(twi->lock);
errpin:
    hal_twi_pinctrl_exit(twi);
errclk:
    hal_twi_clk_exit(twi);

#if !(defined(CONFIG_ARCH_SUN8IW18P1) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN8IW20))
errregu:
    //hal_twi_regulator_exit(twi);
#endif
errsem_dma:
    if (twi->twi_drv_used)
    {
        hal_sem_delete(twi->dma_complete);
    }
errsem:
    hal_sem_delete(twi->hal_sem);
err0:
    return TWI_STATUS_ERROR;
}

twi_status_t hal_twi_init(twi_port_t port)
{
	twi_frequency_t freq = TWI_FREQUENCY_400K;
	freq = hal_twi_get_freq(port);
	return hal_twi_init_with_freq(port, freq);
}

twi_status_t hal_twi_uninit(twi_port_t port)
{
    hal_twi_t *twi = &hal_twi[port];

    if (twi->already_init > 0)
    {
        twi->already_init--;
        if (twi->already_init == 0)
        {
#ifdef CONFIG_COMPONENTS_PM
            pm_devops_unregister(&twi->pm);
#endif
#if (defined(CONFIG_ARCH_SUN300IW1))
            hal_disable_irq(twi->irqnum);
#endif
            twi_disable_irq(twi->base_addr);
            hal_free_irq(twi->irqnum);
            hal_mutex_delete(twi->lock);
            hal_twi_pinctrl_exit(twi);
            if (twi->twi_drv_used)
            {
#ifdef CONFIG_DRIVERS_DMA
                hal_dma_chan_free(twi->dma_chan);
#endif
            }
            hal_twi_clk_exit(twi);
#if !(defined(CONFIG_ARCH_SUN8IW18P1) || defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_DSP))
            hal_twi_regulator_exit(twi);
#endif
            hal_sem_delete(twi->hal_sem);
        }
    }
    return TWI_STATUS_OK;
}

inner_attr void twi_drv_hw_reset(hal_twi_t *twi)
{
	uint32_t reg_data = readl(twi->base_addr + TWI_DRIVER_CTRL);

	if ((reg_data & TWI_DRV_EN) == 0) {
		twi_disable_irq(twi->base_addr);

		hal_twi_pinctrl_exit(twi);
		hal_twi_clk_exit(twi);

		hal_udelay(10);

		twi->status = TWI_XFER_IDLE;

		if (hal_twi_clk_init(twi))
		{
			TWI_ERR_REPORT(twi->port, 0);
			TWI_ERR("[twi%d] clk init error", twi->port);
		}
		if (hal_twi_sys_pinctrl_init(twi)) {
			if (hal_twi_pinctrl_init(twi)) {
				TWI_ERR_REPORT(twi->port, 0);
				TWI_ERR("[twi%d] pinctrl init error", twi->port);
			}
		}
	}
}

twi_status_t hal_twi_reg_slave(twi_port_t port, uint16_t addr, uint8_t ten_bit_en, twi_frequency_t freq, i2c_slave_cb_t slave_cb)
{
    hal_twi_t *twi = &hal_twi[port];

    if (!twi->already_init)
        return TWI_STATUS_ERROR;

    if (twi->twi_drv_used)
        return TWI_STATUS_INVALID_PARAMETER;

#ifdef CONFIG_COMPONENTS_PM
    pm_wakelocks_acquire(&twi->wl, PM_WL_TYPE_WAIT_INC, OS_WAIT_FOREVER);
#endif

    twi->twi_slave_used = true;
    twi->freq = freq;
    twi->slave_cb = slave_cb;

    hal_twi_clk_init(twi);

    twi_soft_reset(twi);

	if (ten_bit_en) {
		twi->slave_addr = ((addr >> 8) & 0x03) | 0x78;
		writel(twi->slave_addr << TWI_ADDR_SHIFT, twi->base_addr + TWI_ADDR_REG);
		twi->slave_addr = addr & TWI_SLAVE_10_BIT_MASK;
		writel(twi->slave_addr, twi->base_addr + TWI_XADDR_REG);
	} else {
		twi->slave_addr = addr & TWI_SLAVE_7_BIT_MASK;
		writel(twi->slave_addr << TWI_ADDR_SHIFT, twi->base_addr + TWI_ADDR_REG);
	}

    twi_enable_ack(twi->base_addr);
    twi_enable_irq(twi->base_addr);
    twi->status = TWI_XFER_SLV_IDLE;

    TWI_INFO("[twi%d] enter slave mode addr(%#x)\n", twi->port, twi->slave_addr);

    return 0;
}

twi_status_t hal_twi_unreg_slave(twi_port_t port)
{
    hal_twi_t *twi = &hal_twi[port];

    if (!twi->twi_slave_used)
        return TWI_STATUS_ERROR;

    TWI_INFO("[twi%d] exit slave mode addr(%#x)\n", twi->port, twi->slave_addr);

    twi_disable_irq(twi->base_addr);
    twi_disable_ack(twi->base_addr);

    writel(0, twi->base_addr + TWI_ADDR_REG);
    writel(0, twi->base_addr + TWI_XADDR_REG);

    twi->slave_cb = NULL;
    twi->twi_slave_used = false;

#ifdef CONFIG_COMPONENTS_PM
    pm_wakelocks_release(&twi->wl);
#endif

    return 0;
}

twi_status_t hal_twi_slave_reset(twi_port_t port)
{
    hal_twi_t *twi = &hal_twi[port];

    if (!twi->twi_slave_used)
        return TWI_STATUS_ERROR;

    TWI_INFO("twi controller soft reset\n");

    twi_disable_irq(twi->base_addr);
    hal_disable_irq(twi->irqnum);
    twi_slave_reset(twi);
    hal_enable_irq(twi->irqnum);
    twi_enable_irq(twi->base_addr);

    return 0;
}

twi_status_t hal_twi_write(twi_port_t port, twi_msg_t *msg, uint32_t msg_num)
{
    twi_status_t status;
    hal_twi_t *twi = &hal_twi[port];

    hal_mutex_lock(twi->lock);
    status = hal_twi_xfer_nolock(port, msg, msg_num);
    hal_mutex_unlock(twi->lock);

    return status;
}

twi_status_t hal_twi_read(twi_port_t port, hal_twi_transfer_cmd_t cmd, uint16_t slave_addr, uint8_t reg_addr, void *buf, uint32_t size)
{
    twi_msg_t msg[2];
    uint8_t num = 2;
    twi_status_t status;
    hal_twi_t *twi = &hal_twi[port];

    hal_mutex_lock(twi->lock);
    switch (cmd)
    {
        case I2C_SLAVE:
        case I2C_SLAVE_FORCE:
            if (slave_addr > 0x7f)
            {
                TWI_ERR("[twi%d] Failed set slave addr 0x%x, Over than 7-Bit", twi->port, slave_addr);
                status = TWI_STATUS_INVALID_PARAMETER;
                goto unlock;
            } else if (twi->status != TWI_XFER_IDLE) {
                TWI_ERR("[twi%d] Failed set slave addr 0x%x, Bus is not idle", twi->port, slave_addr);
                status = TWI_STATUS_ERROR_BUSY;
                goto unlock;
            }
            twi->flags &= ~TWI_M_TEN;
            break;
        case I2C_TENBIT:
            if (slave_addr > 0x7ff)
            {
                TWI_ERR("[twi%d] Failed set slave addr 0x%x, Over than 10-Bit", twi->port, slave_addr);
                status = TWI_STATUS_INVALID_PARAMETER;
                goto unlock;
            } else if (twi->status != TWI_XFER_IDLE) {
                TWI_ERR("[twi%d] Failed set slave addr 0x%x, Bus is not idle", twi->port, slave_addr);
                status = TWI_STATUS_ERROR_BUSY;
                goto unlock;
            }
            twi->flags |= TWI_M_TEN;
            break;
        default:
            status = TWI_STATUS_INVALID_PARAMETER;
            goto unlock;
    }

    twi->slave_addr = slave_addr;
    msg[0].addr =  twi->slave_addr;
    msg[0].flags = twi->flags & TWI_M_TEN;
    msg[0].flags &= ~(TWI_M_RD);
    msg[0].len = 1;
    msg[0].buf = &reg_addr;

    msg[1].addr =  twi->slave_addr;
    msg[1].flags = twi->flags & TWI_M_TEN;
    msg[1].flags |= TWI_M_RD;
    msg[1].len = size;
    msg[1].buf = buf;

    status = hal_twi_xfer_nolock(port, msg, num);
unlock:
    hal_mutex_unlock(twi->lock);

    return status;
}

twi_status_t hal_twi_write_data(twi_port_t port, uint8_t read_addr, const void *buf, uint32_t size)
{
	twi_msg_t msg;
	uint8_t *msg_buf;
	hal_twi_t *twi = &hal_twi[port];
	twi_status_t status;

	msg_buf = (uint8_t *)malloc(size + 1);
	if (!msg_buf)
		return TWI_STATUS_ERROR;

	msg_buf[0] = read_addr;
	memcpy(&msg_buf[1], buf, size);

	hal_mutex_lock(twi->lock);

	msg.addr =  twi->slave_addr;
	msg.flags = twi->flags & TWI_M_TEN;
	msg.flags &= ~(TWI_M_RD);
	msg.len = (size + 1);
	msg.buf = msg_buf;
	status = hal_twi_xfer_nolock(port, &msg, 1);

	hal_mutex_unlock(twi->lock);
	free(msg_buf);
	return status;
}

static twi_status_t hal_twi_control_nolock(twi_port_t port, hal_twi_transfer_cmd_t cmd, void *args)
{
	hal_twi_t *twi = &hal_twi[port];
	twi_msg_t *msg;
	uint16_t slave_addr;

	switch (cmd)
	{
	case I2C_SLAVE:
	case I2C_SLAVE_FORCE:
		slave_addr = *(uint8_t *)args;
		if (slave_addr > 0x7f) {
			TWI_ERR("[twi%d] Failed set slave addr 0x%x, Over than 7-Bit",
				twi->port, slave_addr);
			return TWI_STATUS_INVALID_PARAMETER;
		} else if (twi->status != TWI_XFER_IDLE) {
			TWI_ERR("[twi%d] Failed set slave addr 0x%x, Bus is not idle, status: %x",
				twi->port, slave_addr, twi->status);
			return TWI_STATUS_ERROR_BUSY;
		}
		twi->slave_addr = slave_addr;
		twi->flags &= ~TWI_M_TEN;
		return TWI_STATUS_OK;
	case I2C_TENBIT:
		slave_addr = *(uint16_t *)args;
		if (slave_addr > 0x7ff) {
			TWI_ERR("[twi%d] Failed set slave addr 0x%x, Over than 10-Bit",
				twi->port, slave_addr);
			return TWI_STATUS_INVALID_PARAMETER;
		} else if (twi->status != TWI_XFER_IDLE) {
			TWI_ERR("[twi%d] Failed set slave addr 0x%x, Bus is not idle, status: %x",
				twi->port, slave_addr, twi->status);
			return TWI_STATUS_ERROR_BUSY;
		}
		twi->slave_addr = slave_addr;
		twi->flags |= TWI_M_TEN;
		return TWI_STATUS_OK;
	case I2C_RDWR:
		msg = (twi_msg_t *)args;
		return hal_twi_xfer_nolock(port, msg, 1);
	default:
		return TWI_STATUS_INVALID_PARAMETER;
	}
}

twi_status_t hal_twi_control(twi_port_t port, hal_twi_transfer_cmd_t cmd, void *args)
{
	hal_twi_t *twi = &hal_twi[port];
	twi_status_t status;

	hal_mutex_lock(twi->lock);
	status = hal_twi_control_nolock(port, cmd, args);
	hal_mutex_unlock(twi->lock);
	return status;
}
