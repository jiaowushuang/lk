include project/target/stm32f746g-disco.mk
#include project/virtual/test.mk
#include project/virtual/minip.mk

#MODULES += root/services/loader

include project/virtual/fs.mk

HEAP_IMPLEMENTATION=cmpctmalloc

# Console serial port is on pins PA9(TX) and PB7(RX)
