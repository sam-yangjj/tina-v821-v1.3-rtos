#ifndef __OPENAMP_SUNXI_HELPER_OPENAMP_LOG_H__
#define __OPENAMP_SUNXI_HELPER_OPENAMP_LOG_H__

#include <stdio.h>
#include <hal_log.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef printfFromISR
#define printfFromISR printf
#endif

/* #define OPENAMP_DBG */

#ifdef CONFIG_ARCH_DSP
#define AMP_LOG_PREFIX			"[DSP] "
#endif
#ifdef CONFIG_ARCH_RISCV
#define AMP_LOG_PREFIX			"[RV] "
#endif
#ifdef CONFIG_ARCH_ARM
#define AMP_LOG_PREFIX			"[ARM] "
#endif

#define AMP_LOG_COLOR_NONE		"\e[0m"
#define AMP_LOG_COLOR_RED		"\e[31m"
#define AMP_LOG_COLOR_GREEN		"\e[32m"
#define AMP_LOG_COLOR_YELLOW	"\e[33m"
#define AMP_LOG_COLOR_BLUE		"\e[34m"

#ifdef OPENAMP_DBG
#define openamp_dbg(fmt, args...) \
	printf(AMP_LOG_COLOR_GREEN AMP_LOG_PREFIX "[AMP_DBG][%s:%d]" \
		AMP_LOG_COLOR_NONE fmt, __func__, __LINE__, ##args)
#define openamp_dbgFromISR(fmt, args...) \
	printfFromISR(AMP_LOG_COLOR_GREEN AMP_LOG_PREFIX "[AMP_DBG][%s:%d]" \
		AMP_LOG_COLOR_NONE fmt, __func__, __LINE__, ##args)
#else
#define openamp_dbg(fmt, args...)
#define openamp_dbgFromISR(fmt, args...)
#endif /* OPENAMP_DBG */

#define openamp_info(fmt, args...) \
	printf(AMP_LOG_COLOR_BLUE AMP_LOG_PREFIX "[AMP_INFO][%s:%d]" \
		AMP_LOG_COLOR_NONE fmt, __func__, __LINE__, ##args)
#define openamp_infoFromISR(fmt, args...) \
	printfFromISR(AMP_LOG_COLOR_BLUE AMP_LOG_PREFIX "[AMP_INFO][%s:%d]" \
		AMP_LOG_COLOR_NONE fmt, __func__, __LINE__, ##args)

#define openamp_err(fmt, args...) \
	printf(AMP_LOG_COLOR_RED AMP_LOG_PREFIX "[AMP_ERR][%s:%d]" \
		AMP_LOG_COLOR_NONE fmt, __func__, __LINE__, ##args)
#define openamp_errFromISR(fmt, args...) \
	printfFromISR(AMP_LOG_COLOR_RED AMP_LOG_PREFIX "[AMP_ERR][%s:%d]" \
		AMP_LOG_COLOR_NONE fmt, __func__, __LINE__, ##args)

#ifdef __cplusplus
}
#endif

#endif /* ifndef __OPENAMP_SUNXI_HELPER_OPENAMP_LOG_H__ */

