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

#ifndef HAL_GPADC_H
#define HAL_GPADC_H

#include <hal_clk.h>
#include <hal_reset.h>
#include "sunxi_hal_common.h"
#include <hal_log.h>
#include <hal_mutex.h>
#include <interrupt.h>
#include <gpadc/platform_gpadc.h>
#include <gpadc/common_gpadc.h>
#ifdef CONFIG_COMPONENTS_PM
#include <pm_wakesrc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* define this macro when debugging is required */
/* #define CONFIG_DRIVERS_GPADC_DEBUG */
#ifdef CONFIG_DRIVERS_GPADC_DEBUG
#define GPADC_INFO(fmt, arg...) hal_log_info(fmt, ##arg)
#else
#define GPADC_INFO(fmt, arg...) do {}while(0)
#endif

#define GPADC_ERR(fmt, arg...) hal_log_err(fmt, ##arg)

enum
{
    GPADC_DOWN,
    GPADC_UP
};

typedef enum
{
    GP_CH_0 = 0,
    GP_CH_1,
    GP_CH_2,
    GP_CH_3,
    GP_CH_4,
    GP_CH_5,
    GP_CH_6,
    GP_CH_7,
    GP_CH_8,
    GP_CH_9,
    GP_CH_A,
    GP_CH_B,
    GP_CH_C,
    GP_CH_D,
    GP_CH_E,
    GP_CH_MAX
} hal_gpadc_channel_t;

typedef enum
{
    GPADC_IRQ_ERROR = -4,
    GPADC_CHANNEL_ERROR = -3,
    GPADC_CLK_ERROR = -2,
    GPADC_ERROR = -1,
    GPADC_OK = 0,
} hal_gpadc_status_t;

typedef enum gp_select_mode
{
    GP_SINGLE_MODE = 0,
    GP_SINGLE_CYCLE_MODE,
    GP_CONTINUOUS_MODE,
    GP_BURST_MODE,
} hal_gpadc_mode_t;

typedef int (*gpadc_callback_t)(uint32_t data_type, uint32_t data);

static uint32_t hal_gpadc_regs_offset[] = {
    GP_SR_REG,
    GP_CTRL_REG,
    GP_CS_EN_REG,
    GP_FIFO_INTC_REG,
    GP_FIFO_DATA_REG,
    GP_CB_DATA_REG,
    GP_DATAL_INTC_REG,
    GP_DATAH_INTC_REG,
    GP_DATA_INTC_REG,
    GP_CH0_CMP_DATA_REG,
    GP_CH1_CMP_DATA_REG,
    GP_CH2_CMP_DATA_REG,
    GP_CH3_CMP_DATA_REG,
    GP_CH4_CMP_DATA_REG,
    GP_CH5_CMP_DATA_REG,
    GP_CH6_CMP_DATA_REG,
    GP_CH7_CMP_DATA_REG,
    GP_CH8_CMP_DATA_REG,
    GP_CH9_CMP_DATA_REG,
    GP_CHA_CMP_DATA_REG,
    GP_CHB_CMP_DATA_REG,
    GP_CHC_CMP_DATA_REG,
    GP_CHD_CMP_DATA_REG,
    GP_CHE_CMP_DATA_REG,
};

typedef struct
{
    uint32_t already_init;
    uint32_t reg_base;
    uint32_t channel_num;
    uint32_t irq_num;
    uint32_t sample_rate;
    struct reset_control *reset;
#if defined(CONFIG_SOC_SUN20IW1) || defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW2) || defined(CONFIG_ARCH_SUN55IW3)
    hal_clk_id_t bus_clk;
    hal_clk_id_t rst_clk;
    hal_clk_t mbus_clk;
    hal_clk_t mbus_clk1;
#elif defined(CONFIG_ARCH_SUN300IW1)
    hal_clk_id_t bus_clk;
    hal_clk_id_t bus_clk_gating;
    hal_clk_id_t rst_clk;
    hal_clk_t mbus_clk;
    hal_clk_t mbus_clk_gating;
    hal_clk_t ths_bus_clk;
    struct reset_control *ths_rst_clk;
#else
    hal_clk_id_t mclk;
    hal_clk_id_t pclk;
#endif
    hal_gpadc_mode_t mode;
    gpadc_callback_t callback[CHANNEL_MAX_NUM];
    uint32_t regs_backup[ARRAY_SIZE(hal_gpadc_regs_offset)];
    hal_mutex_t lock;
#ifdef CONFIG_COMPONENTS_PM
		pm_wakesrc_t *ws;
#endif
} hal_gpadc_t;

int hal_gpadc_init(void);
hal_gpadc_status_t hal_gpadc_deinit(void);
hal_gpadc_status_t hal_gpadc_channel_init(hal_gpadc_channel_t channal);
hal_gpadc_status_t hal_gpadc_channel_exit(hal_gpadc_channel_t channal);
uint32_t gpadc_read_channel_data(hal_gpadc_channel_t channal);
hal_gpadc_status_t hal_gpadc_register_callback(hal_gpadc_channel_t channal,
        gpadc_callback_t user_callback);
void gpadc_key_enable_highirq(hal_gpadc_channel_t channal);
void gpadc_key_disable_highirq(hal_gpadc_channel_t channal);
void gpadc_key_disable_lowirq(hal_gpadc_channel_t channal);
void gpadc_channel_enable_highirq(hal_gpadc_channel_t channal);
void gpadc_channel_disable_highirq(hal_gpadc_channel_t channal);
void gpadc_channel_enable_lowirq(hal_gpadc_channel_t channal);
#ifdef CONFIG_COMPONENTS_PM
void hal_gpadc_report_wakeup_event();
#endif

#ifdef __cplusplus
}
#endif

#endif
