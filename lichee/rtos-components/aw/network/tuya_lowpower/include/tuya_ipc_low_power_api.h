/*
 * tuya_ipc_lowpower_api.h
 *
 *  Created on: 2020年8月13日
 *      Author: kuiba
 */

#ifndef SVC_LOWPOWER_INCLUDE_TUYA_IPC_LOWPOWER_API_H_
#define SVC_LOWPOWER_INCLUDE_TUYA_IPC_LOWPOWER_API_H_
 // #include "tuya_cloud_types.h"

typedef unsigned int UINT_T;
typedef unsigned char UCHAR_T;
typedef UCHAR_T IP_ADDR_TYPE;

typedef struct {
#define ipaddr4 u_addr.ip4
#define ipaddr6 u_addr.ip6
	union {
		UINT_T ip6[4];
		UINT_T ip4;
	} u_addr;
	IP_ADDR_TYPE type;
} TUYA_IP_ADDR_T;

/**
 * \fn  int tuya_ipc_lowpower_server_connect
 * \brief connect tuya low power servivce
 * \return  int 0:success.other:failed
 */

int tuya_ipc_low_power_server_connect(TUYA_IP_ADDR_T serverIp, signed int port, char *pdevId, signed int idLen, char *pkey, signed int keyLen);
/**
 * \fn  int tuya_ipc_low_power_socket_fd_get
 * \brief get tuya low power keep alive tcp handler
 * \return  int 0:success.other:failed
 */
int tuya_ipc_low_power_socket_fd_get();
/**
 * \fn  int tuya_ipc_lowpower_server_connect
 * \brief get tuya low power wakeup data
 * \return  int 0:success.other:failed
 */
int tuya_ipc_low_power_wakeup_data_get(char *pdata, int *plen);
/**
 * \fn  int tuya_ipc_lowpower_server_connect
 * \brief get tuya low power heart beat data;
 * \return  int 0:success.other:failed
 */
int tuya_ipc_low_power_heart_beat_get(char *pdata, int *plen);

#endif /* SVC_LOWPOWER_INCLUDE_TUYA_IPC_LOWPOWER_API_H_ */
