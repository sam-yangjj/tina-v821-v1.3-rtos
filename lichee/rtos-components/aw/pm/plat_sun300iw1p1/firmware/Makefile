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
#makefile for standby.bin

CROSS_RTOSDIR ?= $(shell cd ../../../../../rtos && pwd)
CROSS_COMPILE ?= $(CROSS_RTOSDIR)/tools/Xuantie-900-gcc-elf-newlib-x86_64-V2.10.1/bin/riscv64-unknown-elf-
CROSS_CONFIG  := $(CROSS_RTOSDIR)/.config
STANDBY_LDFILE := standby.lds
STANDBY_TARGET := standby.bin

RTOS_PROJECT_NAME ?= v821_e907_evb1
RTOS_AUTOCONF_HEAD_FILE ?= $(CROSS_RTOSDIR)/include/generated/$(RTOS_PROJECT_NAME)/autoconf.h

include $(CROSS_RTOSDIR)/.config

DEFFLAGS := -DSTANDBY_FIRMWARE

INCFLAGS += -Iinclude/
INCFLAGS += -I../include/
INCFLAGS += -I../../include/
INCFLAGS += -I../drivers/include/
INCFLAGS += -I../drivers/flash/include/
INCFLAGS += -I../drivers/dram/include/
INCFLAGS += -include $(RTOS_AUTOCONF_HEAD_FILE)

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
standby-c := $(filter-out ../drivers/load_image.c ../drivers/load_image_common.c ../drivers/heap_retain.c, $(wildcard *.c ../drivers/*.c))
standby-s := $(filter-out link.S, $(wildcard *.S))
standby-y := $(patsubst %.c,%.o,$(standby-c))
standby-y += $(patsubst %.S,%.o,$(standby-s))
standby-dep := $(patsubst %.o, %.d, $(standby-y))
standby-lib := -L../drivers/dram/ -ldram -ldram_standby
-include $(standby-dep)

all: $(STANDBY_TARGET)

../drivers/hal_cache.o:../drivers/hal_cache.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS_O2) $(INCFLAGS) $(DEFFLAGS) $< -o $@

%.o:%.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $(INCFLAGS) $(DEFFLAGS) $< -o $@

%.o:%.S
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $(INCFLAGS) $(DEFFLAGS) $< -o $@

$(STANDBY_LDFILE): link.S
	$(CROSS_COMPILE)gcc -E -P $(CFLAGS) $(INCFLAGS)\
		link.S > $(STANDBY_LDFILE)

standby.elf: $(standby-y) $(STANDBY_LDFILE)
	$(CROSS_COMPILE)gcc -T $(STANDBY_LDFILE) $(LDFLAGS) $(standby-y) $(standby-lib) -o $@ -Wl,-Map=standby.map

standby.bin: standby.elf
	$(CROSS_COMPILE)objcopy -O binary standby.elf standby.bin
	$(CROSS_COMPILE)objdump -D standby.elf > standby.lst

clean:
	-rm -f $(standby-y) $(standby-dep) *.d *.lds *.lst *.map

distclean:
	-rm -f $(standby-y) $(standby-dep) standby.* *.d *.lds *.lst *.map
