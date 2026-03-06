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

#ifndef __TMR_SUN55IW6_H__
#define __TMR_SUN55IW6_H__

#include <hal_clk.h>
#include <hal_reset.h>

#define SUNXI_SYS_HTIMER_BASE	0x03008000
#define SUNXI_R_HTIMER_BASE	0x07091000
#define SUNXI_RISCV_HTIMER_BASE	0x01a08000

#define SUNXI_SYS_TIMER_NUM	8
#define SUNXI_R_TIMER_NUM	4
#define SUNXI_RISCV_TIMER_NUM	4
#define HTIMER_MAX_NUM		16

#define SUNXI_HTIMER_BASE(id)\
({int base = 0;\
if (id <= (SUNXI_SYS_TIMER_NUM - 1))\
base = SUNXI_SYS_HTIMER_BASE;\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER_NUM - 1))\
base  = SUNXI_R_HTIMER_BASE;\
else if (id <= HTIMER_MAX_NUM - 1)\
base = SUNXI_RISCV_HTIMER_BASE;\
base;\
})

#define HTIMER_IRQ_EN(id)\
({int bit = 0;\
if (id <= (SUNXI_SYS_TIMER_NUM -1))\
bit = BIT(id);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER_NUM - 1))\
bit = BIT(id - SUNXI_SYS_TIMER_NUM);\
else if (id <= HTIMER_MAX_NUM -1)\
bit = BIT(id - SUNXI_SYS_TIMER_NUM - SUNXI_R_TIMER_NUM);\
bit;\
})

#define PENDING_BIT(id)\
({int bit = 0;\
if (id <= (SUNXI_SYS_TIMER_NUM -1))\
bit = BIT(id);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER_NUM - 1))\
bit = BIT(id - SUNXI_SYS_TIMER_NUM);\
else if (id <= HTIMER_MAX_NUM -1)\
bit = BIT(id - SUNXI_SYS_TIMER_NUM - SUNXI_R_TIMER_NUM);\
bit;\
})

#define HTIMER_CTL_REG_OFFSET(id)\
({int offset = 0;\
if (id <= (SUNXI_SYS_TIMER_NUM -1))\
offset = (0x20 * (id) + 0x20);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER_NUM) + 0x20);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER_NUM - SUNXI_R_TIMER_NUM) + 0x20);\
offset;\
})

#define HTIMER_INTVAL_LO_REG_OFFSET(id)\
({int offset = 0;\
if (id <= (SUNXI_SYS_TIMER_NUM -1))\
offset = (0x20 * (id) + 0x24);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER_NUM) + 0x24);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER_NUM - SUNXI_R_TIMER_NUM) + 0x24);\
offset;\
})

#define HTIMER_CNTVAL_LO_REG_OFFSET(id)\
({int offset = 0;\
if (id <= (SUNXI_SYS_TIMER_NUM -1))\
offset = (0x20 * (id) + 0x28);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER_NUM) + 0x28);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER_NUM - SUNXI_R_TIMER_NUM) + 0x28);\
offset;\
})

#define HTIMER_INTVAL_HI_REG_OFFSET(id)\
({int offset = 0;\
if (id <= (SUNXI_SYS_TIMER_NUM -1))\
offset = (0x20 * (id) + 0x2c);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER_NUM) + 0x2c);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER_NUM - SUNXI_R_TIMER_NUM) + 0x2c);\
offset;\
})

#define HTIMER_CNTVAL_HI_REG_OFFSET(id)\
({int offset = 0;\
if (id <= (SUNXI_SYS_TIMER_NUM -1))\
offset = (0x20 * (id) + 0x30);\
else if (id <= (SUNXI_R_TIMER_NUM + SUNXI_SYS_TIMER_NUM - 1))\
offset  = (0x20 * (id - SUNXI_SYS_TIMER_NUM) + 0x30);\
else if (id <= HTIMER_MAX_NUM -1)\
offset = (0x20 * (id - SUNXI_SYS_TIMER_NUM - SUNXI_R_TIMER_NUM) + 0x30);\
offset;\
})


#define SUNXI_IRQ_HSTIMER0	MAKE_IRQn(71, 0)
#define SUNXI_IRQ_HSTIMER1	MAKE_IRQn(72, 0)
#define SUNXI_IRQ_HSTIMER2	MAKE_IRQn(73, 0)
#define SUNXI_IRQ_HSTIMER3	MAKE_IRQn(74, 0)
#define SUNXI_IRQ_HSTIMER4	MAKE_IRQn(75, 0)
#define SUNXI_IRQ_HSTIMER5	MAKE_IRQn(76, 0)
#define SUNXI_IRQ_HSTIMER6	MAKE_IRQn(172, 0)
#define SUNXI_IRQ_HSTIMER7	MAKE_IRQn(173, 0)
#define SUNXI_IRQ_HSTIMER8	MAKE_IRQn(212, 0)
#define SUNXI_IRQ_HSTIMER9	MAKE_IRQn(213, 0)
#define SUNXI_IRQ_HSTIMER10	MAKE_IRQn(214, 0)
#define SUNXI_IRQ_HSTIMER11	MAKE_IRQn(215, 0)
#define SUNXI_IRQ_HSTIMER12	MAKE_IRQn(202, 0)
#define SUNXI_IRQ_HSTIMER13	MAKE_IRQn(203, 0)
#define SUNXI_IRQ_HSTIMER14	MAKE_IRQn(204, 0)
#define SUNXI_IRQ_HSTIMER15	MAKE_IRQn(205, 0)

#define TIMER0_PARAMS \
{	.id = 0, \
	.reg_base = SUNXI_SYS_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER0, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER, \
}

#define TIMER1_PARAMS \
{	.id = 1, \
	.reg_base = SUNXI_SYS_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER1, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER1, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER, \
}

#define TIMER2_PARAMS \
{	.id = 2, \
	.reg_base = SUNXI_SYS_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER2, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER2, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER, \
}

#define TIMER3_PARAMS \
{	.id = 3, \
	.reg_base = SUNXI_SYS_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER3, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER3, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER, \
}

#define TIMER4_PARAMS \
{	.id = 4, \
	.reg_base = SUNXI_SYS_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER4, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER4, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER, \
}

#define TIMER5_PARAMS \
{	.id = 5, \
	.reg_base = SUNXI_SYS_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER5, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER5, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER, \
}

#define TIMER6_PARAMS \
{	.id = 6, \
	.reg_base = SUNXI_SYS_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER6, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER6, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER, \
}

#define TIMER7_PARAMS \
{	.id = 7, \
	.reg_base = SUNXI_SYS_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER7, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER7, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_TIMER, \
}

#define TIMER8_PARAMS \
{	.id = 8, \
	.reg_base = SUNXI_R_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER8, \
	.clk_type = HAL_SUNXI_R_CCU, .clk_id = CLK_R_TIMER0, \
	.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_R_TIMER, \
	.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_BUS_R_TIMER, \
}

#define TIMER9_PARAMS \
{	.id = 9, \
	.reg_base = SUNXI_R_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER9, \
	.clk_type = HAL_SUNXI_R_CCU, .clk_id = CLK_R_TIMER1, \
	.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_R_TIMER, \
	.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_BUS_R_TIMER, \
}

#define TIMER10_PARAMS \
{	.id = 10, \
	.reg_base = SUNXI_R_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER10, \
	.clk_type = HAL_SUNXI_R_CCU, .clk_id = CLK_R_TIMER2, \
	.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_R_TIMER, \
	.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_BUS_R_TIMER, \
}

#define TIMER11_PARAMS \
{	.id = 11, \
	.reg_base = SUNXI_R_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER11, \
	.clk_type = HAL_SUNXI_R_CCU, .clk_id = CLK_R_TIMER3, \
	.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_R_TIMER, \
	.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_BUS_R_TIMER, \
}

#define TIMER12_PARAMS \
{	.id = 12, \
	.reg_base = SUNXI_RISCV_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER12, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER0_RV, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER_RV, \
	.reset_type = HAL_SUNXI_RESET, .reset_id =  RST_BUS_TIMER_RV, \
}

#define TIMER13_PARAMS \
{	.id = 13, \
	.reg_base = SUNXI_RISCV_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER13, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER1_RV, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER_RV, \
	.reset_type = HAL_SUNXI_RESET, .reset_id =  RST_BUS_TIMER_RV, \
}

#define TIMER14_PARAMS \
{	.id = 14, \
	.reg_base = SUNXI_RISCV_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER14, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER2_RV, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER_RV, \
	.reset_type = HAL_SUNXI_RESET, .reset_id =  RST_BUS_TIMER_RV, \
}

#define TIMER15_PARAMS \
{	.id = 15, \
	.reg_base = SUNXI_RISCV_HTIMER_BASE, \
	.irq_num = SUNXI_IRQ_HSTIMER15, \
	.clk_type = HAL_SUNXI_CCU, .clk_id = CLK_TIMER3_RV, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_TIMER_RV, \
	.reset_type = HAL_SUNXI_RESET, .reset_id =  RST_BUS_TIMER_RV, \
}
#endif /*__TWI_SUN55IW6_H__  */
