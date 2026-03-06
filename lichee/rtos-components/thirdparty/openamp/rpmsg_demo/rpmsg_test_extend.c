/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <hal_osal.h>
#include <hal_sem.h>
#include <hal_cache.h>
#include <hal_msgbox.h>
#include <aw_list.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/msgbox_ipi.h>
#include "md5.h"

#define RPMSG_SERVICE_NAME "sunxi,rpmsg_test"

#define RPMSG_TEST_RX_QUEUE			(4)

#define log(fmt, ...)	\
		do { \
			if (do_verbose) \
				printf(fmt, ##__VA_ARGS__); \
		} while(0)

static LIST_HEAD(g_endpoints);
static uint32_t tx_delay_ms = 500;
static uint32_t tx_len = 32;
static uint32_t do_verbose = 0;

struct ept_test_entry {
	struct rpmsg_endpoint *ept;
	struct list_head node;
	void *rx_task;
	void *tx_task;

	hal_sem_t rx_sem;
	hal_spinlock_t lock;
	uint16_t stop;
	/* rx queue */
	uint16_t head, tail, cnt;
	uint8_t rx_queue[RPMSG_TEST_RX_QUEUE][RPMSG_BUFFER_SIZE];
	uint16_t rx_len[RPMSG_TEST_RX_QUEUE];
};

static int rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	unsigned long flags;
	struct ept_test_entry *eptdev = ept->priv;

	if (eptdev->cnt == RPMSG_TEST_RX_QUEUE) {
		printf("%s: rx queue is full\r\n", ept->name);
		return 0;
	}

	flags = hal_spin_lock_irqsave(&eptdev->lock);
	memcpy(eptdev->rx_queue[eptdev->tail], data, len);
	eptdev->rx_len[eptdev->tail] = len;
	eptdev->tail++;
	eptdev->cnt++;
	eptdev->tail %= RPMSG_TEST_RX_QUEUE;
	hal_spin_unlock_irqrestore(&eptdev->lock, flags);

	log("%s: rx %zu Bytes\r\n", ept->name, len);
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
	struct rpmsg_endpoint *ept = eptdev->ept;

	printf("%s rx thread start...\r\n", ept->name);

	while (1) {
		ret = hal_sem_timedwait(eptdev->rx_sem, 100 / portTICK_PERIOD_MS);

		if (eptdev->stop)
			break;
		if (ret || eptdev->cnt == 0)
			continue;

		flags = hal_spin_lock_irqsave(&eptdev->lock);
		rx_len = eptdev->rx_len[eptdev->head];
		memcpy(tmpbuf, eptdev->rx_queue[eptdev->head], rx_len);
		eptdev->cnt--;
		eptdev->head++;
		eptdev->head %= RPMSG_TEST_RX_QUEUE;
		hal_spin_unlock_irqrestore(&eptdev->lock, flags);

		checkdata(tmpbuf, rx_len, do_verbose, ept->name);
	}

	printf("%s rx thread exit...\r\n", ept->name);
	hal_thread_stop(NULL);
}

static void rpmsg_tx_thread(void *pram)
{
	int ret;
	struct ept_test_entry *eptdev = pram;
	struct rpmsg_endpoint *ept = eptdev->ept;
	uint8_t tmpbuf[RPMSG_BUFFER_SIZE];

	printf("%s tx thread start...\r\n", ept->name);

	while (1) {
		mkdata(tmpbuf, tx_len, do_verbose, ept->name);

		ret = rpmsg_send(eptdev->ept, tmpbuf, tx_len);
		if (ret < 0)
			printf("%s: Failed to send data\n", ept->name);

		if (eptdev->stop)
			break;
		if (tx_delay_ms)
			vTaskDelay(tx_delay_ms / portTICK_PERIOD_MS);
	}

	printf("%s tx thread exit...\r\n", ept->name);
	hal_thread_stop(NULL);
}

static void rpmsg_unbind_callback(struct rpmsg_endpoint *ept)
{
	struct ept_test_entry *eptdev = ept->priv;

	printf("%s is destroyed\n", ept->name);

	list_del_init(&eptdev->node);
	hal_sem_clear(eptdev->rx_sem);
	hal_thread_stop(eptdev->tx_task);
	hal_thread_stop(eptdev->rx_task);
	hal_sem_delete(eptdev->rx_sem);
	hal_spin_lock_deinit(&eptdev->lock);
	openamp_ept_close(eptdev->ept);
	hal_free(eptdev);
}

/****************************** rpmsg driver(MASTER MODE) ****************************************/
static int rpmsg_test_probe(struct rpmsg_device *rdev, struct rpmsg_endpoint *ept,
				uint32_t src, uint32_t dest)
{
	struct ept_test_entry *eptdev;
	int ret;

	printf("%s probe\r\n", ept->name);

	eptdev = hal_malloc(sizeof(*eptdev));
	if (!eptdev) {
		openamp_err("failed to alloc %s entry\r\n", ept->name);
		return -ENOMEM;
	}

	memset(eptdev, 0, sizeof(*eptdev));
	eptdev->ept = ept;
	ept->priv = eptdev;

	eptdev->rx_sem = hal_sem_create(0);
	if (!eptdev->rx_sem) {
		printf("Failed to create %s rx_sem\n", ept->name);
		ret = -ENOMEM;
		goto free_eptdev;
	}

	hal_spin_lock_init(&eptdev->lock);

	eptdev->rx_task = hal_thread_create(rpmsg_rx_thread, eptdev, ept->name,
					1024, configMAX_PRIORITIES - 4);
	if (!eptdev->rx_task) {
		printf("Failed to create %s rx_task\r\n", ept->name);
		ret = -ENOMEM;
		goto free_sem;
	}
	eptdev->tx_task = hal_thread_create(rpmsg_tx_thread, eptdev, ept->name,
					1024, configMAX_PRIORITIES - 4);
	if (!eptdev->tx_task) {
		printf("Failed to create %s rx_task\r\n", ept->name);
		ret = -ENOMEM;
		goto free_rx_task;
	}

	list_add(&eptdev->node, &g_endpoints);
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

static char rpmsg_driver_name[RPMSG_NAME_SIZE];
static struct rpmsg_driver rpmsg_test_driver = {
	.probe = rpmsg_test_probe,
	.remove = rpmsg_unbind_callback,
	.callback = rpmsg_ept_callback,
};

/****************************** rpmsg endpoint(CLIENT MODE) ****************************************/
static int rpmsg_test_create_ept(int rproc_id, const char *name)
{
	struct ept_test_entry *eptdev;
	int ret;

	printf("create %s rpmsg endpoint\r\n", name);

	eptdev = hal_malloc(sizeof(*eptdev));
	if (!eptdev) {
		openamp_err("failed to alloc %s entry\r\n", name);
		return -ENOMEM;
	}
	memset(eptdev, 0, sizeof(*eptdev));

	eptdev->rx_sem = hal_sem_create(0);
	if (!eptdev->rx_sem) {
		printf("Failed to create %s rx_sem\n", name);
		ret = -ENOMEM;
		goto free_eptdev;
	}

	hal_spin_lock_init(&eptdev->lock);

	eptdev->ept = openamp_ept_open(name, rproc_id, RPMSG_ADDR_ANY,
					RPMSG_ADDR_ANY, eptdev, rpmsg_ept_callback,
					rpmsg_unbind_callback);
	if (!eptdev->ept) {
		printf("Failed to Create Endpoint\r\n");
		goto free_sem;
	}

	eptdev->rx_task = hal_thread_create(rpmsg_rx_thread, eptdev, name,
					1024, HAL_THREAD_PRIORITY_APP);
	if (!eptdev->rx_task) {
		printf("Failed to create %s rx_task\r\n", name);
		ret = -ENOMEM;
		goto destory_ept;
	}
	eptdev->tx_task = hal_thread_create(rpmsg_tx_thread, eptdev, name,
					1024, HAL_THREAD_PRIORITY_APP);
	if (!eptdev->tx_task) {
		printf("Failed to create %s rx_task\r\n", name);
		ret = -ENOMEM;
		goto free_rx_task;
	}

	list_add(&eptdev->node, &g_endpoints);

	hal_thread_start(eptdev->tx_task);
	hal_thread_start(eptdev->rx_task);

	return 0;
free_rx_task:
	hal_thread_stop(eptdev->rx_task);
destory_ept:
	openamp_ept_close(eptdev->ept);
free_sem:
	hal_sem_delete(eptdev->rx_sem);
free_eptdev:
	hal_free(eptdev);

	return ret;
}

static int rpmsg_test_delete_ept(const char *name)
{
	struct ept_test_entry *pos, *tmp;

	list_for_each_entry_safe(pos, tmp, &g_endpoints, node) {
		if (!strcmp(pos->ept->name, name))
			goto find;
	}

	return -ENODEV;
find:
	rpmsg_unbind_callback(pos->ept);

	return 0;
}

static void print_help_msg(void)
{
	printf("\n");
	printf("USAGE:\n");
	printf("  rpmsg_test_extend [OPTIONS]\n");
	printf("OPTIONS:\n");
	printf("  -h          : print help message\n");
	printf("  -v 1/0      : verbosely print check result\n");
	printf("  MASTER MODE :\n");
	printf("  -r name     : register rpmsg driver\n");
	printf("  -u          : unregister rpmsg driver\n");
	printf("  CLIEN MODE :\n");
	printf("  -c          : create endpoint\n");
	printf("  -d          : destory endpoint\n");
	printf("  -t tx_delay : specify tx_delay_ms (Global variable, default: 500)\n");
	printf("  -L tx_len   : specify tx length (Global variable, default: 32 bytes)\n");
	printf("  -N name     : specify rpmsg name\n");
	printf("  -R rproc_id : specify rproc_id(0 ARM, 1 another Core, default:1)\n");
	printf("\n");
	printf("e.g.\n");
	printf("In MASTER\n");
	printf("      rpmsg_test_extend -r \"test\" -L 480 -t 500 -v 0 : register rpmsg driver\n");
	printf("      rpmsg_test_extend -N \"test\" -u                 : unregister rpmsg driver\n");
	printf("In CLIENT\n");
	printf("      rpmsg_test_extend -N \"test1\" -L 480 -t 500 -c -v 0 : send random data\n");
	printf("      rpmsg_test_extend -N \"test1\" -d                : delete endpoint\n");
	printf("\n");
}

static int cmd_rpmsg_test_extend(int argc, char *argv[])
{
	int ret = 0;
	unsigned long len = 32;
	int delay_ms = 500;
	const char *name = RPMSG_SERVICE_NAME;
	int c;
	int do_create = 0;
	int do_delete = 0;
	int do_register = 0;
	int do_unregister = 0;
	int rproc_id = 1;

	if (argc <= 1) {
		print_help_msg();
		ret = -1;
		goto out;
	}

	while ((c = getopt(argc, argv, "hv:r:ucdt:L:N:R:")) != -1) {
		switch(c) {
		case 'h':
			print_help_msg();
			ret = 0;
			goto out;
		case 'v':
			if (atoi(optarg) > 0)
				do_verbose = 1;
			else
				do_verbose = 0;
			break;
		case 'r':
			do_register = 1;
			name = optarg;
			break;
		case 'u':
			do_unregister = 1;
			break;
		case 'c':
			do_create = 1;
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
		case 'R':
			rproc_id = atoi(optarg);
			break;
		default:
			printf("Invalid option: -%c\n", c);
			print_help_msg();
			ret = -1;
			goto out;
		}
	}

	if (strlen(name) >= RPMSG_NAME_SIZE) {
		printf("name too long,shoule less than %d\r\n", RPMSG_NAME_SIZE);
		return -EINVAL;
	}

	if (do_register) {
		if (rpmsg_driver_name[0]) {
			printf("already register %s rpmsg driver.\r\n", rpmsg_test_driver.name);
		} else {
			strncpy(rpmsg_driver_name, name, RPMSG_NAME_SIZE);
			rpmsg_test_driver.name = rpmsg_driver_name;
			printf("register %s rpmsg driver.\r\n", rpmsg_test_driver.name);
			rpmsg_register_driver(&rpmsg_test_driver);
		}
	}

	if (do_unregister) {
		if (rpmsg_driver_name[0]) {
			printf("unregister %s rpmsg driver.\r\n", rpmsg_test_driver.name);
			rpmsg_unregister_driver(&rpmsg_test_driver);
			memset(rpmsg_driver_name, 0, RPMSG_NAME_SIZE);
		} else {
			printf("no any rpmsg driver.\r\n");
		}
	}

	if (do_create) {
		rpmsg_test_create_ept(rproc_id, name);
	}

	if (do_delete) {
		rpmsg_test_delete_ept(name);
	}

	return 0;
out:
	return ret;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_rpmsg_test_extend, rpmsg_test_extend, rpmsg test with another rproc);
