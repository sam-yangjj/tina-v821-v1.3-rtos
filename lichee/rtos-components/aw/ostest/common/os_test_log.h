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
#ifndef __OS_TEST_LOG_H__
#define __OS_TEST_LOG_H__

//#define OS_TEST_DEBUG

/* os test module log macro */
#define OS_TEST_LOG_COLOR_NONE "\e[0m"
#define OS_TEST_LOG_COLOR_RED "\e[31m"
#define OS_TEST_LOG_COLOR_GREEN "\e[32m"
#define OS_TEST_LOG_COLOR_YELLOW "\e[33m"
#define OS_TEST_LOG_COLOR_BLUE "\e[34m"

#define os_test_printf printf

#ifdef OS_TEST_DEBUG
#define os_test_dbg_without_newline(fmt,...) \
			os_test_printf(OS_TEST_LOG_COLOR_BLUE "[OT_D][%s:%d] " fmt \
				OS_TEST_LOG_COLOR_NONE, __func__, __LINE__, ##__VA_ARGS__)

#define os_test_dbg(fmt,...) os_test_dbg_without_newline(fmt"\n", ##__VA_ARGS__)
#else
#define os_test_dbg_without_newline(fmt,...)
#define os_test_dbg(fmt, args...)
#endif /* OS_TEST_DEBUG */

#define os_test_info_without_newline(fmt,...) \
			os_test_printf(OS_TEST_LOG_COLOR_GREEN "[OT_I][%s:%d] " fmt \
				OS_TEST_LOG_COLOR_NONE, __func__, __LINE__, ##__VA_ARGS__)

#define os_test_info(fmt,...) os_test_info_without_newline(fmt"\n", ##__VA_ARGS__)

#define os_test_warn_without_newline(fmt,...) \
			os_test_printf(OS_TEST_LOG_COLOR_YELLOW "[OT_W][%s:%d] " fmt \
				OS_TEST_LOG_COLOR_NONE, __func__, __LINE__, ##__VA_ARGS__)

#define os_test_warn(fmt,...) os_test_warn_without_newline(fmt"\n", ##__VA_ARGS__)

#define os_test_err_without_newline(fmt,...) \
			os_test_printf(OS_TEST_LOG_COLOR_RED "[OT_E][%s:%d] " fmt \
				OS_TEST_LOG_COLOR_NONE, __func__, __LINE__, ##__VA_ARGS__)

#define os_test_err(fmt,...) os_test_err_without_newline(fmt"\n", ##__VA_ARGS__)

#endif /* __OS_TEST_LOG_H__ */
