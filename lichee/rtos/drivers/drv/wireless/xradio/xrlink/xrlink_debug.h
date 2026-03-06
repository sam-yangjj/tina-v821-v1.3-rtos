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

#ifndef __XRLINK_DEBUG_H__
#define __XRLINK_DEBUG_H__
#include <stdio.h>

#define SLAVE_WLAN_INF_ON		0
#define SLAVE_WLAN_DBG_ON		0
#define SLAVE_WLAN_WRN_ON		1
#define SLAVE_WLAN_ERR_ON		1

#define PTC_CMD_INF_ON			1
#define PTC_CMD_DBG_ON			0
#define PTC_CMD_WRN_ON			1
#define PTC_CMD_ERR_ON			1

#define TXRX_INF_ON				0
#define TXRX_DBG_ON				0
#define TXRX_WRN_ON				1
#define TXRX_ERR_ON				1


#define SLAVE_WLAN_SYSLOG		printf
#define SLAVE_WLAN_LOG(flags, fmt, arg...)	\
    do {									\
        if (flags)							\
           SLAVE_WLAN_SYSLOG(fmt, ##arg);	\
    } while (0)

#define SLAVE_WLAN_INF(fmt, arg...)	\
    SLAVE_WLAN_LOG(SLAVE_WLAN_INF_ON, "[SLAVE_WLAN INF] %s "fmt, __func__, ##arg)

#define SLAVE_WLAN_DBG(fmt, arg...)	\
    do {																	\
        SLAVE_WLAN_LOG(SLAVE_WLAN_DBG_ON, "[SLAVE_WLAN_DBG] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);									\
    } while (0)

#define SLAVE_WLAN_WRN(fmt, arg...)	\
    do {																	\
        SLAVE_WLAN_LOG(SLAVE_WLAN_WRN_ON, "[SLAVE_WLAN_WRN] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);									\
    } while (0)

#define SLAVE_WLAN_ERR(fmt, arg...)											\
    do {																	\
        SLAVE_WLAN_LOG(SLAVE_WLAN_ERR_ON, "[SLAVE_WLAN_ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);									\
    } while (0)


#define TXRX_SYSLOG		printf
#define TXRX_LOG(flags, fmt, arg...)		\
    do {									\
        if (flags)							\
           TXRX_SYSLOG(fmt, ##arg);			\
    } while (0)

#define TXRX_INF(fmt, arg...)	\
    TXRX_LOG(TXRX_INF_ON, "[TXRX INF] %s "fmt, __func__, ##arg)

#define TXRX_DBG(fmt, arg...)	\
    do {																	\
        TXRX_LOG(TXRX_DBG_ON, "[TXRX DBG] %s():%d, "fmt,					\
               __func__, __LINE__, ##arg);									\
    } while (0)

#define TXRX_WRN(fmt, arg...)	\
    do {																	\
        TXRX_LOG(TXRX_WRN_ON, "[TXRX WRN] %s():%d, "fmt,					\
               __func__, __LINE__, ##arg);									\
    } while (0)

#define TXRX_ERR(fmt, arg...)												\
    do {																	\
        TXRX_LOG(TXRX_ERR_ON, "[TXRX ERR] %s():%d, "fmt,					\
               __func__, __LINE__, ##arg);									\
    } while (0)



#define PTC_CMD_SYSLOG        printf

#define PTC_CMD_LOG(flags, fmt, arg...)				\
    do {											\
        if (flags)									\
            PTC_CMD_SYSLOG(fmt, ##arg);				\
    } while (0)

#define PTC_CMD_INF(fmt, arg...)	\
		PTC_CMD_LOG(PTC_CMD_INF_ON, "[PTC INF] %s "fmt, __func__, ##arg)

#define PTC_CMD_DBG(fmt, arg...)	\
    do {																	\
        PTC_CMD_LOG(PTC_CMD_DBG_ON, "[PTC DBG] %s():%d, "fmt,				\
               __func__, __LINE__, ##arg);									\
    } while (0)

#define PTC_CMD_WRN(fmt, arg...)    \
    do {																	\
        PTC_CMD_LOG(PTC_CMD_WRN_ON, "[PTC WRN] %s():%d, "fmt,				\
               __func__, __LINE__, ##arg);									\
    } while (0)

#define PTC_CMD_ERR(fmt, arg...)											\
    do {																	\
        PTC_CMD_LOG(PTC_CMD_ERR_ON, "[PTC ERR] %s():%d, "fmt,				\
               __func__, __LINE__, ##arg);									\
    } while (0)


static inline void data_hex_dump(char *pref, int width, unsigned char *buf, int len)
{
	int i, n;
	for (i = 0, n = 1; i < len; i++, n++) {
		if (n == 1)
			printf("%s ", pref);
		printf("%2.2X ", buf[i]);
		if (n == width) {
			printf("\n");
			n = 0;
		}
	}
	if (i && n != 1)
		printf("\n");
}

#endif
