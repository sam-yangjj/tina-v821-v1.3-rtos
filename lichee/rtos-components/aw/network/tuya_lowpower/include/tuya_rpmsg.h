#ifndef __LOW_POWER_RPMSG__H__
#define __LOW_POWER_RPMSG__H__

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

struct ept_test_entry {
	struct rpmsg_ept_client *client;
	unsigned long print_perf_data;
	uint16_t keep_alive;
};

int low_power_rpmsg_init(rpmsg_ept_cb cb);
int low_power_rpmsg_deinit(void);
int low_power_rpmsg_to_cpux(char *send_msg, int len);
int low_power_rpmsg_set_keepalive_state(int state);
int low_power_rpmsg_get_sleep_state(void);

#endif
