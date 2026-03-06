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
#include <hal_osal.h>
#include <hal_cfg.h>
#include <script.h>

script_parser_t *Hal_Cfg_Parser = NULL;

#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
int32_t hal_cfg_init(void)
{
#ifdef CONFIG_SYS_CONFIG_BUILDIN
	extern unsigned char blob_fexconfig_start[];
	Hal_Cfg_Parser = script_parser_init(blob_fexconfig_start);
#elif defined(CONFIG_SYS_CONFIG_PACK)
	Hal_Cfg_Parser = script_parser_init((void *)CONFIG_SYS_CONFIG_PACK_ADDRESS);
#endif
	if(Hal_Cfg_Parser)
		return HAL_OK;
	else
		return HAL_ERROR;
}

int32_t hal_cfg_exit(void)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_exit(Hal_Cfg_Parser);
}

int32_t hal_cfg_get_keyvalue(char *SecName, char *KeyName, int32_t Value[], int32_t Count)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_fetch(Hal_Cfg_Parser, SecName, KeyName, Value, Count);
}

int32_t hal_cfg_get_sec_keycount(char *SecName)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_subkey_count(Hal_Cfg_Parser, SecName);
}

int32_t hal_cfg_get_sec_count(void)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_mainkey_count(Hal_Cfg_Parser);
}

int32_t hal_cfg_get_gpiosec_keycount(char *name)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_mainkey_get_gpio_count(Hal_Cfg_Parser, name);
}

int32_t hal_cfg_get_gpiosec_data(char *GPIOSecName, void *pGPIOCfg, int32_t GPIONum)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_mainkey_get_gpio_cfg(Hal_Cfg_Parser, GPIOSecName, pGPIOCfg, GPIONum);
}

void hal_cfg_show(void)
{
	script_show(Hal_Cfg_Parser);
}
#else
int32_t hal_cfg_init(void)
{
	return HAL_ERROR;
}

int32_t hal_cfg_exit(void)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_keyvalue(char *SecName, char *KeyName, int32_t Value[], int32_t Count)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_sec_keycount(char *SecName)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_sec_count(void)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_gpiosec_keycount(char *name)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_gpiosec_data(char *GPIOSecName, void *pGPIOCfg, int32_t GPIONum)
{
	return HAL_ERROR;
}

void Hal_cfg_show(void)
{
	printf("Not support sys config interface\r\n");
}

#endif
