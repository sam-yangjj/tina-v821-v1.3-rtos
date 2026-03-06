/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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
#include <console.h>

#define MAX_BUFFER 200

static void show_help(void)
{
    printf("Usage: grep <key_string> <file>\n");
}

int grep_main(int argc,char **argv)
{
    char buffer[MAX_BUFFER];
    FILE *fp;
    int res_sum = 0;
    char *buf_ptr = NULL;
    char const *key_ptr = NULL;

    if (argc < 3) {
        show_help();
        return -1;
    }

    if (strlen(argv[2]) == 0) {
        show_help();
        return -1;
    }

    fp = fopen(argv[2],"r");
    if (!fp) {
        printf("open file %s failed\n", argv[2]);
        return -1;
    }
    while (fgets(buffer, sizeof(buffer), fp) != 0) {
        buf_ptr= buffer;
        key_ptr= argv[1];
        while (*buf_ptr!= '\0') {
            if ((*buf_ptr == '\0') | (*key_ptr == '\0'))
                break;
            if (*buf_ptr == *key_ptr) {
                key_ptr++;
                if (*key_ptr == '\0') {
                    printf("%s",buffer);
                    res_sum++;
                }
            } else {
                key_ptr = argv[1];
            }
            buf_ptr++;
        }
    }
    fclose(fp);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(grep_main, grep, search string from file);
