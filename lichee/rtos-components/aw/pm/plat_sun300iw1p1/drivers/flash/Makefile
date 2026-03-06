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
#makefile for flash.bin
CROSS_RTOSDIR ?= $(shell cd ../../../../../../rtos && pwd)
CROSS_COMPILE ?= $(CROSS_RTOSDIR)/tools/Xuantie-900-gcc-elf-newlib-x86_64-V2.10.1/bin/riscv64-unknown-elf-
CROSS_CONFIG  := $(CROSS_RTOSDIR)/.config
RTOS_PROJECT_NAME ?= v821_e907_evb1
RTOS_AUTOCONF_HEAD_FILE ?= $(CROSS_RTOSDIR)/include/generated/$(RTOS_PROJECT_NAME)/autoconf.h
include $(CROSS_RTOSDIR)/.config

FLASH_LDFILE := flash.lds
ifeq ($(CONFIG_PM_LOAD_STORAGE_TYPE_SPINOR), y)
FLASH_TARGET := flash_spinor.bin
endif
ifeq ($(CONFIG_PM_LOAD_STORAGE_TYPE_MMC), y)
FLASH_TARGET := flash_mmc.bin
endif
ifeq ($(CONFIG_PM_LOAD_STORAGE_TYPE_SPINAND), y)
FLASH_TARGET := flash_spinand.bin
endif

INCFLAGS := -include $(RTOS_AUTOCONF_HEAD_FILE)
INCFLAGS += -Iinclude/
ifeq ($(CONFIG_PM_LOAD_STORAGE_TYPE_SPINOR), y)
INCFLAGS += -Ispinor/include/
endif
ifeq ($(CONFIG_PM_LOAD_STORAGE_TYPE_MMC), y)
INCFLAGS += -Immc/include/
endif
ifeq ($(CONFIG_PM_LOAD_STORAGE_TYPE_SPINAND), y)
INCFLAGS += -Ispinand/include/
endif

GIT_COMMIT_ID := $(strip $(shell git rev-parse --verify HEAD 2>/dev/null))
GIT_DIRTY     := $(strip $(shell git diff --quiet -- $(CROSS_RTOSDIR)/components/common/aw/pm || echo "-dirty"))
GIT_COMMIT_ID_DIRTY_FLAG := "$(GIT_COMMIT_ID)$(GIT_DIRTY)"
GIT_CHANGE_ID := "$(strip $(shell git log -1 HEAD | grep "^ \{4,5\}Change-Id:" | sed 's/Change-Id://g' | tail -1))"
BUILD_DATE   := "$(strip $(shell date "+%Y-%m-%d %H:%M:%S"))"
BUILD_AUTHOR := "$(strip $(USER))"

HEADCFLAGS := -DEXT_HEAD_COMMIT_ID=\"$(GIT_COMMIT_ID_DIRTY_FLAG)\" \
        -DEXT_HEAD_CHANGE_ID=\"$(GIT_CHANGE_ID)\" \
	-DEXT_HEAD_BUILD_DATE=\"$(BUILD_DATE)\" \
	-DEXT_HEAD_BUILD_AUTHOR=\"$(BUILD_AUTHOR)\"

CFLAGS := -MMD -MP -g -Os -mtune=e907 -march=rv32imafcxthead -mabi=ilp32f -mcmodel=medlow -fsingle-precision-constant
CFLAGS_O2 := -MMD -MP -g -O2 -mtune=e907 -march=rv32imafcxthead -mabi=ilp32f -mcmodel=medlow -fsingle-precision-constant
LDFLAGS := $(CFLAGS) -nostartfiles --specs=nano.specs -Wl,--gc-sections -Wl,--whole-archive -Wl,--no-whole-archive
ifeq ($(CONFIG_PM_LOAD_STORAGE_TYPE_SPINOR), y)
flash-c := $(wildcard *.c spinor/*.c)
endif
ifeq ($(CONFIG_PM_LOAD_STORAGE_TYPE_MMC), y)
flash-c := $(wildcard *.c mmc/*.c)
endif
ifeq ($(CONFIG_PM_LOAD_STORAGE_TYPE_SPINAND), y)
flash-c := $(wildcard *.c spinand/*.c)
endif
flash-y := $(patsubst %.c,%.o,$(flash-c))
flash-dep := $(patsubst %.o, %.d, $(flash-y))
-include $(flash-dep)

all: $(FLASH_TARGET)

flash_base.o:flash_base.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS_O2) $(INCFLAGS) $(DEFFLAGS) $< -o $@

%.o:%.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $(INCFLAGS) $(DEFFLAGS) $< -o $@

%.o:%.S
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $(INCFLAGS) $(DEFFLAGS) $< -o $@

$(FLASH_LDFILE): flash_link.S
	$(CROSS_COMPILE)gcc -E -P $(CFLAGS) $(INCFLAGS) flash_link.S > $(FLASH_LDFILE)

flash.elf: $(flash-y) $(FLASH_LDFILE)
	$(CROSS_COMPILE)gcc -T $(FLASH_LDFILE) $(LDFLAGS) $(flash-y) -o $@ -Wl,-Map=flash.map
	$(CROSS_COMPILE)objdump -D flash.elf > flash.lst

$(FLASH_TARGET): flash.elf
	$(CROSS_COMPILE)objcopy -O binary --set-section-flags .data=alloc,load,contents flash.elf $(FLASH_TARGET)

clean:
	-rm -f $(flash-y) $(flash-dep) *.d *.lds *.lst *.map

distclean:
	-rm -f $(flash-y) $(flash-dep) flash.* *.d *.lds *.lst *.map *.bin
