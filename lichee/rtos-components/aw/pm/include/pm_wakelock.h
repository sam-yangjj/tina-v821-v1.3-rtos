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

#ifndef _PM_WAKELOCK_H_
#define _PM_WAKELOCK_H_

#include <hal/aw_list.h>
#include <pm_wakesrc.h>

#ifdef __cplusplus
extern "C" {
#endif

enum pm_wakelock_t {
        PM_WL_TYPE_UNKOWN = 0,
        PM_WL_TYPE_WAIT_ONCE,
        PM_WL_TYPE_WAIT_INC,
        PM_WL_TYPE_WAIT_TIMEOUT,
};

struct wakelock {
	const char *name;
	uint16_t ref;
	uint8_t wrap;
	uint32_t expired;
	enum pm_wakelock_t type;
	pm_wakesrc_mode_t ws_mode;
	pm_wakesrc_t *ws;
	struct list_head node;
	struct list_head timer_node;
};

#define PM_WAKELOCK_USE_GLOBE_CNT
#define OS_WAIT_FOREVER		0xffffffffU

int pm_wakelocks_init(void);
int pm_wakelocks_deinit(void);
void pm_wakelocks_setname(struct wakelock *wl, const char *name);
int  pm_wakelocks_acquire(struct wakelock *wl, enum pm_wakelock_t type, uint32_t timeout);
int  pm_wakelocks_release(struct wakelock *wl);
void pm_wakelock_break_link(void);
void pm_show_wakelocks(uint8_t active);
void pm_wakelock_take_mutex(void);
void pm_wakelock_give_mutex(void);

#ifdef __cplusplus
}
#endif
#endif

