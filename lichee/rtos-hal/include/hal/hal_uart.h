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

#ifndef SUNXI_HAL_UART_H
#define SUNXI_HAL_UART_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CONFIG_STANDBY
#include <standby/standby.h>
#endif
#include "sunxi_hal_common.h"
/*
 * include the platform uart header file.
 */
#include <uart/platform-uart.h>

#include <hal_atomic.h>

#ifdef CONFIG_UART_DMA
#include <hal_cache.h>
#include <hal_dma.h>
#include <hal_timer.h>
#include "hal_sem.h"
#endif

struct sunxi_uart_msg {
	unsigned long base;
	uint32_t irqn;
	unsigned int clk_id;
	u32 rst_id;
	u32 uart_tx;
	u32 uart_rx;
	u32 func;
	int drqdst_tx;
	int drqsrc_rx;
	bool no_suspend;
	bool used_dma;
};
/*
 * This enum defines return status of the UART HAL public API.
 * User should check return value after calling these APIs.
 */
typedef enum
{
    HAL_UART_STATUS_RECEIVE_BREAK = -5,         /**< UART port receive break. */
    HAL_UART_STATUS_ERROR_PARAMETER = -4,      /**< Invalid user input parameter. */
    HAL_UART_STATUS_ERROR_BUSY = -3,           /**< UART port is currently in use. */
    HAL_UART_STATUS_ERROR_UNINITIALIZED = -2,  /**< UART port has not been initialized. */
    HAL_UART_STATUS_ERROR = -1,                /**< UART driver detected a common error. */
    HAL_UART_STATUS_OK = 0                     /**< UART function executed successfully. */
} hal_uart_status_t;

typedef enum
{
    UART_0 = 0,
    UART_1,
    UART_2,
    UART_3,
    UART_4,
    UART_5,
    UART_6,
    UART_7,
    UART_8,
    UART_9,
    UART_10,
    UART_11,
    UART_12,
    UART_13,
    UART_14,
    UART_15,
    UART_16,
    UART_MAX,
} uart_port_t;

/* This enum defines baud rate of the UART frame. */
typedef enum
{
    UART_BAUDRATE_300 = 0,
    UART_BAUDRATE_600,
    UART_BAUDRATE_1200,
    UART_BAUDRATE_2400,
    UART_BAUDRATE_4800,
    UART_BAUDRATE_9600,
    UART_BAUDRATE_19200,
    UART_BAUDRATE_38400,
    UART_BAUDRATE_57600,
    UART_BAUDRATE_115200,
    UART_BAUDRATE_230400,
    UART_BAUDRATE_460800,
    UART_BAUDRATE_576000,
    UART_BAUDRATE_921600,
    UART_BAUDRATE_1000000,
    UART_BAUDRATE_1500000,
    UART_BAUDRATE_3000000,
    UART_BAUDRATE_4000000,
    UART_BAUDRATE_MAX,
} uart_baudrate_t;

/* This enum defines word length of the UART frame. */
typedef enum
{
    UART_WORD_LENGTH_5 = 0,
    UART_WORD_LENGTH_6,
    UART_WORD_LENGTH_7,
    UART_WORD_LENGTH_8,
} uart_word_length_t;

/* This enum defines stop bit of the UART frame. */
typedef enum
{
    UART_STOP_BIT_1 = 0,
    UART_STOP_BIT_2,
} uart_stop_bit_t;

/* This enum defines parity of the UART frame. */
typedef enum
{
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD,
    UART_PARITY_EVEN
} uart_parity_t;

typedef void (*user_callback)(void);

/* This struct defines UART configure parameters. */
typedef struct
{
    uart_baudrate_t baudrate;
    uart_word_length_t word_length;
    uart_stop_bit_t stop_bit;
    uart_parity_t parity;
#ifdef CONFIG_UART_DMA
    uint32_t uart_rx_trigger_size;
    int rx_buf_len;
    user_callback callback;
#endif
} _uart_config_t;


/* UART HAL Layer API Version */
#define SUNXI_HAL_UART_API_VERSION               SUNXI_HAL_VERSION_MAJOR_MINOR(1, 0)

/* Driver version */
#define SUNXI_HAL_UART_DRV_VERSION               SUNXI_HAL_VERSION_MAJOR_MINOR(1, 0)

//======================================reg==========================================================//
#define UART_INVAL_DATA_IND     (0xffffffff)

#ifndef BIT
#define BIT(nr)     (1UL << (nr))
#endif

//=================================reg===================================================//
/*
 * brief UART Status
 */
typedef struct sunxi_hal_uart_status
{
    uint32_t tx_busy          : 1;        ///< Transmitter busy flag
    uint32_t rx_busy          : 1;        ///< Receiver busy flag
    uint32_t tx_underflow     : 1;        ///< Transmit data underflow detected (cleared on start of next send operation)
    uint32_t rx_overflow      : 1;        ///< Receive data overflow detected (cleared on start of next receive operation)
    uint32_t rx_break         : 1;        ///< Break detected on receive (cleared on start of next receive operation)
    uint32_t rx_framing_error : 1;        ///< Framing error detected on receive (cleared on start of next receive operation)
    uint32_t rx_parity_error  : 1;        ///< Parity error detected on receive (cleared on start of next receive operation)
    uint32_t reserved         : 25;
} sunxi_hal_uart_status_t;

/*
 *brief UART Modem Control
 */
typedef enum sunxi_hal_uart_modem_control
{
    SUNXI_HAL_UART_RTS_CLEAR,            ///< Deactivate RTS
    SUNXI_HAL_UART_RTS_SET,              ///< Activate RTS
    SUNXI_HAL_UART_DTR_CLEAR,            ///< Deactivate DTR
    SUNXI_HAL_UART_DTR_SET               ///< Activate DTR
} sunxi_hal_uart_modem_control_e;

/*
 *brief UART Modem Status
 */
typedef struct sunxi_hal_uart_modem_status
{
    uint32_t cts      : 1;                ///< CTS state: 1=Active, 0=Inactive
    uint32_t dsr      : 1;                ///< DSR state: 1=Active, 0=Inactive
    uint32_t dcd      : 1;                ///< DCD state: 1=Active, 0=Inactive
    uint32_t ri       : 1;                ///< RI  state: 1=Active, 0=Inactive
    uint32_t reserved : 28;
} sunxi_hal_uart_modem_status_t;

/****** UART Event *****/
#define SUNXI_HAL_UART_EVENT_SEND_COMPLETE       (1UL << 0)  ///< Send completed; however UART may still transmit data
#define SUNXI_HAL_UART_EVENT_RECEIVE_COMPLETE    (1UL << 1)  ///< Receive completed
#define SUNXI_HAL_UART_EVENT_TRANSFER_COMPLETE   (1UL << 2)  ///< Transfer completed
#define SUNXI_HAL_UART_EVENT_TX_COMPLETE         (1UL << 3)  ///< Transmit completed (optional)
#define SUNXI_HAL_UART_EVENT_TX_UNDERFLOW        (1UL << 4)  ///< Transmit data not available (Synchronous Slave)
#define SUNXI_HAL_UART_EVENT_RX_OVERFLOW         (1UL << 5)  ///< Receive data overflow
#define SUNXI_HAL_UART_EVENT_RX_TIMEOUT          (1UL << 6)  ///< Receive character timeout (optional)
#define SUNXI_HAL_UART_EVENT_RX_BREAK            (1UL << 7)  ///< Break detected on receive
#define SUNXI_HAL_UART_EVENT_RX_FRAMING_ERROR    (1UL << 8)  ///< Framing error detected on receive
#define SUNXI_HAL_UART_EVENT_RX_PARITY_ERROR     (1UL << 9)  ///< Parity error detected on receive
#define SUNXI_HAL_UART_EVENT_CTS                 (1UL << 10) ///< CTS state changed (optional)
#define SUNXI_HAL_UART_EVENT_DSR                 (1UL << 11) ///< DSR state changed (optional)
#define SUNXI_HAL_UART_EVENT_DCD                 (1UL << 12) ///< DCD state changed (optional)
#define SUNXI_HAL_UART_EVENT_RI                  (1UL << 13) ///< RI  state changed (optional)




/* This enum defines the UART event when an interrupt occurs. */
typedef enum
{
    UART_EVENT_TRANSACTION_ERROR = -1,
    UART_EVENT_RX_BUFFER_ERROR = -2,
    UART_EVENT_TX_COMPLETE = 1,
    UART_EVENT_RX_COMPLETE = 2,
} uart_callback_event_t;

/** @brief This typedef defines user's callback function prototype.
 *             This callback function will be called in UART interrupt handler when UART interrupt is raised.
 *             User should call uart_register_callback() to register callbacks to UART driver explicitly.
 *             Note, that the callback function is not appropriate for time-consuming operations. \n
 *             parameter "event" : for more information, please refer to description of #uart_callback_event_t.
 *             parameter "user_data" : a user defined data used in the callback function.
 */
typedef void (*uart_callback_t)(uart_callback_event_t event, void *user_data);

typedef struct
{
    uint8_t *buf;
    uint32_t len;
    uint32_t head;
    uint32_t tail;
    int32_t cnt;
} uart_ring_buf_t;

/* This struct defines UART private data */
typedef struct
{
    /* basic info */
    uart_port_t uart_port;
    uint32_t irqn;
    int dma_used;
	int is_inited;

    /* uart register value */
    unsigned char ier;
    unsigned char lcr;
    unsigned char mcr;
    unsigned char fcr;
    unsigned char dll;
    unsigned char dlh;

#ifdef CONFIG_UART_DMA
    /* tx & rx buf */
    char *tx_buf;
    uint32_t tx_size;
    struct sunxi_dma_chan *tx_dma;
    struct dma_slave_config tx_slave_config;
    int tx_dma_inited;
    /* rx ring buf */
    unsigned char *rx_buf;
    struct sunxi_dma_chan *rx_dma;
    uart_ring_buf_t ring_buf;
    struct dma_slave_config rx_slave_config;
    struct sunxi_dma_desc sunxi_desc;
    int rx_dma_inited;
    hal_sem_t   hal_sem;
    hal_sem_t   hal_rxsem;
#endif

    /* user callback */
    uart_callback_t func;
    void *arg;
    int rx_pkgnum;
    int rx_tmpnum;
#ifdef CONFIG_STANDBY
	struct dev_pm *pm;
#endif
	_uart_config_t *uart_config;
	hal_spinlock_t spinlock;
} uart_priv_t;

/**
  \fn          sunxi_hal_version_t SUNXI_HAL_UART_GetVersion (void)
  \brief       Get driver version.
  \return      \ref sunxi_hal_version_t

  \fn          SUNXI_HAL_UART_CAPABILITIES SUNXI_HAL_UART_GetCapabilities (void)
  \brief       Get driver capabilities
  \return      \ref SUNXI_HAL_UART_CAPABILITIES

  \fn          int32_t SUNXI_HAL_UART_Initialize (SUNXI_HAL_UART_SignalEvent_t cb_event)
  \brief       Initialize UART Interface.
  \param[in]   cb_event  Pointer to \ref SUNXI_HAL_UART_SignalEvent
  \return      \ref execution_status

  \fn          int32_t SUNXI_HAL_UART_Uninitialize (void)
  \brief       De-initialize UART Interface.
  \return      \ref execution_status

  \fn          int32_t SUNXI_HAL_UART_PowerControl (SUNXI_HAL_POWER_STATE state)
  \brief       Control UART Interface Power.
  \param[in]   state  Power state
  \return      \ref execution_status

  \fn          int32_t SUNXI_HAL_UART_Send (const void *data, uint32_t num)
  \brief       Start sending data to UART transmitter.
  \param[in]   data  Pointer to buffer with data to send to UART transmitter
  \param[in]   num   Number of data items to send
  \return      \ref execution_status

  \fn          int32_t SUNXI_HAL_UART_Receive (void *data, uint32_t num)
  \brief       Start receiving data from UART receiver.
  \param[out]  data  Pointer to buffer for data to receive from UART receiver
  \param[in]   num   Number of data items to receive
  \return      \ref execution_status

  \fn          int32_t SUNXI_HAL_UART_Transfer (const void *data_out,
                                                 void *data_in,
                                           uint32_t    num)
  \brief       Start sending/receiving data to/from UART transmitter/receiver.
  \param[in]   data_out  Pointer to buffer with data to send to UART transmitter
  \param[out]  data_in   Pointer to buffer for data to receive from UART receiver
  \param[in]   num       Number of data items to transfer
  \return      \ref execution_status

  \fn          uint32_t SUNXI_HAL_UART_GetTxCount (void)
  \brief       Get transmitted data count.
  \return      number of data items transmitted

  \fn          uint32_t SUNXI_HAL_UART_GetRxCount (void)
  \brief       Get received data count.
  \return      number of data items received

  \fn          int32_t SUNXI_HAL_UART_Control (uint32_t control, uint32_t arg)
  \brief       Control UART Interface.
  \param[in]   control  Operation
  \param[in]   arg      Argument of operation (optional)
  \return      common \ref execution_status and driver specific \ref uart_execution_status

  \fn          SUNXI_HAL_UART_STATUS SUNXI_HAL_UART_GetStatus (void)
  \brief       Get UART status.
  \return      UART status \ref SUNXI_HAL_UART_STATUS

  \fn          int32_t SUNXI_HAL_UART_SetModemControl (SUNXI_HAL_UART_MODEM_CONTROL control)
  \brief       Set UART Modem Control line state.
  \param[in]   control  \ref SUNXI_HAL_UART_MODEM_CONTROL
  \return      \ref execution_status

  \fn          SUNXI_HAL_UART_MODEM_STATUS SUNXI_HAL_UART_GetModemStatus (void)
  \brief       Get UART Modem Status lines state.
  \return      modem status \ref SUNXI_HAL_UART_MODEM_STATUS

  \fn          void SUNXI_HAL_UART_SignalEvent (uint32_t event)
  \brief       Signal UART Events.
  \param[in]   event  \ref UART_events notification mask
  \return      none
*/

typedef void (*sunxi_hal_uart_signal_event_t)(uint32_t event);   ///< Pointer to \ref SUNXI_HAL_UART_SignalEvent : Signal UART Event.

/**
\brief UART Device Driver Capabilities.
*/
typedef struct sunxi_hal_uart_capabilities
{
    uint32_t asynchronous       : 1;      ///< supports UART (Asynchronous) mode
    uint32_t synchronous_master : 1;      ///< supports Synchronous Master mode
    uint32_t synchronous_slave  : 1;      ///< supports Synchronous Slave mode
    uint32_t single_wire        : 1;      ///< supports UART Single-wire mode
    uint32_t irda               : 1;      ///< supports UART IrDA mode
    uint32_t smart_card         : 1;      ///< supports UART Smart Card mode
    uint32_t smart_card_clock   : 1;      ///< Smart Card Clock generator available
    uint32_t flow_control_rts   : 1;      ///< RTS Flow Control available
    uint32_t flow_control_cts   : 1;      ///< CTS Flow Control available
    uint32_t event_tx_complete  : 1;      ///< Transmit completed event: \ref SUNXI_HAL_UART_EVENT_TX_COMPLETE
    uint32_t event_rx_timeout   : 1;      ///< Signal receive character timeout event: \ref SUNXI_HAL_UART_EVENT_RX_TIMEOUT
    uint32_t rts                : 1;      ///< RTS Line: 0=not available, 1=available
    uint32_t cts                : 1;      ///< CTS Line: 0=not available, 1=available
    uint32_t dtr                : 1;      ///< DTR Line: 0=not available, 1=available
    uint32_t dsr                : 1;      ///< DSR Line: 0=not available, 1=available
    uint32_t dcd                : 1;      ///< DCD Line: 0=not available, 1=available
    uint32_t ri                 : 1;      ///< RI Line: 0=not available, 1=available
    uint32_t event_cts          : 1;      ///< Signal CTS change event: \ref SUNXI_HAL_UART_EVENT_CTS
    uint32_t event_dsr          : 1;      ///< Signal DSR change event: \ref SUNXI_HAL_UART_EVENT_DSR
    uint32_t event_dcd          : 1;      ///< Signal DCD change event: \ref SUNXI_HAL_UART_EVENT_DCD
    uint32_t event_ri           : 1;      ///< Signal RI change event: \ref SUNXI_HAL_UART_EVENT_RI
    uint32_t reserved           : 11;     ///< Reserved (must be zero)
} sunxi_hal_uart_capabilities_t;

typedef struct sunxi_hal_driver_usart
{

    ///< Pointer to \ref SUNXI_HAL_USART_GetVersion : Get driver version.
    sunxi_hal_version_t (*get_version)(int32_t dev);

    ///< Pointer to \ref SUNXI_HAL_USART_GetCapabilities : Get driver capabilities.
    sunxi_hal_uart_capabilities_t (*get_capabilities)(int32_t dev);

    ///< Pointer to \ref SUNXI_HAL_USART_Initialize : Initialize USART Interface.
    int32_t (*initialize)(int32_t uart_port);

    ///< Pointer to \ref SUNXI_HAL_USART_Uninitialize : De-initialize USART Interface.
    int32_t (*uninitialize)(int32_t uart_port);

    ///< Pointer to \ref SUNXI_HAL_USART_PowerControl : Control USART Interface Power.
    int32_t (*power_control)(int32_t dev, sunxi_hal_power_state_e state);

    ///< Pointer to \ref SUNXI_HAL_USART_Send : Start sending data to USART transmitter.
    int32_t (*send)(int32_t dev, const uint8_t *data, uint32_t num);

    ///< Pointer to \ref SUNXI_HAL_USART_Receive : Start receiving data from USART receiver.
    int32_t (*receive)(int32_t dev, int *data, uint32_t num);

    ///< Pointer to \ref SUNXI_HAL_USART_Transfer : Start sending/receiving data to/from USART.
    int32_t (*transfer)(int32_t dev, const void *data_out, void *data_in, uint32_t    num);

    ///< Pointer to \ref SUNXI_HAL_USART_GetTxCount : Get transmitted data count.
    uint32_t (*get_tx_count)(int32_t dev);

    ///< Pointer to \ref SUNXI_HAL_USART_GetRxCount : Get received data count.
    uint32_t (*get_rx_count)(int32_t dev);

    ///< Pointer to \ref SUNXI_HAL_USART_Control : Control USART Interface.
    int32_t (*control)(int32_t uart_port, int cmd, void *args);

    ///< Pointer to \ref SUNXI_HAL_USART_GetStatus : Get USART status.
    sunxi_hal_uart_status_t (*get_status)(int32_t dev);
    ///< Pointer to \ref SUNXI_HAL_USART_SetModemControl : Set USART Modem Control line state.
    int32_t (*set_modem_control)(int32_t dev, sunxi_hal_uart_modem_control_e control);

    ///< Pointer to \ref SUNXI_HAL_USART_GetModemStatus : Get USART Modem Status lines state.
    sunxi_hal_uart_modem_status_t (*get_modem_status)(int32_t dev);

    int32_t (*receive_polling)(int32_t dev, uint8_t *data, uint32_t num);
    sunxi_hal_poll_ops *poll_ops;
} const sunxi_hal_driver_usart_t;

#define POLLIN          (0x01)
#define POLLRDNORM      (0x01)
#define POLLRDBAND      (0x01)
#define POLLPRI         (0x01)

#define POLLOUT         (0x02)
#define POLLWRNORM      (0x02)
#define POLLWRBAND      (0x02)

#define POLLERR         (0x04)
#define POLLHUP         (0x08)
#define POLLNVAL        (0x10)

sunxi_hal_version_t hal_uart_get_version(int32_t dev);
sunxi_hal_uart_capabilities_t hal_uart_get_capabilities(int32_t dev);
int32_t hal_uart_init(int32_t uart_port);
void uart_multiplex_lpuart(uart_port_t uart_port);
void uart_multiplex_lpuart_nosuspend(uart_port_t uart_port);
int32_t hal_uart_init_for_amp_cli(int32_t uart_port);
int32_t hal_uart_deinit(int32_t uart_port);
int32_t hal_uart_power_control(int32_t dev, sunxi_hal_power_state_e state);
int32_t hal_uart_send(int32_t dev, const uint8_t *data, uint32_t num);
int32_t hal_uart_put_char(int32_t dev, char c);
int32_t hal_uart_receive(int32_t dev, uint8_t *data, uint32_t num);
int32_t hal_uart_receive_no_block(int32_t dev, uint8_t *data, uint32_t num, int32_t timeout);
int hal_uart_terminate_receive(int32_t dev);
uint8_t hal_uart_get_char(int32_t dev);
uint32_t hal_uart_get_tx_count(int32_t dev);
uint32_t hal_uart_get_rx_count(int32_t dev);
int32_t hal_uart_control(int32_t uart_port, int cmd, void *args);
sunxi_hal_uart_status_t hal_uart_get_status(int32_t dev);
int32_t hal_uart_transfer(int32_t dev, const void *data_out,
                        void *data_in, uint32_t num);
int32_t hal_uart_set_modem_control(int32_t dev,
                        sunxi_hal_uart_modem_control_e control);
sunxi_hal_uart_modem_status_t hal_uart_get_modem_status(int32_t dev);
int32_t hal_uart_receive_polling(int32_t dev, uint8_t *data, uint32_t num);
int32_t hal_uart_check_poll_state(int32_t dev_id, short key);
int32_t hal_uart_poll_wakeup(int32_t dev_id, short key);
int32_t hal_uart_register_poll_wakeup(poll_wakeup_func poll_wakeup);
void hal_uart_set_hardware_flowcontrol(uart_port_t uart_port);
void hal_uart_disable_flowcontrol(uart_port_t uart_port);
void hal_uart_set_loopback(uart_port_t uart_port, bool enable);
void sunxi_driver_uart_init(void);
int32_t hal_uart_enable_rx(int32_t uart_port);
int32_t hal_uart_disable_rx(int32_t uart_port);

#ifdef CONFIG_UART_DMA
int32_t hal_uart_cpu_send(int32_t dev, const uint8_t *data, uint32_t num);
hal_uart_status_t hal_uart_dma_send(uart_port_t uart_port, const uint8_t *buf, uint32_t len);
uint32_t hal_uart_dma_recv(uart_port_t uart_port, uint8_t *buf, uint32_t len);
hal_uart_status_t hal_uart_dma_start(uart_port_t uart_port);
hal_uart_status_t hal_uart_dma_stop(uart_port_t uart_port);
#endif

#ifdef __cplusplus
}
#endif

#endif

