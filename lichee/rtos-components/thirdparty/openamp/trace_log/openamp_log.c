/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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
#include <stdio.h>
#include <sunxi_hal_common.h>
#include <hal_interrupt.h>
#include <hal_cache.h>
#include <hal_cmd.h>
#include <string.h>
#include <hal_time.h>
#include <inttypes.h>
#include <errno.h>
#include <stdarg.h>

#ifdef CONFIG_AMP_TRACE_SUPPORT

char amp_log_buffer[CONFIG_AMP_TRACE_BUF_SIZE] __attribute__((aligned(64))) = {0};

/* mem layout: | user0(reader) area | user1(writer) area | data |*/

#define AREA_ALIGN_SIZE		(64)

#define READER_AREA_SIZE	(AREA_ALIGN_SIZE)
#define READER_AREA_OFFSET	(0)

#define WRITER_AREA_SIZE	(AREA_ALIGN_SIZE)
#define WRITER_AREA_OFFSET	(READER_AREA_SIZE)

#define LOG_BUF_OFFSET		(WRITER_AREA_OFFSET + WRITER_AREA_SIZE)
#define LOG_BUF_SIZE		(sizeof(amp_log_buffer) - LOG_BUF_OFFSET)

#pragma pack(1)
struct area_info_t {
	uint64_t pos;
	uint32_t area_size;
	uint32_t size;
	uint32_t overrun;
};
#pragma pack()

#define READER_AREA_PTR		((struct area_info_t *)&amp_log_buffer[READER_AREA_OFFSET])
#define WRITER_AREA_PTR		((struct area_info_t *)&amp_log_buffer[WRITER_AREA_OFFSET])
#define LOG_BUF_PTR		((char *)&amp_log_buffer[LOG_BUF_OFFSET])

static inline void cpy_to_cachemem(void *dst, const void *src, size_t size)
{
	memcpy(dst, src, size);
	hal_dcache_clean((unsigned long)dst, size);
}

static inline void cpy_from_cachemem(void *dst, const void *src, size_t size)
{
	hal_dcache_invalidate((unsigned long)src, size);
	memcpy(dst, src, size);
}

// cache ops may have printf when defined CONFIG_CACHE_ALIGN_CHECK
// pay attention to whether recursive calls occur
static inline unsigned long
ringbuffer_ops_enter(const struct area_info_t *shared, struct area_info_t *local)
{
	unsigned long flags;
#ifdef CONFIG_CACHE_ALIGN_CHECK
	struct area_info_t tmp;
	cpy_from_cachemem(&tmp, shared, sizeof(tmp));
	flags = hal_interrupt_disable_irqsave();
	memcpy(local, &tmp, sizeof(*local));
#else
	flags = hal_interrupt_disable_irqsave();
	cpy_from_cachemem(local, shared, sizeof(*local));
#endif
	return flags;
}

static inline void
ringbuffer_ops_exit(struct area_info_t *shared, const struct area_info_t *local,
		    unsigned long flags)
{
#ifdef CONFIG_CACHE_ALIGN_CHECK
	struct area_info_t tmp;
	memcpy(&tmp, local, sizeof(*tmp));
	hal_interrupt_enable_irqrestore(flags);
	cpy_to_cachemem(shared, &tmp, sizeof(*shared));
#else
	cpy_to_cachemem(shared, local, sizeof(*shared));
	hal_interrupt_enable_irqrestore(flags);
#endif
}

static inline void write_ringbuffer(void* ring, size_t ring_size, uint64_t pos,
				    const void* data, size_t data_size)
{
	size_t start = pos % ring_size;
	size_t end = start + data_size;
	size_t off;

	if (ring_size < data_size)
		return;

	if (end > ring_size) {
		off = ring_size - start;
		cpy_to_cachemem(ring + start, data, off);
		cpy_to_cachemem(ring, data + off, data_size - off);
	} else {
		cpy_to_cachemem(ring + start, data, data_size);
	}
}

static struct area_info_t g_reader_info, g_writer_info;
static inline int ringbuffer_put_internal(void *data, size_t size, int force)
{
	struct area_info_t * const reader = &g_reader_info;
	struct area_info_t * const writer = &g_writer_info;
	char *log_buf = LOG_BUF_PTR;
	const char *msg = NULL;
	size_t writable, msg_size;
	unsigned long flags = ringbuffer_ops_enter(READER_AREA_PTR, reader);

	if (!writer->area_size || !writer->size) {
		// init reader info
		msg = "\r\ninfo: writer init\r\n";
		writer->area_size = WRITER_AREA_SIZE;
		writer->size = LOG_BUF_SIZE;
		// looking for a better way ?
		writer->pos = reader->pos;
	}

	if (size > writer->size)
		size = writer->size;

	// using uint64_t, not considering numerical overflow issues
	if (writer->pos < reader->pos) {
		msg = "\r\nerr: writer->pos < reader->pos\r\n";
		writer->pos = reader->pos;
	}

	// using size_t, not considering numerical overflow issues
	writable = reader->pos + writer->size - writer->pos;

	if (0 == writable) {
		if (force || writer->overrun) {
			writable = writer->size;
			writer->overrun = 1;
		} else {
			goto out;
		}
	} else if (writable > writer->size) {
		writable = writer->size;
		writer->overrun = 1;
	} else {
		writer->overrun = 0;
	}

	// save debug info
	if (msg && (msg_size = strlen(msg)) <= writable) {
		write_ringbuffer(log_buf, writer->size, writer->pos, msg, msg_size);
		writer->pos += msg_size;
		writable -= msg_size;
		msg = NULL;
	}

	if (writable > size)
		writable = size;
	write_ringbuffer(log_buf, writer->size, writer->pos, data, writable);
	writer->pos += writable;

out:
	ringbuffer_ops_exit(WRITER_AREA_PTR, writer, flags);
	return writable;
}

static inline int ringbuffer_put(void *data, int size)
{
	int ret, force = 0, write = 0;

	if (!data || size <= 0)
		return -EINVAL;

	while (size != write) {
		ret = ringbuffer_put_internal(data + write, size - write, force);
		if (ret < 0)
			return ret;
		else if (ret == 0)
			force = 1; // need wait ?
		write += ret;
	}

	return size;
}

int amp_log_put(char ch)
{
	return ringbuffer_put(&ch, 1);
}

int amp_log_put_str(char *buf)
{
	return ringbuffer_put(buf, strlen(buf));
}

int amp_log_put_buf(char *buf, int len)
{
	return ringbuffer_put(buf, len);
}

int amp_printf(const char *fmt, ...)
{
	int ret;
	char tmpbuf[128];
	va_list args;

	va_start(args, fmt);
	ret = vsnprintf(tmpbuf, sizeof(tmpbuf), fmt, args);
	va_end(args);
	if (ret < 0 || ret > sizeof(tmpbuf))
		return -EFAULT;

	return amp_log_put_buf(tmpbuf, ret);
}

static int amp_log_stat(int argc, const char **argv)
{
	struct area_info_t reader, writer;
#ifndef CONFIG_CACHE_ALIGN_CHECK
	unsigned long flags = hal_interrupt_disable_irqsave();
#endif

	cpy_from_cachemem(&reader, READER_AREA_PTR, sizeof(reader));
	cpy_from_cachemem(&writer, WRITER_AREA_PTR, sizeof(writer));
#ifndef CONFIG_CACHE_ALIGN_CHECK
	hal_interrupt_enable_irqrestore(flags);
#endif

	printf("amp log stat:\r\n");
	printf("trace_mem:     %lx\n", (unsigned long)&amp_log_buffer);
	printf("trace_mem_len: %lu\n", (unsigned long)sizeof(amp_log_buffer));
	printf("r->pos:        %llu\n", (unsigned long long)reader.pos);
	printf("r->area_size:  %u\n", reader.area_size);
	printf("w->pos:        %llu\n", (unsigned long long)writer.pos);
	printf("w->size:       %u\n", writer.size);
	printf("w->area_size:  %u\n", writer.area_size);
	printf("w->overrun:    %u\n", writer.overrun);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(amp_log_stat, amp_log_stat, dump amp log stat);

#else
int amp_log_put(char ch)
{
	(void)ch;
	return 0;
}

int amp_log_put_str(char *buf)
{
	(void)buf;
	return 0;
}

int amp_log_put_buf(char *buf, int len)
{
	(void)buf;
	(void)len;
	return 0;
}

int amp_printf(const char *fmt, ...)
{
	return 0;
}
#endif
