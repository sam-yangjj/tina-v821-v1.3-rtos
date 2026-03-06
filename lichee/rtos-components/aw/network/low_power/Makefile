# Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
#
# Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
# the people's Republic of China and other countries.
# All Allwinner Technology Co.,Ltd. trademarks are used with permission.
#
# DISCLAIMER
# THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
# IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
# IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
# ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
# ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
# COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
# YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
#
# THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
# PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
# WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
# THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
# OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#
obj-y += src/main.o
obj-y += src/lp_rpmsg.o
obj-y += src/lp_ctrl_msg.o

ifneq ($(CONFIG_COMPONENTS_LOW_POWER_PIR)$(CONFIG_COMPONENTS_LOW_POWER_POWERKEY),)
obj-y += src/dev_wuio_manager.o
endif

ifeq ($(CONFIG_COMPONENTS_LOW_POWER_PIR),y)
obj-y += src/dev_pir.o
endif

ifeq ($(CONFIG_COMPONENTS_LOW_POWER_POWERKEY),y)
obj-y += src/dev_powerkey_wakeup.o
endif

ifeq ($(CONFIG_COMPONENTS_LOW_POWER_APP_TCP),y)
obj-y += src/tcp_client.o
else ifeq ($(CONFIG_COMPONENTS_LOW_POWER_APP_MQTT),y)
obj-y += src/mqtt_client.o
else ifeq ($(CONFIG_COMPONENTS_LOW_POWER_WLAN_FW_TCP),y)
obj-y += src/wlan_fw_keepalive_client.o
endif

ifeq ($(CONFIG_COMPONENTS_LOW_POWER_APP_MQTT_CAPACITY_READ),y)
obj-y += src/capacity_read.o
endif

ifeq ($(CONFIG_COMPONENTS_LOW_POWER_APP_RTC_WATCH_DOG),y)
obj-y += src/lp_rtc_watchdog.o
endif

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
#CFLAGS += -DMQTT_DO_NOT_USE_CUSTOM_CONFIG

CFLAGS += -Icomponents/common/aw/network/low_power/include
CFLAGS += -I $(BASE)/components/common/thirdparty/openamp/include
CFLAGS += -Icomponents/common/thirdparty/network/mqttc/coreMQTT-2.3.1/example/
CFLAGS += -Icomponents/common/thirdparty/network/mqttc/coreMQTT-2.3.1/example/logging-stack/
CFLAGS += -Icomponents/common/thirdparty/network/mqttc/coreMQTT-2.3.1/port/
CFLAGS += -Icomponents/common/thirdparty/network/mqttc/coreMQTT-2.3.1/source/include/
CFLAGS += -Icomponents/common/thirdparty/network/mqttc/coreMQTT-2.3.1/source/interface/
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include/mbedtls/configs
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include/mbedtls
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Ikernel/FreeRTOS-orig/Source/include/
CFLAGS += -Icomponents/common/aw/xradio/include/
CFLAGS += -Idrivers/drv/wireless/xradio/adapter/net_ctrl
CFLAGS += -Idrivers/drv/wireless/xradio/include
CFLAGS += -Idrivers/drv/wireless/xradio/xrlink
CFLAGS += -Icomponents/common/aw/xradio/cmd/
CFLAGS += -Icomponents/common/aw/xradio/include/sysinfo/
CFLAGS += -Icomponents/common/thirdparty/network/udhcp-0.9.8/include
CFLAGS += -Icomponents/common/aw/xradio/include/sys_ctrl/
