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
#include <sys/statfs.h>
#include <sys/types.h>
#include <dirent.h>

#define KB (1024ULL)
static void df_do_dir(const char *dir)
{
    struct statfs sfs;
    unsigned long long total, used, free;

    if (statfs(dir, &sfs) < 0) {
        printf("statfs %s failed\n", dir);
        return;
    }

    total = sfs.f_bsize * sfs.f_blocks / KB;
    free = sfs.f_bsize * sfs.f_bavail / KB;
    used = total - free;
    printf("%7llu%7llu%7llu%6llu%% %s\n", total, used, free,
            used * 100 / total, dir);
}

static int df_root(void)
{
    DIR *pdir;
    struct dirent *entry = NULL;

    pdir = opendir("/");
    if (!pdir)
        return -1;

    while ((entry = readdir(pdir))) {
        char fpath[128] = {0};

        snprintf(fpath, 128, "/%.*s", 126, entry->d_name);
        df_do_dir(fpath);
    }

    closedir(pdir);
    return 0;
}

static int df_main(int argc, char **argv)
{
    int i;

    printf("Unit(KB)\r\n");
    printf("%7s%7s%7s%7s %s\n", "Total", "Used", "Free", "Use%", "Mount");

    if (argc <= 1) {
        return df_root();
    }

    for (i = 1; i < argc; i++)
        df_do_dir(argv[i]);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(df_main, df, copy file);
