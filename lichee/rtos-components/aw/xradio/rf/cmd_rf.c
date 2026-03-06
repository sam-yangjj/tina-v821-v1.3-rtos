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

#if ((defined(CONFIG_ARCH_SUN20IW2) && \
     (defined(CONFIG_ARCH_ARM_CORTEX_M33) || defined(CONFIG_ARCH_RISCV_C906))) || \
     defined(CONFIG_ARCH_SUN300IW1))

#include "cmd_util.h"
#include "sdd/sdd.h"
#include <console.h>
#include <io.h>
#include <hal_clk.h>

/* for wlan use */
#if (defined(CONFIG_WLAN_STA) || defined(CONFIG_WLAN_AP) || defined(CONFIG_WLAN_MONITOR))
#include "net/wlan/wlan_ext_req.h"
#include "net/wlan/wlan_defs.h"
#include "../adapter/net_ctrl/net_ctrl.h"

#ifdef CONFIG_ARCH_SUN300IW1
#ifdef CONFIG_MUTIL_NET_STACK
#include "xrlink/command.h"
#endif
#define ENABLE_TEMP_FREQ_CALIB 1 //calibrate frequency offset due to temperature changes
#else
#define ENABLE_TEMP_FREQ_CALIB 0
#endif

char *rate_tab_name[11] = {
	"DSSS         1,2",
	"CCK       5.5,11",
	"BPSK1/2    6,6.5",
	"BPSK3/4        9",
	"QPSK1/2    12,13",
	"QPSK3/4  18,19.5",
	"16QAM1/2   24,26",
	"16QAM3/4   36,39",
	"64QAM2/3   48,52",
	"64QAM3/4 54,58.5",
	"64QAM5/6      65"
};

/*
 * print_type:0-HEX type, 1-ASCII type
 */
enum cmd_status cmd_rf_get_sdd_file_exec(char *cmd)
{
	int ret;
	int print_type;

	if (!strcmp("hex", cmd)) {
		print_type = 0;
	} else if (!strcmp("ascii", cmd)) {
		print_type = 1;
	} else {
		CMD_LOG(1, "Invalid params input!Should be [hex | ascii] !\n");
		return -1;
	}
	if (net_sys_status_get() != NET_SYS_STATUS_START) {
		CMD_ERR("net not start!\n");
		return CMD_STATUS_FAIL;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_SDD_FILE,
	                       (uint32_t)(uintptr_t)(&print_type));
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	return CMD_STATUS_OK;
}

enum cmd_status cmd_rf_get_power_exec(char *cmd)
{
	int ret, i;
	int begin_rate, end_rate;
	wlan_ext_power_level_tab_get_t param;

	if (!strcmp("11b", cmd)) {
		begin_rate = 0;
		end_rate   = 1;
	} else if (!strcmp("11gn", cmd)) {
		begin_rate = 2;
		end_rate   = 9;
	} else if (!strcmp("11n_mcs7", cmd)) {
		begin_rate = 10;
		end_rate   = 10;
	} else if (!strcmp("all", cmd)) {
		begin_rate = 0;
		end_rate   = 10;
	} else {
		CMD_LOG(1, "Invalid params input!Should be "
		        "[11b | 11gn | 11n_mcs7 | all] !\n");
		return CMD_STATUS_INVALID_ARG;
	}
	if (net_sys_status_get() != NET_SYS_STATUS_START) {
		CMD_ERR("net not start!\n");
		return CMD_STATUS_FAIL;
	}

	CMD_LOG(1, "------------ power level tab current ------------\n");
	param.PowerTabType = POWER_LEVEL_TAB_TYPE_CUR;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_POWER_LEVEL_TAB,
	                       (uint32_t)(uintptr_t)(&param));
	for (i = begin_rate; i <= end_rate; i++) {
		CMD_LOG(1, "%s: %d\n", rate_tab_name[i],
		        param.PowerTab[i] * 4 / 10 + 2);
	}
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	return CMD_STATUS_ACKED;
}

enum cmd_status cmd_rf_set_power_exec(char *cmd)
{
	int ret, cnt, i;
	int begin_rate, end_rate;
	int power_b = 0, power_gn = 0, power_mcs7 = 0;
	wlan_ext_power_level_tab_get_t param_get;
	wlan_ext_power_level_tab_set_t param_set;

	if (!strncmp("11b ", cmd, 4)) {
		begin_rate = 0;
		end_rate = 1;
		cnt = cmd_sscanf(cmd + 4, "%d", &power_b);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (!strncmp("11gn ", cmd, 5)) {
		begin_rate = 2;
		end_rate = 9;
		cnt = cmd_sscanf(cmd + 5, "%d", &power_gn);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (!strncmp("11n_mcs7 ", cmd, 9)) {
		begin_rate = 10;
		end_rate = 10;
		cnt = cmd_sscanf(cmd + 9, "%d", &power_mcs7);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (!strncmp("all ", cmd, 4)) {
		begin_rate = 0;
		end_rate = 10;
		cnt = cmd_sscanf(cmd + 4, "%d %d %d",
		                 &power_b, &power_gn, &power_mcs7);
		if (cnt != 3) {
			CMD_ERR("cnt %d\n", cnt);
			return CMD_STATUS_INVALID_ARG;
		}
	} else {
		CMD_LOG(1, "Invalid params input!Should be "
		        "[11b | 11gn | 11n_mcs7 | all] !\n");
		return -1;
	}
	if (net_sys_status_get() != NET_SYS_STATUS_START) {
		CMD_ERR("net not start!\n");
		return CMD_STATUS_FAIL;
	}

	param_get.PowerTabType = POWER_LEVEL_TAB_TYPE_CUR;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_POWER_LEVEL_TAB,
	                       (uint32_t)(uintptr_t)(&param_get));
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	for (i = begin_rate; i <= end_rate; i++) {
		if (i >= 0 && i < 2)
			param_get.PowerTab[i] = (power_b - 2) * 10 / 4;
		else if (i >= 2 && i < 10)
			param_get.PowerTab[i] = (power_gn - 2) * 10 / 4;
		else
			param_get.PowerTab[i] = (power_mcs7 - 2) * 10 / 4;
	}
	cmd_memcpy(param_set.PowerTab, param_get.PowerTab,
	           POWER_LEVEL_TAB_USE_LENGTH * sizeof(uint16_t));
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_POWER_LEVEL_TAB,
	                       (uint32_t)(uintptr_t)(&param_set));
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "Set user power level:\n");
	for (i = 0; i < 11; i++) {
		CMD_LOG(1, "%s\t%d\n", rate_tab_name[i],
		        (int)(param_set.PowerTab[i] * 4 / 10 + 2));
	}

	return CMD_STATUS_ACKED;
}

enum cmd_status cmd_rf_get_sdd_power_exec(char *cmd)
{
	int ret;
	int begin_rate, end_rate;
	uint16_t user_power[13];

	if (!strcmp("11b", cmd)) {
		begin_rate = 0;
		end_rate = 1;
	} else if (!strcmp("11gn", cmd)) {
		begin_rate = 2;
		end_rate = 9;
	} else if (!strcmp("11n_mcs7", cmd)) {
		begin_rate = 10;
		end_rate = 10;
	} else if (!strcmp("all", cmd)) {
		begin_rate = 0;
		end_rate = 10;
	} else {
		CMD_LOG(1, "Invalid params input!Should be "
		        "[11b | 11gn | 11n_mcs7 | all] !\n");
		return -1;
	}
	if (net_sys_status_get() != NET_SYS_STATUS_START) {
		CMD_ERR("net not start!\n");
		return CMD_STATUS_FAIL;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_SDD_POWER,
	                       (uint32_t)(uintptr_t)(user_power));
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "Get sdd user power level:\n");
	for (int i = begin_rate; i <= end_rate; i++) {
		CMD_LOG(1, "%s\t%d\n", rate_tab_name[i],
		        (int)(user_power[i] * 4 / 16 + 2));
	}
	return CMD_STATUS_OK;
}

enum cmd_status cmd_rf_set_sdd_power_exec(char *cmd)
{
	int ret, cnt, i;
	int begin_rate, end_rate;
	uint16_t user_power[13];
	int power_b = 0, power_gn = 0, power_mcs7 = 0;

	if (!strncmp("11b ", cmd, 4)) {
		begin_rate = 0;
		end_rate = 1;
		cnt = cmd_sscanf(cmd + 4, "%d", &power_b);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (!strncmp("11gn ", cmd, 5)) {
		begin_rate = 2;
		end_rate = 9;
		cnt = cmd_sscanf(cmd + 5, "%d", &power_gn);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (!strncmp("11n_mcs7 ", cmd, 9)) {
		begin_rate = 10;
		end_rate = 10;
		cnt = cmd_sscanf(cmd + 9, "%d", &power_mcs7);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (!strncmp("all ", cmd, 4)) {
		begin_rate = 0;
		end_rate = 10;
		cnt = cmd_sscanf(cmd + 4, "%d %d %d",
			&power_b, &power_gn, &power_mcs7);
		if (cnt != 3) {
			CMD_ERR("cnt %d\n", cnt);
			return CMD_STATUS_INVALID_ARG;
		}
	} else {
		CMD_LOG(1, "Invalid params input!Should be "
		        "[11b | 11gn | 11n_mcs7 | all] !\n");
		return -1;
	}
	if (net_sys_status_get() != NET_SYS_STATUS_START) {
		CMD_ERR("net not start!\n");
		return CMD_STATUS_FAIL;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_SDD_POWER,
	                       (uint32_t)(uintptr_t)(user_power));
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	for (i = begin_rate; i <= end_rate; i++) {
		if (i < 2)
			user_power[i] = (power_b - 2) * 16 / 4;
		else if (i >= 2 && i < 10)
			user_power[i] = (power_gn - 2) * 16 / 4;
		else
			user_power[i] = (power_mcs7 - 2) * 16 / 4;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SDD_POWER,
	                       (uint32_t)(uintptr_t)(user_power));
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "Set sdd user power level:\n");
	for (i = 0; i < 11; i++) {
		CMD_LOG(1, "%s\t%d\n", rate_tab_name[i],
		        (int)(user_power[i] * 4 / 16 + 2));
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_rf_set_channel_fec(char *cmd)
{
	int ret, cnt;
	int ch1, ch7, ch13;
	wlan_ext_channel_fec_set_t param;

	cnt = cmd_sscanf(cmd, "%d %d %d", &ch1, &ch7, &ch13);
	if (cnt != 3) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	if (net_sys_status_get() != NET_SYS_STATUS_START) {
		CMD_ERR("net not start!\n");
		return CMD_STATUS_FAIL;
	}

	param.FecChannel1  = ch1  * 2;
	param.FecChannel7  = ch7  * 2;
	param.FecChannel13 = ch13 * 2;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_CHANNEL_FEC,
	                       (uint32_t)(uintptr_t)(&param));
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

#if ENABLE_TEMP_FREQ_CALIB
typedef struct sdd_xtal_cal_params {
	uint8_t id;
	uint8_t len;
	uint16_t reserve;
	uint32_t reserve1;
	s64_t   param1;
	s64_t   param2;
	s64_t   param3;
	s64_t   param4;
} sdd_xtal_cal_params_t;

typedef struct sdd_trim_temp {
	uint8_t id;
	uint8_t len;
	uint16_t reserve;
	uint32_t Temp0;
} sdd_trim_temp_t;

enum cmd_status cmd_rf_set_xtal_cal_params_exec(char *cmd)
{
	int ret, cnt;
	float param[4];
	wlan_ext_xtal_calib_params_set_t param_set;
	/* get param */
	cnt = cmd_sscanf(cmd, "%f %f %f %f", &param[0], &param[1], &param[2], &param[3]);

	/* check param */
	if (cnt != 4) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	param_set.param1_value = (s64_t)(param[0] * 10);
	param_set.param2_value = (s64_t)(param[1] * 10);
	param_set.param3_value = (s64_t)(param[2] * 10);
	param_set.param4_value = (s64_t)(param[3] * 10);
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_XTAL_CAL_PARAMS,
	                       (uint32_t)(&param_set));

	CMD_LOG(1, "set param1=%.1f, param2=%.1f, param3=%.1f, param4=%.1f\n", param[0], param[1], param[2], param[3]);
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

#ifndef CONFIG_ARCH_SUN300IW1
enum cmd_status cmd_rf_set_sdd_xtal_cal_params_exec(char *cmd)
{
	int ret, cnt;
	float param[4];
	sdd_xtal_cal_params_t param_sdd;
	struct sdd sdd;
	/* get param */
	cnt = cmd_sscanf(cmd, "%f %f %f %f", &param[0], &param[1], &param[2], &param[3]);

	/* check param */
	if (cnt != 4) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	param_sdd.param1 = (s64_t)(param[0] * 10);
	param_sdd.param2 = (s64_t)(param[1] * 10);
	param_sdd.param3 = (s64_t)(param[2] * 10);
	param_sdd.param4 = (s64_t)(param[3] * 10);

	ret = sdd_request(&sdd);
	if (ret == 0) {
		CMD_ERR("sdd not found!\n");
		return CMD_STATUS_ACKED;
	}
	if (sdd_set_ie(&sdd, SDD_WLAN_DYNAMIC_SECT_ID, SDD_XTAL_TRIM_CALIB_PARAMS_ELT_ID, (uint8_t *)&(param_sdd.reserve))) {
		CMD_ERR("fail to set ie %d\n", SDD_XTAL_TRIM_CALIB_PARAMS_ELT_ID);
		return CMD_STATUS_ACKED;
	}
	if (sdd_save(&sdd)) {
		CMD_ERR("fail to save sdd to flash\n");
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "set param1=%.1f, param2=%.1f, param3=%.1f, param4=%.1f\n", param[0], param[1], param[2], param[3]);

	return CMD_STATUS_OK;
}

enum cmd_status cmd_rf_get_sdd_xtal_cal_params_exec(char *cmd)
{
	int ret;
	sdd_xtal_cal_params_t param_sdd;
	struct sdd sdd;

	ret = sdd_request(&sdd);
	if (ret > 0) {
		struct sdd_ie *ie = sdd_find_ie(&sdd, SDD_WLAN_DYNAMIC_SECT_ID,
		                               SDD_XTAL_TRIM_CALIB_PARAMS_ELT_ID);
		memcpy(&(param_sdd.reserve), ie->data, ie->length);
		CMD_LOG(1, "length is :%d\n", ie->length);
		sdd_release(&sdd);
	} else {
		CMD_ERR("sdd not found!\n");
		return CMD_STATUS_ACKED;
	}

	CMD_LOG(1, "param1 is :%.1f\n", ((float)param_sdd.param1 / 10));
	CMD_LOG(1, "param2 is :%.1f\n", ((float)param_sdd.param2 / 10));
	CMD_LOG(1, "param3 is :%.1f\n", ((float)param_sdd.param3 / 10));
	CMD_LOG(1, "param4 is :%.1f\n", ((float)param_sdd.param4 / 10));

	return CMD_STATUS_OK;
}
#endif

enum cmd_status cmd_rf_set_trim_temp_exec(char *cmd)
{
	int ret, cnt;
	float Temp0;
	uint32_t Trim0;
	wlan_ext_trim_temp_set_t param_set;
#ifdef CONFIG_ARCH_SUN300IW1
	wlan_ext_freq_offset_cfg_t freq_offset = {0};
#endif

	/* get param */
	cnt = cmd_sscanf(cmd, "%d %f", &Trim0, &Temp0);

	/* check param */
	if (cnt != 2) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (Trim0 > 127) {
		CMD_ERR("invalid Trim0 %d\n", Trim0);
		return CMD_STATUS_INVALID_ARG;
	}

#ifdef CONFIG_ARCH_SUN300IW1
	freq_offset.freq_offset = Trim0;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_FREQ_OFFSET,
	                       (uint32_t)(&freq_offset));
	if (ret) {
		CMD_ERR("%s: set freq trim failed ret:%d\n", __func__, ret);
		return CMD_STATUS_ACKED;
	}
#else
	hal_clk_ccu_aon_set_freq_trim(Trim0);
#endif
	CMD_LOG(1, "freq offset is set to %d!\n", Trim0);

	param_set.trim0_value = Trim0;
	param_set.temp0_value = (uint32_t)(Temp0 * 10);
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_TRIM_TEMP,
	                       (uint32_t)(&param_set));

	CMD_LOG(1, "temp0=%.1f\n", Temp0);
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

#ifndef CONFIG_ARCH_SUN300IW1
enum cmd_status cmd_rf_set_sdd_trim_temp_exec(char *cmd)
{
	int ret, cnt;
	float Temp0;
	uint32_t Trim0;
	uint16_t Trim0_16;
	sdd_trim_temp_t param_sdd;

	struct sdd sdd;
	/* get param */
	cnt = cmd_sscanf(cmd, "%d %f", &Trim0, &Temp0);

	/* check param */
	if (cnt != 2) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	if (Trim0 > 127) {
		CMD_ERR("invalid value %d\n", Trim0);
		return CMD_STATUS_INVALID_ARG;
	}

	param_sdd.Temp0 = (uint32_t)(Temp0 * 10);

	ret = sdd_request(&sdd);
	if (ret == 0) {
		CMD_ERR("sdd not found!\n");
		return CMD_STATUS_ACKED;
	}
	Trim0_16 = (uint16_t)Trim0;
	if (sdd_set_ie(&sdd, SDD_WLAN_DYNAMIC_SECT_ID, SDD_XTAL_TRIM_ELT_ID, (uint8_t *)&Trim0_16)) {
		CMD_ERR("fail to set ie %d\n", SDD_XTAL_TRIM_ELT_ID);
		return CMD_STATUS_ACKED;
	}
	if (sdd_set_ie(&sdd, SDD_WLAN_DYNAMIC_SECT_ID, SDD_XTAL_TRIM_TEMPERATURE_ELT_ID, (uint8_t *)&(param_sdd.reserve))) {
		CMD_ERR("fail to set ie %d\n", SDD_XTAL_TRIM_TEMPERATURE_ELT_ID);
		return CMD_STATUS_ACKED;
	}
	if (sdd_save(&sdd)) {
		CMD_ERR("fail to save sdd to flash\n");
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "trim0=%d, temp0=%.1f\n", Trim0, Temp0);

	return CMD_STATUS_OK;
}

enum cmd_status cmd_rf_get_sdd_trim_temp_exec(char *cmd)
{
	int ret;
	sdd_trim_temp_t param_sdd;
	uint16_t value;
	struct sdd sdd;

	ret = sdd_request(&sdd);
	if (ret > 0) {
		struct sdd_ie *ie = sdd_find_ie(&sdd, SDD_WLAN_DYNAMIC_SECT_ID,
		                               SDD_XTAL_TRIM_TEMPERATURE_ELT_ID);
		memcpy(&(param_sdd.reserve), ie->data, ie->length);
		CMD_LOG(1, "length is :%d\n", ie->length);
		ie = sdd_find_ie(&sdd, SDD_WLAN_DYNAMIC_SECT_ID,
		                SDD_XTAL_TRIM_ELT_ID);
		value = le16dec(ie->data);
		sdd_release(&sdd);
	} else {
		CMD_ERR("sdd not found!\n");
		return CMD_STATUS_ACKED;
	}

	CMD_LOG(1, "trim0=%d, temp0=%.1f\n", value, ((float)param_sdd.Temp0 / 10));

	return CMD_STATUS_OK;
}
#endif

enum cmd_status cmd_rf_get_xtal_cal_exec(char *cmd)
{
	int ret;
	uint32_t cali_trim;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_XTAL_CAL,
	                       (uint32_t)&cali_trim);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	CMD_LOG(1, "new freq trim=%d\n", cali_trim);

	return CMD_STATUS_OK;
}

static XR_OS_Timer_t g_auto_xtal_cal_timer;
static void auto_xtal_cal_timer_callback(void *arg)
{
	int ret;
	uint32_t cali_trim;
#ifdef CONFIG_ARCH_SUN300IW1
	wlan_ext_freq_offset_cfg_t freq_offset = {0};
#endif

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_XTAL_CAL,
	                       (uint32_t)&cali_trim);
	if (ret) {
		CMD_ERR("Get freq trim cali error!\n");
		return;
	}
	CMD_LOG(1, "new freq trim=%d\n", cali_trim);
#ifdef CONFIG_ARCH_SUN300IW1
	freq_offset.freq_offset = cali_trim;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_FREQ_OFFSET,
						   (uint32_t)(&freq_offset));
	if (ret) {
		CMD_ERR("%s: set freq trim failed ret:%d\n", __func__, ret);
		return;
	}
#else
	hal_clk_ccu_aon_set_freq_trim(cali_trim);
#endif
	CMD_LOG(1, "Set freq trim to system\n");
}

enum cmd_status cmd_rf_set_auto_xtal_cal_exec(char *cmd)
{
	int cnt;
	uint32_t period;
	/* get param */
	cnt = cmd_sscanf(cmd, "%d", &period);
	/* check param */
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (period == 0) {
		if (XR_OS_TimerIsValid(&g_auto_xtal_cal_timer)) {
			XR_OS_TimerStop(&g_auto_xtal_cal_timer);
			XR_OS_TimerDelete(&g_auto_xtal_cal_timer);
			CMD_LOG(1, "Stoped XTAL TRIM calib timer!\n");
		}
		return CMD_STATUS_OK;
	}

	if (XR_OS_TimerIsActive(&g_auto_xtal_cal_timer)){
		XR_OS_TimerChangePeriod(&g_auto_xtal_cal_timer, period);
	} else {
		XR_OS_TimerSetInvalid(&g_auto_xtal_cal_timer);
		if (XR_OS_TimerCreate(&g_auto_xtal_cal_timer, XR_OS_TIMER_PERIODIC,
		                   auto_xtal_cal_timer_callback,
		                   NULL, period) != XR_OS_OK) {
			CMD_ERR("timer create failed\n");
			return -1;
		}
		XR_OS_TimerStart(&g_auto_xtal_cal_timer);
	}

	return CMD_STATUS_OK;
}
#endif

#endif /* #if (defined(CONFIG_WLAN_STA) || defined(CONFIG_WLAN_AP) ... */

#ifndef CONFIG_ARCH_SUN300IW1
enum cmd_status cmd_rf_set_sdd_freq_offset_exec(char *cmd)
{
	int ret, cnt;
	uint32_t value;
	uint16_t value16;
	struct sdd sdd;
	/* get param */
	cnt = cmd_sscanf(cmd, "%d", &value);

	/* check param */
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	if (value > 127) {
		CMD_ERR("invalid value %d\n", value);
		return CMD_STATUS_INVALID_ARG;
	}

	value16 = (uint16_t)value;
	ret = sdd_request(&sdd);
	if (ret == 0) {
		CMD_ERR("sdd not found!\n");
		return CMD_STATUS_ACKED;
	}
	if (sdd_set_ie(&sdd, SDD_WLAN_DYNAMIC_SECT_ID, SDD_XTAL_TRIM_ELT_ID, (uint8_t *)&value16)) {
		CMD_ERR("fail to set ie %d\n", SDD_XTAL_TRIM_ELT_ID);
		return CMD_STATUS_ACKED;
	}
	if (sdd_save(&sdd)) {
		CMD_ERR("fail to save sdd to flash\n");
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "sdd freq offset is set to %d!\n", value);

	return CMD_STATUS_OK;
}

static __inline uint16_t rf_convert_le16(const void *pp)
{
	uint8_t const *p = (uint8_t const *)pp;
	return ((p[1] << 8) | p[0]);
}

enum cmd_status cmd_rf_get_sdd_freq_offset_exec(char *cmd)
{
	int ret;
	uint16_t value;
	struct sdd sdd;

	ret = sdd_request(&sdd);
	if (ret > 0) {
		struct sdd_ie *id = sdd_find_ie(&sdd, SDD_WLAN_DYNAMIC_SECT_ID,
		                               SDD_XTAL_TRIM_ELT_ID);
		value = rf_convert_le16(id->data);
		sdd_release(&sdd);
	} else {
		CMD_ERR("sdd not found!\n");
		return CMD_STATUS_ACKED;
	}

	CMD_LOG(1, "sdd freq offset is :%d\n", value);
	return CMD_STATUS_OK;
}
#endif

enum cmd_status cmd_rf_set_freq_offset_exec(char *cmd)
{
	int32_t cnt;
	uint32_t value;
#ifdef CONFIG_ARCH_SUN300IW1
	int ret;
	wlan_ext_freq_offset_cfg_t freq_offset = {0};
#endif

	/* get param */
	cnt = cmd_sscanf(cmd, "%d", &value);

	/* check param */
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	if (value > 127) {
		CMD_ERR("invalid value %d\n", value);
		return CMD_STATUS_INVALID_ARG;
	}

#ifdef CONFIG_ARCH_SUN300IW1
	freq_offset.freq_offset = value;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_FREQ_OFFSET,
						   (uint32_t)(&freq_offset));
	if (ret) {
		CMD_ERR("%s: set freq trim failed ret:%d\n", __func__, ret);
		return CMD_STATUS_FAIL;
	}
#else
	hal_clk_ccu_aon_set_freq_trim(value);
#endif
	CMD_LOG(1, "freq offset is set to %d!\n", value);

	return CMD_STATUS_OK;
}

enum cmd_status cmd_rf_get_freq_offset_exec(char *cmd)
{
#ifdef CONFIG_ARCH_SUN300IW1
	int ret;
	wlan_ext_freq_offset_cfg_t freq_offset = {0};
#ifdef CONFIG_WLAN_SUPPORT_EXPAND_CMD
	char buffer[128];
	uint32_t len;
#endif

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_FREQ_OFFSET,
						   (uint32_t)(&freq_offset));
	if (!ret) {
#ifdef CONFIG_WLAN_SUPPORT_EXPAND_CMD
		len = snprintf(buffer, 128, "freq offset(%d) is :%d\n", freq_offset.dcxo_from_a_die, freq_offset.freq_offset);
		CMD_LOG(1, "%s", buffer);
		xrlink_cmd_expand_rsp_data_set(XR_WIFI_ID_EXPAND_CMD_PRINTF, (void *)buffer, len);
#else
		CMD_LOG(1, "freq offset(%d) is :%d\n", freq_offset.dcxo_from_a_die, freq_offset.freq_offset);
#endif
	} else {
#ifdef CONFIG_WLAN_SUPPORT_EXPAND_CMD
		len = snprintf(buffer, 128, "%s: get freq trim failed ret:%d\n", __func__, ret);
		CMD_ERR("%s", buffer);
		xrlink_cmd_expand_rsp_data_set(XR_WIFI_ID_EXPAND_CMD_PRINTF, (void *)buffer, len);
#else
		CMD_ERR("%s: get freq trim failed ret:%d\n", __func__, ret);
#endif
		return CMD_STATUS_FAIL;
	}
#else
	CMD_LOG(1, "freq offset is :%d\n", hal_clk_ccu_aon_get_freq_trim());
#endif

	return CMD_STATUS_OK;
}

#ifndef CONFIG_ARCH_SUN300IW1
typedef enum
{
    RFSEL_DISABLE = 0,
    RF0 = 1,
    RF1,
    RF0_RF1
} PHY_RFSEL;
// set_sdd_ant_mode wlan [parameter 0/1]
// set_sdd_ant_mode bt [parameter 0/1]   Note: R128 BT/BLE use the same ant
// set_sdd_ant_mode ble [parameter 0/1]
// set_sdd_ant_mode dynamicSw [parameter 0/1]
// set_sdd_ant_mode TxPwrDiversityInd [parameter]
enum cmd_status cmd_rf_set_sdd_ant_mode_exec(char *cmd)
{
	int32_t ret;
	int32_t cnt;
	uint32_t Wlan_ant_cfg, Bt_ant_cfg;
	//uint32_t IsAntDynamicSw, Ble_ant_cfg, TxPwr;
	struct sdd sdd;
	struct sdd_ant_mod_elt_value value;
	struct sdd_ie *id;

	/* Read sdd ies SDD_ANT_MOD_ELT_ID parameters */
	ret = sdd_request(&sdd);

	if (ret == 0) {
		CMD_ERR("sdd not found!\n");
		return CMD_STATUS_ACKED;
	}
	if (ret > 0) {
		id = sdd_find_ie(&sdd, SDD_WLAN_BT_BLE_ANT_SECT_ID, SDD_ANT_MOD_ELT_ID);
		if (id) {
			value.IsAntDynamicSw        = id->data[0];
			value.Wlan_ant_cfg          = id->data[1];
			value.Bt_ant_cfg            = id->data[2];
			value.Ble_ant_cfg           = id->data[3];
			value.TxPwrDiversityInd_cfg = id->data[4];
			value.reserved              = id->data[5];
		} else {
			CMD_ERR("sdd not found!\n");
			sdd_release(&sdd);
			return CMD_STATUS_ACKED;
		}
	}

	/* get param */
	if (!strncmp("wlan", cmd, 4)) {
		cnt = cmd_sscanf(cmd + 4, "%d", &Wlan_ant_cfg);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			sdd_release(&sdd);
			return CMD_STATUS_INVALID_ARG;
		}
		if (Wlan_ant_cfg == 0) {
			value.Wlan_ant_cfg = RF0;
			CMD_LOG(1, "Wlan ant is RF%d!\n", Wlan_ant_cfg);
		} else if (Wlan_ant_cfg == 1) {
			value.Wlan_ant_cfg = RF1;
			CMD_LOG(1, "Wlan ant is RF%d!\n", Wlan_ant_cfg);
		} else {
			CMD_ERR("invalid param number %d! Please select 0 or 1!\n", Wlan_ant_cfg);
			sdd_release(&sdd);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (!strncmp("bt", cmd, 2)) {
		cnt = cmd_sscanf(cmd + 2, "%d", &Bt_ant_cfg);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			sdd_release(&sdd);
			return CMD_STATUS_INVALID_ARG;
		}
		if (Bt_ant_cfg == 0) {
			value.Bt_ant_cfg = RF0;
			CMD_LOG(1, "Bt ant is RF%d!\n", Bt_ant_cfg);
		} else if (Bt_ant_cfg == 1) {
			value.Bt_ant_cfg = RF1;
			CMD_LOG(1, "Bt ant is RF%d!\n", Bt_ant_cfg);
		} else {
			CMD_ERR("invalid param number %d! Please select 0 or 1!\n", Bt_ant_cfg);
			sdd_release(&sdd);
			return CMD_STATUS_INVALID_ARG;
		}
	}
#if 0 // ongoing develop and verify
	else if (!strncmp("ble", cmd, 3)) {
		cnt = cmd_sscanf(cmd + 3, "%d", &Ble_ant_cfg);
		if (Ble_ant_cfg == 0) {
			value.Ble_ant_cfg = RF0;
			CMD_LOG(1, "Ble ant is RF%d!\n", Ble_ant_cfg);
		} else if (Ble_ant_cfg == 1) {
			value.Ble_ant_cfg = RF1;
			CMD_LOG(1, "Ble ant is RF%d!\n", Ble_ant_cfg);
		} else {
			CMD_ERR("invalid param number %d! Please select 0 or 1!\n", Ble_ant_cfg);
			sdd_release(&sdd);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (!strncmp("sw", cmd, 2)) {
		cnt = cmd_sscanf(cmd + 2, "%d", &IsAntDynamicSw);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			sdd_release(&sdd);
			return CMD_STATUS_INVALID_ARG;
		}
		if (IsAntDynamicSw == 0) {
			value.IsAntDynamicSw = FALSE;
			CMD_LOG(1, "IsAntDynamicSw is %d!\n", IsAntDynamicSw);
		} else if (IsAntDynamicSw == 1) {
			value.IsAntDynamicSw = TRUE;
			CMD_LOG(1, "IsAntDynamicSw is %d!\n", IsAntDynamicSw);
		} else {
			CMD_ERR("invalid param number %d! Please select 0 or 1!\n", IsAntDynamicSw);
			sdd_release(&sdd);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (!strncmp("pwr", cmd, 3)) {
		cnt = cmd_sscanf(cmd + 3, "%d", &TxPwr);
		CMD_LOG(1, "cmd:%s!\n", &cmd);
		if (cnt != 1) {
			CMD_ERR("cnt %d\n", cnt);
			sdd_release(&sdd);
			return CMD_STATUS_INVALID_ARG;
		}
		value.TxPwrDiversityInd_cfg = (uint8_t) TxPwr;
	}
#endif // ongoing develop and verify
	else {
		CMD_LOG(1, "invalid cmd format! Correct format: rf set_sdd_ant_mode <item> <parameter>!\n");
		sdd_release(&sdd);
		return CMD_STATUS_INVALID_ARG;
	}

	/* write sdd file */
	if (sdd_set_ie(&sdd, SDD_WLAN_BT_BLE_ANT_SECT_ID, SDD_ANT_MOD_ELT_ID, (uint8_t *)&value)) {
		CMD_ERR("fail to set ie 0x%x\n", SDD_ANT_MOD_ELT_ID);
		sdd_release(&sdd);
		return CMD_STATUS_ACKED;
	}
	if (sdd_save(&sdd)) {
		CMD_ERR("fail to save sdd to flash\n");
		sdd_release(&sdd);
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "Please reboot and re-connect to active cfg!\n");
	sdd_release(&sdd);
	return CMD_STATUS_OK;
}

enum cmd_status cmd_rf_get_sdd_ant_mode_exec(char *cmd)
{
	int ret;
	//int32_t IsAntDynamicSw, TxPwr;
	int32_t Wlan_ant_cfg, Bt_ant_cfg;
	__attribute__((__unused__)) int32_t Ble_ant_cfg;
	struct sdd sdd;
	struct sdd_ant_mod_elt_value value;

	ret = sdd_request(&sdd);
	if (ret > 0) {
		struct sdd_ie *id = sdd_find_ie(&sdd, SDD_WLAN_BT_BLE_ANT_SECT_ID, SDD_ANT_MOD_ELT_ID);
		if (id) {
			value.IsAntDynamicSw        = id->data[0];
			value.Wlan_ant_cfg          = id->data[1];
			value.Bt_ant_cfg            = id->data[2];
			value.Ble_ant_cfg           = id->data[3];
			value.TxPwrDiversityInd_cfg = id->data[4];
			value.reserved              = id->data[5];
			sdd_release(&sdd);
			CMD_LOG(1, "Sdd ie is 0x%x!\n", id->id);
		} else {
			CMD_ERR("sdd not found!\n");
			sdd_release(&sdd);
			return CMD_STATUS_ACKED;
		}
	} else {
		CMD_ERR("sdd request failed!\n");
		return CMD_STATUS_FAIL;
	}
	if (value.Wlan_ant_cfg == RF0) {
		Wlan_ant_cfg = 0;
	} else if (value.Wlan_ant_cfg == RF1) {
		Wlan_ant_cfg = 1;
	} else {
		Wlan_ant_cfg = -1;
	}
	if (value.Bt_ant_cfg == RF0) {
		Bt_ant_cfg = 0;
	} else if (value.Bt_ant_cfg == RF1) {
		Bt_ant_cfg = 1;
	} else {
		Bt_ant_cfg = -1;
	}
	if (value.Ble_ant_cfg == RF0) {
		Ble_ant_cfg = 0;
	} else if (value.Ble_ant_cfg == RF1) {
		Ble_ant_cfg = 1;
	}

//	CMD_LOG(1, "IsAntDynamicSw is    %d!\n", value.IsAntDynamicSw);
	CMD_LOG(1, "Wlan ant is          RF%d!\n", Wlan_ant_cfg);
	CMD_LOG(1, "Bt ant is            RF%d!\n", Bt_ant_cfg);
//	CMD_LOG(1, "Ble ant is           RF%d!\n", Ble_ant_cfg);
//	CMD_LOG(1, "TxPwrDiversityInd is %d!\n", value.TxPwrDiversityInd_cfg);

	return CMD_STATUS_OK;
}
#endif /* #ifndef CONFIG_ARCH_SUN300IW1 */

/*
 * rf commands
 */
static enum cmd_status cmd_rf_help_exec(char *cmd);

static const struct cmd_data g_rf_cmds[] = {
#if (defined(CONFIG_WLAN_STA) || defined(CONFIG_WLAN_AP) || defined(CONFIG_WLAN_MONITOR))
	{ "set_sdd_power",        cmd_rf_set_sdd_power_exec, CMD_DESC("set the transmit power in sdd") },
	{ "get_sdd_power",        cmd_rf_get_sdd_power_exec, CMD_DESC("get the transmit power in sdd") },
	{ "set_power",            cmd_rf_set_power_exec,     CMD_DESC("set the transmit power") },
	{ "get_power",            cmd_rf_get_power_exec,     CMD_DESC("get the transmit power") },
	{ "get_sdd_file",         cmd_rf_get_sdd_file_exec,  CMD_DESC("get the sdd file") },
	{ "set_channel_fec",      cmd_rf_set_channel_fec,    CMD_DESC("set the channel fec") },
#if ENABLE_TEMP_FREQ_CALIB
	{ "set_xtal_cal_params",  cmd_rf_set_xtal_cal_params_exec, CMD_DESC("set the frequency offset calib param") },
#ifndef CONFIG_ARCH_SUN300IW1
	{ "set_sdd_xtal_cal_params",  cmd_rf_set_sdd_xtal_cal_params_exec, CMD_DESC("set the frequency offset calib param in sdd") },
	{ "get_sdd_xtal_cal_params",  cmd_rf_get_sdd_xtal_cal_params_exec, CMD_DESC("get the frequency offset calib param in sdd") },
#endif
	{ "set_trim_temp",        cmd_rf_set_trim_temp_exec, CMD_DESC("set the frequency offset trim temperature") },
#ifndef CONFIG_ARCH_SUN300IW1
	{ "set_sdd_trim_temp",    cmd_rf_set_sdd_trim_temp_exec, CMD_DESC("set the frequency offset trim temp in sdd") },
	{ "get_sdd_trim_temp",    cmd_rf_get_sdd_trim_temp_exec, CMD_DESC("get the frequency offset trim temp in sdd") },
#endif
	{ "get_xtal_cal",         cmd_rf_get_xtal_cal_exec, CMD_DESC("get the frequency offset trim calib value") },
	{ "set_auto_xtal_cal",    cmd_rf_set_auto_xtal_cal_exec, CMD_DESC("auto get the frequency offset trim calib value and set it to system") },
#endif
#endif
#ifndef CONFIG_ARCH_SUN300IW1
	{ "set_sdd_freq_offset",  cmd_rf_set_sdd_freq_offset_exec, CMD_DESC("set the frequency offset in sdd") },
	{ "get_sdd_freq_offset",  cmd_rf_get_sdd_freq_offset_exec, CMD_DESC("get the frequency offset in sdd") },
#endif
	{ "get_freq_offset",      cmd_rf_get_freq_offset_exec,     CMD_DESC("get the frequency offset") },
	{ "set_freq_offset",      cmd_rf_set_freq_offset_exec,     CMD_DESC("set the frequency offset") },
#ifndef CONFIG_ARCH_SUN300IW1
	{ "get_sdd_ant_mode",     cmd_rf_get_sdd_ant_mode_exec,    CMD_DESC("get ant mode in sdd") },
	{ "set_sdd_ant_mode",     cmd_rf_set_sdd_ant_mode_exec,    CMD_DESC("set ant mode in sdd") },
#endif
	{ "help",                 cmd_rf_help_exec,                CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_rf_help_exec(char *cmd)
{
	return cmd_help_exec(g_rf_cmds, cmd_nitems(g_rf_cmds), 24);
}

enum cmd_status cmd_rf_exec(char *cmd)
{
	return cmd_exec(cmd, g_rf_cmds, cmd_nitems(g_rf_cmds));
}

static void rf_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_rf_exec);
}

FINSH_FUNCTION_EXPORT_CMD(rf_exec, rf, rf command);

#endif /* #if (defined(CONFIG_ARCH_SUN20IW2) && ...) */
