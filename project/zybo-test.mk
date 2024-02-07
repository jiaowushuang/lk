# top level project rules for the zybo-test project
#
MODULES += \
	root/services/inetsrv \
	root/services/shell \
	root/services/lkboot \
	dev/gpio \

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

include project/target/zybo.mk
include project/virtual/test.mk

