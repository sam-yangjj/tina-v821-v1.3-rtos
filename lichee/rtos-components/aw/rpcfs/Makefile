obj-y += open.o
obj-y += close.o
obj-y += read_write.o
obj-y += link.o
obj-y += stat.o
obj-y += rename.o
obj-y += dir.o
obj-$(CONFIG_COMPONENTS_RPCFS_EXPORT_CMD) += export_cmd.o
obj-$(CONFIG_COMPONENT_VFS) += rpcfs-vfs.o

subdir-ccflags-y += -I components/common/aw/sunxi-rpc/include
subdir-ccflags-y += -I components/common/aw/rpcfs/include
