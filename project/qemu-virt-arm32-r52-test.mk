# main project for qemu-arm32
MODULES += \
	root/services/shell

ifeq (true,$(call TOBOOL,$(WITH_AUX_HYPER_MODE)))
MODULES += \
	root/services/partition
endif

ifeq (true,$(call TOBOOL,$(WITH_HYPER_MODE)))
MODULES += \
	root/services/guest

endif

GLOBAL_DEFINES += \
	WITH_LIB_CONSOLE=1 \

#include project/virtual/test.mk
include project/virtual/fs.mk
#include project/virtual/minip.mk
include project/target/qemu-virt-arm32-r52.mk

