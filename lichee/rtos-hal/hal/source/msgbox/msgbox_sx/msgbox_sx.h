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
#ifndef __MSGBOX_SX_H
#define __MSGBOX_SX_H

#include "hal_msgbox.h"
#include "errno.h"
#include <stdint.h>

enum msgbox_direction {
	MSGBOX_DIRECTION_DSP,
	MSGBOX_DIRECTION_CPU,
};

enum msgbox_channel_direction {
	MSGBOX_CHANNEL_RECEIVE,
	MSGBOX_CHANNEL_SEND,
};


struct messagebox;

struct msg_channel {
	struct messagebox *mb;
	enum msgbox_channel_direction dir;
	void *data;
	unsigned char *send;
	int len;
	int idx;
	int (*cb_of_msg_queue)(int queue, void *data);
	int (*cb_tx_done)(unsigned long d, void *data);
	int (*cb_rx)(unsigned long d, void *data);
	int channel;
};

/**
 * @base: messagebox base address
 * @this_user: the messagebox user
 */
struct messagebox {
	unsigned long base;
	int this_user;

	int (*channel_set_direction)(struct messagebox *, int channel,
				      enum msgbox_channel_direction dir);

	int (*channel_irq_set)(struct messagebox *, int channel, int enable);

	int (*channel_read)(struct messagebox *, int channel,
			    unsigned long *read);
	int (*channel_write)(struct messagebox *, int channel,
			     unsigned long write);
	int (*channel_fifo_len)(struct messagebox *, int channel);

	struct msg_channel msg_handler[0];
};

extern struct messagebox *msgbox_dsp;
extern struct messagebox *msgbox_cpu;

int messagebox_init_sx(void);

struct msg_channel *msgbox_alloc_channel_sx(struct messagebox *mb, int channel,
					 enum msgbox_channel_direction dir,
					 int (*func)(unsigned long, void *),
					 void *data);

void msgbox_free_channel_sx(struct messagebox *mb, struct msg_channel *ch);

int msgbox_channel_send_data_sx(struct msg_channel *ch, unsigned char *d,
			     size_t len);


#define MSGBOX_CHANNEL_DEPTH 	8
#define MSGBOX_MAX_QUEUE 	8
#define MSGBOX_MAX_USER  	2

int msg_ops_init_sx(struct messagebox *mb);
int msg_irq_handler_sx(int i, void *mb);

#endif /* __MSGBOX_SX_H */
