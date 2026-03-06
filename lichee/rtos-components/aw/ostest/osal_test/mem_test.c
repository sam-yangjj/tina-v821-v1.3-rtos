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
#include <stdlib.h>
#include <string.h>
#include <hal_mem.h>
#include <hal_cmd.h>
#include <sunxi_hal_common.h>

#include "mem_test.h"

#define TEST_MEM_SIZE	128
#define TEST_MEM_ALIGN_SIZE1 32
#define TEST_MEM_ALIGN_SIZE2 64

int cmd_test_mem(int argc, char **argv)
{
	int i, flag = 0;
    char *pm1, *pm2;

    pm2 = pm1 = hal_malloc(TEST_MEM_SIZE);
	if (pm1 == NULL) {
		printf ("hal_malloc data buffer fail\n");
		return -MEM_TEST_MALLOC_FAILED;
	}
	printf ("hal_malloc data buffer success\n");

    for (i = 0; i < TEST_MEM_SIZE; i++)
        *pm2++ = 'X';
    pm2 = pm1;
    for (i = 0; i < TEST_MEM_SIZE; i++)
    {
        if (*pm2++ != 'X')
        {
            flag = 1;
        }
    }

	if (flag) {
		printf("Mem data error\n");
		hal_free(pm1);
		return -MEM_TEST_FAILED;
	}
	hal_free(pm1);
	pm1 = NULL;
	printf ("hal_malloc free data buffer success\n");
	printf ("hal_malloc test success\n");

	pm2 = pm1 = hal_malloc_align(TEST_MEM_SIZE, TEST_MEM_ALIGN_SIZE1);
	if (pm1 == NULL) {
		printf (" hal_malloc_align %d data buffer fail\n", TEST_MEM_ALIGN_SIZE1);
		return -MEM_TEST_MALLOC_ALIGN_FAILED;
	}
	if ((uintptr_t)pm1 % TEST_MEM_ALIGN_SIZE1) {
		printf (" hal_malloc_align data not align to %d\n", TEST_MEM_ALIGN_SIZE1);
		hal_free_align(pm1);
		return -MEM_TEST_MALLOC_NOT_ALIGN;
	}

	printf ("hal_malloc_align %d data buffer success\n", TEST_MEM_ALIGN_SIZE1);

    for (i = 0; i < TEST_MEM_SIZE; i++)
        *pm2++ = 'X';
    pm2 = pm1;
    for (i = 0; i < TEST_MEM_SIZE; i++)
    {
        if (*pm2++ != 'X')
        {
            flag = 1;
        }
    }

    hal_free_align(pm1);
	pm1 = NULL;
	printf ("hal_malloc_align %d free data buffer success\n", TEST_MEM_ALIGN_SIZE1);

	pm2 = pm1 = hal_malloc_align(TEST_MEM_SIZE, TEST_MEM_ALIGN_SIZE2);
	if (pm1 == NULL) {
		printf (" hal_malloc_align  %d data buffer fail\n", TEST_MEM_ALIGN_SIZE2);
		return -MEM_TEST_MALLOC_ALIGN_FAILED;
	}
	if ((uintptr_t)pm1 % TEST_MEM_ALIGN_SIZE2) {
		printf (" error: hal_malloc_align %p not align to %d\n", pm1, TEST_MEM_ALIGN_SIZE2);
		hal_free_align(pm1);
		return -MEM_TEST_MALLOC_NOT_ALIGN;
	}
	printf ("hal_malloc_align %d data buffer success\n", TEST_MEM_ALIGN_SIZE2);

    for (i = 0; i < TEST_MEM_SIZE; i++)
        *pm2++ = 'X';
    pm2 = pm1;
    for (i = 0; i < TEST_MEM_SIZE; i++)
    {
        if (*pm2++ != 'X')
        {
            flag = 1;
        }
    }

    hal_free_align(pm1);
	printf ("hal_malloc_align %d free data buffer success\n", TEST_MEM_ALIGN_SIZE2);
	printf ("hal_malloc_align test success\n");

	if (flag) {
		printf("Mem align data error\n");
		return -MEM_TEST_FAILED;
	}

	printf("Mem test success\n");

	return MEM_TEST_RET_OK;

}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_mem, test_mem, mem api tests);

