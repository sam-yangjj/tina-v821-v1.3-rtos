/*
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * Author: Andrey Ryabinin <a.ryabinin@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <kasan_rtos.h>
#include <portable.h>
#include <console.h>
#include <sunxi_hal_common.h>

extern void *pvPortRealloc(void * old, size_t newlen);

static char global_array[10] = {0};

__attribute__((optimize("O0"), noinline)) static void kasan_global_oob(void)
{
    volatile int i = 3;
    char *p = &global_array[ARRAY_SIZE(global_array) + i];

    printf("out-of-bounds global variable\n");
    *(volatile char *)p;
}

__attribute__((optimize("O0"), noinline)) static void kasan_stack_oob(void)
{
    char stack_array[10];
    volatile int i = 0;
    char *p = &stack_array[ARRAY_SIZE(stack_array) + i];

    printf("out-of-bounds on stack\n");
    *(volatile char *)p;
}

__attribute__((optimize("O0"), noinline)) static void kasan_use_after_scope_test(void)
{
    volatile char *volatile p;

    printf("use-after-scope on int\n");
    {
        int local = 0;

        p = (char *)&local;
    }
    p[0] = 1;
    p[3] = 1;

    printf("use-after-scope on array\n");
    {
        char local[1024] = {0};

        p = local;
    }
    p[0] = 1;
    p[1023] = 1;
}

__attribute__((optimize("O0"), noinline)) static void kasan_alloca_oob_left(void)
{
    volatile int i = 10;
    char alloca_array[i];
    char *p = alloca_array - 1;

    printf("out-of-bounds to left on alloca\n");
    *(volatile char *)p;
}

__attribute__((optimize("O0"), noinline)) static void kasan_alloca_oob_right(void)
{
    volatile int i = 10;
    char alloca_array[i];
    char *p = alloca_array + i;

    printf("out-of-bounds to right on alloca\n");
    *(volatile char *)p;
}

/**
 * malloc entry testcase
 */
__attribute__((optimize("O0"), noinline)) static void malloc_oob_right(void)
{
    char *ptr;
    size_t size = 123;

    printf("out-of-bounds to right\n");
    ptr = malloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    ptr[size] = 'x';
    free(ptr);
}

__attribute__((optimize("O0"), noinline)) static void malloc_oob_left(void)
{
    char *ptr;
    size_t size = 15;

    printf("out-of-bounds to left\n");
    ptr = malloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    *ptr = *(ptr - 1);
    free(ptr);
}

__attribute__((optimize("O0"), noinline)) static void oob_realloc_more(void)
{
    char *ptr1, *ptr2;
    size_t size1 = 17;
    size_t size2 = 19;

    printf("out-of-bounds after realloc more\n");
    ptr1 = malloc(size1);
    ptr2 = realloc(ptr1, size2);
    if (!ptr1 || !ptr2)
    {
        printf("Allocation failed\n");
        free(ptr1);
        return;
    }

    ptr2[size2] = 'x';
    free(ptr2);
}

__attribute__((optimize("O0"), noinline)) static void oob_realloc_less(void)
{
    char *ptr1, *ptr2;
    size_t size1 = 17;
    size_t size2 = 15;

    printf("out-of-bounds after realloc less\n");
    ptr1 = malloc(size1);
    ptr2 = realloc(ptr1, size2);
    if (!ptr1 || !ptr2)
    {
        printf("Allocation failed\n");
        free(ptr1);
        return;
    }
    ptr2[size2] = 'x';
    free(ptr2);
}

__attribute__((optimize("O0"), noinline)) static void malloc_oob_access_16(void)
{
    struct
    {
        uint64_t words[2];
    } *ptr1, *ptr2;

    printf("malloc out-of-bounds for 16-bytes access\n");
    ptr1 = malloc(sizeof(*ptr1) - 3);
    ptr2 = malloc(sizeof(*ptr2));
    if (!ptr1 || !ptr2)
    {
        printf("Allocation failed\n");
        free(ptr1);
        free(ptr2);
        return;
    }
    *ptr1 = *ptr2;
    free(ptr1);
    free(ptr2);
}

__attribute__((optimize("O0"), noinline)) static void malloc_oob_memset_2(void)
{
    char *ptr;
    size_t size = 8;

    printf("out-of-bounds in memset2\n");
    ptr = malloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr + 7, 0, 2);
    free(ptr);
}

__attribute__((optimize("O0"), noinline)) static void malloc_oob_memset_4(void)
{
    char *ptr;
    size_t size = 8;

    printf("out-of-bounds in memset4\n");
    ptr = malloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr + 5, 0, 4);
    free(ptr);
}

__attribute__((optimize("O0"), noinline)) static void malloc_oob_memset_8(void)
{
    char *ptr;
    size_t size = 8;

    printf("out-of-bounds in memset8\n");
    ptr = malloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr + 1, 0, 8);
    free(ptr);
}

__attribute__((optimize("O0"), noinline)) static void malloc_oob_memset_16(void)
{
    char *ptr;
    size_t size = 16;

    printf("out-of-bounds in memset16\n");
    ptr = malloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr + 1, 0, 16);
    free(ptr);
}

__attribute__((optimize("O0"), noinline)) static void malloc_oob_in_memset(void)
{
    char *ptr;
    size_t size = 666;

    printf("out-of-bounds in memset\n");
    ptr = malloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr, 0, size + 5);
    free(ptr);
}

__attribute__((optimize("O0"), noinline)) static void malloc_uaf(void)
{
    char *ptr;
    size_t size = 10;

    printf("use-after-free\n");
    ptr = malloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    free(ptr);
    *(ptr + 8) = 'x';
}

__attribute__((optimize("O0"), noinline)) static void malloc_uaf_memset(void)
{
    char *ptr;
    size_t size = 33;

    printf("use-after-free in memset\n");
    ptr = malloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    free(ptr);
    memset(ptr, 0, size);
}

__attribute__((optimize("O0"), noinline)) static void malloc_double_free_small(void)
{
    char *p = malloc(100);

    printf("double free small test case\n");
    free(p);
    free(p);
}

__attribute__((optimize("O0"), noinline)) static void malloc_double_free_large(void)
{
    char *p = malloc(16384 + 10);

    printf("double free large test case\n");
    free(p);
    free(p);
}

/**
 * rtos heap_4 malloc entry testcase
 */
__attribute__((optimize("O0"), noinline)) static void rt_malloc_oob_right(void)
{
    char *ptr;
    size_t size = 123;

    printf("out-of-bounds to right\n");
    ptr = pvPortMalloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    ptr[size] = 'x';
    vPortFree(ptr);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_oob_left(void)
{
    char *ptr;
    size_t size = 15;

    printf("out-of-bounds to left\n");
    ptr = pvPortMalloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    *ptr = *(ptr - 1);
    vPortFree(ptr);
}

__attribute__((optimize("O0"), noinline)) static void rt_oob_realloc_more(void)
{
    char *ptr1, *ptr2;
    size_t size1 = 17;
    size_t size2 = 19;

    printf("out-of-bounds after realloc more\n");
    ptr1 = pvPortMalloc(size1);
    ptr2 = pvPortRealloc(ptr1, size2);
    if (!ptr1 || !ptr2)
    {
        printf("Allocation failed\n");
        vPortFree(ptr1);
        return;
    }

    ptr2[size2] = 'x';
    vPortFree(ptr2);
}

__attribute__((optimize("O0"), noinline)) static void rt_oob_realloc_less(void)
{
    char *ptr1, *ptr2;
    size_t size1 = 17;
    size_t size2 = 15;

    printf("out-of-bounds after realloc less\n");
    ptr1 = pvPortMalloc(size1);
    ptr2 = pvPortRealloc(ptr1, size2);
    if (!ptr1 || !ptr2)
    {
        printf("Allocation failed\n");
        vPortFree(ptr1);
        return;
    }
    ptr2[size2] = 'x';
    vPortFree(ptr2);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_oob_access_16(void)
{
    struct
    {
        uint64_t words[2];
    } *ptr1, *ptr2;

    printf("malloc out-of-bounds for 16-bytes access\n");
    ptr1 = pvPortMalloc(sizeof(*ptr1) - 3);
    ptr2 = pvPortMalloc(sizeof(*ptr2));
    if (!ptr1 || !ptr2)
    {
        printf("Allocation failed\n");
        vPortFree(ptr1);
        vPortFree(ptr2);
        return;
    }
    *ptr1 = *ptr2;
    vPortFree(ptr1);
    vPortFree(ptr2);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_oob_memset_2(void)
{
    char *ptr;
    size_t size = 8;

    printf("out-of-bounds in memset2\n");
    ptr = pvPortMalloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr + 7, 0, 2);
    vPortFree(ptr);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_oob_memset_4(void)
{
    char *ptr;
    size_t size = 8;

    printf("out-of-bounds in memset4\n");
    ptr = pvPortMalloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr + 5, 0, 4);
    vPortFree(ptr);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_oob_memset_8(void)
{
    char *ptr;
    size_t size = 8;

    printf("out-of-bounds in memset8\n");
    ptr = pvPortMalloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr + 1, 0, 8);
    vPortFree(ptr);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_oob_memset_16(void)
{
    char *ptr;
    size_t size = 16;

    printf("out-of-bounds in memset16\n");
    ptr = pvPortMalloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr + 1, 0, 16);
    vPortFree(ptr);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_oob_in_memset(void)
{
    char *ptr;
    size_t size = 666;

    printf("out-of-bounds in memset\n");
    ptr = pvPortMalloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    memset(ptr, 0, size + 5);
    vPortFree(ptr);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_uaf(void)
{
    char *ptr;
    size_t size = 10;

    printf("use-after-free\n");
    ptr = pvPortMalloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    vPortFree(ptr);
    *(ptr + 8) = 'x';
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_uaf_memset(void)
{
    char *ptr;
    size_t size = 33;

    printf("use-after-free in memset\n");
    ptr = pvPortMalloc(size);
    if (!ptr)
    {
        printf("Allocation failed\n");
        return;
    }

    vPortFree(ptr);
    memset(ptr, 0, size);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_uaf2(void)
{
    char *ptr1, *ptr2;
    size_t size = 43;

    printf("use-after-free after another malloc\n");
    ptr1 = pvPortMalloc(size);
    if (!ptr1)
    {
        printf("Allocation failed\n");
        return;
    }

    vPortFree(ptr1);
    ptr2 = pvPortMalloc(size);
    if (!ptr2)
    {
        printf("Allocation failed\n");
        return;
    }

    ptr1[40] = 'x';
    if (ptr1 == ptr2)
    {
        printf("Could not detect use-after-free: ptr1 == ptr2\n");
    }
    vPortFree(ptr2);
}

__attribute__((optimize("O0"), noinline)) static void rt_malloc_double_free_small(void)
{
    char *p = pvPortMalloc(100);

    printf("double free small test case\n");
    vPortFree(p);
    vPortFree(p);
}

static int cmd_kasan_global_oob(int argc, char **argv)
{
	kasan_global_oob();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_kasan_global_oob, kasan_global_oob, kasan_global_oob)

static int cmd_kasan_stack_oob(int argc, char **argv)
{
	kasan_stack_oob();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_kasan_stack_oob, kasan_stack_oob, kasan_stack_oob)

static int cmd_use_after_scope_test(int argc, char **argv)
{
	kasan_use_after_scope_test();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_use_after_scope_test, kasan_use_after_scope_test, kasan_use_after_scope)

static int cmd_kasan_alloca_oob_left(int argc, char **argv)
{
	kasan_alloca_oob_left();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_kasan_alloca_oob_left, kasan_alloca_oob_left, kasan_alloca_oob_left)

static int cmd_kasan_alloca_oob_right(int argc, char **argv)
{
	kasan_alloca_oob_right();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_kasan_alloca_oob_right, kasan_alloca_oob_right, kasan_alloca_oob_right)

/**
 * export CMD for lic malloc entry
 */
static int cmd_malloc_oob_right(int argc, char **argv)
{
	malloc_oob_right();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_oob_right, malloc_oob_right, malloc_oob_right)

static int cmd_malloc_oob_left(int argc, char **argv)
{
	malloc_oob_left();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_oob_left, malloc_oob_left, malloc_oob_left)

static int cmd_oob_realloc_more(int argc, char **argv)
{
	oob_realloc_more();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_oob_realloc_more, oob_realloc_more, oob_realloc_more)

static int cmd_oob_realloc_less(int argc, char **argv)
{
	oob_realloc_less();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_oob_realloc_less, oob_realloc_less, oob_realloc_less)

static int cmd_malloc_oob_access_16(int argc, char **argv)
{
	malloc_oob_access_16();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_oob_access_16, malloc_oob_access_16, malloc_oob_access_16)

static int cmd_malloc_oob_memset_2(int argc, char **argv)
{
	malloc_oob_memset_2();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_oob_memset_2, malloc_oob_memset_2, malloc_oob_memset_2)

static int cmd_malloc_oob_memset_4(int argc, char **argv)
{
	malloc_oob_memset_4();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_oob_memset_4, malloc_oob_memset_4, malloc_oob_memset_4)

static int cmd_malloc_oob_memset_8(int argc, char **argv)
{
	malloc_oob_memset_8();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_oob_memset_8, malloc_oob_memset_8, malloc_oob_memset_8)

static int cmd_malloc_oob_memset_16(int argc, char **argv)
{
	malloc_oob_memset_16();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_oob_memset_16, malloc_oob_memset_16, malloc_oob_memset_16)

static int cmd_malloc_oob_in_memset(int argc, char **argv)
{
	malloc_oob_in_memset();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_oob_in_memset, malloc_oob_in_memset, malloc_oob_in_memset)

static int cmd_malloc_uaf(int argc, char **argv)
{
	malloc_uaf();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_uaf, malloc_uaf, malloc_uaf)

static int cmd_malloc_uaf_memset(int argc, char **argv)
{
	malloc_uaf_memset();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_uaf_memset, malloc_uaf_memset, malloc_uaf_memset)

static int cmd_malloc_double_free_small(int argc, char **argv)
{
	malloc_double_free_small();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_double_free_small, malloc_double_free_small, malloc_double_free_small)

static int cmd_malloc_double_free_large(int argc, char **argv)
{
	malloc_double_free_large();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_malloc_double_free_large, malloc_double_free_large, malloc_double_free_large)

/**
 * export CMD for FreeRTOS malloc entry
 */
static int cmd_rt_malloc_oob_right(int argc, char **argv)
{
	rt_malloc_oob_right();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_oob_right, rt_malloc_oob_right, rt_malloc_oob_right)

static int cmd_rt_malloc_oob_left(int argc, char **argv)
{
	rt_malloc_oob_left();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_oob_left, rt_malloc_oob_left, rt_malloc_oob_left)

static int cmd_rt_oob_realloc_more(int argc, char **argv)
{
	rt_oob_realloc_more();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_oob_realloc_more, rt_oob_realloc_more, rt_oob_realloc_more)

static int cmd_rt_oob_realloc_less(int argc, char **argv)
{
	rt_oob_realloc_less();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_oob_realloc_less, rt_oob_realloc_less, rt_oob_realloc_less)

static int cmd_rt_malloc_oob_access_16(int argc, char **argv)
{
	rt_malloc_oob_access_16();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_oob_access_16, rt_malloc_oob_access_16, rt_malloc_oob_access_16)

static int cmd_rt_malloc_oob_memset_2(int argc, char **argv)
{
	rt_malloc_oob_memset_2();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_oob_memset_2, rt_malloc_oob_memset_2, rt_malloc_oob_memset_2)

static int cmd_rt_malloc_oob_memset_4(int argc, char **argv)
{
	rt_malloc_oob_memset_4();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_oob_memset_4, rt_malloc_oob_memset_4, rt_malloc_oob_memset_4)

static int cmd_rt_malloc_oob_memset_8(int argc, char **argv)
{
	rt_malloc_oob_memset_8();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_oob_memset_8, rt_malloc_oob_memset_8, rt_malloc_oob_memset_8)

static int cmd_rt_malloc_oob_memset_16(int argc, char **argv)
{
	rt_malloc_oob_memset_16();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_oob_memset_16, rt_malloc_oob_memset_16, rt_malloc_oob_memset_16)

static int cmd_rt_malloc_oob_in_memset(int argc, char **argv)
{
	rt_malloc_oob_in_memset();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_oob_in_memset, rt_malloc_oob_in_memset, rt_malloc_oob_in_memset)

static int cmd_rt_malloc_uaf(int argc, char **argv)
{
	rt_malloc_uaf();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_uaf, rt_malloc_uaf, rt_malloc_uaf)

static int cmd_rt_malloc_uaf_memset(int argc, char **argv)
{
	rt_malloc_uaf_memset();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_uaf_memset, rt_malloc_uaf_memset, rt_malloc_uaf_memset)

static int cmd_rt_malloc_double_free_small(int argc, char **argv)
{
	rt_malloc_double_free_small();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rt_malloc_double_free_small, rt_malloc_double_free_small, rt_malloc_double_free_small)
