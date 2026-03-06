/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#ifndef __IRQS_SUN300IW1_H
#define __IRQS_SUN300IW1_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	UART0_IRQn              = 18,
	UART1_IRQn              = 19,
	UART2_IRQn              = 20,
	UART3_IRQn              = 21,

	TWI0_IRQn               = 25,
	TWI1_IRQn               = 26,
	TWI2_IRQn               = 27,

	SPI0_IRQn               = 31,
	SPI1_IRQn               = 32,
	SPI2_IRQn               = 33,

	PWM_IRQn                = 34,
	SPIF_IRQn               = 35,
	LEDC_IRQn               = 36,
	ADDA_IRQn               = 41,
	I2S0_IRQn               = 42,

	USB0_OTG_IRQn           = 45,
	USB0_EHCI_IRQn          = 46,
	USB0_OHCI_IRQn          = 47,

	DMAC1_IRQn              = 53,
	SS_IRQn                 = 54,
	SD0_IRQn                = 56,
	SD1_IRQn                = 57,
	MSI_IRQn                = 59,
	GMAC0_IRQn              = 62,
	HRDY_TOUT_IRQn          = 65,
	DMAC_IRQn               = 66,
	SPINLOCK_IRQn           = 70,
	HSTIMER0_IRQn           = 71,
	HSTIMER1_IRQn           = 72,
	GPA_IRQn                = 73,
	THS_IRQn                = 74,
	TIMER0_IRQn             = 75,
	TIMER1_IRQn             = 76,
	VE_IRQn                 = 82,

	GPIOA_IRQn              = 83,
	GPIOC_IRQn              = 87,
	GPIOD_IRQn              = 89,
	GPIOL_IRQn              = 99,
	DE0_IRQn                = 103,
	G2D_IRQn                = 105,
	LCD0_IRQn               = 106,

	CSI_DMA0_IRQn           = 111,
	CSI_DMA1_IRQn           = 112,
	CSI_PARSER0_IRQn        = 116,
	CSI_PARSER1_IRQn        = 117,
	CSI_CMB_IRQn            = 120,
	CSI_TDM_IRQn            = 121,
	CSI_TOP_PKT_IRQn        = 122,
	CSI_IPS0_IRQn           = 124,
	CSI_IPS1_IRQn           = 125,
	CSI_IPS2_IRQn           = 126,
	CSI_IPS3_IRQn           = 127,
	CSI_VIPP0_IRQn          = 128,
	CSI_VIPP1_IRQn          = 129,

	A27_WDG_IRQn            = 136,
	A27_MBOX_WR_IRQn        = 137,
	A27_MBOX_RD_IRQn        = 138,
	A27_L2C_ERR_INT_IRQn    = 139,
	E907_MBOX_RD_IRQn       = 144,
	E907_MBOX_WR_IRQn       = 145,
	E907_WDG_IRQn           = 146,

	WLAN_IRQn               = 175,
	VCCIO_DET_IRQn          = 179,
	OTP_IRQn                = 180,
	RTC_TMR_IRQn            = 181,
	ALARM0_IRQn             = 182,
	ALARM1_IRQn             = 183,
	WDG_RTC_IRQn            = 184,
	RCO_CAL_IRQn            = 185,
	WAKUPIO_IRQn            = 186,
	BTCOEX_IRQn             = 187,
	BLE_LL_IRQn             = 188,
	BTC_BB_IRQn             = 189,
	BTC_DBG_IRQn            = 190,
	BTC_SLPTMR_IRQn         = 192,
} IRQn_Type;

#define SUNXI_IRQ_MAX   (193)
#ifdef __cplusplus
}
#endif

#endif    /* __IRQS_SUN300IW1_H */
