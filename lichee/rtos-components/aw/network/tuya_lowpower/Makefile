SOURCE = $(notdir $(subst .c,.o,$(wildcard components/common/aw/network/tuya_lowpower/src/*.c)))

SOURCE_O = $(addprefix src/,$(SOURCE))

obj-y += $(SOURCE_O)

ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_16_0),y)
MBEDTLS = mbedtls-2.16.0
else ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_7_0),y)
MBEDTLS = mbedtls-2.7.0
endif

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -Icomponents/common/aw/rpbuf/include
CFLAGS += -Icomponents/common/aw/network/tuya_lowpower/include
CFLAGS += -Icomponents/common/aw/network/tuya_lowpower/src
CFLAGS += -Icomponents/common/thirdparty/md5
CFLAGS += -Icomponents/common/thirdparty/openamp/include
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include/mbedtls
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include

CFLAGS += -Ikernel/FreeRTOS-orig/Source/include/
CFLAGS += -Icomponents/common/aw/xradio/include/
CFLAGS += -Idrivers/drv/wireless/xradio/adapter/net_ctrl
CFLAGS += -Idrivers/drv/wireless/xradio/include
CFLAGS += -Idrivers/drv/wireless/xradio/xrlink
CFLAGS += -Icomponents/common/aw/xradio/cmd/
CFLAGS += -Icomponents/common/thirdparty/network/udhcp-0.9.8/include
CFLAGS += -Icomponents/common/aw/xradio/include/sys_ctrl/
