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
#include "pm_base.h"
#include "pm_devops.h"
#include "pm_task.h"
#include "pm_wakecnt.h"
#include "pm_wakelock.h"
#include "pm_wakesrc.h"
#include "pm_subsys.h"
#include "ccu-sun20iw2-aon.h"
#include "irqs-sun20iw2p1.h"
#include "pal_dbg_io.h"

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/
/* pm */
int32_t PalPmDevopsRegister(struct pm_device *dev);
int32_t PalPmDevopsUnregister(struct pm_device *dev);
int32_t PalPmTaskRegister(TaskHandle_t xHandle, pm_task_type_t type);
int32_t PalPmTaskUnregister(TaskHandle_t xHandle);
void PalPmWakecntInc(int32_t irq);
void PalPmStayAwake(int32_t irq);
void PalPmRelax(int32_t irq, pm_relax_type_t wakeup);
void PalPmWakelocksSetname(struct wakelock *wl, const char *name);
int32_t PalPmWakelockAcquire(struct wakelock *wl, enum pm_wakelock_t type, uint32_t timeout);
int32_t PalPmWakelockRelease(struct wakelock *wl);
uint32_t PalPmSubsysCheckInStatus(pm_subsys_status_t status);

/* wakesrc */
int32_t PalPmWakesrcRegister(const int32_t irq, const char *name, const unsigned int type);
int32_t PalPmWakesrcUnregister(int32_t irq);
int32_t PalPmSetWakeirq(const int32_t irq);
int32_t PalPmClearWakeirq(const int32_t irq);

#endif /*PAL_SYS_PM_H*/
