#include "lp_ctrl_msg.h"

lp_msg_cb lp_ctrl_msg_cb = NULL;
static observer_base *ob = NULL;

int lp_ctrl_msg_init(void)
{
	TUYA_LOG_INFO("lp_ctrl_msg_init\n");
	ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK, NET_CTRL_MSG_ALL, lp_ctrl_msg_cb, NULL);
	if (ob == NULL) {
		TUYA_LOG_INFO("%s:ob is NULL\n", __func__);
		return -1;
	}
	if (sys_ctrl_attach(ob) != 0) {
		TUYA_LOG_INFO("%s:sys_ctrl_attach error\n", __func__);
		return -1;
	}
	TUYA_LOG_INFO("lp_ctrl_msg_init end\n");
	return 0;
}

int lp_ctrl_msg_deinit(void)
{
	TUYA_LOG_INFO("lp_ctrl_msg_deinit\n");
	if (ob)
		sys_ctrl_detach(ob);
	if (ob)
		sys_callback_observer_destroy(ob);
	ob = NULL;
	TUYA_LOG_INFO("lp_ctrl_msg_deinit end\n");
	return 0;
}

int lp_ctrl_msg_set_cb(lp_msg_cb cb)
{
	TUYA_LOG_INFO("tuya_lp_ctrl_msg_register\n");
	lp_ctrl_msg_cb = cb;
	net_ctrl_msg_register(lp_ctrl_msg_init);
	return 0;
}
