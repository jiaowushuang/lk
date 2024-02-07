/*
 * Copyright (c) 2013-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <kern/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

/*
 * OLDK's init system
 */

typedef void (*init_hook)(uint level);

enum bootstrap_init_level {
    INIT_LEVEL_EARLIEST = 1,

    INIT_LEVEL_ARCH_EARLY     = 0x10000,
    INIT_LEVEL_PLATFORM_EARLY = 0x20000,
    INIT_LEVEL_TARGET_EARLY   = 0x30000,
    INIT_LEVEL_HEAP           = 0x40000,
    INIT_LEVEL_VM             = 0x50000,
    INIT_LEVEL_KERNEL         = 0x60000,
    INIT_LEVEL_THREADING      = 0x70000,
    INIT_LEVEL_ARCH           = 0x80000,
    INIT_LEVEL_PLATFORM       = 0x90000,
    INIT_LEVEL_TARGET         = 0xa0000,
    INIT_LEVEL_APPS           = 0xb0000,

    INIT_LEVEL_LAST = UINT_MAX,
};

enum bootstrap_init_flags {
    INIT_FLAG_PRIMARY_CPU     = 0x1,
    INIT_FLAG_SECONDARY_CPUS  = 0x2,
    INIT_FLAG_ALL_CPUS        = INIT_FLAG_PRIMARY_CPU | INIT_FLAG_SECONDARY_CPUS,
    INIT_FLAG_CPU_SUSPEND     = 0x4,
    INIT_FLAG_CPU_RESUME      = 0x8,
};

void bootstrap_init_level(enum bootstrap_init_flags flags, uint start_level, uint stop_level);

static inline void bootstrap_primary_cpu_init_level(uint start_level, uint stop_level) {
    bootstrap_init_level(INIT_FLAG_PRIMARY_CPU, start_level, stop_level);
}

static inline void bootstrap_init_level_all(enum bootstrap_init_flags flags) {
    bootstrap_init_level(flags, INIT_LEVEL_EARLIEST, INIT_LEVEL_LAST);
}

struct bootstrap_init_struct {
    uint level;
    uint flags;
    init_hook hook;
    const char *name;
};

#define INIT_HOOK_FLAGS(_name, _hook, _level, _flags) \
    const struct bootstrap_init_struct _init_struct_##_name __ALIGNED(sizeof(void *)) __SECTION("bootstrap_init") = { \
        .level = _level, \
        .flags = _flags, \
        .hook = _hook, \
        .name = #_name, \
    };

#define INIT_HOOK(_name, _hook, _level) \
    INIT_HOOK_FLAGS(_name, _hook, _level, INIT_FLAG_PRIMARY_CPU)

__END_CDECLS
