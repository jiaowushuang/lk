LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# VIRTIO GUEST DRIVERS

MODULE_SRCS += \
	$(LOCAL_DIR)/virtio.c

include make/module.mk
