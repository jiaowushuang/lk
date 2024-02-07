LOCAL_MAKEFILE:=$(MAKEFILE_LIST)

# macros used all over the build system
include make/macros.mk

BUILDROOT ?= .

# Check the system configuration file .config
ifeq ($(wildcard .config),)
.PHONY : default
default :
	@echo "Missing .config file"
	@echo "--Please do project configuration"
else
include .config
endif

# 'make spotless' is a special rule that skips most of the rest of the build system and
# simply deletes everything in build-*
ifeq ($(MAKECMDGOALS),spotless)
spotless:
	@rm -rf -- "$(BUILDROOT)"/build-*
	@rm -rf .config*
else

ifndef LKROOT
$(error please define LKROOT to the root of the kern build system)
endif

# any local environment overrides can optionally be placed in local.mk
-include local.mk

# If one of our goals (from the commandline) happens to have a
# matching project/goal.mk, then we should re-invoke make with
# that project name specified...

project-name := $(firstword $(MAKECMDGOALS))

ifneq ($(project-name),)
ifneq ($(strip $(foreach d,$(LKINC),$(wildcard $(d)/project/$(project-name).mk))),)
do-nothing := 1
$(MAKECMDGOALS) _all: make-make
	@:
make-make:
	@PROJECT=$(project-name) $(MAKE) -rR -f $(LOCAL_MAKEFILE) $(filter-out $(project-name), $(MAKECMDGOALS))

.PHONY: make-make
endif # expansion of project-name
endif # project-name != null

# some additional rules to print some help
include make/help.mk

ifeq ($(do-nothing),)

ifeq ($(PROJECT),)

ifneq ($(DEFAULT_PROJECT),)
PROJECT := $(DEFAULT_PROJECT)
else
$(error No project specified. Use 'make list' for a list of projects or 'make help' for additional help)
endif # DEFAULT_PROJECT == something

endif # PROJECT == null

DEBUG ?= 2

BUILDDIR_SUFFIX ?=
BUILDDIR := $(BUILDROOT)/build-$(PROJECT)$(BUILDDIR_SUFFIX)
OUTBIN := $(BUILDDIR)/kern.bin
OUTELF := $(BUILDDIR)/kern.elf
CONFIGHEADER := $(BUILDDIR)/config.h

$(info LKINC=$(LKINC))
GLOBAL_INCLUDES := $(BUILDDIR) $(addsuffix /include,$(LKINC))
GLOBAL_OPTFLAGS ?= $(ARCH_OPTFLAGS)
GLOBAL_COMPILEFLAGS := -g -include $(CONFIGHEADER)
GLOBAL_COMPILEFLAGS += -Wextra -Wall -Werror=return-type -Wshadow -Wdouble-promotion
GLOBAL_COMPILEFLAGS += -Wno-multichar -Wno-unused-parameter -Wno-unused-function -Wno-unused-label
GLOBAL_COMPILEFLAGS += -fno-common
# Build with -ffreestanding since we are building an OS kernel and cannot
# rely on all hosted environment functionality being present.
GLOBAL_COMPILEFLAGS += -ffreestanding
GLOBAL_CFLAGS := --std=gnu11 -Werror-implicit-function-declaration -Wstrict-prototypes -Wwrite-strings
GLOBAL_CPPFLAGS := --std=c++14 -fno-exceptions -fno-rtti -fno-threadsafe-statics
GLOBAL_ASMFLAGS := -DASSEMBLY
GLOBAL_LDFLAGS :=

ifeq ($(UBSAN), 1)
# Inject lib/ubsan directly into MODULE_DEPS
# lib/ubsan will itself add the needed CFLAGS
MODULE_DEPS += lib/ubsan
endif

# flags that are sometimes nice to enable to catch problems but too strict to have on all the time.
# add to global flags from time to time to find things, otherwise only available with a module
# option (see make/module.mk re: MODULE_OPTIONS).
EXTRA_MODULE_COMPILEFLAGS := -Wmissing-declarations
EXTRA_MODULE_CFLAGS := -Wmissing-prototypes
EXTRA_MODULE_CPPFLAGS :=
EXTRA_MODULE_ASMFLAGS :=

#GLOBAL_COMPILEFLAGS += -Wpacked
#GLOBAL_COMPILEFLAGS += -Wpadded
#GLOBAL_COMPILEFLAGS += -Winline

# if WERROR is set, add to the compile args
ifeq (true,$(call TOBOOL,$(WERROR)))
GLOBAL_COMPILEFLAGS += -Werror
endif

GLOBAL_LDFLAGS += $(addprefix -L,$(LKINC))

# Architecture specific compile flags
ARCH_COMPILEFLAGS :=
ARCH_COMPILEFLAGS_NOFLOAT := # flags used when compiling with floating point support
ARCH_COMPILEFLAGS_FLOAT := # flags for when not compiling with floating point support
ARCH_CFLAGS :=
ARCH_CPPFLAGS :=
ARCH_ASMFLAGS :=
ARCH_LDFLAGS :=
ARCH_OBJDUMP_FLAGS :=
THUMBCFLAGS := # optional compile switches set by arm architecture when compiling in thumb mode

# top level rule
all:: $(OUTBIN) $(OUTELF).lst $(OUTELF).debug.lst $(OUTELF).sym $(OUTELF).sym.sorted $(OUTELF).size $(OUTELF).dump $(BUILDDIR)/srcfiles.txt $(BUILDDIR)/include_paths.txt

# master module object list
ALLMODULE_OBJS :=

# master object list (for dep generation)
ALLOBJS :=

# master source file list
ALLSRCS :=

# a linker script needs to be declared in one of the project/target/platform files
LINKER_SCRIPT :=

# anything you add here will be deleted in make clean
GENERATED := $(CONFIGHEADER)

# anything added to GLOBAL_DEFINES will be put into $(BUILDDIR)/config.h
GLOBAL_DEFINES := OLDK=1

# Anything added to GLOBAL_SRCDEPS will become a dependency of every source file in the system.
# Useful for header files that may be included by one or more source files.
GLOBAL_SRCDEPS := $(CONFIGHEADER)

# these need to be filled out by the project/target/platform rules.mk files
TARGET :=
PLATFORM :=
ARCH :=
ALLMODULES :=

# add any external module dependencies
MODULES := $(EXTERNAL_MODULES)

# any .mk specified here will be included before build.mk
EXTRA_BUILDRULES :=

# any rules you put here will also be built by the system before considered being complete
EXTRA_BUILDDEPS :=

# any rules you put here will be depended on in clean builds
EXTRA_CLEANDEPS :=

# any objects you put here get linked with the final image
EXTRA_OBJS :=

# any extra linker scripts to be put on the command line
EXTRA_LINKER_SCRIPTS :=

# if someone defines this, the build id will be pulled into lib/version
BUILDID ?=

# comment out or override if you want to see the full output of each command
NOECHO ?= @

# try to include the project file
-include project/$(PROJECT).mk
ifndef TARGET
$(error couldn't find project or project doesn't define target)
endif
include target/$(TARGET)/rules.mk
ifndef PLATFORM
$(error couldn't find target or target doesn't define platform)
endif
include platform/$(PLATFORM)/rules.mk

# include arch/offset/rules.mk

ifndef ARCH
$(error couldn't find arch or platform doesn't define arch)
endif
include arch/$(ARCH)/rules.mk
ifndef TOOLCHAIN_PREFIX
$(error TOOLCHAIN_PREFIX not set in the arch rules.mk)
endif

# default to no ccache
CCACHE ?=
CC ?= $(CCACHE) $(TOOLCHAIN_PREFIX)gcc
LD ?= $(TOOLCHAIN_PREFIX)ld
OBJDUMP ?= $(TOOLCHAIN_PREFIX)objdump
OBJCOPY ?= $(TOOLCHAIN_PREFIX)objcopy
CPPFILT ?= $(TOOLCHAIN_PREFIX)c++filt
SIZE ?= $(TOOLCHAIN_PREFIX)size
NM ?= $(TOOLCHAIN_PREFIX)nm
STRIP ?= $(TOOLCHAIN_PREFIX)strip

# Detect whether we are using ld.lld. If we don't detect ld.lld, we assume
# it's ld.bfd. This check can be refined in the future if we need to handle
# more cases (e.g. ld.gold).
LINKER_TYPE := $(shell $(LD) -v 2>&1 | grep -q "LLD" && echo lld || echo bfd)
$(info LINKER_TYPE=$(LINKER_TYPE))
# Detect whether we are compiling with GCC or Clang
COMPILER_TYPE := $(shell $(CC) -v 2>&1 | grep -q "clang version" && echo clang || echo gcc)
$(info COMPILER_TYPE=$(COMPILER_TYPE))

# Now that CC is defined we can check if warning flags are supported and add
# them to GLOBAL_COMPILEFLAGS if they are.
ifeq ($(call is_warning_flag_supported,-Wnonnull-compare),yes)
GLOBAL_COMPILEFLAGS += -Wno-nonnull-compare
endif
# Ideally we would move this check to arm64/rules.mk, but we can only check
# for supported warning flags once CC is defined.
ifeq ($(ARCH),arm64)
# Clang incorrectly diagnoses msr operations as need a 64-bit operand even if
# the underlying register is actually 32 bits. Silence this common warning.
ifeq ($(call is_warning_flag_supported,-Wasm-operand-widths),yes)
ARCH_COMPILEFLAGS += -Wno-asm-operand-widths
endif
endif

ifeq ($(ARCH),riscv)
# ld.lld does not support linker relaxations yet.
# TODO: This is no longer true as of LLVM 15, so should add a version check
ifeq ($(LINKER_TYPE),lld)
ARCH_COMPILEFLAGS += -mno-relax
# Work around out-of-range undef-weak relocations when building with clang and
# linking with ld.lld. This is not a problem with ld.bfd since ld.bfd rewrites
# the instructions to avoid the out-of-range PC-relative relocation
# See https://github.com/riscv-non-isa/riscv-elf-psabi-doc/issues/126 for more
# details. For now, the simplest workaround is to build with -fpie when using
# a version of clang that does not include https://reviews.llvm.org/D107280.
# TODO: Add a clang 17 version check now that the review has been merged.
ifeq ($(COMPILER_TYPE),clang)
# We also add the -fdirect-access-external-data flag is added to avoid the
# majority of the performance overhead caused by -fPIE.
ARCH_COMPILEFLAGS += -fPIE -fdirect-access-external-data
endif
endif
endif

$(info PROJECT = $(PROJECT))
$(info PLATFORM = $(PLATFORM))
$(info TARGET = $(TARGET))
$(info ARCH = $(ARCH))
$(info USER_ARCH = $(USER_ARCH))
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))
$(info DEBUG = $(DEBUG))

# Derive the standard arch name.
ifneq ($(USER_ARCH),)
$(eval $(call standard_name_for_arch,STANDARD_ARCH_NAME,$(USER_ARCH),$(SUBARCH)))
endif

# include the top level module that includes basic always-there modules
include top/rules.mk

# recursively include any modules in the MODULE variable, leaving a trail of included
# modules in the ALLMODULES list
include make/recurse.mk

# add some automatic configuration defines
GLOBAL_DEFINES += \
	PROJECT_$(PROJECT)=1 \
	PROJECT=\"$(PROJECT)\" \
	TARGET_$(TARGET)=1 \
	TARGET=\"$(TARGET)\" \
	PLATFORM_$(PLATFORM)=1 \
	PLATFORM=\"$(PLATFORM)\" \
	ARCH_$(ARCH)=1 \
	ARCH=\"$(ARCH)\" \
	$(addsuffix =1,$(addprefix WITH_,$(ALLMODULES)))

# debug build?
ifneq ($(DEBUG),)
GLOBAL_DEFINES += \
	SYS_DEBUGLEVEL=$(DEBUG)
endif

# add .config to GLOBAL_DEFINES
CONFIG_FILE := .config
# Read the file line by line and store each line in a variable
CONFIG_OPTIONS := $(shell grep '^[^\#].*=.*' $(CONFIG_FILE) | sed 's/=y/=1/g')

GLOBAL_DEFINES += $(CONFIG_OPTIONS)
$(info GENCONFIG_VAR=$(CONFIG_OPTIONS))

# allow additional defines from outside the build system
ifneq ($(EXTERNAL_DEFINES),)
GLOBAL_DEFINES += $(EXTERNAL_DEFINES)
$(info EXTERNAL_DEFINES = $(EXTERNAL_DEFINES))
endif

# prefix all of the paths in GLOBAL_INCLUDES with -I
GLOBAL_INCLUDES := $(addprefix -I,$(GLOBAL_INCLUDES))

# test for some old variables
ifneq ($(INCLUDES),)
$(error INCLUDES variable set, please move to GLOBAL_INCLUDES: $(INCLUDES))
endif
ifneq ($(DEFINES),)
$(error DEFINES variable set, please move to GLOBAL_DEFINES: $(DEFINES))
endif

# try to have the compiler output colorized error messages if available
export GCC_COLORS ?= 1

# the logic to compile and link stuff is in here
include make/build.mk

DEPS := $(ALLOBJS:%o=%d)

# put all of the global build flags in config.h to force a rebuild if any change
GLOBAL_DEFINES += GLOBAL_INCLUDES=\"$(subst $(SPACE),_,$(GLOBAL_INCLUDES))\"
GLOBAL_DEFINES += GLOBAL_COMPILEFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_COMPILEFLAGS))\"
GLOBAL_DEFINES += GLOBAL_OPTFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_OPTFLAGS))\"
GLOBAL_DEFINES += GLOBAL_CFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_CFLAGS))\"
GLOBAL_DEFINES += GLOBAL_CPPFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_CPPFLAGS))\"
GLOBAL_DEFINES += GLOBAL_ASMFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_ASMFLAGS))\"
GLOBAL_DEFINES += GLOBAL_LDFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_LDFLAGS))\"
GLOBAL_DEFINES += ARCH_COMPILEFLAGS=\"$(subst $(SPACE),_,$(ARCH_COMPILEFLAGS))\"
GLOBAL_DEFINES += ARCH_CFLAGS=\"$(subst $(SPACE),_,$(ARCH_CFLAGS))\"
GLOBAL_DEFINES += ARCH_CPPFLAGS=\"$(subst $(SPACE),_,$(ARCH_CPPFLAGS))\"
GLOBAL_DEFINES += ARCH_ASMFLAGS=\"$(subst $(SPACE),_,$(ARCH_ASMFLAGS))\"
GLOBAL_DEFINES += ARCH_LDFLAGS=\"$(subst $(SPACE),_,$(ARCH_LDFLAGS))\"
GLOBAL_DEFINES += TOOLCHAIN_PREFIX=\"$(subst $(SPACE),_,$(TOOLCHAIN_PREFIX))\"

ifneq ($(OBJS),)
$(warning OBJS=$(OBJS))
$(error OBJS is not empty, please convert to new module format)
endif
ifneq ($(OPTFLAGS),)
$(warning OPTFLAGS=$(OPTFLAGS))
$(error OPTFLAGS is not empty, please use GLOBAL_OPTFLAGS or MODULE_OPTFLAGS)
endif
ifneq ($(CFLAGS),)
$(warning CFLAGS=$(CFLAGS))
$(error CFLAGS is not empty, please use GLOBAL_CFLAGS or MODULE_CFLAGS)
endif
ifneq ($(CPPFLAGS),)
$(warning CPPFLAGS=$(CPPFLAGS))
$(error CPPFLAGS is not empty, please use GLOBAL_CPPFLAGS or MODULE_CPPFLAGS)
endif

$(info LIBGCC = $(LIBGCC))
$(info GLOBAL_COMPILEFLAGS = $(GLOBAL_COMPILEFLAGS))
$(info GLOBAL_OPTFLAGS = $(GLOBAL_OPTFLAGS))

# kconfig
PYTHONCMD ?= python
KCONFIGCMD := PYTHONPATH=$(LKROOT)/tools/Kconfiglib:$$PYTHONPATH $(PYTHONCMD) -B
GEN_KCONFIG_DIR := $(BUILDDIR)/generated/kconfig
GEN_KCONFIG_HEADER := $(GEN_KCONFIG_DIR)/kconfig/genconfig.h
PLAT_DEFCONFIG  := $(PLATFORM)-$(ARCH)_defconfig

.PHONY: menuconfig genconfig defconfig
# 'make menuconfig' is a special rule that generate config_header
menuconfig:
	@$(KCONFIGCMD) $(LKROOT)/tools/Kconfiglib/menuconfig.py
genconfig:
	@mkdir -p `dirname  $(GEN_KCONFIG_HEADER)`
	@$(KCONFIGCMD) $(LKROOT)/tools/Kconfiglib/genconfig.py --header-path $(GEN_KCONFIG_HEADER)
defconfig:
	@$(KCONFIGCMD) $(LKROOT)/tools/Kconfiglib/defconfig.py $(LKROOT)/arch/$(ARCH)/configs/$(PLAT_DEFCONFIG)

# generate a dtb
all-dtbs:=
dtb-y	:=
subdir-y:=

DTS_FILES_DIR := $(LKROOT)/arch/$(ARCH)/boot/dts
DTB_FILES_DIR := $(BUILDDIR)/generated/dtb
DTS_FILES_INCLUDE  := 
DTBINDINGS_TOP_DIR := tools/dts
DTBINDINGS_SUB_DIR := $(shell find $(DTBINDINGS_TOP_DIR) -type d | sort | uniq)
DTBINDINGS_INCLUDE := $(addprefix -I,$(DTBINDINGS_SUB_DIR))
DTBINDINGS_INCLUDE_DTC := $(addprefix -i,$(DTBINDINGS_SUB_DIR))

define dtb-target
$(eval dtb_tmp:=$(strip $(2))/$(strip $(1)))
all-dtbs += $(dtb_tmp)
DTS_FILES_INCLUDE := $(addprefix -I,$(2))
DTS_FILES_INCLUDE_DTC := $(addprefix -i,$(2))
endef

define dtb-dir-include
$(eval include $1/Makefile)
$(foreach dtb, $(dtb-y), $(eval $(call dtb-target, $(dtb), $1)))
endef

include $(DTS_FILES_DIR)/Makefile

ifneq ($(subdir-y),)
$(foreach dir, $(subdir-y), $(eval $(call dtb-dir-include, $(strip $(DTS_FILES_DIR))/$(strip $(dir)))))
else
$(foreach dtb, $(dtb-y), $(eval $(call dtb-target, $(dtb), $(DTS_FILES_DIR))))
endif

DTC := dtc
DTB_FILES := $(all-dtbs)
DTS_FILES := $(patsubst %.dtb,%.dts,$(DTB_FILES))
DTC_INCLUDES := $(DTS_FILES_INCLUDE_DTC)
DTC_FLAGS := -b 0
DTC_CPP_FLAGS := -x assembler-with-cpp -nostdinc \
                 $(DTBINDINGS_INCLUDE)        \
                 $(DTS_FILES_INCLUDE) 	 \
                 -undef -D__DTS__ 

EXTRA_BUILDDEPS += $(DTB_FILES)

$(info DTB_FILES=$(DTB_FILES))
$(info DTS_FILES=$(DTS_FILES))

dtbs: $(DTB_FILES)
# 先做dts中<#include>的预处理CPP，再做DTC转换DTS
# ITB与FIP类似，是为了引导过程所设置的镜像包格式，ITB适用于UBOOT，FIP适用于ATF
%.dtb: %.dts
	@echo "Compiling $< into $@"
	@mkdir -p $(DTB_FILES_DIR)
	cpp $(DTC_CPP_FLAGS) < $< | $(DTC) $(DTC_INCLUDES) -I dts -O dtb $(DTC_FLAGS) -o $@ -
	@cp $@ $(DTB_FILES_DIR)
	@rm -rf $@
.PHONY: dtbs

# make all object files depend on any targets in GLOBAL_SRCDEPS
$(ALLOBJS): $(GLOBAL_SRCDEPS)

# any extra top level build dependencies that someone declared.
# build.mk may add to EXTRA_BUILDDEPS, this must be evalauted after build.mk.
all:: $(EXTRA_BUILDDEPS)

clean: $(EXTRA_CLEANDEPS)
	rm -f $(ALLOBJS) $(DEPS) $(GENERATED) $(OUTBIN) $(OUTELF) $(OUTELF).lst $(OUTELF).debug.lst $(OUTELF).sym $(OUTELF).sym.sorted $(OUTELF).size $(OUTELF).hex $(OUTELF).dump

install: all
	scp $(OUTBIN) 192.168.0.4:/tftpboot

list-arch:
	@echo ARCH = ${ARCH}

list-toolchain:
	@echo TOOLCHAIN_PREFIX = ${TOOLCHAIN_PREFIX}

.PHONY: all clean install list-arch list-toolchain

# generate a config.h file with all of the GLOBAL_DEFINES laid out in #define format
configheader:

$(CONFIGHEADER): configheader
	@$(call MAKECONFIGHEADER,$@,GLOBAL_DEFINES)

.PHONY: configheader

# Empty rule for the .d files. The above rules will build .d files as a side
# effect. Only works on gcc 3.x and above, however.
%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif

endif # do-nothing = 1
endif # make spotless
