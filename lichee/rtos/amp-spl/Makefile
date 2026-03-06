SHELL=bash
srctree := $(PWD)
PATH :=$(srctree)/tools/bin:$(PATH)
MAKEFLAGS += --no-print-directory

include scripts/Build.include
-include .config

ifeq ($(V),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q=@
endif

ifneq ($(CONFIG_ARCH_RISCV),)
CFLAGS += -I$(srctree)/arch/riscv/includes/
endif

TOOLCHAIN	:= $(subst ",,$(CONFIG_TOOLCHAIN_PATH)/bin/$(CONFIG_TOOLCHAIN_PREFIX))
CC			=$(TOOLCHAIN)gcc
LD			=$(TOOLCHAIN)ld
AR			=$(TOOLCHAIN)ar
OBJC		=$(TOOLCHAIN)objcopy
OBJDUMP		=$(TOOLCHAIN)objdump
NM			=$(TOOLCHAIN)nm
SIZE		=$(TOOLCHAIN)size
STRIP		=$(TOOLCHAIN)strip

CFLAGS += -Iinclude
CFLAGS += -Iinclude/driver
CFLAGS += -Iinclude/system
CFLAGS += -Ilibrary/libfdt/include
CFLAGS += $(CFLAGS-y)
CFLAGS += -Iinclude/generated
CFLAGS += -include include/generated/autoconf.h
CFLAGS += -include conf.h

MARCH_FLAGS := $(subst ",,$(CONFIG_TOOLCHAIN_MACH_FLAGS))
MARCH_LD_FLAGS := $(subst ",,$(CONFIG_TOOLCHAIN_LD_FLAGS))
LINKER_SCRIPTS := board/ram.ld

TARTGET_BIN := $(subst ",,$(CONFIG_TARTGET_BIN))

# DRAM and ddr type
export CC LD AR OBJCOPY OBJDUMP STRIP LD_LIBRARY_PATH MARCH_LD_FLAGS
export CFLAGS
export HOSTCC := gcc
#export CONFIG_ = CFG_

target_name = amp-boot0
all: $(target_name).bin

.PHONY : FORCE menuconfig $(FORCE)

define filechk_version
	(echo "/*";\
	echo " * THIS IS CREATE WITH VERSION CHK";\
	echo " * DO NOT CHANGE IT MANUAL";\
	echo " */";\
	echo ;\
	echo "#ifndef _SUB_VER_";\
	echo "#define _SUB_VER_";\
	echo ;\
	echo "#define SUB_VER \"`scripts/setlocalversion`\"";\
	echo ;\
	echo "#endif /* _SUB_VER_ */";)
endef

TOOLCHAIN_PATH=$(subst ",,$(shell awk \-F= '/CONFIG_TOOLCHAIN_PATH/ {print $$2}' $@))
TOOLCHAIN_PACKAGE=$(subst ",,$(shell awk \-F= '/CONFIG_TOOLCHAIN_PACKAGE/ {print $$2}' $@))

DEFCONFIG=$(shell cat .amp-spl.project)

define uncompress_toolchain
	(echo "uncompress toolchain package...";\
	rm -rf $(TOOLCHAIN_PATH); \
	mkdir -p $(TOOLCHAIN_PATH)/; \
	tar zxvf $(TOOLCHAIN_PACKAGE) -C $(TOOLCHAIN_PATH); \
	echo "uncompress toolchain finish"; \
	touch ./.amp-spl.toolchain.flag;)
endef

sub_dir := arch driver library board

prepare := include/generated/conf.h scripts/basic/fixdep
prepare += include/generated/autoconf.h

$(addsuffix /obj-in.o,$(sub_dir)): $(prepare) FORCE
	$(Q)$(MAKE) -f scripts/Makefile.build dir=$(notdir $(patsubst %/,%,$(dir $@)))

clean:
	$(Q)echo CLEAN  objects
	$(Q)find -name '*.o' -delete -o  -name '.*.cmd' -delete -o -name '.*.d' -delete
	$(Q)find -name '*.tmp' -delete
	$(Q)find -wholename './scripts/basic/fixdep' -delete
	$(Q)find -path 'include/generated' -delete
	$(Q)find -wholename './$(target_name).*' -delete

include/generated/conf.h: Makefile FORCE
	$(call filechk,version)

# because old project use CFG_ so we change it
menuconfig: .config .amp-spl.project FORCE
	@scripts/mconf Kconfig
	$(Q)cp .config $(DEFCONFIG)

include/generated/autoconf.h: .config FORCE
	$(Q)mkdir -p include/generated
	$(Q)mkdir -p include/config
	$(Q)scripts/conf --silentoldconfig Kconfig

.amp-spl.project:
	$(Q)echo use make board/xxx/yyyy_defconfig to generate config file
	$(Q)echo
	$(Q)echo e.g. make board/sun300iw1/sun300iw1_defconfig
	$(Q)false

.config:
	$(Q)echo use make board/xxx/yyyy_defconfig to generate config file
	$(Q)echo
	$(Q)echo e.g. make board/sun300iw1/sun300iw1_defconfig
	$(Q)false

test:
	@echo CFLAGS_ARCH=$(CFLAGS-y)
	@echo TOOLCHAIN=$(TOOLCHAIN)

#$(notdir $(wildcard board/*_defconfig)): FORCE
$(wildcard board/*/*_defconfig): FORCE
	$(Q)scripts/conf --defconfig=$@ Kconfig
	$(Q)echo $@ > .amp-spl.project
	$(Q)if ! test -f .amp-spl.toolchain.flag;then $(uncompress_toolchain);fi;

scripts/basic/fixdep: scripts/basic/fixdep.c
	$(Q)gcc $^ -o $@

$(LINKER_SCRIPTS): $(addsuffix .S,$(LINKER_SCRIPTS)) FORCE
	$(Q)$(CC) $(c_flags) -E -P -o $@ $<

#compiler config options
#CFLAGS  += -g -c -nostdlib -Wall -Werror -Os $(MARCH_FLAGS)
CFLAGS  += -g -c -Wall -Werror -Os $(MARCH_FLAGS)

#linker config options
LD_FLAGS := $(subst ",,$(CONFIG_ELF_FORMAT))
LD_FLAGS += --stats -T$(LINKER_SCRIPTS) $(MARCH_LD_FLAGS)

deplibs += $(shell $(CC) $(MARCH_FLAGS) -print-file-name=libc.a)
deplibs += $(shell $(CC) $(MARCH_FLAGS) -print-file-name=libm.a)
deplibs += $(shell $(CC) $(MARCH_FLAGS) -print-file-name=libgcc.a)

$(target_name).dis: $(target_name).elf
	$(Q)$(OBJDUMP) -S -d $< > $@

$(target_name).raw: $(target_name).elf
	$(Q)$(OBJC) -O binary $< $@

$(target_name).elf: $(LINKER_SCRIPTS) $(addsuffix /obj-in.o,$(sub_dir)) FORCE
	@printf "\r\n\t--- Linking ---\r\n"
	$(Q)$(LD) -Map=$(basename $@).map -Bstatic --start-group $(deplibs) $(addsuffix /obj-in.o,$(sub_dir)) $(LD_FLAGS) --end-group -o $@
	$(Q)$(NM) $@ | \
		grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | \
		sort >> $(basename $@).map
		$(Q)$(OBJDUMP) -x -d -S $@ >> $(basename $@).map
	$(Q)$(SIZE) $(addsuffix /obj-in.o,$(sub_dir)) $@

$(target_name).bin: $(target_name).raw
ifneq ($(CFG_OPENRISC),)
	@be2le $^ $@
else
	@cp $^ $@
	cp $@ ../../../../brandy/brandy-2.0/spl/board/sun300iw1p1/amp-boot0.fex
endif

$(target_name).hex: $(target_name).bin
	bin2hex -i $^ -o $@ -s 0x4000

objs_size: FORCE
	$(Q)find . -name '*.o' -exec $(SIZE) {} \+

