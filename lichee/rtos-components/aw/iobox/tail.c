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
#include <stdio.h>
#include <tinatest.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <console.h>
#include <stdbool.h>
#include <fcntl.h>

#define LINE_MAX_SIZE (1024)

static void show_help(void)
{
    printf("Usage: tail [-n line_cnt] <file>\n");
}

int tail_do(const char *file, unsigned int lcnt)
{
    char linebuf[LINE_MAX_SIZE];
    unsigned int num;
    int ret, fd;

    fd = open(file, O_RDONLY);
    if(fd < 0) {
        printf("Can not open the file %s\n", file);
        return -1;
    }

    lseek(fd, -1, SEEK_END);

    num = 0;
    ret = -1;
    while (true) {
        off_t off;
        char c;

        if (read(fd, &c, 1) != 1) {
            printf("read %s failed\n", file);
            goto err;
        }
        if(c == '\n')
             num++;

        if(num > lcnt)
             break;

        off = lseek(fd, -2, SEEK_CUR);
        if (off < 0) {
            printf("lseek %s failed\n", file);
            goto err;
        } else if(off == 0) {
            break;
        }
    }

    while (true) {
        memset(linebuf, 0, LINE_MAX_SIZE);
        ret = read(fd, linebuf, LINE_MAX_SIZE);
        if (ret > 0)
            printf("%s", linebuf);
        else if (ret == 0)
            break;
        else
            goto err;
    }

    ret = 0;
err:
    close(fd);
    return ret;
}

int tail_main(int argc, char **argv)
{
    int opts = 0;
    unsigned int lcnt = (unsigned int)(-1);

    optind = 0;
    while ((opts = getopt(argc, argv, ":hn:")) != EOF) {
        switch (opts) {
        case 'n': lcnt = atoi(optarg); break;
        case 'h': show_help(); return 0;
        case '?': printf("invalid option %c\n", optopt); return -1;
        case ':':
            printf("option -%c requires an argument\n", optopt);
            show_help();
            return -1;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc == 0) {
        printf("Please enter file\n");
        show_help();
        return -1;
    }

    return tail_do(argv[0], lcnt);
}
FINSH_FUNCTION_EXPORT_CMD(tail_main, tail, read file from tail);
