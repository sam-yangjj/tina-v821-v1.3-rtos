include $(DBUILD_ROOT).dbuild/package.mk

obj-$(CONFIG_COMPONENTS_OPENAMP) += mem_map.o
obj-$(CONFIG_COMPONENTS_OPENAMP) += msgbox_ipi.o
obj-$(CONFIG_COMPONENTS_OPENAMP) += openamp.o
obj-$(CONFIG_COMPONENTS_OPENAMP) += openamp_platform.o
obj-$(CONFIG_COMPONENTS_OPENAMP) += shmem_ops.o

$(eval $(call BuildPackage,sunxi_helper))

ifeq ($(CONFIG_ARCH_SUN55IW3), y)
	include $(BASE)/components/common/thirdparty/openamp/sunxi_helper/sunxi/sun55iw3/objects.mk
endif

ifeq ($(CONFIG_ARCH_SUN60IW1), y)
	include $(BASE)/components/common/thirdparty/openamp/sunxi_helper/sunxi/sun60iw1/objects.mk
endif
