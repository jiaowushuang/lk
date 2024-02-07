/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2019 Lexmark International, Inc.
 */

/**
 *  Get the number of supported MPU regions.
 */

/* armv7 only */

#include <arch/spinlock.h>
static inline uint8_t get_num_regions(void)
{
#if defined(ARM_CPU_CORTEX_M0PLUS) || \
	defined(ARM_CPU_CORTEX_M3) || \
	defined(ARM_CPU_CORTEX_M4)
	/* Cortex-M0+, Cortex-M3, and Cortex-M4 MCUs may
	 * have a fixed number of 8 MPU regions.
	 */
	return 8;
#elif defined(NUM_MPU_REGIONS)
	/* Retrieve the number of regions from DTS configuration. */
	return NUM_MPU_REGIONS;
#else
	uint32_t type = MPU->TYPE;

	type = (type & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;

	return (uint8_t)type;
#endif /* CPU_CORTEX_M0PLUS | CPU_CORTEX_M3 | CPU_CORTEX_M4 */
}

static inline void set_region_number(uint32_t index)
{
	MPU->RNR = index;
}

static inline uint32_t mpu_region_get_base(uint32_t index)
{
	MPU->RNR = index;
	return MPU->RBAR & MPU_RBAR_ADDR_Msk;
}

/**
 * This internal function converts the SIZE field value of MPU_RASR
 * to the region size (in bytes).
 */
static inline uint32_t mpu_rasr_size_to_size(uint32_t rasr_size)
{
	return 1 << (rasr_size + 1U);
}

/**
 * This internal function checks if region is enabled or not.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int is_enabled_region(uint32_t index)
{
	/* Lock IRQs to ensure RNR value is correct when reading RASR. */
	uint32_t rasr;
    	spin_lock_saved_state_t state;
    	arch_interrupt_save(&state, SPIN_LOCK_FLAG_INTERRUPTS);

	MPU->RNR = index;
	rasr = MPU->RASR;

    	arch_interrupt_restore(state, SPIN_LOCK_FLAG_INTERRUPTS);
	return (rasr & MPU_RASR_ENABLE_Msk) ? 1 : 0;
}

/**
 * This internal function returns the access permissions of an MPU region
 * specified by its region index.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline uint32_t get_region_ap(uint32_t r_index)
{
	/* Lock IRQs to ensure RNR value is correct when reading RASR. */
	uint32_t rasr;
    	spin_lock_saved_state_t state;

    	arch_interrupt_save(&state, SPIN_LOCK_FLAG_INTERRUPTS);
	MPU->RNR = r_index;
	rasr = MPU->RASR;
    	arch_interrupt_restore(state, SPIN_LOCK_FLAG_INTERRUPTS);

	return (rasr & MPU_RASR_AP_Msk) >> MPU_RASR_AP_Pos;
}

/**
 * This internal function checks if the given buffer is in the region.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int is_in_region(uint32_t r_index, uint32_t start, uint32_t size)
{
	uint32_t r_addr_start;
	uint32_t r_size_lshift;
	uint32_t r_addr_end;
	uint32_t end;

	/* Lock IRQs to ensure RNR value is correct when reading RBAR, RASR. */
	uint32_t rbar, rasr;
    	spin_lock_saved_state_t state;

    	arch_interrupt_save(&state, SPIN_LOCK_FLAG_INTERRUPTS);

	MPU->RNR = r_index;
	rbar = MPU->RBAR;
	rasr = MPU->RASR;
    	arch_interrupt_restore(state, SPIN_LOCK_FLAG_INTERRUPTS);

	r_addr_start = rbar & MPU_RBAR_ADDR_Msk;
	r_size_lshift = ((rasr & MPU_RASR_SIZE_Msk) >>
			MPU_RASR_SIZE_Pos) + 1U;
	r_addr_end = r_addr_start + (1UL << r_size_lshift) - 1UL;

	size = size == 0U ? 0U : size - 1U;
	if (__builtin_add_overflow(start, size, &end)) {
		return 0;
	}

	if ((start >= r_addr_start) && (end <= r_addr_end)) {
		return 1;
	}

	return 0;
}

static inline uint32_t mpu_region_get_size(uint32_t index)
{
	MPU->RNR = index;
	uint32_t rasr_size =
		(MPU->RASR & MPU_RASR_SIZE_Msk) >> MPU_RASR_SIZE_Pos;

	return mpu_rasr_size_to_size(rasr_size);
}
