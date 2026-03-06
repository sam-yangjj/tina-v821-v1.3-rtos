obj-y = sqfs.o sqfs_inode.o sqfs_dir.o sqfs_decompressor.o
obj-$(CONFIG_COMPONENTS_SQUASHFS_CMD) += sqfs_cmd.o

subdir-ccflags-y += -I components/common/aw/flash_read/include/
subdir-ccflags-y += -I components/common/thirdparty/decompress/
subdir-ccflags-y += -I components/common/thirdparty/fs/squashfs/include
