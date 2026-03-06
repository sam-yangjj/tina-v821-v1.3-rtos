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
#include <srpc_func.h>
#include <console.h>
#include <stdlib.h>
#include <string.h>

#define CLASS_ID                (0)
#define FUNC_ID(nr)             (CLASS_ID << 16 | nr)

static DECLARE_SRPC_FUNC(test_rpc0, FUNC_ID(0),
	SRPC_PROTO(int id, int *ret),
	SRPC_ARGS(id, ret),
	SRPC_INPUT__entry(
		_srpc_field(int,  id),
		_srpc_field(int, ret)
	),
	 SRPC_INPUT(
		__entry->id = id;
		__entry->ret = 0;
	),
	 SRPC_OUTPUT(
		*ret = __entry->ret;
	)
);

static DECLARE_SRPC_FUNC(test_rpc1, FUNC_ID(1),
	SRPC_PROTO(int id, int *ret),
	SRPC_ARGS(id, ret),
	SRPC_INPUT__entry(
		_srpc_field(int,  id),
		_srpc_field(int, ret)
	),
	 SRPC_INPUT(
		__entry->id = id;
		__entry->ret = 0;
	),
	 SRPC_OUTPUT(
		*ret = __entry->ret;
	)
);

static int cmd_test_rpc0(int argc, char *argv[])
{
	int ret = 0;
	int id;

	id = 0;

	if (argc >= 2)
		id = atoi(argv[1]);

	if (srpc_test_rpc0(id, &ret)) {
		printf("rpc call test_rpc0 failed!\n");
		return 0;
	}
	printf("rpc call test_rpc0 successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_rpc0, test_rpc0, test call remote func);

static int cmd_test_rpc1(int argc, char *argv[])
{
	int ret = 0;
	int rpc_ret;

	rpc_ret = srpc_test_rpc1(0, &ret);

	if (rpc_ret) {
		printf("rpc call test_rpc1 failed, ret = %d!\n", rpc_ret);
		return 0;
	}
	printf("rpc call test_rpc0 successed! ret = %d\n", ret);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_rpc1, test_rpc1, test call remote func);
