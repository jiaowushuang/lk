mpu - flash and sram region

1. static
        - 2 flash/sram physical address(total)
        - kernel
        - P(RWX),U(RWX)
        - P(RWX),U(NA)
2. dynamic
        - N flash/sram physical address(part, non-overlap)
        - partition
                - rtos, kthread
                        - P(RWX),U(RWX)
                        NOTE: if rtos has MPU enabled, and rtos kernel needs priv regions,
                        so, rtos has P(RWX),U(NA)
                - task/app, uthread
                        - P(RWX),U(RWX)
        - code
                - flash
        - data,bss,stack .etc
                - sram
3. not support virtual address, so application address is non-overlapable