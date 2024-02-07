# routines and rules to print some helpful stuff


#$(warning MAKECMDGOALS = $(MAKECMDGOALS))

# print some help and exit
ifeq ($(firstword $(MAKECMDGOALS)),help)
do-nothing=1

.PHONY: help
help:
	@echo "OLDK build system quick help"
	@echo "Individual projects are built into a build-<project> directory"
	@echo "Output binary is located at build-<project>/kern.bin"
	@echo "Environment or command line variables controlling build:"
	@echo "PROJECT = <project name>"
	@echo "TOOLCHAIN_PREFIX = <absolute path to toolchain or relative path with prefix>"
	@echo ""
	@echo "Special make targets:"
	@echo "make help: This help"
	@echo "make list: List of buildable projects"
	@echo "make clean: cleans build of current project"
	@echo "make spotless: removes all build directories"
	@echo "make <project>: try to build project named <project>"
	@echo ""
	@echo "make list-arch: print the architecture of the current project"
	@echo "make list-toolchain: print the computed toolchain prefix of the current project"
	@echo ""
	@echo "Examples:"
	@echo "PROJECT=testproject make"
	@echo "PROJECT=testproject make clean"
	@echo "make testproject"
	@echo "make testproject clean"
	@echo ""
	@echo "output will be in build-testproject/"
	@echo "KCONFIG Examples:"
	@echo "make DEFAULT_PROJECT=qemu-virt-arm64-test defconfig"
	@echo "make DEFAULT_PROJECT=qemu-virt-arm64-test genconfig"
	@echo "make DEFAULT_PROJECT=qemu-virt-arm64-test menuconfig"
	@echo "make DEFAULT_PROJECT=qemu-virt-arm64-test dtbs"
endif

# list projects
ifeq ($(firstword $(MAKECMDGOALS)),list)
do-nothing=1

# get a list of all the .mk files in the top level project directories
PROJECTS:=$(basename $(strip $(foreach d,$(LKINC),$(wildcard $(d)/project/*.mk))))
PROJECTS:=$(shell basename -a $(PROJECTS))

.PHONY: list
list:
	@echo 'List of all buildable projects: (look in project/ directory)'; \
	for p in $(sort $(PROJECTS)); do \
		echo $$p; \
	done

endif

# vim: set syntax=make noexpandtab
