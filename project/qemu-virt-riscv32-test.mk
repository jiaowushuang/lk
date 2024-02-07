# main project for qemu-riscv32
MODULES += \
	root/services/shell
SUBARCH := 32

include project/virtual/test.mk
include project/virtual/fs.mk
include project/virtual/minip.mk
include project/target/qemu-virt-riscv.mk

