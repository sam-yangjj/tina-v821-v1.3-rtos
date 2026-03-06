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
#include <heap_retain.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <hal/aw_list.h>
#include <hal_cmd.h>
#include <hal_mem.h>
#include <hal_atomic.h>

#define HEAP_RETAIN_CALLER_NUM   (10)
typedef struct {
	void *ptr;
	uint32_t size;
	struct list_head node;
	void *caller[HEAP_RETAIN_CALLER_NUM];
} heap_retain_node_t;

extern void __internal_free(void *ptr);
extern void *__internal_malloc(uint32_t size);
extern void memleak_malloc_sethook(void (*hook)(void *ptr, uint32_t size));
extern void memleak_free_sethook(void (*hook)(void *ptr));

static hal_spinlock_t heap_retain_lock;
static struct list_head heap_retain_list = LIST_HEAD_INIT(heap_retain_list);
static uint32_t heap_retain_open= 0;

static void heap_retain_malloc_hook(void * ptr, uint32_t size)
{
	heap_retain_node_t *retain = NULL;

	if (!heap_retain_open) {
		return ;
	}
	hal_spin_lock(&heap_retain_lock);
	retain = __internal_malloc(sizeof(*retain));
	if (retain == NULL) {
		return ;
	}
	memset(retain, 0x00, sizeof(*retain));
	retain->ptr = ptr;
	retain->size = size;
	backtrace(NULL, retain->caller, HEAP_RETAIN_CALLER_NUM, 3, NULL);
	list_add(&retain->node, &heap_retain_list);
	hal_spin_unlock(&heap_retain_lock);
}

static void heap_retain_free_hook(void *ptr)
{
	uint32_t found = 0;
	struct list_head *pos;
	struct list_head *n;
	heap_retain_node_t *retain;

	if (!heap_retain_open) {
		return ;
	}
	hal_spin_lock(&heap_retain_lock);
	list_for_each_safe(pos, n, &heap_retain_list) {
		retain = container_of(pos, heap_retain_node_t, node);
		if (retain->ptr == ptr) {
			list_del(pos);
			found = 1;
			break;
		}
	}
	if (found) {
		__internal_free(retain);
	}
	hal_spin_unlock(&heap_retain_lock);

}

void heap_retain_init(void)
{
	hal_spin_lock_init(&heap_retain_lock);

	hal_spin_lock(&heap_retain_lock);
	heap_retain_open = 0;
	INIT_LIST_HEAD(&heap_retain_list);
	memleak_malloc_sethook(heap_retain_malloc_hook);
	memleak_free_sethook(heap_retain_free_hook);
	hal_spin_unlock(&heap_retain_lock);
}

void heap_retain_deinit(void)
{
	heap_retain_node_t *retain = NULL;
	struct list_head *pos;
	struct list_head *n;

	hal_spin_lock(&heap_retain_lock);
	list_for_each_safe(pos, n, &heap_retain_list) {
		retain = container_of(pos, heap_retain_node_t, node);
		list_del(pos);
		hal_free(retain);
	}
	memleak_malloc_sethook(NULL);
	memleak_free_sethook(NULL);
	INIT_LIST_HEAD(&heap_retain_list);
	heap_retain_open = 0;
	hal_spin_unlock(&heap_retain_lock);

	hal_spin_lock_deinit(&heap_retain_lock);
}

void heap_retain_start(void)
{
	hal_spin_lock(&heap_retain_lock);
	heap_retain_open = 1;
	hal_spin_unlock(&heap_retain_lock);
}

void heap_retain_stop(void)
{
	hal_spin_lock(&heap_retain_lock);
	heap_retain_open = 0;
	hal_spin_unlock(&heap_retain_lock);
}

void heap_retain_show(void)
{
	uint32_t i = 0;
	uint32_t cnt = 0;
	uint32_t total = 0;
	struct list_head *pos;
	heap_retain_node_t *retain;

	hal_spin_lock(&heap_retain_lock);
	list_for_each(pos, &heap_retain_list) {
		retain = container_of(pos, heap_retain_node_t, node);
		total += retain->size;
		printf("\t heap_retain: %u ptr = %p, size = %u\n", cnt++, retain->ptr, retain->size);
		for (i = 0; i < HEAP_RETAIN_CALLER_NUM; i++)
		{
			if (retain->caller[i] != NULL) {
				printf("                backtrace : 0x%p\n", retain->caller[i]);
			}
		}
	}
	printf("heap_retain total size: %u\n", total);
	hal_spin_unlock(&heap_retain_lock);
}

