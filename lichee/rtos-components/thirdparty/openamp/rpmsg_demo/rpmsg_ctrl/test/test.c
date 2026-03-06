#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <aw_list.h>
#include <hal_atomic.h>
#include <hal_mutex.h>
#include <hal_mem.h>
#include <hal_sem.h>
#include <hal_queue.h>
#include <hal_thread.h>
#include <console.h>
#include <openamp/sunxi_helper/openamp_log.h>
#include <openamp/sunxi_helper/rpmsg_master.h>

#include "md5.h"

#define RPMSG_TEST_RX_QUEUE			(4)

#define log(fmt, ...)	\
		do { \
			if (do_verbose) \
				printf(fmt, ##__VA_ARGS__); \
		} while(0)


static uint32_t tx_delay_ms = 500;
static uint32_t tx_len = 32;
static uint32_t do_verbose = 0;

struct ept_test_entry {
	struct rpmsg_ept_client *client;
	struct list_head list;

	void *rx_task;
	void *tx_task;

	hal_sem_t rx_sem;
	hal_spinlock_t lock;
	uint16_t stop;
	/* rx queue */
	uint16_t head, tail, cnt;
	uint8_t rx_queue[RPMSG_TEST_RX_QUEUE][RPMSG_BUFFER_SIZE];
	uint16_t rx_len[RPMSG_TEST_RX_QUEUE];
	int is_tx_exit, is_rx_exit;
	unsigned long print_perf_data;
};

static int rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	unsigned long flags;
	struct ept_test_entry *eptdev = ept->priv;
	int ret;
	rpmsg_perf_data_t perf_data;

	if (eptdev->cnt == RPMSG_TEST_RX_QUEUE) {
		printf("rpmsg%d: rx queue is full\r\n", (int)eptdev->client->id);
		return 0;
	}

	flags = hal_spin_lock_irqsave(&eptdev->lock);
	memcpy(eptdev->rx_queue[eptdev->tail], data, len);
	eptdev->rx_len[eptdev->tail] = len;
	eptdev->tail++;
	eptdev->cnt++;
	eptdev->tail %= RPMSG_TEST_RX_QUEUE;
	hal_spin_unlock_irqrestore(&eptdev->lock, flags);

	log("rpmsg%d: rx %zu Bytes\r\n", (int)eptdev->client->id, len);

	if (eptdev->print_perf_data) {
		rpmsg_record_receiver_end_ts(ept);
		ret = rpmsg_get_perf_data(ept, &perf_data);
		if (ret) {
			printf("get rpmsg performance data failed, ret: %d\n", ret);
		} else {
			printf("'%s' performance data:\n", ept->name);
			rpmsg_dump_perf_data(&perf_data);
		}
	}

	hal_sem_post(eptdev->rx_sem);

	return 0;
}

static void mkdata(uint8_t *buffer, int size, int verbose, const char *name)
{
	int i;
	int data_len = size - 16;
	uint32_t *pdate = (uint32_t *)buffer;

	srand((int)time(NULL));

	/* generate random data */
	for (i = 0; i < (data_len / 4); i++)
		pdate[i] = (uint32_t)rand();

	md5(buffer, data_len, &buffer[data_len]);

	if (data_len > 16)
		data_len = 16;

	if (verbose) {
		printf("[%s]data:", name);
		for (i = 0; i < data_len; i++)
			printf("%02x", buffer[i]);
		printf("... [md5:");
		for (i = 0; i < 16; i++)
			printf("%02x", buffer[size - 16 + i]);
		printf("]\n\n");
	}
}

static int checkdata(uint8_t *buffer, int size, int verbose, const char *name)
{
	int i;
	int data_len = size - 16;
	uint8_t digest[16];

	md5(buffer, data_len, digest);

	if (data_len > 16)
		data_len = 16;

	if (verbose) {
		printf("data:");
		for (i = 0; i < data_len; i++)
			printf("%02x", buffer[i]);
		printf("... check:");
	}

	for (i = 0; i < 16; i++) {
		if (verbose)
			printf("%02x", buffer[size - 16 + i]);
		if (buffer[size - 16 + i] != digest[i])
			break;
	}

	if (i != 16) {
		printf("[%s] failed [target:", name);
		for (i = 0; i < 16; i++)
			printf("%02x", buffer[size - 16 + i]);
		printf(" <-> cur:");
		for (i = 0; i < 16; i++)
			printf("%02x", digest[i]);
		printf("\n\n");
		return 0;
	} else {
		if (verbose)
			printf("[%s] success\n\n", name);
		return 1;
	}
}

static void rpmsg_rx_thread(void *pram)
{
	int ret;
	unsigned long flags;
	struct ept_test_entry *eptdev = pram;

	int rx_len;
	uint8_t tmpbuf[RPMSG_BUFFER_SIZE];

	printf("rpmsg%d rx thread start...\r\n", eptdev->client->id);

	while (1) {
		if (eptdev->stop)
			break;

		ret = hal_sem_timedwait(eptdev->rx_sem, 100 / portTICK_PERIOD_MS);

		if (ret || eptdev->cnt == 0)
			continue;

		flags = hal_spin_lock_irqsave(&eptdev->lock);
		rx_len = eptdev->rx_len[eptdev->head];
		memcpy(tmpbuf, eptdev->rx_queue[eptdev->head], rx_len);
		eptdev->cnt--;
		eptdev->head++;
		eptdev->head %= RPMSG_TEST_RX_QUEUE;
		hal_spin_unlock_irqrestore(&eptdev->lock, flags);

		checkdata(tmpbuf, rx_len, do_verbose, eptdev->client->name);
	}

	printf("rpmsg%d rx thread exit...\r\n", eptdev->client->id);
	eptdev->is_rx_exit = 1;
	hal_thread_stop(NULL);
}

static void rpmsg_tx_thread(void *pram)
{
	int ret;
	struct ept_test_entry *eptdev = pram;

	uint8_t tmpbuf[RPMSG_BUFFER_SIZE];

	printf("rpmsg%d tx thread start...\r\n", eptdev->client->id);

	while (1) {
		if (eptdev->stop)
			break;
		mkdata(tmpbuf, tx_len, do_verbose, eptdev->client->name);

		ret = rpmsg_send(eptdev->client->ept, tmpbuf, tx_len);
		if (ret < 0)
			printf("rpmsg%d: Failed to send data\n", eptdev->client->id);


		if (tx_delay_ms)
			vTaskDelay(tx_delay_ms / portTICK_PERIOD_MS);
	}

	printf("rpmsg%d tx thread exit...\r\n", eptdev->client->id);
	eptdev->is_tx_exit = 1;
	hal_thread_stop(NULL);
}

static int rpmsg_bind_cb(struct rpmsg_ept_client *client)
{
	int ret;
	struct ept_test_entry *eptdev;

	log("rpmsg%d: binding\r\n", client->id);

	eptdev = hal_malloc(sizeof(*eptdev));
	if (!eptdev) {
		openamp_err("failed to alloc client entry\r\n");
		return -ENOMEM;
	}

	memset(eptdev, 0, sizeof(*eptdev));
	eptdev->client = client;
	client->ept->priv = eptdev;

	eptdev->rx_sem = hal_sem_create(0);
	if (!eptdev->rx_sem) {
		printf("Failed to create rpmsg%d rx_sem\n", client->id);
		ret = -ENOMEM;
		goto free_eptdev;
	}

	hal_spin_lock_init(&eptdev->lock);

	eptdev->print_perf_data = (unsigned long)client->priv;
	eptdev->rx_task = hal_thread_create(rpmsg_rx_thread, eptdev, client->name,
					1024, configMAX_PRIORITIES - 4);
	if (!eptdev->rx_task) {
		printf("Failed to create %s rx_task\r\n", client->name);
		ret = -ENOMEM;
		goto free_sem;
	}
	eptdev->tx_task = hal_thread_create(rpmsg_tx_thread, eptdev, client->name,
					1024, configMAX_PRIORITIES - 4);
	if (!eptdev->tx_task) {
		printf("Failed to create %s rx_task\r\n", client->name);
		ret = -ENOMEM;
		goto free_rx_task;
	}

	hal_thread_start(eptdev->tx_task);
	hal_thread_start(eptdev->rx_task);

	return 0;
free_rx_task:
	hal_thread_stop(eptdev->rx_task);
free_sem:
	hal_sem_delete(eptdev->rx_sem);
free_eptdev:
	hal_free(eptdev);

	return ret;
}

static int rpmsg_unbind_cb(struct rpmsg_ept_client *client)
{
	struct ept_test_entry *eptdev = client->ept->priv;

	log("rpmsg%d: unbinding\r\n", client->id);

	eptdev->stop = 1;

	while (1) {
		hal_msleep(1);
		if (!eptdev->is_tx_exit)
			continue;

		if (!eptdev->is_rx_exit)
			continue;

		break;
	}

	hal_sem_clear(eptdev->rx_sem);
	hal_sem_delete(eptdev->rx_sem);
	hal_spin_lock_deinit(&eptdev->lock);

	hal_free(eptdev);

	return 0;
}

static void print_help_msg(void)
{
	printf("\n");
	printf("USAGE:\n");
	printf("  rpmsg_test [OPTIONS]\n");
	printf("OPTIONS:\n");
	printf("  -h          : print help message\n");
	printf("  -c          : create buffer\n");
	printf("  -d          : destory buffer\n");
	printf("  -s          : send test messagese \n");
	printf("  -v          : verbosely print check result\n");
	printf("  -M          : max rpmsg client (default: 10)\n");
	printf("  -t tx_delay : specify tx_delay_ms (Global variable, default: 500)\n");
	printf("  -L tx_len   : specify tx length (Global variable, default: 32 bytes)\n");
	printf("  -p          : print performance data\n");
	printf("\n");
	printf("e.g.\n");
	printf("      rpmsg_test -N \"xxx\" -L LENGTH -c\n");
	printf("      rpmsg_test -N \"xxx\" -L LENGTH -d\n");
	printf("\n");
}

static int cmd_rpmsg_test(int argc, char *argv[])
{
	int ret = 0;
	unsigned long len = 32;
	int cnt = 10;
	int delay_ms = 500;
	const char *name = NULL;
	int c;
	int do_create = 0;
	int do_delete = 0;
	unsigned long print_perf_data = 0;

	if (argc <= 1) {
		print_help_msg();
		ret = -1;
		goto out;
	}

	while ((c = getopt(argc, argv, "hvcdt:N:L:M:p")) != -1) {
		switch(c) {
		case 'h':
			print_help_msg();
			ret = 0;
			goto out;
		case 'c':
			do_create = 1;
			break;
		case 'v':
			do_verbose = 1;
			break;
		case 'd':
			do_delete = 1;
			break;
		case 't':
			delay_ms = atoi(optarg);
			if (delay_ms == 0) {
				ret = -1;
				printf("Invalid cnt arg.\r\n");
				goto out;
			}
			tx_delay_ms = delay_ms;
			break;
		case 'N':
			name = optarg;
			break;
		case 'L':
			len = strtol(optarg, NULL, 0);
			if (len == -ERANGE || len > RPMSG_BUFFER_SIZE) {
				printf("Invalid length arg.\r\n");
				ret = -1;
				goto out;
			}
			tx_len = len;
			break;
		case 'M':
			cnt = atoi(optarg);
			if (cnt == 0) {
				ret = -1;
				printf("Invalid cnt arg.\r\n");
				goto out;
			}
			break;
		case 'p':
			print_perf_data = 1;
			break;
		default:
			printf("Invalid option: -%c\n", c);
			print_help_msg();
			ret = -1;
			goto out;
		}
	}

	if (strlen(name) >= 32) {
		printf("name too long,shoule less than 32\r\n");
		return -EINVAL;
	}

	if (do_create) {
		printf("bind %s\r\n", name);
		rpmsg_client_bind(name, rpmsg_ept_callback, rpmsg_bind_cb,
						rpmsg_unbind_cb, cnt, (void *)print_perf_data);
	}

	if (do_delete) {
		printf("unbind %s\r\n", name);
		rpmsg_client_unbind(name);
	}

	return 0;
out:
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpmsg_test, rpmsg_test, rpmsg test);

