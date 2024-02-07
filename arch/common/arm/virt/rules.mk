LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ifeq (true,$(call TOBOOL,$(WITH_HYPER_MODE)))
MODULE_SRCS += \
	$(LOCAL_DIR)/vtraps.c \
	$(LOCAL_DIR)/vcpreg.c
GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/../include
endif

include make/module.mk