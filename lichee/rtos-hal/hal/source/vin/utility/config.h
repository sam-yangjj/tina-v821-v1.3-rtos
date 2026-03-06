/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * config.h for device tree and sensor list parser.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *	Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CONFIG__H__
#define __CONFIG__H__

#include <hal_cfg.h>
#include <script.h>

struct fetchfunarr {
    char *sub;
    int(*fun)(char *main_name, char *sub_name, int sensor_id);
};

int parse_modules_from_sys_config(void);

#endif /* __CONFIG__H__ */