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
#include <hal_ts.h>
#include <hal_ccu.h>

#define __ASM_STR(x)   #x
#define CSR_TIME        0xc01
#define csr_read(csr)                       \
    ({                              \
        register unsigned long __v;             \
        __asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)    \
                              : "=r" (__v) :            \
                              : "memory");          \
        __v;                            \
    })

static inline uint32_t get_cycles(void)
{
    return csr_read(CSR_TIME);
}

void HAL_TS_Enable(uint32_t en)
{
	en = en ? 1 : 0;
	HAL_SET_BIT_VAL(HAL_REG_32BIT(CCU_APP + CPUS_TS_CLOCK_REG), CPUS_TS_CLK_EN_SHIFT, CPUS_TS_CLK_EN_VMASK, en);
}

uint32_t HAL_TS_Gettick(void)
{
	return get_cycles();
}

uint32_t HAL_TS_GetUs(void)
{
	return HAL_TS_TickToUs(HAL_TS_Gettick());
}

void HAL_TS_Udelay(uint32_t us)
{
	uint32_t mark = HAL_TS_Gettick();
	uint32_t timeout = HAL_TS_UsToTick(us);

	while (HAL_TS_Gettick() - mark <= timeout);
}

void HAL_TS_Mdelay(uint32_t ms)
{
	uint32_t mark = HAL_TS_Gettick();
	uint32_t timeout = HAL_TS_UsToTick(ms * 1000);

	while (HAL_TS_Gettick() - mark <= timeout);
}

void HAL_TS_Tdelay(uint32_t tick)
{
	uint32_t mark = HAL_TS_Gettick();

	while (HAL_TS_Gettick() - mark < tick);
}

uint32_t HAL_TS_UsToTick(uint32_t us)
{
	return (us * (HAL_CCU_GetHoscFreq() / 1000000));
}

uint32_t HAL_TS_TickToUs(uint32_t tick)
{
	return (tick / (HAL_CCU_GetHoscFreq() / 1000000));
}

