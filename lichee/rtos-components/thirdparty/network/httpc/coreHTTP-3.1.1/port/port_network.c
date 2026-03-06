#include "port_network.h"
#include "MbedTLSWrapper.h"

#define API_TRACE(f) do { if(f) printf("[API TRACE]"" %s:%d\n", __func__, __LINE__);} while(0)
static int32_t socketSetNonblocking(int socket, int on_off)
{
	int32_t	flags_orig , flags_new;

	// Get current socket flags; return if error.
	flags_orig = FCNTL( socket , F_GETFL , 0 );
	if ( flags_orig < 0 )
			return -1;

	// Adjust flags.
	if ( on_off != 0 )
		flags_new = flags_orig | O_NONBLOCK;
	else
		flags_new = flags_orig & ~O_NONBLOCK;

	// Set socket to blocking/nonblocking; return if error.
	if ( FCNTL( socket , F_SETFL , flags_new ) < 0 ) {
		// Set flags back to where they were.
		FCNTL( socket , F_SETFL , flags_orig );
		// And return.
		return -1;
	}
	return 0;
}

int32_t portNetworkInit(NetworkContext_t * pNetworkContext, int32_t use_ssl)
{
    int32_t returnStatus = EXIT_SUCCESS;

#if SUPPORT_SSL
	if (use_ssl) {
		pNetworkContext->Flags |= CLIENT_FLAG_SECURE;
		pNetworkContext->verifyMode = MBEDTLS_SSL_VERIFY_NONE;
	}
#endif

	return returnStatus;
}

int32_t portDisconnectServer(NetworkContext_t * pNetworkContext)
{
    int32_t nRetCode = -1;
	int32_t tcpSocket = pNetworkContext->Socket;

	NETWORK_DBG(("%s disconnect server...\n",__func__));
	if( tcpSocket >= 0 ) {
		if ((pNetworkContext->Flags & CLIENT_FLAG_SECURE) == CLIENT_FLAG_SECURE) {
#if SUPPORT_SSL
			nRetCode = WrapperSSLClose(tcpSocket);
#endif
		}
		( void ) shutdown( tcpSocket, SHUT_RDWR );
		nRetCode = close( tcpSocket );
	} else {
		NETWORK_ERR(( "Parameter check failed: tcpSocket was negative.\n" ));
		nRetCode = -1;
	}

	return nRetCode < 0? EXIT_FAILURE : EXIT_SUCCESS;
}

int32_t portConnectServer( NetworkContext_t * pNetworkContext,
						const ServerInfo_t * pServerInfo,
						uint32_t sendTimeoutMs,
						uint32_t recvTimeoutMs )
{
    int32_t nRetCode = -1;
    uint32_t Address = 0;
    struct sockaddr_in ServerAddress;                      // Socket address structure

	NETWORK_DBG(("%s connect server...\n",__func__));
	// Create a TCP/IP stream socket
	pNetworkContext->Socket = -1;
	pNetworkContext->Socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (pNetworkContext->Socket < 0) {
		NETWORK_ERR(("%s creat socket failed.\n",__func__));
		goto errorExit;
	}
#if SOCKET_NOBLOCKING
    nRetCode = socketSetNonblocking(pNetworkContext->Socket, 1);
	if (nRetCode < 0) {
		NETWORK_ERR(("%s set socket noblocking failed.\n", __func__));
	}
#else
    nRetCode = socketSetNonblocking(pNetworkContext->Socket, 0);
	if (nRetCode < 0) {
		NETWORK_ERR(("%s set socket blocking failed.\n", __func__));
	}
	NETWORK_DBG(("%s set socket(%d) blocking time. sendTimeoutMs: %d, recvTimeoutMs: %d\n", __func__,\
			pNetworkContext->Socket,sendTimeoutMs,recvTimeoutMs));
	nRetCode = setsockopt( pNetworkContext->Socket, SOL_SOCKET, SO_SNDTIMEO, (void *)&sendTimeoutMs,
								   ( socklen_t ) sizeof( sendTimeoutMs ) );
	if (nRetCode < 0) {
		NETWORK_ERR(("%s setsocket SO_SNDTIMEO failed.\n", __func__));
	}

	nRetCode = setsockopt( pNetworkContext->Socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&recvTimeoutMs,
								   ( socklen_t ) sizeof( recvTimeoutMs ) );
	if (nRetCode < 0) {
		NETWORK_ERR(("%s setsocket SO_SNDTIMEO failed.\n", __func__));
	}
#endif
	// Resolve the host name
	nRetCode = HostByName(pServerInfo->pHostName, &Address);
	if (!Address) {
		NETWORK_ERR(("%s dns error. hostname(%s) can not be resolved!!!\n",__func__,\
				pServerInfo->pHostName));
		goto errorExit;
	}
	NETWORK_DBG(("%s hostname: %s(%d.%d.%d.%d) - port: %d\n",__func__, pServerInfo->pHostName,\
			((uint8_t *)&Address)[0],((uint8_t *)&Address)[1],\
			((uint8_t *)&Address)[2],((uint8_t *)&Address)[3],\
			pServerInfo->port));

    memset(&ServerAddress, 0, sizeof(struct sockaddr_in));
	ServerAddress.sin_family        = AF_INET;
	ServerAddress.sin_addr.s_addr   = Address;       // Server's address
    ServerAddress.sin_port          = htons(pServerInfo->port);

	if((pNetworkContext->Flags & CLIENT_FLAG_SECURE) == CLIENT_FLAG_SECURE) {
#if SUPPORT_SSL
		nRetCode = WrapperSSLConnect(pNetworkContext->Socket, \
						(struct sockaddr *)&ServerAddress,	// Server address
						sizeof(struct sockaddr_in),	// Length of server address structure
			pServerInfo->pHostName, pNetworkContext->verifyMode);
#endif
		if (nRetCode < 0) goto errorExit;
#if SUPPORT_SSL
		nRetCode = WrapperSSLNegotiate(pNetworkContext->Socket, 0, 0, pServerInfo->pHostName);
#endif
		if (nRetCode < 0) goto errorExit;
	} else {
		nRetCode = connect(pNetworkContext->Socket,	// Socket
						(struct sockaddr *)&ServerAddress,	// Server address
						sizeof(struct sockaddr_in));	// Length of server address structure
		if(nRetCode < 0) {
#if SOCKET_NOBLOCKING
			if (SocketGetErr() == EINPROGRESS) {
				NETWORK_DBG(("%s connect server:hostname: %s - port: %d. Operation now in progress...\n",\
						__func__, pServerInfo->pHostName, pServerInfo->port));
				return EXIT_SUCCESS;
			}
#endif
			goto errorExit;
		}
	}
	NETWORK_DBG(("%s connect server:hostname: %s - port: %d successfully!\n",__func__, \
			pServerInfo->pHostName, pServerInfo->port));
	return EXIT_SUCCESS;
errorExit:
	NETWORK_ERR(("%s connect server failed.\n",__func__));
	if (pNetworkContext->Socket >= 0) {
		close(pNetworkContext->Socket);
	}
	return EXIT_FAILURE;
}

int32_t portTransportSend( NetworkContext_t * pNetworkContext,
                                     const void * pBuffer,
                                     size_t bytesToSend )
{
    int32_t bytesSent = 0;
    int32_t nSocketEvents, nRetCode = -1;
	struct timeval Timeval = {5, (100 * 1000)};

	NETWORK_DBG(("%s send...\n",__func__));
	FD_ZERO(&pNetworkContext->FDError);
	FD_ZERO(&pNetworkContext->FDWrite);

	FD_SET(pNetworkContext->Socket, &pNetworkContext->FDError);
	FD_SET(pNetworkContext->Socket, &pNetworkContext->FDWrite);

	nSocketEvents = select((pNetworkContext->Socket + 1), 0, \
							&pNetworkContext->FDWrite, \
							&pNetworkContext->FDError, \
							&Timeval);

	if (nSocketEvents < 0) {
		return -1;
	}
	if (nSocketEvents == 0) {
		NETWORK_ERR(("%s socket tx buf is fully.\n",__func__));
		return 0;
	}
	// We had a socket related error
	if(FD_ISSET(pNetworkContext->Socket, &pNetworkContext->FDError)) {
		FD_CLR((uint32_t)pNetworkContext->Socket,&pNetworkContext->FDError);
		NETWORK_ERR(("%s socket error.\n",__func__));
		return -1;
	}
	// Socket is writable (we are connected) so send the data
	if(FD_ISSET(pNetworkContext->Socket, &pNetworkContext->FDWrite)) {
		FD_CLR((int32_t)pNetworkContext->Socket,&pNetworkContext->FDWrite);
		// Send the data
		if ((pNetworkContext->Flags & CLIENT_FLAG_SECURE) == CLIENT_FLAG_SECURE) {
#if SUPPORT_SSL
			nRetCode = WrapperSSLSend(pNetworkContext->Socket, pBuffer, bytesToSend, 0);
#endif
		} else {
			nRetCode = send(pNetworkContext->Socket, pBuffer, bytesToSend, 0);
		}

		if(nRetCode < 0) {
			NETWORK_ERR(("%s error. retCode: %d errorCode: %d\n",__func__, nRetCode, SocketGetErr()));
			return -1;
		} else {
			bytesSent = nRetCode;
		}
	}

	NETWORK_DBG(("%s send bytes: %d\n",__func__, bytesSent));
    return bytesSent;
}

int32_t portTransportRecv( NetworkContext_t * pNetworkContext,
                                     void * pBuffer,
                                     size_t bytesToRecv )
{
    int32_t bytesReceived = 0, nSocketEvents, nRetCode = -1;
	struct timeval Timeval = {0, (3 * 1000)};

	NETWORK_DBG(("%s wanto recv bytes: %d recv...\n",__func__, bytesToRecv));
	if (((pNetworkContext->Flags & CLIENT_FLAG_SECURE) == CLIENT_FLAG_SECURE)
#if SUPPORT_SSL
	&& (WrapperSSLRecvPending(pNetworkContext->Socket))
#endif
		) {
		goto recvData;
	}
	FD_ZERO(&pNetworkContext->FDError);
	FD_ZERO(&pNetworkContext->FDRead);
	// Reset socket events
	FD_SET(pNetworkContext->Socket, &pNetworkContext->FDRead);
	FD_SET(pNetworkContext->Socket, &pNetworkContext->FDError);
	// See if we got any events on the socket
	nSocketEvents = select(pNetworkContext->Socket + 1, &pNetworkContext->FDRead,
					 0,
					 &pNetworkContext->FDError,
					 &Timeval);
	if(nSocketEvents < 0) {
		return -1;
	}
	if(nSocketEvents == 0) {
		NETWORK_DBG(("%s socket no data.\n",__func__));
		return 0; //no data. retry
	}
recvData:
	if((pNetworkContext->Flags & CLIENT_FLAG_SECURE) == CLIENT_FLAG_SECURE) {
#if SUPPORT_SSL
		nRetCode = WrapperSSLRecv(pNetworkContext->Socket, pBuffer, bytesToRecv, 0);
#endif
	} else {
		if(FD_ISSET(pNetworkContext->Socket, &pNetworkContext->FDError)) {
			FD_CLR((uint32_t)pNetworkContext->Socket,&pNetworkContext->FDError);
			return -1;
		}

		if(FD_ISSET(pNetworkContext->Socket, &pNetworkContext->FDRead)) {
			FD_CLR((uint32_t)pNetworkContext->Socket,&pNetworkContext->FDRead);
				nRetCode = recv(pNetworkContext->Socket, pBuffer, bytesToRecv, 0);
		}
	}

	if (nRetCode < 0) { //socket error
		NETWORK_ERR(("%s error. retCode: %d errorCode: %d\n",__func__, \
					nRetCode, SocketGetErr()));
		return -1;
	} else {
		bytesReceived = nRetCode;
	}

	NETWORK_DBG(("%s recv bytes: %d\n",__func__, bytesReceived));
    return bytesReceived;
}

