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
# Create a compressed Tina rtos image from the original Tina rtos(rt_system.elf)

BASE_PATH:=$(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))
BUILD_BASE:=$(patsubst %/,%,$(BASE_PATH))
RTOS_BASE:= $(BUILD_BASE)/../../../../rtos
-include $(RTOS_BASE)/.config

# Q = @ :No compilation LOG output
# Q = :Compilation LOG output
Q = @

#Defaults for compile/build toolchain
CONFIG_ ?= CONFIG_
override TOOLCHAIN 	:= $(shell echo $($(CONFIG_)TOOLCHAIN))
override AR			= $(RTOS_BASE)/$(TOOLCHAIN)ar
override AS			= $(RTOS_BASE)/$(TOOLCHAIN)as
override CC		 	= $(RTOS_BASE)/$(TOOLCHAIN)gcc
override CXX		= $(RTOS_BASE)/$(TOOLCHAIN)g++
override LD			= $(RTOS_BASE)/$(TOOLCHAIN)ld
override OBJCOPY	= $(RTOS_BASE)/$(TOOLCHAIN)objcopy
override OBJDUMP	= $(RTOS_BASE)/$(TOOLCHAIN)objdump
override SIZE		= $(RTOS_BASE)/$(TOOLCHAIN)size
override STRIP		= $(RTOS_BASE)/$(TOOLCHAIN)strip

DB_QUOTES := "
TOOLCHAIN_CC_MACHFLAGS 	:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_MACH_FLAGS))
TOOLCHAIN_LD_FLAGS  := $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_LD_FLAGS))

size_append = printf $(shell						\
dec_size=0;								\
for F in $1; do								\
	fsize=$$(stat -c "%s" $$F);					\
	dec_size=$$(expr $$dec_size + $$fsize);				\
done;									\
printf "%08x\n" $$dec_size |						\
	sed 's/\(..\)/\1 /g' | {					\
		read ch0 ch1 ch2 ch3;					\
		for ch in $$ch3 $$ch2 $$ch1 $$ch0; do			\
			printf '%s%03o' '\\' $$((0x$$ch)); 		\
		done;							\
	}								\
)

cmd_gzip = (cat $(filter-out FORCE,$^) | \
	gzip -9 && $(call size_append, $(filter-out FORCE,$^))) > $@ || \
	(rm -f $@ ; false)

cmd_lzma = (cat $(filter-out FORCE,$^) | \
	lzma -9 && $(call size_append, $(filter-out FORCE,$^))) > $@ || \
	(rm -f $@ ; false)

cmd_lzo = (cat $(filter-out FORCE,$^) | \
	lzop -9 && $(call size_append, $(filter-out FORCE,$^))) > $@ || \
	(rm -f $@ ; false)

cmd_xz = (cat $(filter-out FORCE,$^) | \
	xz -9 && $(call size_append, $(filter-out FORCE,$^))) > $@ || \
	(rm -f $@ ; false)

cmd_lz4 = (cat $(filter-out FORCE,$^) | \
	lz4c -l -c1 stdin stdout && $(call size_append, $(filter-out FORCE,$^))) > $@ || \
	(rm -f $@ ; false)

ifeq ($(CONFIG_KERNEL_COMPRESS_GZIP),y)
CONFIG_KERNEL_COMPRESS_METHON := 0x1
compress_cmd = $(cmd_gzip)
endif
ifeq ($(CONFIG_KERNEL_COMPRESS_LZ0),y)
CONFIG_KERNEL_COMPRESS_METHON := 0x2
compress_cmd = $(cmd_lz0)
endif
ifeq ($(CONFIG_KERNEL_COMPRESS_LZMA),y)
CONFIG_KERNEL_COMPRESS_METHON := 0x3
compress_cmd = $(cmd_lzma)
endif
ifeq ($(CONFIG_KERNEL_COMPRESS_XZ),y)
CONFIG_KERNEL_COMPRESS_METHON := 0x4
compress_cmd = $(cmd_xz)
endif
ifeq ($(CONFIG_KERNEL_COMPRESS_LZ4),y)
compress_cmd = $(cmd_lz4)
CONFIG_KERNEL_COMPRESS_METHON := 0x5
endif

CFLAGS	:= -O0 -Wall
CFLAGS	+= -fpic $(call cc-option,-mno-single-pic-base,) -fno-builtin -I$(BUILD_BASE)
CFLAGS	+= -include $(RTOS_BASE)/include/generated/$(RTOS_PROJECT_NAME)/autoconf.h
CFLAGS	+= -I $(RTOS_BASE)/include
CFLAGS	+= -I $(RTOS_BASE)/include/arch/riscv

CFLAGS_string.o	:= -Os
NOSTDINC_FLAGS +=  -isystem $(shell $(CC) -print-file-name=include)

ZTEXTADDR	:= $(CONFIG_ARCH_START_ADDRESS) + $(CONFIG_ARCH_MEM_LENGTH) / 10 * 7
ZBSSADDR	:= ALIGN(8)
CPPFLAGS_link.lds := -DTEXT_START="$(ZTEXTADDR)" -DBSS_START="$(ZBSSADDR)"
CPPFLAGS_link.lds += -DCONFIG_KERNEL_COMPRESS_METHON="$(CONFIG_KERNEL_COMPRESS_METHON)"

%.o : %.S
	$(Q)$(CC) $(CFLAGS) $(TOOLCHAIN_CC_MACHFLAGS) -c $< -o $@

%.o : %.c
	$(Q)$(CC) $(CFLAGS) $(TOOLCHAIN_CC_MACHFLAGS) -c $< -o $@

# 为string.c单独设置优化级别为-Os
# string library code (-Os is enforced to keep it much smaller)
string.o : string.c
	$(Q)$(CC) $(CFLAGS_string.o) $(TOOLCHAIN_CC_MACHFLAGS) -c $< -o $@

OBJS	+= head.o piggy.o misc.o decompress.o _ashldi3.o string.o
ifeq ($(CONFIG_KERNEL_COMPRESS_DEBUG),y)
OBJS	+= backtrace.o
endif
ifeq ($(CONFIG_KERNEL_COMPRESS_ELF),y)
OBJS	+= elf_loader.o
endif
ifeq ($(CONFIG_ARCH_SUN300IW1P1),y)
OBJS	+= sun300iw1_sysmap.o
endif

ifeq ($(CONFIG_KERNEL_COMPRESS_ELF),y)
DATA := $(RTOS_BASE)/build/$(RTOS_PROJECT_NAME)/img/rt_system.elf
else
DATA := $(RTOS_BASE)/build/$(RTOS_PROJECT_NAME)/img/rt_system.bin
endif

$(BUILD_BASE)/link.lds: $(BUILD_BASE)/link.lds.S FORCE
	$(Q)$(CC) -E -P $(CFLAGS) $(NOSTDINC_FLAGS) $(CPPFLAGS_link.lds) $< > $@

# if defined(CONFIG_COMPONENTS_AMP) && defined(CONFIG_KERNEL_COMPRESS_ELF)
# we need special handling for elf headers
$(BUILD_BASE)/orig_data: $(DATA) FORCE
	@cp $< $@
	$(Q)$(STRIP) $@
ifeq ($(CONFIG_COMPONENTS_AMP), y)
ifeq ($(CONFIG_KERNEL_COMPRESS_ELF),y)
	$(BUILD_BASE)/scripts/markelf $@
endif
endif

$(BUILD_BASE)/piggy_data: $(BUILD_BASE)/orig_data FORCE
	$(Q)$(compress_cmd)

$(BUILD_BASE)/piggy.o: $(BUILD_BASE)/piggy.S $(BUILD_BASE)/piggy_data FORCE
	$(Q)$(CC) $(CFLAGS) $(TOOLCHAIN_CC_MACHFLAGS) -c $< -o $@

# Supply kernel BSS size to the decompressor via a linker symbol.
KBSS_SZ = $(shell $(SIZE) $(RTOS_BASE)/build/$(RTOS_PROJECT_NAME)/img/rt_system.elf | \
		awk 'END{print $$3}')
LDFLAGS_zImage = --defsym _kernel_bss_size=$(KBSS_SZ)
LDFLAGS_zImage += --defsym zelfaddr=$(CONFIG_KERNEL_COMPRESS_LOADADDR)
# Supply ZRELADDR to the decompressor via a linker symbol.
LDFLAGS_zImage += --defsym zreladdr=$(CONFIG_ARCH_START_ADDRESS)
# Report unresolved symbol references
LDFLAGS_zImage += --no-undefined
# Delete all temporary local symbols
LDFLAGS_zImage += -X

$(BUILD_BASE)/zImage: $(BUILD_BASE)/link.lds $(addprefix $(BUILD_BASE)/, $(OBJS)) FORCE
	$(Q)$(LD) $(TOOLCHAIN_LD_FLAGS) $(LDFLAGS_zImage) -T $(BUILD_BASE)/link.lds -o $@ $(addprefix $(BUILD_BASE)/, $(OBJS))

Image: $(BUILD_BASE)/zImage FORCE
ifeq ($(CONFIG_COMPONENTS_AMP), y)
	$(Q)$(OBJCOPY) -O binary -j .resource_table $(DATA) rsc.txt
	$(Q)$(OBJCOPY) --update-section .resource_table=rsc.txt $(BUILD_BASE)/zImage
	$(Q)rm rsc.txt
endif
	$(Q)cp $(BUILD_BASE)/zImage $(RTOS_BASE)/build/$(RTOS_PROJECT_NAME)/img/zImage
	$(Q)$(STRIP) $(RTOS_BASE)/build/$(RTOS_PROJECT_NAME)/img/zImage

.PHONY: clean
clean:
	$(Q)rm -rf $(BUILD_BASE)/*.o $(BUILD_BASE)/link.lds $(BUILD_BASE)/zImage $(BUILD_BASE)/piggy_data $(BUILD_BASE)/orig_data

.PHONY: FORCE
