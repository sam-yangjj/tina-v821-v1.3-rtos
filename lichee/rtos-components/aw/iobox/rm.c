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

static void show_help(void)
{
    printf("Usage: rm [-r] <file1> [<file2>...]\n");
}

int rrmdir(const char *path)
{
    struct stat s;
    DIR *pdir = NULL;
    struct dirent *entry = NULL;
    int ret = -1;
    char *dir, *p;

    if (!path)
        return -EINVAL;

    dir = strdup(path);
    p = dir + strlen(dir) - 1;
    while (*p == '/' && p > dir) {
        *p = '\0';
        p--;
    }

    if (stat(dir, &s) || !S_ISDIR(s.st_mode)) {
        printf("%s not existed or not directory\n", dir);
        goto out;
    }

    pdir = opendir(dir);
    if (!pdir) {
        printf("opendir %s failed - %s\n", dir, strerror(errno));
        goto out;
    }

    ret = 0;
    while (ret == 0 && (entry = readdir(pdir))) {
        char fpath[128];

        snprintf(fpath, 128, "%.*s/%.*s", 63, dir, 63, entry->d_name);

        ret = stat(fpath, &s);
        if (ret) {
            printf("stat %s failed\n", fpath);
            break;
        }

        if (!strcmp(entry->d_name, "."))
            continue;
        if (!strcmp(entry->d_name, ".."))
            continue;

        if (S_ISDIR(s.st_mode))
            ret = rrmdir(fpath);
        else
            ret = unlink(fpath);
    }

    closedir(pdir);
    if (ret == 0) {
        ret = rmdir(dir);
        if (ret)
            printf("rmdir %s failed\n", dir);
    }
out:
    free(dir);
    return ret;
}

#define RM_FLAG_RECURSION (1 << 0)
static int rm_main(int argc, char **argv)
{
    int opts = 0, flags = 0, index, ret = 0;

    optind = 0;
    while ((opts = getopt(argc, argv, ":r")) != EOF) {
        switch (opts) {
        case 'r': flags |= RM_FLAG_RECURSION; break;
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
        struct stat s;
        char *path = argv[index];

        if (!stat(path, &s) && S_ISREG(s.st_mode)) {
            printf("remove %s\n", path);
            ret |= unlink(path);
            continue;
        }
        if (flags & RM_FLAG_RECURSION) {
            printf("remove dir %s\n", path);
            ret |= rrmdir(path);
            continue;
        }
        ret |= -ENOENT;
        printf("%s is not existed or directory\n", path);
    }

    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(rm_main, rm, remove file);
