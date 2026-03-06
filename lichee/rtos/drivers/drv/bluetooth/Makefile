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
ifeq ($(CONFIG_DRIVERS_BLUETOOTH_XRADIO), y)
BT_DRV_BASE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

include $(BT_DRV_BASE)/driver/Makefile

ifeq ($(CONFIG_BT_DIST_MODE_CORRESPOND), y)
BT_DRV_AMP_DIRS += $(shell find $(BT_DRV_BASE)/hci_distribute -type d)
endif

BT_DRV_AMP_DIRS += $(shell find $(BT_DRV_BASE)/lib -type d)

BT_DRV_AMP_SRCS = $(basename $(foreach dir,$(BT_DRV_AMP_DIRS),$(wildcard $(dir)/*.[csS])))

BT_DRV_SER_SRCS = $(basename $(foreach dir,$(BT_DRV_SER_DIRS),$(wildcard $(dir)/*.[csS])))

BT_DRV_AMP_OBJS += $(addsuffix .o,$(BT_DRV_AMP_SRCS))

BT_DRV_SER_OBJS += $(addsuffix .o,$(BT_DRV_SER_SRCS))

ifeq ($(CONFIG_COMPONENTS_AMP_BT), y)
BT_DRV_OBJS += $(BT_DRV_AMP_OBJS)
ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
BT_DRV_OBJS += $(BT_DRV_SER_OBJS)
endif
else
ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
BT_DRV_OBJS += $(BT_DRV_AMP_OBJS)
BT_DRV_OBJS += $(BT_DRV_SER_OBJS)
endif
endif

CFLAGS += -I drivers/drv/bluetooth/driver
CFLAGS += -I drivers/drv/bluetooth/driver/common

CFLAGS += -I drivers/drv/bluetooth/include
CFLAGS += -I drivers/drv/bluetooth/include/lib
CFLAGS += -I drivers/drv/bluetooth/include/driver
CFLAGS += -I drivers/drv/driver/controller/$(XRADIO_CONTROLLER_SELECT)/include
CFLAGS += -I components/common/thirdparty/bluetooth/tools/hcidump/include/hcidump_xr
CFLAGS += -I components/aw/bluetooth/pm
CFLAGS += -I drivers/drv/bluetooth/include/hci_distribute

CFLAGS += -I components/common/aw/xradio/os
CFLAGS += -I components/common/aw/xradio/include

CFLAGS += -I drivers/rtos-hal/hal/source/prcm/
CFLAGS += -I drivers/rtos-hal/hal/source/ccmu/
CFLAGS += -I drivers/rtos-hal/hal/source/ccmu/sunxi-ng/
ifeq ($(CONFIG_ARCH_SUN20IW2), y)
CFLAGS += -I include/arch/arm/mach/sun20iw2p1/
else ifeq ($(CONFIG_ARCH_SUN300IW1), y)
CFLAGS += -I include/arch/riscv/sun300iw1/
endif
CFLAGS += -I include/arch/riscv/

CFLAGS += -I components/common/thirdparty/openamp/include
CFLAGS += -I components/common/aw/rpbuf/include
CFLAGS += -I drivers/drv/wireless/xradio/xrlink
CFLAGS += -I components/common/aw/xradio/include/sysinfo

CFLAGS += -I drivers/drv/wireless/xradio/sip_wifi_rpmsg
CFLAGS += -I components/common/thirdparty/openamp/include
CFLAGS += -I components/common/aw/rpbuf/include
CFLAGS += -I components/common/aw/efpg/
CFLAGS += -I components/common/thirdparty/rtt_memheap
CFLAGS += -I drivers/drv/wireless/xradio/driver
CFLAGS += -I kernel/Interface_remap/include/
CFLAGS += -I ../../lichee/rtos-components/aw/xradio/include

obj-y += $(subst $(BT_DRV_BASE)/, ,$(BT_DRV_OBJS))
endif