/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _PM_WAKESRC_H_
#define _PM_WAKESRC_H_

#include <hal/aw_list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PM_WAKESRC_NAME_LENTH   (32U)
#define PM_WAKESRC_IRQ_NONE     (-1)
#define PM_WAKESRC_IRQ_SOFT     (-2)
typedef enum {
	PM_RELAX_SLEEPY = 0,
	PM_RELAX_WAKEUP,

	PM_RELAX_MAX,
	PM_RELAX_BASE = PM_RELAX_SLEEPY,
} pm_relax_type_t;

typedef enum {
	PM_WAKESRC_MODE_NORMAL = 0,
	PM_WAKESRC_MODE_INTERNAL,
	PM_WAKESRC_MODE_MAX,
} pm_wakesrc_mode_t;

typedef struct {
	char name[PM_WAKESRC_NAME_LENTH];
	int irq;
	int active;
	pm_wakesrc_mode_t mode;
	struct list_head node;
} pm_wakesrc_t;

int32_t pm_wakesrc_get_irq(void);
void pm_wakesrc_clr_irq(void);
void pm_wakesrc_clr_wakeevt(void);
pm_wakesrc_t *pm_wakesrc_register(const int irq, const char *name);
int pm_wakesrc_unregister(pm_wakesrc_t *ws);
void pm_wakesrc_set_mode(pm_wakesrc_t *ws, pm_wakesrc_mode_t mode);
void pm_wakesrc_stay_awake(pm_wakesrc_t *ws);
void pm_wakesrc_relax(pm_wakesrc_t *ws, pm_relax_type_t type);
void pm_wakesrc_get_inpr_cnt(uint32_t *cnt, uint32_t *inpr, uint32_t *inpr_intr);
int pm_wakesrc_save_cnt(uint32_t cnt);
int pm_wakesrc_cnt_changed(uint32_t cnt);
int pm_wakesrc_wait_inpr(uint32_t *cnt, uint32_t timeout);
void pm_wakesrc_showall(void);
int pm_wakesrc_init(void);
int pm_wakesrc_deinit(void);

#ifdef CONFIG_ARCH_SUN20IW2
struct pm_wakeres {
	const uint32_t  pwrcfg;
	const uint32_t  clkcfg;
	const uint32_t  anacfg;
};

#define PM_WAKESRC_REGISTER_MAGIC	(0xadfc4215)
#define INVAL_IRQ_NUM			(-32)

#define PM_SOFT_WAKEUP_IRQ_BASE		(-33)
#define PM_SOFT_WAKESRC_MAJOR_ID	(PM_SOFT_WAKEUP_IRQ_BASE)

#define PM_WAKESRC_NAME_LENTH		(32U)

typedef enum {
	PM_WAKEUP_SRC_WKIO0 = 0,
	PM_WAKEUP_SRC_WKIO1,
	PM_WAKEUP_SRC_WKIO2,
	PM_WAKEUP_SRC_WKIO3,
	PM_WAKEUP_SRC_WKIO4,
	PM_WAKEUP_SRC_WKIO5,
	PM_WAKEUP_SRC_WKIO6,
	PM_WAKEUP_SRC_WKIO7,
	PM_WAKEUP_SRC_WKIO8,
	PM_WAKEUP_SRC_WKIO9,
	PM_WAKEUP_SRC_WKTMR, /*in pmu spec*/
	PM_WAKEUP_SRC_ALARM0, /* 11*/
	PM_WAKEUP_SRC_ALARM1,
	PM_WAKEUP_SRC_WKTIMER0,
	PM_WAKEUP_SRC_WKTIMER1,
	PM_WAKEUP_SRC_WKTIMER2,
	PM_WAKEUP_SRC_LPUART0,
	PM_WAKEUP_SRC_LPUART1,
	PM_WAKEUP_SRC_GPADC,
	PM_WAKEUP_SRC_MAD,
	PM_WAKEUP_SRC_WLAN,
	PM_WAKEUP_SRC_BT,
	PM_WAKEUP_SRC_BLE,
	PM_WAKEUP_SRC_GPIOA,
	PM_WAKEUP_SRC_GPIOB,
	PM_WAKEUP_SRC_GPIOC,
	PM_WAKEUP_SRC_DEVICE,/*Reserved*/

	PM_WAKEUP_SRC_MAX,
	PM_WAKEUP_SRC_BASE = PM_WAKEUP_SRC_WKIO0,

	//PM_WAEUP_SRC_ENUM_INTVAL = PM_ENUM_EXTERN,
	//PM_WAEUP_SRC_ENUM_INTVAL = (0x1 << ((sizeof(int)-1)*8)),
} wakesrc_id_t;

#define  wakesrc_id_valid(_id) \
	((_id) >= PM_WAKEUP_SRC_BASE && (_id) < PM_WAKEUP_SRC_MAX)
#define PM_WAKEUP_SRC_WKIOx(_n) (PM_WAKEUP_SRC_WKIO0 + (_n))

typedef enum {
	PM_WS_AFFINITY_M33 = 0,
	PM_WS_AFFINITY_DSP,
	PM_WS_AFFINITY_RISCV,

	PM_WS_AFFINITY_MAX,
	PM_WS_AFFINITY_BASE = PM_WS_AFFINITY_M33,
} wakesrc_affinity_t;
#define  wakesrc_affinity_valid(_id) \
	((_id) >= PM_WS_AFFINITY_BASE && (_id) < PM_WS_AFFINITY_MAX)

typedef enum {
	PM_WAKESRC_ALWAYS_WAKEUP = 0,
	PM_WAKESRC_MIGHT_WAKEUP,
	PM_WAKESRC_SOFT_WAKEUP,

	PM_WAKESRC_TYPE_MAX,
	PM_WAKESRC_TYPE_BASE = PM_WAKESRC_ALWAYS_WAKEUP,
} wakesrc_type_t;
#define  wakesrc_type_valid(_type) \
	((_type) >= PM_WAKESRC_TYPE_BASE && (_type) < PM_WAKESRC_TYPE_MAX)

typedef enum {
	PM_WAKESRC_ACTION_WAKEUP_SYSTEM = 0,
	PM_WAKESRC_ACTION_SLEEPY,

	PM_WAKESRC_ACTION_MAX,
	PM_WAKESRC_ACTION_BASE = PM_WAKESRC_ACTION_WAKEUP_SYSTEM,
} wakesrc_action_t;
#define  wakesrc_action_valid(_action) \
	((_action) >= PM_WAKESRC_ACTION_BASE && (_action) < PM_WAKESRC_ACTION_MAX)

#if defined(CONFIG_ARCH_ARM_CORTEX_M33)
#define	PM_WAKESRC_HEREON	(0x1 << PM_WS_AFFINITY_M33)
#elif defined(CONFIG_ARCH_RISCV_C906)
#define	PM_WAKESRC_HEREON	(0x1 << PM_WS_AFFINITY_RISCV)
#elif defined(CONFIG_ARCH_DSP)
#define	PM_WAKESRC_HEREON	(0x1 << PM_WS_AFFINITY_DSP)
#else
#define	PM_WAKESRC_HEREON	(0x0)
#endif

struct pm_wakesrc_settled {
	const wakesrc_id_t id;
	const int          irq;
	const char        *name;
	struct pm_wakeres  res;
};

/* struct wakesrc_cnt - CNT record of wakesrc event
 * @event_count: Number of events have been handled.
 * @active_count: Number of times the wakeup source was activated.
 * @relax_count: Number of times the wakeup source was deactivated.
 * @wakeup_count: Number of times the wakeup source might abort suspend.
 */
struct pm_wakesrc_cnt {
	unsigned long event_cnt;
	unsigned long active_cnt;
	unsigned long relax_cnt;
	unsigned long wakeup_cnt;
};

struct pm_wakesrc {
	int id;
	wakesrc_affinity_t affinity;
	int irq;
	int enabled;
	char name[PM_WAKESRC_NAME_LENTH];
	struct pm_wakesrc_cnt cnt;
	unsigned int lock;
	wakesrc_type_t type;
	struct list_head  node;
};

int pm_set_wakeirq(const int irq);
int pm_clear_wakeirq(const int irq);
#endif /* CONFIG_ARCH_SUN20IW2 */

#ifdef __cplusplus
}
#endif

#endif

