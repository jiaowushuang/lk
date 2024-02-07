LOCAL_DIR := $(GET_LOCAL_DIR)

#EXTRA_LINKER_SCRIPTS += $(LOCAL_DIR)/guest.ld

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/guest.c

GUEST_TASKS_BIN := $(BUILDDIR)/guest_tasks.bin
GUEST_TASKS_OBJ := $(BUILDDIR)/guest_tasks.o

GENERATED += $(GUEST_TASKS_BIN) $(GUEST_TASKS_OBJ)

ifeq (true,$(call TOBOOL,$(ENABLE_MPU)))
ALLGUEST_TASK_OBJS := external/image/kern.bin
else
ALLGUEST_TASK_OBJS := external/image/kern-a.bin
endif

$(GUEST_TASKS_BIN): $(ALLGUEST_TASK_OBJS)
	@$(MKDIR)
	@echo combining tasks into $@: $(ALLGUEST_TASK_OBJS)
	$(NOECHO)cat $(ALLGUEST_TASK_OBJS) > $@

$(GUEST_TASKS_OBJ): $(GUEST_TASKS_BIN)
	@$(MKDIR)
	@echo generating $@
	$(NOECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.my_image_section $(GUEST_TASKS_BIN) $@

EXTRA_OBJS += $(GUEST_TASKS_OBJ)

include make/module.mk