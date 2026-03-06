/*
 * (C) Copyright 2016
 *Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *weidonghui <weidonghui@allwinnertech.com>
 *
 */
#include <include.h>

#define TWI_WRITE 0
#define TWI_READ 1

#define TWI_OK 0
#define TWI_NOK 1
#define TWI_NACK 2
#define TWI_NOK_LA 3 /* Lost arbitration */
#define TWI_NOK_TOUT 4 /* time out */

#define TWI_START_TRANSMIT 0x08
#define TWI_RESTART_TRANSMIT 0x10
#define TWI_ADDRWRITE_ACK 0x18
#define TWI_ADDRREAD_ACK 0x40
#define TWI_DATAWRITE_ACK 0x28
#define TWI_READY 0xf8
#define TWI_DATAREAD_NACK 0x58
#define TWI_DATAREAD_ACK 0x50

/* status or interrupt source */
/*------------------------------------------------------------------------------
 * Code   Status
 * 00h	  Bus error
 * 08h	  START condition transmitted
 * 10h	  Repeated START condition transmitted
 * 18h	  Address + Write bit transmitted, ACK received
 * 20h	  Address + Write bit transmitted, ACK not received
 * 28h	  Data byte transmitted in master mode, ACK received
 * 30h	  Data byte transmitted in master mode, ACK not received
 * 38h	  Arbitration lost in address or data byte
 * 40h	  Address + Read bit transmitted, ACK received
 * 48h	  Address + Read bit transmitted, ACK not received
 * 50h	  Data byte received in master mode, ACK transmitted
 * 58h	  Data byte received in master mode, not ACK transmitted
 * 60h	  Slave address + Write bit received, ACK transmitted
 * 68h	  Arbitration lost in address as master, slave address + Write bit received, ACK transmitted
 * 70h	  General Call address received, ACK transmitted
 * 78h	  Arbitration lost in address as master, General Call address received, ACK transmitted
 * 80h	  Data byte received after slave address received, ACK transmitted
 * 88h	  Data byte received after slave address received, not ACK transmitted
 * 90h	  Data byte received after General Call received, ACK transmitted
 * 98h	  Data byte received after General Call received, not ACK transmitted
 * A0h	  STOP or repeated START condition received in slave mode
 * A8h	  Slave address + Read bit received, ACK transmitted
 * B0h	  Arbitration lost in address as master, slave address + Read bit received, ACK transmitted
 * B8h	  Data byte transmitted in slave mode, ACK received
 * C0h	  Data byte transmitted in slave mode, ACK not received
 * C8h	  Last byte transmitted in slave mode, ACK received
 * D0h	  Second Address byte + Write bit transmitted, ACK received
 * D8h	  Second Address byte + Write bit transmitted, ACK not received
 * F8h	  No relevant status information or no interrupt
 *-----------------------------------------------------------------------------*/

static void twi_debug(struct sunxi_twi_reg *twi)
{
	twi_info("twi->addr  :\t0x%p:0x%x\n", &twi->addr, twi->addr);
	twi_info("twi->xaddr :\t0x%p:0x%x\n", &twi->xaddr, twi->xaddr);
	twi_info("twi->data  :\t0x%p:0x%x\n", &twi->data, twi->data);
	twi_info("twi->ctl   :\t0x%p:0x%x\n", &twi->ctl, twi->ctl);
	twi_info("twi->status:\t0x%p:0x%x\n", &twi->status, twi->status);
	twi_info("twi->clk   :\t0x%p:0x%x\n", &twi->clk, twi->clk);
	twi_info("twi->srst  :\t0x%p:0x%x\n", &twi->srst, twi->srst);
	twi_info("twi->eft   :\t0x%p:0x%x\n", &twi->eft, twi->eft);
	twi_info("twi->lcr   :\t0x%p:0x%x\n", &twi->lcr, twi->lcr);
	twi_info("twi->dvfs  :\t0x%p:0x%x\n", &twi->dvfs, twi->dvfs);
}

static s32 twi_sendbyteaddr(struct sunxi_twi_reg *twi, u32 byteaddr)
{
	s32 time = 0xffff;
	u32 tmp_val;

	twi->data = byteaddr & 0xff;
	twi->ctl |= (0x01 << 3); /*write 1 to clean int flag*/

	while ((time--) && (!(twi->ctl & 0x08)))
		;
	if (time <= 0)
		return -TWI_NOK_TOUT;

	tmp_val = twi->status;
	twi_debug(twi);
	if (tmp_val != TWI_DATAWRITE_ACK)
		return -TWI_DATAWRITE_ACK;

	return TWI_OK;
}

static s32 twi_sendstart(struct sunxi_twi_reg *twi)
{
	s32 time = 0xffff;
	u32 tmp_val;

	twi->eft  = 0;
	twi->srst = 1;
	twi->ctl  |= TWI_CTL_STA;

	while ((time--) && (!(twi->ctl & TWI_CTL_INTFLG)))
		;
	if (time <= 0)
		return -TWI_NOK_TOUT;

	tmp_val = twi->status;
	if (tmp_val != TWI_START_TRANSMIT)
		return -TWI_START_TRANSMIT;

	return TWI_OK;
}

static s32 twi_sendslaveaddr(struct sunxi_twi_reg *twi, u32 saddr, u32 rw)
{
	s32 time = 0xffff;
	u32 tmp_val;

	rw &= 1;
	twi->data = ((saddr & 0xff) << 1) | rw;
	twi->ctl |= TWI_CTL_INTFLG; /*write 1 to clean int flag*/
	while ((time--) && (!(twi->ctl & TWI_CTL_INTFLG)))
		;
	if (time <= 0)
		return -TWI_NOK_TOUT;

	tmp_val = twi->status;
	if (rw == TWI_WRITE) { /*+write*/
		if (tmp_val != TWI_ADDRWRITE_ACK)
			return -TWI_ADDRWRITE_ACK;
	} else { /*+read*/
		if (tmp_val != TWI_ADDRREAD_ACK)
			return -TWI_ADDRREAD_ACK;
	}
	twi_debug(twi);

	return TWI_OK;
}

static s32 twi_sendRestart(struct sunxi_twi_reg *twi)
{
	s32 time = 0xffff;
	u32 tmp_val;
	tmp_val = twi->ctl;

	tmp_val |= 0x20;
	twi->ctl = tmp_val;

	while ((time--) && (!(twi->ctl & 0x08)))
		;
	if (time <= 0)
		return -TWI_NOK_TOUT;

	tmp_val = twi->status;
	if (tmp_val != TWI_RESTART_TRANSMIT)
		return -TWI_RESTART_TRANSMIT;

	return TWI_OK;
}

static s32 twi_stop(struct sunxi_twi_reg *twi)
{
	s32 time = 0xffff;
	u32 tmp_val;
	twi->ctl |= (0x01 << 4);
	twi->ctl |= (0x01 << 3);
	while ((time--) && (twi->ctl & 0x10))
		;
	if (time <= 0)
		return -TWI_NOK_TOUT;

	time = 0xffff;
	while ((time--) && (twi->status != TWI_READY))
		;
	tmp_val = twi->status;
	if (tmp_val != TWI_READY)
		return -TWI_NOK_TOUT;

	return TWI_OK;
}

static s32 twi_getdata(struct sunxi_twi_reg *twi, u8 *data_addr, u32 data_count)
{
	s32 time = 0xffff;
	u32 tmp_val;
	u32 i;

	if (data_count == 1) {
		twi->ctl |= (0x01 << 3);
		while ((time--) && (!(twi->ctl & 0x08)))
			;
		if (time <= 0)
			return -TWI_NOK_TOUT;

		for (time = 0; time < 100; time++)
			;
		*data_addr = twi->data;

		tmp_val = twi->status;
		if (tmp_val != TWI_DATAREAD_NACK)
			return -TWI_DATAREAD_NACK;
	} else {
		for (i = 0; i < data_count - 1; i++) {
			time = 0xffff;
			/*host should send ack every time when a data packet finished*/
			tmp_val = twi->ctl | (0x01 << 2);
			tmp_val = twi->ctl | (0x01 << 3);
			tmp_val |= 0x04;
			twi->ctl = tmp_val;
			/*twi->ctl |=(0x01<<3);*/

			while ((time--) && (!(twi->ctl & 0x08)))
				;
			if (time <= 0)
				return -TWI_NOK_TOUT;

			for (time = 0; time < 100; time++)
				;
			time	 = 0xffff;
			data_addr[i] = twi->data;
			while ((time--) && (twi->status != TWI_DATAREAD_ACK))
				;
			if (time <= 0)
				return -TWI_NOK_TOUT;
		}

		time = 0xffff;
		twi->ctl &= 0xFb; /*the last data packet,not send ack*/
		twi->ctl |= (0x01 << 3);
		while ((time--) && (!(twi->ctl & 0x08)))
			;
		if (time <= 0)
			return -TWI_NOK_TOUT;

		for (time = 0; time < 100; time++)
			;
		data_addr[data_count - 1] = twi->data;
		while ((time--) && (twi->status != TWI_DATAREAD_NACK))
			;
		if (time <= 0)
			return -TWI_NOK_TOUT;
	}

	return TWI_OK;
}

int twi_read(unsigned long twi_base, u8 chip, u32 addr, int alen, u8 *buffer, int len)
{
	int i, ret, addrlen;
	char *slave_reg;
	struct sunxi_twi_reg *twi = (struct sunxi_twi_reg *)twi_base;

	ret  = twi_sendstart(twi);
	if (ret)
		goto twi_read_err_occur;

	ret = twi_sendslaveaddr(twi, chip, TWI_WRITE);
	if (ret)
		goto twi_read_err_occur;

	/*send byte address*/
	if (alen >= 3) {
		addrlen = 2;
	} else if (alen <= 1) {
		addrlen = 0;
	} else {
		addrlen = 1;
	}
	slave_reg = (char *)&addr;

	for (i = addrlen; i >= 0; i--) {
		ret = twi_sendbyteaddr(twi, slave_reg[i] & 0xff);
		if (ret)
			goto twi_read_err_occur;
	}

	ret = twi_sendRestart(twi);
	if (ret)
		goto twi_read_err_occur;

	ret = twi_sendslaveaddr(twi, chip, TWI_READ);
	if (ret)
		goto twi_read_err_occur;

	/*get data*/
	ret = twi_getdata(twi, buffer, len);
	if (ret)
		goto twi_read_err_occur;

twi_read_err_occur:
	twi_stop(twi);
#ifdef CONFIG_TWI_DEBUG
	twi_err("%s: ret = %d\n", __func__, ret);
#endif

	return ret;
}

static s32 twi_senddata(struct sunxi_twi_reg *twi, u8 *data_addr, u32 data_count)
{
	s32 time = 0xffff;
	u32 i;

	for (i = 0; i < data_count; i++) {
		time      = 0xffff;
		twi->data = data_addr[i];
		twi->ctl |= (0x01 << 3);
		while ((time--) && (!(twi->ctl & 0x08)))
			;
		if (time <= 0) {
			return -TWI_NOK_TOUT;
		}
		time = 0xffff;
		while ((time--) && (twi->status != TWI_DATAWRITE_ACK)) {
			;
		}
		if (time <= 0) {
			return -TWI_NOK_TOUT;
		}
	}

	return TWI_OK;
}

int twi_write(unsigned long twi_base, u8 chip, u32 addr, int alen, u8 *buffer, int len)
{
	int i, ret, ret0, addrlen;
	char *slave_reg;
	struct sunxi_twi_reg *twi = (struct sunxi_twi_reg *)twi_base;

	ret0 = -1;
	ret  = twi_sendstart(twi);
	if (ret)
		goto twi_write_err_occur;

	ret = twi_sendslaveaddr(twi, chip, TWI_WRITE);
	if (ret)
		goto twi_write_err_occur;
	/*send byte address*/

	if (alen >= 3) {
		addrlen = 2;
	} else if (alen <= 1) {
		addrlen = 0;
	} else {
		addrlen = 1;
	}

	slave_reg = (char *)&addr;
	for (i = addrlen; i >= 0; i--) {
		ret = twi_sendbyteaddr(twi, slave_reg[i] & 0xff);
		if (ret)
			goto twi_write_err_occur;
	}

	ret = twi_senddata(twi, buffer, len);
	if (ret)
		goto twi_write_err_occur;
	ret0 = 0;

twi_write_err_occur:
	twi_stop(twi);
#ifdef CONFIG_TWI_DEBUG
	twi_err("%s: ret = %d\n", __func__, ret);
#endif

	return ret0;
}

static void twi_set_clock(struct sunxi_twi_reg *twi, int speed)
{
	int i;
	u32 clk_m = 0, clk_n = 0, _2_pow_clk_n = 1, duty = 0, src_clk = 0;
	u32 divider, sclk_real;
	/* reset twi control  */
	i  = 0xffff;
	twi->srst = 1;
	while ((twi->srst) && (i))
		i--;

	if ((twi->lcr & 0x30) != 0x30) {
		/* toggle TWI SCL and SDA until bus idle */
		twi->lcr = 0x05;
		udelay(500);
		i = 10;
		while ((i > 0) && ((twi->lcr & 0x02) != 2)) {
			/*control scl and sda output high level*/
			twi->lcr |= 0x08;
			twi->lcr |= 0x02;
			udelay(1000);
			/*control scl and sda output low level*/
			twi->lcr &= ~0x08;
			twi->lcr &= ~0x02;
			udelay(1000);
			i--;
		}
		twi->lcr = 0x0;
		udelay(500);
	}

	src_clk = 192000000; /* AW1882 192MHz */
	divider = src_clk / speed; /* 400kHz or 100kHz */
	sclk_real = 0; /* the real clock frequency */

	/* Search for clk_n and clk_m values */
	if (divider == 0) {
		clk_m = 1;
		goto set_clk;
	}

	/* 3 bits max value is 8 */
	while (clk_n < 8) {
		/* (m+1)*2^n = divider --> m = divider/2^n - 1 */
		clk_m = (divider / _2_pow_clk_n) - 1;
		/* 4 bits max value is 16 */
		while (clk_m < 16) {
			/* Calculate real clock frequency */
			sclk_real = src_clk / (clk_m + 1) / _2_pow_clk_n;
			if (sclk_real <= speed) {
				goto set_clk;
			} else {
				clk_m++;
			}
		}
		clk_n++;
		_2_pow_clk_n *= 2; /* Multiple by 2 */
	}

	set_clk:
	twi->clk &= ~((0xf << 3) | (0x7 << 0));
	twi->clk |= ((clk_m << 3) | clk_n);
	if (speed == 400000) {
		duty = (0x1 << 8);
		twi->clk |= duty;
	} else {
		duty = (0x1 << 7);
		twi->clk &= ~(duty);
	}
	twi->ctl |= 0x40;
	twi->eft = 0;
	twi_debug(twi);
}

static void sunxi_twi_bus_setting(u32 twi_base, int onoff)
{
	int reg_value = 0;
	u32 bus_num = (twi_base - SUNXI_TWI0_BASE)/0x400;
	unsigned long gate_base, gate_bit;
	unsigned long rst_base, rst_bit;

#define INIT_TWI_REG(bus, n)	\
		if (bus == n) { \
			gate_base = SUNXI_TWI##n##_GATE_BASE; \
			gate_bit  = SUNXI_TWI##n##_GATE_BIT; \
			rst_base  = SUNXI_TWI##n##_RST_BASE; \
			rst_bit   = SUNXI_TWI##n##_RST_BIT; \
		} else

#define INIT_TWI_REG_END()	\
	{ \
		printk("invalid twi bus_num %d, range[0, %d]\r\n", bus_num, TWI_NR_MASTER); \
		return; \
	}

	if (bus_num < TWI_NR_MASTER) {
#if TWI_NR_MASTER > 0
		INIT_TWI_REG(bus_num, 0)
#endif
#if TWI_NR_MASTER > 1
		INIT_TWI_REG(bus_num, 1)
#endif
#if TWI_NR_MASTER > 2
		INIT_TWI_REG(bus_num, 2)
#endif
#if TWI_NR_MASTER > 3
		INIT_TWI_REG(bus_num, 3)
#endif
#if TWI_NR_MASTER > 4
		INIT_TWI_REG(bus_num, 4)
#endif
#if TWI_NR_MASTER > 5
		INIT_TWI_REG(bus_num, 5)
#endif
#if TWI_NR_MASTER > 6
		INIT_TWI_REG(bus_num, 6)
#endif
		INIT_TWI_REG_END();
	} else {
		printk("invalid twi bus_num %d, range[0, %d]\r\n", bus_num, TWI_NR_MASTER);
		return;
	}

	if (onoff) {
		//de-assert
		reg_value = readl(rst_base);
		reg_value |= (1 << rst_bit);
		writel(reg_value, rst_base);

		//gating clock pass
		reg_value = readl(gate_base);
		reg_value &= ~(1 << gate_bit);
		writel(reg_value, gate_base);
		udelay(1000);
		reg_value |= (1 << gate_bit);
		writel(reg_value, gate_base);
	} else {
		//gating clock mask
		reg_value = readl(gate_base);
		reg_value &= ~(1 << gate_bit);
		writel(reg_value, gate_base);

		//assert
		reg_value = readl(rst_base);
		reg_value &= ~(1 << gate_bit);
		writel(reg_value, rst_base);
	}
}

void twi_init(unsigned long twi_base, int speed)
{
	struct sunxi_twi_reg *twi = (struct sunxi_twi_reg *)twi_base;
	sunxi_twi_bus_setting(twi_base, 1);
	twi_set_clock(twi, speed);
}

void twi_exit(unsigned long twi_base)
{
	sunxi_twi_bus_setting(twi_base, 0);
}
