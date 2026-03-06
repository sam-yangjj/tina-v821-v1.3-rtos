
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

#ifndef __SUNXI_HAL_PWM_H__
#define __SUNXI_HAL_PWM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_clk.h>
#include <hal_reset.h>
#include <hal_atomic.h>
#include <aw_list.h>
#include <sunxi_hal_common.h>
#include <pwm_ng/platform_pwm_ng.h>
#include <pwm_ng/common_pwm.h>
#ifdef CONFIG_DRIVERS_PWM_DEBUG
#define PWM_INFO(fmt, arg...) hal_log_info(fmt, ##arg)
#else
#define PWM_INFO(fmt, arg...) do {}while(0)
#endif

#define PWM_ERR(fmt, arg...) hal_log_err(fmt, ##arg)

#define PRESCALE_MAX 256


/*************
 *SET_BITS set
* **********/
#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	(((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	(((reg) & CLRMASK(width, shift)) | (val << (shift)))

/* define shift and width */
#define PWM_CLK_SRC_SHIFT 0x7
#define PWM_CLK_SRC_WIDTH 0x2

#define PWM_DIV_M_SHIFT 0x0
#define PWM_DIV_M_WIDTH 0x4

#define PWM_PRESCAL_SHIFT 0x0
#define PWM_PRESCAL_WIDTH 0x8

#define PWM_ACT_CYCLES_SHIFT 0x0
#define PWM_ACT_CYCLES_WIDTH 0x10

#define PWM_PERIOD_CYCLES_SHIFT 0x10
#define PWM_PERIOD_CYCLES_WIDTH 0x10

#define PWM_DZ_EN_SHIFT  0x0
#define PWM_DZ_EN_WIDTH  0x1
#define PWM_PDZINTV_SHIFT  0x8
#define PWM_PDZINTV_WIDTH  0x8

#define PWM_PULSE_SHIFT         0x9
#define PWM_PULSE_WIDTH         0x1

#define PWM_PULSE_NUM_SHIFT    0x10
#define PWM_PULSE_NUM_WIDTH    0x10

#define PWM_PULSE_START_SHIFT  0xa
#define PWM_PULSE_START_WIDTH  0x1

#define PWM_BIND_NUM 2

#define PWM_CAPTURE_CCSR	(0x1 << 0x5)
#define PWM_CAPTURE_CRLF	(0x1 << 0x4)
#define PWM_CAPTURE_CFLF	(0x1 << 0x3)
#define PWM_CAPTURE_CRTE	(0x1 << 0x2)
#define PWM_CAPTURE_CFTE	(0x1 << 0x1)

#define PWM_CAPTURE_CCCSRR	(0x1 << 1)
#define PWM_CAPTURE_CCCRFR	(0X1 << 0)


/*****************************************************************************
 * Enums
 *****************************************************************************/
typedef unsigned long pwm_status_t;
typedef void (*pwm_callback)(void* param);
typedef void (*pwm_cap_cb_t)(uint32_t channel, uint32_t period, uint32_t duty);
typedef void (*pwm_cap_count_cb_t)(uint32_t channel, uint32_t pulse_num);
typedef void (*pwm_pulse_cb_t)(uint32_t channel);

typedef enum {
	HAL_CPUX_PWM0 = 0,
	HAL_CPUX_PWM1 = 1,
	HAL_CPUS_PWM = 2,
	HAL_MCU_PWM = 3,
	HAL_PWM_MAX = PWM_PORT_NUM,
} hal_pwm_port_t;

typedef enum {
	HAL_PWM_STATUS_ERROR_PARAMETER = -3,
	HAL_PWM_STATUS_ERROR_CHANNEL = -2,
	HAL_PWM_STATUS_ERROR = -1,
	HAL_PWM_STATUS_OK = 0
} hal_pwm_status_t;

typedef enum
{
	PWM_CLK_OSC,
	PWM_CLK_APB,
} hal_pwm_clk_src;

typedef enum
{
	PWM_POLARITY_INVERSED = 0,
	PWM_POLARITY_NORMAL = 1,
} hal_pwm_polarity;

typedef enum
{
	PWM_CONTROL = 0,
	PWM_CHANNEL_INT = 1,
	PWM_CHANNEL_UNINT = 2,
} hal_pwm_cmd_t;

typedef struct pwm_config
{
	uint32_t        duty_ns;
	uint32_t        period_ns;
	bool		polarity;
	int		pulse_num;
	bool		output_mode;
} pwm_config_t;

static u32 hal_pwm_regs_offset[] = {
	PWM_PIER,
	PWM_CIER,
	PWM_PCCR01,
	PWM_PCCR23,
	PWM_PCCR45,
	PWM_PCCR67,
	PWM_PCGR,
	PWM_PDZCR01,
	PWM_PDZCR23,
	PWM_PDZCR45,
	PWM_PDZCR67,
	PWM_PER,
	PWM_CER,
	PWM_PCR,
	PWM_PPR,
	PWM_CCR,
	PWM_PCNTR,
};

typedef struct
{
	hal_clk_type_t pwm_clk_type;
	hal_clk_id_t pwm_bus_clk_id;
	hal_clk_t pwm_bus_clk;
	#if defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN55IW6) || defined(CONFIG_ARCH_SUN300IW1)
	hal_clk_id_t pwm_clk_id;
	hal_clk_t pwm_clk;
	#endif
	hal_reset_type_t pwm_reset_type;
	hal_reset_id_t pwm_reset_id;
	struct reset_control *pwm_reset;

	bool pin_state[PWM_CHAN_NUM];
	gpio_pin_t pin[PWM_CHAN_NUM];
	gpio_muxsel_t enable_muxsel[PWM_CHAN_NUM];
	u32 regs_backup[ARRAY_SIZE(hal_pwm_regs_offset)];
	int retrys;
	int port;
	int pwm_init;
	hal_spinlock_t lock;
	hal_spinlock_t en_lock;
} hal_pwm_t;

typedef struct
{
	uint32_t channel;
	uint32_t cnt;
	uint32_t period;
	uint32_t duty;
	uint32_t pulse_num;
	pwm_callback callback;
	uint32_t reserved;
	struct list_head node;
} hal_pwm_cap_info;

typedef struct
{
	uint32_t channel;
	uint32_t port;
	pwm_callback callback;
	void *data;
	uint32_t reserved;
	struct list_head node;
} hal_pwm_pulse_info;

pwm_status_t hal_pwm_init(hal_pwm_port_t port);
pwm_status_t hal_pwm_control(hal_pwm_port_t port, int channel, struct pwm_config *config_pwm);
pwm_status_t hal_pwm_release(hal_pwm_port_t port, int channel, struct pwm_config *config_pwm);
void hal_pwm_enable_controller(hal_pwm_port_t port, uint32_t channel_in);
void hal_pwm_disable_controller(hal_pwm_port_t port, uint32_t channel_in);
pwm_status_t hal_pwm_deinit(hal_pwm_port_t port);

int hal_pwm_resume(void *dev);
int hal_pwm_suspend(void *dev);

int hal_pwm_cap_enable(hal_pwm_port_t port, uint32_t channel, pwm_callback callback);
int hal_pwm_cap_disable(hal_pwm_port_t port, uint32_t channel);

int hal_pwm_cap_count_enable(hal_pwm_port_t port, uint32_t channel);
int hal_pwm_cap_count_disable(hal_pwm_port_t port, uint32_t channel);
uint32_t hal_pwm_cap_get_pulse_num(hal_pwm_port_t port, uint32_t channel_in);

void hal_pwm_pulse_irq_enable(hal_pwm_port_t port, uint32_t channel, pwm_callback callback, void *data);
int hal_pwm_pulse_irq_disable(hal_pwm_port_t port, uint32_t channel);

void hal_pwm_set_active_cycles(hal_pwm_port_t port, uint32_t channel_in, uint32_t active_cycles);
void hal_pwm_set_period_cycles(hal_pwm_port_t port, uint32_t channel_in, uint32_t period_cycles);
int hal_pwm_get_active_cycles(hal_pwm_port_t port, uint32_t channel_in);
int hal_pwm_get_period_cycles(hal_pwm_port_t port, uint32_t channel_in);

#ifdef __cplusplus
}
#endif

#endif /* __SUNXI_HAL_PWM_H__ */
