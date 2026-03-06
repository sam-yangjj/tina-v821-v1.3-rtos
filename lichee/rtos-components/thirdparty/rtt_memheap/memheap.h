/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * File      : memheap.c
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-04-10     Bernard      first implementation
 * 2012-10-16     Bernard      add the mutex lock for heap object.
 * 2012-12-29     Bernard      memheap can be used as system heap.
 *                             change mutex lock to semaphore lock.
 * 2013-04-10     Bernard      add rt_memheap_realloc function.
 * 2013-05-24     Bernard      fix the rt_memheap_realloc issue.
 * 2013-07-11     Grissiom     fix the memory block splitting issue.
 * 2013-07-15     Grissiom     optimize rt_memheap_realloc
 */

#ifndef _MEMHEAP_H_
#define _MEMHEAP_H_

#include <stdint.h>
#include <hal_mutex.h>

struct memheap_item
{
    uint32_t             magic;                      /* *< magic number for memheap */
    struct memheap      *pool_ptr;                   /* *< point of pool */

    struct memheap_item *next;                       /* *< next memheap item */
    struct memheap_item *prev;                       /* *< prev memheap item */

    struct memheap_item *next_free;                  /* *< next free memheap item */
    struct memheap_item *prev_free;                  /* *< prev free memheap item */
};

struct memheap
{
    void                *start_addr;                 /* *< pool start address and size */
    uint32_t             pool_size;                  /* *< pool size */
    uint32_t             available_size;             /* *< available size */
    uint32_t             max_used_size;              /* *< maximum allocated size */
    struct memheap_item *block_list;                 /* *< used block list */
    struct memheap_item *free_list;                  /* *< free block list */
    struct memheap_item  free_header;                /* *< free block list header */
#ifdef CONFIG_RTT_MEMHEAP_USE_MUTEX_LOCK
    struct hal_mutex     lock;                       /* *< mutex lock */
#endif
};

int memheap_init(struct memheap *memheap,
                 const char        *name,
                 void              *start_addr,
                 size_t         size);

int memheap_detach(struct memheap *heap);

void *memheap_alloc(struct memheap *heap, size_t size);
void *memheap_alloc_align(struct memheap *heap, size_t size, size_t align);

void memheap_free(void *ptr);
void memheap_free_align(void *ptr);
#endif
