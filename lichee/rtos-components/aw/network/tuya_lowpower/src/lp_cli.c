#include "lp_cli.h"

int lp_cli_set_socket_o_block(int sock, int block)
{
	int flags = 0;
	printf("lp_cli_set_socket_o_block set mode = %s\n", block ? "block" : "nonblock");
	if (block) { //block
		flags = lwip_fcntl(sock, F_GETFL, 0);
		flags |= O_NONBLOCK;
		if (lwip_fcntl(sock, F_SETFL, flags) == -1) {
			printf("Error setting socket block mode: %s\n", strerror(errno));
			return -1;
		}
	} else {  //non block
		flags = lwip_fcntl(sock, F_GETFL, 0);
		flags &= ~O_NONBLOCK;
		if (lwip_fcntl(sock, F_SETFL, flags) == -1) {
			printf("Error setting socket block mode: %s\n", strerror(errno));
			return -1;
		}
	}
	return 0;
}

int lp_cli_set_socket_timeout(int sock, int ms)
{
	struct timeval tout;
	tout.tv_sec = ms;
	tout.tv_usec = 0;

	if (lwip_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tout, sizeof(tout)) < 0) {
		perror("Failed to set send timeout");
	} else {
		printf("Send timeout set to 5 seconds.\n");
	}

	if (lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout)) < 0) {
		perror("Failed to set receive timeout");
	} else {
		printf("Receive timeout set to 5 seconds.\n");
	}
	return 0;
}

int lp_cli_connect_server(TUYA_IP_ADDR_T hostaddr, int port)
{
	int sock = -1;
	int ret = 0;
	int domain = hostaddr.type == AF_INET ? AF_INET : AF_INET6;
	sock = socket(domain, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("socket error: %s\n", strerror(errno));
		return -1;
	}

	/* close Nagle */
	int flags = 1;
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags))) {
		printf("Error close Nagle: %s\n", strerror(errno));
	}

	/* enable fast ack */
	if (setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, &flags, sizeof(flags))) {
		printf("Error enable Quickack: %s\n", strerror(errno));
	}

	// if (lp_cli_set_socket_o_block(sock, 0) == -1) {
	// 	printf("lp_cli_set_socket_o_block error: %s\n", strerror(errno));
	// }

	struct sockaddr_in addr4;
	memset(&addr4, 0, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_addr.s_addr = htonl(hostaddr.u_addr.ip4);
	addr4.sin_port = htons(port);
	ret = connect(sock, (struct sockaddr *)&addr4, sizeof(addr4));
	if (ret < 0) {
		printf("connect error!=%s\n", strerror(errno));
		close(sock);
		sock = -1;
		return -1;
	}

	// lp_cli_set_socket_timeout(sock, 8);
	return sock;
}

int lp_cli_auth_request(int sock, lp_auth_dev_msg_s *pdevMsg,
	lp_auth_info_s *preqauth)
{
	int ret = 0;
	lp_protocol_hdr_s hdr;
#ifdef CONFIG_COMPONENTS_PM
	/* use malloc to save memory */
	int len = 512;
	int reql = 512;
	unsigned char *data = NULL;
	char *reqd = NULL;
	data = malloc(len);
	if (!data) {
		printf("malloc data error\n");
		ret = -1;
		goto free_return;
	}
	reqd = malloc(reql);
	if (!reqd) {
		printf("malloc reqd error\n");
		ret = -1;
		goto free_return;
	}
#else
	unsigned char data[512] = { 0 };
	int len = sizeof(data);
	char reqd[512] = { 0 };
	int reql = sizeof(reqd);
#endif
	memset(&hdr, 0, sizeof(lp_protocol_hdr_s));
	memset(preqauth, 0, sizeof(lp_auth_info_s));

	snprintf(preqauth->time, sizeof(preqauth->time), "%lld", lp_time_utc_ms());
	lp_system_random_str((unsigned char *)preqauth->random, LP_AUTH_RANDOM_MAX_LEN);

	lp_auth_request(pdevMsg, 1, 1, preqauth, reqd, &reql);
	lp_protocol_set_auth(data, &len, &hdr, pdevMsg, reqd, reql);

	// printf("auth data is\n { ");
	// int i = 0;
	// for (i = 0; i < len; i++) {
	// 	printf("0x%x ", data[i]);
	// }
	// printf("}\n");

	ssize_t sz = send(sock, data, len, 0);
	if (sz != len) {
		printf("sz: %d, errno: %d\n", sz, errno);
		ret = -1;
		goto free_return;
	}
	printf("auth send ok\n");

free_return:
#ifdef CONFIG_COMPONENTS_PM
	if (data) free(data);
	if (reqd) free(reqd);
#endif
	return ret;
}

// signed int lp_cli_recv(int sock, void *buf, unsigned int size)
// {
// 	int count = 0;
// 	signed int ret = 0;
// 	unsigned int rd_size = 0;
// 	printf("lp_cli_recv in\n");
// 	while (rd_size < size) {
// 		ret = recv(sock, buf + rd_size, size - rd_size, 0);
// 		if (ret <= 0) {
// 			if (EWOULDBLOCK == errno || EINTR == errno || EAGAIN == errno) {
// 				printf("recv is error =%d\n", errno);
// 				usleep(500 * 1000);
// 				if (count++ > 10) {
// 					break;
// 				}
// 				continue;
// 			}
// 			break;
// 		}
// 		rd_size += ret;
// 	}
// 	printf("recv ok\n");
// 	lp_cli_set_socket_timeout(sock, 0);
// 	printf("lp_cli_set_socket_timeout 0\n");
// 	return rd_size;
// }

/* tmp */
signed int lp_cli_recv(int sock, void *buf, unsigned int size)
{
	signed int ret = 0;
	unsigned int rd_size = 0;
again:  // TODO : max times
	ret = recv(sock, buf + rd_size, size - rd_size, 0);
	if (ret <= 0) {
		if (EWOULDBLOCK == errno || EINTR == errno || EAGAIN == errno) {
			printf("recv is error =%d\n", errno);
			goto again;
		}
	}
	rd_size += ret;
	printf("auth recv ok\n");
	return rd_size;
}

signed int lp_cli_recv_timeout(int sock, void *buf, unsigned int size, int timeout)
{
	signed int ret = 0;
	unsigned int rd_size = 0;
	fd_set rfds;
	struct timeval tv;
	int retval, maxfd = -1;
again:  // TODO : max times
	FD_ZERO(&rfds);
	maxfd = sock;
	FD_SET(sock, &rfds);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
	if (retval > 0) {
		ret = recv(sock, buf + rd_size, size - rd_size, 0);
		if (ret <= 0) {
			if (EWOULDBLOCK == errno || EINTR == errno || EAGAIN == errno) {
				printf("recv is error =%d\n", errno);
				goto again;
			}
		}
		rd_size += ret;
		printf("auth recv ok, rd_size = %d\n", rd_size);
	} else if (retval <= 0) {
		if (retval == 0)
			printf("auth recv timeout, retry now\n");
		else
			printf("auth recv error, retry now\n");
		return -1;
	}
	return rd_size;
}

int lp_cli_auth_response(int sock, lp_auth_dev_msg_s *pdevMsg, lp_auth_response_s *prsp)
{
	lp_protocol_hdr_s hdr;
	int ret = 0;
#ifdef CONFIG_COMPONENTS_PM
	int message_len = 1024;
	int reqlen = 256;
	char *message = NULL;
	char *reqdata = NULL;

	message = malloc(message_len);
	if (!message) {
		printf("malloc message error\n");
		ret = -1;
		goto free_return;
	}

	reqdata = malloc(reqlen);
	if (!reqdata) {
		printf("malloc reqdata error\n");
		ret = -1;
		goto free_return;
	}
#else
	char message[1024] = { 0 };
	char reqdata[256] = { 0 };
	int reqlen = sizeof(reqdata);
	int message_len = sizeof(message);
#endif
	memset(&hdr, 0, sizeof(lp_protocol_hdr_s));
	memset(prsp, 0, sizeof(lp_auth_response_s));
	int rlen = lp_cli_recv_timeout(sock, message, message_len, 10);
	if (rlen <= 0) {
		printf("recv auth response error, errno: %s\n", strerror(errno));
		ret = -1;
		goto free_return;
	}

	lp_protocol_parse_auth((unsigned char *)message, rlen, &hdr,
		pdevMsg, reqdata, &reqlen);

	if (hdr.version != LP_VERSION_1) {
		printf("version not match!\n");
		ret = -1;
		goto free_return;
	}

	if (lp_auth_parse_response(reqdata, strlen(reqdata), prsp) != 0) {
		printf("parse response error\n");
		ret = -1;
		goto free_return;
	}

free_return:
#ifdef CONFIG_COMPONENTS_PM
	if (message) free(message);
	if (reqdata) free(reqdata);
#endif
	return ret;
}

int lp_cli_init_dev_info(lp_auth_dev_msg_s *pmsg, char *pdevId, int idLen,
	char *pkey, int keyLen)
{
	unsigned char aesdata[128] = { 0 };
	int len = sizeof(aesdata);
	unsigned char key[16] = { 0x23, 0xac, 0x7b, 0x15, 0x0d, 0x89, 0x34, 0x92,
				 0xf1, 0x19, 0x33, 0xde, 0xc8, 0x6a, 0x10, 0x55 };
	unsigned char iv[16] = { 0x1e, 0x25, 0x77, 0xb8, 0x66, 0xc1, 0x10, 0x33,
				0x93, 0x69, 0xcb, 0xa8, 0x2c, 0x54, 0xe5, 0xab };

	memset(pmsg, 0, sizeof(lp_auth_dev_msg_s));

	memcpy(pmsg->key, pkey, keyLen);

	pmsg->keyLen = keyLen;
	pmsg->idLen = sizeof(pmsg->devId);
	printf("origin devid is =%s \n", pdevId);
	lp_crypt_aes_encrypt((unsigned char *)pdevId, idLen, aesdata, &len, key, iv);

	// printf("encrypted devid is = ");
	// for (int i = 0; i < len; i++) {
	// 	printf("%02x", aesdata[i]);
	// }
	// printf("\n");

	if (mbedtls_base64_encode(pmsg->devId, pmsg->idLen,
		(size_t *)&pmsg->idLen, aesdata, len) != 0) {
		printf("base64 encode error\n");
		return -1;
	}

	printf("base64 devid is =  %s\n", pmsg->devId);

	return 0;
}

int lp_cli_auth(int sock, lp_auth_dev_msg_s *pdevMsg)
{
	int ret = -1;
	lp_auth_info_s reqauth;
	lp_auth_response_s rsp;

	printf("begin___\n");

	memset(&reqauth, 0, sizeof(lp_auth_info_s));
	memset(&rsp, 0, sizeof(lp_auth_response_s));

	do {
		if (lp_cli_auth_request(sock, pdevMsg, &reqauth) < 0) {
			break;
		}

		if (lp_cli_auth_response(sock, pdevMsg, &rsp) < 0) {
			break;
		}

		if (lp_auth_check_response(pdevMsg, &reqauth, &rsp)) {
			ret = 0;
		}
	} while (0);

	printf("end___, auth status: %d\n", ret);

	return ret;
}

int lp_cli_wakeup(char *pdata, unsigned int *plen, int wk)
{
	return lp_protocol_wakeup(pdata, (int *)plen, wk);
}

int lp_cli_heartbeat(char *pdata, unsigned int *plen)
{
	return lp_protocol_heartbeat(pdata, (int *)plen);
}
int lp_cli_wake_up_data_seed_computer(char *key, unsigned int len)
{
	if (key == NULL) {
		return -1;
	}
	return hash_crc32i_total(key, len);
}
