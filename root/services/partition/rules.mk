LOCAL_DIR := $(GET_LOCAL_DIR)

#EXTRA_LINKER_SCRIPTS += $(LOCAL_DIR)/partition.ld

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/partition.c

PARTITION_TASKS_BIN := $(BUILDDIR)/partition_tasks.bin
PARTITION_TASKS_OBJ := $(BUILDDIR)/partition_tasks.o

GENERATED += $(PARTITION_TASKS_BIN) $(PARTITION_TASKS_OBJ)

ALLPARTITION_TASK_OBJS := external/image/RTOSDemo.bin

$(PARTITION_TASKS_BIN): $(ALLPARTITION_TASK_OBJS)
	@$(MKDIR)
	@echo combining tasks into $@: $(ALLPARTITION_TASK_OBJS)
	$(NOECHO)cat $(ALLPARTITION_TASK_OBJS) > $@

$(PARTITION_TASKS_OBJ): $(PARTITION_TASKS_BIN)
	@$(MKDIR)
	@echo generating $@
	$(NOECHO)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.my_image_section $(PARTITION_TASKS_BIN) $@

EXTRA_OBJS += $(PARTITION_TASKS_OBJ)

include make/module.mk