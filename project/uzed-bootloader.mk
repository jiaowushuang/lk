# top level project rules for the uzed-bootloader project
#
MODULES += \
	root/services/shell \
	root/services/zynq-common \
	root/services/lkboot

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

# disable SMP for this build, chain loading from SMP currently does not work
WITH_SMP:=0

include project/target/uzed.mk

