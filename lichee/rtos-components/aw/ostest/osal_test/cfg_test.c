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
#include <hal_cfg.h>
#include <script.h>
#include <hal_cmd.h>

#include "cfg_test.h"

int cmd_cfg_test(int argc, char **argv)
{
    int ret = -1;
    int count = 0;
    user_gpio_set_t gpio_cfg[2] = {0};
    int32_t Value[1] = {0};

    ret = hal_cfg_init();
    if (!ret) {
        printf("hal_cfg_init success\n");
    } else {
        printf("hal_cfg_init fail\n");
        return -CFG_TEST_INIT_FAILED;
    }

    ret = hal_cfg_get_keyvalue(SEC_NAME, KEY_NAME, Value, 1);
    if (ret == 0 && (Value[0] == TEST_KEY_VALUE)) {
        printf("hal_cfg_get_keyvalue success, %s:%d\n", KEY_NAME, Value[0]);
    } else {
        printf("hal_cfg_get_keyvalue fail， ret = %d\n", ret);
        ret = -CFG_TEST_GET_KEYVALUE_FAILED;
        goto out;
    }

    count = hal_cfg_get_sec_keycount(SEC_NAME);
    if (count == TEST_KEY_COUNT_VALUE) {
        printf("hal_cfg_get_sec_keycount get success, %s key count:%d\n", SEC_NAME, count);
    } else {
        printf("hal_cfg_get_sec_keycount fail， ret = %d\n", ret);
        ret = -CFG_TEST_GET_SEC_KEY_COUNT_FAILED;
        goto out;
    }

    count = hal_cfg_get_sec_count();
    if (count) {
        printf("hal_cfg_get_sec_count get success:%d\n", count);
    } else {
        printf("hal_cfg_get_sec_count fail， ret = %d\n", ret);
        ret = -CFG_TEST_GET_SEC_COUNT_FAILED;
        goto out;
    }

    count = hal_cfg_get_gpiosec_keycount(SEC_NAME);
    if (count == TEST_GPIO_SEC_COUNT) {
        printf("hal_cfg_get_gpiosec_keycount get success, %s gpio count:%d\n", SEC_NAME, count);
    } else {
        printf("hal_cfg_get_gpiosec_keycount fail， count = %d\n", count);
        ret = -CFG_TEST_GET_GPIOSEC_KEY_COUNT_FAILED;
        goto out;
    }

    ret = hal_cfg_get_gpiosec_data(SEC_NAME, &gpio_cfg, count);
    if (ret == 0) {
        if (gpio_cfg[0].port_num == TEST_GPIO_PORT_NUM) {
            printf("hal_cfg_get_gpiosec_data success, %s port num:%d\n", SEC_NAME, gpio_cfg[0].port_num);
        } else {
            printf("hal_cfg_get_gpiosec_data fail\n");
            ret = -CFG_TEST_GET_GPIOSEC_KEY_DATA_FAILED;
            goto out;
        }
    } else {
        printf("hal_cfg_get_keyvalue fail， ret = %d\n", ret);
        ret = -CFG_TEST_GET_GPIOSEC_KEY_DATA_FAILED;
        goto out;
    }

    //hal_cfg_show(void);

    ret = hal_cfg_exit();
    if (ret == 0) {
        printf("hal_cfg_exit success\n");
        printf("hal_cfg test success!\n");
        ret = CFG_TEST_RET_OK;
    } else {
        printf("hal_cfg_exit fail\n");
        ret = -CFG_TEST_EXIT_FAILED;
    }

    return ret;

out:
    hal_cfg_exit();
    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_cfg_test, test_cfg, cfg tests);
