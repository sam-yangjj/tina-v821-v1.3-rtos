obj-y += example/main.o
obj-y += port/port_network.o
obj-y += port/port_time.o
obj-y += source/core_http_client.o
obj-y += source/dependency/3rdparty/llhttp/src/api.o
obj-y += source/dependency/3rdparty/llhttp/src/http.o
obj-y += source/dependency/3rdparty/llhttp/src/llhttp.o


ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_16_0),y)
MBEDTLS = mbedtls-2.16.0
CFLAGS += -DCONFIG_COMPONENTS_MBEDTLS_2_16_0
else ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_7_0),y)
MBEDTLS = mbedtls-2.7.0
CFLAGS += -DCONFIG_COMPONENTS_MBEDTLS_2_7_0
endif

CFLAGS += -DMBEDTLS_CONFIG_FILE='<config-xr-mini-cliserv.h>'
#CFLAGS += -DHTTP_DO_NOT_USE_CUSTOM_CONFIG

CFLAGS += -Icomponents/thirdparty/include
CFLAGS += -Icomponents/common/thirdparty/network/httpc/coreHTTP-3.1.1/example/
CFLAGS += -Icomponents/common/thirdparty/network/httpc/coreHTTP-3.1.1/example/logging-stack
CFLAGS += -Icomponents/common/thirdparty/network/httpc/coreHTTP-3.1.1/port/
CFLAGS += -Icomponents/common/thirdparty/network/httpc/coreHTTP-3.1.1/source/include/
CFLAGS += -Icomponents/common/thirdparty/network/httpc/coreHTTP-3.1.1/source/dependency/3rdparty/llhttp/include/
CFLAGS += -Icomponents/common/thirdparty/network/httpc/coreHTTP-3.1.1/source/interface/
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include/mbedtls/configs
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include/mbedtls
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Ikernel/FreeRTOS-orig/Source/include/
CFLAGS += -Icomponents/common/aw/xradio/include/
