/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#ifndef __NDMA_SUN300IW1_H__
#define __NDMA_SUN300IW1_H__

#include <sunxi_hal_common.h>

#define DMA_IRQ_NUM				MAKE_IRQn(53, 0)	/*	NDMA 0-8 channel irq non-secure */
#define NR_MAX_CHAN				8					/* total of channels */
#define START_CHAN_OFFSET       0
#define SUNXI_CLK_DMA           CLK_DMA
#define SUNXI_CLK_MBUS_DMA      0
#define SUNXI_RST_DMA           RST_BUS_DMA

#define SUNXI_DMAC_PBASE		(0x43001000ul)

#define DMA_IRQ_EN(x)		(SUNXI_DMAC_PBASE + 0x00 + x * 0)		/* Interrupt enable register */
#define DMA_IRQ_STAT(x)		(SUNXI_DMAC_PBASE + 0x04 + x * 0)		/* Interrupt status register */
#define DMA_IRQ_STAT_OFFSET 1
#define DMA_GATE			(SUNXI_DMAC_PBASE + 0x08)				/* DMA gating register */
#define DMA_AUTO_CLK_GATE   1 << 16
#define DMA_GATE_LEN		(SUNXI_DMAC_PBASE + 0x0c)				/* DMA gating length register */
#define DMA_CFG(x)			(SUNXI_DMAC_PBASE + (0x100 + ((x + START_CHAN_OFFSET) << 5)))	/* Configuration register RO */
#define DMA_CUR_SRC(x)		(SUNXI_DMAC_PBASE + (0x104 + ((x + START_CHAN_OFFSET) << 5)))	/* Current source address RO */
#define DMA_CUR_DST(x)		(SUNXI_DMAC_PBASE + (0x108 + ((x + START_CHAN_OFFSET) << 5)))	/* Current destination address RO */
#define DMA_CNT(x)			(SUNXI_DMAC_PBASE + (0x10c + ((x + START_CHAN_OFFSET) << 5)))	/* Byte counter left register RO */

#define SHIFT_IRQ_MASK(val, ch) ({	\
		(ch + START_CHAN_OFFSET) >= HIGH_CHAN	\
		? (val) << ((ch + START_CHAN_OFFSET - HIGH_CHAN) << 1) \
		: (val) << ((ch + START_CHAN_OFFSET) << 1);	\
		})

#define NDMA_LOADING_SHIFT             31
#define NDMA_BUSY_STATUS_SHIFT         30
#define NDMA_CONTI_MODE_SHIFT          29
#define NDMA_CONTI_MODE_MASK		   0x01
#define NDMA_WAIT_STATE_SHIFT          26
#define NDMA_WAIT_STATE_MASK           0x07
#define NDMA_DEST_DATA_WIDTH_SHIFT     24
#define NDMA_DEST_DATA_WIDTH_MASK      0x03
#define NDMA_DEST_BST_LEN_SHIFT        23
#define NDMA_DEST_BST_LEN_MASK         0x01
#define NDMA_DEST_ADDR_TYPE_SHIFT	   21
#define NDMA_DEST_ADDR_TYPE_MASK	   0x03
#define NDMA_DEST_DRQ_TYPE_SHIFT	   15
#define NDMA_DEST_DRQ_TYPE_MASK	       0x3f
#define NDMA_BC_MODE_SEL_SHIFT	       14
#define NDMA_BC_MODE_SEL_MASK	       0x01
#define NDMA_SRC_DATA_WIDTH_SHIFT      9
#define NDMA_SRC_DATA_WIDTH_MASK	   0x03
#define NDMA_SRC_BST_LEN_SHIFT         8
#define NDMA_SRC_BST_LEN_MASK          0x01
#define NDMA_SRC_ADDR_TYPE_SHIFT	   6
#define NDMA_SRC_ADDR_TYPE_MASK		   0x03
#define NDMA_SRC_DRQ_TYPE_SHIFT		   0
#define NDMA_SRC_DRQ_TYPE_MASK		   0x3f
#define DST_SRQ_MASK                   NDMA_DEST_DRQ_TYPE_MASK
#define SRC_SRQ_MASK                   NDMA_SRC_DRQ_TYPE_MASK
#define NDMA_CFG_MASK(mask, shift)     mask << shift
#define NDMA_CFG_SET(val, shift)	   val << shift

#define NDMA_BYTE_COUNTER_REG_MASK     0x3ffff

#define DMA_LOADING	        ((1) << NDMA_LOADING_SHIFT)
#define DMA_BUSY_STATUS     ((1) << NDMA_BUSY_STATUS_SHIFT)
#define SRC_CONTI(x)		((x) << NDMA_CONTI_MODE_SHIFT)
#define SRC_WIDTH(x)		((x) << NDMA_SRC_DATA_WIDTH_SHIFT)
#define SRC_BURST(x)		((x) << NDMA_SRC_BST_LEN_SHIFT)
#define SRC_ADDR_MODE(x)    ((x) << NDMA_SRC_ADDR_TYPE_SHIFT)
#define SRC_NOCHANGE_MODE	(0x01 << NDMA_SRC_ADDR_TYPE_SHIFT)
#define SRC_INCREMENT_MODE  (0x00 << NDMA_SRC_ADDR_TYPE_SHIFT)
#define SRC_DRQ(x)			((x) << NDMA_SRC_DRQ_TYPE_SHIFT)
#define DST_WIDTH(x)		((x) << NDMA_DEST_DATA_WIDTH_SHIFT)
#define DST_BURST(x)		((x) << NDMA_DEST_BST_LEN_SHIFT)
#define DST_ADDR_MODE(x)    ((x) << NDMA_DEST_ADDR_TYPE_SHIFT)
#define DST_NOCHANGE_MODE	(0x01 << NDMA_DEST_ADDR_TYPE_SHIFT)
#define DST_INCREMENT_MODE	(0x00 << NDMA_DEST_ADDR_TYPE_SHIFT)
#define DST_DRQ(x)			((x) << NDMA_DEST_DRQ_TYPE_SHIFT)
#define DMA_BCMODE(x)		((x) << NDMA_BC_MODE_SEL_SHIFT)
#define DMA_WAIT(x)         ((x) << NDMA_WAIT_STATE_SHIFT)
#define NORMAL_WAIT			(0x03 << NDMA_WAIT_STATE_SHIFT)

#define GET_SRC_DRQ(x)		((x) & (NDMA_SRC_DRQ_TYPE_MASK << NDMA_SRC_DRQ_TYPE_SHIFT))
#define GET_DST_DRQ(x)		((x) & (NDMA_DEST_DRQ_TYPE_MASK << NDMA_DEST_DRQ_TYPE_SHIFT))

#define sunxi_slave_id(d, s) 	(((d)<<15) | (s))

#define NORMAL_DMA
/*
 * The source DRQ type and port corresponding relation
 */
#define DRQSRC_SDRAM			0
#define DRQSRC_LSPSRAM			1
/* #define DRQSRC_OWA			2 */
#define DRQSRC_I2S0_RX			3
/* #define DRQSRC_I2S1_RX		4 */
/* #define DRQSRC_I2S2_RXX		5 */
/*#define DRQSRC_MAD_RX	X		6 */
#define DRQSRC_AUDIO_CODEC		7
/*#define DRQSRC_DMIC			8 */
/*#define DRQSRC_OWA			9 */
/*#define DRQSRC_LSPSRAM_CTRL	10 */
/*#define DRQSRC_FLASH_CTRL     11 */
#define DRQSRC_GPADC			12
/*#define DRQSRC_CODEC_DAC_RX	13 */
#define DRQSRC_UART0_RX			14
#define DRQSRC_UART1_RX			15
#define DRQSRC_UART2_RX         16
#define DRQSRC_UART3_RX			17
/* #define DRQSRC_UART4_RX      18 */
/* #define DRQSRC_UART5_RX      19 */
/* #define DRQSRC_RESEVER       20 */
/* #define DRQSRC_RESEVER       21 */
#define DRQSRC_SPI0_RX			22
#define DRQSRC_SPI1_RX			23
#define DRQSRC_SPI2_RX			24
/* #define DRQSRC_SPI3_RX		25 */
/* #define DRQSRC_RESEVER       26 */
/* #define DRQSRC_RESEVER       27 */
/* #define DRQSRC_SS_MBUS       28 */
/* #define DRQSRC_RESEVER       29 */
#define DRQSRC_OTG_EP1			30
#define DRQSRC_OTG_EP2			31
#define DRQSRC_OTG_EP3          32
#define DRQSRC_OTG_EP4          33
#define DRQSRC_OTG_EP5          34
#define DRQSRC_TWI0_RX			43
#define DRQSRC_TWI1_RX			44
#define DRQSRC_TWI2_RX			45
#define DRQSRC_CE_WRITE			53

/*
 * The destination DRQ type and port corresponding relation
 */
#define DRQDST_SDRAM			0
#define DRQDST_LSPSRAM			1
/* #define DRQDST_OWA			2 */
#define DRQDST_I2S0_TX			3
/* #define DRQDST_I2S1_TX		4 */
/* #define DRQDST_I2S2_TX		5 */
/* #define DRQDST_MAD_TX		6 */
#define DRQDST_AUDIO_CODEC		7
/* #define DRQSRC_DMIC	        8 */
/* #define DRQDST_OWA	        9 */
/* #define DRQDST_LSPSRAM_CTRL	10 */
/* #define DRQDST_FLASH_CTRL    11 */
/* #define DRQSRC_RESEVER       12 */
#define DRQDST_UART0_TX			14
#define DRQDST_UART1_TX			15
#define DRQDST_UART2_TX         16
#define DRQDST_UART3_TX			17
/* #define DRQDST_UART4_TX		18 */
/* #define DRQDST_UART5_TX		19 */
/* #define DRQSRC_RESEVER       20 */
/* #define DRQSRC_RESEVER       21 */
#define DRQDST_SPI0_TX			22
#define DRQDST_SPI1_TX          23
#define DRQDST_SPI2_TX			24
/*#define DRQDST_SPI3_TX        25 */
/* #define DRQSRC_RESEVER       26 */
/* #define DRQSRC_RESEVER       27 */
/* #define DRQSRC_SS_MBUS		28 */
/* #define DRQSRC_RESEVER       29 */
#define DRQDST_OTG_EP1			30
#define DRQDST_OTG_EP2			31
#define DRQDST_OTG_EP3          32
#define DRQDST_OTG_EP4          33
#define DRQDST_OTG_EP5          34
#define DRQDST_TWI0_TX			43
#define DRQDST_TWI1_TX          44
#define DRQDST_TWI2_TX          45
#define DRQDST_CE_READ          53

/*
 * Fix sconfig's bus width according to at_dmac.
 * 1 byte -> 0, 2 bytes -> 1, 4 bytes -> 2
 */

static inline int32_t convert_buswidth(uint32_t *addr_width)
{
    if (*addr_width > 4)
    {
        return -1;
    }

    switch (*addr_width)
    {
        case 2:
            *addr_width = 1;
            break;
        case 4:
            *addr_width = 2;
            break;
        default:
            /* For 1 byte width or fallback */
            *addr_width = 0;
            break;
    }

	return 0;
}

static inline int32_t convert_burst(uint32_t *maxburst)
{
    if (*maxburst > 4)
    {
        return -1;
    }

    switch (*maxburst)
    {
        case 1:
            *maxburst = 0;
            break;
        case 4:
            *maxburst = 1;
            break;
        default:
            *maxburst = 0;
            break;
    }

	return 0;
}

//#define DMA_DEBUG

static inline void sunxi_dump_chan_regs(u32 chan_num)
{
#ifdef DMA_DEBUG
    printf("Chan %d reg:\n"
           "\tDMA_IRQ_EN(0x%08lx):   0x%08x\n"
           "\tDMA_IRQ_STAT(0x%08lx): 0x%08x\n"
           "\tDMA_GATE(0x%08lx):     0x%08x\n"
           "\tDMA_GATE_LEN(0x%08lx): 0x%08x\n"
           "\tDMA_CFG(0x%08lx):      0x%08x\n"
           "\tDMA_CUR_SRC(0x%08lx):  0x%08x\n"
           "\tDMA_CUR_DST(0x%08lx):  0x%08x\n"
           "\tDMA_CNT(0x%08lx):      0x%08x\n\n",
           chan_num,
           DMA_IRQ_EN(chan_num), (uint32_t)hal_readl(DMA_IRQ_EN(chan_num)),
           DMA_IRQ_STAT(chan_num), (uint32_t)hal_readl(DMA_IRQ_STAT(chan_num)),
           DMA_GATE, (uint32_t)hal_readl(DMA_GATE),
           DMA_GATE_LEN, (uint32_t)hal_readl(DMA_GATE_LEN),
           DMA_CFG(chan_num), (uint32_t)hal_readl(DMA_CFG(chan_num)),
           DMA_CUR_SRC(chan_num), (uint32_t)hal_readl(DMA_CUR_SRC(chan_num)),
           DMA_CUR_DST(chan_num), (uint32_t)hal_readl(DMA_CUR_DST(chan_num)),
           DMA_CNT(chan_num), (uint32_t)hal_readl(DMA_CNT(chan_num)));
#endif
}
#endif /*__NDMA_SUN300IW1_H__  */
