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
#include "amp_user_resource.h"
#include <awlog.h>

#ifdef AMP_USER_RESOURCE_0
USER_RSC_DATA_BUF_VAR_DEFINITION(0);
#endif

#ifdef AMP_USER_RESOURCE_1
USER_RSC_DATA_BUF_VAR_DEFINITION(1);
#endif

#ifdef AMP_USER_RESOURCE_2
USER_RSC_DATA_BUF_VAR_DEFINITION(2);
#endif

#ifdef AMP_USER_RESOURCE_3
USER_RSC_DATA_BUF_VAR_DEFINITION(3);
#endif


#define GET_USER_RSC(user_rsc_id, user_rsc_type) \
	if (id == user_rsc_id) \
	{ \
		user_rsc->id = user_rsc_id; \
		user_rsc->type = user_rsc_type; \
		user_rsc->part_name = USER_RSC_PARTITION_NAME(user_rsc_id); \
		user_rsc->len = USER_RSC_DATA_LEN(user_rsc_id); \
		user_rsc->buf = USER_RSC_DATA_BUF_ADDR(user_rsc_id); \
		return 0; \
	}

int get_amp_user_resource(uint32_t id, amp_user_resource_t *user_rsc)
{
	if (!user_rsc)
		return -1;

#ifdef AMP_USER_RESOURCE_0
	GET_USER_RSC(0, USER_RSC_TYPE_PARTITION);
#endif
#ifdef AMP_USER_RESOURCE_1
	GET_USER_RSC(1, USER_RSC_TYPE_PARTITION);
#endif
#ifdef AMP_USER_RESOURCE_2
	GET_USER_RSC(2, USER_RSC_TYPE_PARTITION);
#endif
#ifdef AMP_USER_RESOURCE_3
	GET_USER_RSC(3, USER_RSC_TYPE_PARTITION);
#endif

	return -3;
}

void show_user_resource(uint32_t id)
{
	int ret;
	amp_user_resource_t user_rsc;

	ret = get_amp_user_resource(id, &user_rsc);
	if (ret)
	{
		printf("get_amp_user_resource failed, id: %u, ret: %d\n", id, ret);
		return;
	}

	printf("AMP User Resource:\n");
	printf("ID: %u\n", user_rsc.id);
	printf("Type: %d\n", user_rsc.type);
	printf("Partition Name: '%s'\n", user_rsc.part_name);
	printf("Buffer Addr: %p\n", user_rsc.buf);
	printf("Data Length: %u\n", user_rsc.len);
	printf("Data:\n");
	aw_hexdump((char *)user_rsc.buf, user_rsc.len);
}

void show_all_user_resource(void)
{
#ifdef AMP_USER_RESOURCE_0
	show_user_resource(0);
#endif
#ifdef AMP_USER_RESOURCE_1
	show_user_resource(1);
#endif
#ifdef AMP_USER_RESOURCE_2
	show_user_resource(2);
#endif
#ifdef AMP_USER_RESOURCE_3
	show_user_resource(3);
#endif
}
