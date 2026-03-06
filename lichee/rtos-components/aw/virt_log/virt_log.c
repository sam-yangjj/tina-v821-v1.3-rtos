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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sunxi_hal_common.h>
#include <hal_interrupt.h>
#include <hal_cmd.h>
#include <hal_mem.h>

#define VIRT_LOG_SIZE				(CONFIG_COMPONENTS_VIRT_LOG_SIZE * 1024)

static uint32_t virt_log_offline(void);
#ifdef CONFIG_PM_STANDBY_MEMORY
/* pm standby may drop the data when suspend. */
static char *virt_log_buffer = NULL;
#else
static char virt_log_buffer[VIRT_LOG_SIZE];
#endif
static uint32_t log_offset = 0;
static uint8_t log_enable = 1;
static uint8_t virt_log_output_enable = 0;
static uint32_t output_dirty_offset = 0;
static uint32_t buffer_full_cnt = 0;
static uint32_t buffer_full_cnt_save = 0;

void virt_log_put(char ch)
{
	unsigned long flags;

	if (virt_log_offline())
		return;

	/* currently only the sigle-core case is considered */
	flags = hal_interrupt_disable_irqsave();
	if (!log_enable) {
		hal_interrupt_enable_irqrestore(flags);
		return;
	}

	virt_log_buffer[log_offset] = ch;
	log_offset++;
	if (log_offset >= VIRT_LOG_SIZE) {
		log_offset = 0;
		buffer_full_cnt++;
	}
	hal_interrupt_enable_irqrestore(flags);
}

int virt_log_put_buf(char *buf)
{
	int len, remain, writed;
	unsigned long flags;

	if (virt_log_offline())
		return 0;

	flags = hal_interrupt_disable_irqsave();
	if (!log_enable) {
		hal_interrupt_enable_irqrestore(flags);
		return 0;
	}

	len = strlen(buf);
	if (len > VIRT_LOG_SIZE) {
		buf += (len - VIRT_LOG_SIZE);
		len -= (len - VIRT_LOG_SIZE);
	}
	writed = len;

	/* currently only the sigle-core case is considered */
	remain = VIRT_LOG_SIZE - log_offset;
	if (len > remain) {
		memcpy(&virt_log_buffer[log_offset], buf, remain);
		log_offset = 0;
		len -= remain;
		buf += remain;
	}
	memcpy(&virt_log_buffer[log_offset], buf, len);
	log_offset += len;
	if (log_offset >= VIRT_LOG_SIZE)
		log_offset = 0;
	hal_interrupt_enable_irqrestore(flags);

	return writed;
}

int virt_log_put_buf_len(char *buf, int len)
{
	int remain, writed;
	unsigned long flags;

	if (virt_log_offline())
		return 0;

	flags = hal_interrupt_disable_irqsave();

	if (!log_enable) {
		hal_interrupt_enable_irqrestore(flags);
		return 0;
	}

	if (len > VIRT_LOG_SIZE) {
		buf += (len - VIRT_LOG_SIZE);
		len -= (len - VIRT_LOG_SIZE);
	}
	writed = len;

	/* currently only the sigle-core case is considered */
	remain = VIRT_LOG_SIZE - log_offset;
	if (len > remain) {
		memcpy(&virt_log_buffer[log_offset], buf, remain);
		log_offset = 0;
		buffer_full_cnt++;
		len -= remain;
		buf += remain;
	}
	memcpy(&virt_log_buffer[log_offset], buf, len);
	log_offset += len;
	if (log_offset >= VIRT_LOG_SIZE) {
		log_offset = 0;
		buffer_full_cnt++;
	}
	hal_interrupt_enable_irqrestore(flags);

	return writed;
}

int virt_log_flush(void)
{
	uint32_t offset, i;
	unsigned long flags;

	if (virt_log_offline())
		return -ENODEV;

	flags = hal_interrupt_disable_irqsave();
	log_enable = 0;
	offset = log_offset;

	if (buffer_full_cnt == buffer_full_cnt_save) {
		for (i = output_dirty_offset; i < offset; i++)
			putchar(virt_log_buffer[i]);
	} else if ((buffer_full_cnt - buffer_full_cnt_save) > 1) {
		for (i = offset; i < VIRT_LOG_SIZE; i++)
			putchar(virt_log_buffer[i]);
		for (i = 0; i < offset; i++)
			putchar(virt_log_buffer[i]);
	} else {
		if (output_dirty_offset >= offset) {
			for (i = output_dirty_offset; i < VIRT_LOG_SIZE; i++)
				putchar(virt_log_buffer[i]);
			for (i = 0; i < offset; i++)
				putchar(virt_log_buffer[i]);
		} else {
			for (i = offset; i < VIRT_LOG_SIZE; i++)
				putchar(virt_log_buffer[i]);
			for (i = 0; i < offset; i++)
				putchar(virt_log_buffer[i]);
		}
	}

	output_dirty_offset = offset;
	buffer_full_cnt_save = buffer_full_cnt;
	log_enable = 1;
	hal_interrupt_enable_irqrestore(flags);

	return 0;
}

int virt_log_is_enable(void)
{
    return virt_log_output_enable;
}

void virt_log_enable(int enable)
{
	unsigned long flags;

	if (virt_log_offline())
		return;

	flags = hal_interrupt_disable_irqsave();
	virt_log_output_enable = enable;
	if (enable == 0)
		virt_log_flush();
	hal_interrupt_enable_irqrestore(flags);
}

#ifdef CONFIG_COMMAND_DMESG
static int cmd_dmesg(int argc, const char **argv)
{
	virt_log_flush();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_dmesg, dmesg, dump log info);
#endif

#ifdef CONFIG_PM_STANDBY_MEMORY
static uint32_t virt_log_offline(void)
{
	if (virt_log_buffer == NULL)
		return 1;

	return 0;
}

int virt_log_init(void)
{
	if (virt_log_buffer != NULL) {
		return -EEXIST;
	}

	virt_log_buffer = hal_malloc(sizeof(char) * VIRT_LOG_SIZE);
	if (virt_log_buffer == NULL) {
		return -ENOMEM;
	}

	return 0;
}

void virt_log_deinit(void)
{
	unsigned long flags;

	flags = hal_interrupt_disable_irqsave();
	hal_free(virt_log_buffer);
	virt_log_buffer = NULL;
	hal_interrupt_enable_irqrestore(flags);
}
#else
static uint32_t virt_log_offline(void)
{
	return 0;
}

int virt_log_init(void)
{
	return 0;
}

void virt_log_deint(void)
{

}
#endif
