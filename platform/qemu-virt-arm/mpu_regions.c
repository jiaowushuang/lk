/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(ARCH_HAS_MPU)
#include <mach/mpu/arm_mpu.h>
#include <kern/utils.h>
#include <platform/qemu-virt.h>

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("SRAM_0",
			 MEMBASE,
			 MEMSIZE,
			 REGION_RAM_TEXT_WIDE_ATTR(MEMBASE+MEMSIZE)
			 ),

	/* Region 2 */
	MPU_REGION_ENTRY("Peripheral_0",
			 PERIPHERAL_BASE_PHYS,
			 PERIPHERAL_BASE_SIZE,
			 REGION_DEVICE_ATTR(PERIPHERAL_BASE_PHYS+PERIPHERAL_BASE_SIZE)
			 ),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
#endif /* ARCH_HAS_MPU */