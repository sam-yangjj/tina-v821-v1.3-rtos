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
#include "pal_sys_pm.h"

int32_t PalPmDevopsRegister(struct pm_device *dev)
{
	return pm_devops_register(dev);
}

int32_t PalPmDevopsUnregister(struct pm_device *dev)
{
	return pm_devops_unregister(dev);
}

int32_t PalPmTaskRegister(TaskHandle_t xHandle, pm_task_type_t type)
{
	return pm_task_register(xHandle, type);
}

int32_t PalPmTaskUnregister(TaskHandle_t xHandle)
{
	return pm_task_unregister(xHandle);
}

void PalPmWakecntInc(int32_t irq)
{
	pm_wakecnt_inc(irq);
}

void PalPmStayAwake(int32_t irq)
{
	pm_stay_awake(irq);
}

void PalPmRelax(int32_t irq, pm_relax_type_t wakeup)
{
	pm_relax(irq, wakeup);
}

void PalPmWakelocksSetname(struct wakelock *wl, const char *name)
{
	pm_wakelocks_setname(wl, name);
}

int32_t PalPmWakelockAcquire(struct wakelock *wl, enum pm_wakelock_t type, uint32_t timeout)
{
	return pm_wakelocks_acquire(wl, type, timeout);
}

int32_t PalPmWakelockRelease(struct wakelock *wl)
{
	return pm_wakelocks_release(wl);
}

uint32_t PalPmSubsysCheckInStatus(pm_subsys_status_t status)
{
	return pm_subsys_check_in_status(status);
}

int32_t PalPmWakesrcRegister(const int32_t irq, const char *name, const unsigned int type)
{
#ifdef CONFIG_ARCH_SUN20IW2
	return 0;
#else
	return pm_wakesrc_register(irq, name, type);
#endif
}

int32_t PalPmWakesrcUnregister(int32_t irq)
{
#ifdef CONFIG_ARCH_SUN20IW2
	return 0;
#else
	return pm_wakesrc_unregister(irq);
#endif
}

int32_t PalPmSetWakeirq(const int32_t irq)
{
	return pm_set_wakeirq(irq);
}
int32_t PalPmClearWakeirq(const int32_t irq)
{
	return pm_clear_wakeirq(irq);
}
