/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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
#include <stdlib.h>
#include <time.h>
#include <hal_cmd.h>
#include "memheap.h"


struct memheap_t {
    struct memheap  heaps;                                /* Defines the memory heap array */
    void            *heap_mem[CONFIG_TEST_ITERATIONS];    /* Defines an array for storing allocated memory blocks */
    void            *start_addr;                          /* Pool start address and size */
    int             alloc_count;                          /* Record the location assigned to each heap */
    int             count;                                /* Records the number of memory blocks */
}g_heaps[CONFIG_NUM_HEAPS];


/* Initialize the memory heap */
void init_heaps(void)
{
    for (int i = 0; i < CONFIG_NUM_HEAPS; i++) {
        g_heaps[i].heaps = (struct memheap){};
        for (int j = 0; j < CONFIG_TEST_ITERATIONS; j++) {
            g_heaps[i].heap_mem[j] = NULL;
        }
        g_heaps[i].start_addr = NULL;
        g_heaps[i].alloc_count = 0;
        g_heaps[i].count = 0;

        g_heaps[i].start_addr = malloc(CONFIG_STACK_SIZE);
        if (g_heaps[i].start_addr == NULL) {
            printf("Memory allocation failed for heap %d\n", i);
            exit(1);
        } else {
            printf("Succeeded  applying  memory heap\n");
        }
        memheap_init(&g_heaps[i].heaps, "Heap", g_heaps[i].start_addr, CONFIG_STACK_SIZE);
    }
}

/* Free memory heap */
void deinit_heaps(void)
{
    for (int i = 0; i < CONFIG_NUM_HEAPS; i++) {
        /* Split memory heap */
        if (memheap_detach(g_heaps[i].start_addr) != 0) {
            printf("Failed to detach heap %d\n", i);
        }

        /* Free the allocated memory */
        if (g_heaps[i].start_addr != NULL) {
            free(g_heaps[i].start_addr);
            g_heaps[i].start_addr = NULL;
        } else {
            printf("Heap %d start address is NULL, cannot free\n", i);
        }
    }
}

/* Empty array */
void clear_arrays(void)
{
    for (int i = 0; i < CONFIG_NUM_HEAPS; i++) {
        for (int j = 0; j < CONFIG_TEST_ITERATIONS; j++) {
            g_heaps[i].heap_mem[j] = NULL;
        }
        g_heaps[i].alloc_count = 0;
        g_heaps[i].count = 0;
    }
}

/* Test the memory heap request release */
void test_memheap(void)
{
    int heap_index = 0;
    int alloc_size = 0;
    int action = 0;
    int index = 0;
    void *ptr = NULL;
    int flag = 0;
    srand(time(NULL));
    for (int i = 0; i < 2; i++) {
        for (int iter = 0; iter < CONFIG_TEST_ITERATIONS; iter++) {
            /* Randomly select a memory heap 0 1 */
            heap_index = rand() % CONFIG_NUM_HEAPS;
            /* Randomly allocate a memory block size */
            alloc_size = rand() % CONFIG_MAX_ALLOC_SIZE + 1;
            /* Choose an operation at random(0: alloc or alloc align, 1: free or ferr align) */
            action = rand() % 2;

            if (action == 0) {
                if (flag == 0) {
                    ptr = memheap_alloc(&g_heaps[heap_index].heaps, alloc_size);
                } else {
                    ptr = memheap_alloc_align(&g_heaps[heap_index].heaps, alloc_size, CONFIG_ALIGNMENT);
                }
                if (ptr != NULL) {
                    /* Records allocated memory blocks */
                    g_heaps[heap_index].heap_mem[g_heaps[heap_index].alloc_count++] = ptr;
                    g_heaps[heap_index].count++;
                    printf("Successful allocated memory block : %d bytes   heap :%p   block num: %d\n", alloc_size, &g_heaps[heap_index].heaps, g_heaps[heap_index].count);
                } else {
                    printf("No allocation failure \n");
                }
            } else if (action == 1) {
                if (g_heaps[heap_index].alloc_count <= 0)
                    continue;
                /* Randomly select an allocated memory block */
                index = rand() % g_heaps[heap_index].alloc_count;
                if (g_heaps[heap_index].heap_mem[index] == NULL) {
                    printf("The randomly released memory block is NULL\n");
                    continue;
                }
                ptr = g_heaps[heap_index].heap_mem[index];
                if (flag == 0) {
                    memheap_free(ptr);
                } else {
                    memheap_free_align(ptr);
                }
                /* Sets the pointer to the freed memory block to NULL */
                g_heaps[heap_index].heap_mem[index] = NULL;
                /* Free memory block num - 1 */
                g_heaps[heap_index].count--;
                printf("Freed success   heap :%p remaining memory block num: %d\n", &g_heaps[heap_index].heaps, g_heaps[heap_index].count);
            }
        }
        clear_arrays();
        flag = 1 ;
    }
}

int mumheap_test(void)
{
    init_heaps();
    printf("Start memory heap request release\n");
    test_memheap();

    deinit_heaps();
    printf("Free the memory heap and block\n");
    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(mumheap_test, mumheap_test, mumheap_test test);