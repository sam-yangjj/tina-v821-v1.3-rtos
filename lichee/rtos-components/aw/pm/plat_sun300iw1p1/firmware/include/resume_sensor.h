#ifndef __RESUME_SENSOR_H__
#define __RESUME_SENSOR_H__

/* gpio data structure */
#define GPIO_BANK(pin)		((pin) >> 5)
#define GPIO_CFG_INDEX(pin)	(((pin) & 0x1f) >> 3)
#define GPIO_CFG_OFFSET(pin)	((((pin) & 0x1f) & 0x7) << 2)
#define SUNXI_GPIO_BANKS 	9
#define SUNXI_GPIO_L		11
#define SUNXI_GPIO_BASE		0x42000000L
#define SUNXI_PIO_BASE		(SUNXI_GPIO_BASE)
#define SUNXI_R_PIO_BASE	(0x42000540)

struct sunxi_gpio {
	uint32_t cfg[4];
	uint32_t dat;
	uint32_t drv[2];
	uint32_t pull[2];
};

struct sunxi_gpio_int {
	uint32_t cfg[3];
	uint32_t ctl;
	uint32_t sta;
	uint32_t deb;		/* interrupt debounce */
};

struct sunxi_gpio_reg {
	struct sunxi_gpio gpio_bank[SUNXI_GPIO_BANKS];
	uint8_t res[0xbc];
	struct sunxi_gpio_int gpio_int;
};

#define BANK_TO_GPIO(bank)	(((bank) < SUNXI_GPIO_L) ? \
	&((struct sunxi_gpio_reg *)SUNXI_PIO_BASE)->gpio_bank[bank] : \
	&((struct sunxi_gpio_reg *)SUNXI_R_PIO_BASE)->gpio_bank[(bank) - SUNXI_GPIO_L])

enum sunxi_gpio_number {
	SUNXI_GPIO_A_START = 0,
};

#define SUNXI_GPA(_nr)	(SUNXI_GPIO_A_START + (_nr))

/* twi data structure */
#define BUS_Reset0_REG 				0x00000090 //CCU_APP Bus Reset0 Register
#define BUS_CLK_GATING0_REG_TWI0_PCLK_EN_OFFSET 14
#define BUS_Reset0_REG_PRESETN_TWI0_SW_OFFSET 	14
#define SUNXI_CCU_APP_BASE            	 	0x42001000L
#define SUNXI_CCM_APP_BASE       		(SUNXI_CCU_APP_BASE)

#define SUNXI_TWI0_BASE                	0x42502000L
#define TWI_NR_MASTER			(1)
#define SUNXI_TWI0_GATE_BASE		(SUNXI_CCM_APP_BASE + BUS_CLK_GATING0_REG)
#define SUNXI_TWI0_GATE_BIT		(BUS_CLK_GATING0_REG_TWI0_PCLK_EN_OFFSET)
#define SUNXI_TWI0_RST_BASE		(SUNXI_CCM_APP_BASE + BUS_Reset0_REG)
#define SUNXI_TWI0_RST_BIT		(BUS_Reset0_REG_PRESETN_TWI0_SW_OFFSET)

#define TWI_CTL_ACK     (0x1<<2)
#define TWI_CTL_INTFLG  (0x1<<3)
#define TWI_CTL_STP     (0x1<<4)
#define TWI_CTL_STA     (0x1<<5)
#define TWI_CTL_BUSEN   (0x1<<6)
#define TWI_CTL_INTEN   (0x1<<7)
#define TWI_LCR_WMASK   (TWI_CTL_STA|TWI_CTL_STP|TWI_CTL_INTFLG)

struct sunxi_twi_reg {
	volatile uint32_t addr;        /* slave address     */
	volatile uint32_t xaddr;       /* extend address    */
	volatile uint32_t data;        /* data              */
	volatile uint32_t ctl;         /* control           */
	volatile uint32_t status;      /* status            */
	volatile uint32_t clk;         /* clock             */
	volatile uint32_t srst;        /* soft reset        */
	volatile uint32_t eft;         /* enhanced future   */
	volatile uint32_t lcr;         /* line control      */
	volatile uint32_t dvfs;        /* dvfs control      */
};

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

/* Register read and write operations */
#define REG_DLY             0xffff

/* vin data structure */
typedef unsigned short addr_type;
typedef unsigned char data_type;

struct regval_list {
	addr_type addr;
	data_type data;
};

struct twi_gpio_info {
	uint32_t pin[2];
	unsigned char pin_func[2];
};

#ifdef CONFIG_SENSOR_GC1084_MIPI
static struct regval_list sensor_standby_off_regs[] = {
	   {0x03fe, 0x10}, /* sensor default drop one frame */
	   {0x03f9, 0x21},
	   {0x03f3, 0x00},
	   {0x03fc, 0xae},
	   {0x031d, 0x2d},
	   {0x023e, 0x98},
	   {0x031d, 0x28},
	   {0x0187, 0x51}, /* sensor default drop one frame */
	   {0x03fe, 0x00}, /* sensor default drop one frame */
};

struct twi_gpio_info twi_gpio = {
	.pin = {
		SUNXI_GPA(3), SUNXI_GPA(4),
	},
	.pin_func = {0x4, 0xf},
};

#define SENSOR_TWI_SPEED	400000
#define SENSOR_TWI_BASE		0x42502000L
#define SENSOR_MCLK		(27*1000*1000)
#define SENSOR_TWI_ADDR		0x6e
#else
#error "only support gc1084"
#endif

#endif
