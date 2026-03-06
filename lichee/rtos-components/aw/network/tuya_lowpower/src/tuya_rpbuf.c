#include "tuya_rpbuf.h"

LIST_HEAD(tuya_rpbuf_demo_buffers);
static hal_mutex_t rpbuf_demo_buffers_mutex;

struct rpbuf_demo_buffer_entry {
	int controller_id;
	struct rpbuf_buffer *buffer;
	struct list_head list;
};

static struct rpbuf_demo_buffer_entry *find_buffer_entry(const char *name)
{
	struct rpbuf_demo_buffer_entry *buf_entry;

	list_for_each_entry(buf_entry, &tuya_rpbuf_demo_buffers, list)
	{
		if (0 == strcmp(rpbuf_buffer_name(buf_entry->buffer), name))
			return buf_entry;
	}
	return NULL;
}

static void rpbuf_demo_buffer_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
	TUYA_LOG_INFO("buffer \"%s\" is available\n", rpbuf_buffer_name(buffer));
}

static int rpbuf_demo_buffer_rx_cb(struct rpbuf_buffer *buffer,
	void *data, int data_len, void *priv)
{
	TUYA_LOG_INFO("buffer \"%s\" received data (addr: %p, offset: %d, len: %d):\n",
		rpbuf_buffer_name(buffer), rpbuf_buffer_va(buffer),
		(int)(data - rpbuf_buffer_va(buffer)), data_len);

	// memcpy(&tuya_info, (tuya_lowpower_info *)data, sizeof(tuya_info));
	// TUYA_LOG_INFO("tuya_info.ip.u_addr.ip4 = %d\n", tuya_info.ip.u_addr.ip4);
	// TUYA_LOG_INFO("tuya_info.port = %d\n", tuya_info.port);
	// TUYA_LOG_INFO("tuya_info.devid = %s\n", tuya_info.devid);
	// TUYA_LOG_INFO("tuya_info.local_key = %s\n", tuya_info.local_key);

	// hal_sem_post(tuya_info_sem);
	return 0;
}

static int rpbuf_demo_buffer_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	TUYA_LOG_INFO("buffer \"%s\": remote buffer destroyed\n", rpbuf_buffer_name(buffer));
	return 0;
}

static const struct rpbuf_buffer_cbs rpbuf_demo_cbs = {
    .available_cb = rpbuf_demo_buffer_available_cb,
    .rx_cb = rpbuf_demo_buffer_rx_cb,
    .destroyed_cb = rpbuf_demo_buffer_destroyed_cb,
};

int rpbuf_demo_create(int controller_id, const char *name, int len)
{
	int ret;
	struct rpbuf_demo_buffer_entry *buf_entry = NULL;
	struct rpbuf_controller *controller = NULL;
	struct rpbuf_buffer *buffer = NULL;

	hal_mutex_lock(rpbuf_demo_buffers_mutex);
	buf_entry = find_buffer_entry(name);
	if (buf_entry) {
		TUYA_LOG_INFO("Buffer named \"%s\" already exists\n", name);
		hal_mutex_unlock(rpbuf_demo_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	hal_mutex_unlock(rpbuf_demo_buffers_mutex);

	buf_entry = hal_malloc(sizeof(struct rpbuf_demo_buffer_entry));
	if (!buf_entry) {
		RPBUF_DEMO_LOG(controller_id, name, len,
			"Failed to allocate memory for buffer entry\n");
		ret = -ENOMEM;
		goto err_out;
	}
	buf_entry->controller_id = controller_id;

	controller = rpbuf_get_controller_by_id(controller_id);
	if (!controller) {
		RPBUF_DEMO_LOG(controller_id, name, len,
			"Failed to get controller%d, controller_id\n", controller_id);
		ret = -ENOENT;
		goto err_free_buf_entry;
	}

	buffer = rpbuf_alloc_buffer(controller, name, len, NULL, &rpbuf_demo_cbs, NULL);
	if (!buffer) {
		RPBUF_DEMO_LOG(controller_id, name, len, "rpbuf_alloc_buffer failed\n");
		ret = -ENOENT;
		goto err_free_buf_entry;
	}
	buf_entry->buffer = buffer;
	rpbuf_buffer_set_sync(buffer, true);

	hal_mutex_lock(rpbuf_demo_buffers_mutex);
	list_add_tail(&buf_entry->list, &tuya_rpbuf_demo_buffers);
	hal_mutex_unlock(rpbuf_demo_buffers_mutex);

	return 0;

err_free_buf_entry:
	hal_free(buf_entry);
err_out:
	return ret;
}

int rpbuf_demo_destroy(const char *name)
{
	int ret;
	struct rpbuf_demo_buffer_entry *buf_entry;
	struct rpbuf_buffer *buffer;

	if (rpbuf_demo_buffers_mutex)
		hal_mutex_lock(rpbuf_demo_buffers_mutex);

	buf_entry = find_buffer_entry(name);
	if (!buf_entry) {
		TUYA_LOG_INFO("Buffer named \"%s\" not found\n", name);
		hal_mutex_unlock(rpbuf_demo_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	buffer = buf_entry->buffer;

	ret = rpbuf_free_buffer(buffer);
	if (ret < 0) {
		RPBUF_DEMO_LOG(buf_entry->controller_id,
			rpbuf_buffer_name(buffer),
			rpbuf_buffer_len(buffer),
			"rpbuf_free_buffer failed\n");
		hal_mutex_unlock(rpbuf_demo_buffers_mutex);
		goto err_out;
	}

	list_del(&buf_entry->list);
	hal_free(buf_entry);

	hal_mutex_unlock(rpbuf_demo_buffers_mutex);

	return 0;

err_out:
	return ret;
}

/* rpbuf_demo_transmit */
int rpbuf_demo_transmit(void *data, int data_len, int offset)
{
	int ret;
	struct rpbuf_demo_buffer_entry *buf_entry;
	struct rpbuf_buffer *buffer;

	hal_mutex_lock(rpbuf_demo_buffers_mutex);
	buf_entry = find_buffer_entry(RPBUF_BUFFER_NAME_DEFAULT);
	if (!buf_entry) {
		TUYA_LOG_INFO("Buffer named \"%s\" not found\n", RPBUF_BUFFER_NAME_DEFAULT);
		hal_mutex_unlock(rpbuf_demo_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	buffer = buf_entry->buffer;

	/*
	 * Before putting data to buffer or sending buffer to remote, we should
	 * ensure that the buffer is available.
	 */
	if (!rpbuf_buffer_is_available(buffer)) {
		RPBUF_DEMO_LOG(buf_entry->controller_id,
			rpbuf_buffer_name(buffer),
			rpbuf_buffer_len(buffer),
			"buffer not available\n");
		hal_mutex_unlock(rpbuf_demo_buffers_mutex);
		ret = -EACCES;
		goto err_out;
	}

	if (data_len > rpbuf_buffer_len(buffer)) {
		RPBUF_DEMO_LOG(buf_entry->controller_id,
			rpbuf_buffer_name(buffer),
			rpbuf_buffer_len(buffer),
			"data length too long\n");
		hal_mutex_unlock(rpbuf_demo_buffers_mutex);
		ret = -EACCES;
		goto err_out;
	}
	memcpy(rpbuf_buffer_va(buffer) + offset, data, data_len);

	ret = rpbuf_transmit_buffer(buffer, offset, data_len);
	if (ret < 0) {
		RPBUF_DEMO_LOG(buf_entry->controller_id,
			rpbuf_buffer_name(buffer),
			rpbuf_buffer_len(buffer),
			"rpbuf_transmit_buffer failed\n");
		hal_mutex_unlock(rpbuf_demo_buffers_mutex);
		goto err_out;
	}
	hal_mutex_unlock(rpbuf_demo_buffers_mutex);

	return 0;

err_out:
	return ret;
}
