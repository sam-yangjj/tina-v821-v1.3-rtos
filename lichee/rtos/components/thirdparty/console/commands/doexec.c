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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sunxi_hal_common.h>

#include <console.h>

#define MAX_BUFFER_SIZE 1024

portBASE_TYPE prvCommandEntry( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

static void show_usage(void)
{
	printf("Usage: exec filename\r\n");
}

static int cmd_exec(int argc, char **argv)
{
	char *filename = NULL;
	FILE *file = NULL;
	unsigned char *buffer = NULL;

	if (argc < 2) {
		show_usage();
		return -1;
	}

	buffer = hal_malloc_coherent(MAX_BUFFER_SIZE);
	if (!buffer) {
		printf("alloc memory failed!\r\n");
		return -1;
	}

	filename = argv[1];

	file = fopen(filename, "r");
	if (file == NULL) {
		printf("open %s failed!\r\n", filename);
		hal_free_coherent(file);
		return -1;
	}

	memset(buffer, 0, MAX_BUFFER_SIZE);

	while(fgets(buffer, MAX_BUFFER_SIZE - 1, file)) {
		buffer[strlen(buffer) - 1] = 0;
		prvCommandEntry(NULL, 0, buffer);
		memset(buffer, 0, MAX_BUFFER_SIZE);
	}

	fclose(file);
	hal_free_coherent(file);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_exec, exec, exec file);
