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

#ifndef _STANDBY_HEAD_H_
#define _STANDBY_HEAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <load_image.h>
#include <dram.h>
#include <heap_backup.h>
#ifdef CONFIG_AMP_SHARE_IRQ
#define MAX_GPIO_BANK 16
typedef struct {
	uint32_t bank_num;
	uint32_t gpio_irqs[MAX_GPIO_BANK];
	uint32_t gpio_banks[MAX_GPIO_BANK];
} share_irq_info;
#endif

typedef struct {
	int(*standby_main)(void);
	void *sram_head_start;
	void *sram_head_end;
	void *sram_code_start;
	void *sram_code_end;
	void *sram_bss_start;
	void *sram_bss_end;
	void *sram_stack_top;
	void *sram_stack_btm;
	void *rtos_data_start;
	void *rtos_data_end;
	void *rtos_bss_start;
	void *rtos_bss_end;
	void *rtos_heap_start;
	void *rtos_heap_end;
	void *rtos_data_saved_start;
	void *rtos_data_saved_end;
	void *rtos_bss_saved_start;
	void *rtos_bss_saved_end;
	void *flash_driver_start;
	void *flash_driver_end;
	void *efuse_start;
	uint32_t efuse_size;
	uint32_t pm_mode;
	uint32_t standby_mode;
	uint32_t wuptimer_ms;
	uint32_t pwr_cfg;
	uint32_t *time_record;
	uint32_t boot_param_sector_start;
	__dram_para_t dram_param;
	struct heap_backup_info heap_info;
	elf_info rtos_info;
	uint8_t chip_is_v821b;
	int8_t err;
	uint32_t wlan_keepalive_time;
	uint32_t wdg_suggest_period;
#ifdef CONFIG_AMP_SHARE_IRQ
	share_irq_info *sirq_info;
#endif
} standby_head_t;

standby_head_t *get_standby_head(void);

#ifdef __cplusplus
}
#endif
#endif /* _STANDBY_HEAD_H_ */

