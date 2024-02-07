include project/target/stm32746g-eval2.mk
include project/virtual/test.mk
include project/virtual/minip.mk

MODULES += \
    root/services/loader

include project/virtual/fs.mk

HEAP_IMPLEMENTATION=cmpctmalloc
