/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _XRADIO_RPBUF_H_
#define _XRADIO_RPBUF_H_

#include "xrlink.h"

#define RPBUF_BUFFER_NAME_XRADIO_MTX    "xradio_mtx" // Master TX, Linux -> RTOS
#define RPBUF_BUFFER_NAME_XRADIO_MRX    "xradio_mrx" // Master RX, Linux <- RTOS
#define RPBUF_BUFFER_LENGTH_XRADIO_MTX (12 * 1024)//(12 * 1024)
#define RPBUF_BUFFER_LENGTH_XRADIO_MRX (20 * 1024)//(12 * 1024)
#define RPBUF_BUFFER_MRX_AGG_MAX        24

//#define RPBUF_NO_CHECK_RSP

enum xrlink_rpbuf_op {
	XR_TXBUS_OP_AUTO = 0,
	XR_TXBUS_OP_FORCE_TX,
	XR_TXBUS_OP_TX_RETRY,
	XR_TXBUS_OP_RESET_BUF,
	XR_TXBUS_OP_FLUSH_BUF
};

enum xrlink_rpbuf_status {
	XR_TXD_ST_SUCESS = 0,
	XR_TXD_ST_NO_MEM = 1,
	XR_TXD_ST_NO_QUEUE = 2,
	XR_TXD_ST_DATA_ERR = 3,
	XR_TXD_ST_NO_RSP = 4,
	XR_TXD_ST_BUS_TX_FAIL = 5,
};

int rpbuf_xradio_init(void);
int rpbuf_xradio_deinit(void);
int rpbuf_xradio_send(struct xradio_hdr hdr, void *data, int len, enum xrlink_rpbuf_op op);
#endif
