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
/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      System PM implementation.
 *
 *  Copyright (c) 2020-2021 Xradio, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#ifndef PAL_SYS_PM_H
#define PAL_SYS_PM_H

#include "pal_os.h"

typedef enum {
	PAL_WAKELOCK_ID_BASE = 0,
	PAL_WAKELOCK_ID_BTC = PAL_WAKELOCK_ID_BASE,
	PAL_WAKELOCK_ID_LTASK,
	PAL_WAKELOCK_ID_HCITX,
	PAL_WAKELOCK_ID_HCIRX,
	PAL_WAKELOCK_ID_BTC_BB,
	PAL_WAKELOCK_ID_MAX,
} pal_wakelock_id_t;

typedef enum {
	PAL_WAKESRC_ID_BASE = 0,
	PAL_WAKESRC_ID_SLPTMR = PAL_WAKESRC_ID_BASE,
	PAL_WAKESRC_ID_MAX,
} pal_wakesrc_id_t;

typedef enum {
	PAL_PM_MODE_BASE = 0,
	PAL_PM_MODE_ON = PAL_PM_MODE_BASE,
	PAL_PM_MODE_SLEEP,
	PAL_PM_MODE_STANDBY,
	PAL_PM_MODE_HIBERNATION,
	PAL_PM_MODE_MAX,
} pal_suspend_mode_t;

typedef struct {
	uint32_t suspend_noirq_interval;
	uint32_t enter_standby_interval;
	uint32_t exit_standby_interval;
	uint32_t resume_noirq_interval;
} pal_pm_interval_t;

typedef void *pal_task_handle_t;

int32_t PalPmDevopsRegister(void);
int32_t PalPmDevopsUnregister(void);
int32_t PalPmWakelockAcquire(pal_wakelock_id_t wl);
int32_t PalPmWakelockRelease(pal_wakelock_id_t wl);
int32_t PalPmWakelockGetRefer(pal_wakelock_id_t wl);
void PalPmWakesrcAcquire(pal_wakesrc_id_t ws);
void PalPmWakesrcReleaseSleepy(pal_wakesrc_id_t ws);
void PalPmWakesrcReleaseWakeup(pal_wakesrc_id_t ws);
int32_t PalPmTaskRegister(pal_task_handle_t handle);
int32_t PalPmTaskUnregister(pal_task_handle_t handle);
int32_t PalPmIsHostReady(void);
int32_t PalPmGetInterval(pal_pm_interval_t *interval);
void PalPmClearInterval(void);

#endif /*PAL_SYS_PM_H*/
