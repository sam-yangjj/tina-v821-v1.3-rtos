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
#ifndef SUNXI_HAL_TWI_H
#define SUNXI_HAL_TWI_H

#include "hal_sem.h"
#include <hal_clk.h>
#include <hal_reset.h>
#include "sunxi_hal_common.h"
#include "hal_gpio.h"
#include "sunxi_hal_regulator.h"
#include <twi/platform_twi.h>
#include <twi/common_twi.h>
#include <hal_mutex.h>
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#include <pm_wakelock.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//for debug
//#define CONFIG_DRIVERS_TWI_DEBUG
#ifdef CONFIG_DRIVERS_TWI_DEBUG
#define TWI_INFO(fmt, arg...) hal_log_info(fmt, ##arg)
#else
#define TWI_INFO(fmt, arg...) do {}while(0)
#endif

#define TWI_ERR(fmt, arg...) hal_log_err(fmt, ##arg)
#define TWI_WARN(fmt, arg...) hal_log_warn(fmt, ##arg)

enum i2c_slave_event {
	I2C_SLAVE_READ_REQUESTED,
	I2C_SLAVE_WRITE_REQUESTED,
	I2C_SLAVE_READ_PROCESSED,
	I2C_SLAVE_WRITE_RECEIVED,
	I2C_SLAVE_STOP,
};

typedef int (*i2c_slave_cb_t)(uint8_t port, enum i2c_slave_event event, u8 *val);

typedef enum
{
    TWI_XFER_IDLE    = 0x1,
    TWI_XFER_START   = 0x2,
    TWI_XFER_RUNNING = 0x4,
    TWI_XFER_SLV_IDLE,
    TWI_XFER_SLV_SADDR,
    TWI_XFER_SLV_WDATA,
    TWI_XFER_SLV_RDATA,
    TWI_XFER_SLV_ERROR,
} twi_xfer_status_t;

/** @brief This enum defines the HAL interface return value. */
typedef enum
{
    TWI_STATUS_ERROR = -4,                        /**<  An error occurred and the transaction has failed. */
    //TWI_STATUS_ERROR_TIMEOUT = -4,                /**<  The TWI bus xfer timeout, an error occurred. */
    TWI_STATUS_ERROR_BUSY = -3,                   /**<  The TWI bus is busy, an error occurred. */
    TWI_STATUS_INVALID_PORT_NUMBER = -2,          /**<  A wrong port number is given. */
    TWI_STATUS_INVALID_PARAMETER = -1,            /**<  A wrong parameter is given. */
    TWI_STATUS_OK = 0                             /**<  No error occurred during the function call. */
} twi_status_t;

typedef enum
{
    TWI_MASTER_0,           /**< TWI master 0. */
    TWI_MASTER_1,           /**< TWI master 1. */
    TWI_MASTER_2,           /**< TWI master 0. */
    TWI_MASTER_3,           /**< TWI master 1. */
    TWI_MASTER_4,           /**< TWI master 4. */
    TWI_MASTER_5,           /**< TWI master 5. */
    TWI_MASTER_6,           /**< TWI master 6. */
    S_TWI_MASTER_0,           /**< S_TWI master 0. */
    S_TWI_MASTER_1,           /**< S_TWI master 1. */
    S_TWI_MASTER_2,           /**< S_TWI master 2. */
    TWI_MASTER_MAX              /**< max TWI master number, \<invalid\> */
} twi_port_t;

/** @brief This enum defines the TWI transaction speed.  */
typedef enum
{
    TWI_FREQUENCY_100K = 100000,          /**<  100kbps. */
    TWI_FREQUENCY_200K = 200000,          /**<  200kbps. */
    TWI_FREQUENCY_400K = 400000,          /**<  400kbps. */
} twi_frequency_t;

/** @brief This enum defines the TWI transaction speed.  */
typedef enum
{
    ENGINE_XFER = 0,
    TWI_DRV_XFER = 1,
} twi_mode_t;

typedef struct twi_msg
{
    uint16_t addr;          /* slave address */
    uint16_t flags;
#define TWI_M_RD        0x0001  /* read data, from slave to master
                     * TWI_M_RD is guaranteed to be 0x0001!
                     * */
#define TWI_M_TEN       0x0010  /* this is a ten bit chip address */
    uint16_t len;           /* msg length */
    uint8_t *buf;       /* pointer to msg data */
} twi_msg_t;

typedef struct sunxi_twi
{
    bool twi_slave_used;

    uint8_t port;
    uint8_t result;
    uint8_t already_init;
    uint8_t twi_drv_used;
    uint8_t pkt_interval;

    uint16_t slave_addr;
    uint16_t flags;

    uint32_t timeout;
    uint32_t msgs_num;
    uint32_t msgs_idx;
    uint32_t msgs_ptr;
    unsigned long base_addr;
    uint32_t irqnum;
    char irqname[32];

    struct regulator_dev regulator;
    struct reset_control *reset;
    hal_clk_t pclk;
    hal_clk_t mclk;
    hal_clk_t clk;
    twi_frequency_t freq;

    uint32_t    pinmux;
    uint32_t    pin[TWI_PIN_NUM];
    twi_xfer_status_t   status;
    hal_sem_t   hal_sem;
    twi_msg_t   *msgs;

    struct sunxi_dma_chan *dma_chan;
    hal_sem_t	  dma_complete;

    hal_mutex_t lock;

#ifdef CONFIG_COMPONENTS_PM
    struct pm_device pm;
    struct wakelock wl;
#endif

    i2c_slave_cb_t slave_cb;
} hal_twi_t;

typedef enum
{
    I2C_SLAVE = 0,
    I2C_SLAVE_FORCE = 1,
    I2C_TENBIT = 2,
    I2C_RDWR = 3
} hal_twi_transfer_cmd_t;

//initialize twi port
twi_status_t hal_twi_init(twi_port_t port);
twi_status_t hal_twi_init_with_freq(twi_port_t port, twi_frequency_t freq);
//uninitialize twi port
twi_status_t hal_twi_uninit(twi_port_t port);
//initialize twi port as slave mode
twi_status_t hal_twi_reg_slave(twi_port_t port, uint16_t addr, uint8_t ten_bit_en, twi_frequency_t freq, i2c_slave_cb_t slave_cb);
//uninitialize twi port as slave mode
twi_status_t hal_twi_unreg_slave(twi_port_t port);
//twi slave reset
twi_status_t hal_twi_slave_reset(twi_port_t port);
//twi write
twi_status_t hal_twi_write(twi_port_t port, twi_msg_t *args, uint32_t msg_num);
//twi read
twi_status_t hal_twi_read(twi_port_t port, hal_twi_transfer_cmd_t cmd, uint16_t slave_addr, uint8_t reg_addr, void *buf, uint32_t size);
//twi control
twi_status_t hal_twi_xfer(twi_port_t port, twi_msg_t *msgs, int32_t num);

//twi api version 2
#define TWI_MODE_DRV		(0x1<<0)
#define TWI_MODE_ENGINE		(0x0<<0)

uint32_t hal_twi_get_mode(twi_port_t port);
twi_status_t hal_twi_set_mode(twi_port_t port, uint32_t mode);
uint32_t hal_twi_get_freq(twi_port_t port);
twi_status_t hal_twi_set_freq(twi_port_t port, twi_frequency_t freq);
twi_status_t hal_twi_control(twi_port_t port, hal_twi_transfer_cmd_t cmd, void *args);
twi_status_t hal_twi_write_data(twi_port_t port, uint8_t read_addr, const void *buf, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
