/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
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
/*
 * this is only for , adopt to other must change the m and n.
 */
#include "../platform-msgbox.h"
#include "hal_msgbox.h"
#include "hal_interrupt.h"
#ifdef CONFIG_ARCH_SUN20IW2
#ifdef CONFIG_ARCH_XTENXA /* only for dsp */
#include "aw_io.h"
#endif
#endif
#include "hal_clk.h"
#include "hal_reset.h"
#include "hal_mutex.h"
#include "hal_log.h"
#include <hal_waitqueue.h>
#ifdef CONFIG_STANDBY
#include <standby/standby.h>
#endif

#ifdef CONFIG_COMPONENTS_PM
#include "pm_devops.h"
#endif

static hal_spinlock_t spinlock;
hal_waitqueue_head_t msgbox_waitqueue;
static LIST_HEAD(msgbox_list_head);

#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
static void msgbox_enable_send_int(struct msg_endpoint *medp);
static void msgbox_disable_send_int(struct msg_endpoint *medp);
#endif

#define MSGBOX_MAX_QUEUE 8

enum msgbox_err {
	MSGBOX_OK = 0,
	MSGBOX_ERR = -1,
	MSGBOX_PM_ERR = -2,
};

static inline void *msgbox_base_get(int *m)
{
	void *b = 0;

	switch (*m) {
#ifdef CONFIG_ARCH_ARM_CORTEX_A55
	case MSGBOX_CORE0:
		b = (void *)MSGBOX_CORE0_BASE;
		break;
	case MSGBOX_CORE1:
		b = (void *)MSGBOX_CORE1_BASE;
		break;
	case MSGBOX_CORE2:
		b = (void *)MSGBOX_CORE2_BASE;
		break;
	case MSGBOX_CORE3:
		b = (void *)MSGBOX_CORE3_BASE;
		break;
	default:
		break;
#else
	case MSGBOX_ARM:
		b = (void *)MSGBOX_ARM_BASE;
		break;
	case MSGBOX_DSP:
		b = (void *)MSGBOX_DSP_BASE;
		break;
	case MSGBOX_RISCV:
		b = (void *)MSGBOX_RISCV_BASE;
		break;
	case MSGBOX_CPUS:
		b = (void *)MSGBOX_CPUS_BASE;
		break;
	default:
		break;
#endif
	}

	return b;
}

static inline void *MSGBOX_VER_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x10 + (n)*0x100);
}

static inline void *MSGBOX_RD_IRQ_EN_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x20 + (n)*0x100);
}

static inline void *MSGBOX_RD_IRQ_STA_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x24 + (n)*0x100);
}

static inline void *MSGBOX_WR_IRQ_EN_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x30 + (n)*0x100);
}

static inline void *MSGBOX_WR_IRQ_STA_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x34 + (n)*0x100);
}

static inline void *MSGBOX_DEBUG_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x40 + (n)*0x100);
}

static inline void *MSGBOX_FIFO_STA_REG(int m, int n, int p)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x50 + (n)*0x100 + (p)*0x4);
}

static inline void *MSGBOX_MSG_STA_REG(int m, int n, int p)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x60 + (n)*0x100 + (p)*0x4);
}

static inline void *MSGBOX_MSG_REG(int m, int n, int p)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x70 + (n)*0x100 + (p)*0x4);
}

static inline void *MSGBOX_WR_INIT_THRESHOLD_REG(int m, int n, int p)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x80 + (n)*0x100 + (p)*0x4);
}

static inline int calculte_n(int local, int remote)
{
#if defined(CONFIG_ARCH_SUN8IW20)|| defined(CONFIG_ARCH_SUN20IW1) \
               || defined(CONFIG_ARCH_SUN20IW3) || defined(CONFIG_ARCH_SUN55IW6)
#if defined CONFIG_ARCH_ARM_CORTEX_A55
/*
 *                 | remote core (send)   |
 *                 | ARM:0 | DSP:1 | RV:2 |
 * local  |  ARM:0 |   /   |   0   |  1   |
 * core   |  DSP:1 |   0   |   /   |  1   |
 * (recv) |  RV :2 |   0   |   1   |  /   |
*/
static int to_coef_n[4][4] = {
	{-1, 0, 1, 2},
	{0, -1, 1, 2},
	{0, 1, -1, 2},
	{0, 1, 2, -1}
};
#else
/*
 *                   | remote core (send)   |
 *                   | core0:0 | core1:1 | core3:2 | core4:3 |
 * local  |  core0:0 |    /    |    0    |    1    |    2    |
 * core   |  core1:1 |    0    |    /    |    1    |    2    |
 * (recv) |  core3:2 |    0    |    1    |    /    |    2    |
 *        |  core3:3 |    0    |    1    |    2    |    /    |
*/
static int to_coef_n[4][4] = {
	{-1, 0, 1, -1},
	{0, -1, 1, -1},
	{0, 1, -1, -1},
	{-1, -1, -1, -1}
};
#endif

	return to_coef_n[local][remote];
#elif defined(CONFIG_ARCH_SUN20IW2)
/*
 *                 | remote core (send)   |
 *                 | ARM:0 | DSP:1 | RV:2 |
 * local  |  ARM:0 |   /   |   1   |  0   |
 * core   |  DSP:1 |   0   |   /   |  1   |
 * (recv) |  RV :2 |   0   |   1   |  /   |
*/
static int to_coef_n[4][4] = {
	{-1, 1, 0, -1},
	{0, -1, 1, -1},
	{0, 1, -1, -1},
	{-1, -1, -1, -1},
};
	return to_coef_n[local][remote];

#elif defined(CONFIG_ARCH_SUN55IW3)
/*
 *                 | remote core (send)            |
 *                 | ARM:0 | DSP:1 | CPUS:2 | RV:3 |
 * local  | ARM:0  |   /   |   1   |  0     |  2   |
 * core   | DSP:1  |   0   |   /   |  1     |  2   |
 * (recv) | CPUS:2 |   0   |   1   |  /     |  2   |
 *	  | RV:3   |   2   |   1   |  0     |  /   |
*/
static int to_coef_n[4][4] = {
	{-1, 1, 0, 2},
	{0, -1, 1, 2},
	{0, 1, -1, 2},
	{2, 1, 0, -1}
};
	return to_coef_n[local][remote];

#elif defined(CONFIG_ARCH_SUN60IW1)
/*
 *                 | remote core (send)            |
 *                 | ARM:0 | CPUS:1 | DSP:2 | RV:3 |
 * local  | ARM:0  |   /   |   0   |  1     |  2   |
 * core   | CPUS:1 |   0   |   /   |  1     |  2   |
 * (recv) | DSP:2  |   0   |   1   |  /     |  2   |
 *	  | RV:3   |   0   |   1   |  2     |  /   |
*/
static int to_coef_n[4][4] = {
	{-1, 0, 1, 2},
	{0, -1, 1, 2},
	{0, 1, -1, 2},
	{0, 1, 2, -1}
};
	return to_coef_n[local][remote];

#elif defined(CONFIG_ARCH_SUN300IW1)

/*
 *					| remote core (send) |
 *					| RISCV:0 | ARM:1	 |
 * local  | RISCV:0	|	/	  |	   0	 |
 * core   | ARM:1   |	0	  |	   /	 |
 * (recv)
*/
static int to_coef_n[4][4] = {
	{-1, 0, -1, -1},
	{0, -1, -1, -1},
	{-1, -1, -1, -1},
	{-1, -1, -1, -1}
};
	return to_coef_n[local][remote];
#endif
}

static void sunxi_msgbox_read_handler(struct msg_endpoint *medp) {
	void *msg_sts, *msg_reg, *msg_irq_s;
	u32 data, flag;

	msg_sts = (void *)MSGBOX_MSG_STA_REG(
		medp->local_amp, calculte_n(medp->local_amp, medp->remote_amp),
		medp->read_ch);
	msg_reg = (void *)MSGBOX_MSG_REG(
		medp->local_amp, calculte_n(medp->local_amp, medp->remote_amp),
		medp->read_ch);
	msg_irq_s = (void *)MSGBOX_RD_IRQ_STA_REG(
		medp->local_amp, calculte_n(medp->local_amp, medp->remote_amp));

	while (hal_readl(msg_sts)) {
		data = hal_readl(msg_reg);
		if (medp->rec)
			medp->rec(data, medp->private);
	}

	flag = hal_spin_lock_irqsave(&spinlock);
	hal_writel(1 << (medp->read_ch * 2), msg_irq_s);
	hal_spin_unlock_irqrestore(&spinlock, flag);
}

#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
/* set the msgbox's fifo trigger level */
static void sunxi_msgbox_set_write_irq_threshold(struct msg_endpoint *medp, int n, int p,
							int threshold)
{
	u32 thr_val;
	u32 value;
	void *reg = MSGBOX_WR_INIT_THRESHOLD_REG(medp->remote_amp, n, p);

	switch (threshold) {
	case 8:
		thr_val = 3;
		break;
	case 4:
		thr_val = 2;
		break;
	case 2:
		thr_val = 1;
		break;
	case 1:
		thr_val = 0;
		break;
	default:
		printf("Invalid write irq threshold (%d). Use 1 instead\n", threshold);
		thr_val = 0;
		break;
	}

	value = hal_readl(reg);
	value &= ~(0x3);
	value |= thr_val;
	hal_writel(value, reg);
}


static void sunxi_msgbox_write_handler(struct msg_endpoint *medp)
{
	/*
	 * In msgbox hardware, the write IRQ will be triggered if the empty
	 * level in FIFO reaches the write IRQ threshold. It means that there
	 * is empty space in FIFO for local processor to write. Here we use
	 * the write IRQ to indicate TX is done, to ensure that there is empty
	 * space in FIFO for next message to send.
	 */
	u32 value;
	u32 flag;
	void *msg_w_pending_reg;

	flag = hal_spin_lock_irqsave(&spinlock);

	msgbox_disable_send_int(medp);

	/* Clear write IRQ pending */
	msg_w_pending_reg = (void *)MSGBOX_WR_IRQ_STA_REG(
		medp->remote_amp, calculte_n(medp->remote_amp, medp->local_amp));

	value = hal_readl(msg_w_pending_reg);
	value &= ~(1 << (medp->write_ch * 2 + 1));
	hal_writel(value, msg_w_pending_reg);

	hal_spin_unlock_irqrestore(&spinlock, flag);

	hal_wake_up(&msgbox_waitqueue);
}
#endif

static void irq_msgbox_channel_handler(struct msg_endpoint *medp)
{
	void *msg_r_irq_reg;
	void *msg_r_pending_reg;
	int read_irq_en, read_irq_pending;
#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	void *msg_w_irq_reg;
	void *msg_w_pending_reg;
	int write_irq_en, write_irq_pending;
#endif

	msg_r_irq_reg = (void *)MSGBOX_RD_IRQ_EN_REG(
		medp->local_amp, calculte_n(medp->local_amp, medp->remote_amp));

	msg_r_pending_reg = (void *)MSGBOX_RD_IRQ_STA_REG(
		medp->local_amp, calculte_n(medp->local_amp, medp->remote_amp));

	read_irq_en = hal_readl(msg_r_irq_reg) & (1 << (medp->read_ch * 2));
	read_irq_pending = hal_readl(msg_r_pending_reg) & (1 << (medp->read_ch * 2));

#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	msg_w_irq_reg = (void *)MSGBOX_WR_IRQ_EN_REG(
		medp->remote_amp, calculte_n(medp->remote_amp, medp->local_amp));

	msg_w_pending_reg = (void *)MSGBOX_WR_IRQ_STA_REG(
		medp->remote_amp, calculte_n(medp->remote_amp, medp->local_amp));

	write_irq_en = hal_readl(msg_w_irq_reg) & (1 << (medp->write_ch * 2 + 1));
	write_irq_pending = hal_readl(msg_w_pending_reg) & (1 << (medp->write_ch * 2 + 1));
#endif
	if (read_irq_en && read_irq_pending) {
		sunxi_msgbox_read_handler(medp);
	}

#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	if (write_irq_en && write_irq_pending) {
		sunxi_msgbox_write_handler(medp);
	}
#endif
}

static hal_irqreturn_t irq_msgbox_handler(void *p)
{
	struct msg_endpoint *t;
	struct list_head *entry;
	u32 flag;

	flag = hal_spin_lock_irqsave(&spinlock);
	/* shouled not use mutex in interrupt contex */
	list_for_each(entry, &msgbox_list_head) {
		t = list_entry(entry, struct msg_endpoint, list);
		irq_msgbox_channel_handler(t);
	}
	hal_spin_unlock_irqrestore(&spinlock, flag);

	return HAL_IRQ_OK;
}

#ifdef CONFIG_STANDBY
static void msgbox_enable_rec_int(struct msg_endpoint *medp);

static int msgbox_suspend(void *data)
{
	hal_log_debug("msgbox suspend\r\n");
	return 0;
}

static int msgbox_resume(void *data)
{
	struct reset_control *rst;
	hal_clk_t clk;
	struct msg_endpoint *t;
	struct list_head *entry;
	u32 flag;


	hal_log_debug("msgbox resume\r\n");
	rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_MSGBOX);
	hal_reset_control_deassert(rst);
	hal_reset_control_put(rst);

	clk = hal_clock_get(HAL_SUNXI_CCU, CLK_MSGBOX);
	hal_clock_enable(clk);
	hal_clock_put(clk);

	flag = hal_spin_lock_irqsave(&spinlock);
	/* add to global list */
	list_for_each(entry, &msgbox_list_head) {
		t = list_entry(entry, struct msg_endpoint, list);
		if (t->read_ch > 0) {
			msgbox_enable_rec_int(t);
		}
#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
		if (t->write_ch > 0) {
			msgbox_enable_send_int(t);
		}
#endif
	}
	hal_spin_unlock_irqrestore(&spinlock, flag);

	return 0;
}
#endif

#ifdef CONFIG_COMPONENTS_PM
static void msgbox_enable_rec_int(struct msg_endpoint *medp);
static int hal_msgbox_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	struct reset_control *rst;
	hal_clk_t clk;

	if (mode == PM_MODE_SLEEP)
		return 0;

	clk = hal_clock_get(HAL_SUNXI_CCU, CLK_MSGBOX);
	if (!clk)
		return MSGBOX_PM_ERR;

	hal_clock_disable(clk);
	hal_clock_put(clk);

	rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_MSGBOX);
	if (!rst)
		return MSGBOX_PM_ERR;

	hal_reset_control_assert(rst);
	hal_reset_control_put(rst);

	hal_disable_irq(IRQ_MSGBOX);
#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	hal_disable_irq(IRQ_WR_MSGBOX);
#endif

	return 0;
}

static int hal_msgbox_resume(struct pm_device *dev, suspend_mode_t mode)
{

	struct reset_control *rst;
	hal_clk_t clk;
	struct list_head *entry;
	struct msg_endpoint *t;
	u32 flag;

	if (mode == PM_MODE_SLEEP)
		return 0;

	rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_MSGBOX);
	if (!rst)
		return MSGBOX_PM_ERR;

	hal_reset_control_deassert(rst);
	hal_reset_control_put(rst);

	clk = hal_clock_get(HAL_SUNXI_CCU, CLK_MSGBOX);
	if (!clk)
		return MSGBOX_PM_ERR;

	hal_clock_enable(clk);
	hal_clock_put(clk);

	flag = hal_spin_lock_irqsave(&spinlock);;

	/* reinit all msgbox channel in resume */
	list_for_each(entry, &msgbox_list_head) {
		t = list_entry(entry, struct msg_endpoint, list);
		if (t->read_ch >= 0)
			msgbox_enable_rec_int(t);
#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
		if (t->write_ch >= 0)
			msgbox_enable_send_int(t);
#endif
	}
	hal_spin_unlock_irqrestore(&spinlock, flag);

	hal_enable_irq(IRQ_MSGBOX);
#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	hal_enable_irq(IRQ_WR_MSGBOX);
#endif

	return 0;
}

static struct pm_devops msgbox_devops = {
	.suspend = hal_msgbox_suspend,
	.resume = hal_msgbox_resume,
};

static struct pm_device msgbox_dev = {
	.name = "msgbox",
	.ops = &msgbox_devops,
};
#endif
uint32_t hal_msgbox_init(void)
{
#if (!defined CONFIG_FPGA_PLATFORM) && (!defined CONFIG_ARCH_ARM_CORTEX_A55)
	struct reset_control *rst;
	hal_clk_t clk;
#endif

	INIT_LIST_HEAD(&msgbox_list_head);
	hal_waitqueue_head_init(&msgbox_waitqueue);
#if defined (CONFIG_FPGA_PLATFORM)
	hal_writel(0, MBOX_BGR_REG);
	hal_writel(R_MBOX_RST | R_MBOX_GATINGA, MBOX_BGR_REG);
#elif defined (CONFIG_ARCH_ARM_CORTEX_A55)
	hal_writel(0, 0x02002764);
	hal_writel(R_MBOX_RST | R_MBOX_GATINGA, 0x02002764);
#else
	rst = hal_reset_control_get(RST_MSGBOX_TYPE, RST_MSGBOX);
	hal_reset_control_assert(rst);
	hal_reset_control_deassert(rst);
	hal_reset_control_put(rst);

	clk = hal_clock_get(CLK_MSGBOX_TYPE, CLK_MSGBOX);
	hal_clock_enable(clk);
	hal_clock_put(clk);
#endif

	hal_request_irq(IRQ_MSGBOX, irq_msgbox_handler, "msgbox", NULL);
#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	hal_request_irq(IRQ_WR_MSGBOX, irq_msgbox_handler, "msgbox_w", NULL);
#endif
	hal_enable_irq(IRQ_MSGBOX);
#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	hal_enable_irq(IRQ_WR_MSGBOX);
#endif

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_register(&msgbox_dev);
#endif

#ifdef CONFIG_STANDBY
	register_pm_dev_notify(msgbox_suspend, msgbox_resume, NULL);
#endif

	return 0;
}

static void msgbox_enable_rec_int(struct msg_endpoint *medp)
{
	void *msg_irq_e;

	msg_irq_e = (void *)MSGBOX_RD_IRQ_EN_REG(
		medp->local_amp, calculte_n(medp->local_amp, medp->remote_amp));

	hal_writel(hal_readl(msg_irq_e) | (1 << (medp->read_ch * 2)), msg_irq_e);
}

static void msgbox_disable_rec_int(struct msg_endpoint *medp)
{
	void *msg_irq_e;

	msg_irq_e = (void *)MSGBOX_RD_IRQ_EN_REG(
		medp->local_amp, calculte_n(medp->local_amp, medp->remote_amp));

	hal_writel(hal_readl(msg_irq_e) & ~(1 << (medp->read_ch * 2)), msg_irq_e);

}

#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
static void msgbox_enable_send_int(struct msg_endpoint *medp)
{
	void *msg_w_irq_e;

	msg_w_irq_e = (void *)MSGBOX_WR_IRQ_EN_REG(
		medp->remote_amp, calculte_n(medp->remote_amp, medp->local_amp));

	hal_writel(hal_readl(msg_w_irq_e) | (1 << (medp->write_ch * 2 + 1)), msg_w_irq_e);
}

static void msgbox_disable_send_int(struct msg_endpoint *medp)
{
	void *msg_w_irq_e;

	msg_w_irq_e = (void *)MSGBOX_WR_IRQ_EN_REG(
		medp->remote_amp, calculte_n(medp->remote_amp, medp->local_amp));

	hal_writel(hal_readl(msg_w_irq_e) & ~(1 << (medp->write_ch * 2 + 1)), msg_w_irq_e);
}
#endif

uint32_t hal_msgbox_alloc_channel(struct msg_endpoint *edp, int32_t remote,
			      int32_t read, int32_t write)
{
	u32 value, flag;
	void *msg_w_pending_reg;

	if (!edp) {
		printf("%s fail edp point is empty!!!\n", __func__);
		return -1;
	}

	edp->local_amp = THIS_MSGBOX_USE;
	edp->remote_amp = remote;
	edp->read_ch = read;
	edp->write_ch = write;

	flag = hal_spin_lock_irqsave(&spinlock);
	/* add to global list */
	list_add(&edp->list, &msgbox_list_head);

	/* Clear remote process's write IRQ pending */
	msg_w_pending_reg = (void *)MSGBOX_WR_IRQ_STA_REG(
		edp->remote_amp, calculte_n(edp->remote_amp, edp->local_amp));
	value = hal_readl(msg_w_pending_reg);
	value &= ~(1 << (edp->write_ch * 2 + 1));
	hal_writel(value, msg_w_pending_reg);


	if (read >= 0)
		msgbox_enable_rec_int(edp);
#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	if (write >= 0)
		msgbox_enable_send_int(edp);

	/*
	 * Configure the FIFO empty level to trigger the write IRQ to 1.
	 * It means that if the write IRQ is enabled, once the FIFO is not full,
	 * the write IRQ will be triggered.
	 */
	sunxi_msgbox_set_write_irq_threshold(edp, calculte_n(edp->remote_amp, edp->local_amp), edp->write_ch, 1);
#endif
	hal_spin_unlock_irqrestore(&spinlock, flag);

	return 0;
}

void hal_msgbox_free_channel(struct msg_endpoint *edp)
{
	struct msg_endpoint *t = NULL;
	struct list_head *entry;
	u32 flag;

	flag = hal_spin_lock_irqsave(&spinlock);

	list_for_each(entry, &msgbox_list_head) {
		t = list_entry(entry, struct msg_endpoint, list);
		if (t && t == edp)
			break;
	}

	if (t->read_ch >= 0) {
		msgbox_disable_rec_int(edp);
	}

#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	if (t->write_ch >= 0) {
		msgbox_disable_send_int(edp);
	}
#endif

	list_del(&t->list);
	t = NULL;

	hal_spin_unlock_irqrestore(&spinlock, flag);

	return ;
}

#define MAX_MSGBOX_TRY_SEND_CNT 10000000
int hal_msgbox_channel_send_data(struct msg_endpoint *medp, u32 data)
{
	void *msg_reg;
	void *sta_reg;
	u32 flag;

	msg_reg = (void *)MSGBOX_MSG_REG(
		medp->remote_amp, calculte_n(medp->remote_amp, medp->local_amp),
		medp->write_ch);

#if !defined CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	void *msg_sts;
	msg_sts = (void *)MSGBOX_MSG_STA_REG(
		medp->remote_amp, calculte_n(medp->remote_amp, medp->local_amp),
		medp->write_ch);

	uint32_t loop_cnt = 0;
	while (1) {
		if (hal_readl(msg_sts) != MSGBOX_MAX_QUEUE)
			break;

		loop_cnt++;
		if (loop_cnt == MAX_MSGBOX_TRY_SEND_CNT) {
			hal_log_err("%s(%d): message queue is full for a long time!!!\n", __func__, __LINE__);
			return -1;
		}
	}
#endif

	flag = hal_spin_lock_irqsave(&spinlock);
#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	msgbox_enable_send_int(medp);
#endif
	hal_spin_unlock_irqrestore(&spinlock, flag);

	sta_reg = (void *)MSGBOX_MSG_STA_REG(
		medp->remote_amp, calculte_n(medp->remote_amp, medp->local_amp),
		medp->write_ch);
	if (hal_readl(sta_reg) < MSGBOX_MAX_QUEUE) {
		hal_writel(data, msg_reg);
	} else {
		printf("write fifo is full\n");
		return -1;
	}

#ifdef CONFIG_DRIVERS_MSGBOX_SUPPORT_TXDONE_IRQ
	if (hal_wait_event_timeout(msgbox_waitqueue, true, 2000) <= 0) {
		printf("wait write remote fifo timeout!\n");
		return -1;
	}
#endif

	return 0;
}

u32 hal_msgbox_channel_send(struct msg_endpoint *medp, uint8_t *bf,
			    uint32_t len)
{
	u32 data, i;

	data = 0;

	for (i = 0; i < len; i++) {

		if (!(i % 4))
			data = 0;

		data |= *bf++ << ((i % 4) << 3);

		if ((i % 4) == 3 || i == len - 1) {
			if (hal_msgbox_channel_send_data(medp, data))
				return -1;
		}
	}

	return 0;
}
