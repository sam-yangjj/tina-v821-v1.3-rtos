obj-y += Source/list.o
obj-y += Source/queue.o
obj-y += Source/tasks.o
obj-y += Source/timers.o
obj-y += Source/FreeRTOS_tick_config.o
obj-y += Source/event_groups.o

obj-$(CONFIG_HEAP_4) += Source/portable/MemMang/heap_4.o
obj-$(CONFIG_HEAP_MULTIPLE) += Source/portable/MemMang/heap_multiple.o

ifeq ($(CONFIG_ARCH_SUN20IW2P1)_$(CONFIG_ARCH_ARM_CORTEX_M33), y_y)
obj-y += Source/portable/GCC/ARM_CM33_NTZ/non_secure/port.o
obj-y += Source/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.o
obj-y += Source/portable/GCC/ARM_CM33_NTZ/non_secure/port_misc.o
endif

ifeq ($(CONFIG_ARCH_RISCV)_$(CONFIG_ARCH_RISCV_C906), y_y)
obj-y += Source/portable/GCC/RISC-V/port_C906.o
obj-y += Source/portable/GCC/RISC-V/portASM_C906.o
endif

ifeq ($(CONFIG_ARCH_RISCV)_$(CONFIG_ARCH_RISCV_E906), y_y)
obj-y += Source/portable/GCC/RISC-V/port_E90x.o
obj-y += Source/portable/GCC/RISC-V/portASM_E90x.o
endif

ifeq ($(CONFIG_ARCH_RISCV)_$(CONFIG_ARCH_RISCV_E907), y_y)
obj-y += Source/portable/GCC/RISC-V/port_E90x.o
obj-y += Source/portable/GCC/RISC-V/portASM_E90x.o
endif

obj-$(CONFIG_ARCH_RISCV) += Source/portable/GCC/RISC-V/port_misc.o

ifeq ($(CONFIG_ARCH_SUN20IW2P1)_$(CONFIG_ARCH_ARM_CORTEX_M33), y_y)
CFLAGS += -I $(BASE)/arch/arm/armv8m/include/cmsis/
ASFLAGS += -I $(BASE)/arch/arm/armv8m/include/cmsis/
CFLAGS += -I $(BUILD_DIR)/kernel/FreeRTOS-orig/Source/portable/GCC/ARM_CM33_NTZ/non_secure/
ASFLAGS += -I $(BUILD_DIR)/kernel/FreeRTOS-orig/Source/portable/GCC/ARM_CM33_NTZ/non_secure/
endif

ifeq ($(CONFIG_ARCH_RISCV), y)
ASFLAGS += -I $(BASE)/arch/risc-v/fpu/
endif

ifeq ($(CONFIG_ARCH_ARM_CORTEX_A55), y)
obj-y += Source/portable/GCC/ARM_CA53_64_BIT/port.o
obj-y += Source/portable/GCC/ARM_CA53_64_BIT/portASM.o
obj-y += Source/portable/GCC/ARM_CA53_64_BIT/port_misc.o
CFLAGS += -I $(BASE)/kernel/FreeRTOS-orig/Source/portable/GCC/ARM_CA53_64_BIT/
ASFLAGS += -I $(BASE)/kernel/FreeRTOS-orig/Source/portable/GCC/ARM_CA53_64_BIT/
endif

ifeq ($(CONFIG_KASAN), y)
CFLAGS_REMOVE_heap_4.o += $(CFLAGS_KASAN)
endif
