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
#ifndef _FLASH_READ_H_
#define _FLASH_READ_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPT_INF_EN      (1)
#define GPT_LOG_EN      (0)
#define GPT_ERR_EN      (1)
#if GPT_INF_EN
#define gpt_inf(fmt,...)   printf("[GPT_INF]" fmt,##__VA_ARGS__)
#else
#define GPT_inf(fmt,...)
#endif
#if GPT_LOG_EN
#define gpt_log(fmt,...)   printf("[GPT_LOG]" fmt,##__VA_ARGS__)
#else
#define gpt_log(fmt,...)
#endif
#if GPT_ERR_EN
#define gpt_err(fmt,...)   printf("[GPT_ERR]" fmt,##__VA_ARGS__)
#else
#define gpt_err(fmt,...)
#endif

int flash_init(void);
int flash_deinit(void);
/* return 0 if success */
int flash_read(uint32_t sector, uint32_t sector_num, void *buff);

int spl_flash_init(void);
int spl_flash_deinit(void);
int spl_flash_read(uint32_t sector, uint32_t sector_num, void *buff);

void gpt_set_start_sector(uint32_t sector);
int gpt_init(void);
int gpt_deinit(void);
int get_partition_by_name(char *name, uint32_t *sector, uint32_t *sector_num);

#ifdef __cplusplus
}
#endif

#endif /* _FLASH_READ_H_ */
