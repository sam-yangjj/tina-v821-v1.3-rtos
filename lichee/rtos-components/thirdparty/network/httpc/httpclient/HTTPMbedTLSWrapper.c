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
#include "mbedtls.h"
#include "HTTPMbedTLSWrapper.h"
#include "lwip/opt.h"

#ifdef HTTPC_SSL

#define HTTP_CLIENT_CA
#define HTTPC_CERTIFICATE

#if defined(HTTP_CLIENT_CA)
#define CUSTOM_HTTPC_CRT_RSA                                            \
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
const char httpc_custom_cas_pem[] = CUSTOM_HTTPC_CRT_RSA;
const size_t httpc_custom_cas_pem_len = sizeof(httpc_custom_cas_pem);

#define HTTPC_CUSTOM_CAS_PEM          httpc_custom_cas_pem
#define HTTPC_CUSTOM_CAS_PEM_LEN      httpc_custom_cas_pem_len

#if 0
#if defined(HTTPC_CERTIFICATE)
#define HTTPC_CUSTOM_CA_PEM
#define HTTPC_CUSTOM_CA_PEM_LEN
#define HTTPC_CUSTOM_CRT_PEM
#define HTTPC_CUSTOM_CRT_PEM_LEN
#define HTTPC_CUSTOM_KEY
#define HTTPC_CUSTOM_KEY_LEN
#endif
#endif
#if defined(HTTPC_CERTIFICATE)
extern const char *mbedtls_test_srv_key;
extern const size_t mbedtls_test_srv_key_len;
extern const char *mbedtls_test_srv_crt;
extern const size_t mbedtls_test_srv_crt_len;

#define HTTPC_CUSTOM_CA_PEM           mbedtls_test_cas_pem
#define HTTPC_CUSTOM_CA_PEM_LEN       mbedtls_test_cas_pem_len
#define HTTPC_CUSTOM_CRT_PEM          mbedtls_test_srv_crt
#define HTTPC_CUSTOM_CRT_PEM_LEN      mbedtls_test_srv_crt_len
#define HTTPC_CUSTOM_KEY              mbedtls_test_srv_key
#define HTTPC_CUSTOM_KEY_LEN          mbedtls_test_srv_key_len
#endif
#else
extern const char mbedtls_test_cas_pem[];
extern const size_t mbedtls_test_cas_pem_len;

#define HTTPC_CUSTOM_CAS_PEM          mbedtls_test_cas_pem
#define HTTPC_CUSTOM_CAS_PEM_LEN      mbedtls_test_cas_pem_len

#if defined(HTTPC_CERTIFICATE)
extern const char *mbedtls_test_srv_key;
extern const size_t mbedtls_test_srv_key_len;
extern const char *mbedtls_test_srv_crt;
extern const size_t mbedtls_test_srv_crt_len;

#define HTTPC_CUSTOM_CA_PEM           mbedtls_test_cas_pem
#define HTTPC_CUSTOM_CA_PEM_LEN       mbedtls_test_cas_pem_len
#define HTTPC_CUSTOM_CRT_PEM          mbedtls_test_srv_crt
#define HTTPC_CUSTOM_CRT_PEM_LEN      mbedtls_test_srv_crt_len
#define HTTPC_CUSTOM_KEY              mbedtls_test_srv_key
#define HTTPC_CUSTOM_KEY_LEN          mbedtls_test_srv_key_len
#endif
#endif

#define NUM_CONN MEMP_NUM_NETCONN
static security_client client_param[NUM_CONN];
mbedtls_context *g_pContext[NUM_CONN];
mbedtls_sock g_httpc_net_fd[NUM_CONN];
/////////////////////////////////////////
#if defined(MBEDTLS_SSL_SESSION_TICKETS) && defined(CONFIG_COMPONENTS_MBEDTLS_2_16_0)
#define USE_SESSION_TICKET 1
#else
#define USE_SESSION_TICKET 0
#endif
#if USE_SESSION_TICKET
#define LOCK 0
#define TABLE_LOCK 1
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

static void session_lock(char* hostname)
{
#if LOCK
	XR_OS_Mutex_t *mutex = NULL;

	printf("[ST]%s . ====> HOSTNAME : %s \n", __func__, hostname);
	for (int i = 0; i < MAX_SERVER_NUM; i++) {
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

static void session_unlock(char* hostname)
{
#if LOCK
	XR_OS_Mutex_t *mutex = NULL;

	printf("[ST]%s . ====> HOSTNAME : %s \n", __func__, hostname);
	for (int i = 0; i < MAX_SERVER_NUM; i++) {
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

static void *table_find_session_byhostname(char *hostname)
{
	void *saved_session = NULL;

	printf("[ST]%s . ====> HOSTNAME : %s \n", __func__, hostname);
	for (int i = 0; i < MAX_SERVER_NUM; i++) {
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

static int table_find_free_info(void)
{
	int i;

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
	for (int i = 0; i < MAX_SERVER_NUM; i++) {
		printf("[ST]session info table[%d]: hostname-(%s) <-> session-(%p)\n", i, session_info[i].hostname, session_info[i].session);
	}
	printf("[ST]%s . <==== over.\n", __func__);
}

static int table_set_free_entry(void *session, char *hostname)
{
	int free_index;

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

static int table_update_entry(void *session, char *hostname)
{
	int i;

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


int HTTPWrapperSSLConnect(int s,const struct sockaddr *name,int namelen,char *hostname)
{
	int ret = 0;
	HC_DBG(("Https:connect.."));
	struct sockaddr *ServerAddress = (struct sockaddr *)name;
	int net_fd = s;
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
	if ((user_cert = HTTPC_obtain_user_certs()) == NULL) {
		HC_DBG(("https: config defaults certs.."));
		client_param[s].pCa = (char *)HTTPC_CUSTOM_CAS_PEM;
		client_param[s].nCa = HTTPC_CUSTOM_CAS_PEM_LEN;
#if defined(HTTPC_CERTIFICATE)
		client_param[s].certs.pCa = (char *) HTTPC_CUSTOM_CAS_PEM;
		client_param[s].certs.nCa = HTTPC_CUSTOM_CAS_PEM_LEN;
		client_param[s].certs.pCert = (char *) HTTPC_CUSTOM_CRT_PEM;
		client_param[s].certs.nCert = HTTPC_CUSTOM_CRT_PEM_LEN;
		client_param[s].certs.pKey = (char *) HTTPC_CUSTOM_KEY;
		client_param[s].certs.nKey = HTTPC_CUSTOM_KEY_LEN;
#endif
	} else {
		HC_DBG(("https: config user certs.."));
		memcpy(&client_param[s], user_cert, sizeof(client_param[s]));
	}

	int verify_mode = HTTPC_get_ssl_verify_mode();
	if (verify_mode != MBEDTLS_SSL_VERIFY_NONE && verify_mode != MBEDTLS_SSL_VERIFY_OPTIONAL &&
		verify_mode != MBEDTLS_SSL_VERIFY_REQUIRED && verify_mode != MBEDTLS_SSL_VERIFY_UNSET)
		verify_mode = MBEDTLS_SSL_VERIFY_NONE;
	if ((ret = mbedtls_config_context(pContext, (void *) &client_param[s], verify_mode)) != 0) {
		HC_ERR(("https: config failed.."));
		XR_OS_SetErrno(EFAULT);
		return -1;
	}

	if ((ret = mbedtls_connect(pContext, (mbedtls_sock*) &net_fd, ServerAddress, namelen, hostname)) != 0) {
		HC_ERR(("https: connect failed.."));
		return -1;
	}

	HC_DBG(("Https:connect ok.."));

	return ret;
}

int HTTPWrapperSSLNegotiate(int s,const struct sockaddr *name,int namelen,char *hostname)
{
	int ret = 0;
	g_httpc_net_fd[s].fd = s;
	HC_DBG(("Https:negotiate.."));

#if USE_SESSION_TICKET
	session_lock(hostname);
#endif
	if ((ret = mbedtls_handshake(g_pContext[s], &g_httpc_net_fd[s])) != 0) {
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

	HC_DBG(("Https:negotiate ok.."));
	return 0;
}

int HTTPWrapperSSLSend(int s,char *buf, int len,int flags)
{
	int ret = 0;
	HC_DBG(("Https:send.."));
	if ((ret = mbedtls_send(g_pContext[s], buf, len)) < 0)
		return -1;
	return ret;
}

int HTTPWrapperSSLRecv(int s,char *buf, int len,int flags)
{
	int ret = 0;
	if ((ret = mbedtls_recv(g_pContext[s], buf, len)) < 0)
		return -1;
	return ret;
}

int HTTPWrapperSSLRecvPending(int s)
{
	int ret = 0;
	ret = mbedtls_recv_pending(g_pContext[s]);
	HC_DBG(("Https:recv pending : %d (bytes)..", ret));
	return ret;
}

int HTTPWrapperSSLClose(int s)
{
	HC_DBG(("Https:close.."));
	mbedtls_deinit_context(g_pContext[s]);
	g_httpc_net_fd[s].fd = -1;
	s = -1;
	return 0;
}
#endif /* HTTPC_SSL */
