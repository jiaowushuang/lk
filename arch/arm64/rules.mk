LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
ENBALE_MPU :=

GLOBAL_DEFINES += \
	ARM64_CPU_$(ARM_CPU)=1 \
	ARM_ISA_ARMV8=1 \
	IS_64BIT=1

MODULE_SRCS += \
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/asm.S \
	$(LOCAL_DIR)/exceptions.S \
	$(LOCAL_DIR)/exceptions_c.c \
	$(LOCAL_DIR)/fpu.c \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/cpu_data_array.c \
	$(LOCAL_DIR)/spinlock.S \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/cache-ops.S \
	$(LOCAL_DIR)/pl011_console.S \
	$(LOCAL_DIR)/asm_debug.S \
	$(LOCAL_DIR)/crash.S \
	$(LOCAL_DIR)/cpu-ops.S \
	$(LOCAL_DIR)/usercopy.S

#	$(LOCAL_DIR)/arm/start.S \
	$(LOCAL_DIR)/arm/cache.c \
	$(LOCAL_DIR)/arm/ops.S \
	$(LOCAL_DIR)/arm/faults.c \
	$(LOCAL_DIR)/arm/dcc.S

GLOBAL_DEFINES += \
	ARCH_DEFAULT_STACK_SIZE=4096 \
	CTX_INCLUDE_FPREGS=1 \
	__KERNEL_64__=1 \
	CRASH_REPORTING=1\
	ENABLE_ASSERTIONS=1
	
# if its requested we build with SMP, arm generically supports 4 cpus
ifeq ($(WITH_SMP),1)
SMP_MAX_CPUS ?= 4
SMP_CPU_CLUSTER_SHIFT ?= 8
SMP_CPU_ID_BITS ?= 24 # Ignore aff3 bits for now since they are not next to aff2

GLOBAL_DEFINES += \
    WITH_SMP=1 \
    SMP_MAX_CPUS=$(SMP_MAX_CPUS) \
    SMP_CPU_CLUSTER_SHIFT=$(SMP_CPU_CLUSTER_SHIFT) \
    SMP_CPU_ID_BITS=$(SMP_CPU_ID_BITS)

MODULE_SRCS += \
    $(LOCAL_DIR)/mp.c
else
GLOBAL_DEFINES += \
    SMP_MAX_CPUS=1
endif

#[D]
ARCH_OPTFLAGS := -O0

# we have a mmu and want the vmm/pmm
WITH_KERNEL_VM ?= 1

ifeq ($(WITH_KERNEL_VM),1)

MODULE_SRCS += \
	$(LOCAL_DIR)/mmu.c

KERNEL_ASPACE_BASE ?= 0xffff000000000000
KERNEL_ASPACE_SIZE ?= 0x0001000000000000
ifeq (true,$(call TOBOOL,$(USER_32BIT)))
USER_ASPACE_BASE ?= 0x00001000 
USER_ASPACE_SIZE ?= 0x3fffe000
else
USER_ASPACE_BASE   ?= 0x0000000001000000
USER_ASPACE_SIZE   ?= 0x0000fffffe000000
endif

GLOBAL_DEFINES += \
    KERNEL_ASPACE_BASE=$(KERNEL_ASPACE_BASE) \
    KERNEL_ASPACE_SIZE=$(KERNEL_ASPACE_SIZE) \
    USER_ASPACE_BASE=$(USER_ASPACE_BASE) \
    USER_ASPACE_SIZE=$(USER_ASPACE_SIZE) \
    ARCH_HAS_MMU=1

KERNEL_BASE ?= $(KERNEL_ASPACE_BASE)
KERNEL_LOAD_OFFSET ?= 0

GLOBAL_DEFINES += \
    KERNEL_BASE=$(KERNEL_BASE) \
    KERNEL_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET)

else

KERNEL_BASE ?= $(MEMBASE)
KERNEL_LOAD_OFFSET ?= 0

endif

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE)

# try to find the toolchain
include $(LOCAL_DIR)/toolchain.mk
TOOLCHAIN_PREFIX := $(ARCH_$(ARCH)_TOOLCHAIN_PREFIX)
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))

ARCH_COMPILEFLAGS += $(ARCH_$(ARCH)_COMPILEFLAGS)
ARCH_COMPILEFLAGS += -fno-omit-frame-pointer
ARCH_COMPILEFLAGS_NOFLOAT := # -mgeneral-regs-only
ARCH_COMPILEFLAGS_FLOAT :=

ARCH_LDFLAGS += -z max-page-size=4096

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) -print-libgcc-file-name)

# make sure some bits were set up
MEMVARS_SET := 0
ifneq ($(MEMBASE),)
MEMVARS_SET := 1
endif
ifneq ($(MEMSIZE),)
MEMVARS_SET := 1
endif
ifeq ($(MEMVARS_SET),0)
$(error missing MEMBASE or MEMSIZE variable, please set in target rules.mk)
endif

include tools/gen-symoff/common-inc.mk

MODULE_DEPS += tools/gen-symoff

MODULE_SRCDEPS += $(SYMOFFSET_SYM_H) $(SYMOFFSET_OFF_H)

ifeq (true,$(call TOBOOL,$(ENBALE_MPU)))

MODULE_SRCS += \
	$(LOCAL_DIR)/mach/mpu/arm_mpu.c \
	$(LOCAL_DIR)/mach/mpu/arm_core_mpu.c

GLOBAL_DEFINES += \
	ARCH_HAS_MPU=1 \
	ARCH_ARM_HAS_MPU=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/mach/include

endif

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/system-onesegment.ld

# rules for generating the linker script
$(BUILDDIR)/system-onesegment.ld: $(LOCAL_DIR)/system-onesegment.ld $(wildcard arch/*.ld) linkerscript.phony
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

linkerscript.phony:
.PHONY: linkerscript.phony

include make/module.mk
