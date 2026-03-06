/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SUNXI_HAL_LOG_H
#define SUNXI_HAL_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#ifdef CONFIG_KERNEL_FREERTOS

#include <awlog.h>

#define hal_log_err(fmt, ...)           pr_err(fmt"\n", ##__VA_ARGS__)
#define hal_log_warn(fmt, ...)          pr_warn(fmt"\n", ##__VA_ARGS__)
#define hal_log_info(fmt, ...)          pr_info(fmt"\n", ##__VA_ARGS__)
#define hal_log_debug(fmt, ...)         pr_debug(fmt"\n", ##__VA_ARGS__)

#elif defined CONFIG_RTTKERNEL

#include <log.h>

#define hal_log_err(fmt, ...)           pr_err(fmt"\n", ##__VA_ARGS__)
#define hal_log_warn(fmt, ...)          pr_warn(fmt"\n", ##__VA_ARGS__)
#define hal_log_info(fmt, ...)          pr_info(fmt"\n", ##__VA_ARGS__)
#define hal_log_debug(fmt, ...)         pr_debug(fmt"\n", ##__VA_ARGS__)

#else
int printk(const char *fmt, ...);
#define HAL_XPOSTO(x)   "\033[" #x "D\033[" #x "C"

#define HAL_LOG_LAYOUT      "%s%s%s: [%s:%04u]: %s%s"
#define HAL_LOG_BACKEND_CALL(log_lv, log_color, log_format, color_off, ...) \
    printk(HAL_LOG_LAYOUT log_format "%s""\n\r",                            \
           log_color, log_lv, color_off, __func__, __LINE__, HAL_XPOSTO(30),\
           log_color, ##__VA_ARGS__, color_off)

#define HAL_LOG_COLOR(log_lv, log_color, log_format, ...)                   \
    HAL_LOG_BACKEND_CALL(log_lv, log_color, log_format,                     \
                     HAL_LOG_COLOR_OFF, ##__VA_ARGS__)


#define HAL_LOG_COLOR_OFF                 "\033[0m"
#define HAL_LOG_COLOR_RED                 "\033[1;40;31m"
#define HAL_LOG_COLOR_YELLOW              "\033[1;40;33m"
#define HAL_LOG_COLOR_BLUE                "\033[1;40;34m"
#define HAL_LOG_COLOR_PURPLE              "\033[1;40;35m"

#define HAL_LOG_ERROR_PREFIX            "[ERR]"
#define HAL_LOG_WARNING_PREFIX          "[WRN]"
#define HAL_LOG_INFO_PREFIX             "[INF]"
#define HAL_LOG_DEBUG_PREFIX            "[DBG]"

#define hal_log_err(...) \
	do { HAL_LOG_COLOR(HAL_LOG_ERROR_PREFIX, HAL_LOG_COLOR_OFF, ##__VA_ARGS__); } while(0)
#define hal_log_warn(...) \
	do { HAL_LOG_COLOR(HAL_LOG_WARNING_PREFIX, HAL_LOG_COLOR_OFF, ##__VA_ARGS__); } while(0)
#define hal_log_info(...) \
	do { HAL_LOG_COLOR(HAL_LOG_INFO_PREFIX, HAL_LOG_COLOR_OFF, ##__VA_ARGS__); } while(0)
#define hal_log_debug(...) \
	do { HAL_LOG_COLOR(HAL_LOG_DEBUG_PREFIX, HAL_LOG_COLOR_OFF, ##__VA_ARGS__); } while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif
