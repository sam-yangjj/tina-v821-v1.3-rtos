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

BLUEDROID_CMD_BASE:=$(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

BLUEDROID_CMD_DIRS := $(BLUEDROID_CMD_BASE)

BLUEDROID_CMD_DIRS += $(shell find $(BLUEDROID_CMD_BASE)/cmd_bt -type d)

ifneq ($(CONFIG_BT_DUAL_HOST), y)
BLUEDROID_CMD_DIRS += $(shell find $(BLUEDROID_CMD_BASE)/cmd_mesh -type d)
endif

BLUEDROID_CMD_SRCS := $(basename $(foreach dir,$(BLUEDROID_CMD_DIRS),$(wildcard $(dir)/*.[csS])))

BLUEDROID_CMD_OBJS += $(addsuffix .o,$(BLUEDROID_CMD_SRCS))

ifeq ($(CONFIG_BT_DUAL_HOST), y)
FIRTER_OBJS := $(BLUEDROID_CMD_BASE)/cmd_bt/cmd_gattc.o $(BLUEDROID_CMD_BASE)/cmd_bt/cmd_gatts.o $(BLUEDROID_CMD_BASE)/cmd_bt/cmd_ble.o
BLUEDROID_CMD_OBJS := $(filter-out $(FIRTER_OBJS),$(BLUEDROID_CMD_OBJS))
endif

ifeq ($(CONFIG_COMPONENTS_AW_TINY_ALSA_LIB), y)
FIRTER_OBJS := $(BLUEDROID_CMD_BASE)/bt_audio_fake.o
BLUEDROID_CMD_OBJS := $(filter-out $(FIRTER_OBJS),$(BLUEDROID_CMD_OBJS))
endif

BLUEDROID_BASE_DIR :=components/common/thirdparty/bluetooth/bluedroid

CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/aw/bluetooth/xradio/cmd/bluedroid
CFLAGS += -I components/aw/bluetooth/xradio/cmd/bluedroid/cmd_bt
CFLAGS += -I drivers/drv/wireless/xradio/adapter
CFLAGS += -I components/aw/bluetooth/xradio/framework

CFLAGS += -I components/common/aw/xradio
CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/common/aw/xradio/os
CFLAGS += -I components/common/aw/xradio/os/include

CFLAGS += -I $(BLUEDROID_BASE_DIR)/common/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/common/osi/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/hci/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/hci/include/hci
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/common/include/common
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/stack/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/common/include
CFLAGS += -I components/common/thirdparty/bluetooth/tools/hcidump/include

CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/api/include/api
ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM), y)
CFLAGS += -I components/common/aw/AudioSystem/include
endif

ifneq ($(CONFIG_BT_DUAL_HOST), y)
# ifeq ($(CONFIG_BLE_MESH), y)
CFLAGS += -I $(BLUEDROID_BASE_DIR)/xr_ble_mesh/api/core/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/xr_ble_mesh/api/models/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/xr_ble_mesh/api
# endif
endif

CFLAGS += -I include/freertos/

#include components/common/aw/xradio/plt_def.mk
#CFLAGS += $(PLT_CFLAGS)

obj-y += $(subst $(BLUEDROID_CMD_BASE)/, , $(BLUEDROID_CMD_OBJS))
