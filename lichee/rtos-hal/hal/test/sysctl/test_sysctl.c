/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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
#include <stdlib.h>
#include <hal_log.h>
#include <hal_cmd.h>
#include <sunxi_hal_sysctl.h>

#ifdef CONFIG_ARCH_SUN20IW2P1
static int cmd_test_sysctl(int argc, char **argv)
{
	uint32_t func, data[4];
	uint32_t value;
    if (argc < 2)
    {
        hal_log_info("Usage: hal_sysctl func [data]\n");
        return -1;
    }

    hal_log_info("Run sysctl hal layer test case\n");

    func = strtol(argv[1], NULL, 0);
    data[0] = strtol(argv[2], NULL, 0);

	switch (func) {
		case 0:
			HAL_SYSCTL_SetSipFlashTestMapMode(data[0]);
			break;
		case 1:
			HAL_SYSCTL_RamUseAsBT(data[0]);
			break;
		case 2:
			HAL_SYSCTL_RamUseAsCSI(data[0]);
			break;
		case 3:
			HAL_SYSCTL_RomSecureSel(data[0]);
			break;
		case 4:
			data[1] = strtol(argv[3], NULL, 0);
			data[2] = strtol(argv[4], NULL, 0);
			data[3] = strtol(argv[5], NULL, 0);
			HAL_SYSCTL_SetPsensorControl(data[0], data[1], data[2],  data[3]);
			break;
		case 5:
			data[1] = strtol(argv[3], NULL, 0);
			data[2] = strtol(argv[4], NULL, 0);
			value = HAL_SYSCTL_GetPsensor(data[0], data[1], data[2]);
			hal_log_info("Get Psensor value = %d\n", value);
			break;
		case 6:
			data[1] = strtol(argv[3], NULL, 0);
			HAL_SYSCTL_SetDbgData(data[0], data[1]);
			value = HAL_SYSCTL_GetDegData(data[0]);
			hal_log_info("HAL_SYSCTL_GetDegData value = %x\n", value);
			break;
		default:
			hal_log_info("default func\n");
			break;
	}

    hal_log_info("hal_sysctl test finish\n");
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_sysctl, hal_sysctl, sysctl_hal_APIs_tests)
#endif

#ifdef CONFIG_ARCH_SUN300IW1
#include "xradio/cmd/cmd_util.h"
static void _sysctrl_bustm_irq(void *arg)
{
	SYSCTRL_BusType bus_type = (SYSCTRL_BusType)arg;

	CMD_DBG("bus_type(%d) is timeout!\n", bus_type);
}

/* bus:0~5，sel:0~1, cnt：0~64，irq:0~1*/
static enum cmd_status cmd_sysctrl_bustm_config_exec(char *cmd)
{
	int cnt;
	uint32_t bus_type, interval_sel, interval_cnt, irq_en;
	SYSCTRL_BusTmParam param;

	cnt = cmd_sscanf(cmd, "bus=%u sel=%u cnt=%u irq=%u", &bus_type, &interval_sel, &interval_cnt, &irq_en);
	if (cnt != 4) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	param.interval_sel = interval_sel;
	param.interval_cnt = interval_cnt;
	HAL_SYSCTRL_BusTimeout_ConfigParam(bus_type, &param);
	if (irq_en) {
		HAL_SYSCTRL_BusTimeout_EnableIRQ(bus_type, _sysctrl_bustm_irq, (void *)(bus_type));
	} else {
		HAL_SYSCTRL_BusTimeout_DisableIRQ(bus_type);
	}
	return CMD_STATUS_OK;
}

/* bus:0~5，en:0~1*/
static enum cmd_status cmd_sysctrl_bustm_enable_exec(char *cmd)
{
	int cnt;
	uint32_t bus_type, enable;

	cnt = cmd_sscanf(cmd, "bus=%u en=%u", &bus_type, &enable);
	if (cnt != 2) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	HAL_SYSCTRL_BusTimeout_Enable(bus_type, enable);

	return CMD_STATUS_OK;
}

/* clk:0~13，div:0~3*/
static enum cmd_status cmd_sysctrl_dbgioclk_config_exec(char *cmd)
{
	int cnt;
	uint32_t clk, div;

	cnt = cmd_sscanf(cmd, "clk=%u div=%u", &clk, &div);
	if (cnt != 2) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	} else if (clk >= SYSCTRL_DBGCLK_INVALID || div >= SYSCTRL_DBGCLK_DIV_INVALID) {
		CMD_ERR("invalid clk/div = %d/%d\n", clk, div);
		return CMD_STATUS_INVALID_ARG;
	}

	HAL_SYSCTRL_DebugIO_ConfigClkOutput(clk, div);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_sysctrl_dbgiosignal_config_exec(char *cmd)
{
	int cnt;
	SYSCTRL_DebugIOSignalMaskParam param;

	cnt = cmd_sscanf(cmd, "ve=%x usb0=%x usb1=%x adie=%x wlan=%x pll_lock=%x spif=%x",
	                 &param.ve_mask, &param.usb0_mask, &param.usb1_mask, &param.adie_mask,
	                 &param.wlan_mask, &param.pll_lock_mask, &param.spif_mask);
	if (cnt != 7) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	HAL_SYSCTRL_DebugIO_ConfigSignalOutput(&param);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_sysctrl_dap_config_exec(char *cmd)
{
	int cnt;
	uint32_t dap;

	cnt = cmd_sscanf(cmd, "dap=%u", &dap);
	if (cnt != 1) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	} else if (dap >= SYSCTRL_DBG_DAPSEL_INVALID) {
		CMD_ERR("invalid dap = %d\n", dap);
		return CMD_STATUS_INVALID_ARG;
	}

	HAL_SYSCTRL_DCU_SetDAP(dap);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_sysctrl_srambootmode_enable_exec(char *cmd)
{
	int cnt;
	uint32_t enable;

	cnt = cmd_sscanf(cmd, "en=%u", &enable);
	if (cnt != 1) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (enable) {
		HAL_SYSCTRL_SramRemap_EnableBootMode(1);
	} else {
		HAL_SYSCTRL_SramRemap_EnableBootMode(0);
	}

	return CMD_STATUS_OK;
}

static const struct cmd_data sysctrl_cmds[] = {
	{ "bustm_config",  cmd_sysctrl_bustm_config_exec,  CMD_DESC("bus=xx sel=xx cnt=xx irq=xx. /*bus:0~5， sel:0~1, cnt：0~64，irq:0~1*/")},
	{ "bustm_enable",  cmd_sysctrl_bustm_enable_exec, CMD_DESC("bus=xx en=xx /*bus:0~5， en:0~1*/")},
	{ "dbgioclk",      cmd_sysctrl_dbgioclk_config_exec, CMD_DESC("clk=xx div=xx /* clk:0~13，div:0~3*/")},
	{ "dbgiosignal",   cmd_sysctrl_dbgiosignal_config_exec, CMD_DESC("ve=xx usb0=xx usb1=xx adie=xx wlan=xx pll_lock=xx spif=")},
	{ "dap",           cmd_sysctrl_dap_config_exec, CMD_DESC("dap=xx")},
	{ "srambootmode",  cmd_sysctrl_srambootmode_enable_exec,  CMD_DESC("en=xx") },
};

static enum cmd_status cmd_test_sysctrl(char *cmd)
{
	return cmd_main_exec(cmd, sysctrl_cmds, cmd_nitems(sysctrl_cmds));
}

static void cmd_sysctrl_exec(int argc, char *argv[])
{
	char *ptr;
	ptr = cmd_conv_from_argv(argc, argv, 1);
	if (argc == 1) {
		cmd_help_exec(sysctrl_cmds, cmd_nitems(sysctrl_cmds), 16);
	} else {
		cmd_test_sysctrl(ptr);
	}
}

FINSH_FUNCTION_EXPORT_CMD(cmd_sysctrl_exec, hal_sysctl, Sysctrl hal APIs tests)
#endif