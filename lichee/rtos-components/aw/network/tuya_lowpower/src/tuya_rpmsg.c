#include "tuya_rpmsg.h"

#define BIND_CNT_MAX 10

const char *name = "rpmsg_lp";
static uint32_t tx_len = 32;
char tmpbuf[RPMSG_BUFFER_SIZE];

struct ept_test_entry *eptdev = NULL;

static int rpmsg_bind_cb(struct rpmsg_ept_client *client)
{
	TUYA_RPMSG_LOG("rpmsg%d: binding\r\n", client->id);

	eptdev = hal_malloc(sizeof(*eptdev));
	if (!eptdev) {
		openamp_err("failed to alloc client entry\r\n");
		return -ENOMEM;
	}

	memset(eptdev, 0, sizeof(*eptdev));
	eptdev->client = client;
	client->ept->priv = eptdev;
	eptdev = eptdev;
	eptdev->print_perf_data = (unsigned long)client->priv;

	return 0;
}

static int rpmsg_unbind_cb(struct rpmsg_ept_client *client)
{
	struct ept_test_entry *eptdev = client->ept->priv;

	TUYA_RPMSG_LOG("rpmsg%d: unbinding\r\n", client->id);

	hal_free(eptdev);
	eptdev = NULL;
	return 0;
}

int low_power_rpmsg_init(rpmsg_ept_cb cb)
{
	TUYA_RPMSG_LOG("low_power_rpmsg_init now\n");
	int cnt = 10;
	unsigned long print_perf_data = 0;
	int bind_state = -1;
	int bind_cnt = 0;

rebind:
	TUYA_RPMSG_LOG("wait cpux bind: %s, try cnt: %d/%d\r\n",
		name, ++bind_cnt, BIND_CNT_MAX);
	bind_state = rpmsg_client_bind(name, cb, rpmsg_bind_cb,
		rpmsg_unbind_cb, cnt, (void *)print_perf_data);
	if (bind_state != 0) {
		if (bind_cnt >= BIND_CNT_MAX) return -1;
		hal_msleep(500);
		goto rebind;
	}

	return 0;
}

int low_power_rpmsg_deinit(void)
{
	TUYA_RPMSG_LOG("unbind cpux: %s\r\n", name);
	rpmsg_client_unbind(name); /* self-unbind */
	return 0;
}

int low_power_rpmsg_to_cpux(char *send_msg, int len)
{
	if (eptdev == NULL) {
		TUYA_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}
	int ret = 0;
	memcpy(tmpbuf, send_msg, len);
	tx_len = strlen(tmpbuf);
	TUYA_RPMSG_LOG("tmpbuf = %s,len = %d\n", tmpbuf, len);
	ret = rpmsg_send(eptdev->client->ept, tmpbuf, tx_len);
	if (ret < 0) {
		TUYA_RPMSG_LOG("rpmsg%d: Failed to send data\n", eptdev->client->id);
	}
	return 0;
}

int low_power_rpmsg_set_keepalive_state(int state)
{
	if (eptdev == NULL) {
		TUYA_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}
	eptdev->keep_alive = state;
	return 0;
}

int low_power_rpmsg_get_sleep_state(void)
{
	if (eptdev == NULL) {
		TUYA_RPMSG_LOG("warn:eptdev not init\n");
		return -1;
	}
	return eptdev->keep_alive;
}
