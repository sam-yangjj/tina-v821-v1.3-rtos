#ifndef __LP_CLI_H__
#define __LP_CLI_H__

#include <lwip/sockets.h>
#include <lwip/inet.h>
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/netdb.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "lp_com.h"
#include "lp_protocol.h"
#include "lp_auth.h"
#include "crc32i.h"
#include "tuya_ipc_low_power_api.h"

int lp_cli_set_socket_o_block(int sock, int block);
int lp_cli_set_socket_timeout(int sock, int ms);
int lp_cli_init_dev_info(lp_auth_dev_msg_s *pmsg, char *pdevId, int idLen, char *pkey, int keyLen);
int lp_cli_connect_server(TUYA_IP_ADDR_T hostaddr, int port);
int lp_cli_auth(int sock, lp_auth_dev_msg_s *pdevMsg);
int lp_cli_wake_up_data_seed_computer(char *key, unsigned int len);

#endif
