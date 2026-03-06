obj-y += virtio/virtio.o
obj-y += virtio/virtqueue.o

obj-y += rpmsg/rpmsg.o
obj-y += rpmsg/rpmsg_virtio.o

obj-y += remoteproc/elf_loader.o
obj-y += remoteproc/remoteproc.o
obj-y += remoteproc/remoteproc_virtio.o
obj-y += remoteproc/rsc_table_parser.o

obj-y += version.o

CFLAGS += -DVIRTIO_CACHED_VRINGS -DVIRTIO_CACHED_BUFFERS
#CFLAGS += -I $(BASE)/components/common/thirdparty/openamp/include

CFLAGS += -Wno-error=address-of-packed-member -Wno-address-of-packed-member

ifeq ($(CONFIG_AW_RPMSG_PERF_TRACE), y)
CFLAGS += -I components/common/aw/amp_app/amp_timestamp/
endif

