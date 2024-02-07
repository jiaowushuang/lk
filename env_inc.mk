# copy this and makefile to your external root directory and customize
# according to how you want to use a split repository

# the top level directory that all paths are relative to
LKMAKEROOT := .

# paths relative to LKMAKEROOT where additional modules should be searched
LKINC := projectroot

# the path relative to LKMAKEROOT where the main kern repository lives
LKROOT := .

# set the directory relative to LKMAKEROOT where output will go
BUILDROOT ?= .

# set the default project if no args are passed
DEFAULT_PROJECT ?= 

# TOOLCHAIN_PREFIX := ???

USER_TASK_ENABLE := false
USER_TASK_32BITS := false

# TODO:
WITH_ADMIN_MODE :=

# default mode
WITH_SUPER_MODE := 
WITH_AUX_HYPER_MODE :=
WITH_HYPER_MODE := true

# TODO:
WITH_MONITOR_MODE :=

$(info USER_TASK_ENABLE=$(USER_TASK_ENABLE))
$(info USER_TASK_32BITS=$(USER_TASK_32BITS))

ifdef USER_TASK_ENABLE
LKINC := $(LKROOT)/user \
	 $(LKROOT)/samples

endif