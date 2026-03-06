/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "errno.h"
#include "kernel/os/os.h"
#include "hal_controller.h"
#include "hal_hci.h"
#include "xrbtc.h"
#include "sdd/sdd.h"

enum {
    VIRTUAL_HCI_ENABLED     =   1 << 0,
};

static uint32_t driver_flags;
#define MAC_LEN 6
#define SET_MAC_COMMAND_LEN 11

static int load_sdd_file()
{
#if 0
	int fd = -1, ret = 0;
	uint32_t size, rd_count, wr_count;
	struct stat file_st;
	uint8_t *file_data = NULL;

	char sdd_path[SDD_PATH_MAXLEN + 1] = {0};
	xr_get_sdd_file_path(sdd_path, SDD_PATH_MAXLEN + 1);

	if (stat(sdd_path, &file_st) < 0) {
		printf("Can not access config file %s\n",sdd_path);
		return -ENOENT;
	}

	size = file_st.st_size;
	if (size <= 0 || size > SDD_MAX_SIZE) {
		printf("invalid sdd data size: %ld\n",size);
		return -EINVAL;
	}

	fd = open(sdd_path, O_RDONLY);
	if (fd <= 0) {
		printf("open %s failed\n", sdd_path);
		return -ENOENT;
	}

	file_data = (unsigned char *)malloc(size);
	if (!file_data) {
		printf("can not malloc %d-byte buffer\n", size);
		ret = -ENOSPC;
		goto malloc_fail;
	}

	rd_count = read(fd, file_data, size);
	if (rd_count != size) {
		printf("read config file error, size: %ld rd_count:%ld\n",size,rd_count);
		ret = -EIO;
		goto read_fail;
	}

	ret = xrbtc_sdd_init(size);
	if (ret) {
		printf("xrbtc_sdd_init failed(%d)\n", ret);
		ret = -EIO;
		goto sdd_init_fail;
	}

	wr_count = xrbtc_sdd_write(file_data, size);
	if (wr_count != size) {
		printf("xrbtc_sdd_write failed(%d)\n", ret);
		ret = -EIO;
		goto sdd_write_fail;
	}

sdd_write_fail:
	/* should not be executed here, as controller would call it after getting sdd information */
	//xrbtc_sdd_deinit();

sdd_init_fail:
read_fail:
	free(file_data);

malloc_fail:
	close(fd);

	return ret;
#endif
	return 0;
}

int hal_controller_init()
{
    int ret = 0;
	printf("=========>[%s:%s, :%d]\n", __FILE__, __func__, __LINE__);

    if (driver_flags & VIRTUAL_HCI_ENABLED)
            return -EALREADY;

    ret = load_sdd_file();
    if (ret) {
        printf("load_sdd_file failed\n");
    } else {
        printf("%s %d load_sdd_file success!\n",__func__,__LINE__);
    }

    ret = xrbtc_init();
    xrbtc_enable();

    if (ret) {
        printf("blec failed(%d)\n", ret);
        return -ENODEV;
    }

    driver_flags |= VIRTUAL_HCI_ENABLED;

    return 0;
}

int hal_controller_deinit()
{
    driver_flags &= ~VIRTUAL_HCI_ENABLED;

    xrbtc_disable();
    xrbtc_deinit();

    return 0;
}

uint32_t hal_controller_ready(void)
{
    return driver_flags;
}

void hal_controller_set_mac(uint8_t *mac)
{
    int i = 0;
    uint8_t write_mac[SET_MAC_COMMAND_LEN] = {0x01, 0x32, 0xfc, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (i = 0; i < MAC_LEN; i++) {
        write_mac[SET_MAC_COMMAND_LEN - 2 - i] = mac[i];
    }
    hal_hci_write(write_mac[0], write_mac + 1, sizeof(write_mac) - 1);
}

