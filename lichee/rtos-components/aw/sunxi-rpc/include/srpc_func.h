/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi rpc
 * operation interface of watchdog timer for rpc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SUNXI_RPC_FUNC_H
#define SUNXI_RPC_FUNC_H

#include <aw_types.h>
#include <string.h>
#include <sys/errno.h>
#include <hal_waitqueue.h>

#define SRPC_MACRO_IS_ENABLED3(ignore_this, val, ...)		val
#define SRPC_MACRO_IS_ENABLED2(one_or_two_args)				SRPC_MACRO_IS_ENABLED3(one_or_two_args 1, 0)
#define SRPC_XXXX1											_YYYY,
#define SRPC_MACRO_IS_ENABLED1(macro)						SRPC_MACRO_IS_ENABLED2(SRPC_XXXX##macro)
#define SRPC_MACRO_IS_ENABLED(macro)						SRPC_MACRO_IS_ENABLED1(macro)

#define ARG_N_(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                _11,  _12,  _13,  _14,  _15,  _16,  _17,  _18,  _19,  _20, \
                _21,  _22,  _23,  _24,  _25,  _26,  _27,  _28,  _29,  _30, \
                _31,  _32,  _33,  _34,  _35,  _36,  _37,  _38,  _39,  _40, \
                _41,  _42,  _43,  _44,  _45,  _46,  _47,  _48,  _49,  _50, \
                _51,  _52,  _53,  _54,  _55,  _56,  _57,  _58,  _59,  _60, \
                _61,  _62,  _63,  _64,  _65,  _66,  _67,  _68,  _69,  _70, \
                _71,  _72,  _73,  _74,  _75,  _76,  _77,  _78,  _79,  _80, \
                _81,  _82,  _83,  _84,  _85,  _86,  _87,  _88,  _89,  _90, \
                _91,  _92,  _93,  _94,  _95,  _96,  _97,  _98,  _99,  _100, \
                _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, \
                _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, \
                _121, _122, _123, _124, _125, _126, _127, _128, _129, _130, \
                _131, _132, _133, _134, _135, _136, _137, _138, _139, _140, \
                _141, _142, _143, _144, _145, _146, _147, _148, _149, _150, \
                _151, _152, _153, _154, _155, _156, _157, _158, _159, _160, \
                _161, _162, _163, _164, _165, _166, _167, _168, _169, _170, \
                _171, _172, _173, _174, _175, _176, _177, _178, _179, _180, \
                _181, _182, _183, _184, _185, _186, _187, _188, _189, _190, \
                _191, _192, _193, _194, _195, _196, _197, _198, _199, _200, \
                _201, _202, _203, _204, _205, _206, _207, _208, _209, _210, \
                _211, _212, _213, _214, _215, _216, _217, _218, _219, _220, \
                _221, _222, _223, _224, _225, _226, _227, _228, _229, _230, \
                _231, _232, _233, _234, _235, _236, _237, _238, _239, _240, \
                _241, _242, _243, _244, _245, _246, _247, _248, _249, _250, \
                _251, _252, _253, _254, _255, _256, N, ...) N
#define _COUNT_ARG_HELPER(...)      ARG_N_(__VA_ARGS__)
#define _COUNT_ARG(...)     _COUNT_ARG_HELPER("ignore", ##__VA_ARGS__, \
                                256, 255, 254, 253, 252, 251, \
                               250, 249, 248, 247, 246, 245, 244, 243, 242, 241, \
                               240, 239, 238, 237, 236, 235, 234, 233, 232, 231, \
                               230, 229, 228, 227, 226, 225, 224, 223, 222, 221, \
                               220, 219, 218, 217, 216, 215, 214, 213, 212, 211, \
                               210, 209, 208, 207, 206, 205, 204, 203, 202, 201, \
                               200, 199, 198, 197, 196, 195, 194, 193, 192, 191, \
                               190, 189, 188, 187, 186, 185, 184, 183, 182, 181, \
                               180, 179, 178, 177, 176, 175, 174, 173, 172, 171, \
                               170, 169, 168, 167, 166, 165, 164, 163, 162, 161, \
                               160, 159, 158, 157, 156, 155, 154, 153, 152, 151, \
                               150, 149, 148, 147, 146, 145, 144, 143, 142, 141, \
                               140, 139, 138, 137, 136, 135, 134, 133, 132, 131, \
                               130, 129, 128, 127, 126, 125, 124, 123, 122, 121, \
                               120, 119, 118, 117, 116, 115, 114, 113, 112, 111, \
                               110, 109, 108, 107, 106, 105, 104, 103, 102, 101, \
                               100,  99,  98,  97,  96,  95,  94,  93,  92,  91, \
                                90,  89,  88,  87,  86,  85,  84,  83,  82,  81, \
                                80,  79,  78,  77,  76,  75,  74,  73,  72,  71, \
                                70,  69,  68,  67,  66,  65,  64,  63,  62,  61, \
                                60,  59,  58,  57,  56,  55,  54,  53,  52,  51, \
                                50,  49,  48,  47,  46,  45,  44,  43,  42,  41, \
                                40,  39,  38,  37,  36,  35,  34,  33,  32,  31, \
                                30,  29,  28,  27,  26,  25,  24,  23,  22,  21, \
                                20,  19,  18,  17,  16,  15,  14,  13,  12,  11, \
                                10,   9,   8,   7,   6,   5,   4,   3,   2,   1, 0)

#define FUNC_CAT2_1(a, b)           a##b
#define FUNC_CAT2(a, b)             FUNC_CAT2_1(a, b)
/*
 * FUNC_VA_NAME(prefix) -> prefix0
 * FUNC_VA_NAME(prefix, 1) -> prefix1
 * FUNC_VA_NAME(prefix, 1, 2) -> prefix2
 * */
#define FUNC_CALL_N(prefix, arg_cnt)   FUNC_CAT2(prefix, arg_cnt)
#define FUNC_VA_NAME(prefix, ...)      FUNC_CALL_N(prefix, _COUNT_ARG(__VA_ARGS__))

#define SRPC_PROTO(...)				__VA_ARGS__
#define SRPC_ARGS(...)				__VA_ARGS__
#define SRPC_INPUT__entry(...)		__VA_ARGS__
#define SRPC_INPUT(...)				__VA_ARGS__
#define SRPC_OUTPUT(...)			__VA_ARGS__
#define SRPC_FUNC_assign(...)		__VA_ARGS__

/* define entry struct */
#define _srpc_field(type, member)	type, member

#define _srpc_declare_member0(...)							int dummy;
#define _srpc_declare_member2(type, member)			type member;
#define _srpc_declare_member4(type, member, ...)		type member; _srpc_declare_member2(__VA_ARGS__)
#define _srpc_declare_member6(type, member, ...)		type member; _srpc_declare_member4(__VA_ARGS__)
#define _srpc_declare_member8(type, member, ...)		type member; _srpc_declare_member6(__VA_ARGS__)
#define _srpc_declare_member10(type, member, ...)		type member; _srpc_declare_member8(__VA_ARGS__)
#define _srpc_declare_member12(type, member, ...)		type member; _srpc_declare_member10(__VA_ARGS__)
#define _srpc_declare_member14(type, member, ...)		type member; _srpc_declare_member12(__VA_ARGS__)
#define _srpc_declare_member16(type, member, ...)		type member; _srpc_declare_member14(__VA_ARGS__)
#define _srpc_declare_member18(type, member, ...)		type member; _srpc_declare_member16(__VA_ARGS__)
#define _srpc_declare_member20(type, member, ...)		type member; _srpc_declare_member18(__VA_ARGS__)
#define _srpc_declare_member22(type, member, ...)		type member; _srpc_declare_member20(__VA_ARGS__)
#define _srpc_declare_member24(type, member, ...)		type member; _srpc_declare_member22(__VA_ARGS__)
#define _srpc_declare_member26(type, member, ...)		type member; _srpc_declare_member24(__VA_ARGS__)
#define _srpc_declare_member28(type, member, ...)		type member; _srpc_declare_member26(__VA_ARGS__)
#define _srpc_declare_member30(type, member, ...)		type member; _srpc_declare_member28(__VA_ARGS__)
#define _srpc_declare_member32(type, member, ...)		type member; _srpc_declare_member30(__VA_ARGS__)
#define _srpc_declare_member34(type, member, ...)		type member; _srpc_declare_member32(__VA_ARGS__)
#define _srpc_declare_member36(type, member, ...)		type member; _srpc_declare_member34(__VA_ARGS__)
#define _srpc_declare_member38(type, member, ...)		type member; _srpc_declare_member36(__VA_ARGS__)
#define _srpc_declare_member40(type, member, ...)		type member; _srpc_declare_member38(__VA_ARGS__)
#define _srpc_declare_member42(type, member, ...)		type member; _srpc_declare_member40(__VA_ARGS__)
#define _srpc_declare_member44(type, member, ...)		type member; _srpc_declare_member42(__VA_ARGS__)
#define _srpc_declare_member46(type, member, ...)		type member; _srpc_declare_member44(__VA_ARGS__)
#define _srpc_declare_member48(type, member, ...)		type member; _srpc_declare_member46(__VA_ARGS__)
#define _srpc_declare_member50(type, member, ...)		type member; _srpc_declare_member48(__VA_ARGS__)
#define _srpc_declare_member52(type, member, ...)		type member; _srpc_declare_member50(__VA_ARGS__)
#define _srpc_declare_member54(type, member, ...)		type member; _srpc_declare_member52(__VA_ARGS__)
#define _srpc_declare_member56(type, member, ...)		type member; _srpc_declare_member54(__VA_ARGS__)
#define _srpc_declare_member58(type, member, ...)		type member; _srpc_declare_member56(__VA_ARGS__)
#define _srpc_declare_member60(type, member, ...)		type member; _srpc_declare_member58(__VA_ARGS__)
#define _srpc_declare_member62(type, member, ...)		type member; _srpc_declare_member60(__VA_ARGS__)
#define _srpc_declare_member64(type, member, ...)		type member; _srpc_declare_member62(__VA_ARGS__)

#define _srpc_field_declare(...)			FUNC_VA_NAME(_srpc_declare_member, __VA_ARGS__)(__VA_ARGS__)

#define _srpc_calc_size_with_member0(...)					0
#define _srpc_calc_size_with_member2(type, member)			sizeof(type)
#define _srpc_calc_size_with_member4(type, member, ...)		sizeof(type), _srpc_calc_size_with_member2(__VA_ARGS__)
#define _srpc_calc_size_with_member6(type, member, ...)		sizeof(type), _srpc_calc_size_with_member4(__VA_ARGS__)
#define _srpc_calc_size_with_member8(type, member, ...)		sizeof(type), _srpc_calc_size_with_member6(__VA_ARGS__)
#define _srpc_calc_size_with_member10(type, member, ...)	sizeof(type), _srpc_calc_size_with_member8(__VA_ARGS__)
#define _srpc_calc_size_with_member12(type, member, ...)	sizeof(type), _srpc_calc_size_with_member10(__VA_ARGS__)
#define _srpc_calc_size_with_member14(type, member, ...)	sizeof(type), _srpc_calc_size_with_member12(__VA_ARGS__)
#define _srpc_calc_size_with_member16(type, member, ...)	sizeof(type), _srpc_calc_size_with_member14(__VA_ARGS__)
#define _srpc_calc_size_with_member18(type, member, ...)	sizeof(type), _srpc_calc_size_with_member16(__VA_ARGS__)
#define _srpc_calc_size_with_member20(type, member, ...)	sizeof(type), _srpc_calc_size_with_member18(__VA_ARGS__)
#define _srpc_calc_size_with_member22(type, member, ...)	sizeof(type), _srpc_calc_size_with_member20(__VA_ARGS__)
#define _srpc_calc_size_with_member24(type, member, ...)	sizeof(type), _srpc_calc_size_with_member22(__VA_ARGS__)
#define _srpc_calc_size_with_member26(type, member, ...)	sizeof(type), _srpc_calc_size_with_member24(__VA_ARGS__)
#define _srpc_calc_size_with_member28(type, member, ...)	sizeof(type), _srpc_calc_size_with_member26(__VA_ARGS__)
#define _srpc_calc_size_with_member30(type, member, ...)	sizeof(type), _srpc_calc_size_with_member28(__VA_ARGS__)
#define _srpc_calc_size_with_member32(type, member, ...)	sizeof(type), _srpc_calc_size_with_member30(__VA_ARGS__)
#define _srpc_calc_size_with_member34(type, member, ...)	sizeof(type), _srpc_calc_size_with_member32(__VA_ARGS__)
#define _srpc_calc_size_with_member36(type, member, ...)	sizeof(type), _srpc_calc_size_with_member34(__VA_ARGS__)
#define _srpc_calc_size_with_member38(type, member, ...)	sizeof(type), _srpc_calc_size_with_member36(__VA_ARGS__)
#define _srpc_calc_size_with_member40(type, member, ...)	sizeof(type), _srpc_calc_size_with_member38(__VA_ARGS__)
#define _srpc_calc_size_with_member42(type, member, ...)	sizeof(type), _srpc_calc_size_with_member40(__VA_ARGS__)
#define _srpc_calc_size_with_member44(type, member, ...)	sizeof(type), _srpc_calc_size_with_member42(__VA_ARGS__)
#define _srpc_calc_size_with_member46(type, member, ...)	sizeof(type), _srpc_calc_size_with_member44(__VA_ARGS__)
#define _srpc_calc_size_with_member48(type, member, ...)	sizeof(type), _srpc_calc_size_with_member46(__VA_ARGS__)
#define _srpc_calc_size_with_member50(type, member, ...)	sizeof(type), _srpc_calc_size_with_member48(__VA_ARGS__)
#define _srpc_calc_size_with_member52(type, member, ...)	sizeof(type), _srpc_calc_size_with_member50(__VA_ARGS__)
#define _srpc_calc_size_with_member54(type, member, ...)	sizeof(type), _srpc_calc_size_with_member52(__VA_ARGS__)
#define _srpc_calc_size_with_member56(type, member, ...)	sizeof(type), _srpc_calc_size_with_member54(__VA_ARGS__)
#define _srpc_calc_size_with_member58(type, member, ...)	sizeof(type), _srpc_calc_size_with_member56(__VA_ARGS__)
#define _srpc_calc_size_with_member60(type, member, ...)	sizeof(type), _srpc_calc_size_with_member58(__VA_ARGS__)
#define _srpc_calc_size_with_member62(type, member, ...)	sizeof(type), _srpc_calc_size_with_member60(__VA_ARGS__)
#define _srpc_calc_size_with_member64(type, member, ...)	sizeof(type), _srpc_calc_size_with_member62(__VA_ARGS__)

#define _srpc_field_size_declare(...)		FUNC_VA_NAME(_srpc_calc_size_with_member, __VA_ARGS__)(__VA_ARGS__)

#define MSG_MEMPBLK_MAX				(CONFIG_COMPONENTS_AW_RPC_CLIENT_MAX_MEMBLK)
#define MSG_MEMPBLK_SIZE			(CONFIG_COMPONENTS_AW_RPC_CLIENT_MEMBLK_SIZE)

struct srpc_cookie {
	void *self;
	hal_waitqueue_head_t wq;
	unsigned long msg;
	void *header;
	unsigned long pa;
	int error;
	int done;
};

struct msg_header {
	uint32_t magic;
	uint32_t ret_code;
	uint64_t cookie;
	uint64_t cookie_inverse;
	uint32_t class_id: 8;
	uint32_t func_id: 16;
	uint32_t nr_args: 8;
	uint32_t args_total;
	uint32_t data[]; /* args_size[N] + arg0... arg1... arg2... ... */
} __attribute__ ((packed));

#ifdef CONFIG_COMPONENTS_AW_RPC_CLIENT
/* call remote function */
struct msg_header *_srpc_alloc_header(unsigned int size, struct srpc_cookie *cookie);
void _srpc_header_init(struct msg_header *header, int class_id,
                int func_id, int nr_args, uint32_t arg_size[]);
void _srpc_free_header(struct msg_header *header, unsigned int size);
int _srpc_call(struct msg_header *header, bool atomic);

#define DECLARE_SRPC_FUNC(name, func_id, arg_proto, args, input_entry, arg_prep, arg_finish) \
	int srpc_##name (arg_proto) { \
		struct { \
			_srpc_field_declare(input_entry) \
		} __attribute__ ((packed)) *__entry; \
		const unsigned int nr_args = _COUNT_ARG(input_entry) / 2; \
		uint32_t args_size[_COUNT_ARG(input_entry) / 2] = { _srpc_field_size_declare(input_entry) }; \
		struct msg_header *header; \
		struct srpc_cookie cookie; \
		int __ret; \
                                    \
		header = _srpc_alloc_header(MSG_MEMPBLK_SIZE, &cookie); \
		if (!header) \
			return -ENOMEM; \
		_srpc_header_init(header, func_id >> 16, func_id & 0xff, nr_args, args_size); \
		header->args_total = sizeof(*__entry); \
		__entry = (typeof(__entry))(header->data + nr_args); \
		do { arg_prep; } while(0); \
		__ret = _srpc_call(header, false); \
		if (__ret) \
			goto err_out; \
		do { arg_finish; } while(0); \
		err_out: \
			_srpc_free_header(header, MSG_MEMPBLK_SIZE); \
		return __ret; \
	}
#endif /* CONFIG_COMPONENTS_AW_RPC_CLIENT */

#define DEFINE_SRPC_FUNC(name, input_entry, func_assign) \
	int srpc_##name (struct msg_header *header) { \
		struct { \
			_srpc_field_declare(input_entry) \
		} __attribute__ ((packed)) *__entry; \
		const unsigned int nr_args = _COUNT_ARG(input_entry) / 2; \
									\
		if (header->args_total != sizeof(*__entry)) { \
			printf("%s func args mismatch!, remote:%d local:%d\n", #name, header->args_total, sizeof(*__entry)); \
			return -EFAULT; \
		} \
		if (SRPC_MACRO_IS_ENABLED(CONFIG_SRPC_STRICT_CHECK)) { \
			int i; \
			uint32_t args_size[_COUNT_ARG(input_entry) / 2] = { _srpc_field_size_declare(input_entry) }; \
			if (nr_args != header->nr_args) { \
				printf("%s rpc arg count mismatch!, recv:%d except:%d\n", #name, header->nr_args, nr_args); \
				return -EFAULT; \
			} \
			for (i = 0; i < nr_args; i++) { \
				if (args_size[i] != header->data[i])  { \
					printf("%s rpc arg%d size mismatch!, recv:%d except:%d\n", #name, i, header->data[i], args_size[i]); \
					return -EFAULT; \
				} \
			} \
		} \
		__entry = (typeof(__entry))(header->data + nr_args); \
		do { func_assign; } while(0); \
		return 0; \
	}

#define SRPC_FUNC(name)		{ .fn = srpc_##name }

typedef int (*rpc_func_t)(struct msg_header *header);
struct func_entry {
	rpc_func_t fn;
};

int sunxi_rpc_register_class(int class_id, int max_fn, const struct func_entry fn[]);
int sunxi_rpc_unregister_class(int class_id, const struct func_entry fn[]);

static inline void *local_pa2va(unsigned long pa)
{
	return (void *)pa;
}

static inline unsigned long local_va2pa(void *va)
{
	return (unsigned long)va;
}

static inline void *remote_pa2va(unsigned long pa)
{
	return (void *)pa;
}

#endif
