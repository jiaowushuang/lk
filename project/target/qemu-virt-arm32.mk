# main project for qemu-arm32
ARCH := arm
ARM_CPU := cortex-a15
KERNEL_32BIT := true
USER_ARCH := arm
USER_32BIT := true

include project/target/qemu-virt-arm.mk

