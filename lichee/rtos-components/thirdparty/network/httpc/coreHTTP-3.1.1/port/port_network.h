#ifndef PORT_NETWORK_H_
#define PORT_NETWORK_H_

#ifndef HTTP_DO_NOT_USE_CUSTOM_CONFIG
    #include "core_http_config.h"
#endif

#include "core_http_config_defaults.h"

#define portNetworkInit      httpNetworkInit
#define portConnectServer    httpConnectServer
#define portDisconnectServer httpDisconnectServer
#define portTransportRecv    httpTransportRecv
#define portTransportSend    httpTransportSend

/********************common********************/
#include "transport_interface.h"
#include "sockets.h"
#include "errno.h"

/**
 *  * @brief Number of milliseconds in one second.
 *   */
#define ONE_SEC_TO_MS    ( 1000 )
/**
 *  * @brief Number of microseconds in one millisecond.
 *   */
#define ONE_MS_TO_US     ( 1000 )

#define FCNTL lwip_fcntl

#define PORT_NET_WORK_BIT(n) (1 << n)
#define CLIENT_FLAG_SECURE PORT_NET_WORK_BIT(0)

#ifndef NETWORK_DEBUG_ON
#define NETWORK_DEBUG_ON 0
#endif

#ifndef NETWORK_DEBUG_ERR
#define NETWORK_DEBUG_ERR 0
#endif

#if NETWORK_DEBUG_ON > 0
#define NETWORK_LOGD(fmt, arg...) printf("[NETWORK][DBG]"fmt"\n", ##arg)
#define NETWORK_LOGE(fmt, arg...) printf("[NETWORK][ERR]"fmt"\n", ##arg)
#else
#define NETWORK_LOGD(fmt, arg...)
#define NETWORK_LOGE(fmt, arg...)
#endif

#define NETWORK_DBG(x) do { NETWORK_LOGD x; } while (0)
#define NETWORK_ERR(x) do { NETWORK_LOGE x; } while (0)

struct NetworkContext {
	fd_set FDWrite;
	fd_set FDRead;
	fd_set FDError;
	uint32_t Socket;
	uint32_t Flags;
	uint32_t verifyMode;
};

typedef struct ServerInfo {
    const char * pHostName; /**< @brief Server host name. */
    size_t hostNameLength;  /**< @brief Length of the server host name. */
    uint16_t port;          /**< @brief Server port in host-order. */
} ServerInfo_t;

int32_t portNetworkInit(NetworkContext_t * pNetworkContext, int32_t use_ssl);
int32_t portDisconnectServer( NetworkContext_t * pNetworkContext);
int32_t portConnectServer( NetworkContext_t * pNetworkContext,
						const ServerInfo_t * pServerInfo,
						uint32_t sendTimeoutMs,
						uint32_t recvTimeoutMs );

int32_t portTransportSend( NetworkContext_t * pNetworkContext,
							 const void * pBuffer,
							 size_t bytesToSend );

int32_t portTransportRecv( NetworkContext_t * pNetworkContext,
                                     void * pBuffer,
                                     size_t bytesToRecv );

#endif /* PORT_NETWORK_H_ */
