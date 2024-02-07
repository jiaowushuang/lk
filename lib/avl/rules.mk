LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_OPTIONS := extra_warnings

MODULE_SRCS += \
   $(LOCAL_DIR)/avl.c

include make/module.mk
