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
include components/common/aw/xradio/plt_def.mk

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

ifeq ($(CONFIG_DRIVERS_XRADIO),y)
  ifeq ($(CONFIG_ETF), y)
  else
    WLAN_AMP_SER_OBJS += os/sys/mbuf/mbuf_0.o
  endif
  WLAN_AMP_SER_OBJS += driver/port/gpio.o

  ifeq ($(CONFIG_DRIVER_V821), y)
    WLAN_OBJS += driver/port/xr_chip.o
  else ifeq ($(CONFIG_DRIVER_R128), y)
  else
    WLAN_OBJS += driver/port/sdio.o
  endif
  #WLAN_OBJS += driver/port/delay.o
  WLAN_OBJS += driver/port/xr_drv_mem.o
  WLAN_OBJS += driver/port/ffs.o
  #WLAN_OBJS += driver/port/hexdump.o
  WLAN_OBJS += driver/port/workqueue.o

  ifeq ($(CONFIG_ETF), y)
    WLAN_OBJS += adapter/cmd_wlan/cmd_etf.o
    WLAN_OBJS += $(WLAN_AMP_SER_OBJS)
  else
    ifeq ($(CONFIG_DRIVERS_XRADIO_CMD),y)
      WLAN_AMP_OBJS += adapter/cmd_wlan/utils.o
      WLAN_AMP_OBJS += adapter/cmd_wlan/command.o
      WLAN_AMP_OBJS += adapter/cmd_wlan/cmd_ifconfig.o
      WLAN_AMP_OBJS += adapter/cmd_wlan/cmd_wlan.o
    endif
    WLAN_AMP_OBJS += adapter/cmd_wlan/cmd_lmac.o
    WLAN_AMP_OBJS += adapter/cmd_wlan/cmd_wlancmd.o
    WLAN_AMP_OBJS += ethernetif.o
    WLAN_AMP_OBJS += adapter/net_ctrl/net_ctrl.o
    WLAN_AMP_OBJS += adapter/net_ctrl/net_init.o
    WLAN_AMP_OBJS += adapter/net_ctrl/net_sys.o
    WLAN_AMP_OBJS += adapter/net_ctrl/sc_assistant_port.o
    WLAN_AMP_OBJS += adapter/sys_monitor/sys_monitor.o

    ifeq ($(CONFIG_COMPONENTS_AMP), y)
      ifeq ($(CONFIG_ARCH_RISCV_RV64), y)
        WLAN_OBJS += $(WLAN_AMP_OBJS)
      endif
      ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
        WLAN_OBJS += $(WLAN_AMP_SER_OBJS)
      endif
    else
      WLAN_OBJS += $(WLAN_AMP_OBJS) $(WLAN_AMP_SER_OBJS)
    endif

    CFLAGS += -I drivers/drv/wireless/xradio/adapter/net_ctrl
    CFLAGS += -I drivers/drv/wireless/xradio/adapter/sys_ctrl
    CFLAGS += -I drivers/drv/wireless/xradio/adapter/cmd_wlan
    CFLAGS += -I drivers/drv/wireless/xradio/adapter/sys_monitor
  endif
  CFLAGS += -I components/common/aw/xradio/cmd
endif

CFLAGS += -I drivers/drv/wireless/xradio/include
CFLAGS += -I drivers/drv/wireless/xradio
CFLAGS += -I drivers/drv/wireless/xradio/os
CFLAGS += -I drivers/drv/wireless/xradio/os/include
CFLAGS += -I include/freertos/
CFLAGS += -I projects/

CFLAGS += $(PLT_CFLAGS)

ifeq ($(CONFIG_DRIVERS_XRADIO),y)

  ifeq ($(CONFIG_DRIVER_XR819), y)
    CFLAGS += -I drivers/chip-src/sdmmc
    CFLAGS += -I include/drivers/sdmmc
  else ifeq ($(CONFIG_DRIVER_XR829), y)
    CFLAGS += -I drivers/chip-src/sdmmc
    CFLAGS += -I include/drivers/sdmmc
  else ifeq ($(CONFIG_DRIVER_R128), y)
    CFLAGS += -I drivers/rtos-hal/hal/source/prcm/prcm-sun20iw2
    CFLAGS += -I drivers/rtos-hal/include/hal
  endif

  ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
    LWIP = lwip-2.1.2
  else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
    LWIP = lwip-2.0.3
  endif

  CFLAGS += -I drivers/drv/wireless/xradio/os/include/net
  CFLAGS += -I components/common/thirdparty/network/udhcp-0.9.8/include
  CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include

  CFLAGS += -I include/drivers
  CFLAGS += -I components/common/aw/network/tcpip
  CFLAGS += -I components/thirdparty/console
  CFLAGS += -I drivers/drv/wireless/sunxi-platform
endif

ifeq ($(CONFIG_MUTIL_NET_STACK),y)
  CFLAGS += -I components/common/thirdparty/openamp/include
  CFLAGS += -I components/common/aw/rpbuf/include
  CFLAGS += -I components/common/aw/xradio/include/sysinfo
  CFLAGS += -I drivers/drv/wireless/xradio/xrlink
  CFLAGS += -I drivers/drv/wireless/xradio/driver
  WLAN_OBJS += xrlink/checksum.o
  WLAN_OBJS += xrlink/command.o
  WLAN_OBJS += xrlink/net_utils.o
  WLAN_OBJS += xrlink/wlan.o
  WLAN_OBJS += xrlink/txrx.o
  WLAN_OBJS += xrlink/xrlink.o
  WLAN_OBJS += xrlink/xradio_rpmsg.o
  WLAN_OBJS += xrlink/xradio_rpbuf.o
endif

obj-y +=$(WLAN_OBJS)
