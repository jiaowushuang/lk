/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2020-2023 Google LLC.
 */

#pragma once

#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof(*(_a)))
#define PASTE(a, b) a##b
/* Register size of the current architecture. */
#ifdef __KERNEL_64__
#define REGSZ U(8)
#else
#define REGSZ U(4)
#endif

#if defined(__ASSEMBLER__) || defined(__LINKER__)
#define UL_CONST(x) x
#define ULL_CONST(x) x
#define U(_x) (_x)
#define UL(_x) (_x)
#define ULL(_x) (_x)
#define L(_x) (_x)
#define LL(_x) (_x)
#define GENMASK_32(h, l) (((0xFFFFFFFF) << (l)) & (0xFFFFFFFF >> (32 - 1 - (h))))
#define GENMASK_64(h, l) ((~0 << (l)) & (~0 >> (64 - 1 - (h))))

#else

#define UL_CONST(x) PASTE(x, ul)
#define ULL_CONST(x) PASTE(x, llu)
#define U_(_x) (_x##U)
#define U(_x) U_(_x)
#define UL_(_x) (_x##UL)
#define UL(_x) UL_(_x)
#define ULL_(_x) (_x##ULL)
#define ULL(_x) ULL_(_x)
#define L_(_x) (_x##L)
#define L(_x) L_(_x)
#define LL_(_x) (_x##LL)
#define LL(_x) LL_(_x)
#define GENMASK_32(h, l) (((~UINT32_C(0)) << (l)) & (~UINT32_C(0) >> (32 - 1 - (h))))
#define GENMASK_64(h, l) (((~UINT64_C(0)) << (l)) & (~UINT64_C(0) >> (64 - 1 - (h))))
#endif /* __ASSEMBLER__ || __LINKER__ */

#define BIT_32(nr) (U(1) << (nr))
#define BIT_64(nr) (ULL(1) << (nr))

/* Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 */

#define __stringify_1(x...) #x
#define __stringify(x...)   __stringify_1(x)

#ifdef __KERNEL_64__
#define GENMASK GENMASK_64
#else
#define GENMASK GENMASK_32
#endif

#define KB (1024UL)
#define MB (1024UL * 1024UL)
#define GB (1024UL * 1024UL * 1024UL)
#define DIV_ROUND_UP(n, d) (((n) + (d)-UL(1)) / (d))
#define ARG_UNUSED(x) (void)(x)

#define IS_ENABLED(config_macro) Z_IS_ENABLED1(config_macro)
#define Z_IS_ENABLED1(config_macro) Z_IS_ENABLED2(_XXXX##config_macro)
#define Z_IS_ENABLED2(one_or_two_args) Z_IS_ENABLED3(one_or_two_args 1, 0)
#define Z_IS_ENABLED3(ignore_this, val, ...) val

#define RETURN_IF_ERROR(expr) \
        do { \
            status_t err_ = (expr); \
            if (err_ != NO_ERROR) { \
                return err_; \
            } \
        } while (0)
