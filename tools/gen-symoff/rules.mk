LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# Default offset stub gen tool
SYM_GEN_TOOL ?= tools/gen-symoff/genassym.sh
OFF_GEN_TOOL ?= tools/gen-symoff/genoffset.sh

include $(LOCAL_DIR)/common-inc.mk

MODULE_DEPS += arch/offset
MODULE_SRCS += \
	$(LOCAL_DIR)/misc.c

$(info SYMOFFSET_BUILD_DIR=$(SYMOFFSET_BUILD_DIR)+$(NM))
SYM_GEN_OBJ  ?= $(SYMOFFSET_BUILD_DIR)/arch/offset/genassym.c.o
OFF_GEN_OBJ  ?= $(SYMOFFSET_BUILD_DIR)/arch/offset/genoffset.c.o
NMFLAGS	:= -S

$(SYMOFFSET_SYM_H): $(SYM_GEN_TOOL) $(SYM_GEN_OBJ)
	@$(MKDIR)
	@echo generating asm symbol $@
	@NM=$(NM) NMFLAGS=$(NMFLAGS) sh $(SYM_GEN_TOOL) -o $@ $(SYM_GEN_OBJ)
		
$(SYMOFFSET_OFF_H): $(OFF_GEN_TOOL) $(OFF_GEN_OBJ)
	@$(MKDIR)
	@echo generating symbol offset $@
	@NM=$(NM) NMFLAGS=$(NMFLAGS) sh $(OFF_GEN_TOOL) -o $@ $(OFF_GEN_OBJ)

# Track them as generated
GENERATED += $(SYMOFFSET_SRCS)

# And path to  generated .h to module includes
GLOBAL_INCLUDES += \
	$(SYMOFFSET_SRCS_DIR)

MODULE_ADD_IMPLICIT_DEPS := false

include make/module.mk