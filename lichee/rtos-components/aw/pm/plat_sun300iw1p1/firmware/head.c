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
#include <head.h>

extern uint32_t _sram_standby_head_start;
extern uint32_t _sram_standby_head_end;
extern uint32_t _sram_standby_code_start;
extern uint32_t _sram_standby_code_end;
extern uint32_t _sram_standby_bss_start;
extern uint32_t _sram_standby_bss_end;
extern uint32_t _standby_stack_top;
extern uint32_t _standby_stack_bottom;

extern int standby_main(void);

__attribute__((section(".standby_head"))) standby_head_t g_standby_head = {
	.standby_main = standby_main,
	.sram_head_start = &_sram_standby_head_start,
	.sram_head_end = &_sram_standby_head_end,
	.sram_code_start = &_sram_standby_code_start,
	.sram_code_end = &_sram_standby_code_end,
	.sram_bss_start = &_sram_standby_bss_start,
	.sram_bss_end = &_sram_standby_bss_end,
	.sram_stack_top = &_standby_stack_top,
	.sram_stack_btm = &_standby_stack_bottom,
};

standby_head_t *get_standby_head(void)
{
	return &g_standby_head;
}

