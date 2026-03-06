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

#define MV_BUFSIZE (256)

static void show_help(void)
{
    printf("Usage: mv <from> <to>\n");
}

static int mv_main(int argc, char **argv)
{
    char *from = NULL, *to = NULL;
    int fd_from = 0, fd_to = 0;
    char buf[MV_BUFSIZE] = {0};
    int rlen = 0, wlen = 0, ret = -1, isdir = false;
    struct stat s;

    if (argc != 3) {
        show_help();
        return -1;
    }

    from = argv[1];
    to = argv[2];

    ret = stat(from, &s);
    if (ret) {
        printf("stat %s failed - %d\n", from, ret);
        return -1;
    }

    if (S_ISDIR(s.st_mode)) {
        char *p = strrchr(from, '/');
	char *tmp;

        tmp = malloc(512);
        if (!tmp)
            return -1;

        isdir = true;
        if (!p)
            ret = sprintf(tmp, "%s/%s", to, from);
        else
            ret = sprintf(tmp, "%s%s", to, p);
        if (ret < 0) {
            printf("sprintf failed\n");
            goto free_to;
        }
	to = tmp;
    }

    ret = rename(from, to);
    if (ret < 0 && errno != EXDEV) {
        printf("rename %s to %s failed - %s\n", from, to, strerror(errno));
        goto free_to;
    } else if (ret == 0) {
        goto free_to;
    }

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0) {
        printf("open %s failed - %s\n", from, strerror(errno));
        ret = -1;
        goto free_to;
    }

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd_to < 0) {
        printf("open %s failed - %s\n", to, strerror(errno));
        goto close_from;
    }

    while ((rlen = read(fd_from, buf, MV_BUFSIZE))) {
        if (rlen < 0) {
            printf("read %s failed - %s\n", from, strerror(errno));
            goto close_to;
        }

        wlen = write(fd_to, buf, rlen);
        if (wlen != rlen) {
            printf("write %s failed - %s\n", to, strerror(errno));
            goto close_to;
        }
    }

    ret = unlink(from);
    if (ret) {
        printf("unlink %s failed - %s\n", from, strerror(errno));
        goto close_to;
    }

    ret = 0;
close_to:
    close(fd_to);
close_from:
    close(fd_from);
free_to:
    if (isdir)
        free(to);
    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(mv_main, mv, move file);
