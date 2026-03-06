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
/*
**********************************************************************************************************************
*
*						           the Embedded Secure Bootloader System
*
*
*						       Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      :
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/

#ifndef __SUNXI_SID_H__
#define __SUNXI_SID_H__
#include <stdint.h>
#define CHIP_VER_A		0x0
#define CHIP_VER_B		0x1
#define CHIP_VER_C		0x2
#define CHIP_VER_D		0x3

#define EFUSE_DBG_EN    0

typedef enum efuse_err
{
	EFUSE_ERR_ARG = -1,
	EFUSE_ERR_KEY_NAME_WRONG = -2,
	EFUSE_ERR_KEY_SIZE_TOO_BIG = -3,
	EFUSE_ERR_PRIVATE = -4,
	EFUSE_ERR_ALREADY_BURNED = -5,
	EFUSE_ERR_READ_FORBID = -6,
	EFUSE_ERR_BURN_TIMING = -7,
	EFUSE_ERR_NO_ACCESS = -8,
	EFUSE_ERR_INVALID_ROTPK = -9,
}efuse_err_e;

/* internal struct */
typedef struct efuse_key_map_new{
	#define SUNXI_KEY_NAME_LEN	64
	char name[SUNXI_KEY_NAME_LEN];	/* key_name */
	int offset;	/* key_addr offset */
	int size;	 /* unit: bit */
	int rd_fbd_offset;	/* key can read or not */
	int burned_flg_offset;	/* key has burned or not */
	int sw_rule;
}efuse_key_map_new_t;


int hal_efuse_write(char *key_name, unsigned char *key_data, size_t key_bit_len);
int hal_efuse_read(char *key_name, unsigned char *key_data, size_t key_bit_len);
int hal_efuse_read_ext(uint32_t start_bit, uint32_t bit_num, uint8_t *data);
int hal_efuse_write_ext(uint32_t start_bit, uint32_t bit_num, uint8_t *data);
int hal_efuse_set_security_mode(void);
int hal_efuse_get_security_mode(void);
int hal_efuse_get_chipid(unsigned char *buffer);
int hal_efuse_get_mac(unsigned char *buffer);
int hal_efuse_get_mac_version(unsigned char *buffer);
int hal_efuse_get_mac1(unsigned char *buffer);
int hal_efuse_get_mac2(unsigned char *buffer);
int hal_efuse_get_mac3(unsigned char *buffer);
int hal_efuse_get_thermal_cdata(unsigned char *buffer);
int hal_efuse_get_chip_ver(void);
int hal_get_module_param_from_sid(uint32_t *dst, uint32_t offset, uint32_t len);

efuse_key_map_new_t *efuse_search_key_by_name(const char *key_name);
#endif    /*  #ifndef __EFUSE_H__  */
