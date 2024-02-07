# main project for qemu-virt-arm
TARGET := qemu-virt-arm

ifeq (true,$(call TOBOOL,$(USER_TASK_ENABLE)))

LOCAL_DIR := $(GET_LOCAL_DIR)

ifeq (false,$(call TOBOOL,$(KERNEL_32BIT)))

# Arm64 address space configuration
#KERNEL_ASPACE_BASE := 0xffffffffe0000000
#KERNEL_ASPACE_SIZE := 0x0000000020000000
#KERNEL_BASE        := 0xffffffffe0000000

ifeq (true,$(call TOBOOL,$(USER_32BIT)))
#GLOBAL_DEFINES 	   += MMU_USER_SIZE_SHIFT=25 # 32 MB user-space address space
endif

else
#KERNEL_BASE        := 0xe0000000
endif

# select timer
ifeq (true,$(call TOBOOL,$(KERNEL_32BIT)))
# 32 bit Secure EL1 with a 64 bit EL3 gets the non-secure physical timer
# GLOBAL_DEFINES += TIMER_ARM_GENERIC_SELECTED=CNTP
else
# GLOBAL_DEFINES += TIMER_ARM_GENERIC_SELECTED=CNTPS
endif

#
# GLOBAL definitions
#

# requires linker GC
WITH_LINKER_GC := 1

# Need support for Non-secure memory mapping
WITH_NS_MAPPING := true

# do not relocate kernel in physical memory
GLOBAL_DEFINES += WITH_NO_PHYS_RELOCATION=1

# limit heap grows
GLOBAL_DEFINES += HEAP_GROW_SIZE=8192

# limit physical memory to 38 bit to prevert tt_trampiline from getting larger than arm64_kernel_translation_table
GLOBAL_DEFINES += MMU_IDENT_SIZE_SHIFT=38

#
# Modules to be compiled into kern.bin
#
MODULES += root/kernel

#
# user tasks to be compiled into kern.bin
#

# prebuilt
PREBUILT_USER_TASKS :=

# compiled from source
ALL_USER_TASKS := \
	samples/skel \
	samples/skel2 \
	samples/timer \

# This project requires trusty IPC
WITH_IPC := true

EXTRA_BUILDRULES += make/app/user-tasks.mk

GLOBAL_DEFINES += \
    WITH_LIB_UTHREAD=1 \
    WITH_LIB_SYSCALL=1 \

endif

ifeq (true,$(call TOBOOL,$(KERNEL_32BIT)))
GLOBAL_DEFINES 	   += KERNEL_32BIT=1
endif

ifeq (true,$(call TOBOOL,$(USER_32BIT)))
GLOBAL_DEFINES 	   += USER_32BIT=1
endif

