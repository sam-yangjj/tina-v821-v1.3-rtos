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
#include <string.h>
#include <errno.h>
#include <sunxi-chips.h>
#include <hal_def.h>
#include <hal_osal.h>
#include <hal_interrupt.h>
#include <hal_cpux_cfg.h>
#include <hal_sysctrl.h>
#include <hal_pwrctrl.h>
#include <hal_ccu.h>
#include <hal_ts.h>
#include <hal_rtcwdg.h>
#include <hal_wakeup.h>
#include <hal_sram.h>
#include <hal_msgbox.h>
#include <hal_alarm.h>
#include <hal_queue.h>
#include <hal_cfg.h>
#include <hal_prcm.h>
#include <heap_backup.h>
#include <heap_retain.h>
#include <head.h>
#include <pm_base.h>
#include <pm_debug.h>
#include <pm_adapt.h>
#include <pm_plat.h>
#include <pm_task.h>
#include <pm_platops.h>
#include <pm_wakelock.h>
#include <pm_wakesrc.h>
#include <pm_suspend.h>
#include <pm_syscore.h>
#include <load_image.h>
#include <hal_clk.h>
#include <flash_read.h>
#ifdef CONFIG_PM_STANDBY_MEMORY
#include <hal_mem.h>
#endif
#ifdef CONFIG_COMPONENTS_VIRT_LOG
#include <virt_log.h>
#endif
#ifdef CONFIG_PM_PLAT_DEBUG
#include <pm_plat_debug.h>
#endif
#include "sys/wlan_pm.h"

#define PMBOOT_ITEM_NAME                       ("pmboot")
#define PM_CLIENT_PRIORITY                     (PM_TASK_PRIORITY + 1)
#define PM_UBOOT_SHUTDOWN_PRIORITY             (PM_TASK_PRIORITY + 1)

#define WAKEUP_IO_MAX                          (8)
#define WAKEUP_ALARM_MAX                       (2)

#define MSGBOX_PM_REMOTE                       (1)
#define MSGBOX_PM_RECV_CHANNEL                 (3)
#define MSGBOX_PM_SEND_CHANNEL                 (3)

#define MSGBOX_PM_NOTIFY_RECV_CHANNEL          (2)
#define MSGBOX_PM_NOTIFY_SEND_CHANNEL          (2)
#define REMOTE_NOTIFY_PM_SUSPEND_PREPARE       (0xdbdb0011)
#define REMOTE_NOTIFY_PM_POST_SUSPEND          (0xdbdb0021)

#define CLIENT_CHANNEL_OPENSBI                 (0)
#define CLIENT_CHANNEL_KERNEL                  (1)

#define STEP_REC_HEAD                          (0)
#define STEP_REC_LEN                           (1)
#define STEP_REC_PAYLOAD                       (2)
#define RECV_PARAM_SIZE                        (5)

#define ARISC_MESSAGE_INITIALIZED              (0x02)
#define ARISC_MESSAGE_ATTR_SOFTSYN             (1 << 0)
#define ARISC_MESSAGE_ATTR_HARDSYN             (1 << 1)
#define HEAD_CLR_RESULT(head)                  ((head) &= ~(0xff << 24))
#define ARISC_CPU_OP_REQ                       (0x22)
#define ARISC_SYS_OP_REQ                       (0x24)
#define ARISC_STANDBY_RESTORE_NOTIFY           (0x11)
#define ARISC_REQUIRE_WAKEUP_SRC_REQ           (0x23)
#define ARISC_CLEAR_WAKEUP_SRC_REQ             (0x25)
#define ARISC_SET_WAKEUP_SRC_REQ               (0x26)
#define ARISC_WAKEUP_IO_NUM                    (0x28)
#define ARISC_PWR_CFG                          (0x29)
#define ARISC_CPU_OP_SHUTDOWN_TYPE             (0x30)
#define ARISC_SET_WDG_PERIOD                   (0x17)

#define KERNEL_POST_SUSPEND_MSG                (0x80)
#define KERNEL_SUSPEND_PREARE_MSG              (0x81)
#define KERNEL_FLASH_INFO_MSG                  (0x82)
#define KERNEL_ENABLE_AOV_TIMER_MSG            (0x83)

#define MS_TO_32K(ms)                          (ms * 32)

#define HEAD_GET_STATE(head)                   ((uint8_t)((head >> 0) & 0xff))
#define HEAD_GET_ATTR(head)                    ((uint8_t)((head >> 8) & 0xff))
#define HEAD_GET_TYPE(head)                    ((uint8_t)((head >> 16) & 0xff))
#define GET_LEN(data)                          ((uint16_t)(data & 0xffff))
#define GET_ROOT_IRQ(data)                     ((uint16_t)(data & 0x3ff))
#define GET_WAKESRC_TYPE(data)                 ((uint8_t)((data >> 30) & 0x3))
#define GET_WAKESRC_PARAM(data)                ((uint32_t)((data) & 0x3FFFFFFF))
#define GET_DEBUG_TIME(data)                   ((uint32_t)(data & 0x3fffffff))
#define DRAM_PARAM_ENTRY(name)                 {.string = #name, .offset = offsetof(__dram_para_t, name)}

#define CPUX_WUPIO_IRQ                          (171U)
#define CPUX_WUPTIMER_IRQ                       (166U)
#define CPUX_ALARM0_IRQ                         (167U)
#define CPUX_ALARM1_IRQ                         (168U)
#define CPUX_WLAN_IRQ                           (160U)
#define CPUS_WUPIO_IRQ                         (MAKE_IRQn(186U, 0))
#define CPUS_WUPTIMER_IRQ                      (MAKE_IRQn(181U, 0))
#define CPUS_ALARM0_IRQ                        (MAKE_IRQn(182U, 0))
#define CPUS_ALARM1_IRQ                        (MAKE_IRQn(183U, 0))
#define CPUS_WLAN_IRQ                          (MAKE_IRQn(175U, 0))

typedef enum {
	PM_WAKEUP_MASK_IO_0         = (1 << 0),
	PM_WAKEUP_MASK_IO_1         = (1 << 1),
	PM_WAKEUP_MASK_IO_2         = (1 << 2),
	PM_WAKEUP_MASK_IO_3         = (1 << 3),
	PM_WAKEUP_MASK_IO_4         = (1 << 4),
	PM_WAKEUP_MASK_IO_5         = (1 << 5),
	PM_WAKEUP_MASK_IO_6         = (1 << 6),
	PM_WAKEUP_MASK_IO_7         = (1 << 7),
	PM_WAKEUP_MASK_RTC_ALARM0   = (1 << 8),
	PM_WAKEUP_MASK_RTC_ALARM1   = (1 << 9),
	PM_WAKEUP_MASK_WAKEUP_TIMER = (1 << 10),
	PM_WAKEUP_MASK_WLAN         = (1 << 11),
	PM_WAKEUP_MASK_IO           = (PM_WAKEUP_MASK_IO_0 | PM_WAKEUP_MASK_IO_1 | PM_WAKEUP_MASK_IO_2 | \
                                   PM_WAKEUP_MASK_IO_3 | PM_WAKEUP_MASK_IO_4 | PM_WAKEUP_MASK_IO_5 | \
                                   PM_WAKEUP_MASK_IO_6 | PM_WAKEUP_MASK_IO_7),
} pm_wakeup_mask_t;

typedef struct {
	uint8_t type;
	uint16_t len;
	int (*func)(uint32_t head, uint32_t *param, uint16_t len);
} client_handle_t;

struct client_channel_info {
	uint32_t channel;
	uint32_t data;
};

struct client_machine_status {
	const client_handle_t *handle;
	uint8_t handle_size;
	uint8_t step;
	uint32_t head;
	uint32_t cnt;
	uint32_t len;
	uint32_t param[RECV_PARAM_SIZE];
};

typedef struct {
	int32_t cpus_irq;
	int32_t cpux_irq;
	pm_wakeup_mask_t mask;
} irq_map_t;

struct sunxi_pm {
	struct msg_endpoint ept;
	struct msg_endpoint remote_notify_ept;
	hal_queue_t recv_queue;
	hal_thread_t thread;
};

typedef struct {
	char *string;
	uint8_t offset;
} dram_param_t;

typedef struct {
	uint32_t reg_addr;   //register address
	uint32_t reg_def;    //register default value
	uint32_t reg_mask;   //cpus used register mask
} ccu_spc_reg_t;

extern uint32_t __StackLimit;
extern uint32_t __StackTop;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __heap_start__;
extern uint32_t __heap_end__;
extern uint32_t __stby_unsaved_bss_start__;
extern uint32_t __stby_unsaved_bss_end__;
extern uint32_t __stby_unsaved_data_start__;
extern uint32_t __stby_unsaved_data_end__;
extern uint32_t __stby_saved_data_start__;
extern uint32_t __stby_saved_data_end__;
extern uint32_t __stby_saved_bss_start__;
extern uint32_t __stby_saved_bss_end__;
extern unsigned char _standby_bin_start[];
extern uint32_t __flash_driver_start__;
extern uint32_t __flash_driver_end__;

static int pm_cpus_begin(suspend_mode_t mode);
static int pm_cpus_enter(suspend_mode_t mode);
#ifndef CONFIG_COMPONENTS_PM_CORE_DSP
extern void arch_disable_all_irq(void);
extern void arch_enable_all_irq(void);
#endif
extern int deinit_openamp_sync(void);
extern void multiple_console_early_deinit(void);
extern int multiple_console_early_init(void);
extern int init_openamp(void);
#ifdef CONFIG_DRIVERS_BLUETOOTH_XRADIO
extern void bt_xradio_link_deinit(int is_ultra_standby);
#endif
#ifdef CONFIG_DRIVERS_V821_USE_SIP_WIFI
extern void wlan_smac_rpmsg_deinit(int is_ultra_standby);
#endif
#ifdef CONFIG_INIT_NET_STACK
extern int wlan_fmac_xrlink_deinit(int is_ultra_standby);
#endif
#ifdef CONFIG_VIN_USE_PM
extern int csi_rpmsg_link_deinit(void);
extern int csi_rpmsg_link_init(void);
extern int load_isp_param_from_flash(void);
#endif
__attribute__((weak)) void platform_fastboot_init_early(void) { }

static standby_head_t *dram_standby_head = (standby_head_t *)_standby_bin_start;
static bin_info pmboot_info = {0};
static rproc_info rtos_rproc_info = {0};
static uint32_t dram_size = 0;
static struct sunxi_pm *pm_inst;
static pm_wakesrc_t *wuptimer_ws = NULL;
static pm_wakesrc_t *wupio_ws = NULL;
static pm_wakesrc_t *alarm0_ws = NULL;
static pm_wakesrc_t *alarm1_ws = NULL;
static pm_wakeup_mask_t super_standby_io_mask;
static pm_wakeup_mask_t super_standby_wakeup_mask;
static pm_wakeup_mask_t ultra_standby_wakeup_mask;
static pm_wakeup_mask_t hibernation_wakeup_mask;
static pm_wakeup_mask_t real_wakeup_mask;
static uint32_t cpux_shutdown_type = 0;
static uint32_t cpux_start_address = 0;
static uint32_t sp_reg_backup = 0;
static uint32_t cpux_clk_reg_backup = 0;
static uint32_t cpux_mt_clk_reg_backup = 0;
static uint32_t ccu_app_clk_reg_backup = 0;
static uint32_t rtc_ctrl_backup = 0;
static uint32_t *ccu_app_reg = NULL;
static char wuptmr_aov_en = 0;
static hal_thread_t pm_uboot_shutdown_thread;
static uint32_t rtcwdg_suggest_period = 0;
static osal_timer_t rtcwdg_timer = NULL;

const uint32_t ccu_app_invalid_regaddr[] = {
	CCU_APP + 0x0000,
	CCU_APP + 0x0008,
	CCU_APP + 0x0078,
	CCU_APP + 0x008c,
};
#define CCU_APP_REG_LAST_OFFSET_FOR_V821       (0x009C)
#define CCU_APP_REG_LAST_OFFSET_FOR_V821B      (0x00A4)
#define CCU_APP_REG_NUM_FOR_V821               (((CCU_APP_REG_LAST_OFFSET_FOR_V821 + 4) >> 2) - (ARRAY_SIZE(ccu_app_invalid_regaddr)))
#define CCU_APP_REG_NUM_FOR_V821B              (((CCU_APP_REG_LAST_OFFSET_FOR_V821B + 4) >> 2) - (ARRAY_SIZE(ccu_app_invalid_regaddr)))

const ccu_spc_reg_t ccu_spc_reg[] = {
	{0x4200107c, 0x00000118, 0x00000100},   //reg 0x4200107c
	{0x42001080, 0x5f080088, 0x40000089},   //reg 0x42001080
	{0x42001084, 0x4003e00c, 0x00000000},   //reg 0x42001084
	{0x42001088, 0x00000412, 0x00000400},   //reg 0x42001088
	{0x42001090, 0x08080080, 0x0000008b},   //reg 0x42001090
	{0x42001094, 0x00001008, 0x00001000},   //reg 0x42001094
	{0x42001098, 0x00000001, 0x00000000},   //reg 0x42001098
};

const dram_param_t dram_param[] = {
	DRAM_PARAM_ENTRY(dram_clk),
	DRAM_PARAM_ENTRY(dram_type),
	DRAM_PARAM_ENTRY(dram_zq),
	DRAM_PARAM_ENTRY(dram_odt_en),
	DRAM_PARAM_ENTRY(dram_para1),
	DRAM_PARAM_ENTRY(dram_para2),
	DRAM_PARAM_ENTRY(dram_mr0),
	DRAM_PARAM_ENTRY(dram_mr1),
	DRAM_PARAM_ENTRY(dram_mr2),
	DRAM_PARAM_ENTRY(dram_mr3),
	DRAM_PARAM_ENTRY(dram_tpr0),
	DRAM_PARAM_ENTRY(dram_tpr1),
	DRAM_PARAM_ENTRY(dram_tpr2),
	DRAM_PARAM_ENTRY(dram_tpr3),
	DRAM_PARAM_ENTRY(dram_tpr4),
	DRAM_PARAM_ENTRY(dram_tpr5),
	DRAM_PARAM_ENTRY(dram_tpr6),
	DRAM_PARAM_ENTRY(dram_tpr7),
	DRAM_PARAM_ENTRY(dram_tpr8),
	DRAM_PARAM_ENTRY(dram_tpr9),
	DRAM_PARAM_ENTRY(dram_tpr10),
	DRAM_PARAM_ENTRY(dram_tpr11),
	DRAM_PARAM_ENTRY(dram_tpr12),
	DRAM_PARAM_ENTRY(dram_tpr13),
};

const irq_map_t cpu_irq_map[] = {
	{CPUS_WUPIO_IRQ   , CPUX_WUPIO_IRQ   , PM_WAKEUP_MASK_IO},
	{CPUS_ALARM0_IRQ  , CPUX_ALARM0_IRQ  , PM_WAKEUP_MASK_RTC_ALARM0},
	{CPUS_ALARM1_IRQ  , CPUX_ALARM1_IRQ  , PM_WAKEUP_MASK_RTC_ALARM1},
	{CPUS_WUPTIMER_IRQ, CPUX_WUPTIMER_IRQ, PM_WAKEUP_MASK_WAKEUP_TIMER},
	{CPUS_WLAN_IRQ    , CPUX_WLAN_IRQ    , PM_WAKEUP_MASK_WLAN},
};

static void pm_rtcwdg_timer_cb(void *p)
{
	HAL_RTCWDG_Feed();
}

static void pm_rtcwdg_takeover(suspend_mode_t mode)
{
	if (!rtcwdg_suggest_period) {
		pm_log("pm rtcwdg skip takeover\n");
		return ;
	}
	if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_SUPER) {
		HAL_RTCWDG_Backup();
	}
	HAL_RTCWDG_Init();
	HAL_RTCWDG_Disable();
	HAL_RTCWDG_SetPeriod(MS_TO_RTCWDG_TICK(rtcwdg_suggest_period));
	HAL_RTCWDG_Enable();
	HAL_RTCWDG_Feed();
	rtcwdg_timer = osal_timer_create("", pm_rtcwdg_timer_cb, NULL, rtcwdg_suggest_period >> 1, OSAL_TIMER_FLAG_PERIODIC);
	osal_timer_start(rtcwdg_timer);
	pm_log("pm rtcwdg takeover: %dms\n", rtcwdg_suggest_period);
}

static void pm_rtcwdg_giveback(suspend_mode_t mode)
{
	if (!rtcwdg_suggest_period) {
		return ;
	}
	if (rtcwdg_timer) {
		osal_timer_stop(rtcwdg_timer);
		osal_timer_delete(rtcwdg_timer);
	}
	HAL_RTCWDG_Feed();
	HAL_RTCWDG_Disable();
	HAL_RTCWDG_Deinit();
	if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_SUPER) {
		HAL_RTCWDG_Restore();
	}
	pm_log("pm rtcwdg giveback\n");
}

static void wuptmr_aov_set(uint32_t wuptmr_aov_period)
{

	uint32_t interval_ms;
	uint32_t sleep_ms;
	register unsigned long __v;

	__asm__ volatile ("csrr %0, 0xc81\n"
			: "=r" (__v) : : "memory");

	/* check RISCV TIMEH register */
	if (__v > 0) {
		sleep_ms = wuptmr_aov_period;
		HAL_WUPTIMER_Release();
		HAL_WUPTIMER_ClrIrqPending();
		HAL_WUPTIMER_Enable(0);
		HAL_WUPTIMER_SetInterval(MS_TO_32K(sleep_ms));
		return;
	}

	/* Warnning: hardware boottime & suspend actions on sram are ignored, which may cause +10ms deviation */
	interval_ms = HAL_TS_GetUs() / 1000;
	if (interval_ms >= wuptmr_aov_period) {
		sleep_ms = wuptmr_aov_period + 15;
	} else {
		/* FIXME: wuptmr wake up 20ms earlier than expected(-20ms) + boottime deviation(+10ms). Temporarily +15ms to calibrate time deviations */
		sleep_ms = wuptmr_aov_period - interval_ms + 15;
	}

	HAL_WUPTIMER_Release();
	HAL_WUPTIMER_ClrIrqPending();
	HAL_WUPTIMER_Enable(0);
	HAL_WUPTIMER_SetInterval(MS_TO_32K(sleep_ms));

}

static hal_irqreturn_t pm_wupio_irq_callback(void *dev_id)
{
	uint32_t i;

	for (i = 0; i < WAKEUP_IO_MAX; i++) {
		if (HAL_WUPIO_IsIrqPending(i)) {
			plat_inf("wuoio irq: %d\n", i);
			if (real_wakeup_mask & (1 << i)) {
				pm_wakesrc_relax(wupio_ws, PM_RELAX_WAKEUP);
			} else {
				plat_err("wupio irq err\n");
			}
			break;
		}
	}
	hal_disable_irq(CPUS_WUPIO_IRQ);
	hal_free_irq(CPUS_WUPIO_IRQ);
	return HAL_IRQ_OK;
}

static hal_irqreturn_t pm_wuptimer_irq_callback(void *dev_id)
{
	if (HAL_WUPTIMER_IsIrqPending()) {
		plat_inf("wuptimer irq\n");
		/* only wuptimer pendig should be clear by rtos */
		pm_wakesrc_relax(wuptimer_ws, PM_RELAX_WAKEUP);
		HAL_WUPTIMER_ClrIrqPending();
		hal_disable_irq(CPUS_WUPTIMER_IRQ);
		hal_free_irq(CPUS_WUPTIMER_IRQ);
	}
	return HAL_IRQ_OK;
}

static hal_irqreturn_t pm_alarm0_irq_callback(void *dev_id)
{
	if (HAL_ALARM_IsIrqPending(0)) {
		plat_inf("alarm0 irq\n");
		pm_wakesrc_relax(alarm0_ws, PM_RELAX_WAKEUP);
		hal_disable_irq(CPUS_ALARM0_IRQ);
		hal_free_irq(CPUS_ALARM0_IRQ);
	}
	return HAL_IRQ_OK;
}

static hal_irqreturn_t pm_alarm1_irq_callback(void *dev_id)
{
	if (HAL_ALARM_IsIrqPending(1)) {
		plat_inf("alarm1 irq\n");
		pm_wakesrc_relax(alarm1_ws, PM_RELAX_WAKEUP);
		hal_disable_irq(CPUS_ALARM1_IRQ);
		hal_free_irq(CPUS_ALARM1_IRQ);
	}
	return HAL_IRQ_OK;
}

static void dump_param(uint32_t *param, uint16_t len)
{
	uint16_t i;

	plat_inf("param len: %u\n", len);
	for (i = 0; i < len; i++) {
		plat_inf("param[%u]: 0x%08x\n", i, param[i]);
	}
	plat_inf("\n");
}

static void client_ack_opensbi(uint32_t head, uint32_t len, uint32_t *param)
{
	uint16_t i;

	if (&pm_inst->ept == NULL || HEAD_GET_ATTR(head) != ARISC_MESSAGE_ATTR_HARDSYN) {
		return ;
	}
	HEAD_CLR_RESULT(head);
	hal_msgbox_channel_send(&pm_inst->ept, (uint8_t *)&head, sizeof(head));
	hal_msgbox_channel_send(&pm_inst->ept, (uint8_t *)&len, sizeof(len));
	for (i = 0; i < len; i++) {
		hal_msgbox_channel_send(&pm_inst->ept, (uint8_t *)&param[i], sizeof(param[0]));
	}
}

static void client_ack_kernel(uint32_t head)
{
	if (&pm_inst->ept == NULL || HEAD_GET_ATTR(head) != ARISC_MESSAGE_ATTR_HARDSYN) {
		return ;
	}
	HEAD_CLR_RESULT(head);
	hal_msgbox_channel_send(&pm_inst->remote_notify_ept, (uint8_t *)&head, sizeof(head));
}

static void pm_set_super_standby_mask(int32_t irq)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(cpu_irq_map); i++) {
		if (cpu_irq_map[i].cpux_irq == irq) {
			super_standby_wakeup_mask |= cpu_irq_map[i].mask;
			break;
		}
	}
}

static void pm_clear_super_standby_mask(int32_t irq)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(cpu_irq_map); i++) {
		if (cpu_irq_map[i].cpux_irq == irq) {
			super_standby_wakeup_mask &= ~cpu_irq_map[i].mask;
			break;
		}
	}
}

static void pm_calculate_real_wakeup_mask(suspend_mode_t mode)
{
	if (mode == PM_MODE_HIBERNATION) {
		real_wakeup_mask = hibernation_wakeup_mask;
	} else if (mode == PM_MODE_STANDBY) {
		if (pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
			real_wakeup_mask = ultra_standby_wakeup_mask;
		}
		else if (pm_get_standby_mode() == PM_STANDBY_MODE_SUPER) {
			real_wakeup_mask = super_standby_wakeup_mask;
			if (super_standby_wakeup_mask & PM_WAKEUP_MASK_IO) {
				real_wakeup_mask = super_standby_wakeup_mask & ~(PM_WAKEUP_MASK_IO);
				real_wakeup_mask |= super_standby_io_mask;
			}
			/* super standby will not set wakeup timer mask in client, must set here */
			if (dram_standby_head->wuptimer_ms) {
				real_wakeup_mask |= PM_WAKEUP_MASK_WAKEUP_TIMER;
				pm_set_super_standby_mask(CPUX_WUPTIMER_IRQ);
			}
		}
	}
}

static void pm_enable_irq_by_wakeup_mask(pm_wakeup_mask_t mask)
{
	if (mask & PM_WAKEUP_MASK_IO) {
		hal_request_irq(CPUS_WUPIO_IRQ, pm_wupio_irq_callback, "", NULL);
		hal_enable_irq(CPUS_WUPIO_IRQ);
	}
	if (mask & PM_WAKEUP_MASK_RTC_ALARM0) {
		hal_request_irq(CPUS_ALARM0_IRQ, pm_alarm0_irq_callback, "", NULL);
		hal_enable_irq(CPUS_ALARM0_IRQ);
	}
	if (mask & PM_WAKEUP_MASK_RTC_ALARM1) {
		hal_request_irq(CPUS_ALARM1_IRQ, pm_alarm1_irq_callback, "", NULL);
		hal_enable_irq(CPUS_ALARM1_IRQ);
	}
	if (mask & PM_WAKEUP_MASK_WAKEUP_TIMER) {
		hal_request_irq(CPUS_WUPTIMER_IRQ, pm_wuptimer_irq_callback, "", NULL);
		hal_enable_irq(CPUS_WUPTIMER_IRQ);
	}
	if (mask & PM_WAKEUP_MASK_WLAN) {
		hal_request_irq(CPUS_WLAN_IRQ, NULL, "", NULL);
		hal_enable_irq(CPUS_WLAN_IRQ);
	}
}

static void pm_disable_irq_by_wakeup_mask(pm_wakeup_mask_t mask)
{
	if (mask & PM_WAKEUP_MASK_IO) {
		hal_disable_irq(CPUS_WUPIO_IRQ);
		hal_free_irq(CPUS_WUPIO_IRQ);
	}
	if (mask & PM_WAKEUP_MASK_RTC_ALARM0) {
		hal_disable_irq(CPUS_ALARM0_IRQ);
		hal_free_irq(CPUS_ALARM0_IRQ);
	}
	if (mask & PM_WAKEUP_MASK_RTC_ALARM1) {
		hal_disable_irq(CPUS_ALARM1_IRQ);
		hal_free_irq(CPUS_ALARM1_IRQ);
	}
	if (mask & PM_WAKEUP_MASK_WAKEUP_TIMER) {
		hal_disable_irq(CPUS_WUPTIMER_IRQ);
		hal_free_irq(CPUS_WUPTIMER_IRQ);
		if (wuptmr_aov_en) {
			HAL_WUPTIMER_Enable(0);
			dram_standby_head->wuptimer_ms = 0;
			wuptmr_aov_en = 0;
		}
	}
	if (mask & PM_WAKEUP_MASK_WLAN) {
		hal_disable_irq(CPUS_WLAN_IRQ);
		hal_free_irq(CPUS_WLAN_IRQ);
	}
}

static int client_standby_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	if (param[4] == 0x3) {
		client_ack_opensbi(head, len, param);
		cpux_start_address = param[1];
		plat_log("enter super standby\n");
		pm_set_standby_mode(PM_STANDBY_MODE_SUPER);
		pm_suspend_request(PM_MODE_STANDBY);
	}
	return 0;
}

static int client_set_shutdown_type_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	client_ack_opensbi(head, len, param);
	cpux_shutdown_type = param[0];
	return 0;
}

static void pm_uboot_shutdown_task(void *arg)
{
	while (1) {
		pm_cpus_begin(PM_MODE_HIBERNATION);
		hal_interrupt_disable();
#ifndef CONFIG_COMPONENTS_PM_CORE_DSP
		arch_disable_all_irq();
#endif
		pm_cpus_enter(PM_MODE_HIBERNATION);
#ifndef CONFIG_COMPONENTS_PM_CORE_DSP
		arch_enable_all_irq();
#endif
		hal_interrupt_enable();
		/* will nerver goto here */
		plat_err("pm uboot shutdown fail\n");
		hal_thread_stop(pm_uboot_shutdown_thread);
	}
}

static int client_shutdown_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	if (cpux_shutdown_type == 2) {
		client_ack_opensbi(head, len, param);
		pm_uboot_shutdown_thread = hal_thread_create(pm_uboot_shutdown_task, NULL, "pm_uboot_shutdown", 1024, PM_UBOOT_SHUTDOWN_PRIORITY);
		if (pm_uboot_shutdown_thread == NULL) {
			plat_err("uboot shutdown thread create failed\n");
		}
	} else if (cpux_shutdown_type == 1) {
		client_ack_opensbi(head, len, param);
		plat_log("enter ultra standby\n");
		pm_set_standby_mode(PM_STANDBY_MODE_ULTRA);
		pm_suspend_request(PM_MODE_STANDBY);
	} else {
		client_ack_opensbi(head, len, param);
		plat_log("enter hibernation\n");
		pm_suspend_request(PM_MODE_HIBERNATION);
	}
	return 0;
}

static int client_wakeup_sync_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	client_ack_opensbi(head, len, param);
	return 0;
}

static int client_get_wakesrc_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	uint32_t i;
	int32_t irq;
	uint32_t tparam;

	len = 1;
	irq = pm_wakesrc_get_irq();
	for (i = 0; i < ARRAY_SIZE(cpu_irq_map); i++) {
		if (irq == cpu_irq_map[i].cpus_irq) {
			break;
		}
	}
	if (i == ARRAY_SIZE(cpu_irq_map)) {
		tparam = 0;
		client_ack_opensbi(head, len, &tparam);
		plat_err("wrong irq num\n");
		return -1;
	}

	tparam = cpu_irq_map[i].cpux_irq;
	client_ack_opensbi(head, len, &tparam);
	return 0;
}

static int client_set_wakesrc_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	client_ack_opensbi(head, len, param);
	if (GET_WAKESRC_TYPE(param[0]) == 0x0) {
		/* set super standby wakeup mask */
		pm_set_super_standby_mask(GET_ROOT_IRQ(param[0]));
	} else if (GET_WAKESRC_TYPE(param[0]) == 0x1) {
		/* set ultra standby wakeup mask */
		ultra_standby_wakeup_mask = GET_WAKESRC_PARAM(param[0]);
		plat_inf("set ultra standby mask: 0x%08x\n", ultra_standby_wakeup_mask);
	} else if (GET_WAKESRC_TYPE(param[0]) == 0x2) {
		/* set hibernation wakeup mask */
		hibernation_wakeup_mask = GET_WAKESRC_PARAM(param[0]);
		plat_inf("set hibernation mask: 0x%08x\n", hibernation_wakeup_mask);
	} else if (GET_WAKESRC_TYPE(param[0]) == 0x3) {
		/* set wakeup timer duration */
		dram_standby_head->wuptimer_ms = param[0] & 0x3FFFFFFF;
		plat_inf("cpux set wuptimer: %u\n", dram_standby_head->wuptimer_ms);
		if (dram_standby_head->wuptimer_ms) {
			/* linux will not send wuptimer irq to rtos */
			HAL_WUPTIMER_Release();
			HAL_WUPTIMER_ClrIrqPending();
			HAL_WUPTIMER_Enable(0);
			HAL_WUPTIMER_SetInterval(MS_TO_32K(dram_standby_head->wuptimer_ms));
		} else {
			HAL_WUPTIMER_Reset();
		}
	}

	return 0;
}

static int client_clear_wakesrc_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	client_ack_opensbi(head, len, param);
	if (GET_WAKESRC_TYPE(param[0]) == 0x0) {
		/* set super standby wakeup mask */
		pm_clear_super_standby_mask(GET_ROOT_IRQ(param[0]));
	} else if (GET_WAKESRC_TYPE(param[0]) == 0x3) {
		HAL_WUPTIMER_Reset();
	}
	return 0;
}


static int client_set_wakeup_io_num_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	if (param[0] >= WAKEUP_IO_MAX) {
		plat_err("wrong io num\n");
		return -1;
	}
	client_ack_opensbi(head, len, param);
	super_standby_io_mask |= (1 << param[0]);
	plat_inf("set wakeup io: %u\n", param[0]);
	return 0;
}


static int client_cfg_pwr_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	client_ack_opensbi(head, len, param);
	dram_standby_head->pwr_cfg = param[0];
	return 0;
}

static int client_set_wdg_period_handle(uint32_t head, uint32_t *param, uint16_t len)
{
	client_ack_opensbi(head, len, param);
	rtcwdg_suggest_period = param[0];
	pm_log("set wdg period: %u\n", rtcwdg_suggest_period);
	return 0;
}

static int client_post_suspend_msg(uint32_t head, uint32_t *param, uint16_t len)
{
	client_ack_kernel(head);
	if (pm_state_get() == PM_STATUS_PRESLEEP) {
		pm_state_set(PM_STATUS_RUNNING);
	}
	return 0;
}

static int client_suspend_prepare_msg(uint32_t head, uint32_t *param, uint16_t len)
{
	client_ack_kernel(head);
	if (pm_state_get() == PM_STATUS_RUNNING) {
		pm_state_set(PM_STATUS_PRESLEEP);
	}
	return 0;
}

static int client_flash_info_msg(uint32_t head, uint32_t *param, uint16_t len)
{
	client_ack_kernel(head);
	bootpkg_set_start_sector(param[0]);
	gpt_set_start_sector(param[1]);
	dram_standby_head->efuse_start = (void *)param[2];
	dram_standby_head->efuse_size = param[3];
	return 0;
}

static int client_enable_aov_timer_msg(uint32_t head, uint32_t *param, uint16_t len)
{
	if (param[0] > 0) {
		wuptmr_aov_en = 1;
	} else {
		wuptmr_aov_en = 0;
	}
	client_ack_kernel(head);
	return 0;
}

const client_handle_t client_opensbi_handle[] = {
	{ARISC_CPU_OP_REQ,             5, client_standby_handle},
	{ARISC_CPU_OP_SHUTDOWN_TYPE,   1, client_set_shutdown_type_handle},
	{ARISC_SYS_OP_REQ,             1, client_shutdown_handle},
	{ARISC_STANDBY_RESTORE_NOTIFY, 0, client_wakeup_sync_handle},
	{ARISC_REQUIRE_WAKEUP_SRC_REQ, 0, client_get_wakesrc_handle},
	{ARISC_SET_WAKEUP_SRC_REQ,     1, client_set_wakesrc_handle},
	{ARISC_CLEAR_WAKEUP_SRC_REQ,   1, client_clear_wakesrc_handle},
	{ARISC_WAKEUP_IO_NUM,          1, client_set_wakeup_io_num_handle},
	{ARISC_PWR_CFG,                1, client_cfg_pwr_handle},
	{ARISC_SET_WDG_PERIOD,         1, client_set_wdg_period_handle},

};

const client_handle_t client_kernel_handle[] = {
	{KERNEL_POST_SUSPEND_MSG,      1, client_post_suspend_msg},
	{KERNEL_SUSPEND_PREARE_MSG,    1, client_suspend_prepare_msg},
	{KERNEL_FLASH_INFO_MSG,        4, client_flash_info_msg},
	{KERNEL_ENABLE_AOV_TIMER_MSG,  1, client_enable_aov_timer_msg},
};

static int client_recv_handle(const client_handle_t *handle, uint8_t handle_size, uint32_t head, uint32_t *param, uint16_t len)
{
	uint32_t i;

	for (i = 0; i < handle_size; i++) {
		if (handle[i].type == HEAD_GET_TYPE(head) && handle[i].len == len) {
			dump_param(param, len);
			handle[i].func(head, param, len);
			return 0;
		}
	}
	plat_err("type err\n");
	return -1;
}

void client_recv_machine(struct client_machine_status *status, uint32_t data)
{
	switch (status->step) {
	case STEP_REC_HEAD:
		status->head = data;
		status->cnt = 0;
		status->len = 0;
		status->step = STEP_REC_LEN;
		plat_log("STEP_REC_HEAD state: 0x%02x attr: 0x%02x type: 0x%02x\n",
			   HEAD_GET_STATE(status->head), HEAD_GET_ATTR(status->head), HEAD_GET_TYPE(status->head));
		break;
	case STEP_REC_LEN:
		status->len = GET_LEN(data);
		if (status->len) {
			status->step = STEP_REC_PAYLOAD;
		} else {
			client_recv_handle(status->handle, status->handle_size, status->head, status->param, status->len);
			status->step = STEP_REC_HEAD;
		}
		plat_log("STEP_REC_LEN cnt: 0x%04x len: 0x%04x\n", status->cnt, status->len);
		break;
	case STEP_REC_PAYLOAD:
		if (status->cnt < status->len) {
			status->param[status->cnt++] = data;
			if (status->cnt == status->len) {
				client_recv_handle(status->handle, status->handle_size, status->head, status->param, status->len);
				status->step = STEP_REC_HEAD;
			}
		} else {
			plat_err("payload len err\n");
			status->step = STEP_REC_HEAD;
		}
		break;
	default :
		plat_err("STEP ERROR\n");
		break;
	}
}

static void pm_client_task(void *arg)
{

	struct client_channel_info chan_info;
	struct client_machine_status opensbi_status = {
		.step = STEP_REC_HEAD,
		.handle = client_opensbi_handle,
		.handle_size = ARRAY_SIZE(client_opensbi_handle),
	};
	struct client_machine_status kernel_status = {
		.step = STEP_REC_HEAD,
		.handle = client_kernel_handle,
		.handle_size = ARRAY_SIZE(client_kernel_handle),
	};

	while (1) {
		if (hal_queue_recv(pm_inst->recv_queue, &chan_info, HAL_WAIT_FOREVER) != 0) {
			pm_warn("standby thread read form queue failed\r\n");
			continue;
		}
		if (chan_info.channel == CLIENT_CHANNEL_OPENSBI) {
			plat_inf("opensbi channel recv\n");
			client_recv_machine(&opensbi_status, chan_info.data);
		} else if (chan_info.channel == CLIENT_CHANNEL_KERNEL) {
			plat_inf("kernel channel recv\n");
			client_recv_machine(&kernel_status, chan_info.data);
		}
	}
}

static void pm_client_recv_callback(uint32_t data, void *priv)
{
	int ret;
	struct client_channel_info chan_info;

	//plat_log("opensbi rec data: 0x%08x\n", data);
	chan_info.channel = CLIENT_CHANNEL_OPENSBI;
	chan_info.data = data;
	ret = hal_queue_send(pm_inst->recv_queue, &chan_info);
	if (ret != 0) {
		plat_err("send queue failed\n");
	}
}

static void pm_remote_notify_recv_callback(uint32_t data, void *priv)
{
	int ret;
	struct client_channel_info chan_info;

	//plat_log("kernel rec data: 0x%08x\n", data);
	chan_info.channel = CLIENT_CHANNEL_KERNEL;
	chan_info.data = data;
	ret = hal_queue_send(pm_inst->recv_queue, &chan_info);
	if (ret != 0) {
		plat_err("send queue failed\n");
	}
}

int pm_client_init(void)
{
	int ret;

	pm_inst = hal_malloc(sizeof(*pm_inst));
	if (!pm_inst) {
		plat_err("pm_inst alloc failed\r\n");
		ret = -ENOMEM;
		goto exit;
	}

	pm_inst->recv_queue = hal_queue_create("standby-queue", sizeof(struct client_channel_info), 8);
	if (!pm_inst->recv_queue) {
		plat_err("queue create failed\r\n");
		ret = -ENOMEM;
		goto queue_err;
	}

	pm_inst->thread = hal_thread_create(pm_client_task, NULL, "pm_client", 1024, PM_CLIENT_PRIORITY);
	if (pm_inst->thread == NULL) {
		plat_err("thread create failed\n");
		ret = -ENOMEM;
		goto thread_err;
	}
	pm_inst->ept.rec = pm_client_recv_callback;
	pm_inst->ept.private = NULL;
	 /* used msgbox channel p=3 */
	ret = hal_msgbox_alloc_channel(&pm_inst->ept, MSGBOX_PM_REMOTE, MSGBOX_PM_RECV_CHANNEL, MSGBOX_PM_SEND_CHANNEL);
	if (ret) {
		plat_err("channel alloc failed(remote:%d rx:%d tx:%d)\n",
					MSGBOX_PM_REMOTE, MSGBOX_PM_RECV_CHANNEL, MSGBOX_PM_SEND_CHANNEL);
		ret = -EINVAL;
		goto channel_err;
	}

	pm_inst->remote_notify_ept.rec = pm_remote_notify_recv_callback;
	pm_inst->remote_notify_ept.private = NULL;
	ret = hal_msgbox_alloc_channel(&pm_inst->remote_notify_ept, MSGBOX_PM_REMOTE, MSGBOX_PM_NOTIFY_RECV_CHANNEL, MSGBOX_PM_NOTIFY_SEND_CHANNEL);
	if (ret) {
		plat_err("channel alloc failed(remote:%d rx:%d tx:%d)\n",
					MSGBOX_PM_REMOTE, MSGBOX_PM_NOTIFY_RECV_CHANNEL, MSGBOX_PM_NOTIFY_SEND_CHANNEL);
		ret = -EINVAL;
		goto notify_channel_err;
	}

	goto exit;

notify_channel_err:
	hal_msgbox_free_channel(&pm_inst->ept);
channel_err:
	hal_thread_stop(pm_inst->thread);
	pm_inst->thread = NULL;
thread_err:
	hal_queue_delete(pm_inst->recv_queue);
	pm_inst->recv_queue = NULL;
queue_err:
	hal_free(pm_inst);
	pm_inst = NULL;
exit:
	return ret;
}

int pm_client_deinit(void)
{
	if (pm_inst == NULL) {
		return -ENOMEM;
	}
	hal_msgbox_free_channel(&pm_inst->remote_notify_ept);
	hal_msgbox_free_channel(&pm_inst->ept);
	hal_thread_stop(pm_inst->thread);
	pm_inst->thread = NULL;

	hal_queue_delete(pm_inst->recv_queue);
	pm_inst->recv_queue = NULL;

	hal_free(pm_inst);
	pm_inst = NULL;
	return 0;
}

int adjust_rtos_info(elf_info *info)
{
	uint32_t i;
	uint32_t offset;

	/*
	 * flash driver must at the first of rtos image,
	 * because rtos and standby firmware will share flash driver,
	 * standby firmware should load rtos without flash driver,
	 * so we should check whether flash driver info is right,
	 * and adjust the rtos info.
	 */
	if (info->section_num == 0) {
		return -1;
	}
	for (i = 0; i < info->section_num; i++) {
		if ((info->section[i].dst_addr == (uint32_t)&__flash_driver_start__) &&
			(info->section[i].file_size >= ((uint32_t)&__flash_driver_end__ - (uint32_t)&__flash_driver_start__))) {
			offset = (uint32_t)&__flash_driver_end__ - (uint32_t)&__flash_driver_start__;
			info->section[i].src_addr += offset;
			info->section[i].dst_addr += offset;
			info->section[i].file_size -= offset;
			info->section[i].mem_size -= offset;
			return 0;
		}
	}
	return -1;
}

char* get_rtos_partition_name(void)
{
	if (HAL_GET_BIT_VAL(HAL_REG_32BIT(PRCM + PRCM_SYS_PRIV0),
		PRCM_RTOS_PARTITION_SHIFT, PRCM_RTOS_PARTITION_VMASK) == 1) {
		return "riscv0-r";
	}
	return "riscv0";
}

int get_rtos_info(elf_info *rtos, rproc_info *rproc)
{
	int ret = 0;
	uint32_t sector, sector_num;
	char* name;

	name = get_rtos_partition_name();
	gpt_init();
	ret = get_partition_by_name(name, &sector, &sector_num);
	if (ret) {
		plat_err("get rtos partition fail\n");
		goto exit;
	}
	ret = parse_rtos_fw(sector, sector_num, rtos, rproc);
	if (ret) {
		plat_err("parse rtos elf fail\n");
		goto exit;
	}
	ret = adjust_rtos_info(rtos);
	if (ret) {
		plat_err("adjust rtos info fail\n");
	}

exit:
	gpt_deinit();
	return ret;
}

int get_standby_pmboot_info(bin_info *info)
{
	int ret = 0;
	uint32_t pmboot_addr, pmboot_size;

	bootpkg_init();
	ret = get_bootpkg_by_name(PMBOOT_ITEM_NAME, &pmboot_addr, &pmboot_size);
	bootpkg_deinit();
	if (ret) {
		plat_err("get pmboot fail\n");
		goto exit;
	}

	ret = parse_standby_pmboot_info(pmboot_addr, pmboot_size, info);
	if (ret) {
		plat_err("parse pmboot fail\n");
		goto exit;
	}
exit:
	return ret;
}

static int pll_restore_late(void)
{
	pll_param_t pll_param;

	/* open csi pll */
	HAL_CCU_ClosePll(PLL_IDX_CSI);
	if (HAL_CCU_GetHoscFreq() == HOSC_CLOCK_40M) {
		HAL_CCU_ConfigPll(&pll_param, PLL_N_67, PLL_CSI_D_4);
		HAL_CLR_SET_BITS(CCU_AON + PLL_CSI_PAT0_REG, SPR_FREQ_MODE_MASK, 0x3 << SPR_FREQ_MODE_SHIFT);
		HAL_CLR_SET_BITS(CCU_AON + PLL_CSI_PAT0_REG, WAVE_BOT_MASK, 0x10000 << WAVE_BOT_SHIFT);
	} else {
		HAL_CCU_ConfigPll(&pll_param, PLL_N_56, PLL_D_2);
		HAL_CLR_SET_BITS(CCU_AON + PLL_CSI_PAT0_REG, SPR_FREQ_MODE_MASK, 0x3 << SPR_FREQ_MODE_SHIFT);
		HAL_CLR_SET_BITS(CCU_AON + PLL_CSI_PAT0_REG, WAVE_BOT_MASK, 0x8000 << WAVE_BOT_SHIFT);
	}
	HAL_CLR_SET_BITS(CCU_AON + PLL_CSI_PAT1_REG, SIG_DELT_PAT_EN_MASK, 1 << SIG_DELT_PAT_EN_SHIFT);
	HAL_CCU_OpenPll(PLL_IDX_CSI, &pll_param);
	/* open video pll */
	HAL_CCU_ClosePll(PLL_IDX_VIDEO);
	if (HAL_CCU_GetHoscFreq() == HOSC_CLOCK_40M) {
		HAL_CCU_ConfigPll(&pll_param, PLL_N_30, PLL_D_1);
	} else {
		HAL_CCU_ConfigPll(&pll_param, PLL_N_50, PLL_D_1);
	}
	HAL_CCU_OpenPll(PLL_IDX_VIDEO, &pll_param);
	return 0;
}

static int32_t ccu_is_invalid_app_registers(uint32_t addr)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(ccu_app_invalid_regaddr); i++) {
		if (ccu_app_invalid_regaddr[i] == addr) {
			return 1;
		}
	}
	return 0;
}

static int ccu_backup_app_registers(void)
{
	uint32_t offset;
	uint32_t cnt;
	uint32_t reg_addr;
	uint32_t reg_last_offset;
	uint32_t reg_num;

	if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821B) {
		reg_last_offset = CCU_APP_REG_LAST_OFFSET_FOR_V821B;
		reg_num = CCU_APP_REG_NUM_FOR_V821B;
	} else {
		reg_last_offset = CCU_APP_REG_LAST_OFFSET_FOR_V821;
		reg_num = CCU_APP_REG_NUM_FOR_V821;
	}

	ccu_app_reg = hal_malloc(sizeof(uint32_t) * reg_num);
	if (ccu_app_reg == NULL) {
		return -1;
	}
	cnt = 0;
	for (offset = 0; offset <= reg_last_offset; offset += 4) {
		reg_addr = CCU_APP + offset;
		if (ccu_is_invalid_app_registers(reg_addr)) {
			continue;
		}
		ccu_app_reg[cnt++] = HAL_REG_32BIT(reg_addr);
	}
	return 0;
}

static int ccu_restore_app_registers(void)
{
	uint32_t offset;
	uint32_t cnt;
	uint32_t reg_addr;
	uint32_t i;
	uint32_t reg_last_offset;

	if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821B) {
		reg_last_offset = CCU_APP_REG_LAST_OFFSET_FOR_V821B;
	} else {
		reg_last_offset = CCU_APP_REG_LAST_OFFSET_FOR_V821;
	}

	cnt = 0;
	for (offset = 0; offset <= reg_last_offset; offset += 4) {
		reg_addr = CCU_APP + offset;
		if (ccu_is_invalid_app_registers(reg_addr)) {
			continue;
		}
		for (i = 0; i < ARRAY_SIZE(ccu_spc_reg); i++) {
			if (ccu_spc_reg[i].reg_addr == reg_addr) {
				break;
			}
		}
		if (i == ARRAY_SIZE(ccu_spc_reg)) {
			HAL_REG_32BIT(reg_addr) = ccu_app_reg[cnt++];
		} else {
			HAL_REG_32BIT(ccu_spc_reg[i].reg_addr) = ccu_app_reg[cnt++] & ccu_spc_reg[i].reg_mask;
		}
	}
	if (ccu_app_reg) {
		hal_free(ccu_app_reg);
		ccu_app_reg = NULL;
	}
	return 0;
}

static int ccu_restore_app_registers_late(void)
{
	uint32_t i;

	/* restore ccu registers to default value */
	for (i = 0; i < ARRAY_SIZE(ccu_spc_reg); i++) {
		HAL_REG_32BIT(ccu_spc_reg[i].reg_addr) = ((ccu_spc_reg[i].reg_def | HAL_REG_32BIT(ccu_spc_reg[i].reg_addr)) & ~ccu_spc_reg[i].reg_mask) |
			                                     (HAL_REG_32BIT(ccu_spc_reg[i].reg_addr) & ccu_spc_reg[i].reg_mask);
	}
	return 0;
}

static void rtc_rcsrc_cal_set(uint32_t bak)
{
	uint32_t val;

	val = HAL_REG_32BIT(SYSRTC_CTRL_REG);

	if (bak) {
		/* close rtc rcosc cal because of DCXO off when rtc clk from rcosc */
		if ((val & (SYSRTC_CLK_SRC_MASK << SYSRTC_LFCLK_SRC_SEL_SHIFT)) == 0x0) {
			rtc_ctrl_backup = val;
			HAL_REG_32BIT(SYSRTC_CTRL_REG) = rtc_ctrl_backup & ~(SYSRTC_RCOSC_CAL_MASK << SYSRTC_RCOSC_CAL_SHIFT);
		}
	} else {
		if ((val & (SYSRTC_CLK_SRC_MASK << SYSRTC_LFCLK_SRC_SEL_SHIFT)) == 0x0)
			HAL_REG_32BIT(SYSRTC_CTRL_REG) = val | (rtc_ctrl_backup & (SYSRTC_RCOSC_CAL_MASK << SYSRTC_RCOSC_CAL_SHIFT));
	}
}

static void debug_image_info(void)
{
	uint32_t i;

	plat_inf("##image pmboot info##\n");
	plat_inf("dst_addr: 0x%08x\n", pmboot_info.dst_addr);
	plat_inf("src_addr: 0x%08x\n", pmboot_info.src_addr);
	plat_inf("size: 0x%08x\n", pmboot_info.size);

	plat_inf("##image rtos fw info##\n");
	plat_inf("flash_rtos_addr: 0x%08x\n", rtos_rproc_info.flash_rtos_addr);
	plat_inf("rsvmem_rtos_addr: 0x%08x\n", rtos_rproc_info.rsvmem_rtos_addr);
	plat_inf("runmem_rtos_addr: 0x%08x\n", rtos_rproc_info.runmem_rtos_addr);
	plat_inf("header_size: 0x%08x\n", rtos_rproc_info.ehdr_size);
	plat_inf("header_size: 0x%08x\n", rtos_rproc_info.shdr_offset);
	plat_inf("header_size: 0x%08x\n", rtos_rproc_info.shdr_size);
	plat_inf("strtab_offset: 0x%08x\n", rtos_rproc_info.strtab_offset);
	plat_inf("strtab_size: 0x%08x\n", rtos_rproc_info.strtab_size);
	plat_inf("rsctab_offset: 0x%08x\n", rtos_rproc_info.rsctab_offset);
	plat_inf("rsctab_size: 0x%08x\n", rtos_rproc_info.rsctab_size);
	plat_inf("runmem_rsctab_offset: 0x%08x\n", rtos_rproc_info.runmem_rsctab_offset);

	plat_inf("##image rtos elf info##\n");
	plat_inf("section_num: 0x%08x\n", dram_standby_head->rtos_info.section_num);
	for (i = 0; i < dram_standby_head->rtos_info.section_num; i++) {
		plat_inf("section[%u].dst_addr: 0x%08x\n", i, dram_standby_head->rtos_info.section[i].dst_addr);
		plat_inf("section[%u].src_addr: 0x%08x\n", i, dram_standby_head->rtos_info.section[i].src_addr);
		plat_inf("section[%u].file_size: 0x%08x\n", i, dram_standby_head->rtos_info.section[i].file_size);
		plat_inf("section[%u].mem_size: 0x%08x\n", i, dram_standby_head->rtos_info.section[i].mem_size);
	}
	plat_inf("dram_size: 0x%08x\n", dram_size);
}

static void debug_memory_backup_info(suspend_mode_t mode, struct heap_backup_info *info)
{
	if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
		plat_log("##memory backup info##\n");
		plat_inf("firmware_size: 0x%x\n", dram_standby_head->sram_stack_top - dram_standby_head->sram_head_start);
		plat_inf("flash_driver_size: 0x%x\n", dram_standby_head->flash_driver_end - dram_standby_head->flash_driver_start);
		plat_inf("efuse_memory_size: 0x%x\n", dram_standby_head->efuse_size);
		plat_log("data_size: 0x%x\n", (uint32_t)&__data_end__ - (uint32_t)&__data_start__);
		plat_log("bss_size: 0x%x\n", (uint32_t)&__bss_end__ - (uint32_t)&__bss_start__);
		plat_log("data_saved_size: 0x%x\n", (uint32_t)&__stby_saved_data_end__ - (uint32_t)&__stby_saved_data_start__);
		plat_log("bss_saved_size: 0x%x\n", (uint32_t)&__stby_saved_bss_end__ - (uint32_t)&__stby_saved_bss_start__);
		plat_log("heap_backup_size: 0x%x\n", info->backup_size);
		plat_inf("heap_header_num: %u\n", info->header_num);
		plat_inf("heap_payload_num: %u\n", info->payload_num);
		plat_log("sram_backup_size: 0x%x\n", (dram_standby_head->sram_stack_top - dram_standby_head->sram_head_start) + \
			                                 (dram_standby_head->flash_driver_end - dram_standby_head->flash_driver_start) + \
			                                 (dram_standby_head->efuse_size) +\
			                                 ((uint32_t)&__data_end__ - (uint32_t)&__data_start__) + \
			                                 ((uint32_t)&__bss_end__ - (uint32_t)&__bss_start__) + \
			                                 ((uint32_t)&__stby_saved_data_end__ - (uint32_t)&__stby_saved_data_start__) + \
			                                 ((uint32_t)&__stby_saved_bss_end__ - (uint32_t)&__stby_saved_bss_start__) + \
			                                 (info->backup_size));

		plat_inf("data_unsaved_size: 0x%x\n", (uint32_t)&__stby_unsaved_data_end__ - (uint32_t)&__stby_unsaved_data_start__);
		plat_inf("bss_unsaved_size: 0x%x\n", (uint32_t)&__stby_unsaved_bss_end__ - (uint32_t)&__stby_unsaved_bss_start__);
	}
}

static int debug_standby_main_check(uint32_t err)
{
	if (err == STANDBY_ENTER_ERR_NONE) {
		return 0;
	}
	if (err == STANDBY_ENTER_ERR_SRAM_OVERFLOW) {
		plat_err("##sram overflow##\n");
	}
	return -1;
}

#ifdef CONFIG_COMPONENTS_OPENAMP
extern int deinit_openamp_sync(void);
#endif
#ifdef CONFIG_MULTI_CONSOLE
extern void multiple_console_early_deinit(void);
#endif
static int pm_cpus_pre_begin(suspend_mode_t mode)
{
	int ret = 0;
#ifdef CONFIG_AMP_SHARE_IRQ
	share_irq_info *p = NULL;
#endif

	switch (mode) {
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		pm_wakesrc_clr_irq();
		if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
			plat_inf("cpus pre begin\n");
#ifdef CONFIG_VIN_USE_PM
			csi_rpmsg_link_deinit();
#endif
			plat_inf("csi deinit finish\n");
#ifdef CONFIG_DRIVERS_BLUETOOTH_XRADIO
			bt_xradio_link_deinit(1);
#endif
			plat_inf("bt deinit finish\n");
#ifdef CONFIG_DRIVERS_V821_USE_SIP_WIFI
			wlan_smac_rpmsg_deinit(1);
#endif
			plat_inf("wlan smac deinit finish\n");
#ifdef CONFIG_INIT_NET_STACK
			wlan_fmac_xrlink_deinit(1);
#endif
			plat_inf("wlan fmac deinit finish\n");
#ifdef CONFIG_COMPONENTS_OPENAMP
#ifdef CONFIG_AMP_SHARE_IRQ
			/* save share irq info */
			dram_standby_head->sirq_info = hal_malloc(sizeof(share_irq_info));
			p = dram_standby_head->sirq_info;
			int sunxi_pm_save_share_info(uint32_t *gpio_irqs, uint32_t *gpio_banks, uint32_t *bank_num, uint32_t size);
			sunxi_pm_save_share_info(p->gpio_irqs , p->gpio_banks, &p->bank_num, MAX_GPIO_BANK);
#endif

			deinit_openamp_sync();
#endif
			plat_inf("openamp deinit finish\n");
#ifdef CONFIG_MULTI_CONSOLE
			multiple_console_early_deinit();
#endif
			plat_inf("console deinit finish\n");
		}
		break;
	default:
		break;
	}
	return ret;
}

static int pm_cpus_begin(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		pm_time_record_start(PM_TIME_RECORD_CLOSE_CPUX);
		/* wait CPUX enter WFI */
		while (!HAL_CPUX_CFG_IsEnterWfi()) {
			hal_msleep(2);
		}
		plat_inf("cpux enter wfi success\n");

		/* backup cpux clk_src, clk_div, clk_en */
		cpux_clk_reg_backup = HAL_REG_32BIT(CCU_AON + CPUX_CLK_REG);
		/* backup cpux mt_clk_sel, mt_clk_en */
		cpux_mt_clk_reg_backup = HAL_REG_32BIT(CCU_APP + CPUX_MT_CLK_REG);
		/* backup cpux bus_clk_en, cfg_clk_en*/
		ccu_app_clk_reg_backup = HAL_REG_32BIT(CCU_APP + CCU_APP_CLK_REG);

		HAL_CCU_ResetCPUX(0);
		HAL_CCU_EnableCPUXClk(0);
		HAL_CCU_EnableCPUXMtClk(0);
		HAL_CCU_ResetCPUXMsgbox(0);
		HAL_CCU_EnableCPUXMsgboxClk(0);
		HAL_PWRCTRL_EnableCPUXPowerDomain(0);

		if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
			if (!rtos_rproc_info.ehdr_size || !dram_standby_head->rtos_info.section_num || !pmboot_info.size) {
				flash_init();
				/* parse rtos fw and rtos elf */
				ret = get_rtos_info(&dram_standby_head->rtos_info, &rtos_rproc_info);
				if (ret) {
					memset(&dram_standby_head->rtos_info, 0x00, sizeof(dram_standby_head->rtos_info));
					memset(&rtos_rproc_info, 0x00, sizeof(rtos_rproc_info));
				}
				/* parse  pmboot */
				ret = get_standby_pmboot_info(&pmboot_info);
				if (ret) {
					memset(&pmboot_info, 0x00, sizeof(pmboot_info));
				}
				flash_deinit();
			}
			/* set dram size */
			dram_size = 64;
			debug_image_info();
		}

		pm_client_deinit();

		pm_calculate_real_wakeup_mask(mode);
		pm_enable_irq_by_wakeup_mask(real_wakeup_mask);
		plat_inf("real_wakeup_mask: 0x%08x\n", real_wakeup_mask);

		pm_rtcwdg_takeover(mode);

		pm_time_record_end(PM_TIME_RECORD_CLOSE_CPUX);
		break;
	default:
		break;
	}
	return ret;
}

static int pm_cpus_prepare_notify(suspend_mode_t mode)
{
#ifdef CONFIG_PM_DEBUG_WATCHDOG_RESET
	pm_dbg_start_watchdog();
#endif
	pm_cpus_pre_begin(mode);
	pm_cpus_begin(mode);
	return 0;
}

static int pm_cpus_end(suspend_mode_t mode)
{
	switch (mode) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		pm_time_record_start(PM_TIME_RECORD_OPEN_CPUX);

		pm_rtcwdg_giveback(mode);

		pm_disable_irq_by_wakeup_mask(real_wakeup_mask);
		if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
			/* Fastboot special actions.
			 * Requires the interface to adapt to whether it is fastboot.
			 */
			platform_fastboot_init_early();

			flash_init();
			/* load part of rtos fw */
			load_rtos_fw(&rtos_rproc_info);
			/* reset openamp resource table */
			memcpy((void *)(rtos_rproc_info.runmem_rtos_addr + rtos_rproc_info.runmem_rsctab_offset),
			       (void *)(rtos_rproc_info.rsvmem_rtos_addr + rtos_rproc_info.rsctab_offset),
			       rtos_rproc_info.rsctab_size);
			HAL_CACHE_CleanDcache(rtos_rproc_info.runmem_rtos_addr + rtos_rproc_info.runmem_rsctab_offset,
			                      rtos_rproc_info.rsctab_size);

			/* load pmboot */
			load_bin(&pmboot_info);
			flash_deinit();
#ifdef CONFIG_VIN_USE_PM
			/* when standby is not apply, it should load isp_param from flash */
			load_isp_param_from_flash();
#endif
			/* standby_boot0 didnt init dram, so set the dram size to standby_boot0 head */
			reload_dram_size(pmboot_info.dst_addr, dram_size);
			HAL_CACHE_CleanDcache(pmboot_info.dst_addr, pmboot_info.size);

			/* set cpux start address */
			cpux_start_address = pmboot_info.dst_addr;
		}

		pm_client_init();

		HAL_PWRCTRL_EnableCPUXPowerDomain(1);
		HAL_CCU_EnableCPUXMsgboxClk(1);
		HAL_CCU_ResetCPUXMsgbox(1);
		/* restore cpux clk_src, clk_div, clk_en */
		if (mode == PM_MODE_STANDBY) {
			if (pm_get_standby_mode() == PM_STANDBY_MODE_SUPER) {
				HAL_REG_32BIT(CCU_AON + CPUX_CLK_REG) = (cpux_clk_reg_backup & (CLK_DIV_VMASK << CLK_DIV_SHIFT));
				HAL_REG_32BIT(CCU_AON + CPUX_CLK_REG) |= (cpux_clk_reg_backup & (CLK_SEL_3BIT_VMASK << CLK_SEL_SHIFT));
				HAL_REG_32BIT(CCU_AON + CPUX_CLK_REG) |= (cpux_clk_reg_backup & (CPUX_CLK_EN_VMASK << CPUX_CLK_EN_SHIFT));
			} else if (pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
				HAL_REG_32BIT(CCU_AON + CPUX_CLK_REG) = CPUX_CLK_DIV_1;
				HAL_REG_32BIT(CCU_AON + CPUX_CLK_REG) |= CPUX_CLK_SRC_CPU_PLL;
				HAL_REG_32BIT(CCU_AON + CPUX_CLK_REG) |= CPUX_CLK_ON;
			}
		}

		/* resotre cpux mt_clk_sel, mt_clk_en */
		HAL_REG_32BIT(CCU_APP + CPUX_MT_CLK_REG) = cpux_mt_clk_reg_backup;
		/* restore cpux bus_clk_en, cfg_clk_en*/
		HAL_REG_32BIT(CCU_APP + CCU_APP_CLK_REG) = ccu_app_clk_reg_backup;

		HAL_CCU_ResetCPUXCfg(1);
		HAL_CPUX_CFG_EnableWfiMode(0);
		HAL_CPUX_CFG_SetStartAddr(cpux_start_address);
		HAL_CCU_ResetCPUX(1);
		plat_inf("power up and release cpux 0x%08x\n", cpux_start_address);
		pm_time_record_end(PM_TIME_RECORD_OPEN_CPUX);

#ifdef CONFIG_WAIT_SPIF_CONTROLLER
#define RTC_DATA_REGS(n)			(0x4a000000 + 0x200 + (n) * 4)
#define FLASH_FLAGS_INDEX			(CONFIG_SPIF_WAIT_INDEX)
#define FLASH_FLAGS_BIT				(CONFIG_SPIF_WAIT_BIT)
		if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
			HAL_REG_32BIT(RTC_DATA_REGS(FLASH_FLAGS_INDEX)) &= ~(1 << FLASH_FLAGS_BIT);
			extern void load_ramdisk_sync(void);
			load_ramdisk_sync();
		}
#endif

		break;
	default:
		break;
	}
	return 0;
}
#ifdef CONFIG_MULTI_CONSOLE
extern int multiple_console_early_init(void);
#endif
#ifdef CONFIG_COMPONENTS_OPENAMP
extern int init_openamp(void);
#endif
#if defined(CONFIG_PRELOAD_RAMDISK) || defined(CONFIG_PRELOAD_ROOTFS)
extern int rpmsg_notify(char *name, void *data, int len);
extern void rpmsg_notify_init();
#endif
int pm_cpus_post_end(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
#ifdef CONFIG_MULTI_CONSOLE
			multiple_console_early_init();
#endif
#ifdef CONFIG_COMPONENTS_OPENAMP
			pm_state_set(PM_STATUS_PRERUNNING);
			init_openamp();
#endif
#ifdef CONFIG_VIN_USE_PM
			csi_rpmsg_link_init();
#endif
#if defined(CONFIG_PRELOAD_RAMDISK) || defined(CONFIG_PRELOAD_ROOTFS)
			rpmsg_notify_init();
#ifdef CONFIG_PRELOAD_RAMDISK
			rpmsg_notify("ramdisk", NULL, 0);
#endif
			rpmsg_notify("spif", NULL, 0);
#endif
		}
		break;
	default:
		break;
	}
	return ret;
}

static int pm_cpus_finish_notify(suspend_mode_t mode)
{
	pm_cpus_end(mode);
	pm_cpus_post_end(mode);
#ifdef CONFIG_PM_DEBUG_WATCHDOG_RESET
	pm_dbg_stop_watchdog();
#endif
	return 0;
}

static int pm_enter_standby_main(suspend_mode_t mode)
{
	int ret = 0;

#ifdef CONFIG_PM_HEAP_RETAIN
	heap_retain_stop();
	return -1;
#endif
#ifdef CONFIG_PM_DEBUG_POWERON_REGS_SHOW
	pm_pwron_regs_record(1);
	return -2;
#endif

	/* backup sp */
	__asm__ volatile("mv %0, sp\n" : "=r"(sp_reg_backup));
	/* switch sp */
	__asm__ volatile("mv sp, %0\n" ::"r"(dram_standby_head->sram_stack_top));
	/* sync instruction */
	__asm__ volatile("fence.i");

	ret = dram_standby_head->standby_main();

	/* sync instruction */
	__asm__ volatile("fence.i");
	/* restore sp */
	__asm__ volatile("mv sp, %0\n" ::"r"(sp_reg_backup));

	return ret;
}

static int pm_cpus_enter(suspend_mode_t mode)
{
	int ret = 0;
	uint32_t i;
	uint32_t suspend_record = 0;
	uint32_t resume_record = 0;
#ifdef CONFIG_AMP_SHARE_IRQ
	share_irq_info *p = NULL;
#endif
	/* time record */
	suspend_record = HAL_TS_GetUs();

#if defined(CONFIG_COMPONENTS_VIRT_LOG) && defined(CONFIG_PM_STANDBY_MEMORY) && !defined(CONFIG_ULTRA_STANDBY_VIRT_LOG)
	if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
		/* drop virtlog data */
		virt_log_deinit();
	}
#endif
	rtc_rcsrc_cal_set(1);

	switch (mode) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		/* reset err info */
		dram_standby_head->err = STANDBY_ENTER_ERR_NONE;
		memset(&dram_standby_head->heap_info, 0, sizeof(dram_standby_head->heap_info));

		/* mark sram enter boot mode */
		HAL_REG_32BIT(PRCM_SYS_PRIV0) &= ~(1 << 0);
		HAL_SRAM_Init();

		/* load standby bin to sram */
		memcpy((uint8_t*)dram_standby_head->sram_code_start,
			(uint8_t*)dram_standby_head + (dram_standby_head->sram_code_start - dram_standby_head->sram_head_start),
			dram_standby_head->sram_code_end - dram_standby_head->sram_code_start);

		/* clear bss section */
		memset((uint8_t*)dram_standby_head->sram_bss_start, 0, dram_standby_head->sram_bss_end - dram_standby_head->sram_bss_start);

		/* set rtos data section */
		dram_standby_head->rtos_data_start = &__data_start__;
		dram_standby_head->rtos_data_end = &__data_end__;
		/* set rtos bss section */
		dram_standby_head->rtos_bss_start = &__bss_start__;
		dram_standby_head->rtos_bss_end = &__bss_end__;
		/* set rtos saved data section */
		dram_standby_head->rtos_data_saved_start = &__stby_saved_data_start__;
		dram_standby_head->rtos_data_saved_end = &__stby_saved_data_end__;
		/* set rtos saved bss section */
		dram_standby_head->rtos_bss_saved_start = &__stby_saved_bss_start__;
		dram_standby_head->rtos_bss_saved_end = &__stby_saved_bss_end__;
		/* set rtos heap section */
		dram_standby_head->rtos_heap_start = heap_get_start();
		dram_standby_head->rtos_heap_end = heap_get_end();
		/* set standby mode */
		dram_standby_head->pm_mode = mode;
		dram_standby_head->standby_mode = pm_get_standby_mode();
		/* set flash driver info */
		dram_standby_head->flash_driver_start = &__flash_driver_start__;
		dram_standby_head->flash_driver_end = &__flash_driver_end__;
		/* set boot param start sector for flash */
		dram_standby_head->boot_param_sector_start = bootpkg_get_start_sector();
		/* set wlan keep alive time */
#ifdef CONFIG_DRIVERS_XRADIO
		dram_standby_head->wlan_keepalive_time = xradio_get_keepalive_wakeup_time();
#else
		dram_standby_head->wlan_keepalive_time = 0;
#endif
		/* set watchdog suggest period */
		dram_standby_head->wdg_suggest_period = rtcwdg_suggest_period;

		/* parse dram param from sysconfig */
		for (i = 0; i < ARRAY_SIZE(dram_param); i++) {
			hal_cfg_get_keyvalue("dram_para" , dram_param[i].string, (int32_t *)(((uint8_t *)&dram_standby_head->dram_param) + dram_param[i].offset), 1);
		}

		/* set wuptmr for aov scene */
		if (dram_standby_head->wuptimer_ms && wuptmr_aov_en) {
			if ((dram_standby_head->wuptimer_ms & WUPTMR_AOV_MODE_MASK) == 0) {
				wuptmr_aov_set(dram_standby_head->wuptimer_ms);
				dram_standby_head->wuptimer_ms |= WUPTMR_AOV_MODE_MASK;
			}
		}

		/* set chip type */
		if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821B) {
			dram_standby_head->chip_is_v821b = 1;
		} else {
			dram_standby_head->chip_is_v821b = 0;
		}

		/* load head to sram */
		memcpy((uint8_t *)dram_standby_head->sram_head_start, (uint8_t *)dram_standby_head, sizeof(*dram_standby_head));

		if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {

			/* break link all wakelocks */
			pm_wakelock_break_link();

			hal_clock_deinit();

			/* back ccu app registers */
			ccu_backup_app_registers();

			/* handle task waiting terminate */
			freertos_handle_task_waiting_terminate();
		}

		/* time record */
#if defined(CONFIG_PM_TIME_RECORD_PLUS)
		suspend_record = 0;
#endif
		suspend_record = HAL_TS_GetUs() - suspend_record;
		pm_time_record_set(PM_TIME_RECORD_PLATOPS_ENTER_SUSPEND, suspend_record, 0);

		/* enter standby main */
		ret = pm_enter_standby_main(mode);

		/* time record */
		resume_record = HAL_TS_GetUs();

		/* resume head from sram to dram */
		memcpy((uint8_t *)dram_standby_head, (uint8_t *)dram_standby_head->sram_head_start, sizeof(*dram_standby_head));

		/* time record */
		for (i = PM_TIME_RECORD_STANDBY_START; i <= PM_TIME_RECORD_STANDBY_END; i++) {
			pm_time_record_set(i, dram_standby_head->time_record[i - PM_TIME_RECORD_STANDBY_START], 0);
		}

		/* mark sram exit boot mode */
		HAL_REG_32BIT(PRCM_SYS_PRIV0) |= (1 << 0);
		/* clear csi init flag */
		HAL_REG_32BIT(PRCM_SYS_PRIV0) &= ~(1 << 1);

		/* clear wupimer_ms */
		if ((dram_standby_head->wuptimer_ms & WUPTMR_AOV_MODE_MASK) == 0)
			dram_standby_head->wuptimer_ms = 0;

		if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
			/* restore ccu app registers */
			ccu_restore_app_registers();
			/* ccu init */
			hal_clock_init();
		}
#ifdef CONFIG_AMP_SHARE_IRQ
		p = dram_standby_head->sirq_info;
		int sunxi_pm_restore_share_info(uint32_t *gpio_irqs, uint32_t *gpio_banks, uint32_t bank_num, uint32_t size);
		sunxi_pm_restore_share_info(p->gpio_irqs , p->gpio_banks, p->bank_num, MAX_GPIO_BANK);
		hal_free(dram_standby_head->sirq_info);
		dram_standby_head->sirq_info = NULL;

#endif
		break;
	default:
		break;
	}
	rtc_rcsrc_cal_set(0);
#if defined(CONFIG_COMPONENTS_VIRT_LOG) && defined(CONFIG_PM_STANDBY_MEMORY) && !defined(CONFIG_ULTRA_STANDBY_VIRT_LOG)
	if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA)
		virt_log_init();
#endif

#if defined(CONFIG_PM_TIME_RECORD_PLUS)
	resume_record = 0;
#endif
	resume_record = HAL_TS_GetUs() - resume_record;
	pm_time_record_set(PM_TIME_RECORD_PLATOPS_ENTER_RESUME, resume_record, 0);

	return ret;
}

static int pm_cpus_prepare_late(suspend_mode_t mode)
{
	return pm_common_syscore_suspend(mode);
}

static int pm_cpus_wake(suspend_mode_t mode)
{
	return pm_common_syscore_resume(mode);
}

static int pm_cpus_finish(suspend_mode_t mode)
{
#ifdef CONFIG_PM_HEAP_RETAIN
	heap_retain_show();
	heap_retain_deinit();
#endif
#ifdef CONFIG_PM_DEBUG_POWERON_REGS_SHOW
	pm_pwron_regs_record(0);
#endif
	debug_standby_main_check(dram_standby_head->err);
	debug_memory_backup_info(mode, &dram_standby_head->heap_info);
	if (mode == PM_MODE_STANDBY && pm_get_standby_mode() == PM_STANDBY_MODE_ULTRA) {
		pll_restore_late();
		ccu_restore_app_registers_late();
	}
	return 0;
}

static int pm_cpus_again(suspend_mode_t mode)
{
	uint32_t t_cnt = 0, t_inpr = 0, t_inpr_intr = 0;

	while (1) {
		pm_wakesrc_get_inpr_cnt(&t_cnt, &t_inpr, &t_inpr_intr);
		if (pm_wakesrc_cnt_changed(t_cnt)) {
			break;
		}
		if (!t_inpr && !t_inpr_intr) {
			return 1;
		}
		hal_msleep(2);
	}
	return 0;
}

static uint32_t pm_cpus_get_timestamp(void)
{
	return HAL_TS_GetUs();
}

hardware_wakesrc_mask_t pm_cpus_get_hardware_wakesrc_mask(void)
{
	return HAL_GET_BITS(HARDWARE_WAKESRC_REGISTER, HARDWARE_WAKESRC_MASK_ALL);
}

static const suspend_ops_t pm_cpus_ops = {
	.name = "pm_cpus_ops",
	.prepare_notify = pm_cpus_prepare_notify,
	.pre_begin = NULL,
	.begin = NULL,
	.prepare = NULL,
	.prepare_late =pm_cpus_prepare_late,
	.enter = pm_cpus_enter,
	.wake = pm_cpus_wake,
	.finish = pm_cpus_finish,
	.end = NULL,
	.post_end = NULL,
	.recover = NULL,
	.again = pm_cpus_again,
	.again_late = NULL,
	.finish_notify = pm_cpus_finish_notify,
	.get_timestamp = pm_cpus_get_timestamp,
	.get_hardware_wakesrc_mask = pm_cpus_get_hardware_wakesrc_mask,
};

int pm_plat_platops_init(void)
{
	int ret = 0;

	if (sunxi_chip_alter_version() == SUNXI_CHIP_ALTER_VERSION_V821B) {
		/* enable app sleep list bug fix */
		HAL_CLR_SET_BITS(PWRCTRL_AON + PWR_CFG_REG, APP_SLP_LIST_EXT_MASK, 1 << APP_SLP_LIST_EXT_SHIFT);
	} else {
		/* a hardware bug makes VDDSYS current leakage, must set VDDON_CTRL1 to 0 */
		HAL_SET_BIT_VAL(HAL_REG_32BIT(PWRCTRL_RTC + ANA_PWR_RST_REG), SYS_ANA_VDDON_CTRL1_SHIFT, SYS_ANA_VDDON_CTRL1_VMASK, 0);
	}
	/* change DCXO cap array 0*/
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_AON + DCXO_CFG_REG), DCXO_TRIM_V09_SHIFT, DCXO_TRIM_V09_VMASK, 0x0);
	/* decrease Ddie DCXO current */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_AON + DCXO_CFG_REG), DCXO_ICTRL_V09_SHIFT, DCXO_ICTRL_V09_VMASK, 0xd);
	/* change DCXO startup time */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + DCXO_EN_DLY_CNT_REG), DCXO_EN_DLY_CNT_SHIFT, DCXO_EN_DLY_CNT_VMASK, 0x3);
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_AON + DCXO_CFG1_REG), DCXO_CNT_TG_SHIFT, DCXO_CNT_TG_VMASK, 0x3e8);
	/* rccal set wakeup mode */
	HAL_SET_BIT_VAL(HAL_REG_32BIT(PRCM + RCOSC_CALIB_REG1), RCO_WUP_MODE_SEL_SHIFT, RCO_WUP_MODE_SEL_VMASK, 1);

	wuptimer_ws = pm_wakesrc_register(CPUS_WUPTIMER_IRQ, "wuptimer_ws");
	if (wuptimer_ws == NULL) {
		ret = -1;
		goto wuptimer_err;
	}
	wupio_ws = pm_wakesrc_register(CPUS_WUPIO_IRQ, "wupio_ws");
	if (wupio_ws == NULL) {
		ret = -1;
		goto wupio_err;
	}
	alarm0_ws = pm_wakesrc_register(CPUS_ALARM0_IRQ, "alarm0_ws");
	if (alarm0_ws == NULL) {
		ret = -1;
		goto alarm0_err;
	}
	alarm1_ws = pm_wakesrc_register(CPUS_ALARM0_IRQ, "alarm1_ws");
	if (alarm1_ws == NULL) {
		ret = -1;
		goto alarm1_err;
	}

	ret = pm_platops_register((suspend_ops_t *)&pm_cpus_ops);
	if (ret) {
		ret = -1;
		goto platops_err;
	}
	return 0;

platops_err:
	pm_wakesrc_unregister(alarm1_ws);
	alarm1_ws = NULL;
alarm1_err:
	pm_wakesrc_unregister(alarm0_ws);
	alarm0_ws = NULL;
alarm0_err:
	pm_wakesrc_unregister(wupio_ws);
	wupio_ws = NULL;
wupio_err:
	pm_wakesrc_unregister(wuptimer_ws);
	wuptimer_ws = NULL;
wuptimer_err:
	return ret;
}

int pm_plat_platops_deinit(void)
{
	pm_platops_register(NULL);
	pm_wakesrc_unregister(alarm1_ws);
	alarm1_ws = NULL;
	pm_wakesrc_unregister(alarm0_ws);
	alarm0_ws = NULL;
	pm_wakesrc_unregister(wupio_ws);
	wupio_ws = NULL;
	pm_wakesrc_unregister(wuptimer_ws);
	wuptimer_ws = NULL;
	return 0;
}

