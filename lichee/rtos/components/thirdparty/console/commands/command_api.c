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
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "console.h"

#define FINSH_NEXT_SYSCALL(index)  index++
#define FINSH_NEXT_SYSVAR(index)   index++

struct finsh_syscall* finsh_syscall_lookup(const char* name)
{
    struct finsh_syscall* index;

    for (index = (struct finsh_syscall *)&_syscall_table_begin;
        (unsigned long)index < (unsigned long)&_syscall_table_end;
        FINSH_NEXT_SYSCALL(index))
    {
        if (strcmp(index->name, name) == 0)
            return index;
        if (strncmp(index->name, "__cmd_", 6) == 0) {
            if (strcmp(&index->name[6], name) == 0)
                return index;
        }
    }
    return NULL;
}

void finsh_syscall_show(void)
{
    struct finsh_syscall* index;

    for (index = (struct finsh_syscall *)&_syscall_table_begin;
        (unsigned long)index < (unsigned long)&_syscall_table_end;
        FINSH_NEXT_SYSCALL(index))
    {
        printf("[%20s]--------------%s\n",
            index->name,
            index->desc);
        printf("\n");
    }
}

enum ParseState
{
    PS_WHITESPACE,
    PS_TOKEN,
    PS_STRING,
    PS_ESCAPE
};

void console_parseargs(char *argstr, int *argc_p, char **argv, char **resid)
{
    int argc = 0;
    char c = 0;
    enum ParseState stackedState = PS_WHITESPACE;
    enum ParseState lastState = PS_WHITESPACE;

    /* tokenize the argstr */
    while ((c = *argstr) != 0)
    {
        enum ParseState newState;

        if (c == ';' && lastState != PS_STRING && lastState != PS_ESCAPE)
        {
            break;
        }

        if (lastState == PS_ESCAPE)
        {
            newState = stackedState;
        }
        else if (lastState == PS_STRING)
        {
            if (c == '"')
            {
                newState = PS_WHITESPACE;
                *argstr = 0;
            }
            else
            {
                newState = PS_STRING;
            }
        }
        else if ((c == ' ') || (c == '\t'))
        {
            /* whitespace character */
            *argstr = 0;
            newState = PS_WHITESPACE;
        }
        else if (c == '"')
        {
            newState = PS_STRING;
            *argstr++ = 0;
            argv[argc++] = argstr;
        }
        else
        {
            if (lastState == PS_WHITESPACE)
            {
                argv[argc++] = argstr;
            }
            newState = PS_TOKEN;
        }

        lastState = newState;
        argstr++;
    }

    argv[argc] = NULL;

    if (argc_p != NULL)
    {
        *argc_p = argc;
    }

    if (*argstr == ';')
    {
        *argstr++ = '\0';
    }

    *resid = argstr;
}

portBASE_TYPE prvCommandEntry( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    struct finsh_syscall *cmd_syscall = NULL;
    BaseType_t xReturn = pdFALSE;

    int argc = 0;
    char *argv[SH_MAX_CMD_ARGS];
    char *resid = 0;

    char *buf;
    char temp [SH_MAX_CMD_LEN] = {0};
    int i;
    int cmdlen = 0;

    optarg = NULL;
    optind = opterr = optopt = 0;

    cmdlen = (strlen(pcCommandString) + 1) > SH_MAX_CMD_LEN ? SH_MAX_CMD_LEN : (strlen(pcCommandString) + 1);

    memcpy(temp, pcCommandString, cmdlen);
    buf = (char *)&temp;

    for (i = 0; i < SH_MAX_CMD_ARGS; i++)
    {
        argv[i] = NULL;
    }

    console_parseargs(buf, &argc, argv, &resid);

    if(argc <= 0)
    {
        return pdFALSE;
    }

    cmd_syscall = finsh_syscall_lookup(argv[0]);
    if(cmd_syscall && cmd_syscall->func)
    {
        cmd_syscall->func(argc, argv);
        xReturn = pdTRUE;
    }
    else
    {
        xReturn = pdFALSE;
    }
    return xReturn;
}
