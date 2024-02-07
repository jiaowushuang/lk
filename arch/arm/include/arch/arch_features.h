/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdbool.h>

#include <arch/arch_helpers.h>

static inline bool is_armv7_gentimer_present(void)
{
	return ((read_id_pfr1() >> ID_PFR1_GENTIMER_SHIFT) &
		ID_PFR1_GENTIMER_MASK) != 0U;
}

static inline bool is_armv8_2_ttcnp_present(void)
{
	return ((read_id_mmfr4() >> ID_MMFR4_CNP_SHIFT) &
		ID_MMFR4_CNP_MASK) != 0U;
}


