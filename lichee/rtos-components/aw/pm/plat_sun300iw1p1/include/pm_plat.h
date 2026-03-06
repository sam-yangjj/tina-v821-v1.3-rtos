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
#ifndef __PM_PLAT__
#define __PM_PLAT__

#include <pm_base.h>
#include <pm_platops.h>

#define PM_PLAT_INF_EN      (0)
#define PM_PLAT_LOG_EN      (1)
#define PM_PLAT_ERR_EN      (1)
#if PM_PLAT_INF_EN
#define plat_inf(fmt,...)   printf("[PLAT_INF]" fmt,##__VA_ARGS__)
#else
#define plat_inf(fmt,...)
#endif
#if PM_PLAT_LOG_EN
#define plat_log(fmt,...)   printf("[PLAT_LOG]" fmt,##__VA_ARGS__)
#else
#define plat_log(fmt,...)
#endif
#if PM_PLAT_ERR_EN
#define plat_err(fmt,...)   printf("[PLAT_ERR]" fmt,##__VA_ARGS__)
#else
#define plat_err(fmt,...)
#endif

#define HARDWARE_WAKESRC_REGISTER   (0x4A000214)
#define WLAN_IRQ_STATUS0_REG        (0x7AA80090)
#define WLAN_IRQ_STATUS1_REG        (0x7AA80094)
#define WLAN_IRQ_PENDING_MASK       (0x00000002)

typedef enum {
	STANDBY_ENTER_ERR_NONE          = 0,
	STANDBY_ENTER_ERR_SRAM_OVERFLOW = -1,
} standby_enter_err_t;

typedef enum {
	PWR_CFG_MASK_LDO_1V8 = (1 << 0),
	PWR_CFG_MASK_LDO_2V8 = (1 << 1),
}pwr_cfg_mask_t;

int pm_plat_platops_init(void);
int pm_plat_platops_deinit(void);

void pm_systeminit(void);
int pm_client_init(void);

int pm_trigger_suspend(suspend_mode_t mode);

int pm_client_init(void);

void plat_suspend(void);
void plat_resume(void);

int set_finished_dummy(void);
int clear_finished_dummy(void);
int suspend_is_finished(void);
#endif
