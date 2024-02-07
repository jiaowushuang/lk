/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#if defined(ARM_CPU_CORTEX_M0_PLUS) || \
	defined(ARM_CPU_CORTEX_M3) || \
	defined(ARM_CPU_CORTEX_M4) || \
	defined(ARM_CPU_CORTEX_M7) || \
	defined(ARM_ISA_ARMV7R)
#include <mach/mpu/arm_mpu_v7m.h>
#elif defined(ARM_CPU_CORTEX_M23) || \
	defined(ARM_CPU_CORTEX_M33) || \
	defined(ARM_CPU_CORTEX_M55) || \
	defined(ARM_ISA_ARMV8R) 
#include <mach/mpu/arm_mpu_v8.h>
#else
#error "Unsupported ARM CPU"
#endif

#ifndef ASSEMBLY

/* Region definition data structure */
struct arm_mpu_region {
	/* Region Base Address */
	uint32_t base;
	/* Region Name */
	const char *name;

	/* Region Size */
	uint32_t size;

	/* Region Attributes */
	arm_mpu_region_attr_t attr;
};

/* MPU configuration data structure */
struct arm_mpu_config {
	/* Number of regions */
	uint32_t num_regions;
	/* Regions */
	const struct arm_mpu_region *mpu_regions;
};


#define MPU_REGION_ENTRY(_name, _base, _size, _attr) \
	{\
		.name = _name, \
		.base = _base, \
		.size = _size, \
		.attr = _attr, \
	}
/* Reference to the MPU configuration.
 *
 * This struct is defined and populated for each SoC (in the SoC definition),
 * and holds the build-time configuration information for the fixed MPU
 * regions enabled during kernel initialization. Dynamic MPU regions (e.g.
 * for Thread Stack, Stack Guards, etc.) are programmed during runtime, thus,
 * not kept here.
 */
#define KERNEL_MAX_REGIONS 8
extern const struct arm_mpu_config mpu_config;

#if defined(CONFIG_MULTIPARTITIONING)
extern int arch_mem_domain_max_partitions_get(void);
extern int arch_buffer_validate(void *addr, size_t size, int write);
#endif

#endif /* ASSEMBLY */


