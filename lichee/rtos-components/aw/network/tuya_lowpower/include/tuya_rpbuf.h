#ifndef __LOW_POWER_RPBUF__H__
#define __LOW_POWER_RPBUF__H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <aw_list.h>
#include <hal_atomic.h>
#include <hal_mutex.h>
#include <hal_mem.h>
#include <hal_sem.h>
#include <hal_queue.h>
#include <hal_thread.h>
#include <console.h>
#include <openamp/sunxi_helper/openamp_log.h>
#include <openamp/sunxi_helper/rpmsg_master.h>
#include "lp_com.h"
#include <rpbuf.h>

#define COMM_LEN 30

#define RPBUF_BUFFER_NAME_DEFAULT "rpbuf_tuya"
#define RPBUF_BUFFER_LENGTH_DEFAULT 128
#define RPBUF_DEMO_LOG(controller_id, buf_name, buf_len, fmt, args...) \
	TUYA_LOG_INFO("[%d|%s|%d] " fmt, controller_id, buf_name, buf_len, ##args)

int rpbuf_demo_create(int controller_id, const char *name, int len);
int rpbuf_demo_destroy(const char *name);
int rpbuf_demo_transmit(void *data, int data_len, int offset);

#endif
