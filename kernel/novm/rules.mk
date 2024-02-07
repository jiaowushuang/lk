LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/novm.c \
	$(LOCAL_DIR)/mem_domain.c

MODULE_OPTIONS := extra_warnings

include make/module.mk
