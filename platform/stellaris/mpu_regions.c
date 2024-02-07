/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(ARCH_HAS_MPU)
#include <mach/mpu/arm_mpu.h>
#include <kern/utils.h>
#include <mach/mpu/arm_mpu_mem_cfg.h>
#include "ti_driverlib.h"
static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 ROMBASE,
			ROMSIZE,
#if defined(ARM_ISA_ARMV8M)
			 REGION_FLASH_ATTR(ROMBASE, \
				 ROMSIZE)),
#else
			 REGION_FLASH_ATTR(REGION_ROM_SIZE)),
#endif
	/* Region 1 */
	MPU_REGION_ENTRY("SRAM_0",
			 MEMBASE,
			MEMSIZE,
#if defined(ARM_ISA_ARMV8M)
			 REGION_RAM_ATTR(MEMBASE, \
				 MEMSIZE)),
#else
			 REGION_RAM_ATTR(REGION_SRAM_SIZE)),
#endif
	/* Region 2 */
	MPU_REGION_ENTRY("Peripheral_0",
			 PERIPHERAL_BASE,
			PERIPHERAL_SIZE,
#if defined(ARM_ISA_ARMV8M)
			 REGION_IO_ATTR(PERIPHERAL_BASE, \
				 PERIPHERAL_SIZE)),
#else
			 REGION_IO_ATTR(REGION_32M)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
#endif /* ARCH_HAS_MPU */