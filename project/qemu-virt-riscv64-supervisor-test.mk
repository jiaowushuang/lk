# main project for qemu-riscv64-supervisor
MODULES += \
	root/services/shell
SUBARCH := 64
RISCV_MODE := supervisor

include project/virtual/test.mk
include project/virtual/fs.mk
include project/virtual/minip.mk
include project/target/qemu-virt-riscv.mk

