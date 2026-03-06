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

#ifndef _PM_DEVOPS_H_
#define _PM_DEVOPS_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#else
#error "PM do not support the RTOS!!"
#endif

#include <hal/aw_list.h>
#include "pm_base.h"

typedef enum {
	PM_DEVOPS_TYPE_PREPARED = 0,
	PM_DEVOPS_TYPE_SUSPEND,
	PM_DEVOPS_TYPE_SUSPEND_LATE,
	PM_DEVOPS_TYPE_SUSPEND_NOIRQ,
	PM_DEVOPS_TYPE_RESUME_NOIRQ,
	PM_DEVOPS_TYPE_RESUME_EARLY,
	PM_DEVOPS_TYPE_RESUME,
	PM_DEVOPS_TYPE_COMPLETE,

	PM_DEVOPS_TYPE_MAX,
	PM_DEVOPS_TYPE_BASE = PM_DEVOPS_TYPE_PREPARED,
} pm_devops_type_t;

#define pm_devops_type_valid(_t) \
	((_t) >= PM_DEVOPS_TYPE_BASE && (_t) < PM_DEVOPS_TYPE_MAX)
#define pm_devops_belong_suspend(_t) \
	((_t) >= PM_DEVOPS_TYPE_BASE && (_t) <= PM_DEVOPS_TYPE_SUSPEND_NOIRQ)
#define pm_devops_belong_resume(_t) \
	((_t) >= PM_DEVOPS_TYPE_RESUME_NOIRQ && (_t) < PM_DEVOPS_TYPE_MAX)



struct pm_device;
struct pm_devops {
	int (*prepared) (struct pm_device *dev, suspend_mode_t mode);
	int (*suspend) (struct pm_device *dev, suspend_mode_t mode);
	int (*suspend_late) (struct pm_device *dev, suspend_mode_t mode);
	int (*suspend_noirq) (struct pm_device *dev, suspend_mode_t mode);
	int (*resume_noirq) (struct pm_device *dev, suspend_mode_t mode);
	int (*resume_early) (struct pm_device *dev, suspend_mode_t mode);
	int (*resume) (struct pm_device *dev, suspend_mode_t mode);
	int (*complete) (struct pm_device *dev, suspend_mode_t mode);
};

struct pm_device {
	const char       *name;
	struct list_head  node;
	struct pm_devops  *ops;
	void             *data;
};

int pm_devops_init(void);
int pm_devops_exit(void);
int pm_devops_register(struct pm_device *dev);
int pm_devops_unregister(struct pm_device *dev);

int pm_devops_prepared(suspend_mode_t mode);
int pm_devops_suspend(suspend_mode_t mode);
int pm_devops_suspend_late(suspend_mode_t mode);
int pm_devops_suspend_noirq(suspend_mode_t mode);

void pm_devops_resume_noirq(suspend_mode_t mode);
void pm_devops_resume_early(suspend_mode_t mode);
void pm_devops_resume(suspend_mode_t mode);
void pm_devops_complete(suspend_mode_t mode);

int pm_devops_start_report(void);
int pm_devops_stop_report(void);

const char *pm_devops_type2string(int type);

#endif

