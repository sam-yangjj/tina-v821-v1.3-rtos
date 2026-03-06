#ifndef __LP_CTRL_MSG_H__
#define __LP_CTRL_MSG_H__

#include "lp_cli.h"
#include "sys_ctrl.h"
#include "wlan.h"
#include "net_ctrl.h"

typedef void (*lp_msg_cb)(uint32_t event, uint32_t data, void *arg);

int lp_ctrl_msg_init(void);

int lp_ctrl_msg_deinit(void);

int lp_ctrl_msg_set_cb(lp_msg_cb cb);

#endif
