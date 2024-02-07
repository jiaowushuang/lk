MODULES += \

MODULES += \
	root/services/partition \
	root/services/shell

include project/virtual/fs.mk
# include project/virtual/test.mk
include project/target/lm3s6965evb.mk
