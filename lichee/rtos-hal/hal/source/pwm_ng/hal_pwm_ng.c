
/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.


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

#include <hal_log.h>
#include <stdio.h>
#include <stdint.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <sunxi_hal_common.h>
#include <sunxi_hal_pwm_ng.h>
#include <hal_interrupt.h>
#ifdef CONFIG_DRIVER_SYSCONFIG
#include <hal_cfg.h>
#include <script.h>
#endif
#ifdef CONFIG_STANDBY
#include <standby/standby.h>
#endif
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#endif

hal_pwm_t sunxi_global_pwm[PWM_PORT_NUM];

static pwm_cap_cb_t pwm_cap_cb;
static pwm_pulse_cb_t pwm_pulse_cb;
static uint32_t rise = 0, fall = 0;
static uint32_t period = 0, duty = 0;
LIST_HEAD(cap_info_head);
LIST_HEAD(pulse_info_head);
struct sunxi_pwm_params_t g_sunxi_pwm_params[] = {
#ifdef CPUX_PWM0_PARAMS
	CPUX_PWM0_PARAMS,
#endif
#ifdef CPUX_PWM1_PARAMS
	CPUX_PWM1_PARAMS,
#endif
#ifdef CPUX_PWM2_PARAMS
	CPUX_PWM2_PARAMS,
#endif
#ifdef CPUS_PWM_PARAMS
	CPUS_PWM_PARAMS,
#endif
#ifdef MCU_PWM_PARAMS
	MCU_PWM_PARAMS,
#endif
};

#if defined(CONFIG_ARCH_SUN300IW1)
#ifndef PWM_USE_SYSCONFIG
#define PWM_USE_SYSCONFIG
#endif
#endif

#define SET_REG_VAL(reg_val, shift, width, set_val)     ((reg_val & ~((-1UL) >> (32 - width) << shift)) | (set_val << shift))
#define GET_REG_VAL(reg_val, shift, width)     (reg_val & ~((-1UL) >> (32 - width) << shift))
#if !defined(CONFIG_ARCH_SUN20IW2) && !defined(CONFIG_ARCH_SUN300IW1)
#define CLK_SRC1 100000000
#define CLK_SRC0 24000000
#else
#define CLK_SRC1 96000000
#define CLK_SRC0 40000000
#endif /* CONFIG_ARCH_SUN20IW2 */
#define TIME_1_SECOND 1000000000

#define PWM_MAX_CAP_NUM		0xFFFFFFFF

#define pwm_do_div(n,base) ({                   \
		u32 __base = (base);                \
		u32 __rem;                      \
		__rem = ((u64)(n)) % __base;            \
		(n) = ((u64)(n)) / __base;              \
		if (__rem > __base / 2) \
		++(n); \
		__rem;                          \
		})

#define PWM_CAPTURE_RETRYS	20

static uint32_t get_pccr_reg_offset(uint32_t channel)
{
	switch (channel) {
	case 0:
	case 1:
		return PWM_PCCR01;
		break;
	case 2:
	case 3:
		return PWM_PCCR23;
		break;
	case 4:
	case 5:
		return PWM_PCCR45;
		break;
	case 6:
	case 7:
		return PWM_PCCR67;
		break;
	case 8:
	case 9:
		return PWM_PCCR89;
		break;
	case 10:
	case 11:
		return PWM_PCCRab;
		break;
	case 12:
	case 13:
		return PWM_PCCRcd;
		break;
	case 14:
	case 15:
		return PWM_PCCRef;
		break;
	default:
		PWM_ERR("channel is error \n");
		return PWM_PCCR01;
	}
}

static uint32_t get_pdzcr_reg_offset(uint32_t channel)
{
	switch (channel) {
	case 0:
	case 1:
		return PWM_PDZCR01;
		break;
	case 2:
	case 3:
		return PWM_PDZCR23;
		break;
	case 4:
	case 5:
		return PWM_PDZCR45;
		break;
	case 6:
	case 7:
		return PWM_PDZCR67;
		break;
	case 8:
	case 9:
		return PWM_PDZCR89;
		break;
	case 10:
	case 11:
		return PWM_PDZCRab;
		break;
	case 12:
	case 13:
		return PWM_PDZCRcd;
		break;
	case 14:
	case 15:
		return PWM_PDZCRef;
		break;
	default:
		PWM_ERR("channel is error \n");
		return -1;
	}
}

void hal_pwm_enable_controller(hal_pwm_port_t port, uint32_t channel_in)
{
	unsigned long reg_addr;
	uint32_t reg_val;
	struct sunxi_pwm_params_t *para;
	hal_pwm_t *sunxi_pwm;
	sunxi_pwm = &sunxi_global_pwm[port];
	unsigned long flags;

	flags = hal_spin_lock_irqsave(&sunxi_pwm->en_lock);
	para = &g_sunxi_pwm_params[port];
	reg_addr = para->reg_base + PWM_PER;
	reg_val = readl(reg_addr);
	reg_val |= 1 << channel_in;

	writel(reg_val, reg_addr);
	hal_spin_unlock_irqrestore(&sunxi_pwm->en_lock, flags);
}

/************   disable  **************/
void hal_pwm_disable_controller(hal_pwm_port_t port, uint32_t channel_in)
{
	unsigned long reg_val;
	unsigned long reg_addr;
	struct sunxi_pwm_params_t *para;
	hal_pwm_t *sunxi_pwm;
	sunxi_pwm = &sunxi_global_pwm[port];
	unsigned long flags;

	flags = hal_spin_lock_irqsave(&sunxi_pwm->en_lock);
	para = &g_sunxi_pwm_params[port];
	reg_addr = para->reg_base + PWM_PER;
	reg_val = readl(reg_addr);
	reg_val &= ~(1 << channel_in);

	writel(reg_val, reg_addr);
	hal_spin_unlock_irqrestore(&sunxi_pwm->en_lock, flags);
}

/*************** polarity *****************/
void hal_pwm_porality(hal_pwm_port_t port, uint32_t channel_in, bool polarity)
{
	uint32_t reg_val;
	unsigned long reg_addr;
	struct sunxi_pwm_params_t *para;
	uint32_t channel = channel_in;

	para = &g_sunxi_pwm_params[port];
	reg_addr = para->reg_base + PWM_PCR;
	reg_addr += REG_CHN_OFFSET * channel;
	/*set polarity*/
	reg_val = hal_readl(reg_addr);
	reg_val = SET_REG_VAL(reg_val, PWM_ACT_STA_SHIFT, PWM_ACT_STA_WIDTH, polarity);
	hal_writel(reg_val, reg_addr);
}

/******************** set active **************/
void hal_pwm_set_active_cycles(hal_pwm_port_t port, uint32_t channel_in, uint32_t active_cycles)  //64
{
	unsigned int temp;
	unsigned int reg_offset, reg_shift, reg_width;
	struct sunxi_pwm_params_t *para;
	para = &g_sunxi_pwm_params[port];
	uint32_t channel = channel_in;
	/* config active cycles */
	reg_offset = PWM_PPR + REG_CHN_OFFSET * channel;
	reg_shift = PWM_ACT_CYCLES_SHIFT;
	reg_width = PWM_ACT_CYCLES_WIDTH;
	temp = hal_readl(para->reg_base + reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, active_cycles);
	hal_writel(temp, para->reg_base + reg_offset);
}

/* entire cycles */
void hal_pwm_set_period_cycles(hal_pwm_port_t port, uint32_t channel_in, uint32_t period_cycles)
{
	unsigned int temp;
	unsigned int reg_offset, reg_shift, reg_width;
	struct sunxi_pwm_params_t *para;
	para = &g_sunxi_pwm_params[port];
	uint32_t channel = channel_in;
	/* config period cycles */
	reg_offset = PWM_PPR + REG_CHN_OFFSET * channel;
	reg_shift = PWM_PERIOD_CYCLES_SHIFT;
	reg_width = PWM_PERIOD_CYCLES_WIDTH;
	temp = hal_readl(para->reg_base + reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, period_cycles);
	hal_writel(temp, para->reg_base + reg_offset);

}

int hal_pwm_get_active_cycles(hal_pwm_port_t port, uint32_t channel_in)  //64
{
	uint32_t reg_val;
	unsigned long reg_addr;
	struct sunxi_pwm_params_t *para;
	uint32_t channel = channel_in;

	para = &g_sunxi_pwm_params[port];
	reg_addr = para->reg_base + PWM_PPR;
	reg_addr += REG_CHN_OFFSET * channel;

    /*get active*/
    reg_val = hal_readl(reg_addr);
    reg_val = GET_REG_VAL(reg_val, PWM_ACT_CYCLES_SHIFT, PWM_ACT_CYCLES_WIDTH);
    return reg_val;
}

/* entire cycles */
int hal_pwm_get_period_cycles(hal_pwm_port_t port, uint32_t channel_in)
{
	uint32_t reg_val;
	unsigned long reg_addr;
	struct sunxi_pwm_params_t *para;
	uint32_t channel = channel_in;

	para = &g_sunxi_pwm_params[port];
	reg_addr = para->reg_base + PWM_PPR;
	reg_addr += REG_CHN_OFFSET * channel;

    /*get clock BYPASS*/
    reg_val = hal_readl(reg_addr);
	reg_val = GET_REG_VAL(reg_val, PWM_PERIOD_CYCLES_SHIFT, PWM_PERIOD_CYCLES_WIDTH);
    return reg_val;
}

uint32_t hal_pwm_cap_get_pulse_num(hal_pwm_port_t port, uint32_t channel_in)
{
#if defined(CONFIG_ARCH_SUN300IW1)
	return 0;
#endif
	struct sunxi_pwm_params_t *para;
	para = &g_sunxi_pwm_params[port];

    return hal_readl(para->reg_base + PWM_CCCSR + channel_in * REG_CHN_OFFSET);
}

static int hal_pwm_pinctrl_init(hal_pwm_t *sunxi_pwm, int channel)
{
#if defined(CONFIG_DRIVER_SYSCONFIG) && defined(PWM_USE_SYSCONFIG)
	user_gpio_set_t gpio_cfg[16] = {0};
	char pwm_name[16];
	int count, ret;

	sprintf(pwm_name, "pwm%d", channel);

	count = hal_cfg_get_gpiosec_keycount(pwm_name);
	if (!count) {
		PWM_ERR("[pwm%d] not support in sys_config\n", channel);
		return -1;
	}
	hal_cfg_get_gpiosec_data(pwm_name, &gpio_cfg, count);

	sunxi_pwm->pin[channel] = (gpio_cfg->port - 1) * 32 + gpio_cfg->port_num;
	sunxi_pwm->enable_muxsel[channel] = gpio_cfg->mul_sel;
	ret = hal_gpio_pinmux_set_function(sunxi_pwm->pin[channel], sunxi_pwm->enable_muxsel[channel]);
	if (ret) {
		PWM_ERR("[pwm%d] PIN%u set function failed! return %d\n", channel, sunxi_pwm->pin[channel], ret);
		return -1;
	}

	ret = hal_gpio_set_driving_level(sunxi_pwm->pin[channel], gpio_cfg->drv_level);
	if (ret) {
		PWM_ERR("[pwm%d] PIN%u set driving level failed! return %d\n", channel, gpio_cfg->drv_level, ret);
		return -1;
	}

	if (gpio_cfg->pull)
		return hal_gpio_set_pull(sunxi_pwm->pin[channel], gpio_cfg->pull);

	sunxi_pwm->pin_state[channel] = true;
	return 0;
#else
	PWM_INFO("[pwm%d] not support in sys_config\n", channel);
	return -1;
#endif
}

static int hal_pwm_pinctrl_exit(hal_pwm_port_t port, hal_pwm_t *sunxi_pwm, uint32_t channel)
{
	if (sunxi_pwm->pin[channel])
		return hal_gpio_pinmux_set_function(sunxi_pwm->pin[channel], 0);
	else
		return hal_gpio_pinmux_set_function(pwm_gpio[port][channel].pwm_pin, 0);
}

pwm_status_t hal_pwm_init(hal_pwm_port_t port)
{
	int i;
	hal_pwm_t *sunxi_pwm;
	struct sunxi_pwm_params_t *para;
	sunxi_pwm = &sunxi_global_pwm[port];
	para = &g_sunxi_pwm_params[port];
	PWM_INFO("pwm init start\n");

	if(sunxi_pwm->pwm_init) {
		sunxi_pwm->pwm_init++;
		return 0;
	}

	hal_spin_lock_init(&sunxi_pwm->lock);
	hal_spin_lock_init(&sunxi_pwm->en_lock);

	sunxi_pwm->pwm_clk_type = para->clk_type;
	sunxi_pwm->pwm_bus_clk_id = para->bus_clk_id;
	sunxi_pwm->pwm_clk_id = para->clk_id;
	sunxi_pwm->pwm_reset_type = para->reset_type;
	sunxi_pwm->pwm_reset_id = para->reset_id;
	sunxi_pwm->port = port;
	for (i = 0; i < para->gpio_num; i++)
		sunxi_pwm->pin_state[i] = false;
	PWM_INFO("gpio_num=%d\n", para->gpio_num);
	if (!sunxi_pwm->pwm_reset) {
		sunxi_pwm->pwm_reset = hal_reset_control_get(sunxi_pwm->pwm_reset_type, sunxi_pwm->pwm_reset_id);
	}
	hal_reset_control_reset(sunxi_pwm->pwm_reset);
	if (!sunxi_pwm->pwm_bus_clk)
		sunxi_pwm->pwm_bus_clk = hal_clock_get(sunxi_pwm->pwm_clk_type, sunxi_pwm->pwm_bus_clk_id);
	if (hal_clock_enable(sunxi_pwm->pwm_bus_clk))
		goto err;
	if (!sunxi_pwm->pwm_clk) {
		sunxi_pwm->pwm_clk = hal_clock_get(sunxi_pwm->pwm_clk_type, sunxi_pwm->pwm_clk_id);
	}
	if (hal_clock_enable(sunxi_pwm->pwm_clk))
		goto err;

	PWM_INFO("pwm init end ");
	sunxi_pwm->pwm_init++;
	return 0;

err:
	PWM_ERR("pwm init failed\n");
	return -1;
}

pwm_status_t hal_pwm_deinit(hal_pwm_port_t port)
{
	int i;
	struct sunxi_pwm_params_t *para;
	hal_pwm_t *sunxi_pwm;

	sunxi_pwm = &sunxi_global_pwm[port];
	para = &g_sunxi_pwm_params[port];
	if (sunxi_pwm->pwm_init) {
		sunxi_pwm->pwm_init--;
		if (!sunxi_pwm->pwm_init) {
			for (i = 0; i < para->gpio_num; i++) {
				hal_pwm_pinctrl_exit(port, sunxi_pwm, i);
				sunxi_pwm->pin_state[i] = false;
			}
			hal_reset_control_assert(sunxi_pwm->pwm_reset);
			hal_reset_control_put(sunxi_pwm->pwm_reset);
			hal_clock_disable(sunxi_pwm->pwm_bus_clk);
			hal_clock_disable(sunxi_pwm->pwm_clk);
			hal_clock_put(sunxi_pwm->pwm_bus_clk);
			hal_spin_lock_deinit(&sunxi_pwm->en_lock);
			hal_spin_lock_deinit(&sunxi_pwm->lock);
		}
	}
	PWM_INFO("pwm deinit end");
	return 0;
}

pwm_status_t hal_pwm_control_single(hal_pwm_port_t port, int channel, struct pwm_config *config_pwm)
{
	unsigned int temp;
	unsigned long long c = 0;
	unsigned long entire_cycles = 256, active_cycles = 192;
	unsigned int reg_offset, reg_shift, reg_width;
	unsigned int reg_bypass_shift;
	unsigned int reg_clk_src_shift, reg_clk_src_width;
	unsigned int reg_div_m_shift, reg_div_m_width, value;
	int32_t clk_osc24m;
	char pwm_name[16];
	int err;
	hal_pwm_t *sunxi_pwm;
	struct sunxi_pwm_params_t *para;
	uint32_t pre_scal_id = 0, div_m = 0, prescale = 0;
	uint32_t pre_scal[][2] =
	{
		/* reg_val clk_pre_div */
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};

	PWM_INFO("period_ns = %d\n", config_pwm->period_ns);
	PWM_INFO("duty_ns = %d\n", config_pwm->duty_ns);
	PWM_INFO("polarity = %d\n", config_pwm->polarity);
	PWM_INFO("channel = %d\n", channel);
	PWM_INFO("pulse_num = %d\n", config_pwm->pulse_num);

	para = &g_sunxi_pwm_params[port];
	sunxi_pwm = &sunxi_global_pwm[port];
	if ((config_pwm->period_ns < config_pwm->duty_ns) || (!config_pwm->period_ns)) {
		PWM_ERR("paremeter error : period_ns can't greater than duty_ns and period_ns can't be 0");
		return -1;
	}

	/* pwm set port */
	err = hal_pwm_pinctrl_init(sunxi_pwm, channel);
	if (err) {
		err = hal_gpio_pinmux_set_function(pwm_gpio[port][channel].pwm_pin, pwm_gpio[port][channel].pwm_function);
		if (err) {
			PWM_ERR("pinmux set failed\n");
			return -1;
		}
		sunxi_pwm->pin_state[channel] = true;
	}

	/* pwm enable controller */
	hal_pwm_enable_controller(port, channel);

	/* pwm set polarity */
	hal_pwm_porality(port, channel, config_pwm->polarity);

	reg_clk_src_shift = PWM_CLK_SRC_SHIFT;
	reg_clk_src_width = PWM_CLK_SRC_WIDTH;
	reg_offset = get_pccr_reg_offset(channel);
	sprintf(pwm_name, "pwm%d", channel);
#ifdef CONFIG_DRIVER_SYSCONFIG
	err = hal_cfg_get_keyvalue(pwm_name, "clk_osc24m", &clk_osc24m, 1);
	if (err) {
		clk_osc24m = 0;
		PWM_INFO("[pwm%d] clk_osc24m not support in sys_config\n", channel);
	}
#else
	clk_osc24m = 0;
#endif

	if (clk_osc24m) {
		/* if need freq 24M, then direct output 24M clock,set clk_bypass. */
		reg_bypass_shift = channel;

		temp = hal_readl(para->reg_base + PWM_PCGR);
		temp = SET_BITS(reg_bypass_shift, 1, temp, 1); /* clk_gating set */
		temp = SET_BITS(reg_bypass_shift + 16, 1, temp, 1); /* clk_bypass set */
		hal_writel(temp, para->reg_base + PWM_PCGR);

		/* clk_src_reg */
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 0); /* select clock source */
		hal_writel(temp, para->reg_base + reg_offset);

		return 0;
	}

	if (config_pwm->period_ns > 0 && config_pwm->period_ns <= 10) {
		/* if freq lt 100M, then direct output 100M clock,set by pass. */
		c = CLK_SRC1;
		reg_bypass_shift = channel;

		temp = hal_readl(para->reg_base + PWM_PCGR);
		temp = SET_BITS(reg_bypass_shift, 1, temp, 1); /* clk_gating set */
		temp = SET_BITS(reg_bypass_shift + 16, 1, temp, 1); /* clk_bypass set */
		hal_writel(temp, para->reg_base + PWM_PCGR);
		/* clk_src_reg */
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1); /* select clock source */
		hal_writel(temp, para->reg_base + reg_offset);

		return 0;
	}
	else if (config_pwm->period_ns > 10 && config_pwm->period_ns <= 334) {
		/* if freq between 3M~100M, then select 100M as clock */
		c = CLK_SRC1;

		/* clk_src_reg : use APB1 clock */
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1);
		hal_writel(temp, para->reg_base + reg_offset);
	}
	else if (config_pwm->period_ns > 334) {
		/* if freq < 3M, then select 24M clock */
		c = CLK_SRC0;

		/* clk_src_reg : use OSC24M clock */
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 0);
		hal_writel(temp, para->reg_base + reg_offset);
	}

	c = c * config_pwm->period_ns;
	pwm_do_div(c, TIME_1_SECOND);
	entire_cycles = (unsigned long)c;
	for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
		if (entire_cycles <= 65536)
			break;
		for (prescale = 0; prescale < PRESCALE_MAX + 1; prescale++) {
			entire_cycles = ((unsigned long)c / pre_scal[pre_scal_id][1]) / (prescale + 1);
			if (entire_cycles <= 65536) {
				div_m = pre_scal[pre_scal_id][0];
				break;
			}
		}
	}

	c = (unsigned long long)entire_cycles * config_pwm->duty_ns;
	pwm_do_div(c, config_pwm->period_ns);
	active_cycles = c;
	if (entire_cycles == 0)
		entire_cycles++;

	/* config clk div_m */
	reg_div_m_shift = PWM_DIV_M_SHIFT;
	reg_div_m_width = PWM_DIV_M_WIDTH;
	temp = hal_readl(para->reg_base + reg_offset);
	temp = SET_BITS(reg_div_m_shift, reg_div_m_width, temp, div_m);
	hal_writel(temp, para->reg_base + reg_offset);

	/* config gating */
	reg_shift = channel;
	value = hal_readl(para->reg_base + PWM_PCGR);
	value = SET_BITS(reg_shift, 1, value, 1); /* set gating */
	hal_writel(value, para->reg_base + PWM_PCGR);

	/* config prescal */
	reg_offset = PWM_PCR + REG_CHN_OFFSET * channel;
	reg_shift = PWM_PRESCAL_SHIFT;
	reg_width = PWM_PRESCAL_WIDTH;
	temp = hal_readl(para->reg_base + reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, prescale);
	hal_writel(temp, para->reg_base + reg_offset);

	if (config_pwm->pulse_num) {
		/* config pulse mode */
		reg_shift = PWM_PULSE_SHIFT;
		reg_width = PWM_PULSE_WIDTH;
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, 1);
		hal_writel(temp, para->reg_base + reg_offset);

		/* config pulse num */
		reg_shift = PWM_PULSE_NUM_SHIFT;
		reg_width = PWM_PULSE_NUM_WIDTH;
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, (config_pwm->pulse_num - 1));
		hal_writel(temp, para->reg_base + reg_offset);
	}

	/* config active cycles */
	reg_offset = PWM_PPR + REG_CHN_OFFSET * channel;
	reg_shift = PWM_ACT_CYCLES_SHIFT;
	reg_width = PWM_ACT_CYCLES_WIDTH;
	temp = hal_readl(para->reg_base + reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, active_cycles);
	hal_writel(temp, para->reg_base + reg_offset);

	/* config period cycles */
	reg_offset = PWM_PPR + REG_CHN_OFFSET * channel;
	reg_shift = PWM_PERIOD_CYCLES_SHIFT;
	reg_width = PWM_PERIOD_CYCLES_WIDTH;
	temp = hal_readl(para->reg_base + reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, (entire_cycles - 1));
	hal_writel(temp, para->reg_base + reg_offset);

	if (config_pwm->pulse_num) {
		/* enable pulse start */
		reg_offset = PWM_PCR + REG_CHN_OFFSET * channel;
		reg_shift = PWM_PULSE_START_SHIFT;
		reg_width = PWM_PULSE_START_WIDTH;
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, 1);
		hal_writel(temp, para->reg_base + reg_offset);
	}

	PWM_INFO("pwm control single channel end\n");

	return 0;
}

pwm_status_t hal_pwm_control_dual(hal_pwm_port_t port, int channel, struct pwm_config *config_pwm)
{
	unsigned int temp;
	unsigned long long c, clk_temp, clk = 0;
	unsigned long reg_dead, dead_time = 0, entire_cycles = 256, active_cycles = 192;
	unsigned int reg_offset, reg_shift, reg_width;
	unsigned int reg_bypass_shift;
	unsigned int reg_clk_src_shift, reg_clk_src_width;
	unsigned int reg_div_m_shift, reg_div_m_width, value;
	int channels[2] = {0};
	int err, i;
	hal_pwm_t *sunxi_pwm;
	struct sunxi_pwm_params_t *para;
	uint32_t pre_scal_id = 0, div_m = 0, prescale = 0;
	uint32_t pre_scal[][2] =
	{
		/* reg_val clk_pre_div */
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};

	PWM_INFO("period_ns = %d", config_pwm->period_ns);
	PWM_INFO("duty_ns = %d", config_pwm->duty_ns);
	PWM_INFO("polarity = %d", config_pwm->polarity);
	PWM_INFO("channel = %d", channel);
	para = &g_sunxi_pwm_params[port];
	sunxi_pwm = &sunxi_global_pwm[port];
	if ((config_pwm->period_ns < config_pwm->duty_ns) || (!config_pwm->period_ns)) {
		PWM_ERR("paremeter error : period_ns can't greater than duty_ns and period_ns can't be 0\n");
		return -1;
	}

	channels[0] = channel;
	channels[1] = pwm_gpio[port][channel].bind_channel;
	dead_time = pwm_gpio[port][channel].dead_time;

	reg_offset = get_pdzcr_reg_offset(channels[0]);
	reg_shift = PWM_DZ_EN_SHIFT;
	reg_width = PWM_DZ_EN_WIDTH;

	temp = hal_readl(para->reg_base + reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, 1);
	hal_writel(temp, para->reg_base + reg_offset);
	temp = hal_readl(para->reg_base + reg_offset);
	if (config_pwm->duty_ns < dead_time || temp == 0) {
		PWM_ERR("duty time or dead zone error\n");
		return -1;
	}

	for (i = 0; i < PWM_BIND_NUM; i++) {
		/* pwm set port */
		err = hal_pwm_pinctrl_init(sunxi_pwm, channels[i]);
		if (err) {
			err = hal_gpio_pinmux_set_function(pwm_gpio[port][channels[i]].pwm_pin, pwm_gpio[port][channels[i]].pwm_function);
			if (err) {
				PWM_ERR("pinmux set failed\n");
				return -1;
			}
			sunxi_pwm->pin_state[channels[i]] = true;
		}
	}

	reg_clk_src_shift = PWM_CLK_SRC_SHIFT;
	reg_clk_src_width = PWM_CLK_SRC_WIDTH;

	if (config_pwm->period_ns > 0 && config_pwm->period_ns <= 10) {
		/* if freq lt 100M, then direct output 100M clock,set by pass. */
		clk = CLK_SRC1;
		for (i = 0; i < PWM_BIND_NUM; i++) {
			reg_bypass_shift = channels[i];
			reg_offset = get_pccr_reg_offset(channels[i]);

			temp = hal_readl(para->reg_base + PWM_PCGR);
			temp = SET_BITS(reg_bypass_shift, 1, temp, 1); /* clk_gating set */
			temp = SET_BITS(reg_bypass_shift + 16, 1, temp, 1); /* clk_bypass set */
			hal_writel(temp, para->reg_base + PWM_PCGR);
			/* clk_src_reg */
			temp = hal_readl(para->reg_base + reg_offset);
			temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1); /* select clock source */
			hal_writel(temp, para->reg_base + reg_offset);
		}
	}
	else if (config_pwm->period_ns > 10 && config_pwm->period_ns <= 334) {
		/* if freq between 3M~100M, then select 100M as clock */
		clk = CLK_SRC1;

		for (i = 0; i < PWM_BIND_NUM; i++) {
			reg_offset = get_pccr_reg_offset(channels[i]);
			/* clk_src_reg : use APB1 clock */
			temp = hal_readl(para->reg_base + reg_offset);
			temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1);
			hal_writel(temp, para->reg_base + reg_offset);
		}
	}
	else if (config_pwm->period_ns > 334) {
		/* if freq < 3M, then select 24M clock */
		clk = CLK_SRC0;
		for (i = 0; i < PWM_BIND_NUM; i++) {
			reg_offset = get_pccr_reg_offset(channels[i]);
			/* clk_src_reg : use OSC24M clock */
			temp = hal_readl(para->reg_base + reg_offset);
			temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 0);
			hal_writel(temp, para->reg_base + reg_offset);
		}
	}

	c = clk * config_pwm->period_ns;
	pwm_do_div(c, TIME_1_SECOND);
	entire_cycles = (unsigned long)c;

	clk_temp = clk * dead_time;
	pwm_do_div(clk_temp, TIME_1_SECOND);
	reg_dead = (unsigned long)clk_temp;
	for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
		if (entire_cycles <= 65536 && reg_dead <= 255)
			break;
		for (prescale = 0; prescale < PRESCALE_MAX + 1; prescale++) {
			entire_cycles = ((unsigned long)c / pre_scal[pre_scal_id][1]) / (prescale + 1);
			reg_dead = clk_temp;
			pwm_do_div(reg_dead, pre_scal[pre_scal_id][1] * (prescale + 1));
			if (entire_cycles <= 65536 && reg_dead <= 255) {
				div_m = pre_scal[pre_scal_id][0];
				break;
			}
		}
	}

	c = (unsigned long long)entire_cycles * config_pwm->duty_ns;
	pwm_do_div(c, config_pwm->period_ns);
	active_cycles = c;

	if (entire_cycles == 0)
		entire_cycles++;

	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset = get_pccr_reg_offset(channels[i]);
		/* config clk div_m */
		reg_div_m_shift = PWM_DIV_M_SHIFT;
		reg_div_m_width = PWM_DIV_M_WIDTH;
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_div_m_shift, reg_div_m_width, temp, div_m);
		hal_writel(temp, para->reg_base + reg_offset);

		/* config gating */
		reg_shift = channels[i];
		value = hal_readl(para->reg_base + PWM_PCGR);
		value = SET_BITS(reg_shift, 1, value, 1); /* set gating */
		hal_writel(value, para->reg_base + PWM_PCGR);

		/* config prescal */
		reg_offset = PWM_PCR + REG_CHN_OFFSET * channels[i];
		reg_shift = PWM_PRESCAL_SHIFT;
		reg_width = PWM_PRESCAL_WIDTH;
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, prescale);
		hal_writel(temp, para->reg_base + reg_offset);

		if (config_pwm->pulse_num) {
			/* config pulse mode */
			reg_shift = PWM_PULSE_SHIFT;
			reg_width = PWM_PULSE_WIDTH;
			temp = hal_readl(para->reg_base + reg_offset);
			temp = SET_BITS(reg_shift, reg_width, temp, 1);
			hal_writel(temp, para->reg_base + reg_offset);

			/* config pulse num */
			reg_shift = PWM_PULSE_NUM_SHIFT;
			reg_width = PWM_PULSE_NUM_WIDTH;
			temp = hal_readl(para->reg_base + reg_offset);
			temp = SET_BITS(reg_shift, reg_width, temp, (config_pwm->pulse_num - 1));
			hal_writel(temp, para->reg_base + reg_offset);
		}

		/* config active cycles */
		reg_offset = PWM_PPR + REG_CHN_OFFSET * channels[i];
		reg_shift = PWM_ACT_CYCLES_SHIFT;
		reg_width = PWM_ACT_CYCLES_WIDTH;
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, active_cycles);
		hal_writel(temp, para->reg_base + reg_offset);

		/* config period cycles */
		reg_offset = PWM_PPR + REG_CHN_OFFSET * channels[i];
		reg_shift = PWM_PERIOD_CYCLES_SHIFT;
		reg_width = PWM_PERIOD_CYCLES_WIDTH;
		temp = hal_readl(para->reg_base + reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, (entire_cycles - 1));
		hal_writel(temp, para->reg_base + reg_offset);

		if (config_pwm->pulse_num) {
			/* enable pulse start */
			reg_offset = PWM_PCR + REG_CHN_OFFSET * channels[i];
			reg_shift = PWM_PULSE_START_SHIFT;
			reg_width = PWM_PULSE_START_WIDTH;
			temp = hal_readl(para->reg_base + reg_offset);
			temp = SET_BITS(reg_shift, reg_width, temp, 1);
			hal_writel(temp, para->reg_base + reg_offset);
		}
	}

	/* config dead zone, one config for two pwm */
	reg_offset = get_pdzcr_reg_offset(channels[0]);
	reg_shift = PWM_PDZINTV_SHIFT;
	reg_width = PWM_PDZINTV_WIDTH;
	temp = hal_readl(para->reg_base + reg_offset);
	temp = SET_BITS(reg_shift, reg_width, temp, reg_dead);  /* set reg_dead time */
	hal_writel(temp, para->reg_base + reg_offset);

	/* pwm set channels[0] polarity */
	hal_pwm_porality(port, channels[0], config_pwm->polarity);
	/* channels[1]'s polarity opposite to channels[0]'s polarity */
	hal_pwm_porality(port, channels[1], !config_pwm->polarity);

	for (i = 0; i < PWM_BIND_NUM; i++)
		/* pwm enable controller */
		hal_pwm_enable_controller(port, channels[i]);

	PWM_INFO("pwm control dual channel end\n");

	return 0;
}

pwm_status_t hal_pwm_control(hal_pwm_port_t port, int channel, struct pwm_config *config_pwm)
{
	hal_pwm_t *sunxi_pwm;
	sunxi_pwm = &sunxi_global_pwm[port];
	unsigned long flags;

	flags = hal_spin_lock_irqsave(&sunxi_pwm->lock);
	if (!config_pwm->output_mode) {
		PWM_INFO("pwm control enter single\n");
		hal_pwm_control_single(port, channel, config_pwm);
	} else {
		PWM_INFO("pwm control enter dual\n");
		hal_pwm_control_dual(port, channel, config_pwm);
	}
	hal_spin_unlock_irqrestore(&sunxi_pwm->lock, flags);
	return 0;
}

pwm_status_t hal_pwm_release_single(hal_pwm_port_t port, int channel)
{
	unsigned int reg_shift;
	unsigned int value;
	hal_pwm_t *sunxi_pwm;
	struct sunxi_pwm_params_t *para;
	sunxi_pwm = &sunxi_global_pwm[port];
	para = &g_sunxi_pwm_params[port];
	reg_shift = PWM_CLK_SRC_SHIFT;

	/* pwm disable controller */
	hal_pwm_disable_controller(port, channel);

	/* config gating */
	reg_shift = channel;
	value = hal_readl(para->reg_base + PWM_PCGR);
	value = SET_BITS(reg_shift, 1, value, 0); /* set gating */
	hal_writel(value, para->reg_base + PWM_PCGR);

	/* pwm set port */
	hal_pwm_pinctrl_exit(port, sunxi_pwm, channel);
	sunxi_pwm->pin_state[channel] = false;

	return 0;
}

pwm_status_t hal_pwm_release_dual(hal_pwm_port_t port, int channel)
{
	unsigned int reg_offset, reg_shift, reg_width;
	unsigned int value;
	int channels[2] = {0};
	int i;
	hal_pwm_t *sunxi_pwm;
	struct sunxi_pwm_params_t *para;

	sunxi_pwm = &sunxi_global_pwm[port];
	para = &g_sunxi_pwm_params[port];
	channels[0] = channel;
	channels[1] = pwm_gpio[port][channel].bind_channel;

	for (i = 0; i < PWM_BIND_NUM; i++)
		/* pwm disable controller */
		hal_pwm_disable_controller(port, channel);

	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_shift = PWM_CLK_SRC_SHIFT;
		reg_width = PWM_CLK_SRC_WIDTH;
		reg_offset = get_pccr_reg_offset(channels[i]);
		/* config gating */
		reg_shift = channel;
		value = hal_readl(para->reg_base + PWM_PCGR);
		value = SET_BITS(reg_shift, 1, value, 0); /* set gating */
		hal_writel(value, para->reg_base + PWM_PCGR);
	}

	/* release dead zone, one release for two pwm */
	reg_offset = get_pdzcr_reg_offset(channels[0]);
	reg_shift = PWM_DZ_EN_SHIFT;
	reg_width = PWM_DZ_EN_WIDTH;
	value = hal_readl(para->reg_base + reg_offset);
	value = SET_BITS(reg_shift, reg_width, value, 0);
	hal_writel(value, para->reg_base + reg_offset);

	for (i = 0; i < PWM_BIND_NUM; i++) {
		/* pwm set port */
		hal_pwm_pinctrl_exit(port, sunxi_pwm, channels[i]);
		sunxi_pwm->pin_state[channels[i]] = false;
	}

	return 0;
}

pwm_status_t hal_pwm_release(hal_pwm_port_t port, int channel, struct pwm_config *config_pwm)
{
	hal_pwm_t *sunxi_pwm;
	sunxi_pwm = &sunxi_global_pwm[port];
	unsigned long flags;

	flags = hal_spin_lock_irqsave(&sunxi_pwm->lock);
	if (!config_pwm->output_mode)
		hal_pwm_release_single(port, channel);
	else
		hal_pwm_release_dual(port, channel);
	hal_spin_unlock_irqrestore(&sunxi_pwm->lock, flags);

	return 0;
}

/*************** capture *****************/

uint32_t pwm_get_rise_cnt(hal_pwm_port_t port, uint32_t channel_in)
{
	unsigned long base_addr, reg_addr;
	uint32_t reg_val;
	uint32_t channel = channel_in;
	struct sunxi_pwm_params_t *para = &g_sunxi_pwm_params[port];

	base_addr = para->reg_base;
	reg_addr = base_addr + PWM_CRLR + REG_CHN_OFFSET * channel;
	reg_val = hal_readl(reg_addr);

	return reg_val;
}

uint32_t pwm_get_fall_cnt(hal_pwm_port_t port, uint32_t channel_in)
{
	unsigned long base_addr, reg_addr;
	uint32_t reg_val = 0;
	uint32_t channel = channel_in;
	struct sunxi_pwm_params_t *para = &g_sunxi_pwm_params[port];

	base_addr = para->reg_base;
	reg_addr = base_addr + PWM_CFLR + REG_CHN_OFFSET * channel;
	reg_val = hal_readl(reg_addr);
	reg_val &= 0xFFFF;

	return reg_val;
}

void pwm_irq_handler(void *data)
{
	hal_pwm_t *sunxi_pwm = (hal_pwm_t *)data;
	struct sunxi_pwm_params_t *para = &g_sunxi_pwm_params[sunxi_pwm->port];
	uint32_t pisr_val = 0;
	uint32_t cisr_val = 0;
	uint32_t capture_channel = 0;
	unsigned long long pwm_clk;
	unsigned int reg_offset, pwm_div, temp;
	u32 pre_scal[][2] = {
		/* reg_value  clk_pre_div */
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};

	/* PWM IRQ Status, counter reaches Entire Cycle Value, channel bit is set 1 by hardware.
	 * PWM Capture interrupts do not trigger set 1.
	 */
	pisr_val = readl(para->reg_base + PWM_PISR);
	/* PWM Capture IRQ Ststus */
	cisr_val = hal_readl(para->reg_base + PWM_CISR);

	/*
	 * calculate the pwm channel according to the interrupt status bit, avoid using for().
	 * 0 , 1 --> 0/2
	 * 2 , 3 --> 2/2
	 * 4 , 5 --> 4/2
	 * 6 , 7 --> 6/2
	 */
	capture_channel = ((ffs(cisr_val) - 1) & (~(0x01)))/2;

	if (cisr_val & (0x1 << (capture_channel << 1))) {
		rise = pwm_get_rise_cnt(sunxi_pwm->port, capture_channel) + 1;
		hal_writel(0x1 << (capture_channel << 1), para->reg_base + PWM_CISR); /* clean interrupt status */
		cisr_val &= ~(0x1 << (capture_channel << 1)); /* clean cisr_val flag */

		/* clean capture CRLF and enabled fail interrupt */
		hal_writel(0x1E, para->reg_base + PWM_CCR + capture_channel * REG_CHN_OFFSET);
	}

	if (cisr_val & (0x2 << (capture_channel << 1))) {
		fall = pwm_get_fall_cnt(sunxi_pwm->port, capture_channel) + 1;
		hal_writel(0x2 << (capture_channel << 1), para->reg_base + PWM_CISR); /* clean interrupt status */
		cisr_val &= ~(0x2 << (capture_channel << 1)); /* clean cisr_val flag */

		/* clean capture CFLF and disabled fail interrupt */
		hal_writel(0x1E, para->reg_base + PWM_CCR + capture_channel * REG_CHN_OFFSET);
	}

	if (sunxi_pwm->retrys >= PWM_CAPTURE_RETRYS) {
		reg_offset = get_pccr_reg_offset(capture_channel);
		temp = hal_readl(para->reg_base + reg_offset);
		pwm_div = pre_scal[temp & (0xf)][1];
		if (temp & (0x1 << PWM_CLK_SRC_SHIFT))
			pwm_clk = CLK_SRC1;
		else
			pwm_clk = CLK_SRC0;

		/* period = (1s / clk_src * period_cycles). */
		/* period = (rise + fall) * 1000 / 40; */
		period = (rise + fall) * (TIME_1_SECOND / pwm_clk) * pwm_div;
		duty = fall * (TIME_1_SECOND / pwm_clk) * pwm_div;
		PWM_INFO("temp is %d, pwm_div is %d, pwm_clk is %lld, rise is %d, fall is %d, period is %d, duty is %d\n",
			temp, pwm_div, pwm_clk, rise, fall, period, duty);

		if (pwm_cap_cb)
			pwm_cap_cb(capture_channel, period, duty);
		hal_writel(0x1 << capture_channel, para->reg_base + PWM_PISR);/* clean interrupt status */
		pisr_val &= ~(0x1 << capture_channel); /* clean cisr_val flag */
		sunxi_pwm->retrys = 0;

		/* clean capture CRLF and disabled rise interrupt */
		/* hal_writel(0x18, para->reg_base + PWM_CCR + capture_channel * REG_CHN_OFFSET); */

		hal_writel(0x1 << (capture_channel << 1), para->reg_base + PWM_CISR); /* clean interrupt status */
		cisr_val &= ~(0x1 << (capture_channel << 1)); /* clean cisr_val flag */
	}
	sunxi_pwm->retrys++;

	/* clean other status */
	hal_writel(cisr_val, para->reg_base + PWM_CISR); /* clean interrupt status */
	hal_writel(pisr_val, para->reg_base + PWM_PISR); /* clean interrupt status */

}

void pwm_cap_count_irq_handler(void *data)
{
	hal_pwm_t *sunxi_pwm = (hal_pwm_t *)data;
	struct sunxi_pwm_params_t *para = &g_sunxi_pwm_params[sunxi_pwm->port];
	uint32_t pisr_val = 0, cisr_val = 0, cccisr_val = 0;

	/* PWM IRQ Status, counter reaches Entire Cycle Value, channel bit is set 1 by hardware.
	 * PWM Capture interrupts do not trigger set 1.
	 */
	pisr_val = readl(para->reg_base + PWM_PISR);
	/* PWM Capture IRQ Ststus */
	cisr_val = hal_readl(para->reg_base + PWM_CISR);
	/* PWM Capture Counter IRQ Status */
	cccisr_val = hal_readl(para->reg_base + PWM_CCCISR);

	/* clean interrupt status */
	hal_writel(cccisr_val, para->reg_base + PWM_CCCISR);
	hal_writel(cisr_val, para->reg_base + PWM_CISR);
	hal_writel(pisr_val, para->reg_base + PWM_PISR);
}

void pwm_enable_capture(hal_pwm_port_t port, uint32_t channel_in, pwm_cap_cb_t cb)
{
	unsigned long base_addr, reg_addr;
	uint32_t reg_val, reg_shift;
	uint32_t irq;
	hal_pwm_t *sunxi_pwm;
	struct sunxi_pwm_params_t *para;
	void (*irq_handler)(void *data);
	int ret;
	const char *name = "PWM";

	sunxi_pwm = &sunxi_global_pwm[port];
	para = &g_sunxi_pwm_params[port];
	sunxi_pwm->retrys = 0;

	/* config gating */
	reg_shift = channel_in;
	reg_val = hal_readl(para->reg_base + PWM_PCGR);
	reg_val = SET_BITS(reg_shift, 1, reg_val, 1); /* set gating */
	hal_writel(reg_val, para->reg_base + PWM_PCGR);

	base_addr = para->reg_base;
	irq = para->irq_num;
	irq_handler = pwm_irq_handler;

	/*enable capture*/
	reg_addr = base_addr + PWM_CER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_BITS(channel_in, 0x1, reg_val, 0x1);
	hal_writel(reg_val, reg_addr);

	/* enable rise&fail interrupt */
	reg_addr = base_addr + PWM_CIER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_BITS(channel_in * 0x2, 0x2, reg_val, 0x3);
	hal_writel(reg_val, reg_addr);

	/* enabled rising edge trigger */
	reg_addr = base_addr + PWM_CCR + channel_in * REG_CHN_OFFSET;
	reg_val = PWM_CAPTURE_CRTE | PWM_CAPTURE_CRLF;
	hal_writel(reg_val, reg_addr);

	pwm_cap_cb = cb;

	ret = hal_request_irq(irq, (hal_irq_handler_t)irq_handler, name, sunxi_pwm);
	if (ret < 0) {
		PWM_ERR("request err: %d.\n", ret);
		return;
	}

	ret = hal_enable_irq(irq);

}

void pwm_enable_capture_count(hal_pwm_port_t port, uint32_t channel_in)
{
	unsigned long base_addr, reg_addr;
	uint32_t reg_val, reg_shift;
	uint32_t irq;
	hal_pwm_t *sunxi_pwm;
	struct sunxi_pwm_params_t *para;
	void (*irq_handler)(void *data);
	int ret;
	const char *name = "PWM";

	sunxi_pwm = &sunxi_global_pwm[port];
	para = &g_sunxi_pwm_params[port];
	sunxi_pwm->retrys = 0;

	/* config gating */
	reg_shift = channel_in;
	reg_val = hal_readl(para->reg_base + PWM_PCGR);
	reg_val = SET_BITS(reg_shift, 1, reg_val, 1); /* set gating */
	hal_writel(reg_val, para->reg_base + PWM_PCGR);

	base_addr = para->reg_base;
	irq = para->irq_num;
	irq_handler = pwm_cap_count_irq_handler;

	/* starting capture coutner when CAP_CNT*_EN=1 */
	reg_addr = base_addr + PWM_CCR + channel_in * REG_CHN_OFFSET;
	reg_val = hal_readl(reg_addr);
	reg_val = PWM_CAPTURE_CRTE | PWM_CAPTURE_CRLF;//use single, rasing edge trigger
	hal_writel(reg_val, reg_addr);

	/* raise edge capture and restart counting */
	reg_addr = base_addr + PWM_CCCR + channel_in * REG_CHN_OFFSET;
	reg_val = hal_readl(reg_addr);
	reg_val |= PWM_CAPTURE_CCCSRR;//enable restart counting and use raise edge to count
	hal_writel(reg_val, reg_addr);

	/* set cccnr */
	reg_addr = base_addr + PWM_CCCNR + channel_in * REG_CHN_OFFSET;
	hal_writel(PWM_MAX_CAP_NUM, reg_addr);

	/* enable capture counter inetrrupt */
	reg_addr = base_addr + PWM_CCCIER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_BITS(channel_in, 0x1, reg_val, 0x1);
	hal_writel(reg_val, reg_addr);

	/* use capture counter single */
	/*enable capture counter*/
	reg_addr = base_addr + PWM_CCCER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_BITS(channel_in, 0x1, reg_val, 0x1);
	hal_writel(reg_val, reg_addr);

	ret = hal_request_irq(irq, (hal_irq_handler_t)irq_handler, name, sunxi_pwm);
	if (ret < 0) {
		PWM_ERR("request err: %d.\n", ret);
		return;
	}

	ret = hal_enable_irq(irq);
}

void pwm_disable_capture(hal_pwm_port_t port, uint32_t channel_in)
{
	unsigned long base_addr, reg_addr;
	uint32_t reg_val;
	uint32_t irq;
	struct sunxi_pwm_params_t *para;

	para = &g_sunxi_pwm_params[port];
	base_addr = para->reg_base;

	/* disable pwm capture irq */
	reg_addr = base_addr + PWM_PIER;
	reg_val = hal_readl(reg_addr);
	reg_val = reg_val & ~(0x1 << channel_in);
	hal_writel(reg_val, reg_addr);

	reg_addr = base_addr + PWM_CER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_REG_VAL(reg_val, channel_in, 1, 0);
	hal_writel(reg_val, reg_addr);

	reg_addr = base_addr + PWM_CIER;
	reg_val = SET_REG_VAL(reg_val, channel_in * 2, 2, 0x0);
	hal_writel(reg_val, reg_addr);

	reg_addr = base_addr + PWM_CCR + channel_in * REG_CHN_OFFSET;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_REG_VAL(reg_val, 1, 2, 0x0);
	hal_writel(reg_val, reg_addr);

#if defined(CONFIG_ARCH_SUN55IW6)
	reg_addr = base_addr + PWM_CCCIER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_REG_VAL(reg_val, channel_in, 1, 0);
	hal_writel(reg_val, reg_addr);

	reg_addr = base_addr + PWM_CCCER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_REG_VAL(reg_val, channel_in, 1, 0);
	hal_writel(reg_val, reg_addr);
#endif

	irq = para->irq_num;
	hal_disable_irq(irq);
	hal_free_irq(irq);
}

void pwm_disable_capture_count(hal_pwm_port_t port, uint32_t channel_in)
{
	unsigned long base_addr, reg_addr;
	uint32_t reg_val;
	uint32_t irq;
	struct sunxi_pwm_params_t *para;

	para = &g_sunxi_pwm_params[port];
	base_addr = para->reg_base;

	reg_addr = base_addr + PWM_CCR + channel_in * REG_CHN_OFFSET;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_REG_VAL(reg_val, 1, 2, 0x0);
	hal_writel(reg_val, reg_addr);

	reg_addr = base_addr + PWM_CCCIER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_REG_VAL(reg_val, channel_in, 1, 0);
	hal_writel(reg_val, reg_addr);

	reg_addr = base_addr + PWM_CCCER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_REG_VAL(reg_val, channel_in, 1, 0);
	hal_writel(reg_val, reg_addr);

	irq = para->irq_num;
	hal_disable_irq(irq);
	hal_free_irq(irq);
}

void hal_pwm_cap_get_info(uint32_t channel, hal_pwm_cap_info *info)
{
	hal_pwm_cap_info *i = NULL;

	if (!info)
		return;

	list_for_each_entry(i, &cap_info_head, node) {
		if (i->channel == channel)
			break;
	}

	if (!i || (i->channel != channel)) {
		PWM_ERR("channel%d has not enable.\n", channel);
		return;
	}

	info->channel = i->channel;
	info->cnt = i->cnt;
	info->period = i->period;
	info->duty = i->duty;
	info->reserved = i->reserved;
}

void hal_pwm_cap_callback(uint32_t channel, uint32_t period, uint32_t duty)
{
	hal_pwm_cap_info *info = NULL;

	list_for_each_entry(info, &cap_info_head, node) {
		if (info->channel == channel)
			break;
	}

	if (!info || (info->channel != channel))
		return;

	info->cnt++;
	info->period = period;
	info->duty = duty;

	info->callback(info);
}

int hal_pwm_cap_enable(hal_pwm_port_t port, uint32_t channel, pwm_callback callback)
{
	hal_pwm_cap_info *info;
	int err = 0;
	hal_pwm_t *sunxi_pwm;

	sunxi_pwm = &sunxi_global_pwm[port];
	list_for_each_entry(info, &cap_info_head, node) {
		if (info->channel == channel) {
			/* printf("channel%d has already enable.\n", channel); */
			return HAL_PWM_STATUS_ERROR_PARAMETER;
		}
	}

	info = malloc(sizeof(hal_pwm_cap_info));
	memset(info, 0, sizeof(hal_pwm_cap_info));
	info->channel = channel;
	info->callback = callback;
	list_add_tail(&info->node, &cap_info_head);

	/*pwm_gpio_set_function(cfg, channel);*/
	hal_pwm_init(port);
	err = hal_pwm_pinctrl_init(sunxi_pwm, channel);
	if (err) {
		err = hal_gpio_pinmux_set_function(pwm_gpio[port][channel].pwm_pin, pwm_gpio[port][channel].pwm_function);
		if (err) {
			PWM_ERR("pinmux set failed\n");
			return -1;
		}
		sunxi_pwm->pin_state[channel] = true;
	}

	pwm_enable_capture(port, channel, hal_pwm_cap_callback);
	return HAL_PWM_STATUS_OK;
}

int hal_pwm_cap_count_enable(hal_pwm_port_t port, uint32_t channel)
{
#if defined(CONFIG_ARCH_SUN300IW1)
	return HAL_PWM_STATUS_ERROR;
#endif
	hal_pwm_cap_info *info;
	int err = 0;
	hal_pwm_t *sunxi_pwm;

	sunxi_pwm = &sunxi_global_pwm[port];
	list_for_each_entry(info, &cap_info_head, node) {
		if (info->channel == channel) {
			/* printf("channel%d has already enable.\n", channel); */
			return HAL_PWM_STATUS_ERROR_PARAMETER;
		}
	}

	info = malloc(sizeof(hal_pwm_cap_info));
	memset(info, 0, sizeof(hal_pwm_cap_info));
	info->channel = channel;
	list_add_tail(&info->node, &cap_info_head);

	/*pwm_gpio_set_function(cfg, channel);*/
	hal_pwm_init(port);
	err = hal_pwm_pinctrl_init(sunxi_pwm, channel);
	if (err) {
		err = hal_gpio_pinmux_set_function(pwm_gpio[port][channel].pwm_pin, pwm_gpio[port][channel].pwm_function);
		if (err) {
			PWM_ERR("pinmux set failed\n");
			return -1;
		}
		sunxi_pwm->pin_state[channel] = true;
	}

	pwm_enable_capture_count(port, channel);

	return HAL_PWM_STATUS_OK;
}

int hal_pwm_cap_disable(hal_pwm_port_t port, uint32_t channel)
{
	hal_pwm_cap_info *info;

	pwm_disable_capture(port, channel);

	list_for_each_entry(info, &cap_info_head, node) {
		if (info->channel == channel)
			break;
	}
	if (!info || (info->channel != channel))
		return HAL_PWM_STATUS_ERROR_PARAMETER;
	list_del(&info->node);
	free(info);

	return 0;
}

int hal_pwm_cap_count_disable(hal_pwm_port_t port, uint32_t channel)
{
#if defined(CONFIG_ARCH_SUN300IW1)
	return HAL_PWM_STATUS_ERROR;
#endif
	hal_pwm_cap_info *info;

	pwm_disable_capture_count(port, channel);

	list_for_each_entry(info, &cap_info_head, node) {
		if (info->channel == channel)
			break;
	}
	if (!info || (info->channel != channel))
		return HAL_PWM_STATUS_ERROR_PARAMETER;
	list_del(&info->node);
	free(info);

	return 0;
}

void hal_pwm_pulse_callback(uint32_t channel)
{
	hal_pwm_pulse_info *info = NULL;

	list_for_each_entry(info, &pulse_info_head, node) {
		if (info->channel == channel)
			break;
	}

	if (!info || (info->channel != channel))
		return;

	info->callback(info->data);
}

void pwm_pulse_irq_handler(void *data)
{
	unsigned long base_addr, reg_addr;
	uint32_t channel = 0;
	uint32_t reg_val;
	struct sunxi_pwm_params_t *para = (struct sunxi_pwm_params_t *)data;

	base_addr = para->reg_base;
	reg_addr= base_addr + PWM_PISR;
	reg_val = hal_readl(reg_addr);

	for (int i = 0; i < para->gpio_num; i++) {
		if (reg_val & (1 << i)) {
			channel = i;
			break;
		}
	}

	if (pwm_pulse_cb)
		pwm_pulse_cb(channel);

	hal_writel( 0x1 << channel, reg_addr);
}

void hal_pwm_pulse_irq_enable(hal_pwm_port_t port, uint32_t channel_in, pwm_callback callback, void *data)
{
	hal_pwm_pulse_info *info;
	unsigned long base_addr, reg_addr;
	uint32_t reg_val;
	uint32_t irq;
	void (*irq_handler)(void *data);
	int ret;
	const char *name = "PWM_PULSE";
	struct sunxi_pwm_params_t *para;

	para = &g_sunxi_pwm_params[port];
	base_addr = para->reg_base;

	list_for_each_entry(info, &pulse_info_head, node) {
		if (info->channel == channel_in)
			return;
	}

	info = malloc(sizeof(hal_pwm_pulse_info));
	memset(info, 0, sizeof(hal_pwm_pulse_info));
	info->channel = channel_in;
	info->callback = callback;
	info->data = data;
	info->port = port;
	list_add_tail(&info->node, &pulse_info_head);

	base_addr = para->reg_base;
	irq = para->irq_num;
	irq_handler = pwm_pulse_irq_handler;

	/* clear the pending bit */
	reg_addr = base_addr + PWM_PISR;
	reg_val = hal_readl(base_addr + reg_addr);
	hal_writel(0x1 << channel_in, reg_addr);

	/* enable pwm pulse interrupt */
	reg_addr = base_addr + PWM_PIER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_BITS(channel_in, 0x1, reg_val, 0x1);
	hal_writel(reg_val, reg_addr);

	if (!pwm_pulse_cb)
		pwm_pulse_cb = hal_pwm_pulse_callback;

	ret = hal_request_irq(irq, (hal_irq_handler_t)irq_handler, name, (void *)para);
	if (ret < 0) {
		printf("request err: %d.\n", ret);
		return;
	}
	ret = hal_enable_irq(irq);
}

int hal_pwm_pulse_irq_disable(hal_pwm_port_t port, uint32_t channel_in)
{
	unsigned long base_addr, reg_addr;
	uint32_t reg_val;
	struct sunxi_pwm_params_t *para;
	hal_pwm_pulse_info *info;

	para = &g_sunxi_pwm_params[port];
	base_addr = para->reg_base;

	/* clear the pending bit */
	reg_addr = base_addr + PWM_PISR;
	reg_val = hal_readl(base_addr + reg_addr);
	hal_writel(0x1 << channel_in, reg_addr);

	/* disable pwm pulse interrupt */
	reg_addr = base_addr + PWM_PIER;
	reg_val = hal_readl(reg_addr);
	reg_val = SET_BITS(channel_in, 0x1, reg_val, 0);
	hal_writel(reg_val, reg_addr);

	list_for_each_entry(info, &pulse_info_head, node) {
		if (info->channel == channel_in)
			break;
	}
	if (!info || (info->channel != channel_in))
		return HAL_PWM_STATUS_ERROR_PARAMETER;

	list_del(&info->node);
	free(info);

	return 0;
}
