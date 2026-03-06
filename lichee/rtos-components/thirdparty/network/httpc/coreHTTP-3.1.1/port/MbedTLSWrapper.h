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

#ifndef MBEDTLS_WRAPPER_H_
#define MBEDTLS_WRAPPER_H_

#include <stdlib.h>
#include <stdint.h>
#include "netdb.h"
#include "mbedtls.h"

#ifndef SUPPORT_SSL
#define SUPPORT_SSL 1
#endif

#ifndef WPSSL_DEBUG_ON
#define WPSSL_DEBUG_ON 0
#endif

#if WPSSL_DEBUG_ON > 0
#define WPSSL_LOGD(fmt, arg...) printf("[TLS_WRAPPER][DBG]"fmt"\n", ##arg)
#else
#define WPSSL_LOGD(fmt, arg...)
#endif
#define WPSSL_LOGE(fmt, arg...) printf("[TLS_WRAPPER][ERR]"fmt"\n", ##arg)

#define WPSSL_DBG(x) do { WPSSL_LOGD x; } while (0)
#define WPSSL_ERR(x) do { WPSSL_LOGE x; } while (0)

extern int32_t FreeRTOS_errno;

inline int32_t SocketGetErr(void)
{
	return FreeRTOS_errno;
}

inline void SocketSetErr(int32_t err)
{
	FreeRTOS_errno = err;
}

static int32_t WrapperGetHostByName(const char *name, uint32_t *address);
#define HostByName WrapperGetHostByName

#if SUPPORT_SSL
static int32_t WrapperSSLConnect(int32_t s, struct sockaddr *name, int32_t namelen, const char *hostname, int32_t verifyMode);
static int32_t WrapperSSLNegotiate(int32_t s,struct sockaddr *name,int32_t namelen,const char *hostname);
static int32_t WrapperSSLSend(int32_t s, const char *buf, int32_t len,int32_t flags);
static int32_t WrapperSSLRecv(int32_t s, char *buf, int32_t len,int32_t flags);
static int32_t WrapperSSLRecvPending(int32_t s);
static int32_t WrapperSSLClose(int32_t s);

#define CLIENT_CA
#define CLIENT_CERTIFICATE

#if defined(CLIENT_CA)
#define CUSTOM_CRT_RSA                                            \
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"MIICtDCCAZwCAQAwDQYJKoZIhvcNAQELBQAwIDEeMBwGA1UEAxMVQ2VydGlmaWNh\r\n"  \
"dGUgQXV0aG9yaXR5MB4XDTE3MDcxOTExMzU1NVoXDTIyMDcxODExMzU1NVowIDEe\r\n"  \
"MBwGA1UEAxMVQ2VydGlmaWNhdGUgQXV0aG9yaXR5MIIBIjANBgkqhkiG9w0BAQEF\r\n"  \
"AAOCAQ8AMIIBCgKCAQEA3FPtvNnMiETM5qprK4625nf8z39HnM5pdkyjW3a4JWt4\r\n"  \
"RTuLv6x7b56OnJttZPL0hYyVwGsxt3LeuoXD3pBlLn61iwUK6Fb4NX5Xo04LaKl7\r\n"  \
"gQFQ+lENPPSu/JdzcERly0d1JYIQ4Z11732yCdblf5oZJ/0skUzhTVCusWnYvY3Z\r\n"  \
"xs/O3wjvbuCucxgZoyDv6AZ8ZQ0xKZ3JKjm8URD4yrRYEkQ+gBeTkNC3nVFJ/u8X\r\n"  \
"nrNT/qzhhI6HS8Lf88dkQu3W5gvIVVy5qv49hqpWGXwbEkrIsMgC9OJCZ5qoo17Y\r\n"  \
"iUj/SBH6xVA9ikaFOlVeH9rD1euzwgjIm+X2Wu7S5wIDAQABMA0GCSqGSIb3DQEB\r\n"  \
"CwUAA4IBAQCt9EYWA2vTVJmEajB87MzHHvjTV/cTWRGLKnLoBHL3OKf+Lembmu6Q\r\n"  \
"YxoN8OCqRjrcvh0sgQ1H6jpfmVSIoLKoT9BVy16t5PZ8x0XSSrMlXQKz+pAuOZBc\r\n"  \
"sinSoRPDMr0M92m2CAnJ8mIpr6o0lTtWJfY1xeuT2+LbFzvaI6dtWnQYmN1mxPqA\r\n"  \
"JLhAlQhCqmRiDhgFPfKSwKsmEPdUtrA2InOsgxDGa0utawK2BWgQc6hkhT9uZ4dF\r\n"  \
"DxLkNG8w4QFnHXcm8pdpg5zbO7GSrtPRs2CU2fMpE79CMPKSH3ErxV2F4aPtHFP5\r\n"  \
"sV1Ai+iyFoUKCzjW4iUDTJux2gUTzpmH\r\n"                                  \
"-----END CERTIFICATE-----\r\n"

/* Concatenation of all available CA certificates */
static const char custom_cas_pem[] = CUSTOM_CRT_RSA;
static const size_t custom_cas_pem_len = sizeof(custom_cas_pem);

#define CUSTOM_CAS_PEM          custom_cas_pem
#define CUSTOM_CAS_PEM_LEN      custom_cas_pem_len
#endif

#if defined(CLIENT_CERTIFICATE)
extern const char *mbedtls_test_srv_key;
extern const size_t mbedtls_test_srv_key_len;
extern const char *mbedtls_test_srv_crt;
extern const size_t mbedtls_test_srv_crt_len;

#define CUSTOM_CRT_PEM         mbedtls_test_srv_crt
#define CUSTOM_CRT_PEM_LEN     mbedtls_test_srv_crt_len
#define CUSTOM_KEY             mbedtls_test_srv_key
#define CUSTOM_KEY_LEN         mbedtls_test_srv_key_len
#endif

#define NUM_CONN 20 //MAX SOCKET NUM
static security_client client_param[NUM_CONN];
static mbedtls_context *g_pContext[NUM_CONN];
static mbedtls_sock g_net_fd[NUM_CONN];
/////////////////////////////////////////
#if defined(MBEDTLS_SSL_SESSION_TICKETS) && defined (CONFIG_COMPONENTS_MBEDTLS_2_16_0)
#define USE_SESSION_TICKET 1
#else
#define USE_SESSION_TICKET 0
#endif
#define LOCK 0
#define TABLE_LOCK 1
#if USE_SESSION_TICKET
#if LOCK || TABLE_LOCK
#include <kernel/os/os_mutex.h>
static XR_OS_Mutex_t table_mutex;
#endif

#define MAX_HOSTNAME_LEN 64
#define MAX_SERVER_NUM 8
typedef struct {
	char hostname[MAX_HOSTNAME_LEN];
	void *session;
#if LOCK || TABLE_LOCK
	XR_OS_Mutex_t mutex;
#endif
} save_session_t;

static save_session_t session_info[MAX_SERVER_NUM];

static void session_lock(const char* hostname)
{
#if LOCK
	XR_OS_Mutex_t *mutex = NULL;

	printf("[ST]%s . ====> HOSTNAME : %s \n", __func__, hostname);
	for (int32_t i = 0; i < MAX_SERVER_NUM; i++) {
		printf("[ST]session info table[%d]: hostname-(%s) <-> mutex-(%p)\n", i, session_info[i].hostname, session_info[i].mutex.handle);
		if(!XR_OS_MutexIsValid(&session_info[i].mutex)) {
			if (XR_OS_RecursiveMutexCreate(&session_info[i].mutex) == XR_OS_FAIL) {
				printf("[ST]table lock creat failed!.\n");
				return ;
			}
		}
		if(memcmp(session_info[i].hostname, hostname, strlen(hostname)) == 0) {
			printf("[ST]find mutex(%p) with the hostname : %s\n", session_info[i].mutex.handle, hostname);
			mutex = &session_info[i].mutex;
			break;
		}
	}

	if (mutex) {
		XR_OS_RecursiveMutexLock(mutex, XR_OS_WAIT_FOREVER);
		printf("[ST]%s . <==== over.\n", __func__);
	} else {
		printf("[ST]%s . <==== failed. do not lock session\n", __func__);
	}
#endif
}

static void session_unlock(const char* hostname)
{
#if LOCK
	XR_OS_Mutex_t *mutex = NULL;

	printf("[ST]%s . ====> HOSTNAME : %s \n", __func__, hostname);
	for (int32_t i = 0; i < MAX_SERVER_NUM; i++) {
		printf("[ST]session info table[%d]: hostname-(%s) <-> mutex-(%p)\n", i, session_info[i].hostname, session_info[i].mutex.handle);
		if(XR_OS_MutexIsValid(&session_info[i].mutex)) {
			if(memcmp(session_info[i].hostname, hostname, strlen(hostname)) == 0) {
				printf("[ST]find mutex(%p) with the hostname : %s\n", session_info[i].mutex.handle, hostname);
				mutex = &session_info[i].mutex;
				break;
			}
		}
	}

	if (mutex) {
		XR_OS_RecursiveMutexUnlock(mutex);
		printf("[ST]%s . <==== over.\n", __func__);
	} else {
		printf("[ST]%s . <==== failed. can not find mutex of the hostname\n", __func__);
	}
#endif
}

static void table_lock(void)
{
#if TABLE_LOCK
	XR_OS_Mutex_t *mutex = &table_mutex;

	if (!XR_OS_MutexIsValid(mutex)) {
		if (XR_OS_RecursiveMutexCreate(mutex) == XR_OS_FAIL) {
			printf("[ST]table lock creat failed!.\n");
			return ;
		}
	}

	XR_OS_RecursiveMutexLock(mutex, XR_OS_WAIT_FOREVER);
#endif
}

static void table_unlock(void)
{
#if TABLE_LOCK
	XR_OS_Mutex_t *mutex = &table_mutex;

	XR_OS_RecursiveMutexUnlock(mutex);
#endif
}

static void *table_find_session_byhostname(const char *hostname)
{
	void *saved_session = NULL;

	printf("[ST]%s . ====> HOSTNAME : %s \n", __func__, hostname);
	for (int32_t i = 0; i < MAX_SERVER_NUM; i++) {
		printf("[ST]session info table[%d]: hostname-(%s) <-> session-(%p)\n", i, session_info[i].hostname, session_info[i].session);
		if(session_info[i].session != NULL) {
			if (memcmp(session_info[i].hostname, hostname, strlen(hostname)) == 0) {
				printf("[ST]find old seesion(%p) with same hostname : %s\n", session_info[i].session, hostname);
				saved_session = session_info[i].session;
				break;
			}
		}
	}
	printf("[ST]%s . <==== over.\n", __func__);

	return saved_session;
}

static int32_t table_find_free_info(void)
{
	int32_t i;

	printf("[ST]%s . ====> find.\n", __func__);
	for (i = 0; i < MAX_SERVER_NUM; i++) {
		printf("[ST]session info table[%d]: hostname-(%s) <-> session-(%p)\n", i, session_info[i].hostname, session_info[i].session);
		if(session_info[i].session == NULL) {
			break;
		}
	}
	if (i >= MAX_SERVER_NUM) {
		printf("[ST]%s . <==== failed. no free entry\n", __func__);
		return -1;
	}
	printf("[ST]%s . <==== over.\n", __func__);
	return i;
}

static void table_info_show(void)
{
	printf("[ST]%s . ====> show.\n", __func__);
	for (int32_t i = 0; i < MAX_SERVER_NUM; i++) {
		printf("[ST]session info table[%d]: hostname-(%s) <-> session-(%p)\n", i, session_info[i].hostname, session_info[i].session);
	}
	printf("[ST]%s . <==== over.\n", __func__);
}

static int32_t table_set_free_entry(void *session, const char *hostname)
{
	int32_t free_index;

	table_lock();
	printf("[ST]%s . ====> HOSTNAME : %s \n", __func__, hostname);
	free_index = table_find_free_info();
	if(free_index >= 0 && free_index < MAX_SERVER_NUM) {
		printf("[ST]table has free space, set table[%d]\n", free_index);
		session_info[free_index].session = session;
		memset(session_info[free_index].hostname, 0, MAX_HOSTNAME_LEN);
		memcpy(session_info[free_index].hostname, hostname, strlen(hostname));
	} else {
		printf("[ST]set failed, table is full!!\n");
		printf("[ST]%s . <==== failed.\n", __func__);
		table_info_show();
		table_unlock();
		return -1;
	}
	table_info_show();
	printf("[ST]%s . <==== over.\n", __func__);
	table_unlock();
	return 0;
}

static int32_t table_update_entry(void *session, const char *hostname)
{
	int32_t i;

	printf("[ST]%s . ====> HOSTNAME : %s \n", __func__, hostname);
	for (i = 0; i < MAX_SERVER_NUM; i++) {
		if(session_info[i].session != NULL) {
			if(memcmp(session_info[i].hostname, hostname, strlen(hostname)) == 0) {
				printf("[ST]find old seesion with same hostname : %s\n", hostname);
				printf("[ST]update old session(%p) -> new session(%p)\n", session_info[i].session, session);
				session_info[i].session = session;
				break;
			}
		}
	}
	if (i >= MAX_SERVER_NUM) {
		printf("[ST]%s . <==== failed.\n", __func__);
		return -1;
	}

	printf("[ST]%s . <==== over.\n", __func__);
	return 0;
}
#endif
////////////////////////////////////////

static int32_t WrapperSSLConnect(int32_t s,struct sockaddr *name,int32_t namelen,const char *hostname,int32_t verifyMode)
{
	int32_t ret = 0;
	WPSSL_DBG(("connect.."));
	struct sockaddr *ServerAddress = (struct sockaddr *)name;
	int32_t net_fd = s;
	/* Init client context */
	mbedtls_context *pContext = (mbedtls_context *)mbedtls_init_context(0);
	if (!pContext || !ServerAddress)
		return -1;

///////////////////////////////////////////
#if USE_SESSION_TICKET
	void *saved_session = NULL;

	if (strlen(hostname) < MAX_HOSTNAME_LEN) {
		saved_session = table_find_session_byhostname(hostname);
		if (saved_session == NULL && table_find_free_info() >= 0) { //not find and has free space
			saved_session = mbedtls_init_session_reconnect();
			if (saved_session) {
				table_set_free_entry(saved_session, hostname);
			}
		}
	}
	if(saved_session != NULL)
	{
		if(mbedtls_set_saved_session(pContext, saved_session) == 0)
		{
			printf("set saved session success, Reconnect to the server by using the session ticket\n");
		}
		else
		{
			printf("set saved session failed, Connect to the server normally\n");
		}
	} else {
		printf("no use session ticket, Connect to the server normally\n");
		mbedtls_set_saved_session(pContext, NULL);
	}
#endif
//////////////////////////////////////////
	g_pContext[s] = pContext;

	memset(&client_param[s], 0, sizeof(client_param[s]));

	security_client *user_cert = NULL;
	if (user_cert == NULL) {
		WPSSL_DBG(("config defaults certs.."));
		client_param[s].pCa = (char *)CUSTOM_CAS_PEM;
		client_param[s].nCa = CUSTOM_CAS_PEM_LEN;
#if defined(CLIENT_CERTIFICATE)
		client_param[s].certs.pCa = (char *) CUSTOM_CAS_PEM;
		client_param[s].certs.nCa = CUSTOM_CAS_PEM_LEN;
		client_param[s].certs.pCert = (char *) CUSTOM_CRT_PEM;
		client_param[s].certs.nCert = CUSTOM_CRT_PEM_LEN;
		client_param[s].certs.pKey = (char *) CUSTOM_KEY;
		client_param[s].certs.nKey = CUSTOM_KEY_LEN;
#endif
	} else {
		WPSSL_DBG(("config user certs.."));
		memcpy(&client_param[s], user_cert, sizeof(client_param[s]));
	}

	int32_t verify_mode = verifyMode;
	if (verify_mode != MBEDTLS_SSL_VERIFY_NONE && verify_mode != MBEDTLS_SSL_VERIFY_OPTIONAL &&
		verify_mode != MBEDTLS_SSL_VERIFY_REQUIRED && verify_mode != MBEDTLS_SSL_VERIFY_UNSET)
		verify_mode = MBEDTLS_SSL_VERIFY_NONE;
	if ((ret = mbedtls_config_context(pContext, (void *) &client_param[s], verify_mode)) != 0) {
		WPSSL_ERR(("config failed.."));
		return -1;
	}

	if ((ret = mbedtls_connect(pContext, (mbedtls_sock*) &net_fd, ServerAddress, namelen, (char *)hostname)) != 0) {
		WPSSL_ERR(("connect failed.."));
		return -1;
	}

	WPSSL_DBG(("connect ok.."));

	return ret;
}

static int32_t WrapperSSLNegotiate(int32_t s,struct sockaddr *name,int32_t namelen, const char *hostname)
{
	int32_t ret = 0;
	g_net_fd[s].fd = s;
	WPSSL_DBG(("negotiate.."));

#if USE_SESSION_TICKET
	session_lock(hostname);
#endif
	if ((ret = mbedtls_handshake(g_pContext[s], &g_net_fd[s])) != 0) {
		///////////////////////////////////////////
#if USE_SESSION_TICKET
		void *saved_session = NULL;

		if (strlen(hostname) < MAX_HOSTNAME_LEN) {
			saved_session = table_find_session_byhostname(hostname);
			if(saved_session != NULL)
			{
				mbedtls_deinit_session_reconnect(g_pContext[s], saved_session);
				table_update_entry(NULL, hostname);
				saved_session = NULL;
			}
		}
#endif
		/////////////////////////////////////////
#if USE_SESSION_TICKET
		session_unlock(hostname);
#endif
		return -1;
	}
#if USE_SESSION_TICKET
	session_unlock(hostname);
#endif

	WPSSL_DBG(("negotiate ok.."));
	return 0;
}

static int32_t WrapperSSLSend(int32_t s, const char *buf, int32_t len,int32_t flags)
{
	int32_t ret = 0;
	WPSSL_DBG(("send.."));
	if ((ret = mbedtls_send(g_pContext[s], (char *)buf, len)) <= 0)
		return -1;
	return ret;
}

static int32_t WrapperSSLRecv(int32_t s,char *buf, int32_t len,int32_t flags)
{
	int32_t ret = 0;
	WPSSL_DBG(("recv.."));
	if ((ret = mbedtls_recv(g_pContext[s], buf, len)) <= 0)
		return -1;
	return ret;
}

static int32_t WrapperSSLRecvPending(int32_t s)
{
	int32_t ret = 0;
	ret = mbedtls_recv_pending(g_pContext[s]);
	WPSSL_DBG(("recv pending : %d (bytes)..", ret));
	return ret;
}

static int32_t WrapperSSLClose(int32_t s)
{
	WPSSL_DBG(("close.."));
	mbedtls_deinit_context(g_pContext[s]);
	g_net_fd[s].fd = -1;
	s = -1;
	return 0;
}
#endif /* SUPPORT_SSL */

static int32_t WrapperGetHostByName(const char *name, uint32_t *address)
{
	struct  hostent     *HostEntry;
	int32_t iPos = 0, iLen = 0,iNumPos = 0,iDots =0;
	long    iIPElement;
	char    c = 0;
	char    Num[4];
	int32_t iHostType = 0; // 0 : numeric IP

	// Check if the name is an IP or host
	iLen = strlen(name);
	for(iPos = 0; iPos <= iLen;iPos++)
	{
		c = name[iPos];
		if((c >= 48 && c <= 57)  || (c == '.') )
		{
			// c is numeric or dot
			if(c != '.')
			{
				// c is numeric
				if(iNumPos > 3)
				{
					iHostType++;
					break;
				}
				Num[iNumPos] = c;
				Num[iNumPos + 1] = 0;
				iNumPos ++;
			}
			else
			{
				iNumPos = 0;
				iDots++;
				iIPElement = atol(Num);
				if(iIPElement > 256 || iDots > 3)
				{
					return 0; // error invalid IP
				}
			}
		}
		else
		{

			break; // this is an alpha numeric address type
		}
	}

	if(c == 0 && iHostType == 0 && iDots == 3)
	{
		iIPElement = atol(Num);
		if(iIPElement > 256)
		{
			return 0; // error invalid IP
		}
	}
	else
	{
		iHostType++;
	}

	if(iHostType > 0)
	{
		HostEntry = gethostbyname(name);
		if(HostEntry)
		{

			*(address) = *((u_long*)HostEntry->h_addr_list[0]);

			//*(address) = (unsigned long)HostEntry->h_addr_list[0];
			return 1; // Error
		}
		else
		{
			return 0; // OK
		}
	}

	else // numeric address - no need for DNS resolve
	{
		*(address) = inet_addr(name);
		return 1;

	}
	return 0;
}

#endif /* MBEDTLS_WRAPPER_H_ */
