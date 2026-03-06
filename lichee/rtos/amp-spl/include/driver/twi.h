/*
 * Copyright 2014 - Hans de Goede <hdegoede@redhat.com>
 */
#ifndef _SUNXI_I2C_H_
#define _SUNXI_I2C_H_

#include <include.h>

/* TWI Control Register Bit Fields & Masks, default value: 0x0000_0000*/
#define TWI_CTL_ACK     (0x1<<2)
#define TWI_CTL_INTFLG  (0x1<<3)
#define TWI_CTL_STP     (0x1<<4)
#define TWI_CTL_STA     (0x1<<5)
#define TWI_CTL_BUSEN   (0x1<<6)
#define TWI_CTL_INTEN   (0x1<<7)
#define TWI_LCR_WMASK   (TWI_CTL_STA|TWI_CTL_STP|TWI_CTL_INTFLG)

struct sunxi_twi_reg {
	volatile u32 addr;        /* slave address     */
	volatile u32 xaddr;       /* extend address    */
	volatile u32 data;        /* data              */
	volatile u32 ctl;         /* control           */
	volatile u32 status;      /* status            */
	volatile u32 clk;         /* clock             */
	volatile u32 srst;        /* soft reset        */
	volatile u32 eft;         /* enhanced future   */
	volatile u32 lcr;         /* line control      */
	volatile u32 dvfs;        /* dvfs control      */
};

#ifdef CONFIG_TWI_DEBUG
#define twi_info(fmt...) printf("[twi][info]: " fmt)
#define twi_err(fmt...) printf("[twi][err]: " fmt)
#else
#define twi_info(fmt...)
#define twi_err(fmt...) printf("[twi][err]: " fmt)
#endif

void twi_init(unsigned long twi_base, int speed);
void twi_exit(unsigned long twi_base);
int twi_read(unsigned long twi_base, u8 chip, u32 addr, int alen, u8 *buffer, int len);
int twi_write(unsigned long twi_base, u8 chip, u32 addr, int alen, u8 *buffer, int len);

#endif
