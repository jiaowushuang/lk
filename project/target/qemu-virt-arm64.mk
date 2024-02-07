# main project for qemu-aarch64
ARCH := arm64
ARM_CPU := cortex-a53
KERNEL_32BIT := false

$(info USER_TASK_32BITS=$(USER_TASK_32BITS))
ifeq (true,$(call TOBOOL,$(USER_TASK_32BITS)))
$(info USER_TASK_BITS=32)
USER_ARCH := arm
USER_32BIT := true
else
$(info USER_TASK_BITS=64)
USER_ARCH := arm64
USER_32BIT := false
endif

include project/target/qemu-virt-arm.mk


