/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <kern/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

void arch_early_init(void);
void arch_init(void);
void arch_mpu_init(void);
void arch_quiesce(void);
void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) __NO_RETURN;
void arch_enter_uspace(vaddr_t entry_point, vaddr_t user_stack_top, vaddr_t kernel_stack_top, vaddr_t arg0) __NO_RETURN;
void arch_set_user_tls(vaddr_t tls_ptr);
/* Holds a vaddr large enough for 32 or 64 bit clients */
typedef uint64_t ext_vaddr_t;
#ifdef USER_32BIT
typedef uint32_t user_addr_t;
typedef uint32_t user_size_t;
#else
typedef uint64_t user_addr_t;
typedef uint64_t user_size_t;
#endif

status_t arch_copy_from_user(void *kdest, user_addr_t usrc, user_size_t len);
status_t arch_copy_to_user(user_addr_t udest, const void *ksrc, user_size_t len);
ssize_t arch_strlcpy_from_user(char *kdst, user_addr_t usrc, user_size_t len);
__END_CDECLS

/* arch specific bits */
#include <arch/defines.h>
#ifndef ARM_ONLY_THUMB
#include <arch/arch_helpers.h>
#endif
