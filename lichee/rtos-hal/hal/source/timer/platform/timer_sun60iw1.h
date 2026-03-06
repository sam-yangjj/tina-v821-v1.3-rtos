/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.
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

#ifndef __TMR_SUN60IW1_H__
#define __TMR_SUN60IW1_H__

#include <hal_clk.h>
#include <hal_reset.h>

#define SUNXI_SYS_HTIMER0_BASE	0x03009000
#define SUNXI_SYS_HTIMER1_BASE	0x0300A000
#define SUNXI_R_HTIMER_BASE	0x07091000
#define SUNXI_DSP_HTIMER_BASE	0x07323000
#define SUNXI_RISCV_HTIMER_BASE	0x07138000

#define SUNXI_SYS_TIMER0_NUM	10
#define SUNXI_SYS_TIMER1_NUM	6
#define SUNXI_R_TIMER_NUM	4
#define SUNXI_DSP_TIMER_NUM	4
#define SUNXI_RISCV_TIMER_NUM	4
#define HTIMER_MAX_NUM		28

#define SUNXI_HTIMER_BASE(id)\
({int base;\
if (id <= (SUNXI_SYS_TIMER0_NUM - 1))\
base = SUNXI_SYS_HTIMER0_BASE;\
else if (id <= (SUNXI_SYS_TIMER0_NUM + SUNXI_SYS_TIMER1_NUM - 1))\
base  = SUNXI_SYS_HTIMER1_BASE;\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER1_NUM + SUNXI_SYS_TIMER0_NUM - 1))\
base  = SUNXI_R_HTIMER_BASE;\
else if (id <= HTIMER_MAX_NUM - SUNXI_RISCV_TIMER_NUM - 1)\
base = SUNXI_DSP_HTIMER_BASE;\
else if (id <= HTIMER_MAX_NUM - 1)\
base = SUNXI_RISCV_HTIMER_BASE;\
base;\
})

#if defined(CONFIG_ARCH_DSP)
#define SUNXI_IRQ_HSTIMER0	MAKE_IRQn(74, 0) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER1	MAKE_IRQn(74, 1) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER2	MAKE_IRQn(74, 2) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER3	MAKE_IRQn(74, 3) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER4	MAKE_IRQn(74, 4) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER5	MAKE_IRQn(74, 5) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER6	MAKE_IRQn(94, 0) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER7	MAKE_IRQn(94, 1) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER8	MAKE_IRQn(94, 2) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER9	MAKE_IRQn(94, 3) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER10	MAKE_IRQn(94, 4) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER11	MAKE_IRQn(94, 5) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER12	MAKE_IRQn(94, 6) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER13	MAKE_IRQn(94, 7) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER14	MAKE_IRQn(95, 0) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER15	MAKE_IRQn(95, 1) | RINTC_IRQ_MASK
#define SUNXI_IRQ_HSTIMER16	(RINTC_IRQ_MASK | 21)
#define SUNXI_IRQ_HSTIMER17	(RINTC_IRQ_MASK | 22)
#define SUNXI_IRQ_HSTIMER18	(RINTC_IRQ_MASK | 23)
#define SUNXI_IRQ_HSTIMER19	(RINTC_IRQ_MASK | 46)
#define SUNXI_IRQ_HSTIMER20	24
#define SUNXI_IRQ_HSTIMER21	25
#define SUNXI_IRQ_HSTIMER22	26
#define SUNXI_IRQ_HSTIMER23	27
#define SUNXI_IRQ_HSTIMER24	(RINTC_IRQ_MASK | 54)
#define SUNXI_IRQ_HSTIMER25	(RINTC_IRQ_MASK | 55)
#define SUNXI_IRQ_HSTIMER26	(RINTC_IRQ_MASK | 56)
#define SUNXI_IRQ_HSTIMER27	(RINTC_IRQ_MASK | 57)
#else
#define SUNXI_IRQ_HSTIMER0	MAKE_IRQn(38, 1)
#define SUNXI_IRQ_HSTIMER1	MAKE_IRQn(38, 2)
#define SUNXI_IRQ_HSTIMER2	MAKE_IRQn(38, 3)
#define SUNXI_IRQ_HSTIMER3	MAKE_IRQn(38, 4)
#define SUNXI_IRQ_HSTIMER4	MAKE_IRQn(38, 5)
#define SUNXI_IRQ_HSTIMER5	MAKE_IRQn(38, 6)
#define SUNXI_IRQ_HSTIMER6	MAKE_IRQn(58, 1)
#define SUNXI_IRQ_HSTIMER7	MAKE_IRQn(58, 2)
#define SUNXI_IRQ_HSTIMER8	MAKE_IRQn(58, 3)
#define SUNXI_IRQ_HSTIMER9	MAKE_IRQn(58, 4)
#define SUNXI_IRQ_HSTIMER10	MAKE_IRQn(58, 5)
#define SUNXI_IRQ_HSTIMER11	MAKE_IRQn(58, 6)
#define SUNXI_IRQ_HSTIMER12	MAKE_IRQn(58, 7)
#define SUNXI_IRQ_HSTIMER13	MAKE_IRQn(58, 8)
#define SUNXI_IRQ_HSTIMER14	MAKE_IRQn(59, 1)
#define SUNXI_IRQ_HSTIMER15	MAKE_IRQn(59, 2)
#define SUNXI_IRQ_HSTIMER16	MAKE_IRQn(90, 0)
#define SUNXI_IRQ_HSTIMER17	MAKE_IRQn(91, 0)
#define SUNXI_IRQ_HSTIMER18	MAKE_IRQn(92, 0)
#define SUNXI_IRQ_HSTIMER19	MAKE_IRQn(115, 0)
#define SUNXI_IRQ_HSTIMER20	MAKE_IRQn(125, 0)
#define SUNXI_IRQ_HSTIMER21	MAKE_IRQn(126, 0)
#define SUNXI_IRQ_HSTIMER22	MAKE_IRQn(127, 0)
#define SUNXI_IRQ_HSTIMER23	MAKE_IRQn(128, 0)
#define SUNXI_IRQ_HSTIMER24	MAKE_IRQn(18, 0)
#define SUNXI_IRQ_HSTIMER25	MAKE_IRQn(19, 0)
#define SUNXI_IRQ_HSTIMER26	MAKE_IRQn(20, 0)
#define SUNXI_IRQ_HSTIMER27	MAKE_IRQn(21, 0)
#endif

#define TIMER0_PARAMS \
{	.id = 0, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER0, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK0, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}

#define TIMER1_PARAMS \
{	.id = 1, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER1, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK1, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}

#define TIMER2_PARAMS \
{	.id = 2, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER2, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK2, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}

#define TIMER3_PARAMS \
{	.id = 3, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER3, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK3, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}

#define TIMER4_PARAMS \
{	.id = 4, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER4, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK4, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}

#define TIMER5_PARAMS \
{	.id = 5, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER5, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK5, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}

#define TIMER6_PARAMS \
{	.id = 6, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER6, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK6, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}

#define TIMER7_PARAMS \
{	.id = 7, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER7, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK7, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}

#define TIMER8_PARAMS \
{	.id = 8, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER8, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK8, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}
#define TIMER9_PARAMS \
{	.id = 9, \
	.reg_base = SUNXI_SYS_HTIMER0_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER9, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_CLK9, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER0, \
}

#define TIMER10_PARAMS \
{	.id = 10, \
	.reg_base = SUNXI_SYS_HTIMER1_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER10, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER1_CLK0, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER1, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER1, \
}

#define TIMER11_PARAMS \
{	.id = 11, \
	.reg_base = SUNXI_SYS_HTIMER1_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER11, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER1_CLK1, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER1, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER1, \
}

#define TIMER12_PARAMS \
{	.id = 12, \
	.reg_base = SUNXI_SYS_HTIMER1_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER12, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER1_CLK2, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER1, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER1, \
}

#define TIMER13_PARAMS \
{	.id = 13, \
	.reg_base = SUNXI_SYS_HTIMER1_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER13, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER1_CLK3, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER1, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER1, \
}

#define TIMER14_PARAMS \
{	.id = 14, \
	.reg_base = SUNXI_SYS_HTIMER1_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER14, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER1_CLK4, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER1, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER1, \
}

#define TIMER15_PARAMS \
{	.id = 15, \
	.reg_base = SUNXI_SYS_HTIMER1_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER15, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER1_CLK5, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_TIMER1, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER1, \
}
#define TIMER16_PARAMS \
{	.id = 16, \
	.reg_base = SUNXI_R_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER16, \
	.clk_type = HAL_SUNXI_R_CCU, .clk_id = CLK_R_TIMER0, \
	.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_R_BUS_TIMER, \
	.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_R_TIMER, \
}

#define TIMER17_PARAMS \
{	.id = 17, \
	.reg_base = SUNXI_R_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER17, \
	.clk_type = HAL_SUNXI_R_CCU, .clk_id = CLK_R_TIMER1, \
	.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_R_BUS_TIMER, \
	.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_R_TIMER, \
}

#define TIMER18_PARAMS \
{	.id = 18, \
	.reg_base = SUNXI_R_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER18, \
	.clk_type = HAL_SUNXI_R_CCU, .clk_id = CLK_R_TIMER2, \
	.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_R_BUS_TIMER, \
	.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_R_TIMER, \
}

#define TIMER19_PARAMS \
{	.id = 19, \
	.reg_base = SUNXI_R_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER19, \
	.clk_type = HAL_SUNXI_R_CCU, .clk_id = CLK_R_TIMER3, \
	.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_R_BUS_TIMER, \
	.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_R_TIMER, \
}

#define TIMER20_PARAMS \
{	.id = 20, \
	.reg_base = SUNXI_DSP_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER20, \
	.clk_type = HAL_SUNXI_DSP, .clk_id = CLK_DSP_TIMER0, \
	.bus_type = HAL_SUNXI_DSP, .bus_id = CLK_BUS_DSP_TIMER, \
	.reset_type = HAL_SUNXI_DSP_RESET, .reset_id = RST_BUS_DSP_TIME, \
}

#define TIMER21_PARAMS \
{	.id = 21, \
	.reg_base = SUNXI_DSP_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER21, \
	.clk_type = HAL_SUNXI_DSP, .clk_id = CLK_DSP_TIMER1, \
	.bus_type = HAL_SUNXI_DSP, .bus_id = CLK_BUS_DSP_TIMER, \
	.reset_type = HAL_SUNXI_DSP_RESET, .reset_id = RST_BUS_DSP_TIME, \
}

#define TIMER22_PARAMS \
{	.id = 22, \
	.reg_base = SUNXI_DSP_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER22, \
	.clk_type = HAL_SUNXI_DSP, .clk_id = CLK_DSP_TIMER2, \
	.bus_type = HAL_SUNXI_DSP, .bus_id = CLK_BUS_DSP_TIMER, \
	.reset_type = HAL_SUNXI_DSP_RESET, .reset_id = RST_BUS_DSP_TIME, \
}

#define TIMER23_PARAMS \
{	.id = 23, \
	.reg_base = SUNXI_DSP_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER23, \
	.clk_type = HAL_SUNXI_DSP, .clk_id = CLK_DSP_TIMER3, \
	.bus_type = HAL_SUNXI_DSP, .bus_id = CLK_BUS_DSP_TIMER, \
	.reset_type = HAL_SUNXI_DSP_RESET, .reset_id = RST_BUS_DSP_TIME, \
}

#define TIMER24_PARAMS \
{	.id = 24, \
	.reg_base = SUNXI_RISCV_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER24, \
	.clk_type = HAL_SUNXI_DSP, .clk_id = CLK_RV_TIMER0, \
	.bus_type = HAL_SUNXI_DSP, .bus_id = CLK_BUS_RV_TIMER, \
	.reset_type = HAL_SUNXI_DSP_RESET, .reset_id = RST_BUS_RV_TIME, \
}

#define TIMER25_PARAMS \
{	.id = 25, \
	.reg_base = SUNXI_RISCV_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER25, \
	.clk_type = HAL_SUNXI_DSP, .clk_id = CLK_RV_TIMER1, \
	.bus_type = HAL_SUNXI_DSP, .bus_id = CLK_BUS_RV_TIMER, \
	.reset_type = HAL_SUNXI_DSP_RESET, .reset_id = RST_BUS_RV_TIME, \
}

#define TIMER26_PARAMS \
{	.id = 26, \
	.reg_base = SUNXI_RISCV_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER26, \
	.clk_type = HAL_SUNXI_DSP, .clk_id = CLK_RV_TIMER2, \
	.bus_type = HAL_SUNXI_DSP, .bus_id = CLK_BUS_RV_TIMER, \
	.reset_type = HAL_SUNXI_DSP_RESET, .reset_id = RST_BUS_RV_TIME, \
}

#define TIMER27_PARAMS \
{	.id = 27, \
	.reg_base = SUNXI_RISCV_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER27, \
	.clk_type = HAL_SUNXI_DSP, .clk_id = CLK_RV_TIMER3, \
	.bus_type = HAL_SUNXI_DSP, .bus_id = CLK_BUS_RV_TIMER, \
	.reset_type = HAL_SUNXI_DSP_RESET, .reset_id = RST_BUS_RV_TIME, \
}

#define HTIMER_IRQ_EN(id)\
({int bit;\
if (id <= (SUNXI_SYS_TIMER0_NUM -1))\
bit = BIT(id);\
else if (id <= (SUNXI_SYS_TIMER0_NUM + SUNXI_SYS_TIMER1_NUM - 1))\
bit = BIT(id - SUNXI_SYS_TIMER0_NUM);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER1_NUM + SUNXI_SYS_TIMER0_NUM - 1))\
bit = BIT(id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM);\
else if (id <= HTIMER_MAX_NUM - SUNXI_RISCV_TIMER_NUM - 1)\
bit = BIT(id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM - SUNXI_R_TIMER_NUM);\
else if (id <= HTIMER_MAX_NUM -1)\
bit = BIT(id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM - SUNXI_R_TIMER_NUM - SUNXI_DSP_TIMER_NUM);\
bit;\
})

#define PENDING_BIT(id)\
({int bit;\
if (id <= (SUNXI_SYS_TIMER0_NUM -1))\
bit = BIT(id);\
else if (id <= (SUNXI_SYS_TIMER0_NUM + SUNXI_SYS_TIMER1_NUM - 1))\
bit = BIT(id - SUNXI_SYS_TIMER0_NUM);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER1_NUM + SUNXI_SYS_TIMER0_NUM - 1))\
bit = BIT(id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM);\
else if (id <= HTIMER_MAX_NUM - SUNXI_RISCV_TIMER_NUM - 1)\
bit = BIT(id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM - SUNXI_R_TIMER_NUM);\
else if (id <= HTIMER_MAX_NUM -1)\
bit = BIT(id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM - SUNXI_R_TIMER_NUM - SUNXI_DSP_TIMER_NUM);\
bit;\
})

#define HTIMER_CTL_REG_OFFSET(id)\
({int offset;\
if (id <= (SUNXI_SYS_TIMER0_NUM -1))\
offset = (0x20 * (id) + 0x20);\
else if (id <= (SUNXI_SYS_TIMER0_NUM + SUNXI_SYS_TIMER1_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM) + 0x20);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER1_NUM + SUNXI_SYS_TIMER0_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM) + 0x20);\
else if (id <= HTIMER_MAX_NUM - SUNXI_RISCV_TIMER_NUM - 1)\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM -SUNXI_R_TIMER_NUM) + 0x20);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM - SUNXI_R_TIMER_NUM - SUNXI_DSP_TIMER_NUM) + 0x20);\
offset;\
})

#define HTIMER_INTVAL_LO_REG_OFFSET(id)\
({int offset;\
if (id <= (SUNXI_SYS_TIMER0_NUM -1))\
offset = (0x20 * (id) + 0x24);\
else if (id <= (SUNXI_SYS_TIMER0_NUM + SUNXI_SYS_TIMER1_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM) + 0x24);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER1_NUM + SUNXI_SYS_TIMER0_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM) + 0x24);\
else if (id <= HTIMER_MAX_NUM - SUNXI_RISCV_TIMER_NUM - 1)\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM -SUNXI_R_TIMER_NUM) + 0x24);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM - SUNXI_R_TIMER_NUM - SUNXI_DSP_TIMER_NUM) + 0x24);\
offset;\
})

#define HTIMER_INTVAL_HI_REG_OFFSET(id)\
({int offset;\
if (id <= (SUNXI_SYS_TIMER0_NUM -1))\
offset = (0x20 * (id) + 0x28);\
else if (id <= (SUNXI_SYS_TIMER0_NUM + SUNXI_SYS_TIMER1_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM) + 0x28);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER1_NUM + SUNXI_SYS_TIMER0_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM) + 0x28);\
else if (id <= HTIMER_MAX_NUM - SUNXI_RISCV_TIMER_NUM - 1)\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM -SUNXI_R_TIMER_NUM) + 0x28);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM - SUNXI_R_TIMER_NUM - SUNXI_DSP_TIMER_NUM) + 0x28);\
offset;\
})

#define HTIMER_CNTVAL_LO_REG_OFFSET(id)\
({int offset;\
if (id <= (SUNXI_SYS_TIMER0_NUM -1))\
offset = (0x20 * (id) + 0x28);\
else if (id <= (SUNXI_SYS_TIMER0_NUM + SUNXI_SYS_TIMER1_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM) + 0x2c);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER1_NUM + SUNXI_SYS_TIMER0_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM) + 0x2c);\
else if (id <= HTIMER_MAX_NUM - SUNXI_RISCV_TIMER_NUM - 1)\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM -SUNXI_R_TIMER_NUM) + 0x2c);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM - SUNXI_R_TIMER_NUM - SUNXI_DSP_TIMER_NUM) + 0x2c);\
offset;\
})

#define HTIMER_CNTVAL_HI_REG_OFFSET(id)\
({int offset;\
if (id <= (SUNXI_SYS_TIMER0_NUM -1))\
offset = (0x20 * (id) + 0x30);\
else if (id <= (SUNXI_SYS_TIMER0_NUM + SUNXI_SYS_TIMER1_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM) + 0x30);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER1_NUM + SUNXI_SYS_TIMER0_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM) + 0x30);\
else if (id <= HTIMER_MAX_NUM - SUNXI_RISCV_TIMER_NUM - 1)\
offset  = (0x20 * (id - SUNXI_SYS_TIMER0_NUM -SUNXI_SYS_TIMER1_NUM -SUNXI_R_TIMER_NUM) + 0x30);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER0_NUM - SUNXI_SYS_TIMER1_NUM - SUNXI_R_TIMER_NUM - SUNXI_DSP_TIMER_NUM) + 0x30);\
offset;\
})
#endif /*__TWI_SUN55IW3_H__  */
