# main project for qemu-aarch64
MODULES += root/services/shell

GLOBAL_DEFINES += \
	WITH_LIB_CONSOLE=1 \

# include project/virtual/test.mk
include project/virtual/fs.mk
#include project/virtual/minip.mk
include project/target/qemu-virt-arm64.mk

