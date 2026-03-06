#ifndef __LP_COM_H__
#define __LP_COM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum RPMSG_LOG_STATE {
	DISABLE_LOG,
	ENABLE_LOG
};

#define LOG_INFO_ENABLE  1
#define LOG_WARN_ENABLE  1
#define LOG_ERR_ENABLE   1
#define LOG_DEBUG_ENABLE 1

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define COLOR_RESET "\033[0m"

#define TUYA_LOG_INFO(fmt, ...) \
	do { \
		if (LOG_INFO_ENABLE) \
			printf("[ty_lp][info]%s(%d):"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#define TUYA_LOG_WARN(fmt, ...) \
	do { \
		if (LOG_WARN_ENABLE) \
			printf(COLOR_YELLOW "[ty_lp][warn]%s(%d):"fmt COLOR_RESET, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#define TUYA_LOG_ERR(fmt, ...) \
	do { \
		if (LOG_ERR_ENABLE) \
			printf(COLOR_RED "[ty_lp][err]%s(%d):"fmt COLOR_RESET, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#define TUYA_LOG_DBG(fmt, ...) \
	do { \
		if (LOG_DEBUG_ENABLE) \
			printf(BG_YELLOW "[ty_lp][dbg]%s(%d):"fmt COLOR_RESET, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#define TUYA_RPMSG_LOG(fmt, ...) \
	do { \
		if (1) \
			printf("[tuya_rpmsg]%s(%d):"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while (0)

	int lp_crypt_hmac_sha256(unsigned char *pbuffer, int dlen,
		unsigned char *psecret, int slen,
		unsigned char *pdigest);

	int lp_crypt_aes_encrypt(unsigned char *pinput, int len, unsigned char *poutput, int *polen,
		unsigned char *pkey, unsigned char *piv);

	int lp_crypt_aes_decrypt(unsigned char *pinput, int len, unsigned char *poutput, int *polen,
		unsigned char *pkey, unsigned char *piv);

	void lp_system_random_str(unsigned char s[], int num);

	uint64_t lp_time_utc_ms();

	void lp_cli_print_hex(unsigned char *pdata, int len);

#ifdef __cplusplus
}
#endif

#endif
