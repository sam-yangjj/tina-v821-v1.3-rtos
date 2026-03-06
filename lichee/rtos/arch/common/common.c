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

#include <hal_uart.h>

#include <compiler.h>

typedef struct {
    unsigned long start;
    unsigned long end;
} valid_address_t;

#ifdef CONFIG_ARCH_SUN20IW2
static const valid_address_t valid_addr[] = {
    { .start = 0x40000000, .end = 0x40600000 },
    { .start = 0x04000000, .end = 0x04100000 },
    { .start = 0x08000000, .end = 0x08800000 },
    { .start = 0x0c000000, .end = 0x0c800000 },
};
#else
static const valid_address_t valid_addr[] = {
};
#endif

__weak int check_virtual_address(unsigned long vaddr)
{
    int i;
    for (i = 0; i < sizeof(valid_addr) / sizeof(valid_addr[0]); i++) {
        if (vaddr >= valid_addr[i].start && vaddr < valid_addr[i].end)
        return 1;
    }
    return 0;
}

void dump_memory(unsigned int *buf, int len)
{
    int i;

    for (i = 0; i < len; i ++)
    {
        if ((i % 4) == 0)
            printf("\n\r0x%p: ", buf + i);
        printf("0x%08x ", buf[i]);
    }
    printf("\n\r");
    return;
}

void dump_register_memory(char *name, unsigned long addr, int len)
{
    if (check_virtual_address(addr) && check_virtual_address(addr + len * sizeof(int)))
    {
        printf("\r\ndump_memory:%s\r\n", name);
        dump_memory((unsigned int *)addr, len);
    }
    else
    {
        printf("\n\r%s register corrupted!!\n", name);
    }
    return;
}

#ifdef CONFIG_PANIC_CLI
int g_cli_direct_read = 0;

void panic_goto_cli(void)
{
    if (g_cli_direct_read > 0)
    {
        printf("%s can not be reentrant!\r\n", __func__);
        return;
    }
    g_cli_direct_read = 1;
#ifdef CONFIG_PANIC_CLI_PWD
    {
        char backdoor[] = "backdoor:";
        int check_pos = 0;
        char cRxedChar = 0;
        int len = strlen(backdoor);

        printf("please enter `%s` for panic cli\r\n", backdoor);
        while(1) {
            hal_uart_receive_polling(CONFIG_CLI_UART_PORT, (uint8_t *)&cRxedChar, 1);
            if (cRxedChar == backdoor[check_pos]) {
                check_pos++;
                if (check_pos == len)
                    break;
            } else {
                check_pos = 0;
            }
        }
    }
#endif
#ifdef CONFIG_COMPONENT_FINSH_CLI
    void finsh_thread_entry(void *parameter);
    finsh_thread_entry(NULL);
#elif defined(CONFIG_COMPONENT_CLI)
    void prvUARTCommandConsoleTask( void *pvParameters );
    prvUARTCommandConsoleTask(NULL);
#endif
    while(1);
}
#endif

__weak unsigned long __va_to_pa(unsigned long vaddr)
{
	return vaddr;
}
__weak unsigned long __pa_to_va(unsigned long paddr)
{
	return paddr;
}
