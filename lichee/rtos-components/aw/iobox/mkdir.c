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
#include <stdlib.h>

#define MKDIR_FLAGS_PARENTS (1 << 0)
static void show_help(void)
{
    printf("Usage: mkdir [-p] <dir>\n");
    printf("  -p: make the upper directory if needed.\n");
}

static int mkdir_do(char *path, int flags)
{
    char *s = NULL;

    if (flags & MKDIR_FLAGS_PARENTS) {
        s = path;
    }
    /*
     * All of operations must base on root directory
     * As alios has not root dierctory, we can operate '/data' but not '/'
     */
    if (path[0] == '.') {
        if (path[1] == '\0')
            return 0;
        if (path[1] == '.' && path[2] == '\0')
            return 0;
    }

    while (1) {
        struct stat st;

        if (flags & MKDIR_FLAGS_PARENTS) {
            /* in case of tailing '/', such as '/data/a/' */
            if (*(s++) == '\0')
                break;
            s = strchr(s, '/');
            if (s)
                *s = '\0';
        }

        if (!stat(path, &st)) {
            if (S_ISDIR(st.st_mode))
                goto next;
            printf("make failed - %s already existed and not direcotry\n", path);
            return -1;
        }

        if (mkdir(path, 0777) < 0) {
            printf("mkdir %s failed\n", path);
            return -1;
        }

next:
        if (!s)
            break;
        *s = '/';
    }
    return 0;
}

static int mkdir_main(int argc, char **argv)
{
    int opts = 0, ret = 0, flags = 0, index;

    optind = 0;
    while ((opts = getopt(argc, argv, ":ph")) != EOF) {
        switch (opts) {
        case 'p': flags |= MKDIR_FLAGS_PARENTS; break;
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

    if (argc < 1) {
        show_help();
        return -1;
    }

    for (index = 0; index < argc; index++) {
        char *path;

        path = strdup(argv[index]);
        ret = mkdir_do(path, flags);
        free(path);
        if (ret)
            return ret;
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(mkdir_main, mkdir, make directory);
