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
#include <hal_def.h>
#include <hal_sysctrl.h>
#include <hal_pwrctrl.h>
#include <hal_ccu.h>
#include <hal_ts.h>
#include <hal_cache.h>
#include <hal_audio.h>
#include <hal_prcm.h>
#include <hal_wakeup.h>
#include <hal_alarm.h>
#include <hal_cpus_cfg.h>
#include <hal_rtcwdg.h>
#include <heap_backup.h>
#include <head.h>
#include <flash_head.h>
#include <pm_base.h>
#include <pm_plat.h>
#include <pm_debug.h>
#include <dram.h>
#ifdef CONFIG_RESUME_SENSOR_IN_SRAM
#include "resume_sensor.h"
#endif
#define printf(fmt,...)

extern uint32_t _dram_backup_start;
extern uint32_t _dram_backup_end;
extern void cpu_suspend(void);
extern void hibernation_suspend(void);

#define TIME_RECORD_REBASE(stage)   ((stage) - PM_TIME_RECORD_STANDBY_START)
#define WAKEUP_MASK_ALL             (RTC_TIMER_WAKEUP_MASK | ALARM_WAKEUP_MASK | WAKEUP_IO_WAKEUP_MASK | WLAN_WAKEUP_MASK)
#define WAKEUP_IO_NUM               (8)

#define min(a, b)                   ((a) < (b) ? (a) : (b))
#define max(a,b)                    ((a) < (b) ? (b) : (a))
#define MS_TO_32K(ms)               (ms * 32)
#define HOSC_FREQ_40M               (40000000)
#define RTCWDG_SLEEP_TIME_MARGIN    (3000)

/* clock backup */
static uint32_t cpus_clk_reg_backup = 0;
static uint32_t ahb_clk_reg_backup = 0;
static uint32_t apb_clk_reg_backup = 0;
static uint32_t apb_spc_clk_reg_backup = 0;
static uint32_t rtc_apb_clk_reg_backup = 0;
static uint32_t pll_status_backup = 0;
static uint32_t res_req_ctrl0_reg_backup = 0;
static uint32_t dram_open_power_tick = 0;
static uint32_t ddr_res_calib_reg_backup = 0;
static uint32_t usb_res_calib_reg_backup = 0;
static uint32_t csi_res_calib_reg_backup = 0;

static void update_hardware_wakesrc(void)
{
	uint32_t i;
	uint32_t val;

	/* clear hardware wakesrc rtc register */
	HAL_CLR_BITS(HARDWARE_WAKESRC_REGISTER, HARDWARE_WAKESRC_MASK_ALL);

	val = HAL_REG_32BIT(PRCM + RTC_WAKEUP_IO_STA);
	/* check wakeup io */
	for (i = 0; i <= WAKEUP_IO_NUM; i++) {
		if (val & (1 << i)) {
			HAL_SET_BITS(HARDWARE_WAKESRC_REGISTER, 1 << i);
		}
	}
	/* check wakeup timer */
	if (HAL_GET_BITS(WAKEUP_TIMER + WUP_TMR_VAL, TMR_IRQ_PENDING_VMASK << TMR_IRQ_PENDING_SHIFT)) {
		HAL_SET_BITS(HARDWARE_WAKESRC_REGISTER, HARDWARE_WAKESRC_MASK_WUPTIMER);
	}
	/* check rtc alarm */
	if (HAL_GET_BITS(ALARM + ALARM0_IRQST, ALARM_IRQ_STA_VMASK << ALARM_IRQ_STA_SHIFT)) {
		HAL_SET_BITS(HARDWARE_WAKESRC_REGISTER, HARDWARE_WAKESRC_MASK_ALARM_0);
	}
	if (HAL_GET_BITS(ALARM + ALARM1_IRQST, ALARM_IRQ_STA_VMASK << ALARM_IRQ_STA_SHIFT)) {
		HAL_SET_BITS(HARDWARE_WAKESRC_REGISTER, HARDWARE_WAKESRC_MASK_ALARM_1);
	}
	/* check wlan */
	if (HAL_GET_BITS(WLAN_IRQ_STATUS0_REG, WLAN_IRQ_PENDING_MASK) || HAL_GET_BITS(WLAN_IRQ_STATUS1_REG, WLAN_IRQ_PENDING_MASK)) {
		HAL_SET_BITS(HARDWARE_WAKESRC_REGISTER, HARDWARE_WAKESRC_MASK_WLAN);
	}
}

#if defined(CONFIG_PM_TIME_RECORD)
static uint32_t time_record[PM_TIME_RECORD_MAX] = {0};

static void time_record_start(pm_time_record_t stage)
{
#if defined(CONFIG_PM_TIME_RECORD_PLUS)
#else
	time_record[TIME_RECORD_REBASE(stage)] = HAL_TS_GetUs();
#endif
}

static void time_record_end(pm_time_record_t stage)
{
#if defined(CONFIG_PM_TIME_RECORD_PLUS)
	time_record[TIME_RECORD_REBASE(stage)] = HAL_TS_GetUs();
#else
	time_record[TIME_RECORD_REBASE(stage)] = HAL_TS_GetUs() - time_record[TIME_RECORD_REBASE(stage)];
#endif
}

static void time_record_output(standby_head_t *head)
{
	head->time_record = time_record;
}
#else
static void time_record_start(pm_time_record_t stage) { ; }
static void time_record_end(pm_time_record_t stage) { ; }
static void time_record_output(standby_head_t *head) { ; }
#endif

/* cpu configure backup */
static sysmap_backup_t sysmap_cfg;

static int flash_init(void)
{
	standby_head_t *head = get_standby_head();
	struct flash_head *flash_head = (struct flash_head *)(get_standby_head()->flash_driver_start);
	flash_func_t flash_func = {
		.usleep = NULL,
		.vprintf = NULL,
	};
	flash_head->flash_register_function(&flash_func);
	flash_head->flash_config(&head->boot_param_sector_start);
	return flash_head->flash_init();
}

static int flash_deinit(void)
{
	struct flash_head *flash_head = (struct flash_head *)(get_standby_head()->flash_driver_start);

	return flash_head->flash_deinit();
}

static int flash_read(uint32_t sector, uint32_t sector_num, void *buff)
{
	struct flash_head *flash_head = (struct flash_head *)(get_standby_head()->flash_driver_start);

	return flash_head->flash_read(sector, sector_num, buff);
}

static int load_rtos(elf_info *info)
{
	int ret = 0;
	uint32_t i;

	flash_init();
	if (info->section_num == 0) {
		ret = -1;
		goto error;
	}
	for (i = 0; i < info->section_num; i++) {
		ret = flash_read(info->section[i].src_addr / LOAD_SECTOR_SIZE,
			                 (info->section[i].file_size + (LOAD_SECTOR_SIZE - 1)) / LOAD_SECTOR_SIZE,
			                 (void *)info->section[i].dst_addr);
		if (ret) {
			goto error;
		}
		if (info->section[i].file_size != info->section[i].mem_size) {
			memset((uint8_t *)info->section[i].dst_addr + info->section[i].file_size, 0x00,
				   info->section[i].mem_size - info->section[i].file_size);
		}
	}
error:
	flash_deinit();
	return ret;
}


static void suspend_cache(void)
{
	HAL_CACHE_CleanInvalidAllDcache();
	HAL_CACHE_DisableDcache();
	HAL_SYSMAP_Backup(&sysmap_cfg);
}

static void resume_cache(void)
{
	/* must configure sysmap before cache enable */
	HAL_SYSMAP_Restore(&sysmap_cfg);
	HAL_CACHE_EnableIcache();
	HAL_CACHE_EnableDcache();
}

static void adda_init(void)
{
	//ccu adda pclk en
	HAL_REG_32BIT(CCU_APP + BUS_CLK_GATING1_REG) |= (ADDA_PCLK_EN_VMASK << ADDA_PCLK_EN_SHIFT);
	//wait speedup
	HAL_REG_32BIT(AUDIO + WRA1SPEEDUP_DOWN_CTRL_REG) = 0x0;
	while (!(HAL_REG_32BIT(AUDIO + POWER_REG) & (1 << L_AVCCPOR_SHIFT)));
}

static int suspend_dram(standby_head_t *head)
{
	uint8_t *pbackup;

	if (head->pm_mode == PM_MODE_STANDBY) {
		if (head->standby_mode == PM_STANDBY_MODE_ULTRA) {
			/* backup rtos data to SRAM*/
			time_record_start(PM_TIME_RECORD_DATA_BACKUP);
			pbackup = (uint8_t *)&_dram_backup_start;
			/* backup flash driver */
			memcpy(pbackup, head->flash_driver_start, head->flash_driver_end - head->flash_driver_start);
			pbackup += (head->flash_driver_end - head->flash_driver_start);
			/* backup data */
			memcpy(pbackup, head->efuse_start, head->efuse_size);
			pbackup += head->efuse_size;
			memcpy(pbackup, head->rtos_data_start, head->rtos_data_end - head->rtos_data_start);
			pbackup += (head->rtos_data_end - head->rtos_data_start);
			memcpy(pbackup, head->rtos_bss_start, head->rtos_bss_end - head->rtos_bss_start);
			pbackup += (head->rtos_bss_end - head->rtos_bss_start);
			memcpy(pbackup, head->rtos_data_saved_start, head->rtos_data_saved_end - head->rtos_data_saved_start);
			pbackup += (head->rtos_data_saved_end - head->rtos_data_saved_start);
			memcpy(pbackup, head->rtos_bss_saved_start, head->rtos_bss_saved_end - head->rtos_bss_saved_start);
			pbackup += (head->rtos_bss_saved_end - head->rtos_bss_saved_start);
			if (heap_backup(&head->heap_info, (uint32_t)head->rtos_heap_start, (uint32_t)head->rtos_heap_end,
				            (uint32_t)pbackup, (uint32_t)&_dram_backup_end)) {
				head->err = STANDBY_ENTER_ERR_SRAM_OVERFLOW;
				time_record_end(PM_TIME_RECORD_DATA_BACKUP);
				return -1;
			}
			time_record_end(PM_TIME_RECORD_DATA_BACKUP);

			/* DDR deinit */
			time_record_start(PM_TIME_RECORD_DRAM_SUSPEND);
			dram_power_save_process();
			time_record_end(PM_TIME_RECORD_DRAM_SUSPEND);
		} else if (head->standby_mode == PM_STANDBY_MODE_SUPER) {
			/* clean Dcache to make DDR data right */
			time_record_start(PM_TIME_RECORD_DATA_BACKUP);
			HAL_CACHE_CleanAllDcache();
			time_record_end(PM_TIME_RECORD_DATA_BACKUP);
			/* DDR enter retention */
			time_record_start(PM_TIME_RECORD_DRAM_SUSPEND);
			dram_power_save_process();
			time_record_end(PM_TIME_RECORD_DRAM_SUSPEND);
		}
	} else if (head->pm_mode == PM_MODE_HIBERNATION) {
		/* DDR deinit */
		time_record_start(PM_TIME_RECORD_DRAM_SUSPEND);
		dram_power_save_process();
		time_record_end(PM_TIME_RECORD_DRAM_SUSPEND);
	}
exit:
	return 0;
}

static int resume_dram(standby_head_t *head)
{
	uint8_t *pbackup;

	if (head->pm_mode == PM_MODE_STANDBY && head->standby_mode == PM_STANDBY_MODE_ULTRA) {
		/* DDR init */
		time_record_start(PM_TIME_RECORD_DRAM_RESUME);
		HAL_REG_32BIT(PRCM + BOOT_FLAG_REG) = 0x429B0000;   //dram init need boot flag to be 0
		init_DRAM(0, &head->dram_param);
		time_record_end(PM_TIME_RECORD_DRAM_RESUME);

		time_record_start(PM_TIME_RECORD_LOAD_RTOS);
		pbackup = (uint8_t *)&_dram_backup_start;
		/* restore flash driver */
		memcpy(head->flash_driver_start, pbackup, head->flash_driver_end - head->flash_driver_start);
		pbackup += (head->flash_driver_end - head->flash_driver_start);
		HAL_CACHE_CleanDcache((uint32_t)head->flash_driver_start, (uint32_t)(head->flash_driver_end - head->flash_driver_start));
		/* load rtos */
		load_rtos(&head->rtos_info);
		time_record_end(PM_TIME_RECORD_LOAD_RTOS);
		/* restore DDR from DDR */
		time_record_start(PM_TIME_RECORD_DATA_RESTORE);
		memcpy(head->efuse_start, pbackup, head->efuse_size);
		pbackup += head->efuse_size;
		memcpy(head->rtos_data_start, pbackup, head->rtos_data_end - head->rtos_data_start);
		pbackup += (head->rtos_data_end - head->rtos_data_start);
		memcpy(head->rtos_bss_start, pbackup, head->rtos_bss_end - head->rtos_bss_start);
		pbackup += (head->rtos_bss_end - head->rtos_bss_start);
		memcpy(head->rtos_data_saved_start, pbackup, head->rtos_data_saved_end - head->rtos_data_saved_start);
		pbackup += (head->rtos_data_saved_end - head->rtos_data_saved_start);
		memcpy(head->rtos_bss_saved_start, pbackup, head->rtos_bss_saved_end - head->rtos_bss_saved_start);
		pbackup += (head->rtos_bss_saved_end - head->rtos_bss_saved_start);
		heap_restore((uint32_t)head->rtos_heap_start, (uint32_t)head->rtos_heap_end,
			         (uint32_t)pbackup, (uint32_t)pbackup + head->heap_info.backup_size);
		time_record_end(PM_TIME_RECORD_DATA_RESTORE);
	} else {
		/* DDR exit retention */
		time_record_start(PM_TIME_RECORD_DRAM_RESUME);
		HAL_REG_32BIT(PRCM + BOOT_FLAG_REG) = 0x429B0001;   //dram exit retention need boot flag to be 1
		dram_power_up_process(&head->dram_param);
		HAL_REG_32BIT(PRCM + BOOT_FLAG_REG) = 0x429B0000;
		time_record_end(PM_TIME_RECORD_DRAM_RESUME);
	}
	return 0;
}

static void suspend_dram_power(standby_head_t *head)
{
	uint32_t value;

	if ((head->pm_mode == PM_MODE_STANDBY && head->standby_mode == PM_STANDBY_MODE_ULTRA) ||
		(head->pm_mode == PM_MODE_HIBERNATION)) {
		HAL_PWRCTRL_EnableDramPower(1, 0);
	} else {
		HAL_PWRCTRL_EnableDramPower(1, 1);
	}

	if (head->chip_is_v821b) {
		if (head->pm_mode == PM_MODE_STANDBY) {
			/* set por_sw controlled by sofrware, enable por_sw */
			value = HAL_REG_32BIT(PWRCTRL_RTC + POR_CTRL);
			value |= POR_SW_OVR_EN_MASK;
			value |= POR_SW_OVR_MASK;
			HAL_REG_32BIT(PWRCTRL_RTC + POR_CTRL) = value;

			/* set vcc33_det controlled by sofrware, disable por_sw */
			value = HAL_REG_32BIT(PWRCTRL_RTC + VCC_DET_CTRL);
			value |= VCC33_DET_OVR_EN_MASK;
			value &= ~VCC33_DET_OVR_MASK;
			HAL_REG_32BIT(PWRCTRL_RTC + VCC_DET_CTRL) = value;
		} else if (head->pm_mode == PM_MODE_HIBERNATION) {
			/* set por_sw controlled by pwrctrl */
			value = HAL_REG_32BIT(PWRCTRL_RTC + POR_CTRL);
			value &= ~POR_SW_OVR_EN_MASK;
			value &= ~POR_SW_OVR_MASK;
			HAL_REG_32BIT(PWRCTRL_RTC + POR_CTRL) = value;

			/* set vcc33_det controlled by pwrctrl*/
			value = HAL_REG_32BIT(PWRCTRL_RTC + VCC_DET_CTRL);
			value &= ~VCC33_DET_OVR_EN_MASK;
			value &= ~VCC33_DET_OVR_MASK;
			HAL_REG_32BIT(PWRCTRL_RTC + VCC_DET_CTRL) = value;
		}
	}
}

static void resume_dram_power_early(standby_head_t *head)
{
	if (head->pm_mode == PM_MODE_STANDBY && head->standby_mode == PM_STANDBY_MODE_ULTRA) {
		dram_open_power_tick = HAL_TS_Gettick();
		HAL_PWRCTRL_EnableDramPower(0, 1);
	}
}

static void resume_dram_power_late(standby_head_t *head)
{
	uint32_t value;
	uint32_t durtick, runtick;

	if (head->chip_is_v821b) {
		if (head->pm_mode == PM_MODE_STANDBY) {
			/* disable por_sw */
			value = HAL_REG_32BIT(PWRCTRL_RTC + POR_CTRL);
			value &= ~POR_SW_OVR_MASK;
			HAL_REG_32BIT(PWRCTRL_RTC + POR_CTRL) = value;

			/* enable por_sw */
			value = HAL_REG_32BIT(PWRCTRL_RTC + VCC_DET_CTRL);
			value |= VCC33_DET_OVR_MASK;
			HAL_REG_32BIT(PWRCTRL_RTC + VCC_DET_CTRL) = value;
		}
	}

	if (head->pm_mode == PM_MODE_STANDBY && head->standby_mode == PM_STANDBY_MODE_ULTRA) {
		durtick = HAL_TS_UsToTick(CONFIG_DRAM_POWER_OPEN_DURATION_US);
		runtick = HAL_TS_Gettick() - dram_open_power_tick;
		if (runtick < durtick) {
			HAL_TS_Tdelay(durtick - runtick);
		}
	}
}

static void suspend_clock(standby_head_t *head)
{
	uint32_t i;

	/* backup clk */
	cpus_clk_reg_backup = HAL_REG_32BIT(CCU_AON + CPUS_CLK_REG);
	ahb_clk_reg_backup = HAL_REG_32BIT(CCU_AON + AHB_CLK_REG);
	apb_clk_reg_backup = HAL_REG_32BIT(CCU_AON + APB_CLK_REG);
	apb_spc_clk_reg_backup = HAL_REG_32BIT(CCU_AON + APB_SPC_CLK_REG);
	rtc_apb_clk_reg_backup = HAL_REG_32BIT(CCU_AON + RTC_APB_CLK_REG);
	/* backup pll, except for ddr_pll */
	pll_status_backup = 0;
	for (i = PLL_IDX_BASE; i < PLL_IDX_MAX; i++) {
		if (i == PLL_IDX_DDR) {
			continue;
		}
		if (HAL_CCU_IsPllOn(i) && (i != PLL_IDX_DDR)) {
			pll_status_backup |= (1 << i);
		}
	}

	/* switch clk, switch clk must open video pll */
	HAL_CCU_OpenPll(PLL_IDX_VIDEO, NULL);
	/* clk switch to hosc */
	HAL_REG_32BIT(CCU_AON + CPUS_CLK_REG) &= ~(CPUS_CLK_SEL_MASK);
	HAL_REG_32BIT(CCU_AON + AHB_CLK_REG) &= ~(AHB_CLK_SEL_MASK);
	HAL_REG_32BIT(CCU_AON + APB_CLK_REG) &= ~(APB_CLK_SEL_MASK);
	HAL_REG_32BIT(CCU_AON + APB_SPC_CLK_REG) &= ~(APB_SPC_CLK_SEL_MASK);
	/* clk switch to rc1m */
	HAL_REG_32BIT(CCU_AON + RTC_APB_CLK_REG) &= ~(RTC_APB_CLK_SEL_MASK);
	/* close pll, except for ddr_pll */
	for (i = PLL_IDX_BASE; i < PLL_IDX_MAX; i++) {
		if (i == PLL_IDX_DDR) {
			continue;
		}
		HAL_CCU_ClosePll(i);
	}
}

// set PLL_CPU=hosc*N/D
#ifdef CONFIG_SUNXI_VF_2_1
//1200MHz
#define PLL_CPU_CLK_40M_N PLL_N_60
#define PLL_CPU_CLK_24M_N PLL_N_50
#else
//960MHz
#define PLL_CPU_CLK_40M_N PLL_N_48
#define PLL_CPU_CLK_24M_N PLL_N_40
#endif

static void resume_clock(standby_head_t *head)
{
	uint32_t i;
	pll_param_t pll_param;

	HAL_CCU_EnableMonitor(1);
	if (head->pm_mode == PM_MODE_STANDBY) {
		if (head->standby_mode == PM_STANDBY_MODE_SUPER) {
			/* open pll by backup info */
			for (i = PLL_IDX_BASE; i < PLL_IDX_MAX; i++) {
				if (!(pll_status_backup & (1 << i))) {
					continue;
				}
				HAL_CCU_OpenPll(i, NULL);
			}
			/* switch clk by backup info*/
			HAL_CCU_OpenPll(PLL_IDX_VIDEO, NULL);
			HAL_REG_32BIT(CCU_AON + CPUS_CLK_REG) = (cpus_clk_reg_backup & (CLK_DIV_VMASK << CLK_DIV_SHIFT));
			HAL_REG_32BIT(CCU_AON + CPUS_CLK_REG) |= (cpus_clk_reg_backup & (CLK_SEL_3BIT_VMASK << CLK_SEL_SHIFT));
			HAL_REG_32BIT(CCU_AON + AHB_CLK_REG) = (ahb_clk_reg_backup & (CLK_DIV_VMASK << CLK_DIV_SHIFT));
			HAL_REG_32BIT(CCU_AON + AHB_CLK_REG) |= (ahb_clk_reg_backup & (CLK_SEL_2BIT_VMASK << CLK_SEL_SHIFT));
			HAL_REG_32BIT(CCU_AON + APB_CLK_REG) = (apb_clk_reg_backup & (CLK_DIV_VMASK << CLK_DIV_SHIFT));
			HAL_REG_32BIT(CCU_AON + APB_CLK_REG) |= (apb_clk_reg_backup & (CLK_SEL_2BIT_VMASK << CLK_SEL_SHIFT));
			HAL_REG_32BIT(CCU_AON + APB_SPC_CLK_REG) = (apb_spc_clk_reg_backup & (CLK_DIV_VMASK << CLK_DIV_SHIFT));
			HAL_REG_32BIT(CCU_AON + APB_SPC_CLK_REG) |= (apb_spc_clk_reg_backup & (CLK_SEL_2BIT_VMASK << CLK_SEL_SHIFT));
			HAL_REG_32BIT(CCU_AON + RTC_APB_CLK_REG) = (rtc_apb_clk_reg_backup & (CLK_DIV_VMASK << CLK_DIV_SHIFT));
			HAL_REG_32BIT(CCU_AON + RTC_APB_CLK_REG) |= (rtc_apb_clk_reg_backup & (CLK_SEL_2BIT_VMASK << CLK_SEL_SHIFT));
			if (!(pll_status_backup & (1 << PLL_IDX_VIDEO))) {
				HAL_CCU_ClosePll(PLL_IDX_VIDEO);
			}
		} else if (head->standby_mode == PM_STANDBY_MODE_ULTRA) {
			if (HAL_CCU_GetHoscFreq() == HOSC_CLOCK_40M) {
				HAL_CCU_ConfigPll(&pll_param, PLL_N_192, PLL_D_5);
				HAL_CCU_OpenPll(PLL_IDX_PERI, &pll_param);
				HAL_CCU_PeriPllOutput(1);
				HAL_CCU_ConfigPll(&pll_param, PLL_CPU_CLK_40M_N, PLL_D_2);
				HAL_CCU_OpenPll(PLL_IDX_CPU, &pll_param);
			} else {
				HAL_CCU_ConfigPll(&pll_param, PLL_N_192, PLL_D_3);
				HAL_CCU_OpenPll(PLL_IDX_PERI, &pll_param);
				HAL_CCU_PeriPllOutput(1);
				HAL_CCU_ConfigPll(&pll_param, PLL_CPU_CLK_24M_N, PLL_D_1);
				HAL_CCU_OpenPll(PLL_IDX_CPU, &pll_param);
			}

			HAL_CCU_OpenPll(PLL_IDX_VIDEO, NULL);
			//cpus clk config
			HAL_REG_32BIT(CCU_AON + CPUS_CLK_REG) = CPUS_CLK_DIV_2;
			HAL_REG_32BIT(CCU_AON + CPUS_CLK_REG) |= CPUS_CLK_SRC_PERI_PLL_1024M;
			//ahb clk config
			HAL_REG_32BIT(CCU_AON + AHB_CLK_REG) = AHB_CLK_DIV_4;
			HAL_REG_32BIT(CCU_AON + AHB_CLK_REG) |= AHB_CLK_SRC_PERI_PLL_768M;
			//apb clk config
			HAL_REG_32BIT(CCU_AON + APB_CLK_REG) = APB_CLK_DIV_4;
			HAL_REG_32BIT(CCU_AON + APB_CLK_REG) |= APB_CLK_SRC_PERI_PLL_384M;
			//apb spc clk config
			HAL_REG_32BIT(CCU_AON + APB_SPC_CLK_REG) = APB_SPC_CLK_DIV_1;
			HAL_REG_32BIT(CCU_AON + APB_SPC_CLK_REG) |= APB_SPC_CLK_SRC_PERI_PLL_192M;
			//rtc clk config
			HAL_REG_32BIT(CCU_AON + RTC_APB_CLK_REG) = RTC_APB_CLK_DIV_2;
			HAL_REG_32BIT(CCU_AON + RTC_APB_CLK_REG) |= RTC_APB_CLK_SRC_HOSC;
			HAL_CCU_ClosePll(PLL_IDX_VIDEO);
		}
	}
}


static void suspend_power(standby_head_t *head)
{
	/*
	 * PMC_EN0 controls the DCDC3V3, DCDC0V9, DCDC1V5, only hibernation will close.
	 * PMC_EN1 controls the DCDC3V3_sub.
	 * PMC_EN2 controls the DCDC0V9 voltage adjustment.
	 * PL7 controls the dram power.
	*/

	/* e907 sleep source */
	if (head->chip_is_v821b) {
		HAL_CLR_BITS(PWRCTRL_APP + RES_REQ_CTRL1_REG, PWRSRC_LDO1V8_MASK);
#ifdef CONFIG_PMC_EN1_FOR_PWRCTRL
		HAL_CLR_BITS(PWRCTRL_APP + RES_REQ_CTRL1_REG, PWRSRC_PMC_EN1_MASK);
#endif
#ifdef CONFIG_PMC_EN2_FOR_PWRCTRL
		HAL_CLR_BITS(PWRCTRL_APP + RES_REQ_CTRL1_REG, PWRSRC_PMC_EN2_MASK);
#endif
	} else {
		HAL_CLR_BITS(PWRCTRL_AON + WLAN_SLP_RES, PWRSRC_LDO1V8_MASK);
#ifdef CONFIG_PMC_EN1_FOR_PWRCTRL
		HAL_CLR_BITS(PWRCTRL_AON + WLAN_SLP_RES, PWRSRC_PMC_EN1_MASK);
#endif
#ifdef CONFIG_PMC_EN2_FOR_PWRCTRL
		HAL_CLR_BITS(PWRCTRL_AON + WLAN_SLP_RES, PWRSRC_PMC_EN2_MASK);
#endif
	}

	/* e907 active source */
	HAL_SET_BITS(PWRCTRL_APP + RES_REQ_CTRL0_REG, PWRSRC_LDO1V8_MASK);
#ifdef CONFIG_PMC_EN1_FOR_PWRCTRL
	HAL_SET_BITS(PWRCTRL_APP + RES_REQ_CTRL0_REG, PWRSRC_PMC_EN1_MASK);
#endif
#ifdef CONFIG_PMC_EN2_FOR_PWRCTRL
	HAL_SET_BITS(PWRCTRL_APP + RES_REQ_CTRL0_REG, PWRSRC_PMC_EN2_MASK);
#endif

	/* wlan sleep source */
	if (head->chip_is_v821b) {
		HAL_CLR_BITS(PWRCTRL_AON + WLAN_SLP_RES, PWRSRC_LDO1V8_MASK);
#ifdef CONFIG_PMC_EN1_FOR_PWRCTRL
		HAL_CLR_BITS(PWRCTRL_AON + WLAN_SLP_RES, PWRSRC_PMC_EN1_MASK);
#endif
#ifdef CONFIG_PMC_EN2_FOR_PWRCTRL
		HAL_CLR_BITS(PWRCTRL_AON + WLAN_SLP_RES, PWRSRC_PMC_EN2_MASK);
#endif
	}

	/* wlan active source */
	HAL_CLR_BITS(PWRCTRL_AON + WLAN_ACT_RES, PWRSRC_LDO1V8_MASK);
#ifdef CONFIG_PMC_EN1_FOR_PWRCTRL
	HAL_CLR_BITS(PWRCTRL_AON + WLAN_ACT_RES, PWRSRC_PMC_EN1_MASK);
#endif
#ifdef CONFIG_PMC_EN2_FOR_PWRCTRL
	HAL_SET_BITS(PWRCTRL_AON + WLAN_ACT_RES, PWRSRC_PMC_EN2_MASK);
#endif

	/* set pl0 no pull and low driving*/
	HAL_CLR_BITS(PRCM + PWR_EN_CFG, PWR_EN0_PULL_MASK | PWR_EN0_DRV_MASK);
#ifdef CONFIG_PMC_EN1_FOR_PWRCTRL
	/* set pl1 no pull and low driving*/
	HAL_CLR_BITS(PRCM + PWR_EN_CFG, PWR_EN1_PULL_MASK | PWR_EN1_DRV_MASK);
#endif
#ifdef CONFIG_PMC_EN2_FOR_PWRCTRL
	if (head->pm_mode == PM_MODE_STANDBY) {
		/* set pl2 low driving level and no pull */
		HAL_CLR_BITS(PRCM + PWR_EN_CFG, PWR_EN2_PULL_MASK | PWR_EN2_DRV_MASK);
	} else {
		/* set pl2 normal io, set pull up */
		HAL_CLR_SET_BITS(PRCM + PWR_EN_CFG, PWR_EN2_WAKE_MODE_MASK, PWR_EN2_MODE_WAKEUP_IO_MASK);
		HAL_CLR_SET_BITS(PRCM + PWR_EN_CFG, PWR_EN2_PULL_MASK, PWR_EN2_PULL_UP_MASK);
	}
#endif

	/* backup dcdc and ldo status */
	res_req_ctrl0_reg_backup = HAL_REG_32BIT(PWRCTRL_APP + RES_REQ_CTRL0_REG);

	if (head->pm_mode == PM_MODE_STANDBY) {
		HAL_PWRCTRL_SetSleepMode(PWRCTRL_MODE_STANDBY);
	} else if(head->pm_mode == PM_MODE_HIBERNATION) {
		HAL_PWRCTRL_SetSleepMode(PWRCTRL_MODE_HIBERNATION);
	}
	HAL_PWRCTRL_EnableAnaVddon(1);
	HAL_PWRCTRL_StopAppPwrReq();
}

static void resume_power(standby_head_t *head)
{
	uint32_t val;
	uint8_t ldoa_trim, ldob_trim, ibias;
	uint32_t mark;

	HAL_PWRCTRL_EnableAnaVddon(0);

	adda_init();

	/* restore dcdc and ldo status */
	HAL_REG_32BIT(PWRCTRL_APP + RES_REQ_CTRL0_REG) = res_req_ctrl0_reg_backup;
	while ((HAL_REG_32BIT(PWRCTRL_APP + RES_RDY_REG) & res_req_ctrl0_reg_backup) !=
		   (HAL_REG_32BIT(PWRCTRL_APP + RES_RDY_REG) & res_req_ctrl0_reg_backup));
}

static void resistance_calibration_backup(standby_head_t *head)
{
	if (head->chip_is_v821b) {
		ddr_res_calib_reg_backup = HAL_REG_32BIT(SYSCTRL + EXT_DDR_RES_CTRL_REG) & RES_240_TRIM_MASK;
		usb_res_calib_reg_backup = HAL_REG_32BIT(SYSCTRL + INT_USB_RES_CTRL_REG) & USB_RES_TRIM_MASK;
		csi_res_calib_reg_backup = HAL_REG_32BIT(SYSCTRL + INT_CSI_RES_CTRL_REG) & CSI_RES_TRIM_MASK;
	}
}

static void resistance_calibration_restore(standby_head_t *head)
{
	if (head->chip_is_v821b) {
		HAL_REG_32BIT(SYSCTRL + EXT_DDR_RES_CTRL_REG) = (RES_TRIM_KEY_FIELD << 16) | ddr_res_calib_reg_backup;
		HAL_REG_32BIT(SYSCTRL + INT_USB_RES_CTRL_REG) = (RES_TRIM_KEY_FIELD << 16) | usb_res_calib_reg_backup;
		HAL_REG_32BIT(SYSCTRL + INT_CSI_RES_CTRL_REG) = (RES_TRIM_KEY_FIELD << 16) | csi_res_calib_reg_backup;
	}
}

#ifdef CONFIG_RESUME_SENSOR_IN_SRAM

static void sunxi_gpio_set_cfgbank(struct sunxi_gpio *pio, int bank_offset, uint32_t val)
{
	uint32_t index = GPIO_CFG_INDEX(bank_offset);
	uint32_t offset = GPIO_CFG_OFFSET(bank_offset);

	HAL_CLR_SET_BITS(&pio->cfg[0] + index, 0xf << offset, val << offset);
}

static void sunxi_gpio_set_cfgpin(uint32_t pin, uint32_t val)
{
	uint32_t bank = GPIO_BANK(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	sunxi_gpio_set_cfgbank(pio, pin, val);
}

static void twi_set_clock(struct sunxi_twi_reg *twi, int speed)
{
	int i;
	uint32_t clk_m = 0, clk_n = 0, _2_pow_clk_n = 1, duty = 0, src_clk = 0;
	uint32_t divider, sclk_real;
	/* reset twi control  */
	i  = 0xffff;
	twi->srst = 1;
	while ((twi->srst) && (i))
		i--;

	if ((twi->lcr & 0x30) != 0x30) {
		/* toggle TWI SCL and SDA until bus idle */
		twi->lcr = 0x05;
		HAL_TS_Udelay(500);
		i = 10;
		while ((i > 0) && ((twi->lcr & 0x02) != 2)) {
			/*control scl and sda output high level*/
			twi->lcr |= 0x08;
			twi->lcr |= 0x02;
			HAL_TS_Udelay(1000);
			/*control scl and sda output low level*/
			twi->lcr &= ~0x08;
			twi->lcr &= ~0x02;
			HAL_TS_Udelay(1000);
			i--;
		}
		twi->lcr = 0x0;
		HAL_TS_Udelay(500);
	}

	src_clk = 192000000; /* AW1882 192MHz */
	divider = src_clk / speed; /* 400kHz or 100kHz */
	sclk_real = 0; /* the real clock frequency */

	/* Search for clk_n and clk_m values */
	if (divider == 0) {
		clk_m = 1;
		goto set_clk;
	}

	/* 3 bits max value is 8 */
	while (clk_n < 8) {
		/* (m+1)*2^n = divider --> m = divider/2^n - 1 */
		clk_m = (divider / _2_pow_clk_n) - 1;
		/* 4 bits max value is 16 */
		while (clk_m < 16) {
			/* Calculate real clock frequency */
			sclk_real = src_clk / (clk_m + 1) / _2_pow_clk_n;
			if (sclk_real <= speed) {
				goto set_clk;
			} else {
				clk_m++;
			}
		}
		clk_n++;
		_2_pow_clk_n *= 2; /* Multiple by 2 */
	}

	set_clk:
	twi->clk &= ~((0xf << 3) | (0x7 << 0));
	twi->clk |= ((clk_m << 3) | clk_n);
	if (speed == 400000) {
		duty = (0x1 << 8);
		twi->clk |= duty;
	} else {
		duty = (0x1 << 7);
		twi->clk &= ~(duty);
	}
	twi->ctl |= 0x40;
	twi->eft = 0;
}

static void sunxi_twi_bus_setting(uint32_t twi_base, int onoff)
{
	int reg_value = 0;
	uint32_t bus_num = (twi_base - SUNXI_TWI0_BASE)/0x400;
	unsigned long gate_base, gate_bit;
	unsigned long rst_base, rst_bit;

#define INIT_TWI_REG(bus, n)	\
		if (bus == n) { \
			gate_base = SUNXI_TWI##n##_GATE_BASE; \
			gate_bit  = SUNXI_TWI##n##_GATE_BIT; \
			rst_base  = SUNXI_TWI##n##_RST_BASE; \
			rst_bit   = SUNXI_TWI##n##_RST_BIT; \
		} else

#define INIT_TWI_REG_END()	\
	{ \
		return; \
	}

	if (bus_num < TWI_NR_MASTER) {
#if TWI_NR_MASTER > 0
		INIT_TWI_REG(bus_num, 0)
#endif
#if TWI_NR_MASTER > 1
		INIT_TWI_REG(bus_num, 1)
#endif
#if TWI_NR_MASTER > 2
		INIT_TWI_REG(bus_num, 2)
#endif
#if TWI_NR_MASTER > 3
		INIT_TWI_REG(bus_num, 3)
#endif
#if TWI_NR_MASTER > 4
		INIT_TWI_REG(bus_num, 4)
#endif
#if TWI_NR_MASTER > 5
		INIT_TWI_REG(bus_num, 5)
#endif
#if TWI_NR_MASTER > 6
		INIT_TWI_REG(bus_num, 6)
#endif
		INIT_TWI_REG_END();
	} else {
		//printk("invalid twi bus_num %d, range[0, %d]\r\n", bus_num, TWI_NR_MASTER);
		return;
	}

	if (onoff) {
		//de-assert
		reg_value = HAL_REG_32BIT(rst_base);
		reg_value |= (1 << rst_bit);
		HAL_SET_BITS(rst_base, reg_value);

		//gating clock pass
		reg_value = HAL_REG_32BIT(gate_base);
		reg_value &= ~(1 << gate_bit);
		HAL_SET_BITS(gate_base, reg_value);
		HAL_TS_Udelay(1000);
		reg_value |= (1 << gate_bit);
		HAL_SET_BITS(gate_base, reg_value);
	} else {
		//gating clock mask
		reg_value = HAL_REG_32BIT(gate_base);
		reg_value &= ~(1 << gate_bit);
		HAL_SET_BITS(gate_base, reg_value);

		//assert
		reg_value = HAL_REG_32BIT(rst_base);
		reg_value &= ~(1 << gate_bit);
		HAL_SET_BITS(rst_base, reg_value);
	}
}

static void twi_init(unsigned long twi_base, int speed)
{
	struct sunxi_twi_reg *twi = (struct sunxi_twi_reg *)twi_base;
	sunxi_twi_bus_setting(twi_base, 1);
	twi_set_clock(twi, speed);
}

void twi_exit(unsigned long twi_base)
{
	sunxi_twi_bus_setting(twi_base, 0);
}

static int twi_stop(struct sunxi_twi_reg *twi)
{
	int time = 0xffff;
	uint32_t tmp_val;
	twi->ctl |= (0x01 << 4);
	twi->ctl |= (0x01 << 3);
	while ((time--) && (twi->ctl & 0x10))
		;
	if (time <= 0)
		return -TWI_NOK_TOUT;

	time = 0xffff;
	while ((time--) && (twi->status != TWI_READY))
		;
	tmp_val = twi->status;
	if (tmp_val != TWI_READY)
		return -TWI_NOK_TOUT;

	return TWI_OK;
}

static int twi_senddata(struct sunxi_twi_reg *twi, uint8_t *data_addr, uint32_t data_count)
{
	int time = 0xffff;
	uint32_t i;

	for (i = 0; i < data_count; i++) {
		time      = 0xffff;
		twi->data = data_addr[i];
		twi->ctl |= (0x01 << 3);
		while ((time--) && (!(twi->ctl & 0x08)))
			;
		if (time <= 0) {
			return -TWI_NOK_TOUT;
		}
		time = 0xffff;
		while ((time--) && (twi->status != TWI_DATAWRITE_ACK)) {
			;
		}
		if (time <= 0) {
			return -TWI_NOK_TOUT;
		}
	}

	return TWI_OK;
}

static int twi_sendbyteaddr(struct sunxi_twi_reg *twi, uint32_t byteaddr)
{
	int time = 0xffff;
	uint32_t tmp_val;

	twi->data = byteaddr & 0xff;
	twi->ctl |= (0x01 << 3); /*write 1 to clean int flag*/

	while ((time--) && (!(twi->ctl & 0x08)))
		;
	if (time <= 0)
		return -TWI_NOK_TOUT;

	tmp_val = twi->status;
	if (tmp_val != TWI_DATAWRITE_ACK)
		return -TWI_DATAWRITE_ACK;

	return TWI_OK;
}

static int twi_sendslaveaddr(struct sunxi_twi_reg *twi, uint32_t saddr, uint32_t rw)
{
	int time = 0xffff;
	uint32_t tmp_val;

	rw &= 1;
	twi->data = ((saddr & 0xff) << 1) | rw;
	twi->ctl |= TWI_CTL_INTFLG; /*write 1 to clean int flag*/
	while ((time--) && (!(twi->ctl & TWI_CTL_INTFLG)))
		;
	if (time <= 0)
		return -TWI_NOK_TOUT;

	tmp_val = twi->status;
	if (rw == TWI_WRITE) { /*+write*/
		if (tmp_val != TWI_ADDRWRITE_ACK)
			return -TWI_ADDRWRITE_ACK;
	} else { /*+read*/
		if (tmp_val != TWI_ADDRREAD_ACK)
			return -TWI_ADDRREAD_ACK;
	}

	return TWI_OK;
}

static int twi_sendstart(struct sunxi_twi_reg *twi)
{
	int time = 0xffff;
	uint32_t tmp_val;

	twi->eft  = 0;
	twi->srst = 1;
	twi->ctl  |= TWI_CTL_STA;

	while ((time--) && (!(twi->ctl & TWI_CTL_INTFLG)))
		;
	if (time <= 0)
		return -TWI_NOK_TOUT;

	tmp_val = twi->status;
	if (tmp_val != TWI_START_TRANSMIT)
		return -TWI_START_TRANSMIT;

	return TWI_OK;
}

int twi_write(unsigned long twi_base, uint8_t chip, uint32_t addr, int alen, uint8_t *buffer, int len)
{
	int i, ret, ret0, addrlen;
	char *slave_reg;
	struct sunxi_twi_reg *twi = (struct sunxi_twi_reg *)twi_base;

	ret0 = -1;
	ret  = twi_sendstart(twi);
	if (ret)
		goto twi_write_err_occur;

	ret = twi_sendslaveaddr(twi, chip, TWI_WRITE);
	if (ret)
		goto twi_write_err_occur;
	/*send byte address*/

	if (alen >= 3) {
		addrlen = 2;
	} else if (alen <= 1) {
		addrlen = 0;
	} else {
		addrlen = 1;
	}

	slave_reg = (char *)&addr;
	for (i = addrlen; i >= 0; i--) {
		ret = twi_sendbyteaddr(twi, slave_reg[i] & 0xff);
		if (ret)
			goto twi_write_err_occur;
	}

	ret = twi_senddata(twi, buffer, len);
	if (ret)
		goto twi_write_err_occur;
	ret0 = 0;

twi_write_err_occur:
	twi_stop(twi);
#ifdef CONFIG_TWI_DEBUG
	twi_err("%s: ret = %d\n", __func__, ret);
#endif

	return ret0;
}

static int sunxi_twi_init(struct twi_gpio_info *twi_gpio)
{
	sunxi_gpio_set_cfgpin(twi_gpio->pin[0], twi_gpio->pin_func[0]);
	sunxi_gpio_set_cfgpin(twi_gpio->pin[1], twi_gpio->pin_func[0]);
	twi_init(SENSOR_TWI_BASE, SENSOR_TWI_SPEED);
}

static int sunxi_twi_exit(struct twi_gpio_info *twi_gpio)
{
	twi_exit(SENSOR_TWI_BASE);
	sunxi_gpio_set_cfgpin(twi_gpio->pin[0], twi_gpio->pin_func[1]);
	sunxi_gpio_set_cfgpin(twi_gpio->pin[1], twi_gpio->pin_func[1]);
}

int vin_set_mclk(unsigned int on_off, unsigned long freq)
{
	if (on_off) {
		if (freq == 24000000 || freq == 12000000 || freq == 6000000) {

		} else {
			HAL_SET_BITS(0x4a010048, 0xf9004205);
			HAL_CLR_SET_BITS(0x42001028, ~0x0, 0x81000018);
		}
		sunxi_gpio_set_cfgpin(SUNXI_GPA(1), 0x2);
	} else {
		sunxi_gpio_set_cfgpin(SUNXI_GPA(1), 0xf);
	}

	return 0;
}

int sensor_write_array(struct regval_list *regs, int array_size)
{
	int ret = 0, i = 0;
	unsigned long twi_base = SENSOR_TWI_BASE;
	int sensor_twi_addr = SENSOR_TWI_ADDR;

	if (!regs)
		return -1;

	while (i < array_size) {
		if (regs->addr == REG_DLY) {
			HAL_TS_Udelay(regs->data * 1000);
		} else {
			ret = twi_write(twi_base, sensor_twi_addr >> 1,  regs->addr,  2, &(regs->data), 1);
			if (ret < 0) {
				return -1;
			}
		}
		i++;
		regs++;
	}
	return 0;
}

static int resume_sensor(void)
{
	sunxi_twi_init(&twi_gpio);

	vin_set_mclk(1, SENSOR_MCLK);

	sensor_write_array(sensor_standby_off_regs, sizeof(sensor_standby_off_regs) / sizeof(sensor_standby_off_regs[0]));

	sunxi_twi_exit(&twi_gpio);
}
#endif

static uint32_t get_hardware_sleeptime(void)
{
	uint32_t alarm_time = 0xFFFFFFFF;
	uint32_t wakeuptimer_time = 0xFFFFFFFF;
	uint32_t wlan_time = 0xFFFFFFFF;
	uint32_t min_time = 0;
	standby_head_t *head = get_standby_head();

	if (HAL_RTCALARM_IsEnable()) {
		alarm_time = MS_TO_32K(HAL_RTCALARM_GetInterval() * 1000);
	}
	if (HAL_WUPTIMER_IsEnable()) {
		wakeuptimer_time = HAL_WUPTIMER_GetInterval();
	}
	if (head->wlan_keepalive_time) {
		wlan_time = MS_TO_32K(head->wlan_keepalive_time * 1000);
	}

	min_time = min(alarm_time, wakeuptimer_time);
	min_time = min(min_time, wlan_time);
	if (min_time == 0xFFFFFFFF) {
		return 0;
	}
	return min_time;
}

static void suspend_rtcwdg(standby_head_t *head)
{
	uint32_t sleep_time;
	uint32_t rtcwdg_period;

	if (!head->wdg_suggest_period) {
		return ;
	}

	HAL_RTCWDG_Feed();
	sleep_time = get_hardware_sleeptime();
	if (!sleep_time) {
		HAL_RTCWDG_Disable();
		return ;
	}

	rtcwdg_period = RTCWDG_TICK_TO_TICK_32K(HAL_RTCWDG_GetPeriod());
	if (sleep_time + MS_TO_32K(RTCWDG_SLEEP_TIME_MARGIN) > rtcwdg_period) {
		rtcwdg_period = sleep_time + MS_TO_32K(RTCWDG_SLEEP_TIME_MARGIN);
		HAL_RTCWDG_Feed();
		HAL_RTCWDG_Disable();
		HAL_RTCWDG_SetPeriod(TICK_32K_TO_RTCWDG_TICK(rtcwdg_period));
		HAL_RTCWDG_Enable();
		HAL_RTCWDG_Feed();
	}
}

static void resume_rtcwdg(standby_head_t *head)
{
	if (!head->wdg_suggest_period) {
		return ;
	}

	HAL_RTCWDG_Feed();
	HAL_RTCWDG_Disable();
	HAL_RTCWDG_SetPeriod(MS_TO_RTCWDG_TICK(head->wdg_suggest_period));
	HAL_RTCWDG_Enable();
	HAL_RTCWDG_Feed();
}

int standby_main(void)
{
	int32_t ret = 0;
	standby_head_t *head = get_standby_head();

	/* clear standby main err */
	head->err = STANDBY_ENTER_ERR_NONE;

	/* suspend ddr */
	ret = suspend_dram(head);
	if (ret) {
		goto suspend_dram_err;
	}

	/* suspend ddr power */
	suspend_dram_power(head);

	/* backup resistance calibration */
	resistance_calibration_backup(head);

	/* suspend clock */
	time_record_start(PM_TIME_RECORD_CLOCK_SUSPEND);
	suspend_clock(head);
	time_record_end(PM_TIME_RECORD_CLOCK_SUSPEND);

	/* suspend power */
	time_record_start(PM_TIME_RECORD_POWER_SUSPEND);
	suspend_power(head);
	time_record_end(PM_TIME_RECORD_POWER_SUSPEND);

	/* disable D cache */
	time_record_start(PM_TIME_RECORD_CACHE_SUSPEND);
	suspend_cache();
	time_record_end(PM_TIME_RECORD_CACHE_SUSPEND);

	/* disable cpus ts */
	HAL_TS_Enable(0);

	/* enable wuptimer at last time */
	if (head->wuptimer_ms) {
		HAL_WUPTIMER_Enable(1);
	}

	/* suspend rtc watchdog */
	suspend_rtcwdg(head);

	/* enable all wakeup mask */
	HAL_WAKEUP_SetWakeupMask(WAKEUP_MASK_ALL);

	/* disable irq to avoid cpus wfi fail */
	HAL_CPUS_CFG_EnableIrq(0);

	if (head->pm_mode == PM_MODE_STANDBY) {
		cpu_suspend();
	} else if (head->pm_mode == PM_MODE_HIBERNATION) {
#ifdef CONFIG_PM_DISABLE_RCCAL_IN_HIBERNATION
		HAL_WAKEUP_RstRccal();
#endif
		hibernation_suspend();
	}

	/* get hardware wakesrc */
	update_hardware_wakesrc();

	/* enable irq */
	HAL_CPUS_CFG_EnableIrq(1);

	/* disable all wakeup mask */
	HAL_WAKEUP_ClrWakeupMask(WAKEUP_MASK_ALL);

	/* resume rtc watchdog */
	resume_rtcwdg(head);

	/* disable wuptimer */
	if (head->wuptimer_ms && ((head->wuptimer_ms & WUPTMR_AOV_MODE_MASK) == 0)) {
		HAL_WUPTIMER_Enable(0);
	}

	/* enable cpus ts */
	HAL_TS_Enable(1);

	/* resume ddr power early */
	resume_dram_power_early(head);

	time_record_start(PM_TIME_RECORD_CACHE_RESUME);
	resume_cache();
	time_record_end(PM_TIME_RECORD_CACHE_RESUME);

	/* resume power */
	time_record_start(PM_TIME_RECORD_POWER_RESUME);
	resume_power(head);
	time_record_end(PM_TIME_RECORD_POWER_RESUME);

	/* resume clock */
	time_record_start(PM_TIME_RECORD_CLOCK_RESUME);
	resume_clock(head);
	time_record_end(PM_TIME_RECORD_CLOCK_RESUME);

#ifdef CONFIG_RESUME_SENSOR_IN_SRAM
	time_record_start(PM_TIME_RECORD_SENSOR_RESUME);
	resume_sensor();
	time_record_end(PM_TIME_RECORD_SENSOR_RESUME);
#endif

	/* restore resistance calibration */
	resistance_calibration_restore(head);

	/* resume ddr power late */
	resume_dram_power_late(head);

	/* resume ddr */
	resume_dram(head);
	time_record_output(head);

suspend_dram_err:
	return ret;
}

