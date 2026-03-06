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
#include "lp_ctrl_msg.h"
#include "public.h"

lp_msg_cb lp_ctrl_msg_cb = NULL;
static observer_base *ob = NULL;

int lp_ctrl_msg_init(void)
{
	LP_LOG_INFO("lp_ctrl_msg_init\n");
	ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK, NET_CTRL_MSG_ALL, lp_ctrl_msg_cb, NULL);
	if (ob == NULL) {
		LP_LOG_INFO("%s:ob is NULL\n", __func__);
		return -1;
	}
	if (sys_ctrl_attach(ob) != 0) {
		LP_LOG_INFO("%s:sys_ctrl_attach error\n", __func__);
		return -1;
	}
	LP_LOG_INFO("lp_ctrl_msg_init end\n");
	return 0;
}

int lp_ctrl_msg_deinit(void)
{
	LP_LOG_INFO("lp_ctrl_msg_deinit\n");
	if (ob)
		sys_ctrl_detach(ob);
	if (ob)
		sys_callback_observer_destroy(ob);
	ob = NULL;
	LP_LOG_INFO("lp_ctrl_msg_deinit end\n");
	return 0;
}

int lp_ctrl_msg_register(lp_msg_cb cb)
{
	LP_LOG_INFO("lp_ctrl_msg_register\n");
	lp_ctrl_msg_cb = cb;
	net_ctrl_msg_register(lp_ctrl_msg_init);
	return 0;
}
