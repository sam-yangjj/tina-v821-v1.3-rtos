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
#ifndef SUNXI_HAL_MEM_H
#define SUNXI_HAL_MEM_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

void *hal_realloc(void *ptr, uint32_t size);
void *hal_calloc(uint32_t numb, uint32_t size);

void *hal_malloc(uint32_t size);
void hal_free(void *p);

void *hal_malloc_align(uint32_t size, int align);
void hal_free_align(void *p);

void hal_malloc_coherent_init(void);
void hal_free_coherent_prot(void *addr);
void *hal_malloc_coherent_prot(size_t size, unsigned long prot);
void *hal_malloc_coherent_non_cacheable(size_t size);
void hal_free_coherent_non_cacheable(void *addr);

int dma_coherent_heap_init(void);
void *dma_coherent_heap_alloc(size_t size);
void dma_coherent_heap_free(void *ptr);

void *dma_coherent_heap_alloc_align(size_t size,int align);
void dma_coherent_heap_free_align(void *ptr);

unsigned long get_memory_prot(unsigned long prot);

void *hal_alloc_page(int npages);
void hal_free_page(void *page, int npages);

#ifdef CONFIG_KERNEL_FREERTOS
extern unsigned long __va_to_pa(unsigned long vaddr);
extern unsigned long __pa_to_va(unsigned long paddr);
#elif defined(CONFIG_OS_MELIS)
#include <rtthread.h>
#include <kapi.h>

#if !defined(CONFIG_SOC_SUN20IW1)
unsigned long awos_arch_virt_to_phys(unsigned long virtaddr);
unsigned long awos_arch_phys_to_virt(unsigned long phyaddr);
#define __va_to_pa(vaddr) awos_arch_virt_to_phys((vaddr))
#define __pa_to_va(paddr) awos_arch_phys_to_virt((paddr))
#else
#define __va_to_pa(vaddr) esMEMS_VA2PA((vaddr))
#endif
#else

#define __va_to_pa(vaddr) ((unsigned long)vaddr)
#define __pa_to_va(vaddr) ((unsigned long)vaddr)

#endif /* CONFIG_KERNEL_FREERTOS */

#define PAGE_MEM_ATTR_NON_CACHEABLE 0x000000001UL
#define PAGE_MEM_ATTR_CACHEABLE     0x000000002UL

#ifdef __cplusplus
}
#endif
#endif
