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

#ifndef __SLAVE_WLAN_H__
#define __SLAVE_WLAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "xrlink_debug.h"

typedef enum {
    WIFI_COUNTRY_CODE_AU = 0,
    WIFI_COUNTRY_CODE_CA,
    WIFI_COUNTRY_CODE_CN,
    WIFI_COUNTRY_CODE_DE,
    WIFI_COUNTRY_CODE_EU,
    WIFI_COUNTRY_CODE_FR,
    WIFI_COUNTRY_CODE_JP,
    WIFI_COUNTRY_CODE_RU,
    WIFI_COUNTRY_CODE_SA,
    WIFI_COUNTRY_CODE_US,
    WIFI_COUNTRY_CODE_NONE
} wifi_country_code_t;

typedef struct {
    wifi_country_code_t code;
} wifi_country_code_info_t;

typedef int (*wlan_user_msg_cb)(void);

int wlan_net_ctrl_init(void);
int wlan_net_ctrl_deinit(void);
int wlan_net_ctrl_msg_register(wlan_user_msg_cb init_cb, wlan_user_msg_cb deinit_cb);

int wifi_set_countrycode(wifi_country_code_t countrycode);
wifi_country_code_t wifi_get_countrycode(void);

#ifdef __cplusplus
}
#endif

#endif
