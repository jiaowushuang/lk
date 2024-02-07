LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# can override this in local.mk
ENABLE_THUMB?=true

# default to the regular arm subarch
SUBARCH := arm


GLOBAL_DEFINES += \
	ARM_CPU_$(ARM_CPU)=1

# do set some options based on the cpu core
HANDLED_CORE := false
ifeq ($(ARM_CPU),cortex-m0)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M0=1 \
	ARM_ISA_ARMV6M=1 \
	ARM_WITH_THUMB=1 \
	USE_BUILTIN_ATOMICS=0
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
endif
ifeq ($(ARM_CPU),cortex-m0plus)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M0_PLUS=1 \
	ARM_ISA_ARMV6M=1 \
	ARM_WITH_THUMB=1 \
	USE_BUILTIN_ATOMICS=0
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
ENBALE_MPU := true
endif
ifeq ($(ARM_CPU),cortex-m3)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M3=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
ENBALE_MPU := true
endif
ifeq ($(ARM_CPU),cortex-m4)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M4=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
ENBALE_MPU := true
endif
ifeq ($(ARM_CPU),cortex-m4f)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M4=1 \
	ARM_CPU_CORTEX_M4F=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_VFP_SP_ONLY=1
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
ENBALE_MPU := true
endif
ifeq ($(ARM_CPU),cortex-m55)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M55=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
ENBALE_MPU := true
endif
ifeq ($(ARM_CPU),cortex-m7)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M7=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
ENBALE_MPU := true
endif
ifeq ($(ARM_CPU),cortex-m7-fpu-sp-d16)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M7=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_VFP_SP_ONLY=1
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
ENBALE_MPU := true
endif
ifeq ($(ARM_CPU),cortex-a7)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARCH_HAS_MMU=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7A=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_NEON=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1 \
	ARM_WITH_HYP=1
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),cortex-a15)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARCH_HAS_MMU=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7A=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1 \
	ARM_WITH_L2=1 \
	ARM_GIC_VERSION=2 \
	ARM_WITH_HYP=1

ifneq ($(ARM_WITHOUT_VFP_NEON),true)
#ifneq (true,$(call TOBOOL,$(WITH_HYPER_MODE)))
#GLOBAL_DEFINES += \
	ARM_WITH_VFP=1 \
	ARM_WITH_NEON=1
#endif	
endif
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),cortex-a8)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARCH_HAS_MMU=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7A=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_NEON=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1 \
	ARM_WITH_L2=1
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),cortex-a9)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARCH_HAS_MMU=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7A=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),cortex-a9-neon)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_A9=1 \
	ARM_WITH_CP15=1 \
	ARCH_HAS_MMU=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7A=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_NEON=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),arm1136j-s)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARCH_HAS_MMU=1 \
	ARM_ISA_ARMV6=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_CACHE=1 \
	ARM_CPU_ARM1136=1
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),arm1176jzf-s)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARCH_HAS_MMU=1 \
	ARM_ISA_ARMV6=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_CACHE=1 \
	ARM_CPU_ARM1136=1
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),cortex-r4f)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_R4F=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7R=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_THUMB=1 \
	ARM_CPU_CORTEX_R=1
ENABLE_THUMB := true
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),cortex-r52)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_R52=1 \
	ARM_ISA_ARMV8=1 \
	ARM_ISA_ARMV8R=1 \
	ARM_GIC_VERSION=2 \
	ARM_CPU_CORTEX_R=1 \
	ARM_FSR_LONG_FORMAT=1\
	ARM_WITH_HYP=1
# ARM_WITH_VFP=1
HANDLED_CORE := true
ENBALE_MPU := true
endif
ifeq ($(ARM_CPU),armemu)
# flavor of emulated cpu by the armemu project
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_ISA_ARMV7=1 \
	ARM_ISA_ARMV7A=1 \
	ARM_WITH_CACHE=1
HANDLED_CORE := true
ENABLE_THUMB := false # armemu doesn't currently support thumb properly
endif

ifneq ($(HANDLED_CORE),true)
$(error $(LOCAL_DIR)/rules.mk doesnt have logic for arm core $(ARM_CPU))
endif

THUMBCFLAGS :=
THUMBINTERWORK :=
ifeq ($(ENABLE_THUMB),true)
THUMBCFLAGS := -mthumb -D__thumb__
ifneq ($(SUBARCH),arm-m)
# Only enable thumb interworking switch if we're compiling in a mixed
# arm/thumb environment. Also possible this switch is not needed anymore.
THUMBINTERWORK := -mthumb-interwork
endif
endif

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/$(SUBARCH)/include

ifeq ($(SUBARCH),arm)
MODULE_SRCS += \
	$(LOCAL_DIR)/arm/asm.S \
	$(LOCAL_DIR)/arm/cache-ops.S \
	$(LOCAL_DIR)/arm/cache.c \
	$(LOCAL_DIR)/arm/debug.c \
	$(LOCAL_DIR)/arm/ops.S \
	$(LOCAL_DIR)/arm/faults.c \
	$(LOCAL_DIR)/arm/fpu.c \
	$(LOCAL_DIR)/arm/thread.c

ifeq (true,$(call TOBOOL,$(WITH_HYPER_MODE)))
MODULE_SRCS += \
	$(LOCAL_DIR)/virt/head.S \
	$(LOCAL_DIR)/virt/exceptions.S \
	$(LOCAL_DIR)/virt/vcpu.c

ifeq (false,$(call TOBOOL,$(ENBALE_MPU)))
MODULE_SRCS += \
	$(LOCAL_DIR)/virt/mmu.c \
	$(LOCAL_DIR)/virt/usercopy.c
endif

else
MODULE_SRCS += \
	$(LOCAL_DIR)/arm/start.S \
	$(LOCAL_DIR)/arm/exceptions.S

ifeq (false,$(call TOBOOL,$(ENBALE_MPU)))
MODULE_SRCS += \	
	$(LOCAL_DIR)/arm/mmu.c \
	$(LOCAL_DIR)/arm/usercopy.S
endif
endif

MODULE_ARM_OVERRIDE_SRCS := \
	$(LOCAL_DIR)/arm/arch.c

GLOBAL_DEFINES += \
	ARCH_DEFAULT_STACK_SIZE=4096

ARCH_OPTFLAGS := -O2
WITH_LINKER_GC ?= 1

# use the numeric registers when disassembling code
ARCH_OBJDUMP_FLAGS := -Mreg-names-raw

# we have a mmu and want the vmm/pmm
WITH_KERNEL_VM ?= 1


ifeq (true,$(call TOBOOL,$(WITH_HYPER_MODE)))
MODULE_DEPS := arch/common/arm/virt
# for arm with virt, kernal occupy the entire 4GB of virtual space
# but put the kernel itself at 0x80000000.
# this leaves 0x00001000 - 0x80000000 open for kernel space to use.
# user occupy the entire 4GB of virtual space of ipa 32bits, but max 40bits
GLOBAL_DEFINES += \
    KERNEL_ASPACE_BASE=0x00001000 \
    KERNEL_ASPACE_SIZE=0xfffff000 \
    USER_ASPACE_BASE=0x00000000 \
    USER_ASPACE_SIZE=0xffffffff \
    WITH_HYPER_MODE=1

ifeq (true,$(call TOBOOL,$(ENBALE_MPU)))
KERNEL_BASE ?= $(MEMBASE)
KERNEL_MEMSIZE := 0x04000000
# guest configuration
# TODO: according to kernel setting(DTS/ITS)
GLOBAL_DEFINES += \
    GUEST_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET) \
    GUEST_MEMBASE=0x44000000 \
    GUEST_MEMSIZE=0x04000000 \
    GUEST_DEVBASE=0x0 \
    GUEST_DEVSIZE=0x40000000
else    
# Stage2 IPA range : 0~0xffffffff
# Stage1 VA PL2 mode range : 0~0xffffffff
KERNEL_BASE ?= 0x80000000
KERNEL_MEMSIZE := 0x04000000

# guest configuration
# TODO: according to kernel setting(DTS/ITS)
GLOBAL_DEFINES += \
    GUEST_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET) \
    GUEST_MEMBASE=$(MEMBASE) \
    GUEST_MEMSIZE=0x04000000 \
    GUEST_DEVBASE=0x0 \
    GUEST_DEVSIZE=0x40000000 \
    GUEST_VMEMBASE=$(KERNEL_BASE) \
    GUEST_VDEVBASE=0xc0000000
endif

else
# for arm, have the kernel occupy the entire top 3GB of virtual space,
# but put the kernel itself at 0x80000000.
# this leaves 0x40000000 - 0x80000000 open for kernel space to use.
ifeq (true,$(call TOBOOL,$(ENBALE_MPU)))
KERNEL_BASE ?= $(MEMBASE)
KERNEL_MEMSIZE := 0x04000000
else
GLOBAL_DEFINES += \
    KERNEL_ASPACE_BASE=0x40000000 \
    KERNEL_ASPACE_SIZE=0xc0000000 \
    USER_ASPACE_BASE=0x00001000 \
    USER_ASPACE_SIZE=0x3fffe000

KERNEL_BASE ?= 0x80000000
KERNEL_MEMSIZE := 0x04000000 # guest sram size
endif
endif

KERNEL_LOAD_OFFSET ?= 0

GLOBAL_DEFINES += \
    KERNEL_BASE=$(KERNEL_BASE) \
    KERNEL_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET) \
    KERNEL_MEMSIZE=$(KERNEL_MEMSIZE)

# if its requested we build with SMP, arm generically supports 4 cpus
ifeq ($(WITH_SMP),1)
SMP_MAX_CPUS ?= 4
SMP_CPU_CLUSTER_SHIFT ?= 8
SMP_CPU_ID_BITS ?= 24

GLOBAL_DEFINES += \
    WITH_SMP=1 \
    SMP_MAX_CPUS=$(SMP_MAX_CPUS) \
    SMP_CPU_CLUSTER_SHIFT=$(SMP_CPU_CLUSTER_SHIFT) \
    SMP_CPU_ID_BITS=$(SMP_CPU_ID_BITS) \

MODULE_SRCS += \
	$(LOCAL_DIR)/arm/mp.c
else
GLOBAL_DEFINES += \
    SMP_MAX_CPUS=1
endif

ifeq (true,$(call TOBOOL,$(WITH_NS_MAPPING)))
GLOBAL_DEFINES += \
    WITH_ARCH_MMU_PICK_SPOT=1
endif

endif
ifeq ($(SUBARCH),arm-m)
MODULE_SRCS += \
	$(LOCAL_DIR)/arm-m/arch.c \
	$(LOCAL_DIR)/arm-m/cache.c \
	$(LOCAL_DIR)/arm-m/exceptions.c \
	$(LOCAL_DIR)/arm-m/start.c \
	$(LOCAL_DIR)/arm-m/spin_cycles.c \
	$(LOCAL_DIR)/arm-m/thread.c \
	$(LOCAL_DIR)/arm-m/vectab.c

# we're building for small binaries
GLOBAL_DEFINES += \
	ARM_ONLY_THUMB=1 \
	ARCH_DEFAULT_STACK_SIZE=1024 \
	SMP_MAX_CPUS=1

MODULE_DEPS += \
	arch/arm/arm-m/CMSIS

ARCH_OPTFLAGS := -Os
WITH_LINKER_GC ?= 1
endif

ifeq (true,$(call TOBOOL,$(ENBALE_MPU)))
WITH_KERNEL_VM := 0

MODULE_SRCS += \
	$(LOCAL_DIR)/mach/mpu/arm_core_mpu.c

ifeq (true,$(call TOBOOL,$(NXP_MPU_ENABLE)))
MODULE_SRCS += 	$(LOCAL_DIR)/mach/mpu/nxp_mpu.c
GLOBAL_DEFINES += \
	ARCH_NXP_HAS_MPU=1
else
MODULE_SRCS += 	$(LOCAL_DIR)/mach/mpu/arm_mpu.c
GLOBAL_DEFINES += \
	ARCH_ARM_HAS_MPU=1
endif

GLOBAL_DEFINES += \
	ARCH_HAS_MPU=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/mach/include \
	$(LOCAL_DIR)/mach/mpu
ifeq ($(SUBARCH),arm-m)
GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/mach/mpu/cortex_m
else
GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/mach/mpu/cortex_a_r
endif

ENBALE_MPU_NULLPTRDEBUG := false
ifeq (true,$(call TOBOOL,$(ENBALE_MPU_NULLPTRDEBUG)))
GLOBAL_INCLUDES += \
	ARM_NULL_POINTER_EXCEPTION_DETECTION_MPU=1 \
	ARM_CORTEX_M_NULL_POINTER_EXCEPTION_PAGE_SIZE=1024
endif

ifeq (true,$(call TOBOOL,$(WITH_AUX_HYPER_MODE)))
KERNEL_MEMSIZE := $(MEMSIZE)
KERNEL_ROMSIZE := 0x20000
GLOBAL_DEFINES += \
	WITH_AUX_HYPER_MODE=1
endif

ifeq (true,$(call TOBOOL,$(WITH_HYPER_MODE)))
GLOBAL_DEFINES += \
	WITH_HYPER_MODE=1
endif
endif


# try to find toolchain
include $(LOCAL_DIR)/toolchain.mk
TOOLCHAIN_PREFIX := $(ARCH_$(ARCH)_TOOLCHAIN_PREFIX)
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))

ARCH_COMPILEFLAGS += $(ARCH_$(ARCH)_COMPILEFLAGS)

GLOBAL_COMPILEFLAGS += $(THUMBINTERWORK)

# set the max page size to something more reasonable (defaults to 64K or above)
ARCH_LDFLAGS += -z max-page-size=4096

# find the direct path to libgcc.a for our particular multilib variant
LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) $(THUMBCFLAGS) -print-libgcc-file-name)
#$(info LIBGCC = $(LIBGCC))
#$(info LIBGCC COMPILEFLAGS = $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) $(THUMBCFLAGS))

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

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE) \
	ROMBASE=$(ROMBASE) \
	ROMSIZE=$(ROMSIZE)

ifeq ($(SUBARCH),arm)
include tools/gen-symoff/common-inc.mk
MODULE_DEPS += tools/gen-symoff
MODULE_SRCDEPS += $(SYMOFFSET_SYM_H) $(SYMOFFSET_OFF_H)
endif

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/system-onesegment.ld \
	$(BUILDDIR)/system-twosegment.ld

# rules for generating the linker scripts
$(BUILDDIR)/system-onesegment.ld: $(LOCAL_DIR)/system-onesegment.ld $(wildcard arch/*.ld) linkerscript.phony
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%KERNEL_MEMSIZE%/$(KERNEL_MEMSIZE)/;s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

$(BUILDDIR)/system-twosegment.ld: $(LOCAL_DIR)/system-twosegment.ld $(wildcard arch/*.ld) linkerscript.phony
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%KERNEL_ROMSIZE%/$(KERNEL_ROMSIZE)/;s/%KERNEL_MEMSIZE%/$(KERNEL_MEMSIZE)/;s/%ROMBASE%/$(ROMBASE)/;s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

linkerscript.phony:
.PHONY: linkerscript.phony

# arm specific script to try to guess stack usage
$(OUTELF).stack: LOCAL_DIR:=$(LOCAL_DIR)
$(OUTELF).stack: $(OUTELF)
	$(NOECHO)echo generating stack usage $@
	$(NOECHO)$(OBJDUMP) $(ARCH_OBJDUMP_FLAGS) -d $< | $(LOCAL_DIR)/stackusage | $(CPPFILT) | sort -n -k 1 -r > $@

EXTRA_BUILDDEPS += $(OUTELF).stack
GENERATED += $(OUTELF).stack

include make/module.mk
