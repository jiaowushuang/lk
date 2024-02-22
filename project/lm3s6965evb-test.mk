MODULES += \

MODULES += \
	root/services/shell

ifeq (true,$(call TOBOOL,$(ENABLE_MPU)))
MODULES += \
	root/services/partition
endif

include project/virtual/fs.mk
# include project/virtual/test.mk
include project/target/lm3s6965evb.mk
