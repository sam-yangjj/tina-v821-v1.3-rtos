CFLAGS += -I $(BASE)/components/common/thirdparty/openamp/include
CFLAGS += -I components/common/thirdparty/openamp/sunxi_helper

obj-y += mem_map.o
obj-y += msgbox_ipi.o
obj-y += openamp.o
obj-y += remoteproc_common.o
obj-y += openamp_platform.o
obj-y += shmem_ops.o
obj-y += rpmsg_ns.o
obj-$(CONFIG_AW_RPMSG_PERF_TRACE) += rpmsg_perf.o

obj-$(CONFIG_ARCH_SUN8IW20) += sunxi/sun8iw20/
obj-$(CONFIG_ARCH_SUN55IW3) += sunxi/sun55iw3/
obj-$(CONFIG_ARCH_SUN60IW1) += sunxi/sun60iw1/
obj-$(CONFIG_ARCH_SUN55IW6) += sunxi/sun55iw6/
obj-$(CONFIG_ARCH_SUN300IW1) += sunxi/sun300iw1/

ifeq ($(CONFIG_COMPONENTS_AMP_USER_MEMORY), y)
CFLAGS += -I components/common/aw/amp_app/amp_user_memory/
endif

ifeq ($(CONFIG_COMPONENTS_AMP_USER_RESOURCE), y)
CFLAGS += -I components/common/aw/amp_app/amp_user_resource/
endif

ifeq ($(CONFIG_AW_RPMSG_PERF_TRACE), y)
CFLAGS += -I components/common/aw/amp_app/amp_timestamp/
endif
