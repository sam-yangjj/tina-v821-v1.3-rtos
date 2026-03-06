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

#ifndef _PM_BASE_H_
#define _PM_BASE_H_

#include <stdint.h>

/**
 * @brief Defined all supported low power state.
 * @note:
 *       PM_MODE_ON is used for test.
 *       PM_MODE_SLEEP is used for devices wakeup system. In this mode CPU is
 *         in WFI mode and running at low frequency, all devices are powered on.
 *         set to work mode if you want this device to wakeup system, or else
 *         disable this device to save power.
 *       PM_MODE_STANDBY is used for network or some special wakeup sources to
 *         wakeup system. In this mode CPU and all devices has been powered off,
 *         network can work normally and can wakeup system by received data from
 *         network. Also some special wakeup sources like wakeup timer or IO can
 *         wakeup system if you set this wakeup sources properly.
 *       PM_MODE_HIBERNATION is used for some special wakeup sources to wakeup system.
 *         System will restartup when wakeup. In this mode CPU and all devices
 *         has been powered off beside network. Only some special wakeup sources
 *         can startup system, and can get wakeup event at startup.
 */
typedef enum {
	PM_MODE_ON = 0U,
	PM_MODE_SLEEP,
	PM_MODE_STANDBY,
	PM_MODE_HIBERNATION,
	PM_MODE_MAX,
	PM_MODE_BASE = PM_MODE_ON,
} suspend_mode_t;

typedef enum {
	PM_STANDBY_MODE_SUPER = 0,
	PM_STANDBY_MODE_ULTRA,
	PM_STANDBY_MODE_MAX,
} pm_standby_mode_t;

#define PM_SUSPEND_OK      (0)
#define PM_SUSPEND_AGAIN   (1)
#define PM_SUSPEND_FAIL    (-1)

#define pm_suspend_mode_valid(_t) \
	((_t) >= PM_MODE_BASE && (_t) < PM_MODE_MAX)

/* remain temporarily */
struct pm_suspend_stat {
	const char *name;
	void *failed_unit;
	uint32_t cnt;
};

const char *pm_mode2string(int mode);
int pm_set_wakeup_timer_ms(unsigned int ms);
#endif

