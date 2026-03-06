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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <heap_trace.h>
#include <osal/hal_interrupt.h>
#include <backtrace.h>
#include <console.h>

#define HEAP_MAGIC_LEN          4
#ifdef CONFIG_ARCH_ARM_CORTEX_M33 //CONFIG_ARCH_ARM_CORTEX_M33
#define MF_RECORD               1
#define MEM_OUT_OF_CHECK        1
#define HEAP_BACKTRACE          1
#define HEAP_MEM_DBG_MIN_SIZE   100
#define HEAP_MEM_MAX_CNT        1024

#ifdef MF_RECORD
#define MF_RECORD_MAX_CNT       4096
#endif

#ifdef HEAP_BACKTRACE
#define BACKTRACE_COUNT         2
#define BACKTRACE_OFFSET        4
#endif

#elif CONFIG_ARCH_RISCV /*CONFIG_ARCH_RISCV*/
#define MF_RECORD               1
#define MEM_OUT_OF_CHECK        1
#define HEAP_BACKTRACE          1
#define HEAP_MEM_DBG_MIN_SIZE   0
#define HEAP_MEM_MAX_CNT        8192

#ifdef MF_RECORD
#define MF_RECORD_MAX_CNT       8192
#endif

#ifdef HEAP_BACKTRACE
#define BACKTRACE_COUNT         4
#define BACKTRACE_OFFSET        2
#endif

#elif CONFIG_ARCH_DSP /*CONFIG_ARCH_DSP*/
#define MF_RECORD               1
#define MEM_OUT_OF_CHECK        1
#define HEAP_BACKTRACE          1
#define HEAP_MEM_DBG_MIN_SIZE   100
#define HEAP_MEM_MAX_CNT        1024

#ifdef MF_RECORD
#define MF_RECORD_MAX_CNT       4096
#endif

#ifdef HEAP_BACKTRACE
#define BACKTRACE_COUNT         2
#define BACKTRACE_OFFSET        4
#endif
#endif /*CONFIG_ARCH_DSP*/

#if (MF_RECORD == 0) && (MEM_OUT_OF_CHECK == 0)
#error "One must be selected"
#endif

#define HEAP_MEM_DBG_ON         0
#define HEAP_MEM_ERR_ON         1

#define HEAP_SYSLOG             printf

#define HEAP_MEM_IS_TRACED(size)    (size > HEAP_MEM_DBG_MIN_SIZE)

#define HEAP_MEM_LOG(flags, fmt, arg...)    \
    do {                                    \
        if (flags)                          \
            HEAP_SYSLOG(fmt, ##arg);        \
    } while (0)

#define HEAP_MEM_DBG(fmt, arg...) \
	HEAP_MEM_LOG(HEAP_MEM_DBG_ON, "[heap] "fmt, ##arg)

#define HEAP_MEM_ERR(fmt, arg...) \
	HEAP_MEM_LOG(HEAP_MEM_ERR_ON, "[heap ERR] %s():%d, "fmt, \
	                              __func__, __LINE__, ##arg);

#if MF_RECORD
struct mf_trace {
	uint8_t is_m;
	void *ptr;
	size_t size;
#if HEAP_BACKTRACE
	void *backtrace[BACKTRACE_COUNT];
#endif
};
static uint32_t mf_pos;
static struct mf_trace g_mf_trace[MF_RECORD_MAX_CNT];
#endif

static void *g_backtrace[BACKTRACE_COUNT];

static inline void malloc_mutex_lock(void)
{
    if (!hal_interrupt_get_nest())
		vTaskSuspendAll();
}

static inline void malloc_mutex_unlock(void)
{
    if (!hal_interrupt_get_nest())
		xTaskResumeAll();
}

#if MEM_OUT_OF_CHECK

struct heap_mem {
	void *ptr;
	size_t size;
#if HEAP_BACKTRACE
	void *backtrace[BACKTRACE_COUNT];
#endif
};

/* To avoid backtrace nesting being too deep
 * and affecting performance, define a variable
 * for backtrace at the outermost level of the
 * function.*/

static int g_mem_empty_idx = 0; /* beginning idx to do the search for new one */
static struct heap_mem g_mem[HEAP_MEM_MAX_CNT];
static const char g_mem_magic[HEAP_MAGIC_LEN] = {0x4a, 0x5b, 0x6c, 0x7f};
#define MEM_SET_MAGIC(p, l)  memcpy((((char *)(p)) + (l)), g_mem_magic, HEAP_MAGIC_LEN)
#define MEM_CHK_MAGIC(p, l)  memcmp((((char *)(p)) + (l)), g_mem_magic, HEAP_MAGIC_LEN)

static void heap_mem_show(int verbose, int index, struct heap_mem *heap)
{
#if HEAP_BACKTRACE
	int i;
#endif
	if (verbose) {
		HEAP_SYSLOG("%d, %p, %u\n", index, heap->ptr, heap->size);
#if HEAP_BACKTRACE
		HEAP_SYSLOG("backtrace:");
		for (i = 0; i < BACKTRACE_COUNT; i++) {
			HEAP_SYSLOG("%p ", heap->backtrace[i]);
		}
		HEAP_SYSLOG("\n");
#endif
	}

	if (MEM_CHK_MAGIC(heap->ptr, heap->size)) {
		HEAP_MEM_ERR("mem (%p) corrupt\n", heap->ptr);
#if HEAP_BACKTRACE
		HEAP_SYSLOG("backtrace:");
		for (i = 0; i < BACKTRACE_COUNT; i++) {
			HEAP_SYSLOG("%p ", heap->backtrace[i]);
		}
		HEAP_SYSLOG("\n");
#endif
	}
}

void heap_trace_info_show(int verbose)
{
	int i;
	malloc_mutex_lock();
	for (i = 0;i < HEAP_MEM_MAX_CNT; i++) {
		if (g_mem[i].ptr != NULL)
			heap_mem_show(1, i, &g_mem[i]);
	}
	malloc_mutex_unlock();
}

/* Note: @ptr != NULL */
static void heap_trace_add_entry(void *ptr, size_t size)
{
	int i;

	MEM_SET_MAGIC(ptr, size);

	for (i = g_mem_empty_idx; i < HEAP_MEM_MAX_CNT; ++i) {
		if (g_mem[i].ptr == NULL) {
			g_mem[i].ptr = ptr;
			g_mem[i].size = size;
#if HEAP_BACKTRACE
#if MF_RECORD
			memcpy(g_mem[i].backtrace,
					g_mf_trace[mf_pos - 1].backtrace, sizeof(void *) * BACKTRACE_COUNT);
#else
			memcpy(g_mem[i].backtrace,
					g_backtrace, sizeof(void *) * BACKTRACE_COUNT);
#endif
#endif
			g_mem_empty_idx = i + 1;

			break;
		}
	}

	if (i >= HEAP_MEM_MAX_CNT) {
		HEAP_MEM_ERR("heap mem count exceed %d\n", HEAP_MEM_MAX_CNT);
	}
}

/* Note: @ptr != NULL */
static size_t heap_trace_delete_entry(void *ptr)
{
	int i;
	size_t size;

	for (i = 0; i < HEAP_MEM_MAX_CNT; ++i) {
		if (g_mem[i].ptr == ptr) {
			size = g_mem[i].size;
			if (MEM_CHK_MAGIC(ptr, size)) {
				HEAP_MEM_ERR("mem f (%p, %u) corrupt\n", ptr, size);
#if HEAP_BACKTRACE
				int j;
				HEAP_SYSLOG("backtrace:");
				for (j = 0; j < BACKTRACE_COUNT; j++) {
					HEAP_SYSLOG("%p ", g_mem[i].backtrace[j]);
				}
				HEAP_SYSLOG("\n");
#endif
			}
			g_mem[i].ptr = NULL;
			g_mem[i].size = 0;
			if (i < g_mem_empty_idx)
				g_mem_empty_idx = i;
			break;
		}
	}

	if (i >= HEAP_MEM_MAX_CNT) {
		HEAP_MEM_ERR("heap mem entry (%p) missed\n", ptr);
#if HEAP_BACKTRACE
        backtrace(NULL, NULL, 0, 0, printf);
#endif
		size = -1;
	}

	return size;
}

/* Note: @old_ptr != NULL, @new_ptr != NULL, @new_size != 0 */
static size_t heap_trace_update_entry(void *old_ptr, void *new_ptr, size_t new_size)
{
	int i;
	size_t old_size;
	MEM_SET_MAGIC(new_ptr, new_size);

	for (i = 0; i < HEAP_MEM_MAX_CNT; ++i) {
		if (g_mem[i].ptr == old_ptr) {
			old_size = g_mem[i].size;
			g_mem[i].ptr = new_ptr;
			g_mem[i].size = new_size;
#if HEAP_BACKTRACE
			memcpy(g_mem[i].backtrace,
					g_backtrace, sizeof(void *) * BACKTRACE_COUNT);
#endif
			break;
		}
	}

	if (i >= HEAP_MEM_MAX_CNT) {
		HEAP_MEM_ERR("heap mem entry (%p) missed\n", new_ptr);
#if HEAP_BACKTRACE
        backtrace(NULL, NULL, 0, 0, printf);
#endif
		old_size = -1;
	}

	return old_size;
}
#endif /*MEM_OUT_OF_CHECK*/

#if MF_RECORD

static int heap_trace_update_mf_trace(void *ptr, size_t size, uint8_t is_m)
{
	if (mf_pos >= MF_RECORD_MAX_CNT)
		mf_pos = 0;
	g_mf_trace[mf_pos].ptr = ptr;
	g_mf_trace[mf_pos].size = size;
	g_mf_trace[mf_pos].is_m = is_m;
	mf_pos ++;
	return 0;
}
#endif

static int _heap_trace_malloc(void *ptr, size_t size)
{
	if (HEAP_MEM_IS_TRACED(size)) {
		HEAP_MEM_DBG("m (%p, %u)\n", ptr, size);
#if MF_RECORD
		heap_trace_update_mf_trace(ptr, size, 1);
#endif
	} else
		mf_pos ++;


#if MEM_OUT_OF_CHECK
	if (ptr) {
		heap_trace_add_entry(ptr, size);
	} else {
		HEAP_MEM_ERR("heap mem exhausted (%u)\n", size);
	}
#endif
	return 0;
}

static int _heap_trace_free(void *ptr)
{
	size_t size = -1;

	if (ptr == NULL) {
		return 0;
	}
#if MEM_OUT_OF_CHECK
	size = heap_trace_delete_entry(ptr);
#elif MF_RECORD
	int i;
	uint8_t over = 0;
	for(i = mf_pos; i >= 0; i--) {
		if (g_mf_trace[i].ptr == ptr) {
			size = g_mf_trace[i].size;
			break;
		}
		if (i == 0) {
			i = MF_RECORD_MAX_CNT;
			over = 1;
		}

		if (i <= (mf_pos + 1) && over) {
			break;
		}
	}
	/* When the BACKTRACE COUNT is relatively small,
	 * only the most recent memory can be recorded,
	 * and previous memory records will be overwritten,
	 * so the released memory size may not be accurate.*/
#endif
	if (size != -1 && HEAP_MEM_IS_TRACED(size)) {
		HEAP_MEM_DBG("f (%p, %u)\n", ptr, size);
#if MF_RECORD
		heap_trace_update_mf_trace(ptr, size, 0);
#endif
	} else
		mf_pos ++;
	return 0;
}

static int _heap_trace_realloc(void *old_ptr, void *new_ptr, size_t new_size)
{

#if MF_RECORD
	int i;

	if (new_size == 0)
		return 0;
	if (old_ptr != NULL) {
		for (i = 0; i < MF_RECORD_MAX_CNT; i++) {
			if (g_mf_trace[i].ptr == old_ptr) {

				g_mf_trace[i].ptr = new_ptr;
				g_mf_trace[i].size = new_size;

#if HEAP_BACKTRACE
			memcpy(g_mf_trace[i].backtrace,
					g_backtrace, sizeof(void *) * BACKTRACE_COUNT);
#endif
				break;
			}
		}
	} else {
		heap_trace_update_mf_trace(new_ptr, new_size, 2);
	}
#endif

#if MEM_OUT_OF_CHECK
	size_t old_size = 0;

	if (new_size == 0) {
		if (old_ptr) {
			old_size = heap_trace_delete_entry(old_ptr);
		}
	} else {
		if (old_ptr == NULL) {
			if (new_ptr != NULL) {
				heap_trace_add_entry(new_ptr, new_size);
			} else {
				HEAP_MEM_ERR("heap mem exhausted (%p, %u)\n", old_ptr, new_size);
			}
		} else {
			if (new_ptr != NULL) {
				old_size = heap_trace_update_entry(old_ptr, new_ptr, new_size);
			} else {
				HEAP_MEM_ERR("heap mem exhausted (%p, %u)\n", old_ptr, new_size);
			}
		}
	}
	if (HEAP_MEM_IS_TRACED(new_size) || HEAP_MEM_IS_TRACED(old_size)) {
		HEAP_MEM_DBG("r (%p, %u) -> (%p, %u)\n", old_ptr, old_size, new_ptr, new_size);
	}
#endif
	return 0;
}

static uint8_t g_do_reallocing = 0;
void *heap_trace_malloc(size_t size)
{
    void *result = NULL;
	malloc_mutex_lock();

#if HEAP_BACKTRACE
#if MF_RECORD
	backtrace(NULL, g_mf_trace[mf_pos].backtrace,
			BACKTRACE_COUNT, BACKTRACE_OFFSET, NULL);
#else
	backtrace(NULL, g_backtrace,
			BACKTRACE_COUNT, BACKTRACE_OFFSET, NULL);
#endif
#endif

#if MEM_OUT_OF_CHECK
	size_t real_size = 0;
	if (!g_do_reallocing)
		real_size = size + HEAP_MAGIC_LEN;
    result = pvPortMalloc(real_size);
#elif MF_RECORD
	result = pvPortMalloc(size);
#endif

	if (result && !g_do_reallocing)
		_heap_trace_malloc(result, size);

	malloc_mutex_unlock();
	return result;
}

void *heap_trace_realloc(void *old, size_t newlen)
{
	void *new = NULL;
    void *pvPortRealloc(void * old, size_t newlen);

	malloc_mutex_lock();

#if HEAP_BACKTRACE
	backtrace(NULL, g_backtrace,
			BACKTRACE_COUNT, BACKTRACE_OFFSET, NULL);
#endif

#if MEM_OUT_OF_CHECK
	size_t real_size = 0;

	g_do_reallocing = 1;

	if (newlen)
		real_size = newlen + HEAP_MAGIC_LEN;

	new =  pvPortRealloc(old, real_size);
	if (newlen == 0)
		_heap_trace_realloc(old, new, newlen);
	else
		_heap_trace_realloc(old, new, real_size - HEAP_MAGIC_LEN);

#elif MF_RECORD
	new = pvPortRealloc(old, newlen);
	_heap_trace_realloc(old, new, newlen);
#endif

	g_do_reallocing = 0;

	malloc_mutex_unlock();

	return new;
}

void *heap_trace_calloc(size_t size, size_t len)
{
    void *result = NULL;

	malloc_mutex_lock();

#if HEAP_BACKTRACE
#if MF_RECORD
	backtrace(NULL, g_mf_trace[mf_pos].backtrace,
			BACKTRACE_COUNT, BACKTRACE_OFFSET, NULL);
#else
	backtrace(NULL, g_backtrace,
			BACKTRACE_COUNT, BACKTRACE_OFFSET, NULL);
#endif
#endif

#if MEM_OUT_OF_CHECK
	size_t real_size = 0;

	if (!g_do_reallocing)
		real_size = size * len + HEAP_MAGIC_LEN;

	result = pvPortMalloc(real_size);
#elif MF_RECORD
	result = pvPortMalloc(size * len);
#endif
	if (result) {
		memset(result, 0, size * len);
		_heap_trace_malloc(result, size * len);
	}

	malloc_mutex_unlock();

	return result;

}

void heap_trace_free(void *ptr)
{
	malloc_mutex_lock();

#if HEAP_BACKTRACE
#if MF_RECORD
	backtrace(NULL, g_mf_trace[mf_pos].backtrace,
			BACKTRACE_COUNT, BACKTRACE_OFFSET, NULL);
#else
	backtrace(NULL, g_backtrace,
			BACKTRACE_COUNT, BACKTRACE_OFFSET, NULL);
#endif
#endif

	if (!g_do_reallocing)
		_heap_trace_free(ptr);

    vPortFree(ptr);

	malloc_mutex_unlock();
}

#if MEM_OUT_OF_CHECK
void cmd_mem_info(int argc, char *argv[])
{
	heap_trace_info_show(1);
}
FINSH_FUNCTION_EXPORT_CMD(cmd_mem_info, mem_info, malloc_trace);
#endif /*MEM_OUT_OF_CHECK*/

#if MF_RECORD
void cmd_malloc_free_trace(int argc, char *argv[])
{
	int i, j;
	uint8_t over = 0;
	malloc_mutex_lock();
	for (i = mf_pos; i < MF_RECORD_MAX_CNT; i++) {

		if (g_mf_trace[i].ptr != NULL) {

			if (g_mf_trace[i].is_m == 1) {
				HEAP_SYSLOG("%d m(%p, %u)\n", i, g_mf_trace[i].ptr, g_mf_trace[i].size);
			} else if (g_mf_trace[i].is_m == 2) {
				HEAP_SYSLOG("%d r(%p, %u)\n", i, g_mf_trace[i].ptr, g_mf_trace[i].size);
			} else {
				HEAP_SYSLOG("%d f(%p, %u)\n", i, g_mf_trace[i].ptr, g_mf_trace[i].size);
			}
#if HEAP_BACKTRACE
			HEAP_SYSLOG("backtrace:");
			for (j = 0; j < BACKTRACE_COUNT; j++) {
				HEAP_SYSLOG("%p ", g_mf_trace[i].backtrace[j]);
			}
			HEAP_SYSLOG("\n");
#endif /*HEAP_BACKTRACE*/
		}

		if (i == MF_RECORD_MAX_CNT - 1) {
			i = -1;
			over = 1;
		}

		if (i == (mf_pos - 1) && over)
			break;
	}
	HEAP_SYSLOG("current pos:%d\n",mf_pos);
	malloc_mutex_unlock();
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_free_trace, mf_trace, malloc free trace);
#endif /*MF_RECORD*/
