/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 Bootlin
 *
 * Author: Joao Marcos Costa <joaomarcos.costa@bootlin.com>
 *
 * squashfs.h: SquashFS filesystem implementation.
 */

#ifndef _SQFS_H_
#define _SQFS_H_


#define SQFS_DIRENT_NAME_LEN 256

struct squashfs_dir_stream;
struct squashfs_dirent {
	/** @type:      one of FS_DT_x (not a mask) */
	unsigned int type;
	/** @size:      file size */
	u64 size;
	/** @flags:     attribute flags (FS_ATTR_*) */
	u32 attr;
	/** create_time:    time of creation */
	//struct rtc_time create_time;
	/** access_time:    time of last access */
	//struct rtc_time access_time;
	/** change_time:    time of last modification */
	//struct rtc_time change_time;
	/** name:       file name */
	char name[SQFS_DIRENT_NAME_LEN];
};

int sqfs_opendir(const char *filename, struct squashfs_dir_stream **dirsp);
int sqfs_readdir(struct squashfs_dir_stream *fs_dirs, struct squashfs_dirent **dentp);
int sqfs_probe(u32 start, u32 size, u32 blksz);
int sqfs_read(const char *filename, void *buf, u32 offset, u32 len, u32 *actread);
int sqfs_size(const char *filename, uint32_t *size);
int sqfs_exists(const char *filename);
void sqfs_close(void);
void sqfs_closedir(struct squashfs_dir_stream *dirs);

#endif /* SQFS_H  */
