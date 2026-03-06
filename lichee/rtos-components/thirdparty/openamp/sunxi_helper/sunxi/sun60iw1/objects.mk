include $(DBUILD_ROOT).dbuild/package.mk

obj-y += rproc_ops.o
obj-y += rsc_tab.o
obj-y += memory.o

$(eval $(call BuildPackage,sun60iw1))
