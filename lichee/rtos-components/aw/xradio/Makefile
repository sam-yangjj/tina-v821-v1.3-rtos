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
ifeq ($(CONFIG_DRIVER_XR819), y)
CFLAGS += -DCONFIG_OS_NUTTX
else ifeq ($(CONFIG_DRIVER_XR829), y)
CFLAGS += -DCONFIG_OS_NUTTX
else ifeq ($(CONFIG_DRIVER_R128), y)
CFLAGS += -DCONFIG_OS_TINA
else ifeq ($(CONFIG_DRIVER_V821), y)
CFLAGS += -DCONFIG_OS_TINA
endif
CFLAGS += $(CFLAGS_COMMON)
obj-y += sdd/sdd.o    # should only on RV?
obj-y += sys/atomic.o
obj-y += os/os_mutex.o
obj-y += os/os_queue.o
obj-y += os/os_semaphore.o
obj-y += os/os_thread.o
obj-y += os/os_time.o
obj-y += os/os_timer.o
obj-y += os/os_error.o
obj-y += os/os_util.o
obj-y += cmd/cmd_util.o
XRADIO_AMP_OBJS += sysinfo/sysinfo.o
XRADIO_AMP_OBJS += sys_ctrl/container.o
XRADIO_AMP_OBJS += sys_ctrl/event_queue.o
XRADIO_AMP_OBJS += sys_ctrl/looper.o
XRADIO_AMP_OBJS += sys_ctrl/observer.o
XRADIO_AMP_OBJS += sys_ctrl/publisher.o
XRADIO_AMP_OBJS += sys_ctrl/sys_ctrl.o

#XRADIO_AMP_OBJS += cmd/cmd_util.o
ifeq ($(CONFIG_ARCH_SUN20IW2), y)
ifneq ($(CONFIG_ETF), y)
XRADIO_AMP_OBJS += rf/cmd_rf.o
endif
endif
ifeq ($(CONFIG_ARCH_SUN300IW1), y)
ifneq ($(CONFIG_ETF), y)
ifeq ($(CONFIG_MUTIL_NET_STACK), y)
CFLAGS += -I drivers/drv/wireless/xradio
endif
ifeq ($(strip $(filter $(CONFIG_XRADIO_RF_CMD) $(CONFIG_SUPPORT_EXP_RF_CMD), y)), y)
XRADIO_AMP_OBJS += rf/cmd_rf.o
endif
endif
endif

##use search:
#_BASE_DIRS += components/common/aw/xradio/sdd    # should only on RV?
#_BASE_DIRS += components/common/aw/xradio/sys
#_BASE_DIRS += components/common/aw/xradio/os
#_BASE__DIRS := $(shell find $(_BASE_DIRS) -type d)
#_BASE_SRCS := $(sort $(basename $(foreach dir,$(_BASE__DIRS),$(wildcard $(dir)/*.[csS]))))
#_BASE_W_OBJS := $(addsuffix .o,$(_BASE_SRCS))
#obj-y += $(_BASE_W_OBJS)
#_AMP_DIRS += components/common/aw/xradio/sysinfo
#_AMP_DIRS += components/common/aw/xradio/sys_ctrl
#_AMP_DIRS += components/common/aw/xradio/cmd
#_AMP__DIRS := $(shell find $(_AMP_DIRS) -type d)
#_AMP_SRCS := $(sort $(basename $(foreach dir,$(_AMP__DIRS),$(wildcard $(dir)/*.[csS]))))
#_AMP_W_OBJS := $(addsuffix .o,$(_AMP_SRCS))
#XRADIO_AMP_OBJS += $(_AMP_W_OBJS)

obj-$(CONFIG_AW_COMPONENT_XBTC_UART) += xbtc_uart/

ifeq ($(CONFIG_COMPONENTS_AMP), y)
ifeq ($(CONFIG_ARCH_RISCV_RV64), y)
obj-y += $(XRADIO_AMP_OBJS)
endif
ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
obj-y += $(XRADIO_AMP_SER_OBJS)
endif
else
obj-y += $(XRADIO_AMP_OBJS)
endif

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I kernel/Interface_remap/include

#ifeq ($(CONFIG_DRIVERS_XRADIO), y)
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I drivers/drv/wireless/xradio/include
CFLAGS += -I drivers/rtos-hal/include/hal
#endif

CFLAGS += -I include
CFLAGS += -I os
CFLAGS += -I os/include
CFLAGS += -I include/freertos/

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)
