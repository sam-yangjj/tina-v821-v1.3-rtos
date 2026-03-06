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
#include "amp_user_memory.h"
#include <awlog.h>

typedef struct amp_user_memory_internal
{
	uint32_t len;
	void *addr;
} amp_user_memory_internal_t;

#define USER_MEM_OBJECT_DEFINITION(id) static amp_user_memory_internal_t g_user_mem_##id

#ifdef AMP_USER_MEMORY_0
USER_MEM_OBJECT_DEFINITION(0);
#endif

#ifdef AMP_USER_MEMORY_1
USER_MEM_OBJECT_DEFINITION(1);
#endif

#ifdef AMP_USER_MEMORY_2
USER_MEM_OBJECT_DEFINITION(2);
#endif

#ifdef AMP_USER_MEMORY_3
USER_MEM_OBJECT_DEFINITION(3);
#endif

#define IS_USER_MEM_EXIST(user_mem_id) \
	if (id == user_mem_id) \
	{ \
		return 1;\
	}

#define SAVE_USER_MEM_INFO(user_mem_id) \
	if (id == user_mem_id) \
	{ \
		g_user_mem_##user_mem_id.addr = addr; \
		g_user_mem_##user_mem_id.len = len; \
		return 0; \
	}

#define GET_USER_MEM(user_mem_id) \
	if (id == user_mem_id) \
	{ \
		if (!g_user_mem_##user_mem_id.len) \
			return -3; \
		user_mem->id = user_mem_id; \
		user_mem->name = USER_MEM_NAME(user_mem_id); \
		user_mem->len = g_user_mem_##user_mem_id.len; \
		user_mem->addr = g_user_mem_##user_mem_id.addr; \
		return 0; \
	}

int is_amp_user_memory_exist(uint32_t id)
{
	if (id >= AMP_USER_MEMORY_MAX_NUM)
		return 0;

#ifdef AMP_USER_MEMORY_0
	IS_USER_MEM_EXIST(0);
#endif
#ifdef AMP_USER_MEMORY_1
	IS_USER_MEM_EXIST(1);
#endif
#ifdef AMP_USER_MEMORY_2
	IS_USER_MEM_EXIST(2);
#endif
#ifdef AMP_USER_MEMORY_3
	IS_USER_MEM_EXIST(3);
#endif

	return 0;
}

int save_amp_user_memory_info(uint32_t id, void *addr, uint32_t len)
{
	if (id >= AMP_USER_MEMORY_MAX_NUM)
		return -1;

	if (!addr)
		return -2;

	if (len == 0)
		return -3;

#ifdef AMP_USER_MEMORY_0
	SAVE_USER_MEM_INFO(0);
#endif
#ifdef AMP_USER_MEMORY_1
	SAVE_USER_MEM_INFO(1);
#endif
#ifdef AMP_USER_MEMORY_2
	SAVE_USER_MEM_INFO(2);
#endif
#ifdef AMP_USER_MEMORY_3
	SAVE_USER_MEM_INFO(3);
#endif

	return -4;
}

int get_amp_user_memory(uint32_t id, amp_user_memory_t *user_mem)
{
	if (!user_mem)
		return -1;

#ifdef AMP_USER_MEMORY_0
	GET_USER_MEM(0);
#endif
#ifdef AMP_USER_MEMORY_1
	GET_USER_MEM(1);
#endif
#ifdef AMP_USER_MEMORY_2
	GET_USER_MEM(2);
#endif
#ifdef AMP_USER_MEMORY_3
	GET_USER_MEM(3);
#endif

	return -2;
}

void show_amp_user_memory(uint32_t id)
{
	int ret;
	amp_user_memory_t user_mem;

	ret = get_amp_user_memory(id, &user_mem);
	if (ret)
	{
		printf("get_amp_user_memory failed, id: %u, ret: %d\n", id, ret);
		return;
	}

	printf("AMP User Memory:\n");
	printf("ID: %u\n", user_mem.id);
	printf("Name: '%s'\n", user_mem.name);
	printf("Buffer Addr: %p\n", user_mem.addr);
	printf("Data Length: %u\n", user_mem.len);
	printf("Data:\n");
	aw_hexdump((char *)user_mem.addr, user_mem.len);
}

void show_all_amp_user_memory(void)
{
#ifdef AMP_USER_MEMORY_0
	show_amp_user_memory(0);
#endif
#ifdef AMP_USER_MEMORY_1
	show_amp_user_memory(1);
#endif
#ifdef AMP_USER_MEMORY_2
	show_amp_user_memory(2);
#endif
#ifdef AMP_USER_MEMORY_3
	show_amp_user_memory(3);
#endif
}
