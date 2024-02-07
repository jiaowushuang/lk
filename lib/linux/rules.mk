LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_OPTIONS := extra_warnings

MODULE_SRCS += \
   $(LOCAL_DIR)/atomic64.c \
   $(LOCAL_DIR)/bitmap.c \
   $(LOCAL_DIR)/find_bit.c \
   $(LOCAL_DIR)/kfifo.c \
   $(LOCAL_DIR)/klist.c \
   $(LOCAL_DIR)/list_sort.c \
   $(LOCAL_DIR)/llist.c \
   $(LOCAL_DIR)/plist.c \
   $(LOCAL_DIR)/radix-tree.c \
   $(LOCAL_DIR)/rbtree.c \
   $(LOCAL_DIR)/ring_buffer.c \
   $(LOCAL_DIR)/scatterlist.c \
   $(LOCAL_DIR)/xarray.c \
   $(LOCAL_DIR)/iov_iter.c \
   $(LOCAL_DIR)/clz_ctz.c \
   $(LOCAL_DIR)/percpu_counter.c \
   $(LOCAL_DIR)/percpu-refcount.c \
   $(LOCAL_DIR)/refcount.c \
   $(LOCAL_DIR)/rhashtable.c \
   $(LOCAL_DIR)/generic-radix-tree.c \
   $(LOCAL_DIR)/lockref.c \
   $(LOCAL_DIR)/sg_pool.c \
   $(LOCAL_DIR)/sg_split.c \

ifneq ($(ENHANCED_SYNC_LOCK_ENABLED),)
MODULE_SRCS += \
   $(LOCAL_DIR)/mutex.c \
   $(LOCAL_DIR)/semaphore.c \
   $(LOCAL_DIR)/rwsem.c \
   $(LOCAL_DIR)/percpu-rwsem.c \
   $(LOCAL_DIR)/futex/core.c \
   $(LOCAL_DIR)/rcu/update.o \
   $(LOCAL_DIR)/rcu/sync.o \
   
ifneq ($(CONFIG_TREE_SRCU),)
MODULE_SRCS += \
	$(LOCAL_DIR)/rcu/srcutree.o
endif
ifneq ($(CONFIG_TINY_SRCU),)
MODULE_SRCS += \
	$(LOCAL_DIR)/rcu/srcutiny.o
endif
ifneq ($(CONFIG_RCU_TORTURE_TEST),)
MODULE_SRCS += \
	$(LOCAL_DIR)/rcu/rcutorture.o
endif
ifneq ($(CONFIG_RCU_SCALE_TEST),)
MODULE_SRCS += \
	$(LOCAL_DIR)/rcu/rcuscale.o
endif
ifneq ($(CONFIG_RCU_REF_SCALE_TEST),)
MODULE_SRCS += \
	$(LOCAL_DIR)/rcu/refscale.o
endif
ifneq ($(CONFIG_TREE_RCU),)
MODULE_SRCS += \
	$(LOCAL_DIR)/rcu/tree.o
endif
ifneq ($(CONFIG_TINY_RCU),)
MODULE_SRCS += \
	$(LOCAL_DIR)/rcu/tiny.o
endif
ifneq ($(CONFIG_RCU_NEED_SEGCBLIST),)
MODULE_SRCS += \
	$(LOCAL_DIR)/rcu/rcu_segcblist.o
endif

ifneq ($(CONFIG_DEBUG_IRQFLAGS),)
MODULE_SRCS += \
	$(LOCAL_DIR)/irqflag-debug.c \
endif
ifneq ($(CONFIG_DEBUG_MUTEXES),)
MODULE_SRCS += \
	$(LOCAL_DIR)/mutex-debug.c \
endif
ifneq ($(CONFIG_LOCKDEP),)
MODULE_SRCS += \
	$(LOCAL_DIR)/lockdep.c \
ifneq ($(CONFIG_PROC_FS),)
MODULE_SRCS += \
	$(LOCAL_DIR)/lockdep_proc.c \
endif
endif

ifneq ($(CONFIG_SMP),)
MODULE_SRCS += \
	$(LOCAL_DIR)/spinlock.c \
endif
ifneq ($(CONFIG_LOCK_SPIN_ON_OWNER),)
MODULE_SRCS += \
	$(LOCAL_DIR)/osq_lock.c \
endif
ifneq ($(CONFIG_PROVE_LOCKING),)
MODULE_SRCS += \
	$(LOCAL_DIR)/spinlock.c \
endif
ifneq ($(CONFIG_QUEUED_SPINLOCKS),)
MODULE_SRCS += \
	$(LOCAL_DIR)/qspinlock.c \
endif
ifneq ($(CONFIG_RT_MUTEXES),)
MODULE_SRCS += \
	$(LOCAL_DIR)/rtmutex_api.c \
endif
ifneq ($(CONFIG_PREEMPT_RT),)
MODULE_SRCS += \
	$(LOCAL_DIR)/spinlock_rt.c \
	$(LOCAL_DIR)/ww_rt_mutex.c \
endif
ifneq ($(CONFIG_DEBUG_SPINLOCK),)
MODULE_SRCS += \
	$(LOCAL_DIR)/spinlock.c \
endif
ifneq ($(CONFIG_DEBUG_SPINLOCK),)
MODULE_SRCS += \
	$(LOCAL_DIR)/spinlock_debug.c \
endif
ifneq ($(CONFIG_QUEUED_RWLOCKS),)
MODULE_SRCS += \
	$(LOCAL_DIR)/qrwlock.c \
endif
ifneq ($(CONFIG_LOCK_TORTURE_TEST),)
MODULE_SRCS += \
	$(LOCAL_DIR)/locktorture.c \
endif
ifneq ($(CONFIG_WW_MUTEX_SELFTEST),)
MODULE_SRCS += \
	$(LOCAL_DIR)/test-ww_mutex.c \
endif
ifneq ($(CONFIG_LOCK_EVENT_COUNTS),)
MODULE_SRCS += \
	$(LOCAL_DIR)/lock_events.c \
endif



endif

include make/module.mk
