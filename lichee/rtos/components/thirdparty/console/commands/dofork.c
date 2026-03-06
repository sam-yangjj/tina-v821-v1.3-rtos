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
#include "console.h"
#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

struct fork_arg_t
{
    struct finsh_syscall * syscall;
    int argc;
    char *argv[SH_MAX_CMD_ARGS];
    char * argv_cmd;
};

static int priority = 15;
static uint32_t thread_size = 8 * 1024 * sizeof(StackType_t);

static int cmd_forkarg(int argc, char **argv)
{
    int opts = 0;
    int size = thread_size;
    int prio = priority;
    char *err = NULL;

    optind = 0;
    while ((opts = getopt(argc, argv, ":hs:p:")) != EOF)
    {
        switch (opts)
        {
            case 'h':
                printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                goto out;
            case 's':
            {
                size = strtoul(optarg, &err, 0);
                if (!size)
                {
                    printf("size %s is zero or invalid\n", optarg);
                    printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                    goto out;
                }
                err = NULL;
                break;
            }
            case 'p':
            {
                prio = strtoul(optarg, &err, 0);
                if (!prio)
                {
                    printf("priority %s is zero or invalid\n", optarg);
                    printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                    goto out;
                }
                err = NULL;
                break;
            }
            case '?':
                printf("invalid option %c\n", optopt);
                printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                goto out;
            case ':':
                printf("option -%c requires an argument\n", optopt);
                printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                goto out;
            default:
                break;
        }
    }

    if (prio > (configMAX_PRIORITIES - 1) || prio <= 0)
    {
        printf("The priority(%d) out of range[%d-%d]\n", prio, 0, configMAX_PRIORITIES - 1);
        goto out;
    }
    priority = prio;

    if ((size > (USHRT_MAX + 1) * sizeof(StackType_t)) || size <= 0)
    {
        printf("The stack size(%d) out of range[%d-%lu] (bytes)\n", size, 0, (long unsigned)(USHRT_MAX + 1) * sizeof(StackType_t));
        goto out;
    }
    thread_size = size;

    printf("fork command priority is %d, stack size is %d (bytes)\n", priority, thread_size);
out:
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_forkarg, forkarg, Set form command prority and stack size);

static void print_usage(void)
{
    printf("Unage:fork [command arg0 arg1 ...]\n \
        example:\n \
            fork help\n \
            fork ls -l /data\n \
            fork cd /data\n");
}

static void fork_thread_entry(void * arg)
{
    if(arg == NULL)
    {
        vTaskDelete(NULL);
    }

    struct fork_arg_t * fork_arg = arg;
    struct finsh_syscall * call = fork_arg->syscall;

    call->func(fork_arg->argc, fork_arg->argv);

    if(fork_arg->argv_cmd != NULL)
    {
        free(fork_arg->argv_cmd);
    }

    free(fork_arg);

    vTaskDelete(NULL);
}

int cmd_fork(int argc, char ** argv)
{
    portBASE_TYPE ret;
    struct finsh_syscall* call;
    char * command_name = NULL;
    int i;

    if(argc < 2)
    {
        print_usage();
        return -1;
    }

    command_name = argv[1];

    call = finsh_syscall_lookup(command_name);
    if(call == NULL)
    {
        printf("The command no exist!\n");
        return -1;
    }

    if(call->func == NULL)
    {
        printf("Command entry no exist\n");
        return -1;
    }

    struct fork_arg_t * fork_arg = malloc(sizeof(struct fork_arg_t));
    if(fork_arg == NULL)
    {
        printf("Alloc memory failed!\n");
        return -1;
    }
    memset(fork_arg, 0, sizeof(struct fork_arg_t));

    char * fork_args_cmd = malloc(SH_MAX_CMD_LEN);
    if(fork_args_cmd == NULL)
    {
        printf("Alloc memory failed!\n");
        free(fork_arg);
        return -1;
    }
    memset(fork_args_cmd, 0, SH_MAX_CMD_LEN);

    fork_arg->argv_cmd = fork_args_cmd;
    fork_arg->syscall = call;
    fork_arg->argc = argc - 1;

    for(i = 1; i < argc; i++)
    {
        fork_arg->argv[i - 1] = fork_args_cmd;
        memcpy(fork_args_cmd, argv[i], strlen(argv[i]));
        fork_args_cmd += (strlen(argv[i]) + 1);
    }

    ret = xTaskCreate(fork_thread_entry, call->name, thread_size / sizeof(StackType_t), (void *)fork_arg, priority, NULL);
    if (ret != pdPASS) {
	    printf("create task %s failed\r\n", call->name);
	    return -1;
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_fork, fork, Create a task to run: fork command_name arg1 arg2 ...);
