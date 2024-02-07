LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ifeq ($(ARCH),)
ARCH := arm64
endif
ifeq ($(ARCH),arm64)
ARM_CPU ?= cortex-a53
endif
ifeq ($(ARCH),arm)
ARM_CPU ?= cortex-a15
endif
WITH_SMP ?= 1

$(info ARM_CPU = $(ARM_CPU))

HEAP_IMPLEMENTATION := dlmalloc

#HEAP_IMPLEMENTATION := miniheap

MODULE_SRCS += \
    $(LOCAL_DIR)/debug.c \
    $(LOCAL_DIR)/platform.c \
    $(LOCAL_DIR)/uart.c \
    $(LOCAL_DIR)/mpu_regions.c
ifeq ($(ARCH),arm64)    
    MODULE_SRCS += $(LOCAL_DIR)/platform_helpers.S
endif

# r52 MEMBASE/MEMSIZE for hypervisor
# and others 
MEMBASE ?= 0x40000000
MEMSIZE ?= 0x08000000   # 128MB
# r52 MEMBASE/MEMSIZE for guest
ifeq (true,$(call TOBOOL,$(WITH_SUPER_MODE)))
ifeq ($(ARM_CPU),cortex-r52)
MEMBASE := 0x44000000
MEMSIZE := 0x04000000   # 64MB
endif
endif

KERNEL_LOAD_OFFSET := 0x100000 # 1MB

MODULE_DEPS += \
    dev/interrupt/arm_gic \
    dev/power/psci \
    dev/timer/arm_generic \
    lib/cbuf \
    dev/virtio/block \

#[D]dev/bus/pci \
    dev/bus/pci/drivers \
    dev/virtio/gpu \
    dev/virtio/net \
    lib/fdtwalk


GLOBAL_DEFINES += \
    MEMBASE=$(MEMBASE) \
    MEMSIZE=$(MEMSIZE) \
    PLATFORM_SUPPORTS_PANIC_SHELL=1 \
    CONSOLE_HAS_INPUT_BUFFER=1

GLOBAL_DEFINES += MMU_WITH_TRAMPOLINE=1 \
	WITH_DEV_VIRTIO_BLOCK=1

LINKER_SCRIPT += \
    $(BUILDDIR)/system-onesegment.ld

include make/module.mk
