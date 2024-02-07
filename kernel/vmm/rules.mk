LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/heap.c \
	$(LOCAL_DIR)/ipa.c \
	$(LOCAL_DIR)/pa_init.c \
	$(LOCAL_DIR)/pa_util.c \
	$(LOCAL_DIR)/pa.c \
	$(LOCAL_DIR)/percpu.c \

MODULE_DEPS := \
	lib/avl

MODULE_OPTIONS := extra_warnings

include make/module.mk
