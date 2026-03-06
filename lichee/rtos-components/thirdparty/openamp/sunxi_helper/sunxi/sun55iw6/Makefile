obj-y += rproc_ops.o
obj-y += rsc_tab.o
ifeq ($(CONFIG_ARCH_DSP), y)
obj-y += dsp_memory.o
else
obj-y += memory.o
endif

ifeq ($(CONFIG_COMPONENTS_AMP_USER_MEMORY), y)
CFLAGS += -I components/common/aw/amp_app/amp_user_memory/
endif

ifeq ($(CONFIG_COMPONENTS_AMP_USER_RESOURCE), y)
CFLAGS += -I components/common/aw/amp_app/amp_user_resource/
endif
