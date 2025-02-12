/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kern/err.h>
#include <kern/debug.h>
#include <dev/uart.h>
#include <platform.h>
#include <platform/stm32.h>
#include <arch/arm/cm.h>
#include <stm32f4xx_rcc.h>
#include "system_stm32f4xx.h"

void platform_early_init(void) {
    // Crank up the clock before initing timers.
    SystemInit();

    // start the systick timer
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);
    arm_cm_systick_init(clocks.SYSCLK_Frequency);

    stm32_timer_early_init();
    stm32_gpio_early_init();
}

void platform_init(void) {
    stm32_timer_init();
}
