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

ADAPTER_BASE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq ($(CONFIG_COMPONENTS_ZEPHYR), y)
ADAPTER_DIRS += $(shell find $(ADAPTER_BASE)/zephyr -type d)
endif

ifeq ($(CONFIG_COMPONENTS_BLUEDROID), y)
ADAPTER_DIRS += $(shell find $(ADAPTER_BASE)/bluedroid -type d)
endif

ADAPTER_SRCS = $(basename $(foreach dir,$(ADAPTER_DIRS),$(wildcard $(dir)/*.[csS])))

ADAPTER_OBJS += $(addsuffix .o,$(ADAPTER_SRCS))

CFLAGS += -I drivers/drv/bluetooth/include/lib
CFLAGS += -I drivers/drv/bluetooth/include/driver

CFLAGS += -I components/common/aw/xradio/include

ifeq ($(CONFIG_COMPONENTS_ZEPHYR), y)
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble/drivers/bluetooth
endif

ifeq ($(CONFIG_COMPONENTS_BLUEDROID), y)
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/common
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/common/include
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/common/osi/include
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/host/bluedroid/common/include
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/host/bluedroid/hci/include/hci
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/host/bluedroid/stack/include
endif

CFLAGS += -I components/common/thirdparty/bluetooth/tools/hcidump/include

obj-y += $(subst $(ADAPTER_BASE)/, , $(ADAPTER_OBJS))
