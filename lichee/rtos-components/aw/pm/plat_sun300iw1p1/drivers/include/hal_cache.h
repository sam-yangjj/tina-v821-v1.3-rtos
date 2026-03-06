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

#ifndef _STANDBY_HAL_CACHE_H_
#define _STANDBY_HAL_CACHE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_def.h>

typedef struct sysmap_backup {
    uint32_t addr0;
    uint32_t addr1;
    uint32_t addr2;
    uint32_t addr3;
    uint32_t addr4;
    uint32_t addr5;
    uint32_t addr6;
    uint32_t addr7;
    uint32_t attr_set;
} sysmap_backup_t;

void HAL_CACHE_CleanInvalidAllDcache(void);
void HAL_CACHE_CleanAllDcache(void);
void HAL_CACHE_IvalidAllDcache(void);
void HAL_CACHE_CleanDcache(unsigned long addr, unsigned long size);
void HAL_CACHE_InvalidDcache(unsigned long addr, unsigned long size);
void HAL_CACHE_EnableDcache(void);
void HAL_CACHE_DisableDcache(void);
void HAL_CACHE_EnableIcache(void);
void HAL_SYSMAP_Backup(sysmap_backup_t *cfg);
void HAL_SYSMAP_Restore(sysmap_backup_t *cfg);

#ifdef __cplusplus
}
#endif
#endif /* _STANDBY_HAL_CCU_H_ */

