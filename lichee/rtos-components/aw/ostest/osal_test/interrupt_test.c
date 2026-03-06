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
#include <stdlib.h>
#include <string.h>
#include <hal_interrupt.h>
#include <hal_cmd.h>
#include <hal_osal.h>
#include <sunxi_hal_common.h>

#include "interrupt_test.h"

/*
test api :
    hal_interrupt_clear_pending();
    hal_interrupt_is_pending();
    hal_interrupt_set_pending();
    hal_request_irq();
    hal_free_irq();
    arch_enable_irq();
    hal_disable_irq();
    hal_interrupt_enable();
    hal_interrupt_disable();
    hal_interrupt_disable_irqsave();
    hal_interrupt_enable_irqrestore();
*/
int cmd_test_interrupt(int argc, char **argv)
{
    int ret, i, status, irq_num;
    unsigned long flag = 0;

    /*
    ============================
    Test interrupt
    ============================
    */
    flag = hal_interrupt_disable_irqsave();

    ret = hal_interrupt_is_disable();
    if (ret) {
        printf("hal_interrupt_disable_irqsave ok!\n");
    } else {
        printf("hal_interrupt_disable_irqsave fail\n");
        return -INTERRUPT_TEST_RET_DISABLE_IRQSAVE_FAIL;
    }

    hal_interrupt_enable();
    ret = hal_interrupt_is_disable();
    if (!ret) {
        printf("hal_interrupt_enable ok!\n");
    } else {
        printf("hal_interrupt_enable fail\n");
        return -INTERRUPT_TEST_RET_ENABLE_FAIL;
    }

    hal_interrupt_disable();
    ret = hal_interrupt_is_disable();
    if (ret) {
        printf("hal_interrupt_disable ok!\n");
    } else {
        printf("hal_interrupt_disable fail\n");
        return -INTERRUPT_TEST_RET_DISABLE_FAIL;
    }

    hal_interrupt_enable_irqrestore(flag);
    ret = hal_interrupt_is_disable();
    if (!ret) {
        printf("hal_interrupt_enable_irqrestore ok!\n");
    } else {
        printf("hal_interrupt_enable_irqrestore fail\n");
        return -INTERRUPT_TEST_RET_ENABLE_IRQSRORE;
    }



    /*
    ============================
    Test irq
    ============================
    */
    printf("test all irq\n");
    hal_interrupt_disable();
    for (i = IRQ_START; i < IRQ_NUM; i++) {
        irq_num = MAKE_IRQn(i, 0);
        status = GET_IRQ_STATUS(i);
        ret = hal_enable_irq(irq_num);
        if (ret) {
            printf("hal_enable_irq fail %d\n", ret);
            return ret;
        }
        if (GET_IRQ_STATUS(i)) {
            //printf("enable irq:%d ok!\n", i);
        } else {
            printf("enable irq:%d fail!\n", i);
            ret = -INTERRUPT_TEST_RET_IRQ_ENABLE_FAIL;
            goto irq_err;
        }

        hal_disable_irq(irq_num);
        if (GET_IRQ_STATUS(i)) {
            printf("disenable irq:%d fail!\n", i);
            ret = -INTERRUPT_TEST_RET_IRQ_ENABLE_FAIL;
            goto irq_err;
        }

#if defined(CONFIG_ARCH_ARM_CORTEX_M33)
        hal_interrupt_set_pending(i);
        ret = hal_interrupt_is_pending(i);
        if (ret) {
            //printf("hal_interrupt_set_pending ok!\n");
        } else {
            printf("hal_interrupt_set_pending fail\n");
            ret = -INTERRUPT_TEST_RET_SET_PENDING_FAIL;
            goto irq_err;
        }

        hal_interrupt_clear_pending(i);
        ret = hal_interrupt_is_pending(i);
        if (!ret) {
            //printf("hal_interrupt_clear_pending ok!\n");
        } else {
            printf("hal_interrupt_clear_pending fail\n");
            ret = -INTERRUPT_TEST_RET_CLEAN_PENDING_FAIL;
            goto irq_err;
        }
#endif
        if (status)
            hal_enable_irq(irq_num);

    }
    hal_interrupt_enable();
    printf("test all irq ok\n");

    printf("Interrupt test success!\n");

    return INTERRUPT_TEST_RET_OK;

irq_err:
    hal_interrupt_enable();
    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_interrupt, test_interrupt, inteerupt api tests);
