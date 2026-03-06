
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

#ifndef __PWM_SUN55IW6_H__
#define __PWM_SUN55IW6_H__

#include <hal_gpio.h>
/*****************************************************************************
 * define  register offset
 *****************************************************************************/
#define PWM_CHAN_NUM		10
#define PWM_PORT_NUM		4

#define REG_CHN_OFFSET		0x40

/* CPUX PWM0 config */
#define SUNXI_CPUX_PWM0_CLK_TYPE     HAL_SUNXI_CCU
#define SUNXI_CPUX_PWM0_CLK_BUS_ID   0
#define SUNXI_CPUX_PWM0_CLK_ID       CLK_PWM0
#define SUNXI_CPUX_PWM0_RESET_TYPE   HAL_SUNXI_RESET
#define SUNXI_CPUX_PWM0_RESET_ID     RST_BUS_PWM0
#define PWM0_CPUX_BASE	             0x02090000UL
#define SUNXI_CPUX_IRQ_PWM0          MAKE_IRQn(35, 0)

/* CPUX PWM1 config */
#define SUNXI_CPUX_PWM1_CLK_TYPE     HAL_SUNXI_CCU
#define SUNXI_CPUX_PWM1_CLK_ID       CLK_PWM1
#define SUNXI_CPUX_PWM1_CLK_BUS_ID   0
#define SUNXI_CPUX_PWM1_RESET_TYPE   HAL_SUNXI_RESET
#define SUNXI_CPUX_PWM1_RESET_ID     RST_BUS_PWM1
#define PWM1_CPUX_BASE               0x02091000UL
#define SUNXI_CPUX_IRQ_PWM1          MAKE_IRQn(158, 0)

/* CPUX PWM2 config */
#define SUNXI_CPUX_PWM2_CLK_TYPE     HAL_SUNXI_CCU
#define SUNXI_CPUX_PWM2_CLK_ID       CLK_PWM2
#define SUNXI_CPUX_PWM2_CLK_BUS_ID   0
#define SUNXI_CPUX_PWM2_RESET_TYPE   HAL_SUNXI_RESET
#define SUNXI_CPUX_PWM2_RESET_ID     RST_BUS_PWM2
#define PWM2_CPUX_BASE               0x02092000UL
#define SUNXI_CPUX_IRQ_PWM2          MAKE_IRQn(61, 0)

/* CPUS PWM config */
#define SUNXI_CPUS_PWM_CLK_TYPE     HAL_SUNXI_R_CCU
#define SUNXI_CPUS_PWM_CLK_BUS_ID   CLK_R_BUS_PWM
#define SUNXI_CPUS_PWM_CLK_ID       CLK_R_PWM
#define SUNXI_CPUS_PWM_RESET_TYPE   HAL_SUNXI_R_RESET
#define SUNXI_CPUS_PWM_RESET_ID     RST_BUS_R_PWM
#define SUNXI_CPUS_IRQ_PWM          MAKE_IRQn(227, 0)
#define PWM_CPUS_BASE               0x07023000UL

#define CPUX_PWM0_PARAMS \
{	.port = 0, \
	.reg_base = PWM0_CPUX_BASE, .irq_num = SUNXI_CPUX_IRQ_PWM0, .gpio_num = 10, \
	.clk_type = SUNXI_CPUX_PWM0_CLK_TYPE, .bus_clk_id = SUNXI_CPUX_PWM0_CLK_BUS_ID, \
	.clk_id = SUNXI_CPUX_PWM0_CLK_ID, .reset_type = SUNXI_CPUX_PWM0_RESET_TYPE, \
	.reset_id = SUNXI_CPUX_PWM0_RESET_ID, \
}

#define CPUX_PWM1_PARAMS \
{	.port = 1, \
	.reg_base = PWM1_CPUX_BASE, .irq_num = SUNXI_CPUX_IRQ_PWM1, .gpio_num = 10, \
	.clk_type = SUNXI_CPUX_PWM1_CLK_TYPE, .bus_clk_id = SUNXI_CPUX_PWM1_CLK_BUS_ID, \
	.clk_id = SUNXI_CPUX_PWM1_CLK_ID, .reset_type = SUNXI_CPUX_PWM1_RESET_TYPE, \
	.reset_id = SUNXI_CPUX_PWM1_RESET_ID, \
}

#define CPUX_PWM2_PARAMS \
{	.port = 2, \
	.reg_base = PWM2_CPUX_BASE, .irq_num = SUNXI_CPUX_IRQ_PWM2, .gpio_num = 10, \
	.clk_type = SUNXI_CPUX_PWM2_CLK_TYPE, .bus_clk_id = SUNXI_CPUX_PWM2_CLK_BUS_ID, \
	.clk_id = SUNXI_CPUX_PWM2_CLK_ID, .reset_type = SUNXI_CPUX_PWM2_RESET_TYPE, \
	.reset_id = SUNXI_CPUX_PWM2_RESET_ID, \
}


#define CPUS_PWM_PARAMS \
{	.port = 3, \
	.reg_base = PWM_CPUS_BASE, .irq_num = SUNXI_CPUS_IRQ_PWM, .gpio_num = 4, \
	.clk_type = SUNXI_CPUS_PWM_CLK_TYPE, .bus_clk_id = SUNXI_CPUS_PWM_CLK_BUS_ID, \
	.clk_id = SUNXI_CPUS_PWM_CLK_ID, .reset_type = SUNXI_CPUS_PWM_RESET_TYPE, \
	.reset_id = SUNXI_CPUS_PWM_RESET_ID, \
}

/*****************************************************************************
 * define  gpio
 *****************************************************************************/
typedef struct pwm_gpio_t
{
	gpio_pin_t pwm_pin;
	int pwm_function;
	u32 bind_channel;
	u32 dead_time;
} pwm_gpio_t;

__attribute__((__unused__)) static pwm_gpio_t pwm_gpio[PWM_PORT_NUM][PWM_CHAN_NUM] =
{
	{
		/* CPUX0 PWM0 */
		{
			/* pwm0 */
			.pwm_pin = GPIOD(0),
			.pwm_function = 5,
		},
		{
			/* pwm1 */
			.pwm_pin = GPIOD(1),
			.pwm_function = 5,
			.bind_channel = 0,
			.dead_time = 20,
		},
		{
			/* pwm2 */
			.pwm_pin = GPIOD(2),
			.pwm_function = 5,
		},
		{
			/* pwm3 */
			.pwm_pin = GPIOD(3),
			.pwm_function = 5,
		},
		{
			/* pwm4 */
			.pwm_pin = GPIOD(4),
			.pwm_function = 5,
		},
		{
			/* pwm5 */
			.pwm_pin = GPIOD(5),
			.pwm_function = 5,
		},
		{
			/* pwm6 */
			.pwm_pin = GPIOD(6),
			.pwm_function = 5,
		},
		{
			/* pwm7 */
			.pwm_pin = GPIOD(7),
			.pwm_function = 5,
		},
		{
			/* pwm8 */
			.pwm_pin = GPIOD(8),
			.pwm_function = 5,
		},
		{
			/* pwm9 */
			.pwm_pin = GPIOD(9),
			.pwm_function = 5,
		},
	},

	{
		/* CPUX1 PWM1 */
		{
			/* pwm0 */
			.pwm_pin = GPIOD(10),
			.pwm_function = 5,
		},
		{
			/* pwm1 */
			.pwm_pin = GPIOD(11),
			.pwm_function = 5,
		},
		{
			/* pwm2 */
			.pwm_pin = GPIOD(12),
			.pwm_function = 5,
		},
		{
			/* pwm3 */
			.pwm_pin = GPIOD(13),
			.pwm_function = 5,
		},
		{
			/* pwm4 */
			.pwm_pin = GPIOD(14),
			.pwm_function = 5,
		},
			{
			/* pwm5 */
			.pwm_pin = GPIOD(15),
			.pwm_function = 5,
		},
		{
			/* pwm6 */
			.pwm_pin = GPIOD(16),
			.pwm_function = 5,
		},
		{
			/* pwm7 */
			.pwm_pin = GPIOD(17),
			.pwm_function = 5,
		},
		{
			/* pwm8 */
			.pwm_pin = GPIOD(18),
			.pwm_function = 5,
		},
		{
			/* pwm9 */
			.pwm_pin = GPIOD(19),
			.pwm_function = 5,
		},
	},

	{
		/* CPUX PWM2 */
		{
			 /* pwm0 */
		 	.pwm_pin = GPIOJ(10),
		 	.pwm_function = 2,
		},
		{
			/* pwm1 */
			.pwm_pin = GPIOJ(11),
			.pwm_function = 2,
		},
		{
			/* pwm2 */
			.pwm_pin = GPIOJ(12),
			.pwm_function = 2,
		},
		{
			/* pwm3 */
			.pwm_pin = GPIOJ(13),
			.pwm_function = 2,
		},
		{
			/* pwm4 */
			.pwm_pin = GPIOJ(14),
			.pwm_function = 2,
		},
			{
			/* pwm5 */
			.pwm_pin = GPIOJ(15),
			.pwm_function = 2,
		},
		{
			/* pwm6 */
			.pwm_pin = GPIOJ(16),
			.pwm_function = 2,
		},
		{
			/* pwm7 */
			.pwm_pin = GPIOJ(17),
			.pwm_function = 2,
		},
		{
			/* pwm8 */
			.pwm_pin = GPIOJ(18),
			.pwm_function = 2,
		},
		{
			/* pwm9 */
			.pwm_pin = GPIOJ(19),
			.pwm_function = 2,
		},
	},

	{
		/* CPUS PWM */
		{
			/* pwm0 */
			.pwm_pin = GPIOL(10),
			.pwm_function = 2,
		},
		{
			/* pwm1 */
			.pwm_pin = GPIOL(9),
			.pwm_function = 5,
		},
		{
			/* pwm2 */
			.pwm_pin = GPIOL(2),
			.pwm_function = 4,
		},
		{
			/* pwm3 */
			.pwm_pin = GPIOL(3),
			.pwm_function = 4,
		},
	}
};

#endif /* __PWM_SUN55IW6_H__ */
