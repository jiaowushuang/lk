LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += 	$(LOCAL_DIR)/genassym.c \
		$(LOCAL_DIR)/genoffset.c


# include make/module.mk
