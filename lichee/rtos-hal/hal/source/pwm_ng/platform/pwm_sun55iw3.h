
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

#ifndef __PWM_SUN55IW3_H__
#define __PWM_SUN55IW3_H__

#include <hal_gpio.h>
/*****************************************************************************
 * define  register offset
 *****************************************************************************/
#define  REG_CHN_OFFSET     0x20

#define  PWM0_CPUX_BASE	    0x02000c00UL
#define  PWM1_CPUX_BASE     0x02051000UL
#define  PWM_CPUS_BASE      0x07020c00UL
#define  PWM_MCU_BASE       0x07103000UL

#define PWM_PIER            0x00  /*PWM IRQ enable register 0x00*/
#define PWM_PISR            0x04  /*PWM IRQ status register 0x04*/
#define PWM_CIER            0x10  /*PWM capture IRQ enable register 0x10*/

#define PWM_CISR            0X14  /*PWM capture IRQ status register 0X14*/

#define PWM_PCCR_BASE       0x20
#define PWM_PCCR01          0x20  /*PWM01 clock configuration register*/
#define PWM_PCCR23          0x24  /*PWM23 clock configuration register */
#define PWM_PCCR45          0x28  /*PWM45 clock configuration register */
#define PWM_PCCR67          0x2C  /*PWM67 clock configuration register */
#define PWM_PCCR89          0x30  /*PWM89 clock configuration register */
#define PWM_PCCRab          0x34  /*PWMab clock configuration register */
#define PWM_PCCRcd          0x38  /*PWMcd clock configuration register */
#define PWM_PCCRef          0x3C  /*PWMef clock configuration register */

#define PWM_PCGR            0x40  /*PWM CLOCK Gating Register  0X40*/

#define PWM_PDZCR_BASE      0x60
#define PWM_PDZCR01         0x60  /*PWM01 Dead Zone Contol Register 0X60*/
#define PWM_PDZCR23         0x64  /*PWM23 Dead Zone Contol Register 0X64*/
#define PWM_PDZCR45         0x68  /*PWM45 Dead Zone Contol Register 0X68*/
#define PWM_PDZCR67         0x6C  /*PWM67 Dead Zone Contol Register 0X6C*/
#define PWM_PDZCR89         0X70  /*PWM89 Dead Zone Contol Register 0X70*/
#define PWM_PDZCRab         0X74  /*PWMad Dead Zone Contol Register 0X70*/
#define PWM_PDZCRcd         0X78  /*PWMcd Dead Zone Contol Register 0X70*/
#define PWM_PDZCRef         0X7C  /*PWMef Dead Zone Contol Register 0X70*/

#define PWM_PER             0x80  /*enable register 0x80*/

#define PWM_PGR0            0x90  /*PWM Group0 Register 0X90*/
#define PWM_PGR1            0x94  /*PWM Group1 Register 0X94*/
#define PWM_PGR2            0x98  /*PWM Group2 Register 0X98*/
#define PWM_PGR3            0x9c  /*PWM Group3 Register 0X9c*/

#define PWM_CER             0xc0  /*PWM Capture Enable Register */

#define PWM_PCR             0x0100/*PWM Contorl Register */
/* offset: 0x0100+0x0000+N*0x0020 N=0~8 */

#define PWM_PPR             0x0104/*PWM Period Register */
/* offset: 0x0100+0x00004+N*0x0020 N=0~8 */

#define PWM_PCNTR           0x0108/*PWM Counter Register */
/* offset: 0x0100+0x0008+N*0x0020 N=0~8 */

#define PWM_PPCNTR          0x010C/*PWM Pulse Counter Register */
/* offset: 0x0100+0x000c+N*0x0020 N=0~8 */

#define PWM_CCR             0x0110/*Capture Control Register */
/* offset: 0x0100+0x0010+N*0x0020 N=0~8 */

#define PWM_CRLR            0x0114/*Capture RIse Lock Register */
/* offset: 0x0100+0x0014+N*0x0020 N=0~8 */

#define PWM_CFLR            0x0118/*Capture Control Register */
/* offset: 0x0100+0x0018+N*0x0020 N=0~8 */

#define PWM_VR              0x03f0/*PWM Version Register */

/*****************************************************************************
 * define  PWM SET
 *****************************************************************************/
#define PWM_CLK_SRC_SHIFT   0x7
#define PWM_CLK_SRC_WIDTH   0x2

#define PWM_DIV_M_SHIFT     0x0
#define PWM_DIV_M_WIDTH     0x4

#define PWM_PRESCAL_SHIFT     0x0
#define PWM_PRESCAL_WIDTH     0x8

#define PWM_ACTIVE_CYCLES_SHIFT     0x0
#define PWM_ACTIVE_CYCLES_WIDTH     0x10

#define PWM_PERIOD_SHIFT     0x10
#define PWM_PERIOD_WIDTH     0x10

#define PWM_CLK_GATING_SHIFT    0x0
#define PWM_CLK_GATING_WIDTH    0x1

#define PWM_EN_CONTROL_SHIFT    0x0
#define PWM_EN_CONTORL_WIDTH    0x1

#define PWM_ACT_STA_SHIFT     0x8
#define PWM_ACT_STA_WIDTH     0x1
#define PWM_CHAN_NUM		16
#define PWM_PORT_NUM		4
/* CPUX PWM0 config */
#define SUNXI_CPUX_PWM0_CLK_TYPE     HAL_SUNXI_CCU
#define SUNXI_CPUX_PWM0_CLK_BUS_ID   0
#define SUNXI_CPUX_PWM0_CLK_ID       CLK_PWM
#define SUNXI_CPUX_PWM0_RESET_TYPE   HAL_SUNXI_RESET
#define SUNXI_CPUX_PWM0_RESET_ID     RST_BUS_PWM
#define PWM0_CPUX_BASE	             0x02000c00UL
#define SUNXI_CPUX_IRQ_PWM0          MAKE_IRQn(92, 4)

/* CPUX PWM1 config */
#define SUNXI_CPUX_PWM1_CLK_TYPE     HAL_SUNXI_CCU
#define SUNXI_CPUX_PWM1_CLK_ID       CLK_PWM1
#define SUNXI_CPUX_PWM1_CLK_BUS_ID   0
#define SUNXI_CPUX_PWM1_RESET_TYPE   HAL_SUNXI_RESET
#define SUNXI_CPUX_PWM1_RESET_ID     RST_BUS_PWM1
#define PWM1_CPUX_BASE               0x02051000UL
#define SUNXI_CPUX_IRQ_PWM1          MAKE_IRQn(107, 7)

/* CPUS PWM config */
#define SUNXI_CPUS_PWM_CLK_TYPE     HAL_SUNXI_R_CCU
#define SUNXI_CPUS_PWM_CLK_BUS_ID   CLK_BUS_R_PWM
#define SUNXI_CPUS_PWM_CLK_ID       CLK_R_PWM
#define SUNXI_CPUS_PWM_RESET_TYPE   HAL_SUNXI_R_RESET
#define SUNXI_CPUS_PWM_RESET_ID     RST_R_PWM
#define SUNXI_CPUS_IRQ_PWM          MAKE_IRQn(72, 0)
#define PWM_CPUS_BASE               0x07020c00UL

/* MCU PWM config */
#define SUNXI_MCU_PWM_CLK_TYPE     HAL_SUNXI_MCU
#define SUNXI_MCU_PWM_CLK_BUS_ID   CLK_BUS_MCU_PWM
#define SUNXI_MCU_PWM_CLK_ID       CLK_MCU_PWM
#define SUNXI_MCU_PWM_RESET_TYPE   HAL_SUNXI_MCU_RESET
#define SUNXI_MCU_PWM_RESET_ID     RST_BUS_MCU_PWM
#define SUNXI_MCU_IRQ_PWM          MAKE_IRQn(17, 0)
#define PWM_MCU_BASE               0x07103000UL

#define CPUX_PWM0_PARAMS \
{	.port = 0, \
	.reg_base = PWM0_CPUX_BASE, .irq_num = SUNXI_CPUX_IRQ_PWM0, .gpio_num = 16, \
	.clk_type = SUNXI_CPUX_PWM0_CLK_TYPE, .bus_clk_id = SUNXI_CPUX_PWM0_CLK_BUS_ID, \
	.clk_id = SUNXI_CPUX_PWM0_CLK_ID, .reset_type = SUNXI_CPUX_PWM0_RESET_TYPE, \
	.reset_id = SUNXI_CPUX_PWM0_RESET_ID, \
}

#define CPUX_PWM1_PARAMS \
{	.port = 1, \
	.reg_base = PWM1_CPUX_BASE, .irq_num = SUNXI_CPUX_IRQ_PWM1, .gpio_num = 4, \
	.clk_type = SUNXI_CPUX_PWM1_CLK_TYPE, .bus_clk_id = SUNXI_CPUX_PWM1_CLK_BUS_ID, \
	.clk_id = SUNXI_CPUX_PWM1_CLK_ID, .reset_type = SUNXI_CPUX_PWM1_RESET_TYPE, \
	.reset_id = SUNXI_CPUX_PWM1_RESET_ID, \
}

#define CPUS_PWM_PARAMS \
{	.port = 2, \
	.reg_base = PWM_CPUS_BASE, .irq_num = SUNXI_CPUS_IRQ_PWM, .gpio_num = 2, \
	.clk_type = SUNXI_CPUS_PWM_CLK_TYPE, .bus_clk_id = SUNXI_CPUS_PWM_CLK_BUS_ID, \
	.clk_id = SUNXI_CPUS_PWM_CLK_ID, .reset_type = SUNXI_CPUS_PWM_RESET_TYPE, \
	.reset_id = SUNXI_CPUS_PWM_RESET_ID, \
}

#define MCU_PWM_PARAMS \
{	.port = 3, \
	.reg_base = PWM_MCU_BASE, .irq_num = SUNXI_MCU_IRQ_PWM, .gpio_num = 8, \
	.clk_type = SUNXI_MCU_PWM_CLK_TYPE, .bus_clk_id = SUNXI_MCU_PWM_CLK_BUS_ID, \
	.clk_id = SUNXI_MCU_PWM_CLK_ID, .reset_type = SUNXI_MCU_PWM_RESET_TYPE, \
	.reset_id = SUNXI_MCU_PWM_RESET_ID, \
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
			.bind_channel = 1,
			.dead_time = 20,
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
		{
			/* pwm10 */
			.pwm_pin = GPIOD(10),
			.pwm_function = 5,
		},
		{
			/* pwm11 */
			.pwm_pin = GPIOD(11),
			.pwm_function = 5,
		},
		{
			/* pwm12 */
			.pwm_pin = GPIOD(12),
			.pwm_function = 5,
		},
		{
			/* pwm13 */
			.pwm_pin = GPIOD(13),
			.pwm_function = 5,
		},
		{
			/* pwm14 */
			.pwm_pin = GPIOD(14),
			.pwm_function = 5,
		},
		{
			/* pwm15 */
			.pwm_pin = GPIOD(15),
			.pwm_function = 5,
		},
	},

	{
		/* CPUX1 PWM1 */
		{
			/* pwm0 */
			.pwm_pin = GPIOD(16),
			.pwm_function = 5,
		},
		{
			/* pwm1 */
			.pwm_pin = GPIOD(17),
			.pwm_function = 5,
		},
		{
			/* pwm2 */
			.pwm_pin = GPIOD(18),
			.pwm_function = 5,
		},
		{
			/* pwm3 */
			.pwm_pin = GPIOD(19),
			.pwm_function = 5,
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

	},

	{
		/* MCU PWM */
		{
			/* pwm0 */
			.pwm_pin = GPIOL(2),
			.pwm_function = 4,
		},
		{
			/* pwm1 */
			.pwm_pin = GPIOL(3),
			.pwm_function = 4,
		},
		{
			/* pwm2 */
			.pwm_pin = GPIOL(4),
			.pwm_function = 4,
			.bind_channel = 3,
			.dead_time = 20,
		},
		{
			/* pwm3 */
			.pwm_pin = GPIOL(5),
			.pwm_function = 4,
		},
		{
			/* pwm4 */
			.pwm_pin = GPIOL(6),
			.pwm_function = 3,
			.bind_channel = 5,
			.dead_time = 20,
		},
		{
			/* pwm5 */
			.pwm_pin = GPIOL(7),
			.pwm_function = 3,
		},
		{
			/* pwm6 */
			.pwm_pin = GPIOL(8),
			.pwm_function = 3,
		},
		{
			/* pwm7 */
			.pwm_pin = GPIOL(13),
			.pwm_function = 3,
		},
	}
};

#endif /* __PWM_SUN55IW3_H__ */
