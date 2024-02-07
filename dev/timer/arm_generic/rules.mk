LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += \
	PLATFORM_HAS_DYNAMIC_TIMER=1

MODULE_SRCS += \
	$(LOCAL_DIR)/arm_generic_timer.c

ifeq (true,$(call TOBOOL,$(WITH_HYPER_MODE)))
MODULE_SRCS += \
	$(LOCAL_DIR)/arm_generic_vtimer.c
endif

MODULE_DEPS += \
	lib/fixed_point

include make/module.mk
