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
/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      UART driver definition.
 *
 *  Copyright (c) 2018 ARM Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#ifndef PAL_UART_H
#define PAL_UART_H

#include "pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup PAL_UART
 *  \{ */

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief      Completion callback. */
typedef void (*PalUartCompCback_t)(void);

/*! \brief      Peripheral configuration. */
typedef struct {
	PalUartCompCback_t rdCback;   /*!< Read data completion callback. */
	PalUartCompCback_t wrCback;   /*!< Write data completion callback. */
	uint32_t baud;                /*!< Baud rate. */
	bool_t hwFlow;                /*!< Use HW Flow control */
} PalUartConfig_t;

/*! \brief      Operational states. */
typedef enum {
	PAL_UART_STATE_UNINIT = 0,    /*!< Uninitialized state. */
	PAL_UART_STATE_ERROR  = 0,    /*!< Error state. */
	PAL_UART_STATE_READY  = 1,    /*!< Ready state. */
	PAL_UART_STATE_BUSY   = 2,    /*!< Busy state. */
} PalUartState_t;

/*! \brief      Reserved UART ID. */
typedef enum {
	PAL_UART_ID_USER      = 0,    /*!< UART 0. */
	PAL_UART_ID_CHCI      = 1,    /*!< UART CHCI. */
	PAL_UART_ID_TERMINAL  = 2,    /*!< UART TERMINAL. */
	PAL_UART_ID_MAX               /*!< Number of UART instances. */
} PalUartId_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* Initialization */
void PalUartInit(PalUartId_t id, const PalUartConfig_t *pCfg);
void PalUartDeInit(PalUartId_t id);

/* Control and Status */
PalUartState_t PalUartGetState(PalUartId_t id);

/* Data Transfer */
void PalUartReadData(PalUartId_t id, uint8_t *pData, uint16_t len);
void PalUartWriteData(PalUartId_t id, const uint8_t *pData, uint16_t len);

/*! \} */    /* PAL_UART */

#ifdef __cplusplus
};
#endif

#endif /* PAL_UART_H */
