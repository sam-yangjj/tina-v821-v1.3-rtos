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
#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#define   DATA_TYPE_SINGLE_WORD  (1)
#define   DATA_TYPE_STRING       (2)
#define   DATA_TYPE_MULTI_WORD   (3)
#define   DATA_TYPE_GPIO_WORD    (4)

#define   SCRIPT_PARSER_OK       (0)
#define   SCRIPT_PARSER_EMPTY_BUFFER    (-1)
#define   SCRIPT_PARSER_KEYNAME_NULL    (-2)
#define   SCRIPT_PARSER_DATA_VALUE_NULL (-3)
#define   SCRIPT_PARSER_KEY_NOT_FIND    (-4)
#define   SCRIPT_PARSER_BUFFER_NOT_ENOUGH (-5)

typedef struct
{
	int main_key_count;
	int version[3];
} script_head_t;

typedef struct
{
	char name[32];
	int  lenth;
	int  offset;
} script_main_key_t;

typedef struct
{
	char name[32];
	int  offset;
	int  pattern;
} script_sub_key_t;

typedef struct
{
	char name[32];
	int  port;
	int  port_num;
	int  mul_sel;
	int  pull;
	int  drv_level;
	int  data;
} user_gpio_set_t;

typedef struct
{
	char  *script_mod_buf;
	int    script_mod_buf_size;
	int    script_main_key_count;
} script_parser_t;

script_parser_t *script_parser_init(void *script_buf);
int script_parser_exit(script_parser_t *parser);
int script_parser_fetch(script_parser_t *parser, const char *main_name,
				const char *sub_name, int value[], int count);
int script_parser_subkey_count(script_parser_t *parser, const char *main_name);
int script_parser_mainkey_count(script_parser_t *parser);
int script_parser_mainkey_get_gpio_count(script_parser_t *parser,
				const char *main_name);
int script_parser_mainkey_get_gpio_cfg(script_parser_t *parser, const char *main_name,
				user_gpio_set_t *gpio_cfg, int gpio_count);
void script_show(script_parser_t *parser);

#endif
